/*
 * plsel: the player select overlay's two tasks.
 *  - plsel_entry, the overlay's entry before a quest (PlselEntryMode): it
 *    fills the players' slots in gQuestSync and waits until every player
 *    in the quest is ready (QSYNC_STATE_SETUP), or sends the local player
 *    to the results.
 *  - Plsel_SelectTask, the training quest's select screen (PlselSelMode):
 *    each player takes one of a set's four preset hunters (PlselPreset,
 *    gPlselPresets' sets), shown as preview players, with the online chat
 *    log (PlselSelWork's ring, Plsel_AddChatLog).
 * The shared state is gQuestSync (qsync.h), kept in step with the other
 * players by game message 10 (QSync_SendState). sub_main calls
 * Plsel_ApplyPresetEquip and Plsel_ApplyStageEquip as it sets the players
 * up; gm_sub's QSync_OnChat calls Plsel_LogChatFrom.
 */
#include "common.h"
#include "sprite.h"
#include "syssettings.h"
#include "screen_fade.h"
extern u16 gFrameTick; /* sub_main: the frame counter */
extern s8 gChatTarget;
extern s32 gTexHandles[];
#include "camera.h"
#include "pad.h"
#include "m2c_macros.h"
#include "plsel.h"
#include "task.h"
#include "player.h"
#include "scene.h"
#include "save.h"
#include "qsync.h"
#include "sound.h"
#include "render_state.h"
#include "chat_log.h"
#include "lobby_net.h"         /* NetUserRec: a chat sender */
#include "window.h"
#include "hud.h"               /* IconUV: the weapon icons' texture corners */
#include "prim.h"              /* PrimLine: Prim_FillRect's rectangle */
#include "plsel/plsel.h"   /* the texts this file uses (tools/content_gen.py) */
#include "area_draw.h"
#include "bgm.h"
#include "chat_box.h"
#include "place_rules.h"
#include "fishing.h"
#include "kbd.h"
#include "light.h"
#include "model.h"
#include "qnet.h"
#include "stream.h"
#include "text.h"
#include "texture.h"
#include "area.h"
#include "chat_phrases.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "file_load.h"
#include "font.h"
#include "frame.h"
#include "game_util.h"
#include "gs_setup.h"
#include "sys_main.h"
#include "tex_files.h"
#include "libc.h"

/* The game compiled this file at optimization level 4 (the build's -O3,p
 * plus MWCC's level-4 passes): Plsel_InitSlots matches only there, and every
 * other function compiles the same at both levels. */
#pragma optimization_level 4

struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel); /* sub_main: b is a ListPanel * */

/* Plsel_ChatRunKbd's call passes held as a full word and takes the result
 * as an int; with the definition's s8 (char *dst, s16 held, s16 pressed),
 * the call would narrow both first, which the game's code doesn't do.
 * unit's own declaration: declared as the caller uses it. */
s32 Kbd_Run(char *dst, s32, s16);
extern PlselPreset *gPlselPresets[];
extern u16 gPlselPresetStages[];

extern s32 gItemColors[];
extern char gMemberNames[][17];

void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);

extern s16 *gStartItemLists[];

extern PlselPreset *gTrainingPresets[];

void Plsel_AddChatLog(u8 who, char *msg, NetUserRec *from, u8 flag);
extern u8 gChatClassColors[];
extern char gPlayerIds[][8];

extern DrawSlot gSelectDrawHook;

void Player_LoadPartModels(struct Player *);
extern f32 gStartPos[][3];

struct PlModels;
void Player_FreeModels(struct Player *p);
void Player_FreeWeaponModel(Player *p);

u8 Equip_GetClass(EquipItem *e);

extern char gMyUserId[];
extern char gMyName[];

extern char gPlselFmtString[];

/* The overlay's data: the preset hunters' looks, the weapon icons' tints
 * and texture corners, the slot colours, the panels. */
PlselPresetPlayer gPlselPresetPlayers[2] = {
#include "plsel/00D75F80.inc"
};
u32 gPlselWeaponIconColors[4] = {
#include "plsel/00D75FA0.inc"
};
IconUV gPlselWeaponIconUVs[11] = {
#include "plsel/00D75FB0.inc"
};
u8 gPlselSlotColors[8] = {
#include "plsel/00D76008.inc"
};
ListPanel gPlselListPanel = {
#include "plsel/00D76010.inc"
};
u8 gPlselEquipIconIds[16] = {
#include "plsel/00D76030.inc"
};
Panel gPlselChatWin = {
#include "plsel/00D76040.inc"
};

/*
 * The overlay's entry task: runs its steps (PlselEntryMode: the setup, the
 * wait for the other players, the finish, the debug menu). Online, a local
 * player who is no longer in the quest goes to the results and the task
 * ends; otherwise every 60 frames (gQuestSync.bumpLeft counts them) it sends
 * QSYNC_STATE_IDLE.
 */
void plsel_entry(TaskSlot *task)
{
    if (gTexHandles[1] != 0) {
        Tex_Use(1, (u32 *) &gTexHandles[1]);
    }
    switch (task->mode) {
    case PLSEL_ENTRY_SETUP:
        Plsel_Setup(task);
        break;
    case PLSEL_ENTRY_WAIT:
        Plsel_WaitPlayersReady(task);
        break;
    case PLSEL_ENTRY_FINISH:
        Plsel_Finish(task);
        break;
    case PLSEL_ENTRY_DEBUG:
        Plsel_DebugMenu(task);
        break;
    }
    if (Game_IsOnline() == 1 && gScene.inQuest[gScene.localPlayer] == IN_QUEST_NO_PLAYER) {
        Loading_Stop();
        gScene.scene = SCENE_RESULTS;
        Quest_Finish();
        FadeA_Reset();
        TaskSlot_End(task);
    } else {
        if (++gQuestSync.bumpLeft >= 60) {
            gQuestSync.bumpLeft = 0;
            QSync_SendState(QSYNC_STATE_IDLE, gScene.localPlayer);
        }
        Frame_Draw();
    }
}

/*
 * PLSEL_ENTRY_SETUP: resets the subsystems, the players, the area and the
 * fade, loads the texture file and the overlay's pack (PLSEL_TEX_BG on), and
 * moves on.
 */
void Plsel_Setup(TaskSlot *task)
{
    Game_ResetSubsystems();
    Player_ClearAll();
    Area_ClearState();
    FadeA_Reset();
    Tex_LoadFile(0, 1, 0);
    Tex_LoadSystemPack(0);
    Tex_LoadPack(gPlselTexPack, PLSEL_TEX_BG, 0);
    task->unk14 = 0;
    task->mode++;
}

/*
 * PLSEL_ENTRY_WAIT. Step 0 clears gQuestSync, fills the slots
 * (Plsel_InitSlots), marks the local player ready, starts a 3600-frame
 * timeout and sends QSYNC_STATE_SETUP. Step 1 sends the local player to the
 * results if it left the quest or the time ran out; once every slot is
 * ready or empty it moves to PLSEL_ENTRY_FINISH.
 */
void Plsel_WaitPlayersReady(TaskSlot *task)
{
    QuestSync *sync;
    u16 self;
    s16 i;

    sync = &gQuestSync;
    self = gScene.localPlayer;
    switch (task->step) {
    case 0:
        task->step++;
        QuestSync_Clear();
        Plsel_InitSlots();
        sync->ready[self] = 1;
        sync->timer = 3600;
        QSync_SendState(QSYNC_STATE_SETUP, gScene.localPlayer);
        break;
    case 1:
        if (Game_IsOnline() == 1 && gScene.inQuest[gScene.localPlayer] == IN_QUEST_NO_PLAYER) {
            Loading_Stop();
            gScene.scene = SCENE_RESULTS;
            Quest_Finish();
            TaskSlot_End(task);
        } else if (--sync->timer <= 0) {
            Loading_Stop();
            gScene.scene = SCENE_RESULTS;
            Quest_Finish();
            TaskSlot_End(task);
        } else {
            for (i = 0; i < 4; i++) {
                if (sync->ready[i] == 0 && gScene.inQuest[i] != IN_QUEST_NO_PLAYER) {
                    return;
                }
            }
            task->mode++;
            sync->step = 0;
        }
        break;
    }
}

/*
 * PLSEL_ENTRY_FINISH: sends QSYNC_STATE_SETUP, waits 30 frames, then resets
 * the subsystems, ends the task and clears gScene.allLocal.
 */
void Plsel_Finish(TaskSlot *task)
{
    QuestSync *sync;

    sync = &gQuestSync;
    switch (sync->step) {
    case 0:
        sync->step++;
        sync->timer = 30;
        QSync_SendState(QSYNC_STATE_SETUP, gScene.localPlayer);
    case 1:
        if (--sync->timer <= 0) {
            gSysSettings.unk03 = 1;
            Game_ResetSubsystems();
            TaskSlot_End(task);
            gScene.allLocal = 0;
        }
        break;
    }
}

/*
 * Empty: called last by Plsel_DebugMenu with its state and task (a debug
 * display compiled out).
 */
static void Plsel_DrawDebugMenu(QuestSync *sync, TaskSlot *task)
{

}

/*
 * PLSEL_ENTRY_DEBUG, a debug menu of the quest's monsters: up and down pick
 * a monster group (0-3), right and left change its kind (0-73, wrapping),
 * cross and circle its count (0-15); start ends the task when the four
 * counts total under 16. The pad and the scene are read through raw
 * addresses: their named forms change the code.
 */
void Plsel_DebugMenu(TaskSlot *task)
{
    QuestSync *sync;
    u16 pad;
    u16 sum;
    s32 i;

    sync = &gQuestSync;
    pad = gPadState[0].pressed | gPadState[0].repeat | gPadState[1].pressed | gPadState[1].repeat;
    switch (gQuestSync.step) {
    case 0:
        sync->step++;
        sync->cursor = 0;
        break;
    case 1:
        if (pad & PAD_UP) {
            if (sync->cursor <= 0) {
                sync->cursor = 3;
            } else {
                sync->cursor--;
            }
        }
        if (pad & PAD_DOWN) {
            if (sync->cursor >= 3) {
                sync->cursor = 0;
            } else {
                sync->cursor++;
            }
        }
        if (pad & PAD_RIGHT) {
            if (sync->emKind[sync->cursor] >= 73) {
                sync->emKind[sync->cursor] = 0;
            } else {
                sync->emKind[sync->cursor]++;
            }
        }
        if (pad & PAD_LEFT) {
            if (sync->emKind[sync->cursor] <= 0) {
                sync->emKind[sync->cursor] = 73;
            } else {
                sync->emKind[sync->cursor]--;
            }
        }
        if (pad & PAD_CROSS) {
            if (sync->emCount[sync->cursor] != 0) {
                sync->emCount[sync->cursor]--;
            }
        }
        if (pad & PAD_CIRCLE) {
            if (sync->emCount[sync->cursor] < 15) {
                sync->emCount[sync->cursor]++;
            }
        }
        if (pad & PAD_START) {
            sum = 0;
            for (i = 0; i < 4; i++) {
                sum += sync->emCount[i];
            }
            if (sum < 16) {
                Loading_SetMessage(4);
                gSysSettings.unk03 = 1;
                Game_ResetSubsystems();
                TaskSlot_End(task);
                gScene.allLocal = 0;
            }
        }
        break;
    }
    Plsel_DrawDebugMenu(sync, task);
}

/*
 * Resets the select state's ten slots: online, the connected players' slots
 * are ready 0 and the others -1 (not in the quest); offline, slot 0 is 0 and
 * the others 1. The local player's slot (gScene.localPlayer) takes
 * gSave.equip[0] as its weapon when gSave.used is set, else an empty blade
 * weapon. The file is compiled at level 4 for this function.
 */
void Plsel_InitSlots(void)
{
    QuestSync *sync;
    s32 i;

    sync = &gQuestSync;
    for (i = 0; i < PLAYER_SLOTS; i++) {
        if (Game_IsOnline() == 1) {
            if (i < gScene.playerCount) {
                sync->ready[i] = 0;
            } else {
                sync->ready[i] = -1;
            }
        } else if (i == 0) {
            sync->ready[i] = 0;
        } else {
            sync->ready[i] = 1;
        }
        sync->gender[i] = (i & 1) != 0;
        sync->startItemList[i] = 0;
        if (i == gScene.localPlayer) {
            sync->presetEquip[i] = 0;
            if (gSave.used != 0) {
                sync->weapon[i] = gSave.equip[SAVE_EQUIP_WEAPON];
            } else {
                sync->weapon[i].type = EQUIP_BLADE;
                sync->weapon[i].id = 0;
                sync->weapon[i].level = 0;
            }
        } else {
            sync->weapon[i].type = EQUIP_BLADE;
            sync->weapon[i].id = 0;
            sync->weapon[i].level = 0;
        }
        sync->unk16[i] = 4;
    }
    sync->emKind[0] = Quest_DefaultEmKind();
    sync->emCount[0] = 1;
    sync->emKind[1] = 0;
    sync->emCount[1] = 0;
    sync->emKind[2] = 0;
    sync->emCount[2] = 0;
    sync->emKind[3] = 0;
    sync->emCount[3] = 0;
}

/*
 * The training quest's select task (sub_main starts it when Quest_IsDrill):
 * runs its steps (PlselSelMode), updates the players, the lights and the
 * camera and draws the frame; draws the keyboard's box with depth writes off
 * while it is open, and the chat window and log while the keyboard is open
 * or the log's timer runs. Online, it closes the overlay once the local
 * player has left the quest.
 */
void Plsel_SelectTask(TaskSlot *task)
{
    PlselSelWork *w;
    s32 zwrite;
    u8 slotState;

    w = gPlselSelWork;
    if (gTexHandles[1] != 0) {
        Tex_Use(1, (u32 *) &gTexHandles[1]);
    }
    switch (task->mode) {
    case PLSEL_SEL_INIT:
        Plsel_SelectInit(task);
        break;
    case PLSEL_SEL_SOUNDS:
        Plsel_SelectWaitSound(task);
        break;
    case PLSEL_SEL_START_CHOOSE:
        Plsel_SelectStartChoose(task);
        break;
    case PLSEL_SEL_SETUP_PLAYERS:
        Plsel_SelectSetupPlayers(task);
        break;
    case PLSEL_SEL_CHOOSE:
        Plsel_StateChoose(task);
        break;
    case PLSEL_SEL_READY:
        Plsel_StateReady(task);
        break;
    }
    Players_RunEquipEffects();
    Players_UpdateMatrices();
    LightSet_UpdateAll();
    Camera_Update();
    Frame_Draw();
    if ((s8)Kbd_IsOpen() != 0) {
        zwrite = Gs_GetState(RS_ZWRITE);
        Gs_SetState(RS_ZWRITE, 1);
        Gs_ClearColor(0, 0);
        Gs_SetState(RS_ZWRITE, 0);
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
        Chat_DrawKbdBox(w->kbdOpen != 0 ? 0xBDB59F6C : 0xBD6C6CC5);
        Gs_SetState(RS_ZWRITE, zwrite);
        Text_FlushPages();
    }
    if ((s8)Kbd_IsOpen() != 0 || w->logShowTimer != 0) {
        Plsel_DrawChatWindow(gChatTitleText, 18, 1);
        Plsel_DrawChatLog(0, 0, 0, 18);
        Text_DrawAllPages();
    }
    Plsel_TickTimers();
    if (Game_IsOnline() != 0) {
        slotState = gScene.inQuest[gScene.localPlayer];
        if (slotState != 1) {
            gSysSettings.unk03 = 1;
            gScene.plselOpen = 0;
            Game_ResetSubsystems();
            TaskSlot_End(task);
            gScene.allLocal = 0;
            gSysSettings.plselLoaded = 0;
        }
    }
}

/*
 * Draws row `row` (32 pixels high) of the header strips (PLSEL_TEX_HEADER),
 * 192 by 48.
 */
void Plsel_DrawHeader(u16 row)
{
    Sprite spr;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, PLSEL_TEX_HEADER);
    Tex_Bind(PLSEL_TEX_HEADER);
    spr.x = -10;
    spr.u0 = 0;
    spr.y = 10;
    spr.w = 192;
    spr.h = 48;
    spr.color = -1;
    spr.v0 = row << 5;
    spr.u1 = 127;
    spr.v1 = spr.v0 + 31;
    Sprite_Draw(&spr);
}

/*
 * Draws the full-screen background (PLSEL_TEX_BG) and the bottom panel
 * (PLSEL_TEX_PANEL) with depth writes off, then turns them back on.
 */
void Plsel_DrawFrame(s32 unused)
{
    Sprite spr;

    Gs_SetState(RS_ZWRITE, 0);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, PLSEL_TEX_BG);
    Tex_Bind(PLSEL_TEX_BG);
    spr.x = 0;
    spr.y = 0;
    spr.w = 640;
    spr.h = 448;
    spr.color = -1;
    spr.v0 = 0;
    spr.u0 = 0;
    spr.u1 = 511;
    spr.v1 = 447;
    Sprite_Draw(&spr);
    Tex_Prepare(1, PLSEL_TEX_PANEL);
    Tex_Bind(PLSEL_TEX_PANEL);
    spr.x = 32;
    spr.y = 372;
    spr.w = 256;
    spr.h = 70;
    spr.color = 0xE0202020;
    spr.v0 = 0;
    spr.u0 = 0;
    spr.u1 = 255;
    spr.v1 = 69;
    Sprite_Draw(&spr);
    Gs_SetState(RS_ZWRITE, 1);
}

/*
 * Aims the camera at the preview hunter (gPlayers[PLSEL_PREVIEW_PLAYER]):
 * the target raised 125 and shifted 40, the eye 160 in front.
 */
void Plsel_SetCamera(void)
{
    Player *pl;

    pl = &gPlayers[PLSEL_PREVIEW_PLAYER];
    Vec_Copy(gCamera->at, pl->pos);
    Vec_Copy(gCamera->eye, pl->pos);
    gCamera->at[0] += 40.0f;
    gCamera->eye[0] = gCamera->at[0];
    gCamera->at[1] += 125.0f;
    gCamera->eye[1] += 125.0f;
    gCamera->eye[2] += 160.0f;
}

extern u16 gNextQuestId;
/*
 * PLSEL_SEL_INIT: resets, loads the system packs (PLSEL_TEX_ICONS and
 * PLSEL_TEX_BG), fills the slots and picks the preset set for the stage,
 * builds the preview hunter from the save's gender, gives gPlayers[0-5]
 * their draw slots, starts the online countdown (3600 frames), marks the
 * select screen open and clears the chat work.
 */
void Plsel_SelectInit(TaskSlot *task)
{
    s16 i;
    Player *pl;

    QNetIf_Ready(0);
    Game_ResetSubsystems();
    Player_ClearAll();
    Area_ClearState();
    Gs_SetTableColor(0);
    Tex_LoadPack(gSystemTexFiles[1], PLSEL_TEX_ICONS, 0);
    Tex_LoadPack(gSystemTexFiles[3], PLSEL_TEX_BG, 0);
    task->mode++;
    task->unk14 = 0;
    task->unk15 = gSysSettings.unk45;
    Plsel_InitSlots();
    gNextQuestId = 65535;
    gQuestSync.presetSet = Plsel_PresetSetForStage();
    gQuestSync.builtSet = PLSEL_SET_NONE;
    gQuestSync.pick = 1;
    gQuestSync.unkD9 = 0;
    Anim_LoadCommon(5);
    Camera_Reset();
    CamCtl_Clear();
    gScene.area = 0;
    Light_InitAll(gScene.area);
    gCamera->fov = 0.87266463f;
    gCamera->roll = 0.0f;
    Plsel_SetupPresetPlayer(&gPlayers[PLSEL_PREVIEW_PLAYER], gSave.gender & 1);
    Plsel_SetCamera();
    FadeA_Start(FADE_IN_BLACK);
    for (i = 0, pl = gPlayers; i < PLSEL_PREVIEW_PLAYER + 1; i++, pl++) {
        pl->drawSlotId = DrawSlot_Alloc();
        pl->index = i;
        if (pl->drawSlotId != -1) {
            pl->drawSlot = DrawSlot_Get(pl->drawSlotId);
            pl->drawSlot->player = pl->index;
            pl->drawSlot->draw = DrawCb_LocalPlayer;
        }
    }
    task->timeLeft = 3600;
    gScene.plselOpen = 1;
    Plsel_InitSelWork();
    Kbd_Close();
    gQuestSync.bumpLeft = 0;
    gQuestSync.bumpRight = 0;
}

/*
 * PLSEL_SEL_SOUNDS: once the select screen's sound banks are loaded, moves on
 * after 15 frames and plays the music bank's sounds 0 and 1.
 */
void Plsel_SelectWaitSound(TaskSlot *task)
{
    if (Sound_LoadSelectBanks(task) != 0) {
        Sound_OnBanksLoaded();
        task->mode++;
        task->step = 0;
        gQuestSync.timer = 15;
        Sound_PlaySeMusicVol(1, 0, 0);
        Sound_PlaySeMusicVol(1, 1, 0);
    }
}

/*
 * Readies a preview player: active, visible, slot index slotIndex, scale and
 * alpha 1, two motion layers, at the area's start position, heading 0xC00;
 * loads its parts and models.
 */
void Plsel_InitPlayer(Player *pl, s16 slotIndex)
{
    pl->active = 1;
    pl->visible = 1;
    pl->index = slotIndex;
    pl->isEm = 0;
    pl->alpha = 1.0f;
    pl->motionLayerCount = 2;
    pl->scale[0] = 1.0f;
    pl->scale[1] = 1.0f;
    pl->scale[2] = 1.0f;
    pl->motion[0].step = 2.0f;
    pl->motion[1].step = 2.0f;
    pl->motionSpeed = 1.0f;
    pl->weaponOut = 0;
    pl->pos[0] = gStartPos[gScene.area][0];
    pl->pos[1] = gStartPos[gScene.area][1];
    pl->pos[2] = gStartPos[gScene.area][2];
    pl->rot[1] = 0;
    pl->rot[1] = 0xC00;
    pl->rot[2] = 0;
    Player_InitPartTable(pl->index);
    Player_LoadPartModels(pl);
    Player_ClearFxBlock(pl);
    Player_InitRecords(pl);
}

/*
 * Builds the preview hunter from gPlselPresetPlayers[gender]: its look and
 * armour, no weapon, at PLSEL_PREVIEW_PLAYER, in motion 697.
 */
void Plsel_SetupPresetPlayer(Player *pl, s16 n)
{
    PlselPresetPlayer *look;

    look = &gPlselPresetPlayers[n];
    pl->weaponShown = 0;
    pl->gender = look->gender;
    pl->hairStyle = look->hairStyle;
    pl->hairColor = look->hairColor;
    pl->charType = 0;
    Player_SetColorWord(pl, 2);
    pl->lookVariant = 0;
    pl->equip[EQUIP_HEAD].id = look->head;
    pl->equip[EQUIP_BODY].id = look->body;
    pl->equip[EQUIP_ARMS].id = look->arms;
    pl->equip[EQUIP_WAIST].id = look->waist;
    pl->equip[EQUIP_LEGS].id = look->legs;
    pl->equip[EQUIP_SLOT_FACE].id = look->face;
    pl->weaponType = 0;
    pl->weaponKind = 0;
    Plsel_InitPlayer(pl, PLSEL_PREVIEW_PLAYER);
    Player_SetMotionPair(pl, 697, 0, 0);
    Player_UpdateMotionStep(pl);
}

/*
 * Builds preview player slotIndex from the save's look with the current set's
 * preset hunter's equipment (armour and weapon). The empty check after the
 * call keeps &gSave in a register for the save's reads, as in the game (a
 * compiled-out check of the gender; any two-test condition gives the same
 * code).
 */
void Plsel_SetupSavedPlayer(Player *pl, s16 id)
{
    PlselPreset *preset;
    SaveData *save;

    save = &gSave;
    preset = gPlselPresets[gQuestSync.presetSet];
    preset += id;
    pl->weaponShown = 1;
    pl->gender = save->gender;
    pl->hairStyle = save->hairStyle;
    pl->hairColor = save->hairColor;
    pl->charType = save->charType;
    Player_SetColorWord(pl, save->symbolColor);
    if (save == NULL || save->gender > 1) { /* compiled-out check */
    }
    pl->equip[EQUIP_LEGS].used = 1;
    pl->equip[EQUIP_LEGS].type = EQUIP_LEGS;
    pl->equip[EQUIP_LEGS].level = 0;
    pl->equip[EQUIP_LEGS].deco[0] = 0;
    pl->equip[EQUIP_LEGS].deco[1] = 0;
    pl->equip[EQUIP_LEGS].deco[2] = 0;
    pl->equip[EQUIP_SLOT_FACE].used = 1;
    pl->equip[EQUIP_SLOT_FACE].type = EQUIP_SLOT_FACE;
    pl->equip[EQUIP_SLOT_FACE].level = 0;
    pl->equip[EQUIP_SLOT_FACE].deco[0] = 0;
    pl->equip[EQUIP_SLOT_FACE].deco[1] = 0;
    pl->equip[EQUIP_SLOT_FACE].deco[2] = 0;
    pl->equip[EQUIP_HEAD].used = 1;
    pl->equip[EQUIP_HEAD].type = EQUIP_HEAD;
    pl->equip[EQUIP_HEAD].level = 0;
    pl->equip[EQUIP_HEAD].deco[0] = 0;
    pl->equip[EQUIP_HEAD].deco[1] = 0;
    pl->equip[EQUIP_HEAD].deco[2] = 0;
    pl->equip[EQUIP_BODY].used = 1;
    pl->equip[EQUIP_BODY].type = EQUIP_BODY;
    pl->equip[EQUIP_BODY].level = 0;
    pl->equip[EQUIP_BODY].deco[0] = 0;
    pl->equip[EQUIP_BODY].deco[1] = 0;
    pl->equip[EQUIP_BODY].deco[2] = 0;
    pl->equip[EQUIP_ARMS].used = 1;
    pl->equip[EQUIP_ARMS].type = EQUIP_ARMS;
    pl->equip[EQUIP_ARMS].level = 0;
    pl->equip[EQUIP_ARMS].deco[0] = 0;
    pl->equip[EQUIP_ARMS].deco[1] = 0;
    pl->equip[EQUIP_ARMS].deco[2] = 0;
    pl->equip[EQUIP_WAIST].used = 1;
    pl->equip[EQUIP_WAIST].type = EQUIP_WAIST;
    pl->equip[EQUIP_WAIST].level = 0;
    pl->equip[EQUIP_WAIST].deco[0] = 0;
    pl->equip[EQUIP_WAIST].deco[1] = 0;
    pl->equip[EQUIP_WAIST].deco[2] = 0;
    pl->equip[EQUIP_HEAD].id = preset->head;
    pl->equip[EQUIP_BODY].id = preset->body;
    pl->equip[EQUIP_ARMS].id = preset->arms;
    pl->equip[EQUIP_WAIST].id = preset->waist;
    pl->equip[EQUIP_LEGS].id = preset->legs;
    pl->equip[EQUIP_SLOT_FACE].id = save->face + 1;
    pl->equip[EQUIP_SLOT_WEAPON].used = 1;
    pl->equip[EQUIP_SLOT_WEAPON].type = preset->weaponType;
    pl->equip[EQUIP_SLOT_WEAPON].level = preset->weaponLevel;
    pl->equip[EQUIP_SLOT_WEAPON].deco[0] = 0;
    pl->equip[EQUIP_SLOT_WEAPON].deco[1] = 0;
    pl->equip[EQUIP_SLOT_WEAPON].deco[2] = 0;
    Plsel_SetupPresetPouch(pl, preset->startItems);
    pl->equip[EQUIP_SLOT_WEAPON].id = preset->weaponId;
    pl->weaponKind = Equip_GetKind(&pl->equip[EQUIP_SLOT_WEAPON]);
    pl->weaponType = Equip_GetClass(&pl->equip[EQUIP_SLOT_WEAPON]);
    pl->isGunner = WeaponClass_IsGunner(pl->weaponType);
    pl->weaponFlags = WeaponType_Flags(pl->weaponType);
    pl->bladeSpecial = Equip_GetBladeSpecial(&pl->equip[EQUIP_SLOT_WEAPON]);
    Plsel_InitPlayer(pl, id);
    Player_LoadWeaponModel(pl->weaponKind, pl->index);
    Player_SetMotionPair(pl, 1, 0, 0);
    Player_UpdateMotionStep(pl);
}

/*
 * Returns the current set's preset hunter under the cursor
 * (gQuestSync.pick, at most PLSEL_SET_PRESETS - 1).
 */
PlselPreset *Plsel_CurPreset(void)
{
    PlselPreset *presets;
    s32 n;

    presets = gPlselPresets[gQuestSync.presetSet];
    n = gQuestSync.pick;
    if (n >= PLSEL_SET_PRESETS) {
        n = PLSEL_SET_PRESETS - 1;
    }
    return presets + n;
}

/*
 * Returns the preset set whose two stages (gPlselPresetStages' row) hold
 * gQuest.stage, else set 0.
 */
u8 Plsel_PresetSetForStage(void)
{
    u16 *stagePairs;
    s32 i;
    s32 n;
    u16 stageId;

    stageId = gQuest.stage;
    stagePairs = gPlselPresetStages;
    n = 0;
    for (i = 0; i < PLSEL_PRESET_SETS; i++, stagePairs += 2, n++) {
        if (stagePairs[0] == stageId) {
            return n;
        }
        if (stagePairs[1] == stageId) {
            return n;
        }
    }
    return 0;
}

static s32 Plsel_SlotCount(u8 set)
{
    return 4;
}

s32 Plsel_ViewedSlotNo(u8 set, u8 sel)
{
    return (gQuestSync.pick + 1) & 0xFF;
}

/* PLSEL_SEL_START_CHOOSE: moves on with the cursor on the first preset. */
void Plsel_SelectStartChoose(TaskSlot *task)
{
    task->mode++;
    task->step = 0;
    gQuestSync.unkD3 = 0;
    gQuestSync.pick = 0;
}

/*
 * PLSEL_SEL_SETUP_PLAYERS: rebuilds preview players 0-3 when the set
 * changed, turns them to face the camera, places the camera, fades in and
 * starts the select music.
 */
void Plsel_SelectSetupPlayers(TaskSlot *task)
{
    QuestSync *sync;
    s16 i;
    Player *pl;

    task->mode++;
    task->step = 0;
    sync = &gQuestSync;
    if (gQuestSync.presetSet != gQuestSync.builtSet) {
        sync->builtSet = sync->presetSet;
        for (i = 0, pl = gPlayers; i < PLSEL_SET_PRESETS; i++, pl++) {
            Player_FreeModels(pl);
            Player_FreeWeaponModel(pl);
            Plsel_SetupSavedPlayer(pl, i);
        }
    }
    sync->yaw = ANGLE_TURN / 16;
    gCamera->at[0] += 80.0f;
    gCamera->eye[0] = gCamera->at[0];
    gCamera->eye[2] += 190.0f;
    gCamera->at[1] -= 35.0f;
    gCamera->eye[1] -= 35.0f;
    FadeA_Start(FADE_IN_BLACK);
    QNetIf_Unready();
    Loading_Stop();
    Stream_Play(0, 293);
}

/*
 * Draws the equipment page's panel and sets the text colour, position and
 * spacing for it.
 */
void Plsel_DrawEquipPanel(void)
{
    ListPanel_Draw(&gPlselListPanel, 0, 0, -1);
    Text_SetColor(0);
    Font_SetPos(326, 334);
    Font_SetSpacing(20, 20);
}

/*
 * Draws weapon-class icon weaponClass (PLSEL_TEX_WEAPON_ICONS) at x, y in a
 * size by size square, tinted with player slot's colour (slot & 3).
 */
void Plsel_DrawWeaponIcon(s16 x, s16 y, s16 size, s32 slot, s32 weaponClass)
{
    Sprite spr;

    Tex_Prepare(1, PLSEL_TEX_WEAPON_ICONS);
    Tex_Bind(PLSEL_TEX_WEAPON_ICONS);
    spr.x = x + 4;
    spr.y = y + 2;
    spr.w = size;
    spr.h = size;
    spr.color = gPlselWeaponIconColors[slot & 3];
    /* Each corner is copied as one word, as the game's code does: Sprite
     * keeps a corner as two halfwords, which the cast reaches as one UV. */
    *(UV *)&spr.u0 = gPlselWeaponIconUVs[weaponClass].uv0;
    *(UV *)&spr.u1 = gPlselWeaponIconUVs[weaponClass].uv1;
    Sprite_Draw(&spr);
}

/*
 * Draws an item's icon (its cell + 1 of an 8-column grid of 32-pixel cells)
 * in the item's colour.
 */
void Plsel_DrawItemIcon(u16 item, s16 x, s16 y)
{
    Sprite spr;
    ItemData *data;
    s32 n;
    s32 cellU;
    s32 cellV;

    data = &gItemData[item];
    n = data->icon + 1;
    spr.x = x;
    spr.y = y;
    spr.w = 27;
    spr.h = 20;
    cellU = (n & 7) * 32;
    cellV = (n >> 3) * 32;
    spr.u0 = cellU + 1;
    spr.u1 = cellU + 31;
    spr.v0 = cellV + 1;
    spr.v1 = cellV + 31;
    spr.color = gItemColors[data->color];
    Sprite_Draw(&spr);
}

/*
 * Draws an equipment icon: flag 0 weapon class n's (icon n + 63), flag 1
 * armour piece n's (gPlselEquipIconIds' byte n + 74).
 */
void Plsel_DrawEquipIcon(s16 x, s16 y, u8 flag, u8 n, s32 unused)
{
    Sprite spr;
    s32 id;
    s32 cellU;
    s32 cellV;

    if (flag == 0) {
        id = n + 63;
    } else {
        id = gPlselEquipIconIds[n] + 74;
    }
    spr.x = x;
    spr.y = y - 2;
    spr.w = 24;
    spr.h = 24;
    cellU = (id & 7) * 32;
    cellV = (id >> 3) * 32;
    spr.u0 = cellU + 1;
    spr.u1 = cellU + 31;
    spr.v0 = cellV + 1;
    spr.v1 = cellV + 31;
    spr.color = -1;
    Sprite_Draw(&spr);
}

/* Draws the left (flag 0) or right arrow beside the preset being chosen,
 * pushed out by its bump counter (gQuestSync.bumpLeft and bumpRight, set to
 * 5 when the arrow is pressed). The counters are read by name for the
 * match. */
void Plsel_DrawArrow(s32 flag, s16 x, s16 y)
{
    Sprite spr;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, PLSEL_TEX_WEAPON_ICONS);
    Tex_Bind(PLSEL_TEX_WEAPON_ICONS);
    spr.w = 24;
    spr.x = x;
    spr.y = y;
    spr.h = 30;
    spr.color = 0xFF32BC64;
    if (flag == 0) {
        spr.x -= gQuestSync.bumpLeft;
        spr.u0 = 255;
        spr.u1 = 240;
    } else {
        spr.x += gQuestSync.bumpRight;
        spr.u0 = 240;
        spr.u1 = 255;
    }
    spr.v0 = 236;
    spr.v1 = 255;
    Sprite_Draw(&spr);
}

INCLUDE_RODATA("asm/plsel/nonmatchings/plsel", gPlselFmtConcat);

/*
 * Draws the local player's panel (the weapon-class icon of the preset under
 * its cursor and its player number), then a smaller panel for each other
 * player in the quest, with the ready label once its pick is taken.
 */
void Plsel_DrawPlayerList(void)
{
    ListPanel win;
    char buf[16];
    char txt[16];
    s16 x;
    s16 i;

    x = 30;
    win = gPlselListPanel;
    win.flags = 0;
    Font_SetSpacing(18, 18);
    win.x = x;
    win.y = 80;
    win.charW = 18;
    win.charH = 15;
    win.lineChars = 3;
    win.lines = 3;
    ListPanel_Draw(&win, 0, 0, -1);
    Plsel_DrawWeaponIcon(x, 80, 48, gScene.localPlayer, (s16)gPlayers[gQuestSync.pick].weaponType);
    sprintf(buf, gPlayerNumFormat, gScene.localPlayer + 1);
    Sjis_Widen(buf, txt);
    Font_SetPos(38, 56);
    Text_Printf("~C00%s", txt);
    win.lineChars = 2;
    x += 80;
    win.lines = 2;
    for (i = 0; i < gScene.playerCount; i++) {
        if (i != gScene.localPlayer && gScene.inQuest[i] == IN_QUEST_YES) {
            win.x = x;
            ListPanel_Draw(&win, 0, 0, -1);
            Plsel_DrawWeaponIcon(x, 80, 32, i, (s16)gPlayers[gQuestSync.picks[i] & QSYNC_PICK_INDEX].weaponType);
            Font_SetSpacing(16, 16);
            sprintf(buf, gPlayerNumFormat, i + 1);
            Sjis_Widen(buf, txt);
            Font_SetPos(x + 4, 58);
            Text_Printf("~C00%s", txt);
            if (gQuestSync.picks[i] & QSYNC_PICK_TAKEN) {
                Font_SetPos(x - 8, 124);
                Text_Printf(gPlselFmtString, gPlselReadyText);
            }
            Font_SetSpacing(18, 18);
            x += 54;
        }
    }
}

INCLUDE_RODATA("asm/plsel/nonmatchings/plsel", gPlselFmtString);

extern char gPlselFmtConcat[];

/* The equipment page, a draw callback (hook is the draw slot, its owner the
 * select task): the preset's name and number, the weapon and the armour
 * pieces, the items, then the prompt of the task's step. */
void Plsel_DrawEquipPage(DrawSlot *hook)
{
    ListPanel win;
    PrimLine rect;
    char buf[128];
    char txt[128];
    s16 x;
    PlselPreset *preset;
    TaskSlot *task;
    s16 sel;
    QuestSync *sync;
    s16 *list;
    s16 i;
    s32 y;

    sync = &gQuestSync;
    task = hook->owner;
    if (task->step == PLSEL_CHOOSE_MENU) {
        Plsel_DrawPlayerMenu();
        Plsel_DrawPlayerList();
        return;
    }
    Math_Sin((f32)(u16)((gFrameTick & 0x3F) << 10) * 0.0000958738f);
    win = gPlselListPanel;
    win.x = 328;
    win.y = 30;
    win.lineChars = 14;
    win.lines = 7;
    win.flags = 0;
    ListPanel_Draw(&win, 0, " ", -1);
    sel = sync->pick;
    preset = Plsel_CurPreset();
    Font_SetSpacing(16, 16);
    Text_PrintfAt(346, 34, 5, gPlselFmtConcat, gPlselEquipTypeLabel, gWeaponTypeNames[(u8)Weapon_GetClass(preset->weaponType, preset->weaponId)]);
    Text_PrintfAt(566, 34, 5, "%d/%d", (u8)Plsel_ViewedSlotNo(sync->presetSet, sel), (u8)Plsel_SlotCount(sync->presetSet));
    Plsel_DrawArrow(0, 314, 26);
    Plsel_DrawArrow(1, 598, 26);
    Font_SetSpacing(18, 18);
    Tex_Prepare(1, PLSEL_TEX_ICONS);
    Tex_Bind(PLSEL_TEX_ICONS);
    Plsel_DrawEquipIcon(338, 60, 0, Weapon_GetClass(preset->weaponType, preset->weaponId), 0);
    if (preset->weaponType == EQUIP_BLADE) {
        Text_PrintfAt(362, 60, 0, TXT(0x00D795C0) /* a full-width colon, then the equipment's name */, gBladeNames[preset->weaponId]);
    } else {
        Text_PrintfAt(362, 60, 0, TXT(0x00D795C0) /* a full-width colon, then the equipment's name */, gGunNames[preset->weaponId]);
    }
    for (i = 0, y = 84; i < 5; i++, y += 24) {
        Plsel_DrawEquipIcon(338, y, 1, i, 0);
    }
    Text_PrintfAt(362, 84, 0, TXT(0x00D795C0) /* a full-width colon, then the equipment's name */, gArmorHeadNames[preset->head]);
    Text_PrintfAt(362, 108, 0, TXT(0x00D795C0) /* a full-width colon, then the equipment's name */, gArmorBodyNames[preset->body]);
    Text_PrintfAt(362, 132, 0, TXT(0x00D795C0) /* a full-width colon, then the equipment's name */, gArmorArmsNames[preset->arms]);
    Text_PrintfAt(362, 156, 0, TXT(0x00D795C0) /* a full-width colon, then the equipment's name */, gArmorWaistNames[preset->waist]);
    Text_PrintfAt(362, 180, 0, TXT(0x00D795C0) /* a full-width colon, then the equipment's name */, gArmorLegsNames[preset->legs]);
    win.y = 220;
    win.lines = 4;
    ListPanel_Draw(&win, 0, 0, -1);
    rect.x0 = win.x;
    rect.y0 = win.y + win.charH * 3 + 4;
    rect.x1 = rect.x0 + win.charW * win.lineChars;
    rect.y1 = rect.y0 + 1;
    rect.color = win.color;
    Prim_FillRect(&rect);
    rect.y0 = win.y + win.charH * 3 + 6;
    rect.x1 = rect.x0 + win.charW * win.lineChars;
    rect.y1 = rect.y0 + 21;
    rect.color = (win.color & 0xFFFFFF) | 0x60000000;
    Prim_FillRect(&rect);
    Tex_Prepare(1, PLSEL_TEX_ICONS);
    Tex_Bind(PLSEL_TEX_ICONS);
    list = gStartItemLists[preset->startItems + gQuestSync.presetSet * 4];
    for (i = 0; *list != -1; i++) {
        list++;
        Plsel_DrawItemIcon(*list++, i % 10 * 28 + 329, i / 10 * 28 + 226);
    }
    Plsel_DrawPlayerList();
    switch (task->step) {
    case PLSEL_CHOOSE_BROWSE:
    default:
    case PLSEL_CHOOSE_RETURN:
        Font_SetPos(332, 287);
        Font_SetSpacing(18, 18);
        if (gScene.playerCount < 2) {
            Text_Printf(gPlselFmtString, gPlselHelpOffline);
        } else {
            Text_Printf(gPlselFmtString, gPlselHelpOnline);
        }
        Plsel_DrawEquipPanel();
        preset = Plsel_CurPreset();
        Font_SetPos(326, 336);
        sprintf(buf, TXT(0x00D795C8) /* the rate, then a full-width percent sign */, preset->pointRatePct);
        Sjis_Widen(buf, txt);
        Text_Printf(gPlselPointRateFormat, txt);
        break;
    case PLSEL_CHOOSE_CONFIRM:
    case 4: /* no code sets step 4; it draws as the question */
        Font_SetPos(338, 287);
        Font_SetSpacing(18, 18);
        Text_Printf(gPlselFmtString, gPlselNoRechooseText);
        Plsel_DrawEquipPanel();
        if (gScene.playerCount < 2) {
            x = (280 - Text_DisplayLength(gPlselStartPrompt) * 10) / 2.0f;
            Font_SetPos(x + 326, 344);
            Text_Printf(gPlselFmtString, gPlselStartPrompt);
        } else {
            x = (280 - Text_DisplayLength(gPlselEquipPrompt) * 10) / 2.0f;
            Font_SetPos(x + 326, 344);
            Text_Printf(gPlselFmtString, gPlselEquipPrompt);
        }
        x = (280 - Text_DisplayLength(gPlselYesNoTexts[0]) * 10) / 2.0f;
        Font_SetPos(x + 326, 364);
        Text_Printf(gPlselFmtString, gPlselYesNoTexts[task->flag & 1]);
        break;
    case PLSEL_CHOOSE_TAKEN:
        Font_SetPos(338, 287);
        Font_SetSpacing(18, 18);
        Text_Printf(gPlselFmtString, gPlselNoRechooseText);
        Plsel_DrawEquipPanel();
        if (gScene.playerCount > 1 && !(gFrameTick & 0x10)) {
            x = (280 - Text_DisplayLength(gPlselWaitLine1) * 10) / 2.0f;
            Font_SetPos(x + 326, 344);
            Text_Printf(gPlselFmtString, gPlselWaitLine1);
            x = (280 - Text_DisplayLength(gPlselWaitLine2) * 10) / 2.0f;
            Font_SetPos(x + 326, 364);
            Text_Printf(gPlselFmtString, gPlselWaitLine2);
        }
        break;
    }
    if (Game_IsOnline() != 0 && task->timeLeft != 0) {
        Text_SetColor(0);
        if (task->step == PLSEL_CHOOSE_MENU) {
            Font_SetPos(460, 28);
        } else {
            Font_SetPos(460, 406);
        }
        Text_Printf(gPlselTexts[112], task->timeLeft / 30);
    }
}


/*
 * Draws the frame and header row 3, hooks Plsel_DrawEquipPage into the draw
 * list, poses preview players 0-3, queues the one under the cursor, and
 * prints the online time left.
 */
void Plsel_DrawChooseScreen(TaskSlot *task)
{
    QuestSync *sync;
    s32 i;
    Player *pl;

    sync = &gQuestSync;
    Plsel_DrawFrame(0);
    Plsel_DrawHeader(3);
    gSelectDrawHook.owner = task;
    gSelectDrawHook.draw = Plsel_DrawEquipPage;
    OT_AddAt(gDrawOtSorted, &gSelectDrawHook, 0, 64);
    for (i = 0, pl = gPlayers; i < PLSEL_SET_PRESETS; i++, pl++) {
        Player_UpdateMotionStep(pl);
        pl->pos[1] = -40.0f;
        pl->rot[1] = sync->yaw;
        if (i == sync->pick) {
            OT_AddByDepth(gDrawOt32, pl->drawSlot, 32, 0);
        }
    }
    if (Game_IsOnline() == 1 && task->timeLeft != 0) {
        Text_SetColor(0);
        if (task->step == PLSEL_CHOOSE_MENU) {
            Font_SetPos(460, 28);
        } else {
            Font_SetPos(460, 406);
        }
        Text_Printf(gPlselTexts[112], task->timeLeft / 30);
    }
}

/* PLSEL_SEL_CHOOSE, choosing a preset hunter (PlselChooseStep): right and
 * left cycle the set's four presets (their arrows bump), PAD_START opens
 * the player menu (Plsel_OpenPlayerMenu), PAD_CIRCLE asks for the
 * confirmation, and when the online countdown runs out the preset under
 * the cursor is taken at once (the goto into the confirmation's branch). */
void Plsel_StateChoose(TaskSlot *task)
{
    QuestSync *sync;
    u16 pad;
    s8 chat;
    PlselPreset *preset;

    chat = Kbd_IsOpen();
    pad = gPadState[0].pressed | gPadState[0].repeat;
    sync = &gQuestSync;
    sync->yaw += 0x100;
    if ((gPadState[0].dir & PAD_RSTICK_LEFT) && chat == 0) {
        sync->yaw -= 0x500;
    }
    if ((gPadState[0].dir & PAD_RSTICK_RIGHT) && chat == 0) {
        sync->yaw += 0x300;
    }
    if (sync->timer > 0) {
        sync->timer--;
    }
    if (Game_IsOnline() == 1 && --task->timeLeft == 0) {
        gScene.menuOpen = 0;
        goto start;
    }
    switch (task->step) {
    case PLSEL_CHOOSE_BROWSE:
        if (sync->timer != 0) {
            break;
        }
        if (chat == 0 && (gPadState[0].pressed & PAD_START)) {
            Plsel_OpenPlayerMenu(sync->pick);
            task->step = PLSEL_CHOOSE_MENU;
            task->flag = 0;
            break;
        }
        if (gQuestSync.bumpLeft != 0) {
            gQuestSync.bumpLeft--;
        }
        if (gQuestSync.bumpRight != 0) {
            gQuestSync.bumpRight--;
        }
        if (Plsel_ChatUpdate() == 1) {
            break;
        }
        if (pad & PAD_RIGHT) {
            gQuestSync.bumpRight = 5;
            if (sync->pick >= PLSEL_SET_PRESETS - 1) {
                sync->pick = 0;
            } else {
                sync->pick++;
            }
            Plsel_PlayCursorSe();
            QSync_SendState(QSYNC_STATE_PICK_CURSOR, gScene.localPlayer);
        } else if (pad & PAD_LEFT) {
            gQuestSync.bumpLeft = 5;
            if (sync->pick == 0) {
                sync->pick = PLSEL_SET_PRESETS - 1;
            } else {
                sync->pick--;
            }
            Plsel_PlayCursorSe();
            QSync_SendState(QSYNC_STATE_PICK_CURSOR, gScene.localPlayer);
        } else if (gPadState[0].pressed & PAD_CIRCLE) {
            Plsel_PlayDecideSe();
            task->step = PLSEL_CHOOSE_CONFIRM;
            task->flag = 1;
        }
        break;
    case PLSEL_CHOOSE_RETURN:
        if (FadeA_GetState() != 1) {
            task->mode = PLSEL_SEL_START_CHOOSE;
            task->step = PLSEL_CHOOSE_BROWSE;
            Plsel_SetCamera();
            FadeA_Start(FADE_IN_BLACK);
            sync->timer = 15;
            return;
        }
        break;
    case PLSEL_CHOOSE_MENU:
        Plsel_PlayerMenuUpdate(pad);
        if (gScene.menuOpen == 0) {
            task->step = PLSEL_CHOOSE_BROWSE;
        }
        break;
    case PLSEL_CHOOSE_CONFIRM:
        if (gPadState[0].pressed & PAD_CIRCLE) {
            if (task->flag == 0) {
            start:
                preset = Plsel_CurPreset();
                QSync_SendState(QSYNC_STATE_PICK_TAKEN, gScene.localPlayer);
                sync->picks[gScene.localPlayer] = sync->pick | 0x80;
                sync->pointRatePct = preset->pointRatePct;
                Plsel_PlayChooseSe();
                task->mode++;
                task->step = PLSEL_CHOOSE_TAKEN;
                task->flag = 0;
                task->count = 0;
            } else {
                task->step = PLSEL_CHOOSE_BROWSE;
                Sound_PlayCancel();
            }
        } else if (gPadState[0].pressed & PAD_CROSS) {
            Sound_PlayCancel();
            if (task->flag == 1) {
                task->step = PLSEL_CHOOSE_BROWSE;
            } else {
                task->flag = 1;
            }
        } else if ((gPadState[0].pressed & PAD_LEFT) && task->flag == 1) {
            Plsel_PlayCursorSe();
            task->flag = 0;
        } else if ((gPadState[0].pressed & PAD_RIGHT) && task->flag == 0) {
            Plsel_PlayCursorSe();
            task->flag = 1;
        }
        break;
    }
    Plsel_DrawChooseScreen(task);
}

/*
 * PLSEL_SEL_READY, by TaskSlot.flag: waits for every player in the quest to
 * take a preset hunter (sending QSYNC_STATE_PICK_TAKEN again every 64
 * frames), then stops the keyboard, fades the music out over 10 frames and
 * the screen; then waits for the fade and 60 frames more, and closes the
 * overlay.
 */
void Plsel_StateReady(TaskSlot *task)
{
    QuestSync *sync;
    s32 i;

    sync = &gQuestSync;
    gQuestSync.yaw += 0x100;
    if (Game_IsOnline() == 1 && task->timeLeft != 0) {
        task->timeLeft--;
    }
    switch (task->flag) {
    case 0:
        task->flag++;
    case 1:
        Plsel_ChatUpdate();
        for (i = 0; i < gScene.playerCount; i++) {
            if (gScene.inQuest[i] == IN_QUEST_YES && !(sync->picks[i] & QSYNC_PICK_TAKEN)) {
                break;
            }
        }
        if (i >= gScene.playerCount) {
            if ((s8)Kbd_IsOpen() != 0) {
                Kbd_Cancel();
            }
            Stream_FadeOut(0, 10);
            FadeA_Start(FADE_OUT_BLACK_SLOW);
            task->flag++;
        } else {
            task->count = (task->count + 1) & 0x3F;
            if (task->count == 0) {
                QSync_SendState(QSYNC_STATE_PICK_TAKEN, gScene.localPlayer);
            }
        }
        break;
    case 2:
        if (FadeA_GetState() != 1) {
            task->count = 60;
            task->flag++;
        }
        break;
    case 3:
        if (--task->count == 0) {
            gScene.plselOpen = 0;
            gSysSettings.unk03 = 1;
            Game_ResetSubsystems();
            TaskSlot_End(task);
            gScene.allLocal = 0;
            gSysSettings.plselLoaded = 0;
            return;
        }
        break;
    }
    if (task->flag < 3) {
        Plsel_DrawChooseScreen(task);
    }
}

/*
 * Called by sub_main's player setup in a training quest: when player idx
 * took a preset hunter, equips it with that preset's weapon and armour and
 * sets its startItemList (set * 4 + preset + 1) and presetEquip.
 */
void Plsel_ApplyPresetEquip(Player *pl, s32 idx)
{
    s16 set;
    u8 pick;
    s16 presetIndex;
    PlselPreset *preset;

    set = gQuestSync.presetSet;
    pick = gQuestSync.picks[idx];
    if (pick & QSYNC_PICK_TAKEN) {
        presetIndex = pick & QSYNC_PICK_INDEX;
        preset = gPlselPresets[set];
        preset += presetIndex;
        pl->equip[EQUIP_SLOT_WEAPON].used = 1;
        pl->equip[EQUIP_SLOT_WEAPON].type = preset->weaponType;
        pl->equip[EQUIP_SLOT_WEAPON].level = preset->weaponLevel;
        pl->equip[EQUIP_SLOT_WEAPON].deco[0] = 0;
        pl->equip[EQUIP_SLOT_WEAPON].deco[1] = 0;
        pl->equip[EQUIP_SLOT_WEAPON].deco[2] = 0;
        pl->equip[EQUIP_LEGS].used = 1;
        pl->equip[EQUIP_LEGS].type = EQUIP_LEGS;
        pl->equip[EQUIP_LEGS].level = 0;
        pl->equip[EQUIP_LEGS].deco[0] = 0;
        pl->equip[EQUIP_LEGS].deco[1] = 0;
        pl->equip[EQUIP_LEGS].deco[2] = 0;
        pl->equip[EQUIP_SLOT_FACE].used = 1;
        pl->equip[EQUIP_SLOT_FACE].type = EQUIP_SLOT_FACE;
        pl->equip[EQUIP_SLOT_FACE].level = 0;
        pl->equip[EQUIP_SLOT_FACE].deco[0] = 0;
        pl->equip[EQUIP_SLOT_FACE].deco[1] = 0;
        pl->equip[EQUIP_SLOT_FACE].deco[2] = 0;
        pl->equip[EQUIP_HEAD].used = 1;
        pl->equip[EQUIP_HEAD].type = EQUIP_HEAD;
        pl->equip[EQUIP_HEAD].level = 0;
        pl->equip[EQUIP_HEAD].deco[0] = 0;
        pl->equip[EQUIP_HEAD].deco[1] = 0;
        pl->equip[EQUIP_HEAD].deco[2] = 0;
        pl->equip[EQUIP_BODY].used = 1;
        pl->equip[EQUIP_BODY].type = EQUIP_BODY;
        pl->equip[EQUIP_BODY].level = 0;
        pl->equip[EQUIP_BODY].deco[0] = 0;
        pl->equip[EQUIP_BODY].deco[1] = 0;
        pl->equip[EQUIP_BODY].deco[2] = 0;
        pl->equip[EQUIP_ARMS].used = 1;
        pl->equip[EQUIP_ARMS].type = EQUIP_ARMS;
        pl->equip[EQUIP_ARMS].level = 0;
        pl->equip[EQUIP_ARMS].deco[0] = 0;
        pl->equip[EQUIP_ARMS].deco[1] = 0;
        pl->equip[EQUIP_ARMS].deco[2] = 0;
        pl->equip[EQUIP_WAIST].used = 1;
        pl->equip[EQUIP_WAIST].type = EQUIP_WAIST;
        pl->equip[EQUIP_WAIST].level = 0;
        pl->equip[EQUIP_WAIST].deco[0] = 0;
        pl->equip[EQUIP_WAIST].deco[1] = 0;
        pl->equip[EQUIP_WAIST].deco[2] = 0;
        pl->equip[EQUIP_SLOT_WEAPON].id = preset->weaponId;
        pl->equip[EQUIP_LEGS].id = preset->legs;
        pl->equip[EQUIP_HEAD].id = preset->head;
        pl->equip[EQUIP_BODY].id = preset->body;
        pl->equip[EQUIP_ARMS].id = preset->arms;
        pl->equip[EQUIP_WAIST].id = preset->waist;
        pl->startItemList = preset->startItems + gQuestSync.presetSet * 4 + 1;
        pl->presetEquip = 1;
        pl->weaponKind = Equip_GetKind(&pl->equip[EQUIP_SLOT_WEAPON]);
    }
}

/*
 * Clears the player's 20 pouch slots, then fills them from
 * gStartItemLists[set * 4 + presetIndex]: (count, item) pairs ending at -1.
 */
void Plsel_SetupPresetPouch(Player *pl, s32 presetIndex)
{
    s16 i;
    s16 *list;
    s16 itemCount;
    u16 itemId;

    for (i = 0; i < 20; i++) {
        pl->pouch[i].item = 0;
        pl->pouch[i].count = 0;
    }
    pl->startItemList = (u8)presetIndex + gQuestSync.presetSet * 4;
    list = gStartItemLists[pl->startItemList];
    itemCount = list[0];
    while (itemCount != -1) {
        itemId = list[1];
        list += 2;
        Pouch_SetAtLeast(pl, 0, itemId, itemCount);
        itemCount = list[0];
    }
}

/*
 * In a training quest (Quest_IsTraining), equips the stage's own preset
 * (gTrainingPresets by gQuest.stage) and sets startItemList to the stage + 1.
 */
void Plsel_ApplyStageEquip(Player *pl)
{
    PlselPreset *preset;

    preset = gTrainingPresets[gQuest.stage];
    pl->equip[EQUIP_SLOT_WEAPON].used = 1;
    pl->equip[EQUIP_SLOT_WEAPON].type = preset->weaponType;
    pl->equip[EQUIP_SLOT_WEAPON].level = preset->weaponLevel;
    pl->equip[EQUIP_SLOT_WEAPON].deco[0] = 0;
    pl->equip[EQUIP_SLOT_WEAPON].deco[1] = 0;
    pl->equip[EQUIP_SLOT_WEAPON].deco[2] = 0;
    pl->equip[EQUIP_LEGS].used = 1;
    pl->equip[EQUIP_LEGS].type = EQUIP_LEGS;
    pl->equip[EQUIP_LEGS].level = 0;
    pl->equip[EQUIP_LEGS].deco[0] = 0;
    pl->equip[EQUIP_LEGS].deco[1] = 0;
    pl->equip[EQUIP_LEGS].deco[2] = 0;
    pl->equip[EQUIP_SLOT_FACE].used = 1;
    pl->equip[EQUIP_SLOT_FACE].type = EQUIP_SLOT_FACE;
    pl->equip[EQUIP_SLOT_FACE].level = 0;
    pl->equip[EQUIP_SLOT_FACE].deco[0] = 0;
    pl->equip[EQUIP_SLOT_FACE].deco[1] = 0;
    pl->equip[EQUIP_SLOT_FACE].deco[2] = 0;
    pl->equip[EQUIP_HEAD].used = 1;
    pl->equip[EQUIP_HEAD].type = EQUIP_HEAD;
    pl->equip[EQUIP_HEAD].level = 0;
    pl->equip[EQUIP_HEAD].deco[0] = 0;
    pl->equip[EQUIP_HEAD].deco[1] = 0;
    pl->equip[EQUIP_HEAD].deco[2] = 0;
    pl->equip[EQUIP_BODY].used = 1;
    pl->equip[EQUIP_BODY].type = EQUIP_BODY;
    pl->equip[EQUIP_BODY].level = 0;
    pl->equip[EQUIP_BODY].deco[0] = 0;
    pl->equip[EQUIP_BODY].deco[1] = 0;
    pl->equip[EQUIP_BODY].deco[2] = 0;
    pl->equip[EQUIP_ARMS].used = 1;
    pl->equip[EQUIP_ARMS].type = EQUIP_ARMS;
    pl->equip[EQUIP_ARMS].level = 0;
    pl->equip[EQUIP_ARMS].deco[0] = 0;
    pl->equip[EQUIP_ARMS].deco[1] = 0;
    pl->equip[EQUIP_ARMS].deco[2] = 0;
    pl->equip[EQUIP_WAIST].used = 1;
    pl->equip[EQUIP_WAIST].type = EQUIP_WAIST;
    pl->equip[EQUIP_WAIST].level = 0;
    pl->equip[EQUIP_WAIST].deco[0] = 0;
    pl->equip[EQUIP_WAIST].deco[1] = 0;
    pl->equip[EQUIP_WAIST].deco[2] = 0;
    pl->equip[EQUIP_SLOT_WEAPON].id = preset->weaponId;
    pl->equip[EQUIP_LEGS].id = preset->legs;
    pl->equip[EQUIP_HEAD].id = preset->head;
    pl->equip[EQUIP_BODY].id = preset->body;
    pl->equip[EQUIP_ARMS].id = preset->arms;
    pl->equip[EQUIP_WAIST].id = preset->waist;
    pl->presetEquip = 1;
    pl->weaponKind = Equip_GetKind(&pl->equip[EQUIP_SLOT_WEAPON]);
    pl->startItemList = gQuest.stage + 1;
}

/* Plays SE_SYS_CONFIRM. */
void Plsel_PlayDecideSe(void)
{
    Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
}

/* Plays SE_SYS_CURSOR. */
void Plsel_PlayCursorSe(void)
{
    Sound_PlaySeFlat(0, SE_SYS_CURSOR, 0);
}

/* Plays sound bank 1's sound 4 (select_chara.c plays it too). */
void Plsel_PlayChooseSe(void)
{
    Sound_PlaySeFlat(1, 4, 0);
}

/*
 * Returns 2 when a keyboard key asks for the chat, 1 for PAD_TRIANGLE, else 0.
 */
s32 Plsel_ChatOpenRequested(void)
{
    u16 pad;

    pad = gPadState[0].pressed | (gPadState[0].repeat & PAD_DPAD);
    if (Kbd_ActHotkeys() == 1) {
        return 2;
    }
    if (pad & PAD_TRIANGLE) {
        return 1;
    }
    return 0;
}

/*
 * Gives the held and pressed buttons for the keyboard: the pad's, plus the
 * right stick's direction as one D-pad bit.
 */
void Plsel_GetPad(u16 *outHeld, u16 *outPressed)
{
    u16 stick;

    *outHeld = gPadState[0].held;
    *outPressed = gPadState[0].pressed;
    stick = gPadState[0].dir;
    if (stick & PAD_RSTICK_UP) {
        *outHeld |= PAD_UP;
    } else if (stick & PAD_RSTICK_DOWN) {
        *outHeld |= PAD_DOWN;
    } else if (stick & PAD_RSTICK_LEFT) {
        *outHeld |= PAD_LEFT;
    } else if (stick & PAD_RSTICK_RIGHT) {
        *outHeld |= PAD_RIGHT;
    }
    stick = gPadState[0].dirPressed;
    if (stick & PAD_RSTICK_UP) {
        *outPressed |= PAD_UP;
    } else if (stick & PAD_RSTICK_DOWN) {
        *outPressed |= PAD_DOWN;
    } else if (stick & PAD_RSTICK_LEFT) {
        *outPressed |= PAD_LEFT;
    } else if (stick & PAD_RSTICK_RIGHT) {
        *outPressed |= PAD_RIGHT;
    }
}

/*
 * Clears the select screen's work and copies each member's name into its
 * player.
 */
void Plsel_InitSelWork(void)
{
    Player *pl;
    s32 i;

    pl = gPlayers;
    memset(gPlselSelWork, 0, sizeof(PlselSelWork));
    for (i = 0; i < gScene.playerCount; i++, pl++) {
        strcpy((char *)pl->name, gMemberNames[i]);
    }
}

/*
 * Opens the chat keyboard when mode is nonzero (Plsel_ChatOpenRequested's
 * result; 2 opens it without the focus), as sub_main's Chat_OpenInput does.
 */
void Plsel_ChatOpenKbd(s32 mode)
{
    PlselSelWork *w;
    s32 flags;

    w = gPlselSelWork;
    if (mode != 0) {
        w->kbdOpen++;
        flags = 4;
        if (mode == 2) {
            flags |= CHAT_KBD_INACTIVE;
        }
        Kbd_SetPos(110, 80.0f);
        Chat_OpenKbd(flags | CHAT_KBD_TARGETED, 14, 44, 0);
        Kbd_SetSubmitWait(w->sendWait);
        Sound_PlaySeFlat(0, SE_SYS_OPEN, 0);
        Plsel_ChatInput(0);
    }
}

/*
 * Returns the player slot the chat's target names when that player is in
 * the quest and not the local one, else -1 (everyone).
 */
s32 Plsel_ChatTargetPlayer(u8 *unused)
{
    s32 n;
    s32 i;

    n = gChatTarget;
    if (n < 2) {
        return -1;
    }
    n--;
    for (i = 0; i < gScene.playerCount; i++) {
        if (i == gScene.localPlayer) {
            n--;
        } else if (gScene.inQuest[i] != IN_QUEST_YES) {
            n--;
        } else {
            n--;
            if (n == 0) {
                return i;
            }
        }
    }
    return -1;
}

/*
 * Runs the keyboard for a frame. A text sent that isn't blank after the
 * filter is logged, sent to its target (or QSYNC_CHAT_TO_ALL) and shows the
 * log for 300 frames; then the send waits 60 frames and the keyboard closes.
 */
void Plsel_ChatInput(u16 unused)
{
    s16 held;
    s16 pressed;
    u8 text[76];
    u8 mask;
    Player *pl;
    PlselSelWork *w;
    s8 result;
    s8 target;

    w = gPlselSelWork;
    Plsel_GetPad((u16 *)&held, (u16 *)&pressed);
    text[0] = 0;
    result = Plsel_ChatRunKbd(text, held, pressed);
    if (result != 0) {
        if ((s8)text[0] != 0 && result > 0) {
            if (Chat_FilterIfNotBlank(text) != 0) {
                target = Plsel_ChatTargetPlayer(text);
                if ((s8)text[0] != 0) {
                    pl = &gPlayers[gScene.localPlayer];
                    if (target < 0) {
                        mask = 255;
                    } else {
                        mask = 1 << target;
                    }
                    Plsel_AddChatLog(pl->index, (char *)text, 0, mask);
                    QSync_SendChat(pl->index, QSYNC_CHAT_TEXT, (char *)text, mask);
                    w->newMsg = 1;
                    w->newMsgMine = 1;
                    w->logShowTimer = 300;
                }
            }
            w->sendWait = 60;
        }
        Kbd_Cancel();
        w->kbdOpen = 0;
    } else {
        w->newMsg = w->logShowTimer = 0;
    }
}

/* Counts down the chat's send wait and the log's display timer. */
void Plsel_TickTimers(void)
{
    PlselSelWork *w;

    w = gPlselSelWork;
    if (w->sendWait != 0) {
        w->sendWait--;
    }
    if (w->logShowTimer != 0) {
        w->logShowTimer--;
    }
}

/*
 * Online: opens the keyboard when asked, or runs it while it is open.
 * Returns 1 while the keyboard takes the input.
 */
s32 Plsel_ChatUpdate(void)
{
    PlselSelWork *w;

    w = gPlselSelWork;
    if (Game_IsOnline() == 0) {
        return 0;
    }
    switch (w->kbdOpen) {
    case 0:
        Plsel_ChatOpenKbd(Plsel_ChatOpenRequested());
        break;
    case 1:
        Plsel_ChatInput(gPadState[0].pressed | (gPadState[0].repeat & PAD_DPAD));
        return 1;
    }
    return 0;
}

/*
 * PAD_SELECT cancels (dst emptied, 255 returned); otherwise returns Kbd_Run's result.
 */
u8 Plsel_ChatRunKbd(u8 *dst, s32 held, s16 pressed)
{
    if (pressed & PAD_SELECT) {
        *dst = 0;
        return 255;
    }
    return Kbd_Run((char *) dst, held, pressed);
}

/*
 * Adds msg to the 64-entry chat log ring. The colours, id and name come from
 * the sender's record `from`, or for NULL the local player's (its slot
 * colour, class colour 3 to everyone or 1 to one player). The text is split
 * into two lines of 22 bytes without cutting a two-byte character.
 */
void Plsel_AddChatLog(u8 who, char *msg, NetUserRec *from, u8 flag)
{
    PlselSelWork *w;
    ChatLine *entry;
    s32 i;
    s32 room;
    char *dst;
    char *line;
    s32 k;
    u8 byte;

    w = gPlselSelWork;
    if (*msg == 0) {
        return;
    }
    entry = &w->log[w->logHead];
    entry->who = who;
    if (from != NULL) {
        entry->color[0] = from->color[0];
        entry->color[1] = from->color[1];
        entry->color[2] = from->color[2];
        entry->color[3] = from->color[3];
        strcpy((char *)entry->id, from->u.id);
        strcpy((char *)entry->name, from->u.name);
    } else {
        if (flag == 255) {
            flag = 3;
        } else {
            flag = 1;
        }
        entry->color[0] = 0;
        entry->color[1] = entry->color[2] = gPlselSlotColors[entry->who];
        entry->color[3] = gChatClassColors[flag];
        strcpy((char *)entry->id, gMyUserId);
        strcpy((char *)entry->name, gMyName);
    }
    w->logHead++;
    w->logHead &= PLSEL_CHAT_LOG_LEN - 1;
    w->logCount++;
    if (w->logCount > PLSEL_CHAT_LOG_LEN) {
        w->logCount = PLSEL_CHAT_LOG_LEN;
    }
    entry->rows = 0;
    for (i = 0, line = (char *)entry->text; i < 2; i++, line += sizeof(entry->text[0])) {
        dst = line;
        if (*msg == 0) {
            line[0] = 0;
            return;
        }
        line[PLSEL_CHAT_ROW_LEN] = 0;
        room = PLSEL_CHAT_ROW_LEN;
        do {
            if (*msg == 0) {
                *dst = 0;
                entry->rows++;
                return;
            }
            byte = *msg;
            if ((byte >= 128 && byte <= 159) || (byte >= 224 && byte <= 255)) {
                dst[0] = byte;
                dst[1] = msg[1];
                room -= 2;
                msg += 2;
                dst += 2;
            } else {
                *dst = byte;
                msg++;
                dst++;
                room--;
            }
        } while (room > 0);
        *dst = 0;
        entry->rows++;
    }
}

/*
 * Draws the chat window (gPlselChatWin) with the title text, at character
 * size n, in frame style color.
 */
void Plsel_DrawChatWindow(s32 text, u8 n, s32 color)
{
    s16 pos[2];
    Panel w;
    s32 lineH;

    lineH = n * 18 / 21;
    pos[0] = 0;
    pos[1] = 420 - (lineH + 1) * 12;
    w = gPlselChatWin;
    w.charW = n;
    w.charH = lineH - 1;
    w.flags = ((color + 2) & WINDOW_STYLE_MASK) | WINDOW_FILL;
    Window_DrawText(&w, (s16 *)pos, (u8 *)text);
}

extern char *gChatTypeTexts;
/*
 * Writes a log entry's name tag: the name's format, then the sender's slot
 * number.
 */
void Plsel_FormatPlayerTag(char *buf, ChatLine *entry)
{
    char tag[64];
    char wide[64];

    sprintf(buf, gChatTypeTexts, entry->name);
    sprintf(tag, "%dP", entry->who + 1);
    Sjis_Widen(tag, wide);
    strcat(buf, wide);
}

/* Returns the rows the log takes: each entry's lines, plus one per name tag. */
s32 Plsel_ChatLogLineCount(void)
{
    PlselSelWork *w;
    s32 idx;
    s32 n;
    s32 sum;
    ChatLine *entry;

    w = gPlselSelWork;
    sum = 0;
    n = w->logCount;
    idx = w->logHead - 1;
    for (; n > 0; n--, idx--) {
        entry = &w->log[idx & (PLSEL_CHAT_LOG_LEN - 1)];
        sum += entry->rows;
        if (entry->id[0] != 0) {
            sum++;
        }
    }
    return sum;
}

/* Draws the chat log: newest first from the bottom when it is full (12
 * lines or more) and `all` is 0, else from the oldest down. The upward
 * branch has its own y offset (yOffUp) and counts lines in the parameter
 * all, which is dead there; the downward one keeps the message count in
 * all and counts lines in left (the match's form). */
void Plsel_DrawChatLog(u8 skip, s32 y, s32 all, u8 size)
{
    PlselSelWork *w;
    s32 idx;
    s32 left;
    s32 yOffDown;
    s16 yScaled;
    s16 lim;
    s16 small;
    s16 lineH;
    s16 fsz;
    ChatLine *entry;
    s16 lineY;
    char *row;
    s32 yOffUp;
    char buf[32];

    w = gPlselSelWork;
    if (w->logCount == 0) {
        return;
    }
    small = (s16)size * 20 / 21;
    fsz = (s16)size * 18 / 21;
    Font_SetSpacing(size, fsz);
    lineH = fsz + 1;
    yScaled = (s16)y * (u8)size / 21;
    if ((u32)Plsel_ChatLogLineCount() > 11 && (u8)all == 0) {
        lineY = 420 - lineH;
        lim = 420 - lineH * 11;
        idx = w->logHead - 1 - skip;
        left = w->logCount - skip;
        if (left == 0) {
            return;
        }
        yOffUp = yScaled;
        do {
            idx &= PLSEL_CHAT_LOG_LEN - 1;
            entry = &w->log[idx];
            Text_SetColor(entry->color[3]);
            for (all = entry->rows - 1; all >= 0; all--) {
                row = (char *)entry->text[all];
                Font_SetPos(24, lineY + yOffUp);
                if (row[PLSEL_CHAT_ROW_LEN] != 0) {
                    Font_SetSpacing(small, fsz);
                    Text_Draw(row);
                    Font_SetSpacing(size, fsz);
                } else {
                    Text_Draw(row);
                }
                lineY -= lineH;
                if (lineY < lim) {
                    return;
                }
            }
            if (entry->id[0] != 0) {
                Plsel_FormatPlayerTag(buf, entry);
                Text_DrawShadow2(24, (s16)(lineY + yOffUp), 1, entry->color[2], buf);
                lineY -= lineH;
                if (lineY < lim) {
                    return;
                }
            }
            left--;
            idx--;
        } while (left != 0);
    } else {
        all = w->logCount;
        idx = w->logHead - all;
        lineY = 420 - lineH * 11;
        if (all == 0) {
            return;
        }
        yOffDown = yScaled;
        do {
            idx &= PLSEL_CHAT_LOG_LEN - 1;
            entry = &w->log[idx];
            if (entry->id[0] != 0) {
                Plsel_FormatPlayerTag(buf, entry);
                Text_DrawShadow2(24, (s16)(lineY + yOffDown), 1, entry->color[2], buf);
                lineY += lineH;
                if (lineY >= 420) {
                    return;
                }
            }
            Text_SetColor(entry->color[3]);
            for (left = 0; left < entry->rows; left++) {
                row = (char *)entry->text[left];
                Font_SetPos(24, lineY + yOffDown);
                if (row[PLSEL_CHAT_ROW_LEN] != 0) {
                    Font_SetSpacing(small, fsz);
                    Text_Draw(row);
                    Font_SetSpacing(size, fsz);
                } else {
                    Text_Draw(row);
                }
                lineY += lineH;
                if (lineY >= 420) {
                    return;
                }
            }
            all--;
            idx++;
        } while (all != 0);
    }
}

/* Adds player `slot`'s chat message to the log (filtered, with the
 * player's name and colours), counts it and plays the chat sound
 * (QSync_OnChat calls it while the select screen is open). The empty
 * compound check keeps the colour stores in the game's order, for the
 * match. */
void Plsel_LogChatFrom(u8 slot, u8 flag, char *text)
{
    struct {
        NetUserRec user;
        u8 pad[4];              /* unused: the game's frame holds 292 bytes here */
    } rec;
    PlselSelWork *w;
    u8 k;

    w = gPlselSelWork;
    Chat_FilterWords(text);
    k = (flag == 255) ? 3 : 1;
    rec.user.color[0] = 0;
    rec.user.color[1] = rec.user.color[2] = gPlselSlotColors[slot];
    if (slot >= 10 || flag > 1) { /* compiled-out check */
    }
    rec.user.color[3] = gChatClassColors[k];
    strcpy(rec.user.u.name, gMemberNames[slot]);
    strcpy(rec.user.u.id, gPlayerIds[slot]);
    Plsel_AddChatLog(slot, (char *)text, &rec.user, 0);
    if ((u32)Plsel_ChatLogLineCount() > 11) {
        w->logScroll++;
        if (w->logScroll > 63) {
            w->logScroll = 63;
        }
    }
    w->newMsg = 1;
    w->newMsgMine = 0;
    w->logShowTimer = 300;
    Sound_PlaySeFlat(0, 24, 0);
}

/* Returns player slot's colour byte; slots above 3 take colour 4. */
u8 Plsel_SlotColor(s32 slot)
{
    if (slot > 3) {
        slot = 4;
    }
    return gPlselSlotColors[slot];
}
