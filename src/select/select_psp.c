/*
 * select: CONNECT, the link with MONSTER HUNTER PORTABLE on a PSP over USB
 * (USBPSPCM.IRX, the usbpspcm: device, a thread of its own)
 * (0x6C2DB0-0x6C7D80), the last unit with code. The file owns its .data
 * (0x6CC1E0-0x6CD7F8), .rodata (0x6D0720-0x6D0980) and .bss
 * (0x6D21A8-0x6D3280).
 */
#include "common.h"
#include "window.h"
#include "syssettings.h"
extern u16 gFrameTick; /* sub_main: the frame counter */
#include "pad.h"
#include "m2c_macros.h"
#include "select.h"
#include "syssave.h"
#include "save.h"

/* An animation's start (gPspObjAnims points to them). */
typedef struct PspAnim {
    u8 unk0;
    s8 unk1;                /* 0x01: 1 at a loop point */
    s8 next;                /* 0x02: the next frame (< 0: the end) */
    s8 se;                  /* 0x03: a sound (1 + its number) */
    s16 unk4;               /* 0x04 */
    s16 ox;                 /* 0x06: drawing offset */
    s16 oy;                 /* 0x08 */
    s16 dx;                 /* 0x0A */
    s16 dy;                 /* 0x0C */
    s16 tex;                /* 0x0E: file (gPspWork->unk100) */
    s16 cell;               /* 0x10 */
    u8 unk12[2];
    s16 *cells;             /* 0x14: u, v, texture w, h, screen w, h */
} PspAnim;

/* A moving picture of the CONNECT screens. */
typedef struct PspObj {
    s32 active;             /* 0x00 */
    s32 unk4;               /* 0x04 */
    s32 unk8;               /* 0x08 */
    s32 flip;               /* 0x0C */
    s32 unk10;              /* 0x10 */
    s32 x;                  /* 0x14 */
    s32 y;                  /* 0x18 */
    s32 unk1C;              /* 0x1C */
    s32 unk20;              /* 0x20 */
    s32 unk24;              /* 0x24 */
    s32 type;               /* 0x28 */
    PspAnim *anim;          /* 0x2C */
} PspObj;

/* An actor of the CONNECT screens (32 of them in PspWork). */
typedef struct PspActor {
    s32 unk0;               /* 0x00 */
    s32 unk4;               /* 0x04 */
    s32 unk8;               /* 0x08 */
    s8 unkC;                /* 0x0C: state */
    u8 unkD[0x20 - 0xD];
    f32 unk20;              /* 0x20: speed */
    u8 unk24[0x30 - 0x24];
    f32 unk30;              /* 0x30: position */
    f32 unk34;              /* 0x34 */
    s32 unk38;              /* 0x38: mirrored horizontally */
    s32 unk3C;              /* 0x3C: mirrored vertically */
    s16 unk40;              /* 0x40: size */
    s16 unk42;              /* 0x42 */
    u8 unk44[4];
    s32 unk48;              /* 0x48 */
    f32 unk4C;              /* 0x4C: scale */
    f32 unk50;              /* 0x50 */
    s32 unk54;              /* 0x54: dimmed */
    void *unk58;            /* 0x58: its part (PspPart) */
    s32 unk5C;              /* 0x5C */
    void *unk60;            /* 0x60 */
} PspActor;

/* A part an actor draws (gPspActorAnims, 16 bytes each). */
typedef struct PspPart {
    s16 tex;                /* 0x0: file (gPspWork->unk100) */
    s8 kind;                /* 0x2: 0: a filled rectangle */
    s8 blend;               /* 0x3 */
    u32 color;              /* 0x4 */
    s16 u;                  /* 0x8: texture rectangle */
    s16 v;                  /* 0xA */
    s16 uw;                 /* 0xC */
    s16 vh;                 /* 0xE */
} PspPart;

/* A filled rectangle (Prim_FillRect). */
typedef struct PspRect {
    s16 x0, y0, x1, y1;     /* 0x0 */
    u32 color;              /* 0x8 */
} PspRect;

/* A textured triangle (Prim_DrawTexTri, main). */
typedef struct PspTri {
    s16 x0, y0, x1, y1, x2, y2;     /* 0x00 */
    u32 color;                      /* 0x0C */
    s16 u0, v0, u1, v1, u2, v2;     /* 0x10 */
} PspTri;

/* A screen of CONNECT's (Psp_UpdateActor's): its window rectangle. */
typedef struct PspView {
    u8 unk0[0x30];
    f32 x;                  /* 0x30 */
    f32 y;                  /* 0x34 */
    u8 unk38[8];
    s16 w;                  /* 0x40 */
    s16 h;                  /* 0x42 */
    u8 unk44[0x50];
    f32 x2;                 /* 0x94 */
    f32 y2;                 /* 0x98 */
    u8 unk9C[8];
    s16 w2;                 /* 0xA4 */
    s16 h2;                 /* 0xA6 */
} PspView;

/* CONNECT's work: 0x1490 bytes at 0x100 in the area gSysSettings.unk60
 * points to (gPspWork). */
typedef struct PspWork {
    s32 unk0;               /* 0x000 */
    s32 unk4;               /* 0x004 */
    u8 unk8[8];
    s32 unk10;              /* 0x010: USBPSPCM.IRX's module id */
    u8 unk14[8];
    s32 unk1C;              /* 0x01C */
    s32 unk20;              /* 0x020: the PSP's answer (devctl) */
    s32 *unk24;             /* 0x024 */
    s32 *unk28;             /* 0x028 */
    s32 *unk2C;             /* 0x02C */
    s32 unk30;              /* 0x030 */
    u8 unk34[0x40 - 0x34];
    s32 unk40;              /* 0x040: the features offered this time (bits) */
    s32 unk44;              /* 0x044 */
    u8 unk48[0x54 - 0x48];
    s32 unk54;              /* 0x054 */
    s32 unk58;              /* 0x058: the CONNECT step whose lines show */
    u8 unk5C[0xA4 - 0x5C];
    u8 unkA4[16];           /* 0x0A4: per feature: offered */
    u8 unkB4[16];           /* 0x0B4: per menu: what it shows */
    u8 unkC4[16];           /* 0x0C4: a copy of unkB4 */
    s32 unkD4;              /* 0x0D4: menus with something */
    s8 unkD8[6];            /* 0x0D8: per icon: the entry of its list found */
    s8 unkDE[6];            /* 0x0DE: per icon: one found */
    s8 unkE4;               /* 0x0E4: the last icon with one */
    s8 unkE5;               /* 0x0E5 */
    s8 unkE6;               /* 0x0E6 */
    s8 unkE7;               /* 0x0E7: its column in gPspCursorGrid */
    s8 unkE8;               /* 0x0E8: its row */
    u8 unkE9[0xFC - 0xE9];
    s32 unkFC;              /* 0x0FC */
    s32 unk100[13];         /* 0x100: loaded files */
    s32 unk134;             /* 0x134 */
    PspActor actors[32];    /* 0x138 */
    s32 unkDB8;             /* 0xDB8: actors in use */
    PspView *unkDBC[32];    /* 0xDBC: the screens */
    PspObj objs[32];        /* 0xE3C */
    s32 unk143C[6];         /* 0x143C */
    u16 unk1454[6];         /* 0x1454: the screens' swing phases */
    u16 unk1460[6];         /* 0x1460 */
    u16 unk146C[6];         /* 0x146C */
    s16 unk1478[6];         /* 0x1478: the screens' centres, x */
    s16 unk1484[6];         /* 0x1484: ... and y */
} PspWork;

/* Sony's kernel thread parameters */
struct ThreadParam {
    s32 status;
    void *entry;
    void *stack;
    s32 stackSize;
    void *gpReg;
    s32 initPriority;
    s32 currentPriority;
    u32 attr;
    u32 option;
    s32 waitType;
    s32 waitId;
    s32 wakeupCount;
};

s32 CreateThread(struct ThreadParam *);
s32 TerminateThread(s32);
s32 DeleteThread(s32);
s32 func_114C60(s32, void *);       /* StartThread */
s32 func_18CC10(void);
s32 Save_TestEventFlag(s32);
s32 Save_TestListBit(u16 item);
s32 LoadCompressedFile(u8 *dst, s32 id);
u32 Tex_CreateFromTim2(u8 *tim2, s32 mode);
extern s32 gOvlTex[];
void Psp_LoadFiles(s32 *out);
void Psp_ResetSceneActors(void);
void Psp_MarkIcons(void);
void Psp_UpdateObj(PspObj *o);
void Psp_UpdateActor(s32 i, PspActor *a);
typedef struct PspFileEnt {
    s32 offset;
    s32 size;
} PspFileEnt;
void Psp_InitActors(void);
void Psp_FreeTextures(void);
void Psp_Thread(void *);
extern char _gp;


s32 sceSifLoadStartModule(char *, s32, char *, s32 *);
s32 sceSifStopModule(s32 modid, s32 args, const char *argp, s32 *result); /* main's libsif: sceSifStopModule's arguments */
s32 sceSifUnloadModule(s32);
void Mem_Compact(void);
void Tex_FreeRange(s32 first, s32 count);

/* .bss, declared from the highest address down (MWCC emits uninitialized
 * variables last-declared first). */
u8 gPspThreadStack[0x1070];
u8 gPspDevctlArg[0x10];
u8 gPspDevPath[0x40];
s32 gPspState;
s32 gPspThreadId;
PspWork *gPspWork;

/* .data */
Frame gPspFrameA = {
#include "select/006CC1E0.inc"
};
Frame gPspFrameB = {
#include "select/006CC200.inc"
};
u32 D_006CC220[156] = {
#include "select/006CC220.inc"
};
u32 gPspObjAnims_0[300] = {
#include "select/006CC490.inc"
};
u32 gPspObjAnims_8[36] = {
#include "select/006CC940.inc"
};
u32 gPspObjAnims_9[8] = {
#include "select/006CC9D0.inc"
};
u32 D_006CC9F0[20] = {
#include "select/006CC9F0.inc"
};
u32 gPspObjAnims_1[60] = {
#include "select/006CCA40.inc"
};
u32 D_006CCB30[56] = {
#include "select/006CCB30.inc"
};
u32 gPspObjAnims_2[132] = {
#include "select/006CCC10.inc"
};
u32 gPspObjAnims_3[48] = {
#include "select/006CCE20.inc"
};
u32 D_006CCEE0[24] = {
#include "select/006CCEE0.inc"
};
u32 gPspObjAnims_4[36] = {
#include "select/006CCF40.inc"
};
u32 gPspObjAnims_5[36] = {
#include "select/006CCFD0.inc"
};
u32 D_006CD060[36] = {
#include "select/006CD060.inc"
};
u32 gPspObjAnims_6[84] = {
#include "select/006CD0F0.inc"
};
u32 D_006CD240[56] = {
#include "select/006CD240.inc"
};
u32 gPspObjAnims_7[108] = {
#include "select/006CD320.inc"
};
PspAnim *gPspObjAnims[12] = {
    (PspAnim *)gPspObjAnims_0, (PspAnim *)gPspObjAnims_1, (PspAnim *)gPspObjAnims_2, (PspAnim *)gPspObjAnims_3,
    (PspAnim *)gPspObjAnims_4, (PspAnim *)gPspObjAnims_5, (PspAnim *)gPspObjAnims_6, (PspAnim *)gPspObjAnims_7,
    (PspAnim *)gPspObjAnims_8, (PspAnim *)gPspObjAnims_9, NULL, NULL,
};
u32 gPspObjParams[28] = {
#include "select/006CD500.inc"
};
u32 gPspFrameRects[16] = {
#include "select/006CD570.inc"
};
u32 gPspFrameColors[8] = {
#include "select/006CD5B0.inc"
};
u32 gPspMarkKind[2] = {
#include "select/006CD5D0.inc"
};
u32 gPspIconIndex[2] = {
#include "select/006CD5D8.inc"
};
s8 gPspCursorGrid[4][4] = {
#include "select/006CD5E0.inc"
};
s8 gPspCursorCols[5][3] = {
#include "select/006CD5F0.inc"
};
s16 gPspStepTextPos[12][2] = {
#include "select/006CD600.inc"
};
s16 gPspLinePos[4][2] = {
#include "select/006CD630.inc"
};
u32 gPspActorAnims[48] = {
#include "select/006CD640.inc"
};
u32 gPspActorInit[20] = {
#include "select/006CD700.inc"
};
u32 gPspIconPos[6] = {
#include "select/006CD750.inc"
};
u32 gPspIconActors[2] = {
#include "select/006CD768.inc"
};
u32 gPspIconLists[8] = {
#include "select/006CD770.inc"
};
u32 gPspViewRects[24] = {
#include "select/006CD790.inc"
};
/* Psp_UpdateActor: the flying screens' speeds */
s16 gPspActorSpeeds[4] = {
#include "select/006CD7F0.inc"
};


void Psp_PlayDecideSe(void) {
    Sound_PlaySeFlat(0, 0x13, 0);
}

void Psp_PlayCancelSe(void) {
    Sound_PlaySeFlat(0, 0x14, 0);
}

void Psp_PlayMoveSe(void) {
    Sound_PlaySeFlat(7, 0x26, 0);
}

void Psp_PlayStopSe(void) {
    Sound_PlaySeFlat(7, 0x25, 0);
}

void Psp_PlayConfirmSe(void) {
    Sound_PlaySeFlat(1, 0x73, 0);
}

void Psp_ResetSlotAnim(s32 i) {
    gPspWork->unk143C[i] = 0x1800;
    gPspWork->unk1454[i] = 0;
    gPspWork->unk1460[i] = 0x4000;
    gPspWork->unk146C[i] = 0x800;
}

const char D_006D0720[10] = {
#include "select/006D0720.inc"
};

void SeqA_Reset(void);
void Chara_SelectTask(TaskSlot *t);
void Game_ResetSubsystems(void);
void Sound_LoadPspBank(void);
void Tex_LoadSystemPack(s32);
void Psp_Init(void);
void Psp_UpdateScene(void);
void Callbacks_Add(void (*fn)(void));
void Psp_Draw(void);
void Psp_DrawTexts(void);
u8 SeqA_GetState(void);
void Psp_Shutdown(PspWork *w);
void Psp_StartThread(PspWork *w);
void Psp_InitItemMask(void);
s32 sceDevctl(const char *, s32, void *, s32, void *, s32);  /* devctl */
void Psp_SelectFeatures(void);
void Stream_Reset(void);
void Stream_Play(s32, s32);
void Psp_SpawnObject(s32 type, s32 flip);
void Stream_FadeOut(s32 ch, s32 frames);
s32 Psp_AnyObjectActive(void);
void Psp_Release(void);
void Stream_Stop(s32 ch);
void Task_SetBootMode1(void);
/* unit's own declaration: with the definition's u8 (void) (sub_main/unk_3D54B0.c), Psp_ConnectTask: 8 of 959 words differ */
s32 NetMenu_Update(void);
void Gfx_DrawBlackLineFixed(void);
void Frame_Draw(void);
void Task_SetBootMode2(void);

/* CONNECT's task, every frame: waits for the character selection's end,
 * connects to the PSP (step 4, by gPspWork->unk0: the link, the features,
 * the objects sent and the icon grid), then leaves to the game (5) or
 * back (6, 8). */
/* Each column search is an empty-then if around a do-while that takes the
 * column's table row in r (the game's out-of-line preheader with the row
 * hoisted; a while reloads the row); the right search has its own row and
 * counter (j, n); the up/down searches and the loops after a left move
 * index the tables instead of walking them, and those loops count the
 * column in a variable of their own (i). */
void Psp_ConnectTask(TaskSlot *t) {
    s32 last;
    s32 c;
    s32 col;
    s32 row;
    s32 k;
    s32 i;
    s32 n;
    u16 trig;
    s32 j;
    s8 *r;

    trig = gPadState[0].pressed;
    switch (t->mode) {
    case 0:
        SeqA_Reset();
        gSysSettings.galleryPlay = 2;
        gSysSettings.pspCharaWait = 1;
        TaskSlot_Set(Chara_SelectTask, 4);
        t->mode++;
    case 1:
        if (gSysSettings.pspCharaWait != 0) {
            return;
        }
        Game_ResetSubsystems();
        Sound_LoadPspBank();
        Tex_LoadSystemPack(0);
        t->mode++;
    case 2:
        Psp_Init();
        t->mode++;
        SeqA_Start(2);
        break;
    case 3:
        if ((u8)SeqA_GetState() != 1) {
            gPspWork->unk58 = 6;
            if (trig & 0x40) {
                Psp_PlayCancelSe();
                Psp_Shutdown(gPspWork);
                t->mode = 9;
            } else if (trig & 0x20) {
                Psp_PlayDecideSe();
                Psp_StartThread(gPspWork);
                Psp_InitItemMask();
                t->mode++;
                gPspWork->unk30 = 0x1E;
            }
        }
        break;
    case 4:
        switch (gPspWork->unk0) {
        case 0:
            gPspWork->unk58 = 2;
            if (gPspState == 4
                || sceDevctl(D_006D0720, 0x55504301, NULL, 0, &gPspWork->unk20, 4) != 0
                || gPspWork->unk20 == 0) {
                if (trig & 0x40) {
                    Psp_PlayCancelSe();
                    Psp_Shutdown(gPspWork);
                    t->mode = 9;
                }
            } else {
                gPspWork->unk0++;
                gPspState = 1;
            }
            break;
        case 1:
            gPspWork->unk58 = 2;
            if (gPspState == 1) {
                if (trig & 0x40) {
                    Psp_PlayCancelSe();
                    gPspWork->unk58 = 8;
                    gPspWork->unk0 = 5;
                    gPspState = 0;
                    Psp_Shutdown(gPspWork);
                }
                break;
            }
            if (gPspState == 5) {
                gPspWork->unk58 = 7;
                gPspWork->unk0 = 5;
                gPspState = 0;
                Psp_Shutdown(gPspWork);
                break;
            }
            gPspWork->unk0++;
        case 2:
            gPspWork->unk0++;
        case 3:
            gPspWork->unk0++;
        case 4:
            gPspWork->unk58 = 4;
            Psp_Shutdown(gPspWork);
            Psp_SelectFeatures();
            gPspWork->unk0 = 10;
            gPspWork->unk30 = 1;
            break;
        case 5:
            if (trig & 0x20) {
                Psp_PlayDecideSe();
                Psp_Shutdown(gPspWork);
                t->mode = 3;
                gPspWork->unk44 = 0;
            }
            break;
        case 10:
            if (trig & 0x20) {
                Psp_PlayDecideSe();
                gPspWork->unk58 = 0;
                if (gPspWork->unkD4 == 0) {
                    SeqA_Start(0xB);
                    Stream_Reset();
                    t->mode = 6;
                } else {
                    Stream_Play(0, 0x2E);
                    gPspWork->unk0++;
                    gPspWork->unk4 = 0;
                    gPspWork->unk30 = 1;
                    gPspWork->unk54 = 0;
                }
            }
            break;
        case 11:
            switch (gPspWork->unk4) {
            case 0:
                gPspWork->unk58 = 9;
                if (--gPspWork->unk30 == 0) {
                    for (row = 0; row < 16; row++) {
                        if (gPspWork->unkC4[row] != 0) {
                            break;
                        }
                    }
                    if (row == 16) {
                        gPspWork->unkD4 = 0;
                        gPspWork->unk4++;
                        break;
                    }
                    Psp_SpawnObject(gPspWork->unkC4[row], 1);
                    gPspWork->unkC4[row] = 0;
                    gPspWork->unkD4--;
                    if (gPspWork->unkD4 == 0) {
                        gPspWork->unk4++;
                        break;
                    }
                    gPspWork->unk30 = 0x5A;
                }
                if (trig & 0x40) {
                    Psp_PlayCancelSe();
                    gPspWork->unk4++;
                    goto stop;
                }
                break;
            case 1:
                gPspWork->unk58 = 9;
                if (Psp_AnyObjectActive() == 0 || (trig & 0x40)) {
                    Psp_PlayDecideSe();
                stop:
                    Psp_PlayStopSe();
                    gPspWork->unk58 = 0;
                    Stream_FadeOut(0, 0xA);
                    memset(gPspWork->objs, 0, 0x600);
                    gPspWork->unk4++;
                    gPspWork->unk30 = 0x1E;
                }
                break;
            case 2:
                if (gPspWork->unk30 != 0) {
                    gPspWork->unk30--;
                } else {
                    gPspWork->unk58 = 10;
                }
                for (k = 0; k < 6; k++) {
                    if (gPspWork->unk143C[k] != 0) {
                        gPspWork->unk1454[k] += gPspWork->unk1460[k];
                        if (gPspWork->unk1454[k] == 0) {
                            gPspWork->unk1454[k] = 0;
                            gPspWork->unk143C[k] -= gPspWork->unk146C[k];
                            if (gPspWork->unk143C[k] < 0) {
                                gPspWork->unk143C[k] = 0;
                            }
                        }
                    }
                }
                if (trig & 0x400) {
                    col = gPspWork->unkE7;
                    last = gPspWork->unkE4;

                    if (col == 3) { /* compiled-out check */
                    } else {
                        do {

                            col++;
                            j = gPspWork->unkE8;
                            r = gPspCursorCols[col];
                            for (n = 0; n < 3; n++) {
                                c = r[j];
                                if (c != -1 && gPspWork->unkDE[((s8 *)gPspIconIndex)[c]] != 0) {
                                    break;
                                }
                                j++;
                                if (j > 2) {
                                    j = 0;
                                }
                            }
                            if (n < 3) {
                                goto found_r;
                            }
                        } while (col != 3);
                    }
                    goto try_left;
                found_r:
                        gPspWork->unkE8 = j;
                        c = gPspCursorGrid[gPspWork->unkE8][col];
                        if (c == 2) {
                            gPspWork->unkE5 = gPspWork->unkE4;
                            gPspWork->unkE4 = gPspCursorGrid[gPspWork->unkE8][col];
                            gPspWork->unkE7 = col;
                        } else if (c == 4) {
                            gPspWork->unkE6 = gPspWork->unkE4;
                            gPspWork->unkE4 = gPspCursorGrid[gPspWork->unkE8][col];
                            gPspWork->unkE7 = col;
                        } else {
                            gPspWork->unkE4 = c;
                            gPspWork->unkE7 = col;
                        }
                        Psp_PlayMoveSe();
                        Psp_ResetSlotAnim(gPspCursorGrid[gPspWork->unkE8][gPspWork->unkE7]);
                        goto done;
                }
            try_left:
                if (trig & 0x800) {
                    col = gPspWork->unkE7;

                    if (col == 0) { /* compiled-out check */
                    } else {
                        do {

                            col--;
                            row = gPspWork->unkE8;
                            r = gPspCursorCols[col];
                            for (k = 0; k < 3; k++) {
                                c = r[row];
                                if (c != -1 && gPspWork->unkDE[((s8 *)gPspIconIndex)[c]] != 0) {
                                    break;
                                }
                                row++;
                                if (row > 2) {
                                    row = 0;
                                }
                            }
                            if (k < 3) {
                                goto found_l;
                            }
                        } while (col != 0);
                    }
                    goto try_up;
                found_l:
                        gPspWork->unkE8 = row;
                        if (last == 2 && gPspWork->unkE5 != -1) {
                            gPspWork->unkE4 = gPspWork->unkE5;
                            for (row = 0; row < 3; row++) {
                                for (i = 0; i < 4; i++) {
                                    if (gPspWork->unkE4 == gPspCursorGrid[row][i]) {
                                        gPspWork->unkE7 = i;
                                        gPspWork->unkE8 = row;
                                        gPspWork->unkE4 = gPspCursorGrid[gPspWork->unkE8][gPspWork->unkE7];
                                        break;
                                    }
                                }
                            }
                        }
                        if (last == 4 && gPspWork->unkE6 != -1) {
                            gPspWork->unkE4 = gPspWork->unkE6;
                            for (row = 0; row < 3; row++) {
                                for (i = 0; i < 4; i++) {
                                    if (gPspWork->unkE4 == gPspCursorGrid[row][i]) {
                                        gPspWork->unkE7 = i;
                                        gPspWork->unkE8 = row;
                                        gPspWork->unkE4 = gPspCursorGrid[gPspWork->unkE8][gPspWork->unkE7];
                                        break;
                                    }
                                }
                            }
                        }
                        gPspWork->unkE4 = gPspCursorGrid[gPspWork->unkE8][col];
                        gPspWork->unkE7 = col;
                        Psp_PlayMoveSe();
                        Psp_ResetSlotAnim(gPspCursorGrid[gPspWork->unkE8][gPspWork->unkE7]);
                        goto done;
                }
            try_up:
                if (trig & 0x1000) {
                    row = gPspWork->unkE8;
                    col = gPspWork->unkE7;
                    c = gPspCursorGrid[row][col];
                    if (c != 2 && c != 4) {
                        for (;;) {
                            if (row == 2) {
                                goto nav_up;
                            }
                            row++;
                            c = gPspCursorGrid[row][gPspWork->unkE7];
                            if (c == -1) {
                                continue;
                            }
                            if (gPspWork->unkDE[((s8 *)gPspIconIndex)[c]] != 0) {
                                break;
                            }
                        }
                        gPspWork->unkE4 = gPspCursorGrid[row][col];
                        gPspWork->unkE8 = row;
                        Psp_PlayMoveSe();
                        Psp_ResetSlotAnim(gPspCursorGrid[gPspWork->unkE8][gPspWork->unkE7]);
                        break;
                    }
                }
            nav_up:
                if (trig & 0x2000) {
                    row = gPspWork->unkE8;
                    col = gPspWork->unkE7;
                    c = gPspCursorGrid[row][col];
                    if (c != 2 && c != 4) {
                        for (;;) {
                            if (row == 0) {
                                goto nav_ok;
                            }
                            row--;
                            c = gPspCursorGrid[row][gPspWork->unkE7];
                            if (c == -1) {
                                continue;
                            }
                            if (gPspWork->unkDE[((s8 *)gPspIconIndex)[c]] != 0) {
                                break;
                            }
                        }
                        gPspWork->unkE4 = gPspCursorGrid[row][col];
                        gPspWork->unkE8 = row;
                        Psp_PlayMoveSe();
                        Psp_ResetSlotAnim(gPspCursorGrid[gPspWork->unkE8][gPspWork->unkE7]);
                        break;
                    }
                }
            nav_ok:
                if (trig & 0x20) {
                    Psp_PlayConfirmSe();
                    gPspWork->unk58 = 0;
                    SeqA_Start(0xB);
                    Stream_Reset();
                    t->mode = 6;
                }
            done:
                break;
            }
            break;
        }
        break;
    case 5:
        if ((u8)SeqA_GetState() != 1) {
            Psp_Release();
            Stream_Stop(0);
            TaskSlot_End(t);
            gSysSettings.systemSaved = 0;
            Task_SetBootMode1();
            SeqA_Start(0xC);
            return;
        }
        break;
    case 6:
        if ((u8)SeqA_GetState() != 1) {
            SeqA_Reset();
            Stream_Reset();
            t->mode = 8;
            return;
        }
        break;
    case 7:
        if ((u8)NetMenu_Update()) {
            t->mode++;
            SeqA_Start(7);
        }
        Gfx_DrawBlackLineFixed();
        Frame_Draw();
        return;
    case 8:
        Psp_Release();
        Stream_Stop(0);
        TaskSlot_End(t);
        Task_SetBootMode2();
        SeqA_Start(0xC);
        return;
    case 9:
        gPspWork->unk58 = 0xD;
        if (trig & 0x20) {
            Psp_PlayDecideSe();
            SeqA_Start(0xB);
            t->mode = 5;
        }
        break;
    }
    Psp_UpdateScene();
    Callbacks_Add(Psp_Draw);
    Callbacks_Add(Psp_DrawTexts);
}

void Psp_Init(void) {
    u8 *base;

    base = gSysSettings.selWork;
    gPspWork = (PspWork *)(base + 0x100);
    memset(gPspWork, 0, sizeof(PspWork));
    memset(gPspWork->unk24 = (s32 *)(base + 0x15C0), 0, 0x40);
    memset(gPspWork->unk28 = (s32 *)(base + 0x1600), 0, 0x40);
    memset(gPspWork->unk2C = (s32 *)(base + 0x1640), 0, 0x40);
    Psp_LoadFiles(gPspWork->unk100);
    Psp_InitActors();
    gPspWork->unk54 = 0xB;
    gPspWork->unk10 = -1;
    gPspState = 0;
}

void Psp_Release(void) {
    gPspWork = NULL;
    Psp_FreeTextures();
}

void Psp_InitItemMask(void) {
    s32 *mask;
    s32 i;

    gPspWork->unk1C = -1;
    gPspWork->unk0 = 0;
    memset(gPspWork->unk28, 0, 0x40);
    memset(gPspWork->unk24, 0, 0x40);
    mask = gPspWork->unk28;
    for (i = 0; i < 0xE; i++) {
        if (i == 9) {
            if (Save_TestEventFlag(0x81) == 0) {
                continue;
            }
        }
        if (i == 0xA) {
            if (Save_TestListBit(0x138C) == 0) {
                continue;
            }
        }
        *mask |= 1 << i;
    }
}

s32 Psp_LoadModule(char *path, s32 argc, char *argv) {
    s32 res;
    s32 ret;

    do {
        ret = sceSifLoadStartModule(path, argc, argv, &res);
    } while (ret < 0);
    if (res == 1) {
        ret = -1;
    }
    return ret;
}

void Psp_UnloadModule(s32 id) {
    s32 res;

    do {

    } while (sceSifStopModule(id, 0, 0, &res) < 0);
    do {

    } while (sceSifUnloadModule(id) < 0);
}

/* The thread's gp is read with an asm statement written just before the
 * initPriority store, which puts `sw gp` in CreateThread's delay slot. */
void Psp_StartThread(PspWork *w) {
    struct ThreadParam tp;
    s32 id;
    void *g;

    if (w->unk10 < 0) {
        id = Psp_LoadModule("cdrom0:\\IOP\\MODULES\\USBPSPCM.IRX;1", 0x20, "-psp=1\0-unit=1\0-title=ULJM05066");
        if (id >= 0) {
            w->unk10 = id;
            func_18CC10();
            if (gPspThreadId == 0) {
                gPspState = 0;
                tp.entry = Psp_Thread;
                tp.stack = gPspThreadStack;
                tp.stackSize = 0x1000;
                __asm__ volatile ("move %0, $gp" : "=r"(g));
                tp.gpReg = g;
                tp.initPriority = 0x28;
                gPspThreadId = CreateThread(&tp);
                func_114C60(gPspThreadId, 0);
            }
        }
    }
}



void Psp_Shutdown(PspWork *w) {
    gPspState = 3;
    if (w->unk10 >= 0) {
        if (gPspThreadId != 0) {
            TerminateThread(gPspThreadId);
            DeleteThread(gPspThreadId);
        }
        Psp_UnloadModule(w->unk10);
        w->unk10 = -1;
        gPspThreadId = 0;
        func_18CC10();
    }
}

void PspLink_AddCount(void);
void EventFlag_Set(s32);
void Save_ClearEventFlag(s32);
void PspLink_SetFlags(u32 flags);
u16 Rand_Next(s32 stream);
s32 Psp_IsItemUnlocked(u16 id);

/* Picks what CONNECT offers this time: the features of *unk24 whose event
 * flag isn't set yet (and whose conditions hold), then the menus they
 * appear in, some at random. */
void Psp_SelectFeatures(void) {
    s32 *p;
    s32 mask;
    s32 i;
    s32 bit;

    p = gPspWork->unk24;
    *p |= 2;
    *p |= 0x800;
    PspLink_AddCount();
    EventFlag_Set(0x35);
    Save_ClearEventFlag(0x31);
    mask = 0;
    for (i = 0; i < 12; i++) {
        bit = 1 << i;
        if (*p & bit) {
            switch (i) {
            case 0:
                if (Save_TestEventFlag(0x78) == 0) {
                    mask |= bit;
                }
                break;
            case 1:
                if (Save_TestEventFlag(0x79) == 0 && Save_TestEventFlag(6) == 1) {
                    mask |= bit;
                }
                break;
            case 2:
                if (Save_TestEventFlag(0x7A) == 0 && Save_TestEventFlag(0xB6) == 1 && gSave.styleBits != 0
                    && !(gSave.rateFlags & 8)) {
                    mask |= bit;
                }
                break;
            case 3:
                mask |= bit;
                break;
            case 4:
                if (gSysSave.unk458 == 0 && Save_TestEventFlag(0x7C) == 0) {
                    mask |= bit;
                }
                break;
            case 5:
                if (Save_TestEventFlag(0x7D) == 0) {
                    mask |= bit;
                }
                break;
            case 6:
                if (Save_TestEventFlag(0x7E) == 0) {
                    mask |= bit;
                }
                break;
            case 7:
                if (Save_TestEventFlag(0x7F) == 0) {
                    mask |= bit;
                }
                break;
            case 8:
                if (Psp_IsItemUnlocked(0xA) == 1 && Save_TestEventFlag(0x82) == 0) {
                    mask |= bit;
                }
                break;
            case 9:
                if ((gSave.monsterFlags & 0x400) && Save_TestEventFlag(0x83) == 0) {
                    mask |= bit;
                }
                break;
            case 10:
                if (Save_TestEventFlag(0x49) == 0) {
                    mask |= bit;
                }
                break;
            case 11:
                mask |= bit;
                break;
            }
        }
    }
    PspLink_SetFlags(mask);
    for (i = 0; i < 11; i++) {
        if (mask & (1 << i)) {
            gPspWork->unkA4[i] = 1;
            switch (i) {
            case 0:
            case 3:
            case 6:
            case 10:
                gPspWork->unkB4[1] = 6;
                break;
            case 2:
            case 7:
            case 8:
            case 9:
                gPspWork->unkB4[4] = Rand_Next(1) % 2 + 4;
                break;
            case 1:
                gPspWork->unkB4[3] = 1;
                break;
            case 5:
                gPspWork->unkB4[2] = Rand_Next(1) % 2 + 2;
                break;
            case 4:
                gPspWork->unkB4[6] = 7;
                break;
            }
        }
    }
    gPspWork->unkD4 = 0;
    for (i = 0; i < 16; i++) {
        gPspWork->unkC4[i] = gPspWork->unkB4[i];
        if (gPspWork->unkC4[i] != 0) {
            gPspWork->unkD4++;
        }
    }
    gPspWork->unk40 = mask;
}

/* Loads archive 0x27 and passes up to 13 of its files to Tex_CreateFromTim2; the
   results go to gOvlTex and out. The loop keeps the original's shape: i is
   set before the test, p after it. */
void Psp_LoadFiles(s32 *out) {
    PspFileEnt *ent;
    u8 *buf;
    s32 n;
    s32 *p;
    s32 i;

    buf = gSysSettings.loadBuf;
    if (LoadCompressedFile(buf, 0x27) > 0) {
        n = *(s32 *)(buf + 4);
        ent = (PspFileEnt *)(buf + 8);
        if (n > 0xD) {
            n = 0xD;
        }
        i = 0;
        if (i < n) {
            p = gOvlTex;
            do {
                *p = Tex_CreateFromTim2(buf + (ent++)->offset, 4);
                out[i] = *p;
                p++;
                i++;
            } while (i < n);
        }
        Mem_Compact();
    }
}

void Psp_FreeTextures(void) {
    Tex_FreeRange(0xA3, 0xD);
    Mem_Compact();
}

void Psp_InitActors(void) {
    s32 i;
    s8 *p;
    PspActor *a;

    p = (s8 *)gPspActorInit;
    for (i = 0; i < 16; i++) {
        a = &gPspWork->actors[*p++];
        a->unk0 = 1;
        a->unk4C = 1.0f;
        a->unk50 = 1.0f;
        a->unk4 = *p++;
        if (*p >= 0) {
            a->unk58 = (u8 *)gPspActorAnims + *p++ * 16;
        } else {
            a->unk58 = NULL;
            p++;
        }
        a->unk8 = *p++;
        if (a->unk8 != 0) {
            a->unk5C = 1;
            a->unk60 = &gPspWork->actors[1];
        }
        a->unk48 = *p++;
    }
    gPspWork->unkDB8 = 0x16;
    gPspWork->unk134 = 0;
    memset(gPspWork->objs, 0, 0x600);
}

s32 PspLink_HasFlag(s32 item);

/* Marks the icons whose items are there: for each of the six icons, the
 * first entry of its list (gPspIconLists, 4 each, -1: none) that
 * PspLink_HasFlag accepts; its actor and the next one get their place and
 * scale. Then the last icon with one is looked up in gPspCursorGrid (3 rows
 * of 4). */
void Psp_MarkIcons(void) {
    s8 *row;
    PspActor *a;
    s32 i;
    s8 c;
    s32 j;
    s8 *p;
    s8 *icon;
    s16 *pos;
    s8 *act;
    s8 *kind;
    s8 *lists;

    for (i = 0, act = (s8 *)gPspIconActors, lists = (s8 *)gPspIconLists, icon = (s8 *)gPspIconIndex, pos = (s16 *)gPspIconPos, kind = (s8 *)gPspMarkKind; i < 6; i++, act++, lists += 4, icon++, pos += 2, kind++) {
        p = lists;
        a = &gPspWork->actors[*act];
        for (j = 0; j < 4; j++) {
            c = *p++;
            if (c != -1 && PspLink_HasFlag(c) != 0) {
                break;
            }
        }
        if (j == 4) {
            continue;
        }
        gPspWork->unkDE[*icon] = 1;
        gPspWork->unkD8[*icon] = j;
        a->unk4 = 1;
        a->unkC++;
        if (a->unk48 == 0) {
            a->unk4C = 1.0f;
            a->unk50 = 1.2f;
        } else {
            a->unk4C = 1.2f;
            a->unk50 = 1.4f;
        }
        a->unk30 = pos[0];
        a->unk34 = pos[1];
        a[1].unk0 = 1;
        a[1].unk4 = 1;
        a[1].unk8 = *kind;
        a[1].unk48 = a->unk48;
        a[1].unk4C = a->unk4C;
        a[1].unk50 = a->unk50;
        a[1].unk30 = a->unk30;
        a[1].unk34 = a->unk34;
        a[1].unk5C = a->unk5C;
        a[1].unk60 = a->unk60;
    }
    gPspWork->unkE4 = -1;
    gPspWork->unkE5 = -1;
    gPspWork->unkE6 = -1;
    for (i = 0; i < 6; i++) {
        if (gPspWork->unkDE[((s8 *)gPspIconIndex)[i]] != 0) {
            gPspWork->unkE4 = i;
        }
    }
    for (i = 0, row = (s8 *)gPspCursorGrid; i < 3; i++, row += 4) {
        for (j = 0; j < 4; j++) {
            if (gPspWork->unkE4 == row[j]) {
                gPspWork->unkE7 = j;
                gPspWork->unkE8 = i;
                return;
            }
        }
    }
}

/* Steps actors 0, 1, 20 and 22, stops actors 2-7 and every picture, then
 * marks the icons again. MWCC reloads gPspWork after every store through
 * it; the element pointers a and o, each taken from gPspWork once, give
 * the game's loads (the 2-7 loop is unrolled fully). */
void Psp_ResetSceneActors(void) {
    s32 i;
    PspActor *a;
    PspObj *o;

    gPspWork->actors[0].unkC++;
    a = &gPspWork->actors[1];
    a->unk4 = 1;
    a->unkC++;
    gPspWork->actors[20].unkC++;
    gPspWork->actors[22].unkC++;
    for (i = 2; i < 8; i++) {
        a = &gPspWork->actors[i];
        a->unkC = 0;
        a->unk4 = 0;
    }
    o = gPspWork->objs;
    for (i = 0; i < 0x20; i++, o++) {
        o->active = 0;
    }
    Psp_MarkIcons();
}

static void Psp_SetAnim(PspObj *o, PspAnim *a) {
    s32 dx;
    s32 dy;

    o->active = 1;
    o->unk4 = 0;
    o->unk8 = 0;
    o->unk1C = 0;
    o->unk20 = 0;
    o->anim = a;
    o->unk10 = 0;
    o->unk4 = a->unk4;
    dx = a->dx;
    if (o->flip != 0) {
        dx = -dx;
    }
    dy = a->dy;
    o->x += dx;
    o->y += dy;
}

void Sound_PlaySeFlatPan(u32 slot, u32 id, s32 prog, s32 pan);
void Psp_SpawnChild(PspObj *src, s32 type, s32 x, s32 y);

/* Moves a CONNECT picture: its walk by type, then at the end of a frame's
 * time the next frame (random turns for types 2 and 6), the frame's step
 * and sound (panned by x), and the end of a type 0 ends its type 9 child.
 * `d *= -1` keeps the game's run-time negu (`d = -d` is folded), and
 * `pan = K * pan` on the int gives the pan's operand order (constant in
 * f1, x in f0). */
void Psp_UpdateObj(PspObj *o) {
    PspAnim *f;
    s32 d;
    s32 e;
    s32 pan;
    s32 snd;
    PspObj *p;
    s32 i;

    switch (o->type) {
    case 1:
        d = -4;
        if (o->flip != 0) {
            d *= -1;
        }
        o->x += d;
        break;
    case 2:
        if (o->unk10 < 4) {
            d = -1;
            if (o->flip != 0) {
                d *= -1;
            }
            o->x += d;
        }
        break;
    case 3:
        d = -1;
        if (o->flip != 0) {
            d *= -1;
        }
        o->x += d;
        break;
    case 4:
        d = -1;
        if (o->flip != 0) {
            d *= -1;
        }
        o->x += d;
        break;
    case 5:
        d = -2;
        if (o->flip != 0) {
            d *= -2;
        }
        o->x += d;
        break;
    case 6:
        if (o->unk10 < 6) {
            d = -1;
            if (o->flip != 0) {
                d *= -1;
            }
            o->x += d;
        }
        break;
    case 9:
        return;
    }
    if (o->unk4 > 0) {
        o->unk4--;
        return;
    }
    f = &o->anim[o->unk10];
    switch (o->type) {
    case 0:
        if (f->unk1 == 1) {
            o->unk8++;
            if (o->unk8 >= 8) {
                o->unk8 = 0;
                o->unk10 = 7;
                if (o->flip == 0) {
                    Psp_SpawnChild(o, 8, 0x14, -0x62);
                } else {
                    Psp_SpawnChild(o, 8, -0x14, -0x62);
                }
            } else {
                o->unk10 = f->next;
            }
        } else {
            o->unk10 = f->next;
        }
        break;
    case 2:
        if (f->unk1 == 1) {
            if ((Rand_Next(1) % 10) < 2) {
                o->unk10 = 4;
            } else {
                o->unk10 = f->next;
            }
        } else {
            o->unk10 = f->next;
        }
        break;
    case 6:
        if (f->unk1 == 1) {
            if ((Rand_Next(1) % 10) < 2) {
                o->unk10 = 6;
            } else {
                o->unk10 = f->next;
            }
        } else {
            o->unk10 = f->next;
        }
        break;
    default:
        if (f->next < 0) {
            o->active = 0;
            return;
        }
        o->unk10 = f->next;
        break;
    }
    f = &o->anim[o->unk10];
    o->unk4 = f->unk4;
    d = f->dx;
    if (o->flip != 0) {
        d *= -1;
    }
    e = f->dy;
    o->x += d;
    o->y += e;
    if (o->flip != 0) {
        if (o->x > o->unk24) {
            o->active = 0;
        }
    } else if (o->x < o->unk24) {
        o->active = 0;
    }
    snd = f->se;
    if (snd != 0) {
        pan = o->x - 0x140;
        pan = 0.15625f * pan;
        if (pan < 0) {
            if (pan < -0x32) {
                pan = -0x32;
            }
        } else if (pan > 0x32) {
            pan = 0x32;
        }
        if (snd == 10) {
            o->unk8++;
            if ((o->unk8 & 3) == 0) {
                Sound_PlaySeFlatPan(6, 7, 0, pan);
            } else {
                Sound_PlaySeFlatPan(6, f->se - 1, 0, pan);
            }
        } else {
            Sound_PlaySeFlatPan(6, snd - 1, 0, pan);
        }
    }
    if (o->active == 0 && o->type == 0) {
        p = gPspWork->objs;
        for (i = 0; i < 0x20; i++, p++) {
            if (p->active != 0 && p->type == 9 && o->flip == p->flip) {
                p->active = 0;
                return;
            }
        }
    }
}

void Gfx_SetBlend(s32 a, s32 b);

void Psp_DrawObject(PspObj *o) {
    PspAnim *f;
    s16 *c;
    Sprite spr;
    s16 u;
    s16 v;
    s16 tw;
    s16 th;
    s16 sw;
    s16 sh;

    f = &o->anim[o->unk10];
    c = f->cells + f->cell * 6;
    if (gPspWork->unk100[f->tex] != 0) {
        Gfx_SetBlend(4, 5);
        Tex_Prepare(1, f->tex + 0xA3);
        Tex_Bind(f->tex + 0xA3);
        Gs_SetState(0x63, 0);
        u = *c++;
        v = *c++;
        tw = *c++;
        th = *c++;
        sw = *c++;
        sh = *c;
        if (o->flip == 0) {
            spr.x = (s16)o->x + f->ox;
        } else {
            spr.x = (s16)o->x - (sw + f->ox);
        }
        spr.y = (s16)o->y + f->oy;
        spr.w = sw;
        spr.h = sh;
        spr.color = -1;
        if (o->flip == 0) {
            spr.u0 = u;
            spr.u1 = spr.u0 + tw - 1;
        } else {
            spr.u1 = u;
            spr.u0 = spr.u1 + tw - 1;
        }
        spr.v0 = v;
        spr.v1 = spr.v0 + th - 1;
        Sprite_Draw(&spr);
    }
}

s32 Psp_AnyObjectActive(void) {
    PspObj *o;
    s32 i;

    for (i = 0, o = gPspWork->objs; i < 0x20; i++, o++) {
        if (o->active != 0) {
            return 1;
        }
    }
    return 0;
}


void Psp_SpawnObject(s32 type, s32 flip) {
    PspObj *o;
    s32 i;
    s16 *p;

    for (i = 0, o = gPspWork->objs; i < 0x20; i++, o++) {
        if (o->active == 0) {
            break;
        }
    }
    if (i < 0x20) {
        o->type = type;
        o->flip = flip;
        p = (s16 *)gPspObjParams + type * 5;
        if (flip != 0) {
            o->x = p[0];
            o->unk24 = p[4];
        } else {
            o->x = p[1];
            o->unk24 = p[3];
        }
        o->y = p[2];
        Psp_SetAnim(o, gPspObjAnims[type]);
        if (type == 0) {
            if (o->flip == 0) {
                Psp_SpawnChild(o, 9, 0x1D6, o->y);
            } else {
                Psp_SpawnChild(o, 9, 0xAA, o->y);
            }
        }
    }
}

void Psp_SpawnChild(PspObj *src, s32 type, s32 x, s32 y) {
    PspObj *o;
    s32 i;

    for (i = 0, o = gPspWork->objs; i < 0x20; i++, o++) {
        if (o->active == 0) {
            break;
        }
    }
    if (i < 0x20) {
        o->type = type;
        o->flip = src->flip;
        if (type == 9) {
            o->x = x;
            o->y = y;
            o->unk24 = src->unk24;
        } else {
            o->x = src->x + x;
            o->y = src->y + y;
            o->unk24 = src->unk24;
        }
        Psp_SetAnim(o, gPspObjAnims[type]);
    }
}

void Psp_SetViewRect(u32 kind, PspView *v) {
    s16 *p;

    p = (s16 *)gPspViewRects;
    switch (kind) {
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        p += 0x14;
        p += (kind - 2) * 4;
        break;
    case 8:
    case 10:
    case 12:
    case 14:
    case 16:
    case 18:
        p += 0x10;
        break;
    case 21:
        return;
    case 20:
        p += 4;
        break;
    case 9:
    case 11:
    case 13:
    case 15:
    case 17:
    case 19:
        return;
    case 22:
        p += 0x2C;
        break;
    }
    v->x = *p++;
    v->y = *p++;
    v->w = *p++;
    v->h = *p;
    switch (kind) {
    case 8:
    case 10:
    case 12:
    case 14:
    case 16:
    case 18:
    case 22:
        v->x2 = v->x;
        v->y2 = v->y;
        v->w2 = v->w;
        v->h2 = v->h;
        break;
    }
}

f32 Math_Sin(f32);

/* Moves CONNECT's screen i (a state machine in a->unkC): the two that slide
 * in, the six that fly across at a random speed and height, and the menu
 * screens, dimmed unless theirs is the icon chosen. A screen in use
 * (a->unk4) goes into the frame's list. */
void Psp_UpdateActor(s32 i, PspActor *a) {
    s32 r;
    PspView **views;
    s16 spd;
    s32 k;
    PspWork *w;
    s32 e;

    views = gPspWork->unkDBC;
    switch (i) {
    case 0:
        switch (a->unkC) {
        case 0:
            Psp_SetViewRect(i, (PspView *)a);
            a->unk30 = 0.0f;
            a->unk34 = 0.0f;
            a->unk40 = 0x280;
            a->unk20 = 30.0f;
            a->unkC++;
            break;
        case 1:
            break;
        case 2:
            a->unk30 += a->unk20;
            a->unk40 -= (s32)(s16)a->unk20;
            if (a->unk30 >= 640.0f) {
                a->unk30 = 640.0f;
                a->unkC++;
                a->unk4 = 0;
            }
            break;
        case 3:
            break;
        }
        break;
    case 1:
        switch (a->unkC) {
        case 0:
            Psp_SetViewRect(i, (PspView *)a);
            a->unk20 = 30.0f;
            a->unk30 = -640.0f;
            a->unk34 = 0.0f;
            a->unkC++;
            break;
        case 1:
            break;
        case 2:
            a->unk30 += a->unk20;
            if (a->unk30 >= 0.0f) {
                a->unk30 = 0.0f;
                a->unkC++;
            }
            break;
        case 3:
            break;
        }
        break;
    case 20:
        switch (a->unkC) {
        case 0:
            Psp_SetViewRect(i, (PspView *)a);
            a->unkC++;
            break;
        case 1:
            break;
        case 2:
            a->unk4 = 0;
            a->unkC++;
            break;
        case 3:
            break;
        }
        break;
    case 21:
        switch (a->unkC) {
        case 0:
            Psp_SetViewRect(i, (PspView *)a);
            a->unkC++;
            break;
        case 1:
            break;
        }
        break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        switch (a->unkC) {
        case 0:
            break;
        case 1:
            Psp_SetViewRect(i, (PspView *)a);
            r = Rand_Next(1) & 1;
            k = Rand_Next(1) % 4;
            spd = gPspActorSpeeds[k];
            a->unk8 = r;
            if (r == 0) {
                a->unk20 = -1.0f * spd;
                a->unk30 = 700.0f;
                a->unk38 = 0;
            } else {
                a->unk20 = spd;
                a->unk30 = -200.0f;
                a->unk38 = 1;
            }
            r = Rand_Next(1) & 0x7FFF;
            a->unk34 = 100.0f + 100.0f * Math_Sin(6.2831855f * r / 65536.0f);
            a->unkC++;
            break;
        case 2:
            a->unk30 += a->unk20;
            if (a->unk8 == 0) {
                if (a->unk30 <= -200.0f) {
                    a->unkC = 1;
                }
            } else if (a->unk30 >= 700.0f) {
                a->unkC = 1;
            }
            break;
        }
        break;
    case 8:
    case 10:
    case 12:
    case 14:
    case 16:
    case 18:
    case 22:
        switch (a->unkC) {
        case 0:
            Psp_SetViewRect(i, (PspView *)a);
            a->unkC++;
            break;
        case 1:
            break;
        case 2:
            a->unk54 = 1;
            w = gPspWork;
            e = w->unkE4;
            if ((i - 8) / 2 == e && w->unkDE[((s8 *)gPspIconIndex)[e]] != 0) {
                a->unk54 = 0;
            }
            break;
        }
        break;
    }
    if (a->unk4 != 0) {
        views[i] = (PspView *)a;
    }
}

void Psp_UpdateScene(void) {
    s32 *st;
    PspActor *a;
    s32 i;
    s32 n;
    PspObj *o;

    memset(gPspWork->unkDBC, 0, 0x80);
    st = &gPspWork->unk134;
    switch (*st) {
    case 0:
        if (gPspWork->unk0 == 0xB && gPspWork->unk4 == 2) {
            (*st)++;
            Psp_ResetSceneActors();
        }
        break;
    case 1:
        break;
    }
    n = gPspWork->unkDB8;
    for (i = 0, a = gPspWork->actors; i < n; i++, a++) {
        Psp_UpdateActor(i, a);
    }
    for (i = 0, o = gPspWork->objs; i < 0x20; i++, o++) {
        if (o->active != 0) {
            Psp_UpdateObj(o);
        }
    }
}

struct PrimLine;
s32 Prim_FillRect(struct PrimLine *r);

/* Draws an actor: its part as a filled rectangle (kind 0) or a sprite,
 * placed relative to its parent (unk60) when unk5C is set, scaled,
 * mirrored and dimmed as the actor says. */
void Psp_DrawActor(PspActor *a) {
    PspRect r;
    Sprite spr;
    PspPart *pt;
    s16 px;
    s16 py;
    s16 w;
    s16 h;
    u32 c;

    if (a->unk0 == 0) {
        return;
    }
    if (a->unk4 == 0) {
        return;
    }
    if ((pt = (PspPart *)a->unk58) == NULL) {
        return;
    }
    px = py = 0;
    if (a->unk5C != 0 && a->unk60 != NULL) {
        px = ((PspActor *)a->unk60)->unk30;
        py = ((PspActor *)a->unk60)->unk34;
    }
    w = a->unk40 * a->unk4C;
    h = a->unk42 * a->unk50;
    if (pt->kind == 0) {
        Gfx_SetBlend(4, 5);
        r.x0 = px + (s16)a->unk30;
        r.y0 = py + (s16)a->unk34;
        r.x1 = r.x0 + w;
        r.y1 = r.y0 + h;
        r.color = pt->color;
        Prim_FillRect((struct PrimLine *)&r);
        return;
    }
    Gfx_SetBlend(4, 5);
    if (gPspWork->unk100[pt->tex] == 0) {
        return;
    }
    Tex_Prepare(1, pt->tex + 0xA3);
    Tex_Bind(pt->tex + 0xA3);
    if (pt->blend == 0) {
        Gs_SetState(0x63, 0x10000);
    } else {
        Gs_SetState(0x63, 0);
    }
    spr.x = px + (s16)a->unk30;
    spr.y = py + (s16)a->unk34;
    spr.w = w;
    spr.h = h;
    if (a->unk54 == 0) {
        spr.color = pt->color;
    } else {
        c = pt->color;
        spr.color = ((c & 0xFF) >> 1 & 0xFF) | (((c & 0xFF00) >> 1 & 0xFF00) | ((c & 0xFF000000) | ((c & 0xFF0000) >> 1 & 0xFF0000)));
    }
    if (a->unk38 == 0) {
        spr.u0 = pt->u;
        spr.u1 = spr.u0 + pt->uw - 1;
    } else {
        spr.u1 = pt->u;
        spr.u0 = spr.u1 + pt->uw - 1;
    }
    if (a->unk3C == 0) {
        spr.v0 = pt->v;
        spr.v1 = spr.v0 + pt->vh - 1;
    } else {
        spr.v1 = pt->v;
        spr.v0 = spr.v1 + pt->vh - 1;
    }
    Sprite_Draw(&spr);
}

void Vec_RotateZ(f32 *v, f32 a);
struct PrimTexTri;
s32 Prim_DrawTexTri(struct PrimTexTri *t);

/* An actor on a CONNECT screen drawn as two textured triangles, turned by
 * its screen swing (unk143C/unk1454 per screen k); the centre goes to
 * unk1478/unk1484 for Psp_DrawSwingFrame.
 * FAKEMATCH: q01/q30/q31 point at p[0][1], p[3][0] and p[3][1] only so
 * that those three addresses are declared values. The address of a p
 * element that is stored and read back is one value across the calls; the
 * game keeps these three in saved registers and rematerialises the others,
 * as MWCC does when they have one reference more or are numbered first
 * (a duplicate store to each also gives it, at 3 extra words). The natural
 * form behind it is not known. */
void Psp_DrawSwingActor(PspActor *a, s32 k) {
    PspTri t[2];
    f32 p[4][3];
    PspPart *pt;
    s16 px;
    s16 py;
    s16 w;
    s16 h;
    s16 cy;
    s16 cx;
    f32 hw;
    u16 r;
    s32 i;
    u32 c;
    f32 *q01;
    f32 *q30;
    f32 *q31;
    s16 x1;
    s16 y1;
    s16 x2;
    s16 y2;

    if (a->unk0 == 0) {
        return;
    }
    if (a->unk4 == 0) {
        return;
    }
    if ((pt = (PspPart *)a->unk58) == NULL) {
        return;
    }
    px = py = 0;
    if (a->unk5C != 0 && a->unk60 != NULL) {
        px = ((PspActor *)a->unk60)->unk30;
        py = ((PspActor *)a->unk60)->unk34;
    }
    w = a->unk40 * a->unk4C;
    h = a->unk42 * a->unk50;
    Gfx_SetBlend(4, 5);
    if (gPspWork->unk100[pt->tex] == 0) {
        return;
    }
    Tex_Prepare(1, pt->tex + 0xA3);
    Tex_Bind(pt->tex + 0xA3);
    if (pt->blend == 0) {
        Gs_SetState(0x63, 0x10000);
    } else {
        Gs_SetState(0x63, 0);
    }
    cx = w / 2 + (px + (s16)a->unk30);
    cy = py + (s16)a->unk34;
    gPspWork->unk1478[k] = cx;
    gPspWork->unk1484[k] = cy;
    hw = w / 2.0f;
    q01 = &p[0][1];
    q30 = &p[3][0];
    q31 = &p[3][1];
    p[0][0] = -hw;
    *q01 = 0.0f;
    p[0][2] = 0.0f;
    p[1][0] = -hw;
    p[1][1] = h;
    p[1][2] = 0.0f;
    p[2][0] = hw;
    p[2][1] = 0.0f;
    p[2][2] = 0.0f;
    *q30 = hw;
    *q31 = h;
    p[3][2] = 0.0f;
    r = gPspWork->unk143C[k] * Math_Sin(6.2831855f * gPspWork->unk1454[k] / 65536.0f);
    for (i = 0; i < 4; i++) {
        Vec_RotateZ(p[i], 6.2831855f * r / 65536.0f);
    }
    if (a->unk54 == 0) {
        c = pt->color;
    } else {
        c = pt->color;
        c = ((c & 0xFF) >> 1 & 0xFF) | (((c & 0xFF00) >> 1 & 0xFF00) | ((c & 0xFF000000) | ((c & 0xFF0000) >> 1 & 0xFF0000)));
    }
    t[0].color = c;
    t[1].color = c;
    t[0].x0 = cx + (s16)p[0][0];
    t[0].y0 = cy + (s16)*q01;
    t[0].x1 = x1 = cx + (s16)p[1][0];
    t[0].y1 = y1 = cy + (s16)p[1][1];
    t[0].x2 = x2 = cx + (s16)p[2][0];
    t[0].y2 = y2 = cy + (s16)p[2][1];
    t[0].u0 = pt->u;
    t[0].v0 = pt->v;
    t[0].u1 = pt->u;
    t[0].v1 = pt->v + pt->vh - 1;
    t[0].u2 = pt->u + pt->uw - 1;
    t[0].v2 = pt->v;
    t[1].x0 = x1;
    t[1].y0 = y1;
    t[1].x1 = x2;
    t[1].y1 = y2;
    t[1].x2 = cx + (s16)*q30;
    t[1].y2 = cy + (s16)*q31;
    t[1].u0 = pt->u;
    t[1].v0 = pt->v + pt->vh - 1;
    t[1].u1 = pt->u + pt->uw - 1;
    t[1].v1 = pt->v;
    t[1].u2 = pt->u + pt->uw - 1;
    t[1].v2 = pt->v + pt->vh - 1;
    Prim_DrawTexTri((struct PrimTexTri *)&t[0]);
    Prim_DrawTexTri((struct PrimTexTri *)&t[1]);
}

/* A frame on CONNECT screen k (texture 0xAF, rectangle gPspFrameRects[unk8])
 * drawn as two triangles turned by the screen's swing around the centre
 * Psp_DrawSwingActor left in unk1478/unk1484; dimmed unless unk8 is the
 * selected kind.
 * FAKEMATCH: q01/q30/q31 point at p[0][1], p[3][0] and p[3][1], as in
 * Psp_DrawSwingActor (the game keeps those three addresses in saved
 * registers); the natural form is not known. The empty range check of k
 * between the two centre reads is not contrived: it is a compiled-out
 * check (docs/mwcc-matching.md, "Statements scheduled as if a check were still
 * there"). It splits the tail into blocks, so MWCC reuses the k * 2 it
 * made before the rotation loop (s0) for both reads, as the game does. */
void Psp_DrawSwingFrame(PspActor *a, s32 k) {
    s32 i;
    PspTri t[2];
    f32 p[4][3];
    s16 *rec;
    u32 c;
    s16 h;
    s16 cx;
    s16 cy;
    s16 u;
    s16 v;
    s16 u2;
    s16 v2;
    s16 x1;
    s16 y1;
    s16 x2;
    s16 y2;
    f32 hw;
    u16 r;
    s16 w;
    f32 *q01;
    f32 *q30;
    f32 *q31;

    if (a->unk0 == 0) {
        return;
    }
    if (a->unk4 == 0) {
        return;
    }
    rec = (s16 *)&gPspFrameRects[a->unk8 * 2];
    w = rec[2] * a->unk4C;
    h = rec[3] * a->unk4C;
    Gfx_SetBlend(4, 5);
    if (gPspWork->unk100[12] == 0) {
        return;
    }
    Tex_Prepare(1, 0xAF);
    Tex_Bind(0xAF);
    Gs_SetState(0x63, 0);
    c = gPspFrameColors[a->unk8];
    if (a->unk8 != ((s8 *)gPspMarkKind)[gPspWork->unkE4]) {
        c = ((c & 0xFF) >> 1 & 0xFF) | (((c & 0xFF00) >> 1 & 0xFF00) | ((c & 0xFF000000) | ((c & 0xFF0000) >> 1 & 0xFF0000)));
    }
    hw = w / 2.0f;
    q01 = &p[0][1];
    q30 = &p[3][0];
    q31 = &p[3][1];
    p[0][0] = -hw;
    p[0][1] = 30.0f;
    p[0][2] = 0.0f;
    p[1][0] = -hw;
    p[1][1] = 30.0f + h;
    p[1][2] = 0.0f;
    p[2][0] = hw;
    p[2][1] = 30.0f;
    p[2][2] = 0.0f;
    p[3][0] = hw;
    p[3][1] = 30.0f + h;
    p[3][2] = 0.0f;
    r = gPspWork->unk143C[k] * Math_Sin(6.2831855f * gPspWork->unk1454[k] / 65536.0f);
    for (i = 0; i < 4; i++) {
        Vec_RotateZ(p[i], 6.2831855f * r / 65536.0f);
    }
    t[0].color = c;
    t[1].color = c;
    u = rec[0];
    u2 = u + rec[2] - 1;
    v = rec[1];
    v2 = v + rec[3] - 1;
    cy = gPspWork->unk1484[k];
    if (k < 0 || k >= 6) { /* compiled-out check */
    }
    cx = gPspWork->unk1478[k];
    t[0].x0 = cx + (s16)p[0][0];
    t[0].y0 = cy + (s16)*q01;
    t[0].x1 = x1 = cx + (s16)p[1][0];
    t[0].y1 = y1 = cy + (s16)p[1][1];
    t[0].x2 = x2 = cx + (s16)p[2][0];
    t[0].y2 = y2 = cy + (s16)p[2][1];
    t[0].u0 = u;
    t[0].v0 = v;
    t[0].u1 = u;
    t[0].v1 = v2;
    t[0].u2 = u2;
    t[0].v2 = v;
    t[1].x0 = x1;
    t[1].y0 = y1;
    t[1].x1 = x2;
    t[1].y1 = y2;
    t[1].x2 = cx + (s16)*q30;
    t[1].y2 = cy + (s16)*q31;
    t[1].u0 = u;
    t[1].v0 = v2;
    t[1].u1 = u2;
    t[1].v1 = v;
    t[1].u2 = u2;
    t[1].v2 = v2;
    Prim_DrawTexTri((struct PrimTexTri *)&t[0]);
    Prim_DrawTexTri((struct PrimTexTri *)&t[1]);
}

void Psp_DrawBackground(void) {
    Sprite spr;

    if (gPspWork->unk100[1] != 0) {
        Gfx_SetBlend(4, 5);
        Tex_Prepare(1, 0xA4);
        Tex_Bind(0xA4);
        Gs_SetState(0x63, 0);
        spr.x = 0;
        spr.y = 0;
        spr.w = 0x280;
        spr.h = 0x1C0;
        spr.color = -1;
        spr.u0 = 0;
        spr.v0 = 0;
        spr.u1 = 0xFF;
        spr.v1 = 0xFF;
        Sprite_Draw(&spr);
    }
}

void Psp_DrawActor(PspActor *a);
void Psp_DrawSwingActor(PspActor *a, s32 k);
void Psp_DrawSwingFrame(PspActor *a, s32 k);
void Psp_DrawObject(PspObj *o);
void Sel_DrawHeader(s16);

void Psp_Draw(void) {
    PspView **v;
    PspObj *o;
    s32 i;

    Gs_SetState(0x6C, 0);
    for (i = 0, v = gPspWork->unkDBC; i < 0x20; i++, v++) {
        if (*v != NULL) {
            switch (i) {
            case 9:
            case 11:
            case 13:
            case 15:
            case 17:
            case 19:
                Psp_DrawSwingFrame((PspActor *)*v, (i - 9) / 2);
                break;
            case 8:
            case 10:
            case 12:
            case 14:
            case 16:
            case 18:
                Psp_DrawSwingActor((PspActor *)*v, (i - 8) / 2);
                break;
            default:
                Psp_DrawActor((PspActor *)*v);
                break;
            }
        }
    }
    for (i = 0, o = gPspWork->objs; i < 0x20; i++, o++) {
        if (o->active != 0) {
            Psp_DrawObject(o);
        }
    }
    if (gPspWork->unk54 != 0) {
        Sel_DrawHeader(gPspWork->unk54);
    }
    Psp_DrawBackground();
    Gs_SetState(0x6C, 1);
}

extern u32 gPspStepTexts[12];
extern u32 gPspLines[2];
extern u32 gPspPromptTexts[2];
extern u32 gPspValueNames[8];
char *strcpy(char *, char *);
char *strchr(char *, s32);                  /* strchr */
/* unit's own declaration: with the definition's void (s32 id, s32 x, s32 y, s32 depth, u32 color, f32 scale) (sub_main/unk_36BE70.c), Psp_DrawTexts: 677 of 770 words differ */
void Hud_DrawButtonIcon(s32 id, s16 x, s16 y, s32 depth, u32 color, f32 scale);  /* a button icon */
s32 Str_Length(char *);                         /* a message's width */
struct Frame;
void WindowFrame_Draw(struct Frame *f, s16 *off);                    /* draws a window */
void Font_SetPal(s32);
void Text_Printf(const char *fmt, ...);
void Text_PrintfColored(const char *fmt, ...);
s32 Font_GetPage(void);
void Text_DrawBoxes(s32);
void Font_DrawAllPages(void);

/* The CONNECT screen's texts: the found item's message (gPspValueNames) and
 * the lines of the current step (gPspStepTexts[unk58]), "%a"/"%b" lines
 * replaced by the centred gPspPromptTexts prompts, "%r" drawn as a button.
 * The three loops are copies that differ in where the position is set
 * (the centred cases share one marking loop through a goto).
 * What gave the game's registers: `s = buf;` inside each loop (the
 * out-of-line preheader with buf hoisted), no guards around the loops, the
 * position tables as s16 pairs (gPspStepTextPos + 2 as its own base), the width
 * as `w = Str_Length(..); w = (640.0f - 10.0f * w) / 2.0f;` (w reused as the
 * x), lines loaded before x and y, Hud_DrawButtonIcon's s16 x, y, and
 * an early return when there are no lines (the branch goes to the epilogue).
 * FAKEMATCH: the dead `lines = NULL;` after the help text's lookup (found by
 * the permuter as `lines++; lines--;`) gives lines a web there, which puts
 * lines in s4 and q in s3 in the second and third loops as in the game;
 * without it the two swap (22 words). */
void Psp_DrawTexts(void) {
    char buf[256];
    char **lines;
    char *s;
    char *q;
    s32 x;
    s32 y;
    char **m;
    s32 w;
    s8 e;
    s8 kind;

    if (gPspWork->unk0 == 0xB && gPspWork->unk4 == 2 && gPspWork->unk30 == 0) {
        if (gPspWork->unkFC == 0) {
            WindowFrame_Draw(&gPspFrameB, 0);
        }
        e = gPspWork->unkE4;
        if (e != -1) {
            kind = ((s8 *)gPspIconIndex)[e];
            s = ((char **)gPspValueNames[kind])[gPspWork->unkD8[kind]];
        } else {
            s = ((char **)gPspStepTexts[11])[0];
        }
        lines = NULL;
        if (s != NULL) {
            Font_SetPos(0x32, 0x172);
            Font_SetSpacing(0x14, 0x14);
            Font_SetPal(0);
            Font_SetColor(0x80808080);
            for (;;) {
                q = strchr(s, '%');
                if (!q != 0) {
                    break;
                }
                if (q[1] == 'r') {
                    Hud_DrawButtonIcon(0x19, (q - s) * 10 + 0x32, 0x172, 0x155, -1, 1.0f);
                    q[0] = ' ';
                    q[1] = ' ';
                }
            }
            Text_PrintfColored(s);
        }
    }
    if (gPspWork->unk0 < 0xB && gPspWork->unkFC == 0) {
        WindowFrame_Draw(&gPspFrameA, 0);
    }
    if (gPspWork->unk58 == 0) {
        return;
    }
    Font_SetSpacing(0x14, 0x14);
    Font_SetPal(0);
    Font_SetColor(0x80808080);
    lines = (char **)gPspStepTexts[gPspWork->unk58];
    x = gPspStepTextPos[gPspWork->unk58][0];
    y = gPspStepTextPos[gPspWork->unk58][1];
    for (; *lines != NULL; lines++, y += 0x18) {
        s = buf;
        strcpy(buf, *lines);
        if (buf[0] == '%' && buf[1] == 'a') {
            if (gFrameTick & 0x10) {
                continue;
            }
            m = (char **)gPspPromptTexts[1];
            strcpy(buf, *m);
            w = Str_Length(*m);
            w = (640.0f - 10.0f * w) / 2.0f;
            Font_SetPos(w, y);
            goto mark1;
        } else if (buf[0] == '%' && buf[1] == 'b') {
            if (gFrameTick & 0x10) {
                continue;
            }
            m = (char **)gPspPromptTexts[0];
            strcpy(buf, *m);
            w = Str_Length(*m);
            w = (640.0f - 10.0f * w) / 2.0f;
            Font_SetPos(w, y);
            goto mark1;
        } else if (x == -1) {
            w = Str_Length(*lines);
            w = (640.0f - 10.0f * w) / 2.0f;
            Font_SetPos(w, y);
    mark1:
            for (;;) {
                q = strchr(s, '%');
                if (!q != 0) {
                    break;
                }
                if (q[1] == 'r') {
                    Hud_DrawButtonIcon(0x19, w + (q - s) * 10, y, 0x155, -1, 1.0f);
                    q[0] = ' ';
                    q[1] = ' ';
                }
            }
        } else {
            Font_SetPos(x, y);
            for (;;) {
                q = strchr(s, '%');
                if (!q != 0) {
                    break;
                }
                if (q[1] == 'r') {
                    Hud_DrawButtonIcon(0x19, x + (q - s) * 10, y, 0x155, -1, 1.0f);
                    q[0] = ' ';
                    q[1] = ' ';
                }
            }
        }
        Text_Printf(s);
    }
    if (gPspWork->unk58 == 2 && (gFrameTick & 0x10)) {
        lines = (char **)gPspStepTexts[3];
        x = gPspStepTextPos[3][0];
        y = gPspStepTextPos[3][1];
        for (; *lines != NULL; lines++, y += 0x18) {
            s = buf;
            strcpy(buf, *lines);
            if (buf[0] == '%' && buf[1] == 'a') {
                if (gFrameTick & 0x10) {
                    continue;
                }
                m = (char **)gPspPromptTexts[1];
                strcpy(buf, *m);
                w = Str_Length(*m);
                w = (640.0f - 10.0f * w) / 2.0f;
                Font_SetPos(w, y);
                goto mark2;
            } else if (buf[0] == '%' && buf[1] == 'b') {
                if (gFrameTick & 0x10) {
                    continue;
                }
                m = (char **)gPspPromptTexts[0];
                strcpy(buf, *m);
                w = Str_Length(*m);
                w = (640.0f - 10.0f * w) / 2.0f;
                Font_SetPos(w, y);
                goto mark2;
            } else if (x == -1) {
                w = Str_Length(*lines);
                w = (640.0f - 10.0f * w) / 2.0f;
        mark2:
                for (;;) {
                    q = strchr(s, '%');
                    if (!q != 0) {
                        break;
                    }
                    if (q[1] == 'r') {
                        Hud_DrawButtonIcon(0x19, w + (q - s) * 10, y, 0x155, -1, 1.0f);
                        q[0] = ' ';
                        q[1] = ' ';
                    }
                }
                Font_SetPos(w, y);
            } else {
                for (;;) {
                    q = strchr(s, '%');
                    if (!q != 0) {
                        break;
                    }
                    if (q[1] == 'r') {
                        Hud_DrawButtonIcon(0x19, x + (q - s) * 10, y, 0x155, -1, 1.0f);
                        q[0] = ' ';
                        q[1] = ' ';
                    }
                }
                Font_SetPos(x, y);
            }
            Text_Printf(s);
        }
    }
    if (gPspWork->unk58 == 6 || gPspWork->unk58 == 2) {
        lines = (char **)gPspLines[0];
        x = gPspLinePos[0][0];
        y = gPspLinePos[0][1];
        for (; *lines != NULL; lines++, y += 0x18) {
            s = buf;
            strcpy(buf, *lines);
            if (buf[0] == '%' && buf[1] == 'a') {
                if (gFrameTick & 0x10) {
                    continue;
                }
                m = (char **)gPspPromptTexts[1];
                strcpy(buf, *m);
                w = Str_Length(*m);
                w = (640.0f - 10.0f * w) / 2.0f;
                Font_SetPos(w, y);
                goto mark3;
            } else if (buf[0] == '%' && buf[1] == 'b') {
                if (gFrameTick & 0x10) {
                    continue;
                }
                m = (char **)gPspPromptTexts[0];
                strcpy(buf, *m);
                w = Str_Length(*m);
                w = (640.0f - 10.0f * w) / 2.0f;
                Font_SetPos(w, y);
                goto mark3;
            } else if (x == -1) {
                w = Str_Length(*lines);
                w = (640.0f - 10.0f * w) / 2.0f;
                Font_SetPos(w, y);
        mark3:
                for (;;) {
                    q = strchr(s, '%');
                    if (!q != 0) {
                        break;
                    }
                    if (q[1] == 'r') {
                        Hud_DrawButtonIcon(0x19, w + (q - s) * 10, y, 0x155, -1, 1.0f);
                        q[0] = ' ';
                        q[1] = ' ';
                    }
                }
            } else {
                for (;;) {
                    q = strchr(s, '%');
                    if (!q != 0) {
                        break;
                    }
                    if (q[1] == 'r') {
                        Hud_DrawButtonIcon(0x19, x + (q - s) * 10, y, 0x155, -1, 1.0f);
                        q[0] = ' ';
                        q[1] = ' ';
                    }
                }
                Font_SetPos(x, y);
            }
            Text_Printf(s);
        }
    }
    Text_DrawBoxes(Font_GetPage());
    Font_DrawAllPages();
}

s32 sceDevctl(const char *, s32, void *, s32, void *, s32);  /* devctl */
s32 sceOpen(void *, s32, s32);                              /* open */
s32 sceWrite(s32, void *, s32);                              /* read */
s32 sceRead(s32, void *, s32);                              /* write */
s32 sceClose(s32);                                           /* close */
s32 DelayThread(s32);

/* CONNECT's thread: opens the usbpspcm: device and exchanges one block. */
void Psp_Thread(void *arg) {
    for (;;) {
        switch (gPspState) {
        case 0:
            gPspState = 0;
            gPspWork->unk1C = -1;
            break;
        case 1:
            *(s32 *)gPspDevctlArg = 0;
            if (sceDevctl(D_006D0720, 0x55504302, gPspDevctlArg, 8, gPspDevPath, 0x10) < 0) {
                gPspState = 5;
                break;
            }
            gPspWork->unk1C = sceOpen(gPspDevPath, 3, 0);
            if (gPspWork->unk1C < 0) {
                gPspState = 5;
                break;
            }
            if (sceWrite(gPspWork->unk1C, gPspWork->unk28, 0x40) < 0) {
                if (gPspWork->unk1C >= 0) {
                    sceClose(gPspWork->unk1C);
                }
                gPspState = 5;
                break;
            }
            if (sceRead(gPspWork->unk1C, gPspWork->unk24, 0x40) < 0) {
                if (gPspWork->unk1C >= 0) {
                    sceClose(gPspWork->unk1C);
                }
                gPspState = 5;
                break;
            }
            sceClose(gPspWork->unk1C);
            gPspState = 2;
            break;
        case 4:
            if (gPspWork->unk1C >= 0) {
                sceClose(gPspWork->unk1C);
            }
            gPspState = 0;
            break;
        case 2:
        case 3:
        case 5:
            break;
        }
        DelayThread(100);
    }
}

/* Whether an item of the exchange is unlocked (event flags; the sets are
 * unlocked when one of their items is). */
s32 Psp_IsItemUnlocked(u16 id) {
    switch (id) {
    case 0xA:
        if (Save_TestEventFlag(0x12) == 1) {
            return 1;
        }
        break;
    case 0x9:
        if (Save_TestEventFlag(0x11) == 1) {
            return 1;
        }
        break;
    case 0x8:
        if (Save_TestEventFlag(0xC) == 1) {
            return 1;
        }
        break;
    case 0x7:
        if (Save_TestEventFlag(0xB) == 1) {
            return 1;
        }
        break;
    case 0x6:
        if (Save_TestEventFlag(0x9) == 1) {
            return 1;
        }
        break;
    case 0x5:
        if (Save_TestEventFlag(0x8) == 1) {
            return 1;
        }
        break;
    case 0x4:
        if (Save_TestEventFlag(0x7) == 1) {
            return 1;
        }
        break;
    case 0x3:
        if (Save_TestEventFlag(0x6) == 1) {
            return 1;
        }
        break;
    case 0x2:
        if (Save_TestEventFlag(0x5) == 1) {
            return 1;
        }
        break;
    case 0x1:
        if (Save_TestEventFlag(0x0) == 1) {
            return 1;
        }
        break;
    case 0x33:
        if (Psp_IsItemUnlocked(0xA) != 0) {
            return 1;
        }
        break;
    case 0x32:
        if (Psp_IsItemUnlocked(0x7) != 0) {
            return 1;
        }
        break;
    case 0x31:
        if (Psp_IsItemUnlocked(0x3) != 0) {
            return 1;
        }
        break;
    case 0x30:
        if (Psp_IsItemUnlocked(0x2) != 0) {
            return 1;
        }
        break;
    case 0x2F:
        if (Psp_IsItemUnlocked(0x1) != 0) {
            return 1;
        }
        break;
    case 0x21:
        if (Psp_IsItemUnlocked(0x6) != 0) {
            return 1;
        }
        break;
    case 0x20:
        if (Psp_IsItemUnlocked(0x3) != 0) {
            return 1;
        }
        break;
    case 0x1F:
        if (Psp_IsItemUnlocked(0x1) != 0) {
            return 1;
        }
        break;
    case 0x43:
        if (Psp_IsItemUnlocked(0xA) != 0) {
            return 1;
        }
        break;
    case 0x42:
        if (Psp_IsItemUnlocked(0x7) != 0) {
            return 1;
        }
        break;
    case 0x41:
        if (Psp_IsItemUnlocked(0x3) != 0) {
            return 1;
        }
        break;
    case 0x40:
        if (Psp_IsItemUnlocked(0x2) != 0) {
            return 1;
        }
        break;
    case 0x3F:
        if (Psp_IsItemUnlocked(0x1) != 0) {
            return 1;
        }
        break;
    case 0:
        return 1;
    }
    return 0;
}

/*
 * The overlay's messages (.data 0x6CD7F8-0x6CDD80, .rodata 0x6D0980-0x6D2180):
 * the texts of every screen, as tables of string pointers that all the units
 * use. They come after this unit's own data and strings, so they were either
 * a unit of their own with no code, linked last, or defined at the end of
 * this one; they are kept here because the build needs code in every C file.
 */
/* .data */
u32 gBootTexts[2] = {
#include "select/006CD7F8.inc"
};
u32 gContentWarningText[4] = {
#include "select/006CD800.inc"
};
u32 gCharaPartNames[8] = {
#include "select/006CD810.inc"
};
u32 gNameEntryHelpTexts[4] = {
#include "select/006CD830.inc"
};
u32 gCharaPartHelpTexts[8] = {
#include "select/006CD840.inc"
};
u32 gCharaPartTitles[8] = {
#include "select/006CD860.inc"
};
u32 gRotateTexts[2] = {
#include "select/006CD880.inc"
};
u32 gNameInvalidTexts[2] = {
#include "select/006CD888.inc"
};
u32 gCharaConfirmTexts[2] = {
#include "select/006CD890.inc"
};
u32 gCharaQuestionTexts[2] = {
#include "select/006CD898.inc"
};
u32 gCharaYesNoTexts[2] = {
#include "select/006CD8A0.inc"
};
u32 gCharaYesNoTexts2[2] = {
#include "select/006CD8A8.inc"
};
u32 gCharaEditHelpTexts[2] = {
#include "select/006CD8B0.inc"
};
u32 gCharaSelectTexts[2] = {
#include "select/006CD8B8.inc"
};
u32 gCardSavingTexts[4] = {
#include "select/006CD8C0.inc"
};
u32 gCharaPageTexts[2] = {
#include "select/006CD8D0.inc"
};
u32 gOkTypeTexts[2] = {
#include "select/006CD8D8.inc"
};
u32 gGenderBackTexts[2] = {
#include "select/006CD8E0.inc"
};
u32 gAgeFeatureTexts[2] = {
#include "select/006CD8E8.inc"
};
u32 D_006CD8F0[2] = {
#include "select/006CD8F0.inc"
};
u32 D_006CD8F8[2] = {
#include "select/006CD8F8.inc"
};
u32 gNoMatchTexts[4] = {
#include "select/006CD900.inc"
};
u32 gPartsLabels[40] = {
#include "select/006CD910.inc"
};
u32 gTitleMenuA[8] = {
#include "select/006CD9B0.inc"
};
u32 gTitleMenuB[12] = {
#include "select/006CD9D0.inc"
};
u32 gModeSelTexts[4] = {
#include "select/006CDA00.inc"
};
u32 gTitleHelpMenu[4] = {
#include "select/006CDA10.inc"
};
u32 gTitleHelpTexts[2] = {
#include "select/006CDA20.inc"
};
u32 gPspStepTexts_1[3] = {
#include "select/006CDA28.inc"
};
u32 gPspStepTexts_2[8] = {
#include "select/006CDA40.inc"
};
u32 gPspStepTexts_3[8] = {
#include "select/006CDA60.inc"
};
u32 gPspStepTexts_4[4] = {
#include "select/006CDA80.inc"
};
u32 gPspStepTexts_5[4] = {
#include "select/006CDA90.inc"
};
u32 gPspStepTexts_6[8] = {
#include "select/006CDAA0.inc"
};
u32 gPspStepTexts_7[4] = {
#include "select/006CDAC0.inc"
};
u32 gPspStepTexts_8[4] = {
#include "select/006CDAD0.inc"
};
u32 gPspStepTexts_9[2] = {
#include "select/006CDAE0.inc"
};
u32 gPspStepTexts_10[2] = {
#include "select/006CDAE8.inc"
};
u32 gPspStepTexts_11[2] = {
#include "select/006CDAF0.inc"
};
u32 gPspLines_0[2] = {
#include "select/006CDAF8.inc"
};
u32 gPspLines_1[4] = {
#include "select/006CDB00.inc"
};
u32 gPspPromptTexts_0[2] = {
#include "select/006CDB10.inc"
};
u32 gPspPromptTexts_1[2] = {
#include "select/006CDB18.inc"
};
u32 gPspStepTexts[12] = {
    0x00000000, (u32)gPspStepTexts_1, (u32)gPspStepTexts_2, (u32)gPspStepTexts_3,
    (u32)gPspStepTexts_4, (u32)gPspStepTexts_5, (u32)gPspStepTexts_6, (u32)gPspStepTexts_7,
    (u32)gPspStepTexts_8, (u32)gPspStepTexts_9, (u32)gPspStepTexts_10, (u32)gPspStepTexts_11,
};
u32 gPspLines[2] = {
    (u32)gPspLines_0, (u32)gPspLines_1,
};
u32 gPspPromptTexts[2] = {
    (u32)gPspPromptTexts_0, (u32)gPspPromptTexts_1,
};
u32 gPspValueNames_0[4] = {
#include "select/006CDB60.inc"
};
u32 gPspValueNames_5[4] = {
#include "select/006CDB70.inc"
};
u32 gPspValueNames_2[4] = {
#include "select/006CDB80.inc"
};
u32 gPspValueNames_3[4] = {
#include "select/006CDB90.inc"
};
u32 gPspValueNames_1[4] = {
#include "select/006CDBA0.inc"
};
u32 gPspValueNames_4[4] = {
#include "select/006CDBB0.inc"
};
u32 gPspValueNames[8] = {
    (u32)gPspValueNames_0, (u32)gPspValueNames_1, (u32)gPspValueNames_2, (u32)gPspValueNames_3,
    (u32)gPspValueNames_4, (u32)gPspValueNames_5, 0x00000000, 0x00000000,
};
u32 gOptGameLabels[12] = {
#include "select/006CDBE0.inc"
};
u32 gOptGameHelp[12] = {
#include "select/006CDC10.inc"
};
u32 gOptSoundLabels[8] = {
#include "select/006CDC40.inc"
};
u32 gOptSoundHelp[8] = {
#include "select/006CDC60.inc"
};
u32 gSoundModeTexts[4] = {
#include "select/006CDC80.inc"
};
u32 gVolumeBarTexts[8] = {
#include "select/006CDC90.inc"
};
u32 gOffOnTexts[2] = {
#include "select/006CDCB0.inc"
};
u32 gReverseTexts[3] = {
#include "select/006CDCB8.inc"
};
u32 gSizeTexts[3] = {
#include "select/006CDCC8.inc"
};
u32 gTypeNumTexts[2] = {
#include "select/006CDCD8.inc"
};
u32 gNetOptLabels[8] = {
#include "select/006CDCE0.inc"
};
u32 gNetOptHelp[8] = {
#include "select/006CDD00.inc"
};
u32 gNetOptValueNames_0[4] = {
#include "select/006CDD20.inc"
};
u32 gNetOptValueNames_1[4] = {
#include "select/006CDD30.inc"
};
u32 gNetOptConfirmTexts[8] = {
#include "select/006CDD40.inc"
};
u32 gScreenPosTexts[2] = {
#include "select/006CDD60.inc"
};
u32 gBackResetTexts[2] = {
#include "select/006CDD68.inc"
};
u32 gOptBrightnessTexts[4] = {
#include "select/006CDD70.inc"
};
