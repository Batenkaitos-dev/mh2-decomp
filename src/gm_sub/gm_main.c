/*
 * gm_sub's main: the title screen and its menu. GmSub_Task runs in
 * TASK_SLOT_GMSUB over gGmWork (select.h's GmWork, in
 * gSysSettings.workArea), which select's title drawing (select_title.c)
 * reads. Each of the menu's six entries (enum GmSubMenuEntry) starts one
 * of select's tasks while this task is held, and the title screen comes
 * back when it ends. The load entry goes on to mode select (online or
 * offline), and the game then starts in the lobby's network setup or its
 * scene (GmSub_StartGame). Cancelling the menu goes back to the boot
 * screens.
 */
#include "common.h"
#include "syssettings.h"
#include "pad.h"
#include "scene.h"
#include "save.h"
#include "task.h"
#include "qsync.h"
#include "select.h"
#include "sound.h"
#include "stream.h"
#include "text.h"
#include "gm_main.h"
#include "camera.h"
#include "file_load.h"
#include "frame.h"
#include "game_util.h"
#include "rand.h"
#include "sys_main.h"
#include "screen_fade.h"
#include "bgm.h"
#include "libc.h"


/* The title screen's state: gSysSettings.workArea, set by GmSub_Task. */
GmWork *gGmWork;

void ModeSel_Start(s32); /* unit's own declaration: the definition (select) is void ModeSel_Start(s16 sel); the calls here match only with this one */
s32 ModeSel_IsIdle(void);

/*
 * The overlay's entry, at its first address. GmSub_Task calls it after a
 * gallery scene (gSysSettings.galleryPlay 1): the title menu is set on
 * GMSUB_MENU_GALLERY and Gallery_Task starts again in TASK_SLOT_MENU while
 * this task is held; the title screen comes back as GMSUB_STEP_RESET.
 */
void gm_sub_entry(void)
{
    gGmWork->mode = GMSUB_MODE_MENU;
    gGmWork->step = GMSUB_STEP_RESET;
    gGmWork->logoFull = 1;
    gGmWork->menuSel = GMSUB_MENU_GALLERY;
    gGmWork->sel = GMSUB_MODE_SEL_ONLINE;
    gGmWork->menuChoice = GMSUB_MENU_GALLERY;
    gGmWork->modeChoice = -1;
    gGmWork->unk4C = 0;
    gGmWork->logoAnim = GMSUB_LOGO_ANIM_END;
    Game_ResetSubsystems();
    Tex_LoadSystemPack(0);
    Sel_LoadCommonTextures();
    TaskSlot_Set(Gallery_Task, TASK_SLOT_MENU);
    TaskSlot_Hold(TASK_SLOT_GMSUB);
}

/*
 * Opens mode select over the title screen, its cursor on the choice saved
 * last time (SaveData.modeSelChoice) for GMSUB_MENU_LOAD_CHARA, else on
 * online, and sets GMSUB_STEP_REOPEN_MODE_SELECT. Only that step calls it,
 * and only it sets the step: no code seen reaches it.
 */
void GmSub_OpenModeSelect(void)
{
    if (gGmWork != NULL) {
        gGmWork->layerCallback = ModeSel_DrawScreen;
        Title_ResetBgAnim();
        if ((gGmWork->menuSel == GMSUB_MENU_LOAD_CHARA) && (gSave.modeSelChoice != 0)) {
            gGmWork->sel = GMSUB_MODE_SEL_OFFLINE;
        } else {
            gGmWork->sel = GMSUB_MODE_SEL_ONLINE;
        }
        ModeSel_Start(gGmWork->sel);
        gGmWork->step = GMSUB_STEP_REOPEN_MODE_SELECT;
    }
}

/*
 * gm_sub's task, in TASK_SLOT_GMSUB: the title screen. The first frame
 * clears gGmWork, or goes straight back to the gallery after a gallery
 * scene (gm_sub_entry). Then each frame runs GmWork.mode's phase: the
 * set-up, the menus (GmSub_UpdateMenu), or the start of the chosen entry,
 * whose select task runs while this one is held (TaskSlot_Hold). The frame
 * ends by queueing the logo (unless mode select is drawn) and the screen's
 * layer drawing, updating the camera and drawing the frame. The two gotos
 * are the game's shared tails.
 */
void GmSub_Task(TaskSlot *task)
{
    u16 padHeld = gPadState[0].held;
    u16 padPressed = gPadState[0].pressed;

    switch (task->mode) {
    case GMSUB_TASK_INIT:
        gGmWork = (GmWork *)gSysSettings.workArea;
        memset(gGmWork, 0, sizeof(GmWork));
        if (gSysSettings.galleryPlay == 1) {
            gm_sub_entry();
            return;
        }
        task->mode++; /* GMSUB_TASK_RUN */
    case GMSUB_TASK_RUN:
        switch (gGmWork->mode) {
        case GMSUB_MODE_SETUP:
            if (GmSub_Setup() != 0) {
                gGmWork->mode++; /* GMSUB_MODE_MENU */
                gGmWork->step = GMSUB_STEP_INIT;
            }
            break;
        case GMSUB_MODE_MENU:
            GmSub_UpdateMenu(padHeld, padPressed);
            break;
        case GMSUB_MODE_START_CHARA:
            switch (gGmWork->menuSel) {
            case GMSUB_MENU_NEW_CHARA:
                gSysSettings.systemSaved = 0;
                TaskSlot_Set(Chara_CreateTask, TASK_SLOT_MENU);
                TaskSlot_Hold(TASK_SLOT_GMSUB);
                gGmWork->mode = GMSUB_MODE_MENU;
                gGmWork->step = GMSUB_STEP_RESET;
                break;
            case GMSUB_MENU_LOAD_CHARA:
                if (gGmWork->modeChoice >= 0) {
                    task->mode = GMSUB_TASK_LAUNCH;
                } else {
                    gSysSettings.systemSaved = 0;
                    TaskSlot_Set(Chara_SelectTask, TASK_SLOT_MENU);
                    TaskSlot_Hold(TASK_SLOT_GMSUB);
                    gGmWork->mode = GMSUB_MODE_MENU;
                    gGmWork->step = GMSUB_STEP_RESET;
                }
                break;
            }
            break;
        case GMSUB_MODE_START_ENTRY:
            switch (gGmWork->menuSel) {
            case GMSUB_MENU_OPTIONS:
                TaskSlot_Set(Option_Task, TASK_SLOT_MENU);
            entry_started:
                TaskSlot_Hold(TASK_SLOT_GMSUB);
                gGmWork->mode = GMSUB_MODE_MENU;
                gGmWork->step = GMSUB_STEP_BACK;
                break;
            case GMSUB_MENU_NET_OPTIONS:
                TaskSlot_Set(NetOption_Task, TASK_SLOT_MENU);
                goto entry_started;
            case GMSUB_MENU_GALLERY:
                TaskSlot_Set(Gallery_Task, TASK_SLOT_MENU);
                TaskSlot_Hold(TASK_SLOT_GMSUB);
                gGmWork->mode = GMSUB_MODE_MENU;
                gGmWork->step = GMSUB_STEP_RESET;
                break;
            case GMSUB_MENU_PSP_LINK:
                TaskSlot_Set(Psp_ConnectTask, TASK_SLOT_SCENE);
                TaskSlot_Hold(TASK_SLOT_GMSUB);
                gGmWork->mode = GMSUB_MODE_MENU;
                gGmWork->step = GMSUB_STEP_BACK;
                break;
            }
            break;
        }
    queue_draw:
        if (gGmWork->layerCallback != ModeSel_DrawScreen && gGmWork->frameCallback != NULL) {
            Callbacks_Add(gGmWork->frameCallback);
        }
        if (gGmWork->layerCallback != NULL) {
            LayerCallbacks_Add(gGmWork->layerCallback, 1);
        }
        break;
    case GMSUB_TASK_LAUNCH:
        if (GmSub_StartGame(task, padHeld, padPressed) == 1) {
            return;
        }
        goto queue_draw;
    case GMSUB_TASK_EXIT:
        if (GmSub_ExitToBoot(task, padHeld, padPressed) == 1) {
            return;
        }
        if (task->count <= 1) {
            goto queue_draw;
        }
        break;
    }
    if (task->mode != GMSUB_TASK_INIT) {
        Camera_Update();
        Callbacks_Add(Frame_Draw);
    }
}

/*
 * GMSUB_MODE_SETUP: sets the GS table colour, turns the reverb on and
 * resets the game's subsystems. Returns 1: done.
 */
s32 GmSub_Setup(void)
{
    Gs_SetTableColor(0);
    Sound_ReverbOn();
    Game_ResetSubsystems();
    return 1;
}

/*
 * Sets the title menu up (GMSUB_STEP_INIT): resets the subsystems, loads
 * the system and common textures and gm_sub's sound banks. The cursor
 * starts on GMSUB_MENU_LOAD_CHARA when the memory card holds a character,
 * and on GMSUB_MENU_PSP_LINK back from the PSP link (galleryPlay 2). When
 * select's boot asked for gm_sub (gSysSettings.bootMode), mode select opens
 * at once; otherwise the title screen's stream starts.
 */
void GmSub_InitMenu(void)
{
    Game_ResetSubsystems();
    Tex_LoadSystemPack(0);
    Sel_LoadCommonTextures();
    Sound_LoadTitleMenuBanks();
    Gs_SetTableColor(0);
    if (gSysSettings.cardHasChara != 0) {
        gGmWork->menuSel = GMSUB_MENU_LOAD_CHARA;
    }
    if (gSysSettings.galleryPlay == 2) {
        gGmWork->menuSel = GMSUB_MENU_PSP_LINK;
    }
    gSysSettings.galleryPlay = 0;
    gGmWork->frameCallback = Title_DrawLogo;
    gGmWork->layerCallback = NULL;
    gGmWork->menuChoice = -1;
    gGmWork->modeChoice = -1;
    if (gSysSettings.bootMode != BOOT_MODE_SCREENS) {
        gSysSettings.bootMode = BOOT_MODE_SCREENS;
        gGmWork->menuSel = GMSUB_MENU_LOAD_CHARA;
        gGmWork->logoFull = 1;
        gGmWork->logoAnim = GMSUB_LOGO_ANIM_END;
        gGmWork->step = GMSUB_STEP_MODE_SELECT;
        gGmWork->modeChoice = -1;
        if (gSave.modeSelChoice != 0) {
            gGmWork->sel = GMSUB_MODE_SEL_OFFLINE;
        } else {
            gGmWork->sel = GMSUB_MODE_SEL_ONLINE;
        }
        ModeSel_Start(gGmWork->sel);
        gGmWork->layerCallback = ModeSel_DrawScreen;
        return;
    }
    Stream_Play(0, GMSUB_TITLE_STREAM);
}

/*
 * Moves a vertical menu's cursor *sel up or down among count entries on
 * the pad's key repeat, wrapping round, with the cursor sound. held and
 * pressed are the pad's buttons.
 */
void GmSub_MoveCursorV(u16 held, u16 pressed, u8 *sel, u8 count)
{
    u16 key = Pad_KeyRepeat(0, held, pressed);

    switch (key) {
    case PAD_UP:
        if (*sel == 0) {
            *sel = count - 1;
        } else {
            *sel = *sel - 1;
        }
        Sound_PlayCursorMove();
        break;
    case PAD_DOWN:
        if (*sel == count - 1) {
            *sel = 0;
        } else {
            *sel = *sel + 1;
        }
        Sound_PlayCursorMove();
        break;
    }
}

/*
 * Moves mode select's cursor *sel among count entries on the pad's key
 * repeat, without wrapping: PAD_RIGHT to the entry before, PAD_LEFT to the
 * one after. Each move plays the cursor sound and slides the entries
 * (ModeSel_StartSlide).
 */
void GmSub_MoveCursorH(u16 held, u16 pressed, u8 *sel, u8 count)
{
    u16 key = Pad_KeyRepeat(0, held, pressed);

    switch (key) {
    case PAD_RIGHT:
        if (*sel != 0) {
            *sel = *sel - 1;
            Sound_PlayCursorMove();
            ModeSel_StartSlide();
        }
        break;
    case PAD_LEFT:
        if (*sel != count - 1) {
            *sel = *sel + 1;
            Sound_PlayCursorMove();
            ModeSel_StartSlide();
        }
        break;
    }
}

extern u8 gPoogieErrand;
/*
 * The title screen's steps (GmWork.step, enum GmSubStep): the fade-in and
 * the logo's animation, the title menu, mode select, and the ways back
 * from the entries' tasks. Choosing an entry fades out and sets GmWork.mode
 * to start it; PAD_CROSS on the title menu goes back to the boot screens.
 * FadeA_GetState returns 1 while the fade runs. held and pressed are the
 * pad's buttons. Returns 0.
 */
s32 GmSub_UpdateMenu(u16 held, u16 pressed)
{
    s32 prevSel;

    switch (gGmWork->step) {
    case GMSUB_STEP_INIT:
        gGmWork->step++; /* GMSUB_STEP_FADE_IN */
        GmSub_InitMenu();
        FadeA_Start(FADE_IN_BLACK_FAST);
        break;
    case GMSUB_STEP_FADE_IN:
        if ((u8)FadeA_GetState() != 1) {
            gGmWork->step = GMSUB_STEP_LOGO;
            if (gGmWork->logoFull != 0) {
                if (gGmWork->menuChoice == GMSUB_MENU_LOAD_CHARA) {
                    gGmWork->step = GMSUB_STEP_MODE_SELECT;
                    gGmWork->modeChoice = -1;
                    gGmWork->layerCallback = ModeSel_DrawScreen;
                } else {
                    gGmWork->step = GMSUB_STEP_MENU;
                    gGmWork->layerCallback = Title_Draw;
                    Title_ResetBgAnim();
                }
                gGmWork->menuChoice = -1;
            } else {
                gGmWork->logoAnim = 0;
                Title_RandomView();
            }
        }
        break;
    case GMSUB_STEP_LOGO:
        if (gGmWork->logoAnim >= GMSUB_LOGO_ANIM_END) {
            if (gGmWork->menuChoice == GMSUB_MENU_LOAD_CHARA) {
                ModeSel_Start(0);
                gGmWork->step = GMSUB_STEP_MODE_SELECT;
                gGmWork->modeChoice = -1;
                gGmWork->layerCallback = ModeSel_DrawScreen;
            } else {
                gGmWork->step = GMSUB_STEP_MENU;
                gGmWork->layerCallback = Title_Draw;
                Title_ResetBgAnim();
            }
            gGmWork->logoFull = 1;
            gGmWork->menuChoice = -1;
        } else {
            gGmWork->logoAnim++;
        }
        break;
    case GMSUB_STEP_MENU:
        switch (pressed) {
        case PAD_CIRCLE:
        case PAD_START:
            gGmWork->step++; /* GMSUB_STEP_CHOSEN */
            Sound_PlayConfirm();
            gGmWork->menuChoice = gGmWork->menuSel;
            switch (gGmWork->menuSel) {
            case GMSUB_MENU_NEW_CHARA:
                FadeA_Start(FADE_OUT_BLACK_FAST);
                break;
            case GMSUB_MENU_LOAD_CHARA:
                FadeA_Start(FADE_OUT_BLACK_FAST);
                break;
            case GMSUB_MENU_GALLERY:
            case GMSUB_MENU_OPTIONS:
            case GMSUB_MENU_NET_OPTIONS:
                FadeA_Start(FADE_OUT_BLACK_FAST);
                break;
            }
            break;
        case PAD_CROSS:
            Sound_PlayCancel();
            gGmWork->step = GMSUB_STEP_QUIT;
            FadeA_Start(FADE_OUT_BLACK_FAST);
            break;
        default:
            prevSel = gGmWork->menuSel;
            GmSub_MoveCursorV(held, pressed, (u8 *)&gGmWork->menuSel, GMSUB_MENU_COUNT);
            if (prevSel != gGmWork->menuSel) {
                Title_ResetBgAnim();
            } else {
                Title_StepBgAnim();
            }
            break;
        }
        break;
    case GMSUB_STEP_CHOSEN:
        if ((u8)FadeA_GetState() != 1) {
            Stream_Stop(0);
            switch (gGmWork->menuSel) {
            case GMSUB_MENU_NEW_CHARA:
                gGmWork->mode = GMSUB_MODE_START_CHARA;
                if (gGmWork->modeChoice < 0) {
                    gQuestSync.charIndex = 0;
                }
                break;
            case GMSUB_MENU_LOAD_CHARA:
                gGmWork->mode = GMSUB_MODE_START_CHARA;
                if (gGmWork->modeChoice < 0) {
                    gQuestSync.charIndex = 0;
                }
                break;
            case GMSUB_MENU_GALLERY:
            case GMSUB_MENU_OPTIONS:
            case GMSUB_MENU_NET_OPTIONS:
            default:
                gGmWork->mode = GMSUB_MODE_START_ENTRY;
                break;
            }
        }
        break;
    case GMSUB_STEP_MODE_SELECT:
        if ((u8)FadeA_GetState() != 1) {
            switch (GmSub_UpdateModeSelect(held, pressed)) {
            case 1:
                gGmWork->modeChoice = gGmWork->sel;
                gSave.modeSelChoice = gGmWork->sel;
                Sound_PlayConfirm();
                gPoogieErrand = 0;
                gGmWork->step = GMSUB_STEP_CHOSEN;
                FadeA_Start(FADE_OUT_BLACK_SLOW);
                break;
            case -1:
                Sound_PlayCancel();
                gGmWork->step = GMSUB_STEP_MODE_CANCEL;
                FadeA_Start(FADE_OUT_BLACK_SLOW);
                break;
            }
        }
        break;
    case GMSUB_STEP_MODE_CANCEL:
        if ((u8)FadeA_GetState() != 1) {
            gGmWork->step = GMSUB_STEP_RESET;
            gGmWork->layerCallback = Title_Draw;
            Title_ResetBgAnim();
            gSysSettings.bootMode = BOOT_MODE_GMSUB;
            Stream_Play(0, GMSUB_TITLE_STREAM);
        }
        break;
    case GMSUB_STEP_BACK:
        FadeA_Start(FADE_IN_BLACK_FAST);
        gGmWork->step = GMSUB_STEP_FADE_IN;
        gGmWork->logoFull = 1;
        gGmWork->logoAnim = GMSUB_LOGO_ANIM_END;
        Stream_Play(0, GMSUB_TITLE_STREAM);
        break;
    case GMSUB_STEP_REOPEN_MODE_SELECT:
        GmSub_OpenModeSelect();
        Game_ResetSubsystems();
        Tex_LoadSystemPack(0);
        FadeA_Start(FADE_IN_BLACK_FAST);
        gGmWork->step = GMSUB_STEP_FADE_IN;
        gGmWork->logoFull = 1;
        gGmWork->logoAnim = GMSUB_LOGO_ANIM_END;
    case GMSUB_STEP_RESET:
        gSysSettings.systemSaved = 0;
        gGmWork->menuChoice = -1;
        gGmWork->modeChoice = -1;
        Game_ResetSubsystems();
        Tex_LoadSystemPack(0);
        FadeA_Start(FADE_IN_BLACK_FAST);
        gGmWork->step = GMSUB_STEP_FADE_IN;
        gGmWork->logoFull = 1;
        gGmWork->logoAnim = GMSUB_LOGO_ANIM_END;
        Title_RandomView();
        Stream_Play(0, GMSUB_TITLE_STREAM);
        gGmWork->frameCallback = Title_DrawLogo;
        break;
    case GMSUB_STEP_QUIT:
        if ((u8)FadeA_GetState() != 1) {
            TaskSlot_End(TaskSlot_Get(TASK_SLOT_GMSUB));
            Task_StartBootScreens();
        }
        break;
    }
    return 0;
}

/*
 * GMSUB_TASK_LAUNCH: for the character entries, once the fade is over,
 * sets the game's settings from mode select's choice (online, offline, or
 * offline with the save's equipment), ends this task and asks for the
 * lobby's network setup (online) or its scene (offline). Returns 1 for the
 * character entries (GmSub_Task's frame ends there), else 0.
 */
s32 GmSub_StartGame(TaskSlot *task, u16 held, u16 pressed)
{
    switch (gGmWork->menuSel) {
    case GMSUB_MENU_NEW_CHARA:
    case GMSUB_MENU_LOAD_CHARA:
        if ((u8)FadeA_GetState() != 1) {
            gSysSettings.galleryPlay = 0;
            gSysSettings.unk45 = 0;
            switch (gGmWork->sel) {
            case GMSUB_MODE_SEL_ONLINE:
                gSysSettings.online = 1;
                gSysSettings.useSavedEquip = 0;
                break;
            case GMSUB_MODE_SEL_OFFLINE:
                gSysSettings.online = 0;
                gSysSettings.useSavedEquip = 0;
                break;
            case GMSUB_MODE_SEL_SAVED_EQUIP:
                gSysSettings.useSavedEquip = 1;
                gSysSettings.online = 0;
                break;
            }
            gSysSettings.unk02 = 0;
            gSysSettings.unk03 = 1;
            gSysSettings.unk05 = 0;
            TaskSlot_End(task);
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

/*
 * GMSUB_TASK_EXIT, in task->count's steps: fades out, waits for the fade,
 * then resets the subsystems, ends this task and starts select's Boot_Task
 * in TASK_SLOT_BOOT (gSysSettings.bootFrom BOOT_FROM_GMSUB). Returns 1 once
 * the task has ended.
 */
s32 GmSub_ExitToBoot(TaskSlot *task, u16 held, u16 pressed)
{
    switch (task->count) {
    case 0:
        task->count++;
        FadeA_Start(FADE_OUT_BLACK_FAST);
        break;
    case 1:
        if ((u8)FadeA_GetState() != 1) {
            task->count++;
        }
        break;
    case 2:
        Game_ResetSubsystems();
        TaskSlot_End(task);
        gSysSettings.bootFrom = BOOT_FROM_GMSUB;
        TaskSlot_Set(Boot_Task, TASK_SLOT_BOOT);
        return 1;
    }
    return 0;
}

/*
 * Runs mode select for a frame: slides its entries, and once they rest
 * reads the pad. Returns 1 when PAD_CIRCLE confirms, -1 when PAD_CROSS
 * cancels, else 0. The background's animation restarts when the cursor
 * moves.
 */
s32 GmSub_UpdateModeSelect(u16 held, u16 pressed)
{
    s32 prevSel = gGmWork->sel;

    ModeSel_UpdateSlide();
    if (ModeSel_IsIdle() != 0) {
        if (pressed & PAD_CROSS) {
            return -1;
        }
        if (pressed & PAD_CIRCLE) {
            return 1;
        }
        GmSub_MoveCursorH(held, pressed, (u8 *)&gGmWork->sel, GMSUB_MODE_SEL_COUNT);
    }
    if (prevSel != gGmWork->sel) {
        Title_ResetBgAnim();
    } else {
        Title_StepBgAnim();
    }
    return 0;
}

/*
 * 1 when the character select runs for the title menu: the PSP link waits
 * for it (gSysSettings.pspCharaWait) or the menu's entry is
 * GMSUB_MENU_LOAD_CHARA. select's character screens test it.
 */
s32 GmSub_IsCharaSelectMenu(void)
{
    if (gSysSettings.pspCharaWait == 1) {
        return 1;
    }
    if (gGmWork->menuSel == GMSUB_MENU_LOAD_CHARA) {
        return 1;
    }
    return 0;
}
