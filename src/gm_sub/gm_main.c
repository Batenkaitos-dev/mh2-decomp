/* The quest sub-module's main: a small state machine over a work area
 * (gGmWork, 0xB0 bytes, allocated by the caller) that shows the
 * quest-start screens through the lobby overlay's menu code (0x6B9B10..). */
#include "common.h"
#include "syssettings.h"
#include "pad.h"
#include "scene.h"
#include "save.h"
#include "task.h"
#include "qsync.h"

typedef struct GmWork {
    s32 unk0;                   /* 0x00: the mode */
    s32 unk4;                   /* 0x04: the step within the mode */
    u8 unk8[0x20 - 0x08];
    s32 unk20;                  /* 0x20 */
    s32 unk24;                  /* 0x24: the entry chosen */
    s32 unk28;                  /* 0x28 */
    s32 unk2C;                  /* 0x2C */
    s32 unk30;                  /* 0x30 */
    void *unk34;                /* 0x34 */
    void *unk38;                /* 0x38 */
    u8 unk3C[0x4C - 0x3C];
    s32 unk4C;                  /* 0x4C */
    s32 unk50;                  /* 0x50 */
    u8 unk54[0xB0 - 0x54];
} GmWork;

/* The caller's task (GmSub_Task's argument). */

GmWork *gGmWork;

void D_6B5230(TaskSlot *t); /* select Boot_Task */
extern u8 D_6BA0C0[];
extern u8 D_6BA810[];
extern u8 D_6BF5A0[];

void Sound_ReverbOn(void);
void Stream_Play(s32, s32);
void Sound_LoadGmSubBanks(void);
void Gs_SetTableColor(s32);
u8 FadeA_GetState(void);
void FadeA_Start(s32);
void Tex_LoadSystemPack(s32);
void TaskSlot_Set(void *func, s16 slot);
void TaskSlot_End(TaskSlot *t);
void TaskSlot_Hold(s16 slot);
void Task_EndGmSubForNetScene(void);
void Task_EndGmSubForNetSetup(void);
u16 Pad_KeyRepeat(s32 pad, u16 held, u16 pressed);
void Game_ResetSubsystems(void);
void Sound_PlayCursorMove(void);
void Title_ResetBgAnim(void);
void Title_StepBgAnim(void);
void ModeSel_StartSlide(void);
void ModeSel_Start(s32); /* unit's own declaration: the definition (select) is void ModeSel_Start(s16 sel); the calls here match only with this one */
s32 ModeSel_IsIdle(void);
void ModeSel_UpdateSlide(void);
void Sel_LoadCommonTextures(void);
void GmSub_MoveCursorH(u16, u16, u8 *, u8);
void *memset(void *dst, s32 c, u32 n);
void gm_sub_entry(void);
s32 GmSub_Setup(void);
s32 GmSub_UpdateMenu(u16, u16);
s32 GmSub_StepLaunch(TaskSlot *, u16, u16);
s32 GmSub_StepExit(TaskSlot *, u16, u16);
void Callbacks_Add(void (*fn)(void));
void LayerCallbacks_Add(void (*fn)(void), u8 layer);
void Camera_Update(void);
void Frame_Draw(void);
extern u8 D_6B1EA0[];
extern u8 D_6B3B50[];
extern u8 D_6B6840[];
extern u8 D_6B8C50[];
extern u8 D_6C2E60[];
extern u8 D_6B9CF0[];
void Title_RandomView(void);
void Sound_PlaySe73(void);
void Sound_PlayCancel(void);
void Stream_Stop(s32);
TaskSlot *TaskSlot_Get(s16 slot);
void Task_StartBootScreens(void);
void GmSub_InitMenu(void);
void GmSub_MoveCursorV(u16, u16, u8 *, u8);
s32 GmSub_UpdateYesNo(u16, u16);
void GmSub_OpenYesNo(void);

void gm_sub_entry(void) {
    gGmWork->unk0 = 1;
    gGmWork->unk4 = 0xB;
    gGmWork->unk20 = 1;
    gGmWork->unk24 = 2;
    gGmWork->unk28 = 0;
    gGmWork->unk2C = 2;
    gGmWork->unk30 = -1;
    gGmWork->unk4C = 0;
    gGmWork->unk50 = 4;
    Game_ResetSubsystems();
    Tex_LoadSystemPack(0);
    Sel_LoadCommonTextures();
    TaskSlot_Set(D_6BF5A0, 4);
    TaskSlot_Hold(2);
}

void GmSub_OpenYesNo(void) {
    if (gGmWork != NULL) {
        gGmWork->unk38 = D_6BA810;
        Title_ResetBgAnim();
        if ((gGmWork->unk24 == 1) && (gSave.unk20D9 != 0)) {
            gGmWork->unk28 = 1;
        } else {
            gGmWork->unk28 = 0;
        }
        ModeSel_Start(gGmWork->unk28);
        gGmWork->unk4 = 0xC;
    }
}

void GmSub_Task(TaskSlot *t) {
    u16 a = gPadState[0].held;
    u16 b = gPadState[0].pressed;

    switch (t->mode) {
    case 0:
        gGmWork = (GmWork *)gSysSettings.workArea;
        memset(gGmWork, 0, 0xB0);
        if (gSysSettings.galleryPlay == 1) {
            gm_sub_entry();
            return;
        }
        t->mode++;
    case 1:
        switch (gGmWork->unk0) {
        case 0:
            if (GmSub_Setup() != 0) {
                gGmWork->unk0++;
                gGmWork->unk4 = 0;
            }
            break;
        case 1:
            GmSub_UpdateMenu(a, b);
            break;
        case 2:
            switch (gGmWork->unk24) {
            case 0:
                gSysSettings.systemSaved = 0;
                TaskSlot_Set(D_6B1EA0, 4);
                TaskSlot_Hold(2);
                gGmWork->unk0 = 1;
                gGmWork->unk4 = 0xB;
                break;
            case 1:
                if (gGmWork->unk30 >= 0) {
                    t->mode = 3;
                } else {
                    gSysSettings.systemSaved = 0;
                    TaskSlot_Set(D_6B3B50, 4);
                    TaskSlot_Hold(2);
                    gGmWork->unk0 = 1;
                    gGmWork->unk4 = 0xB;
                }
                break;
            }
            break;
        case 3:
            switch (gGmWork->unk24) {
            case 3:
                TaskSlot_Set(D_6B6840, 4);
            common3:
                TaskSlot_Hold(2);
                gGmWork->unk0 = 1;
                gGmWork->unk4 = 0xA;
                break;
            case 4:
                TaskSlot_Set(D_6B8C50, 4);
                goto common3;
            case 2:
                TaskSlot_Set(D_6BF5A0, 4);
                TaskSlot_Hold(2);
                gGmWork->unk0 = 1;
                gGmWork->unk4 = 0xB;
                break;
            case 5:
                TaskSlot_Set(D_6C2E60, 5);
                TaskSlot_Hold(2);
                gGmWork->unk0 = 1;
                gGmWork->unk4 = 0xA;
                break;
            }
            break;
        }
    common:
        if (gGmWork->unk38 != D_6BA810 && gGmWork->unk34 != NULL) {
            Callbacks_Add(gGmWork->unk34);
        }
        if (gGmWork->unk38 != NULL) {
            LayerCallbacks_Add(gGmWork->unk38, 1);
        }
        break;
    case 3:
        if (GmSub_StepLaunch(t, a, b) == 1) {
            return;
        }
        goto common;
    case 4:
        if (GmSub_StepExit(t, a, b) == 1) {
            return;
        }
        if (t->count <= 1) {
            goto common;
        }
        break;
    }
    if (t->mode != 0) {
        Camera_Update();
        Callbacks_Add(Frame_Draw);
    }
}

/* Mode 0: resets the game subsystems and turns the reverb on. */
s32 GmSub_Setup(void) {
    Gs_SetTableColor(0);
    Sound_ReverbOn();
    Game_ResetSubsystems();
    return 1;
}

void GmSub_InitMenu(void) {
    Game_ResetSubsystems();
    Tex_LoadSystemPack(0);
    Sel_LoadCommonTextures();
    Sound_LoadGmSubBanks();
    Gs_SetTableColor(0);
    if (gSysSettings.cardHasChara != 0) {
        gGmWork->unk24 = 1;
    }
    if (gSysSettings.galleryPlay == 2) {
        gGmWork->unk24 = 5;
    }
    gSysSettings.galleryPlay = 0;
    gGmWork->unk34 = D_6BA0C0;
    gGmWork->unk38 = NULL;
    gGmWork->unk2C = -1;
    gGmWork->unk30 = -1;
    if (gSysSettings.bootMode != 0) {
        gSysSettings.bootMode = 0;
        gGmWork->unk24 = 1;
        gGmWork->unk20 = 1;
        gGmWork->unk50 = 4;
        gGmWork->unk4 = 5;
        gGmWork->unk30 = -1;
        if (gSave.unk20D9 != 0) {
            gGmWork->unk28 = 1;
        } else {
            gGmWork->unk28 = 0;
        }
        ModeSel_Start(gGmWork->unk28);
        gGmWork->unk38 = D_6BA810;
        return;
    }
    Stream_Play(0, 0x2F);
}

void GmSub_MoveCursorV(u16 a, u16 b, u8 *sel, u8 n) {
    u16 key = Pad_KeyRepeat(0, a, b);

    switch (key) {
    case 0x2000:
        if (*sel == 0) {
            *sel = n - 1;
        } else {
            *sel = *sel - 1;
        }
        Sound_PlayCursorMove();
        break;
    case 0x1000:
        if (*sel == n - 1) {
            *sel = 0;
        } else {
            *sel = *sel + 1;
        }
        Sound_PlayCursorMove();
        break;
    }
}

void GmSub_MoveCursorH(u16 a, u16 b, u8 *sel, u8 n) {
    u16 key = Pad_KeyRepeat(0, a, b);

    switch (key) {
    case 0x400:
        if (*sel != 0) {
            *sel = *sel - 1;
            Sound_PlayCursorMove();
            ModeSel_StartSlide();
        }
        break;
    case 0x800:
        if (*sel != n - 1) {
            *sel = *sel + 1;
            Sound_PlayCursorMove();
            ModeSel_StartSlide();
        }
        break;
    }
}

extern u8 gOnlineBonusState;
s32 GmSub_UpdateMenu(u16 a, u16 b) {
    s32 old;

    switch (gGmWork->unk4) {
    case 0:
        gGmWork->unk4++;
        GmSub_InitMenu();
        FadeA_Start(0xC);
        break;
    case 1:
        if ((u8)FadeA_GetState() != 1) {
            gGmWork->unk4 = 2;
            if (gGmWork->unk20 != 0) {
                if (gGmWork->unk2C == 1) {
                    gGmWork->unk4 = 5;
                    gGmWork->unk30 = -1;
                    gGmWork->unk38 = D_6BA810;
                } else {
                    gGmWork->unk4 = 3;
                    gGmWork->unk38 = D_6B9CF0;
                    Title_ResetBgAnim();
                }
                gGmWork->unk2C = -1;
            } else {
                gGmWork->unk50 = 0;
                Title_RandomView();
            }
        }
        break;
    case 2:
        if (gGmWork->unk50 >= 4) {
            if (gGmWork->unk2C == 1) {
                ModeSel_Start(0);
                gGmWork->unk4 = 5;
                gGmWork->unk30 = -1;
                gGmWork->unk38 = D_6BA810;
            } else {
                gGmWork->unk4 = 3;
                gGmWork->unk38 = D_6B9CF0;
                Title_ResetBgAnim();
            }
            gGmWork->unk20 = 1;
            gGmWork->unk2C = -1;
        } else {
            gGmWork->unk50++;
        }
        break;
    case 3:
        switch (b) {
        case 0x20:
        case 0x8000:
            gGmWork->unk4++;
            Sound_PlaySe73();
            gGmWork->unk2C = gGmWork->unk24;
            switch (gGmWork->unk24) {
            case 0:
                FadeA_Start(0xB);
                break;
            case 1:
                FadeA_Start(0xB);
                break;
            case 2:
            case 3:
            case 4:
                FadeA_Start(0xB);
                break;
            }
            break;
        case 0x40:
            Sound_PlayCancel();
            gGmWork->unk4 = 0x14;
            FadeA_Start(0xB);
            break;
        default:
            old = gGmWork->unk24;
            GmSub_MoveCursorV(a, b, (u8 *)&gGmWork->unk24, 6);
            if (old != gGmWork->unk24) {
                Title_ResetBgAnim();
            } else {
                Title_StepBgAnim();
            }
            break;
        }
        break;
    case 4:
        if ((u8)FadeA_GetState() != 1) {
            Stream_Stop(0);
            switch (gGmWork->unk24) {
            case 0:
                gGmWork->unk0 = 2;
                if (gGmWork->unk30 < 0) {
                    gQuestSync.charIndex = 0;
                }
                break;
            case 1:
                gGmWork->unk0 = 2;
                if (gGmWork->unk30 < 0) {
                    gQuestSync.charIndex = 0;
                }
                break;
            case 2:
            case 3:
            case 4:
            default:
                gGmWork->unk0 = 3;
                break;
            }
        }
        break;
    case 5:
        if ((u8)FadeA_GetState() != 1) {
            switch (GmSub_UpdateYesNo(a, b)) {
            case 1:
                gGmWork->unk30 = gGmWork->unk28;
                gSave.unk20D9 = gGmWork->unk28;
                Sound_PlaySe73();
                gOnlineBonusState = 0;
                gGmWork->unk4 = 4;
                FadeA_Start(5);
                break;
            case -1:
                Sound_PlayCancel();
                gGmWork->unk4 = 6;
                FadeA_Start(5);
                break;
            }
        }
        break;
    case 6:
        if ((u8)FadeA_GetState() != 1) {
            gGmWork->unk4 = 0xB;
            gGmWork->unk38 = D_6B9CF0;
            Title_ResetBgAnim();
            gSysSettings.bootMode = 1;
            Stream_Play(0, 0x2F);
        }
        break;
    case 10:
        FadeA_Start(0xC);
        gGmWork->unk4 = 1;
        gGmWork->unk20 = 1;
        gGmWork->unk50 = 4;
        Stream_Play(0, 0x2F);
        break;
    case 12:
        GmSub_OpenYesNo();
        Game_ResetSubsystems();
        Tex_LoadSystemPack(0);
        FadeA_Start(0xC);
        gGmWork->unk4 = 1;
        gGmWork->unk20 = 1;
        gGmWork->unk50 = 4;
    case 11:
        gSysSettings.systemSaved = 0;
        gGmWork->unk2C = -1;
        gGmWork->unk30 = -1;
        Game_ResetSubsystems();
        Tex_LoadSystemPack(0);
        FadeA_Start(0xC);
        gGmWork->unk4 = 1;
        gGmWork->unk20 = 1;
        gGmWork->unk50 = 4;
        Title_RandomView();
        Stream_Play(0, 0x2F);
        gGmWork->unk34 = D_6BA0C0;
        break;
    case 20:
        if ((u8)FadeA_GetState() != 1) {
            TaskSlot_End(TaskSlot_Get(2));
            Task_StartBootScreens();
        }
        break;
    }
    return 0;
}

/* Phase 3: once the fade is over, sets the game flags from the yes/no answer and ends the task. */
s32 GmSub_StepLaunch(TaskSlot *t, u16 a, u16 b) {
    switch (gGmWork->unk24) {
    case 0:
    case 1:
        if ((u8)FadeA_GetState() != 1) {
            gSysSettings.galleryPlay = 0;
            gSysSettings.unk45 = 0;
            switch (gGmWork->unk28) {
            case 0:
                gSysSettings.online = 1;
                gSysSettings.useSavedEquip = 0;
                break;
            case 1:
                gSysSettings.online = 0;
                gSysSettings.useSavedEquip = 0;
                break;
            case 2:
                gSysSettings.useSavedEquip = 1;
                gSysSettings.online = 0;
                break;
            }
            gSysSettings.unk02 = 0;
            gSysSettings.unk03 = 1;
            gSysSettings.unk05 = 0;
            TaskSlot_End(t);
            if (gSysSettings.online == 0) {
                Task_EndGmSubForNetScene();
            } else {
                Task_EndGmSubForNetSetup();
            }
        }
        return 1;
    }
    return 0;
}

/* Phase 4: fades out, then ends the task and starts the task D_6B5230 in slot 3 (0x6ADF94 = 1). */
s32 GmSub_StepExit(TaskSlot *t, u16 a, u16 b) {
    switch (t->count) {
    case 0:
        t->count++;
        FadeA_Start(0xB);
        break;
    case 1:
        if ((u8)FadeA_GetState() != 1) {
            t->count++;
        }
        break;
    case 2:
        Game_ResetSubsystems();
        TaskSlot_End(t);
        gSysSettings.bootFrom = 1;
        TaskSlot_Set(D_6B5230, 3);
        return 1;
    }
    return 0;
}

s32 GmSub_UpdateYesNo(u16 a, u16 b) {
    s32 old = gGmWork->unk28;

    ModeSel_UpdateSlide();
    if (ModeSel_IsIdle() != 0) {
        if (b & 0x40) {
            return -1;
        }
        if (b & 0x20) {
            return 1;
        }
        GmSub_MoveCursorH(a, b, (u8 *)&gGmWork->unk28, 2);
    }
    if (old != gGmWork->unk28) {
        Title_ResetBgAnim();
    } else {
        Title_StepBgAnim();
    }
    return 0;
}

/* 1 when the scene flag 0x6ADFD8 is 1 or the entry chosen is 1 (select's character screens test it). */
s32 GmSub_IsCharaSelectMenu(void) {
    if (gSysSettings.pspCharaWait == 1) {
        return 1;
    }
    if (gGmWork->unk24 == 1) {
        return 1;
    }
    return 0;
}
