/*
 * plsel: the player select overlay's entry (plsel_entry, a task with four
 * states) and the character select flow (0xD60280-0xD64560).
 */
#include "common.h"
#include "sprite.h"
#include "syssettings.h"
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
#include "plsel/plsel.h"   /* the texts this file uses (tools/content_gen.py) */

/* The game compiled this file at optimization level 4 (the build's -O3,p
 * plus MWCC's level-4 passes): Plsel_InitSlots matches only there, and every
 * other function compiles the same at both levels. */
#pragma optimization_level 4

void SeqA_Reset(void);
void Text_Printf(const char *fmt, ...);
s32 Tex_LoadPack(s32 id, s32 slot, s32 kind);
void Tex_LoadSystemPack(s32);
void Tex_LoadFile(s32 file, s32 slot, s32 kind);
void Game_ResetSubsystems(void);
void Area_ClearState(void);
void Player_ClearAll(void);
void Sound_PlaySeMusicVol(u32 slot, u32 id, s32 prog);
s32 Sound_LoadSelectBanks(void *task);
void func_195A00(void);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16, s16);
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel); /* sub_main: b is a ListPanel * */
void Text_SetColor(s16 color);

void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
/* unit's own declaration: with the definition's s8 (char *dst, s16 held, s16 pressed) (sub_main/unk_314E00.c), Plsel_ChatRunKbd: 4 of 17 words differ */
s32 Kbd_Run(char *dst, s32, s16);
s32 Kbd_ActHotkeys(void);
s32 sprintf(char *, const char *, ...);
char *strcat(char *dst, const char *src);
void Sjis_Widen(char *src, char *dst);
extern PlselPreset *gPlselPresets[];
extern u16 D_896340[];

void Gfx_SetState63(s32);
void Tex_Prepare(s32, s32);
void Tex_Bind(s32);
s32 Sprite_Draw(Sprite *s);
s32 Game_IsOnline(void);
void Plsel_ChatOpenKbd(s32 mode);
void Plsel_ChatInput(u16 unused);
char *strcpy(char *dst, const char *src);
void *memset(void *, s32, u32);
extern s32 gItemColors[];
extern char gMemberNames[][0x11];

void func_901790(s32, s32);
void TaskSlot_End(TaskSlot *t);
void Player_SetColorWord(Player *pl, u8 idx);
void Plsel_InitPlayer(Player *p, s16 id);
void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);
void Player_UpdateMotionStep(struct Player *);

s32 Pouch_SetAtLeast(Player *p, u16 mode, u16 item, s16 n);
extern s16 *gStartItemLists[];

typedef struct Point16 {
    s16 x;
    s16 y;
} Point16;

typedef struct UVRect {
    Point16 a;
    Point16 b;
} UVRect;


typedef struct PlselVec3 {
    f32 x;
    f32 y;
    f32 z;
} PlselVec3;

u8 Plsel_PresetSetForStage(void);
void Plsel_InitSelWork(void);

void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
void Kbd_SetSubmitWait(s32 arg0);
void Vec_Copy(f32 *dst, f32 *src);

void Window_DrawText(void *p, s16 *pos, u8 *text);

u16 Equip_GetKind(EquipItem *e);
extern PlselPreset *D_896D90[];

s32 Tex_Use(s32 n, u32 *handles);
void Plsel_Setup(TaskSlot *task);
void Plsel_Finish(TaskSlot *task);
void Plsel_WaitPlayersReady(TaskSlot *task);
void Plsel_DebugMenu(TaskSlot *task);
void Loading_Stop(void);
void Quest_Finish(void);
void Frame_Draw(void);
extern s32 D_6AE3D4;

void QuestSync_Clear(void);
void Plsel_InitSlots(void);

void Loading_SetMessage(u32 mode);

s16 func_364BC0(void);

void Chat_FilterWords(char *s);
void Plsel_AddChatLog(u8 who, char *msg, u8 *pl, u8 flag);
extern u8 gChatClassColors[];
extern char gPlayerIds[][8];

s32 Chat_FilterIfNotBlank(u8 *);
void func_901E80(u8, s32, void *, u8);
void Kbd_Cancel(void);
u8 Plsel_ChatRunKbd(u8 *, s32, s16);

void Plsel_DrawFrame(s32 arg0);
s32 Gs_SetState(u8 reg, u32 v);
void Plsel_DrawHeader(u16);
void Plsel_DrawEquipPage(u8 *ot);
s32 OT_AddAt(u32 *table, void *node, s32 idx, s32 n);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
extern u8 gSelectDrawHook[];

void Player_InitPartTable(s32 idx);
void Player_LoadPartModels(struct Player *);
void Player_ClearFxBlock(Player *p);
void Player_InitRecords(Player *p);
extern PlselVec3 gStartPos[];
void QNetIf_Ready(s32);
void Gs_SetTableColor(s32);
void Anim_LoadCommon(s32);
void Camera_Reset(void);
void CamCtl_Clear(void);
void Light_InitAll(s32 unused0);
void Plsel_SetupPresetPlayer(Player *, s16);
void SeqA_Start(s32);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void Kbd_Close(void);
void DrawCb_LocalPlayer(DrawSlot *r);

struct PlModels;
void Player_FreeModels(struct Player *p);
void Player_FreeWeaponModel(Player *p);
void Plsel_SetupSavedPlayer(Player *p, s16 id);
void QNetIf_Unready(void);
void Stream_Play(s32, s32);

void Plsel_SetupPresetPouch(Player *p, s32 n);
u8 Equip_GetClass(EquipItem *e);
u8 WeaponClass_IsGunner(u8);
u8 WeaponType_Flags(u8);
u8 Equip_GetBladeSpecial(EquipItem *e);
void Player_LoadWeaponModel(s32 kind, s32 pl);

s8 Kbd_IsOpen(void);
void Stream_FadeOut(s32 ch, s32 frames);
u8 SeqA_GetState(void);

extern char gMyUserId[];
extern char gMyName[];

extern char D_00D795A8[];

void Plsel_StateReady(TaskSlot *task);
void Plsel_SelectInit(TaskSlot *task);
void Plsel_SelectWaitSound(TaskSlot *task);
void Plsel_SelectStartChoose(TaskSlot *task);
void Plsel_SelectSetupPlayers(TaskSlot *task);
void Plsel_StateChoose(TaskSlot *task);
void Players_RunEquipEffects(void);
void Players_UpdateMatrices(void);
void LightSet_UpdateAll(void);
void Camera_Update(void);
s32 Gs_GetState(u8 reg);
s32 Gs_ClearColor(u8 mode, u32 color);
void Gfx_SetBlend(s32, s32);
void Chat_DrawKbdBox(s32 text);
void Text_FlushPages(void);
void Plsel_OnChatMessage(s32 arg0, u8 n, s32 arg2);
void Plsel_DrawChatLog(u8 skip, s32 y, s32 all, u8 size);
void Text_DrawAllPages(void);
void Plsel_TickTimers(void);

void Plsel_PlayDecideSe(void);
void Plsel_PlayCursorSe(void);
void Plsel_PlayChooseSe(void);
PlselPreset *Plsel_CurPreset(void);
void Plsel_SetCamera(void);
s32 Plsel_ChatUpdate(void);
void Plsel_DrawChooseScreen(TaskSlot *task);
void Plsel_PlayerMenuUpdate(u16 pad);
void Plsel_OpenPlayerMenu(u16 idx);
void Sound_PlayCancel(void);

/* .data (0xD75F80-0xD76060) */
PlselPresetPlayer gPlselPresetPlayers[2] = {
#include "plsel/00D75F80.inc"
};
u32 gPlselWeaponIconColors[4] = {
#include "plsel/00D75FA0.inc"
};
UVRect gPlselWeaponIconUVs[11] = {
#include "plsel/00D75FB0.inc"
};
u32 gPlselSlotColors[2] = {
#include "plsel/00D76008.inc"
};
ListPanel gPlselListPanel = {
#include "plsel/00D76010.inc"
};
u32 gPlselEquipIconIds[4] = {
#include "plsel/00D76030.inc"
};
Panel gPlselChatWin = {
#include "plsel/00D76040.inc"
};

void plsel_entry(TaskSlot *task) {
    if (gTexHandles[1] != 0) {
        Tex_Use(1, (u32 *) &D_6AE3D4);
    }
    switch (task->mode) {
    case 0:
        Plsel_Setup(task);
        break;
    case 1:
        Plsel_WaitPlayersReady(task);
        break;
    case 2:
        Plsel_Finish(task);
        break;
    case 3:
        Plsel_DebugMenu(task);
        break;
    }
    if (Game_IsOnline() == 1 && gScene.inQuest[gScene.localPlayer] == 0xFF) {
        Loading_Stop();
        gScene.scene = 5;
        Quest_Finish();
        SeqA_Reset();
        TaskSlot_End(task);
    } else {
        if (++gQuestSync.bumpLeft >= 60) {
            gQuestSync.bumpLeft = 0;
            func_901790(8, gScene.localPlayer);
        }
        Frame_Draw();
    }
}

extern s32 D_0042F1D8;
void Plsel_Setup(TaskSlot *task) {
    Game_ResetSubsystems();
    Player_ClearAll();
    Area_ClearState();
    SeqA_Reset();
    Tex_LoadFile(0, 1, 0);
    Tex_LoadSystemPack(0);
    Tex_LoadPack(D_0042F1D8, 0xA3, 0);
    task->unk14 = 0;
    task->mode++;
}

void Plsel_WaitPlayersReady(TaskSlot *task) {
    QuestSync *st;
    u16 self;
    s16 i;

    st = &gQuestSync;
    self = gScene.localPlayer;
    switch (task->step) {
    case 0:
        task->step++;
        QuestSync_Clear();
        Plsel_InitSlots();
        st->ready[self] = 1;
        st->timer = 0xE10;
        func_901790(2, gScene.localPlayer);
        break;
    case 1:
        if (Game_IsOnline() == 1 && gScene.inQuest[gScene.localPlayer] == 0xFF) {
            Loading_Stop();
            gScene.scene = 5;
            Quest_Finish();
            TaskSlot_End(task);
        } else if (--st->timer <= 0) {
            Loading_Stop();
            gScene.scene = 5;
            Quest_Finish();
            TaskSlot_End(task);
        } else {
            for (i = 0; i < 4; i++) {
                if (st->ready[i] == 0 && gScene.inQuest[i] != 0xFF) {
                    return;
                }
            }
            task->mode++;
            st->step = 0;
        }
        break;
    }
}

void Plsel_Finish(TaskSlot *task) {
    QuestSync *st;

    st = &gQuestSync;
    switch (st->step) {
    case 0:
        st->step++;
        st->timer = 0x1E;
        func_901790(2, gScene.localPlayer);
    case 1:
        if (--st->timer <= 0) {
            gSysSettings.unk03 = 1;
            Game_ResetSubsystems();
            TaskSlot_End(task);
            gScene.allLocal = 0;
        }
        break;
    }
}

static void func_00D606D0(QuestSync *st, TaskSlot *task) {

}

/* The raw pad and scene addresses stay here: gPadState[0] and gScene's fields
 * change the code (7 words). */
void Plsel_DebugMenu(TaskSlot *task) {
    QuestSync *st;
    u16 pad;
    u16 sum;
    s32 i;

    st = &gQuestSync;
    pad = gPadState[0].pressed | gPadState[0].repeat | gPadState[1].pressed | gPadState[1].repeat;
    switch (gQuestSync.step) {
    case 0:
        st->step++;
        st->cursor = 0;
        break;
    case 1:
        if (pad & 0x2000) {
            if (st->cursor <= 0) {
                st->cursor = 3;
            } else {
                st->cursor--;
            }
        }
        if (pad & 0x1000) {
            if (st->cursor >= 3) {
                st->cursor = 0;
            } else {
                st->cursor++;
            }
        }
        if (pad & 0x400) {
            if (st->emKind[st->cursor] >= 0x49) {
                st->emKind[st->cursor] = 0;
            } else {
                st->emKind[st->cursor]++;
            }
        }
        if (pad & 0x800) {
            if (st->emKind[st->cursor] <= 0) {
                st->emKind[st->cursor] = 0x49;
            } else {
                st->emKind[st->cursor]--;
            }
        }
        if (pad & 0x40) {
            if (st->emCount[st->cursor] != 0) {
                st->emCount[st->cursor]--;
            }
        }
        if (pad & 0x20) {
            if (st->emCount[st->cursor] < 0xF) {
                st->emCount[st->cursor]++;
            }
        }
        if (pad & 0x8000) {
            sum = 0;
            for (i = 0; i < 4; i++) {
                sum += st->emCount[i];
            }
            if (sum < 0x10) {
                Loading_SetMessage(4);
                gSysSettings.unk03 = 1;
                Game_ResetSubsystems();
                TaskSlot_End(task);
                gScene.allLocal = 0;
            }
        }
        break;
    }
    func_00D606D0(st, task);
}

/* Resets the select state's ten slots: online, the connected players' slots
 * are active (0) and the others unused (-1); offline, slot 0 is active and
 * the others 1. The local player's slot (0x6ADCEC) takes the record at
 * gSave.equip[0] (the weapon) when gSave.unk0 is set. */
void Plsel_InitSlots(void) {
    QuestSync *st;
    s32 i;

    st = &gQuestSync;
    for (i = 0; i < 10; i++) {
        if (Game_IsOnline() == 1) {
            if (i < gScene.playerCount) {
                st->ready[i] = 0;
            } else {
                st->ready[i] = -1;
            }
        } else if (i == 0) {
            st->ready[i] = 0;
        } else {
            st->ready[i] = 1;
        }
        st->gender[i] = (i & 1) != 0;
        st->startItemList[i] = 0;
        if (i == gScene.localPlayer) {
            st->presetEquip[i] = 0;
            if (gSave.used != 0) {
                st->weapon[i] = gSave.equip[0];
            } else {
                st->weapon[i].type = 6;
                st->weapon[i].id = 0;
                st->weapon[i].level = 0;
            }
        } else {
            st->weapon[i].type = 6;
            st->weapon[i].id = 0;
            st->weapon[i].level = 0;
        }
        st->unk16[i] = 4;
    }
    st->emKind[0] = func_364BC0();
    st->emCount[0] = 1;
    st->emKind[1] = 0;
    st->emCount[1] = 0;
    st->emKind[2] = 0;
    st->emCount[2] = 0;
    st->emKind[3] = 0;
    st->emCount[3] = 0;
}

extern s32 gChatTitleText;
void Plsel_SelectTask(TaskSlot *task) {
    PlselSelWork *w;
    s32 t;
    u8 k;

    w = gPlselSelWork;
    if (D_6AE3D4 != 0) {
        Tex_Use(1, (u32 *) &D_6AE3D4);
    }
    switch (task->mode) {
    case 0:
        Plsel_SelectInit(task);
        break;
    case 1:
        Plsel_SelectWaitSound(task);
        break;
    case 2:
        Plsel_SelectStartChoose(task);
        break;
    case 3:
        Plsel_SelectSetupPlayers(task);
        break;
    case 4:
        Plsel_StateChoose(task);
        break;
    case 5:
        Plsel_StateReady(task);
        break;
    }
    Players_RunEquipEffects();
    Players_UpdateMatrices();
    LightSet_UpdateAll();
    Camera_Update();
    Frame_Draw();
    if ((s8)Kbd_IsOpen() != 0) {
        t = Gs_GetState(0x6C);
        Gs_SetState(0x6C, 1);
        Gs_ClearColor(0, 0);
        Gs_SetState(0x6C, 0);
        Gfx_SetBlend(4, 5);
        Chat_DrawKbdBox(w->kbdOpen != 0 ? 0xBDB59F6C : 0xBD6C6CC5);
        Gs_SetState(0x6C, t);
        Text_FlushPages();
    }
    if ((s8)Kbd_IsOpen() != 0 || w->logShowTimer != 0) {
        Plsel_OnChatMessage(gChatTitleText, 0x12, 1);
        Plsel_DrawChatLog(0, 0, 0, 0x12);
        Text_DrawAllPages();
    }
    Plsel_TickTimers();
    if (Game_IsOnline() != 0) {
        k = gScene.inQuest[gScene.localPlayer];
        if (k != 1) {
            gSysSettings.unk03 = 1;
            gScene.plselOpen = 0;
            Game_ResetSubsystems();
            TaskSlot_End(task);
            gScene.allLocal = 0;
            gSysSettings.plselLoaded = 0;
        }
    }
}

void Plsel_DrawHeader(u16 row) {
    Sprite s;

    Gfx_SetState63(1);
    Tex_Prepare(1, 0xA5);
    Tex_Bind(0xA5);
    s.x = -0xA;
    s.u0 = 0;
    s.y = 0xA;
    s.w = 0xC0;
    s.h = 0x30;
    s.color = -1;
    s.v0 = row << 5;
    s.u1 = 0x7F;
    s.v1 = s.v0 + 0x1F;
    Sprite_Draw(&s);
}

void Plsel_DrawFrame(s32 unused) {
    Sprite s;

    Gs_SetState(0x6C, 0);
    Gfx_SetState63(1);
    Tex_Prepare(1, 0xA3);
    Tex_Bind(0xA3);
    s.x = 0;
    s.y = 0;
    s.w = 0x280;
    s.h = 0x1C0;
    s.color = -1;
    s.v0 = 0;
    s.u0 = 0;
    s.u1 = 0x1FF;
    s.v1 = 0x1BF;
    Sprite_Draw(&s);
    Tex_Prepare(1, 0xA4);
    Tex_Bind(0xA4);
    s.x = 0x20;
    s.y = 0x174;
    s.w = 0x100;
    s.h = 0x46;
    s.color = 0xE0202020;
    s.v0 = 0;
    s.u0 = 0;
    s.u1 = 0xFF;
    s.v1 = 0x45;
    Sprite_Draw(&s);
    Gs_SetState(0x6C, 1);
}

void Plsel_SetCamera(void) {
    Player *pl;

    pl = &gPlayers[5];
    Vec_Copy(gCamera->at, pl->pos);
    Vec_Copy(gCamera->eye, pl->pos);
    gCamera->at[0] += 40.0f;
    gCamera->eye[0] = gCamera->at[0];
    gCamera->at[1] += 125.0f;
    gCamera->eye[1] += 125.0f;
    gCamera->eye[2] += 160.0f;
}

extern s32 gSystemTexFiles[];
extern u16 D_006ADB48;
void Plsel_SelectInit(TaskSlot *task) {
    s16 i;
    Player *pl;

    QNetIf_Ready(0);
    Game_ResetSubsystems();
    Player_ClearAll();
    Area_ClearState();
    Gs_SetTableColor(0);
    Tex_LoadPack(gSystemTexFiles[1], 0x144, 0);
    Tex_LoadPack(gSystemTexFiles[3], 0xA3, 0);
    task->mode++;
    task->unk14 = 0;
    task->unk15 = gSysSettings.unk45;
    Plsel_InitSlots();
    D_006ADB48 = 0xFFFF;
    gQuestSync.presetSet = Plsel_PresetSetForStage();
    gQuestSync.builtSet = 0xFF;
    gQuestSync.pick = 1;
    gQuestSync.unkD9 = 0;
    Anim_LoadCommon(5);
    Camera_Reset();
    CamCtl_Clear();
    gScene.area = 0;
    Light_InitAll(gScene.area);
    gCamera->fov = 0.87266463f;
    gCamera->roll = 0.0f;
    Plsel_SetupPresetPlayer(&gPlayers[5], gSave.gender & 1);
    Plsel_SetCamera();
    SeqA_Start(2);
    for (i = 0, pl = gPlayers; i < 6; i++, pl++) {
        pl->drawSlotId = DrawSlot_Alloc();
        pl->index = i;
        if (pl->drawSlotId != -1) {
            pl->drawSlot = DrawSlot_Get(pl->drawSlotId);
            pl->drawSlot->player = pl->index;
            pl->drawSlot->draw = DrawCb_LocalPlayer;
        }
    }
    task->timeLeft = 0xE10;
    gScene.plselOpen = 1;
    Plsel_InitSelWork();
    Kbd_Close();
    gQuestSync.bumpLeft = 0;
    gQuestSync.bumpRight = 0;
}

void Plsel_SelectWaitSound(TaskSlot *task) {
    if (Sound_LoadSelectBanks(task) != 0) {
        func_195A00();
        task->mode++;
        task->step = 0;
        gQuestSync.timer = 0xF;
        Sound_PlaySeMusicVol(1, 0, 0);
        Sound_PlaySeMusicVol(1, 1, 0);
    }
}

void Plsel_InitPlayer(Player *p, s16 id) {
    p->active = 1;
    p->visible = 1;
    p->index = id;
    p->isEm = 0;
    p->alpha = 1.0f;
    p->motionLayerCount = 2;
    p->scale[0] = 1.0f;
    p->scale[1] = 1.0f;
    p->scale[2] = 1.0f;
    p->motion[0].step = 2.0f;
    p->motion[1].step = 2.0f;
    p->motionSpeed = 1.0f;
    p->weaponOut = 0;
    p->pos[0] = gStartPos[gScene.area].x;
    p->pos[1] = gStartPos[gScene.area].y;
    p->pos[2] = gStartPos[gScene.area].z;
    p->rot[1] = 0;
    p->rot[1] = 0xC00;
    p->rot[2] = 0;
    Player_InitPartTable(p->index);
    Player_LoadPartModels(p);
    Player_ClearFxBlock(p);
    Player_InitRecords(p);
}

void Plsel_SetupPresetPlayer(Player *p, s16 n) {
    PlselPresetPlayer *e;

    e = &gPlselPresetPlayers[n];
    p->weaponShown = 0;
    p->gender = e->gender;
    p->hairStyle = e->hairStyle;
    p->hairColor = e->hairColor;
    p->charType = 0;
    Player_SetColorWord(p, 2);
    p->lookVariant = 0;
    p->equip[2].id = e->head;
    p->equip[3].id = e->body;
    p->equip[4].id = e->arms;
    p->equip[5].id = e->waist;
    p->equip[0].id = e->legs;
    p->equip[1].id = e->unk1;
    p->weaponType = 0;
    p->weaponKind = 0;
    Plsel_InitPlayer(p, 5);
    Player_SetMotionPair(p, 0x2B9, 0, 0);
    Player_UpdateMotionStep(p);
}

/* The compiled-out check after the call (of the saved gender; any two-test
 * condition gives the same code) keeps &gSave in s0 for save[2] (the reads
 * before the call stay absolute), as the game does. */
void Plsel_SetupSavedPlayer(Player *p, s16 id) {
    PlselPreset *t;
    SaveData *save;

    save = &gSave;
    t = gPlselPresets[gQuestSync.presetSet];
    t += id;
    p->weaponShown = 1;
    p->gender = save->gender;
    p->hairStyle = save->hairStyle;
    p->hairColor = save->hairColor;
    p->charType = save->charType;
    Player_SetColorWord(p, save->symbolColor);
    if (save == NULL || save->gender > 1) { /* compiled-out check */
    }
    p->equip[0].used = 1;
    p->equip[0].type = 0;
    p->equip[0].level = 0;
    p->equip[0].deco[0] = 0;
    p->equip[0].deco[1] = 0;
    p->equip[0].deco[2] = 0;
    p->equip[1].used = 1;
    p->equip[1].type = 1;
    p->equip[1].level = 0;
    p->equip[1].deco[0] = 0;
    p->equip[1].deco[1] = 0;
    p->equip[1].deco[2] = 0;
    p->equip[2].used = 1;
    p->equip[2].type = 2;
    p->equip[2].level = 0;
    p->equip[2].deco[0] = 0;
    p->equip[2].deco[1] = 0;
    p->equip[2].deco[2] = 0;
    p->equip[3].used = 1;
    p->equip[3].type = 3;
    p->equip[3].level = 0;
    p->equip[3].deco[0] = 0;
    p->equip[3].deco[1] = 0;
    p->equip[3].deco[2] = 0;
    p->equip[4].used = 1;
    p->equip[4].type = 4;
    p->equip[4].level = 0;
    p->equip[4].deco[0] = 0;
    p->equip[4].deco[1] = 0;
    p->equip[4].deco[2] = 0;
    p->equip[5].used = 1;
    p->equip[5].type = 5;
    p->equip[5].level = 0;
    p->equip[5].deco[0] = 0;
    p->equip[5].deco[1] = 0;
    p->equip[5].deco[2] = 0;
    p->equip[2].id = t->head;
    p->equip[3].id = t->body;
    p->equip[4].id = t->arms;
    p->equip[5].id = t->waist;
    p->equip[0].id = t->legs;
    p->equip[1].id = save->face + 1;
    p->equip[6].used = 1;
    p->equip[6].type = t->weaponType;
    p->equip[6].level = t->weaponLevel;
    p->equip[6].deco[0] = 0;
    p->equip[6].deco[1] = 0;
    p->equip[6].deco[2] = 0;
    Plsel_SetupPresetPouch(p, t->startItems);
    p->equip[6].id = t->weaponId;
    p->weaponKind = Equip_GetKind(&p->equip[6]);
    p->weaponType = Equip_GetClass(&p->equip[6]);
    p->isGunner = WeaponClass_IsGunner(p->weaponType);
    p->weaponFlags = WeaponType_Flags(p->weaponType);
    p->bladeSpecial = Equip_GetBladeSpecial(&p->equip[6]);
    Plsel_InitPlayer(p, id);
    Player_LoadWeaponModel(p->weaponKind, p->index);
    Player_SetMotionPair(p, 1, 0, 0);
    Player_UpdateMotionStep(p);
}

PlselPreset *Plsel_CurPreset(void) {
    PlselPreset *p;
    s32 n;

    p = gPlselPresets[gQuestSync.presetSet];
    n = gQuestSync.pick;
    if (n >= 4) {
        n = 3;
    }
    return p + n;
}

u8 Plsel_PresetSetForStage(void) {
    u16 *p;
    s32 i;
    s32 n;
    u16 v;

    v = gQuest.stage;
    p = D_896340;
    n = 0;
    for (i = 0; i < 0x11; i++, p += 2, n++) {
        if (p[0] == v) {
            return n;
        }
        if (p[1] == v) {
            return n;
        }
    }
    return 0;
}

static s32 Plsel_SlotCount(u8 set) {
    return 4;
}

s32 Plsel_ViewedSlotNo(u8 set, u8 sel) {
    return (gQuestSync.pick + 1) & 0xFF;
}

void Plsel_SelectStartChoose(TaskSlot *task) {
    task->mode++;
    task->step = 0;
    gQuestSync.unkD3 = 0;
    gQuestSync.pick = 0;
}

void Plsel_SelectSetupPlayers(TaskSlot *task) {
    QuestSync *st;
    s16 i;
    Player *pl;

    task->mode++;
    task->step = 0;
    st = &gQuestSync;
    if (gQuestSync.presetSet != gQuestSync.builtSet) {
        st->builtSet = st->presetSet;
        for (i = 0, pl = gPlayers; i < 4; i++, pl++) {
            Player_FreeModels(pl);
            Player_FreeWeaponModel(pl);
            Plsel_SetupSavedPlayer(pl, i);
        }
    }
    st->yaw = 0x1000;
    gCamera->at[0] += 80.0f;
    gCamera->eye[0] = gCamera->at[0];
    gCamera->eye[2] += 190.0f;
    gCamera->at[1] -= 35.0f;
    gCamera->eye[1] -= 35.0f;
    SeqA_Start(2);
    QNetIf_Unready();
    Loading_Stop();
    Stream_Play(0, 0x125);
}

void Plsel_DrawEquipPanel(void) {
    ListPanel_Draw(&gPlselListPanel, 0, 0, -1);
    Text_SetColor(0);
    Font_SetPos(0x146, 0x14E);
    Font_SetSpacing(0x14, 0x14);
}

void Plsel_DrawWeaponIcon(s16 x, s16 y, s16 sz, s32 col, s32 idx) {
    Sprite s;

    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    s.x = x + 4;
    s.y = y + 2;
    s.w = sz;
    s.h = sz;
    s.color = gPlselWeaponIconColors[col & 3];
    *(Point16 *)&s.u0 = gPlselWeaponIconUVs[idx].a;
    *(Point16 *)&s.u1 = gPlselWeaponIconUVs[idx].b;
    Sprite_Draw(&s);
}

void Plsel_DrawItemIcon(u16 item, s16 x, s16 y) {
    Sprite s;
    ItemData *it;
    s32 n;
    s32 u;
    s32 v;

    it = &gItemData[item];
    n = it->icon + 1;
    s.x = x;
    s.y = y;
    s.w = 0x1B;
    s.h = 0x14;
    u = (n & 7) * 32;
    v = (n >> 3) * 32;
    s.u0 = u + 1;
    s.u1 = u + 0x1F;
    s.v0 = v + 1;
    s.v1 = v + 0x1F;
    s.color = gItemColors[it->color];
    Sprite_Draw(&s);
}

void Plsel_DrawEquipIcon(s16 x, s16 y, u8 flag, u8 n, s32 unused) {
    Sprite s;
    s32 id;
    s32 u;
    s32 v;

    if (flag == 0) {
        id = n + 0x3F;
    } else {
        id = ((u8 *)gPlselEquipIconIds)[n] + 0x4A;
    }
    s.x = x;
    s.y = y - 2;
    s.w = 0x18;
    s.h = 0x18;
    u = (id & 7) * 32;
    v = (id >> 3) * 32;
    s.u0 = u + 1;
    s.u1 = u + 0x1F;
    s.v0 = v + 1;
    s.v1 = v + 0x1F;
    s.color = -1;
    Sprite_Draw(&s);
}

/* The left (flag 0) or right arrow beside the slot being chosen, pushed out
 * by its bump counter (gQuestSync.bumpLeft and bumpRight, set to 5 when the arrow is
 * pressed). The counters are read as named data: with absolute addresses
 * MWCC can't move their loads above the stores to the sprite (a local
 * whose address is passed on). */
void Plsel_DrawArrow(s32 flag, s16 x, s16 y) {
    Sprite s;

    Gfx_SetState63(1);
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    s.w = 0x18;
    s.x = x;
    s.y = y;
    s.h = 0x1E;
    s.color = 0xFF32BC64;
    if (flag == 0) {
        s.x -= gQuestSync.bumpLeft;
        s.u0 = 0xFF;
        s.u1 = 0xF0;
    } else {
        s.x += gQuestSync.bumpRight;
        s.u0 = 0xF0;
        s.u1 = 0xFF;
    }
    s.v0 = 0xEC;
    s.v1 = 0xFF;
    Sprite_Draw(&s);
}

INCLUDE_RODATA("asm/plsel/nonmatchings/plsel", D_00D79598);

extern char *D_00446F60;
extern char *D_00446F64;
void Plsel_DrawPlayerList(void) {
    ListPanel win;
    char buf[0x10];
    char txt[0x10];
    s16 x;
    s16 i;

    x = 0x1E;
    win = gPlselListPanel;
    win.flags = 0;
    Font_SetSpacing(0x12, 0x12);
    win.x = x;
    win.y = 0x50;
    win.charW = 0x12;
    win.charH = 0xF;
    win.lineChars = 3;
    win.lines = 3;
    ListPanel_Draw(&win, 0, 0, -1);
    Plsel_DrawWeaponIcon(x, 0x50, 0x30, gScene.localPlayer, (s16)gPlayers[gQuestSync.pick].weaponType);
    sprintf(buf, D_00446F60, gScene.localPlayer + 1);
    Sjis_Widen(buf, txt);
    Font_SetPos(0x26, 0x38);
    Text_Printf("~C00%s", txt);
    win.lineChars = 2;
    x += 0x50;
    win.lines = 2;
    for (i = 0; i < gScene.playerCount; i++) {
        if (i != gScene.localPlayer && gScene.inQuest[i] == 1) {
            win.x = x;
            ListPanel_Draw(&win, 0, 0, -1);
            Plsel_DrawWeaponIcon(x, 0x50, 0x20, i, (s16)gPlayers[gQuestSync.picks[i] & 0x7F].weaponType);
            Font_SetSpacing(0x10, 0x10);
            sprintf(buf, D_00446F60, i + 1);
            Sjis_Widen(buf, txt);
            Font_SetPos(x + 4, 0x3A);
            Text_Printf("~C00%s", txt);
            if (gQuestSync.picks[i] & 0x80) {
                Font_SetPos(x - 8, 0x7C);
                Text_Printf(D_00D795A8, D_00446F64);
            }
            Font_SetSpacing(0x12, 0x12);
            x += 0x36;
        }
    }
}

INCLUDE_RODATA("asm/plsel/nonmatchings/plsel", D_00D795A8);

typedef struct PlselRect {
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
    u32 color;
} PlselRect;

f32 Math_Sin(f32);
struct PrimLine;
s32 Prim_FillRect(struct PrimLine *r);
s32 Text_DisplayLength(char *);
void Text_PrintfAt(s16 x, s16 y, s16 color, const char *fmt, ...);
u8 Weapon_GetClass(u8, u16);
void Plsel_DrawPlayerMenu(void);
extern char *gWeaponTypeNames[];
extern char *D_446F40[];
extern char *gBladeNames[];
extern char *gGunNames[];
extern char *gArmorHeadNames[];
extern char *gArmorBodyNames[];
extern char *gArmorArmsNames[];
extern char *gArmorWaistNames[];
extern char *gArmorLegsNames[];
extern char D_00D79598[];

extern char *D_00446EF8;
extern char *D_00446EFC;
extern char *D_00446F3C;
extern char *D_00446F48;
extern char *D_00446F50;
extern char *D_00446F54;
extern char *D_00446F58;
extern char *D_00446F68;
extern char *D_00446F6C;
extern char*gPlselTexts[];
/* The equipment page (a draw callback; `ot + 0x18` is the select task):
 * the equipment set's name and number, the weapon and the armour pieces,
 * the set's items, then the task state's prompt. */
void Plsel_DrawEquipPage(u8 *ot) {
    ListPanel win;
    PlselRect r;
    char buf[0x80];
    char txt[0x80];
    s16 x;
    PlselPreset *eq;
    TaskSlot *task;
    s16 sel;
    QuestSync *st;
    s16 *list;
    s16 i;
    s32 y;

    st = &gQuestSync;
    task = *(TaskSlot **)(ot + 0x18);
    if (task->step == 2) {
        Plsel_DrawPlayerMenu();
        Plsel_DrawPlayerList();
        return;
    }
    Math_Sin((f32)(u16)((gFrameTick & 0x3F) << 10) * 0.0000958738f);
    win = gPlselListPanel;
    win.x = 0x148;
    win.y = 0x1E;
    win.lineChars = 0xE;
    win.lines = 7;
    win.flags = 0;
    ListPanel_Draw(&win, 0, " ", -1);
    sel = st->pick;
    eq = Plsel_CurPreset();
    Font_SetSpacing(0x10, 0x10);
    Text_PrintfAt(0x15A, 0x22, 5, D_00D79598, D_00446F58, gWeaponTypeNames[(u8)Weapon_GetClass(eq->weaponType, eq->weaponId)]);
    Text_PrintfAt(0x236, 0x22, 5, "%d/%d", (u8)Plsel_ViewedSlotNo(st->presetSet, sel), (u8)Plsel_SlotCount(st->presetSet));
    Plsel_DrawArrow(0, 0x13A, 0x1A);
    Plsel_DrawArrow(1, 0x256, 0x1A);
    Font_SetSpacing(0x12, 0x12);
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    Plsel_DrawEquipIcon(0x152, 0x3C, 0, Weapon_GetClass(eq->weaponType, eq->weaponId), 0);
    if (*(u16 *)eq == 6) {
        Text_PrintfAt(0x16A, 0x3C, 0, TXT(0x00D795C0) /* a full-width colon, then the equipment's name */, gBladeNames[eq->weaponId]);
    } else {
        Text_PrintfAt(0x16A, 0x3C, 0, TXT(0x00D795C0) /* a full-width colon, then the equipment's name */, gGunNames[eq->weaponId]);
    }
    for (i = 0, y = 0x54; i < 5; i++, y += 0x18) {
        Plsel_DrawEquipIcon(0x152, y, 1, i, 0);
    }
    Text_PrintfAt(0x16A, 0x54, 0, TXT(0x00D795C0) /* a full-width colon, then the equipment's name */, gArmorHeadNames[eq->head]);
    Text_PrintfAt(0x16A, 0x6C, 0, TXT(0x00D795C0) /* a full-width colon, then the equipment's name */, gArmorBodyNames[eq->body]);
    Text_PrintfAt(0x16A, 0x84, 0, TXT(0x00D795C0) /* a full-width colon, then the equipment's name */, gArmorArmsNames[eq->arms]);
    Text_PrintfAt(0x16A, 0x9C, 0, TXT(0x00D795C0) /* a full-width colon, then the equipment's name */, gArmorWaistNames[eq->waist]);
    Text_PrintfAt(0x16A, 0xB4, 0, TXT(0x00D795C0) /* a full-width colon, then the equipment's name */, gArmorLegsNames[eq->legs]);
    win.y = 0xDC;
    win.lines = 4;
    ListPanel_Draw(&win, 0, 0, -1);
    r.x0 = win.x;
    r.y0 = win.y + win.charH * 3 + 4;
    r.x1 = r.x0 + win.charW * win.lineChars;
    r.y1 = r.y0 + 1;
    r.color = win.color;
    Prim_FillRect((struct PrimLine *)&r);
    r.y0 = win.y + win.charH * 3 + 6;
    r.x1 = r.x0 + win.charW * win.lineChars;
    r.y1 = r.y0 + 0x15;
    r.color = (win.color & 0xFFFFFF) | 0x60000000;
    Prim_FillRect((struct PrimLine *)&r);
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    list = gStartItemLists[eq->startItems + gQuestSync.presetSet * 4];
    for (i = 0; *list != -1; i++) {
        list++;
        Plsel_DrawItemIcon(*list++, i % 10 * 28 + 0x149, i / 10 * 28 + 0xE2);
    }
    Plsel_DrawPlayerList();
    switch (task->step) {
    case 0:
    default:
    case 1:
        Font_SetPos(0x14C, 0x11F);
        Font_SetSpacing(0x12, 0x12);
        if (gScene.playerCount < 2) {
            Text_Printf(D_00D795A8, D_00446EF8);
        } else {
            Text_Printf(D_00D795A8, D_00446EFC);
        }
        Plsel_DrawEquipPanel();
        eq = Plsel_CurPreset();
        Font_SetPos(0x146, 0x150);
        sprintf(buf, TXT(0x00D795C8) /* the rate, then a full-width percent sign */, eq->pointRatePct);
        Sjis_Widen(buf, txt);
        Text_Printf(D_00446F54, txt);
        break;
    case 3:
    case 4:
        Font_SetPos(0x152, 0x11F);
        Font_SetSpacing(0x12, 0x12);
        Text_Printf(D_00D795A8, D_00446F50);
        Plsel_DrawEquipPanel();
        if (gScene.playerCount < 2) {
            x = (0x118 - Text_DisplayLength(D_00446F48) * 10) / 2.0f;
            Font_SetPos(x + 0x146, 0x158);
            Text_Printf(D_00D795A8, D_00446F48);
        } else {
            x = (0x118 - Text_DisplayLength(D_00446F3C) * 10) / 2.0f;
            Font_SetPos(x + 0x146, 0x158);
            Text_Printf(D_00D795A8, D_00446F3C);
        }
        x = (0x118 - Text_DisplayLength(D_446F40[0]) * 10) / 2.0f;
        Font_SetPos(x + 0x146, 0x16C);
        Text_Printf(D_00D795A8, D_446F40[task->flag & 1]);
        break;
    case 5:
        Font_SetPos(0x152, 0x11F);
        Font_SetSpacing(0x12, 0x12);
        Text_Printf(D_00D795A8, D_00446F50);
        Plsel_DrawEquipPanel();
        if (gScene.playerCount > 1 && !(gFrameTick & 0x10)) {
            x = (0x118 - Text_DisplayLength(D_00446F68) * 10) / 2.0f;
            Font_SetPos(x + 0x146, 0x158);
            Text_Printf(D_00D795A8, D_00446F68);
            x = (0x118 - Text_DisplayLength(D_00446F6C) * 10) / 2.0f;
            Font_SetPos(x + 0x146, 0x16C);
            Text_Printf(D_00D795A8, D_00446F6C);
        }
        break;
    }
    if (Game_IsOnline() != 0 && task->timeLeft != 0) {
        Text_SetColor(0);
        if (task->step == 2) {
            Font_SetPos(0x1CC, 0x1C);
        } else {
            Font_SetPos(0x1CC, 0x196);
        }
        Text_Printf(gPlselTexts[112], task->timeLeft / 30);
    }
}

extern u8 *D_006A39E8;
extern void *D_006A39E4;
void Plsel_DrawChooseScreen(TaskSlot *task) {
    QuestSync *st;
    s32 i;
    Player *pl;

    st = &gQuestSync;
    Plsel_DrawFrame(0);
    Plsel_DrawHeader(3);
    D_006A39E8 = (u8 *) task;
    D_006A39E4 = Plsel_DrawEquipPage;
    OT_AddAt(gDrawOtSorted, gSelectDrawHook, 0, 0x40);
    for (i = 0, pl = gPlayers; i < 4; i++, pl++) {
        Player_UpdateMotionStep(pl);
        pl->pos[1] = -40.0f;
        pl->rot[1] = st->yaw;
        if (i == st->pick) {
            OT_AddByDepth(gDrawOt32, pl->drawSlot, 0x20, 0);
        }
    }
    if (Game_IsOnline() == 1 && task->timeLeft != 0) {
        Text_SetColor(0);
        if (task->step == 2) {
            Font_SetPos(0x1CC, 0x1C);
        } else {
            Font_SetPos(0x1CC, 0x196);
        }
        Text_Printf(gPlselTexts[112], task->timeLeft / 30);
    }
}

/* Select state 4: choosing the slot. Left and right cycle the four slots
 * (their arrows bump), button 0x8000 opens Plsel_OpenPlayerMenu's screen
 * (sub-state 2), circle asks for confirmation (sub-state 3), and when the
 * online countdown runs out the current slot is taken. The pad is read as gPadState and the counter at
 * gQuestSync + 0xD4 through st: as named objects, MWCC schedules the pad
 * load before that store, as the game does. */
void Plsel_StateChoose(TaskSlot *task) {
    QuestSync *st;
    u16 pad;
    s8 chat;
    PlselPreset *e;

    chat = Kbd_IsOpen();
    pad = gPadState[0].pressed | gPadState[0].repeat;
    st = &gQuestSync;
    st->yaw += 0x100;
    if ((gPadState[0].dir & 8) && chat == 0) {
        st->yaw -= 0x500;
    }
    if ((gPadState[0].dir & 4) && chat == 0) {
        st->yaw += 0x300;
    }
    if (st->timer > 0) {
        st->timer--;
    }
    if (Game_IsOnline() == 1 && --task->timeLeft == 0) {
        gScene.menuOpen = 0;
        goto start;
    }
    switch (task->step) {
    case 0:
        if (st->timer != 0) {
            break;
        }
        if (chat == 0 && (gPadState[0].pressed & 0x8000)) {
            Plsel_OpenPlayerMenu(st->pick);
            task->step = 2;
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
        if (pad & 0x400) {
            gQuestSync.bumpRight = 5;
            if (st->pick >= 3) {
                st->pick = 0;
            } else {
                st->pick++;
            }
            Plsel_PlayCursorSe();
            func_901790(0xD, gScene.localPlayer);
        } else if (pad & 0x800) {
            gQuestSync.bumpLeft = 5;
            if (st->pick == 0) {
                st->pick = 3;
            } else {
                st->pick--;
            }
            Plsel_PlayCursorSe();
            func_901790(0xD, gScene.localPlayer);
        } else if (gPadState[0].pressed & 0x20) {
            Plsel_PlayDecideSe();
            task->step = 3;
            task->flag = 1;
        }
        break;
    case 1:
        if (SeqA_GetState() != 1) {
            task->mode = 2;
            task->step = 0;
            Plsel_SetCamera();
            SeqA_Start(2);
            st->timer = 0xF;
            return;
        }
        break;
    case 2:
        Plsel_PlayerMenuUpdate(pad);
        if (gScene.menuOpen == 0) {
            task->step = 0;
        }
        break;
    case 3:
        if (gPadState[0].pressed & 0x20) {
            if (task->flag == 0) {
            start:
                e = Plsel_CurPreset();
                func_901790(0xC, gScene.localPlayer);
                st->picks[gScene.localPlayer] = st->pick | 0x80;
                st->pointRatePct = e->pointRatePct;
                Plsel_PlayChooseSe();
                task->mode++;
                task->step = 5;
                task->flag = 0;
                task->count = 0;
            } else {
                task->step = 0;
                Sound_PlayCancel();
            }
        } else if (gPadState[0].pressed & 0x40) {
            Sound_PlayCancel();
            if (task->flag == 1) {
                task->step = 0;
            } else {
                task->flag = 1;
            }
        } else if ((gPadState[0].pressed & 0x800) && task->flag == 1) {
            Plsel_PlayCursorSe();
            task->flag = 0;
        } else if ((gPadState[0].pressed & 0x400) && task->flag == 0) {
            Plsel_PlayCursorSe();
            task->flag = 1;
        }
        break;
    }
    Plsel_DrawChooseScreen(task);
}

void Plsel_StateReady(TaskSlot *task) {
    QuestSync *st;
    s32 i;

    st = &gQuestSync;
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
            if (gScene.inQuest[i] == 1 && !(st->picks[i] & 0x80)) {
                break;
            }
        }
        if (i >= gScene.playerCount) {
            if ((s8)Kbd_IsOpen() != 0) {
                Kbd_Cancel();
            }
            Stream_FadeOut(0, 10);
            SeqA_Start(5);
            task->flag++;
        } else {
            task->count = (task->count + 1) & 0x3F;
            if (task->count == 0) {
                func_901790(0xC, gScene.localPlayer);
            }
        }
        break;
    case 2:
        if (SeqA_GetState() != 1) {
            task->count = 0x3C;
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

void Plsel_ApplyPresetEquip(Player *p, s32 idx) {
    s16 sel;
    u8 v;
    s16 k;
    PlselPreset *t;

    sel = gQuestSync.presetSet;
    v = gQuestSync.picks[idx];
    if (v & 0x80) {
        k = v & 0x7F;
        t = gPlselPresets[sel];
        t += k;
        p->equip[6].used = 1;
        p->equip[6].type = t->weaponType;
        p->equip[6].level = t->weaponLevel;
        p->equip[6].deco[0] = 0;
        p->equip[6].deco[1] = 0;
        p->equip[6].deco[2] = 0;
        p->equip[0].used = 1;
        p->equip[0].type = 0;
        p->equip[0].level = 0;
        p->equip[0].deco[0] = 0;
        p->equip[0].deco[1] = 0;
        p->equip[0].deco[2] = 0;
        p->equip[1].used = 1;
        p->equip[1].type = 1;
        p->equip[1].level = 0;
        p->equip[1].deco[0] = 0;
        p->equip[1].deco[1] = 0;
        p->equip[1].deco[2] = 0;
        p->equip[2].used = 1;
        p->equip[2].type = 2;
        p->equip[2].level = 0;
        p->equip[2].deco[0] = 0;
        p->equip[2].deco[1] = 0;
        p->equip[2].deco[2] = 0;
        p->equip[3].used = 1;
        p->equip[3].type = 3;
        p->equip[3].level = 0;
        p->equip[3].deco[0] = 0;
        p->equip[3].deco[1] = 0;
        p->equip[3].deco[2] = 0;
        p->equip[4].used = 1;
        p->equip[4].type = 4;
        p->equip[4].level = 0;
        p->equip[4].deco[0] = 0;
        p->equip[4].deco[1] = 0;
        p->equip[4].deco[2] = 0;
        p->equip[5].used = 1;
        p->equip[5].type = 5;
        p->equip[5].level = 0;
        p->equip[5].deco[0] = 0;
        p->equip[5].deco[1] = 0;
        p->equip[5].deco[2] = 0;
        p->equip[6].id = t->weaponId;
        p->equip[0].id = t->legs;
        p->equip[2].id = t->head;
        p->equip[3].id = t->body;
        p->equip[4].id = t->arms;
        p->equip[5].id = t->waist;
        p->startItemList = t->startItems + gQuestSync.presetSet * 4 + 1;
        p->presetEquip = 1;
        p->weaponKind = Equip_GetKind(&p->equip[6]);
    }
}

void Plsel_SetupPresetPouch(Player *p, s32 n) {
    s16 i;
    s16 *t;
    s16 v;
    u16 k;

    for (i = 0; i < 20; i++) {
        p->pouch[i].item = 0;
        p->pouch[i].count = 0;
    }
    p->startItemList = (u8)n + gQuestSync.presetSet * 4;
    t = gStartItemLists[p->startItemList];
    v = t[0];
    while (v != -1) {
        k = t[1];
        t += 2;
        Pouch_SetAtLeast(p, 0, k, v);
        v = t[0];
    }
}

void Plsel_ApplyStageEquip(Player *p) {
    PlselPreset *t;

    t = D_896D90[gQuest.stage];
    p->equip[6].used = 1;
    p->equip[6].type = t->weaponType;
    p->equip[6].level = t->weaponLevel;
    p->equip[6].deco[0] = 0;
    p->equip[6].deco[1] = 0;
    p->equip[6].deco[2] = 0;
    p->equip[0].used = 1;
    p->equip[0].type = 0;
    p->equip[0].level = 0;
    p->equip[0].deco[0] = 0;
    p->equip[0].deco[1] = 0;
    p->equip[0].deco[2] = 0;
    p->equip[1].used = 1;
    p->equip[1].type = 1;
    p->equip[1].level = 0;
    p->equip[1].deco[0] = 0;
    p->equip[1].deco[1] = 0;
    p->equip[1].deco[2] = 0;
    p->equip[2].used = 1;
    p->equip[2].type = 2;
    p->equip[2].level = 0;
    p->equip[2].deco[0] = 0;
    p->equip[2].deco[1] = 0;
    p->equip[2].deco[2] = 0;
    p->equip[3].used = 1;
    p->equip[3].type = 3;
    p->equip[3].level = 0;
    p->equip[3].deco[0] = 0;
    p->equip[3].deco[1] = 0;
    p->equip[3].deco[2] = 0;
    p->equip[4].used = 1;
    p->equip[4].type = 4;
    p->equip[4].level = 0;
    p->equip[4].deco[0] = 0;
    p->equip[4].deco[1] = 0;
    p->equip[4].deco[2] = 0;
    p->equip[5].used = 1;
    p->equip[5].type = 5;
    p->equip[5].level = 0;
    p->equip[5].deco[0] = 0;
    p->equip[5].deco[1] = 0;
    p->equip[5].deco[2] = 0;
    p->equip[6].id = t->weaponId;
    p->equip[0].id = t->legs;
    p->equip[2].id = t->head;
    p->equip[3].id = t->body;
    p->equip[4].id = t->arms;
    p->equip[5].id = t->waist;
    p->presetEquip = 1;
    p->weaponKind = Equip_GetKind(&p->equip[6]);
    p->startItemList = gQuest.stage + 1;
}

void Plsel_PlayDecideSe(void) {
    Sound_PlaySeFlat(0, 0x13, 0);
}

void Plsel_PlayCursorSe(void) {
    Sound_PlaySeFlat(0, 0x12, 0);
}

void Plsel_PlayChooseSe(void) {
    Sound_PlaySeFlat(1, 4, 0);
}

s32 Plsel_ChatOpenRequested(void) {
    u16 pad;

    pad = gPadState[0].pressed | (gPadState[0].repeat & 0x3C00);
    if (Kbd_ActHotkeys() == 1) {
        return 2;
    }
    if (pad & 0x100) {
        return 1;
    }
    return 0;
}

void Plsel_GetPad(u16 *a, u16 *b) {
    u16 v;

    *a = gPadState[0].held;
    *b = gPadState[0].pressed;
    v = gPadState[0].dir;
    if (v & 0x20) {
        *a |= 0x2000;
    } else if (v & 0x10) {
        *a |= 0x1000;
    } else if (v & 8) {
        *a |= 0x800;
    } else if (v & 4) {
        *a |= 0x400;
    }
    v = gPadState[0].dirPressed;
    if (v & 0x20) {
        *b |= 0x2000;
    } else if (v & 0x10) {
        *b |= 0x1000;
    } else if (v & 8) {
        *b |= 0x800;
    } else if (v & 4) {
        *b |= 0x400;
    }
}

void Plsel_InitSelWork(void) {
    Player *p;
    s32 i;

    p = gPlayers;
    memset(gPlselSelWork, 0, 0x13DC);
    for (i = 0; i < gScene.playerCount; i++, p++) {
        strcpy((char *)p + 0x82C, gMemberNames[i]);
    }
}

void Plsel_ChatOpenKbd(s32 mode) {
    PlselSelWork *w;
    s32 flags;

    w = gPlselSelWork;
    if (mode != 0) {
        w->kbdOpen++;
        flags = 4;
        if (mode == 2) {
            flags |= 0x40000000;
        }
        Kbd_SetPos(0x6E, 80.0f);
        Chat_OpenKbd(flags | 0x80000000, 0xE, 0x2C, 0);
        Kbd_SetSubmitWait(w->sendWait);
        Sound_PlaySeFlat(0, 0x11, 0);
        Plsel_ChatInput(0);
    }
}

s32 Plsel_ChatTargetPlayer(u8 *unused) {
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
        } else if (gScene.inQuest[i] != 1) {
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

void Plsel_ChatInput(u16 unused) {
    s16 a;
    s16 b;
    u8 buf[0x4C];
    u8 mask;
    Player *pl;
    PlselSelWork *w;
    s8 r;
    s8 k;

    w = gPlselSelWork;
    Plsel_GetPad((u16 *)&a, (u16 *)&b);
    buf[0] = 0;
    r = Plsel_ChatRunKbd(buf, a, b);
    if (r != 0) {
        if ((s8)buf[0] != 0 && r > 0) {
            if (Chat_FilterIfNotBlank(buf) != 0) {
                k = Plsel_ChatTargetPlayer(buf);
                if ((s8)buf[0] != 0) {
                    pl = &gPlayers[gScene.localPlayer];
                    if (k < 0) {
                        mask = 0xFF;
                    } else {
                        mask = 1 << k;
                    }
                    Plsel_AddChatLog(pl->index, (char *)buf, 0, mask);
                    func_901E80(pl->index, 1, buf, mask);
                    w->newMsg = 1;
                    w->newMsgMine = 1;
                    w->logShowTimer = 0x12C;
                }
            }
            w->sendWait = 0x3C;
        }
        Kbd_Cancel();
        w->kbdOpen = 0;
    } else {
        w->newMsg = w->logShowTimer = 0;
    }
}

void Plsel_TickTimers(void) {
    PlselSelWork *w;

    w = gPlselSelWork;
    if (w->sendWait != 0) {
        w->sendWait--;
    }
    if (w->logShowTimer != 0) {
        w->logShowTimer--;
    }
}

s32 Plsel_ChatUpdate(void) {
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
        Plsel_ChatInput(gPadState[0].pressed | (gPadState[0].repeat & 0x3C00));
        return 1;
    }
    return 0;
}

u8 Plsel_ChatRunKbd(u8 *dst, s32 held, s16 pressed) {
    if (pressed & 0x4000) {
        *dst = 0;
        return 0xFF;
    }
    return Kbd_Run((char *) dst, held, pressed);
}

void Plsel_AddChatLog(u8 who, char *msg, u8 *pl, u8 flag) {
    PlselSelWork *w;
    PlselChatEntry *e;
    s32 i;
    s32 n;
    u8 *p;
    u8 *line;
    s32 k;
    u8 c;

    w = gPlselSelWork;
    if (*msg == 0) {
        return;
    }
    e = &w->log[w->logHead];
    e->who = who;
    if (pl != NULL) {
        e->color[0] = pl[0x11C];
        e->color[1] = pl[0x11D];
        e->color[2] = pl[0x11E];
        e->color[3] = pl[0x11F];
        strcpy(e->id, (char *)pl);
        strcpy(e->name, (char *)pl + 8);
    } else {
        if (flag == 0xFF) {
            flag = 3;
        } else {
            flag = 1;
        }
        e->color[0] = 0;
        e->color[1] = e->color[2] = ((u8 *)gPlselSlotColors)[e->who];
        e->color[3] = gChatClassColors[flag];
        strcpy(e->id, gMyUserId);
        strcpy(e->name, gMyName);
    }
    w->logHead++;
    w->logHead &= 0x3F;
    w->logCount++;
    if (w->logCount > 0x40) {
        w->logCount = 0x40;
    }
    e->lineCount = 0;
    for (i = 0, line = (u8 *)e->lines; i < 2; i++, line += 0x18) {
        p = line;
        if (*msg == 0) {
            line[0] = 0;
            return;
        }
        line[0x16] = 0;
        n = 0x16;
        do {
            if (*msg == 0) {
                *p = 0;
                e->lineCount++;
                return;
            }
            c = *msg;
            if ((c >= 0x80 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFF)) {
                p[0] = c;
                p[1] = msg[1];
                n -= 2;
                msg += 2;
                p += 2;
            } else {
                *p = c;
                msg++;
                p++;
                n--;
            }
        } while (n > 0);
        *p = 0;
        e->lineCount++;
    }
}

void Plsel_OnChatMessage(s32 text, u8 n, s32 color) {
    s16 pos[2];
    Panel w;
    s32 q;

    q = n * 18 / 21;
    pos[0] = 0;
    pos[1] = 0x1A4 - (q + 1) * 12;
    w = gPlselChatWin;
    w.charW = n;
    w.charH = q - 1;
    w.flags = ((color + 2) & 7) | 0x8000;
    Window_DrawText(&w, (s16 *)pos, (u8 *)text);
}

extern char *gChatTypeTexts;
void Plsel_FormatPlayerTag(char *buf, PlselChatEntry *p) {
    char tag[0x40];
    char wide[0x40];

    sprintf(buf, gChatTypeTexts, p->name);
    sprintf(tag, "%dP", p->who + 1);
    Sjis_Widen(tag, wide);
    strcat(buf, wide);
}

s32 Plsel_ChatLogLineCount(void) {
    PlselSelWork *w;
    s32 idx;
    s32 n;
    s32 sum;
    u8 *r;

    w = gPlselSelWork;
    sum = 0;
    n = w->logCount;
    idx = w->logHead - 1;
    for (; n > 0; n--, idx--) {
        r = (u8 *)w + (idx & 0x3F) * 0x4F;
        sum += r[0x3B];
        if ((s8)r[0x41] != 0) {
            sum++;
        }
    }
    return sum;
}

void Text_Draw(char *);
void Text_DrawShadow2(s16 x, s16 y, s32 c1, s32 c2, char *s);

/* Draws the chat log: newest first from the bottom when it is full (12
 * lines or more) and `all` is 0, else from the oldest down. Both line loops
 * index p = e->lines[i]; the upward branch has its own y offset (oy2) and
 * counts lines in the dead parameter all, the downward one keeps the message
 * count in all and counts lines in left. */
void Plsel_DrawChatLog(u8 skip, s32 y, s32 all, u8 size) {
    PlselSelWork *w;
    s32 idx;
    s32 left;
    s32 oy;
    s16 yo;
    s16 lim;
    s16 small;
    s16 lh;
    s16 fsz;
    PlselChatEntry *e;
    s16 ly;
    u8 *p;
    s32 oy2;
    char buf[0x20];

    w = gPlselSelWork;
    if (w->logCount == 0) {
        return;
    }
    small = (s16)size * 20 / 21;
    fsz = (s16)size * 18 / 21;
    Font_SetSpacing(size, fsz);
    lh = fsz + 1;
    yo = (s16)y * (u8)size / 21;
    if ((u32)Plsel_ChatLogLineCount() > 11 && (u8)all == 0) {
        ly = 0x1A4 - lh;
        lim = 0x1A4 - lh * 11;
        idx = w->logHead - 1 - skip;
        left = w->logCount - skip;
        if (left == 0) {
            return;
        }
        oy2 = yo;
        do {
            idx &= 0x3F;
            e = &w->log[idx];
            Text_SetColor(e->color[3]);
            for (all = e->lineCount - 1; all >= 0; all--) {
                p = (u8 *)e->lines[all];
                Font_SetPos(0x18, ly + oy2);
                if ((s8)p[0x16] != 0) {
                    Font_SetSpacing(small, fsz);
                    Text_Draw((char *)p);
                    Font_SetSpacing(size, fsz);
                } else {
                    Text_Draw((char *)p);
                }
                ly -= lh;
                if (ly < lim) {
                    return;
                }
            }
            if ((s8)e->id[0] != 0) {
                Plsel_FormatPlayerTag(buf, e);
                Text_DrawShadow2(0x18, (s16)(ly + oy2), 1, e->color[2], buf);
                ly -= lh;
                if (ly < lim) {
                    return;
                }
            }
            left--;
            idx--;
        } while (left != 0);
    } else {
        all = w->logCount;
        idx = w->logHead - all;
        ly = 0x1A4 - lh * 11;
        if (all == 0) {
            return;
        }
        oy = yo;
        do {
            idx &= 0x3F;
            e = &w->log[idx];
            if ((s8)e->id[0] != 0) {
                Plsel_FormatPlayerTag(buf, e);
                Text_DrawShadow2(0x18, (s16)(ly + oy), 1, e->color[2], buf);
                ly += lh;
                if (ly >= 0x1A4) {
                    return;
                }
            }
            Text_SetColor(e->color[3]);
            for (left = 0; left < e->lineCount; left++) {
                p = (u8 *)e->lines[left];
                Font_SetPos(0x18, ly + oy);
                if ((s8)p[0x16] != 0) {
                    Font_SetSpacing(small, fsz);
                    Text_Draw((char *)p);
                    Font_SetSpacing(size, fsz);
                } else {
                    Text_Draw((char *)p);
                }
                ly += lh;
                if (ly >= 0x1A4) {
                    return;
                }
            }
            all--;
            idx++;
        } while (all != 0);
    }
}

/* Adds player `slot`'s chat message to the log (filtered, with the
 * player's name and colours), counts it and plays the chat sound. The
 * empty check is what gives the game's schedule of the colour stores:
 * only a compound condition there does (a debug message compiled out,
 * most likely). */
void Plsel_LogChatFrom(u8 slot, u8 flag, char *text) {
    struct {
        char id[8];
        char name[0x114];
        u8 b0;
        u8 b1;
        u8 b2;
        u8 b3;
        u8 pad[4];
    } rec;
    PlselSelWork *w;
    u8 k;

    w = gPlselSelWork;
    Chat_FilterWords(text);
    k = (flag == 0xFF) ? 3 : 1;
    rec.b0 = 0;
    rec.b1 = rec.b2 = ((u8 *)gPlselSlotColors)[slot];
    if (slot >= 10 || flag > 1) { /* compiled-out check */
    }
    rec.b3 = gChatClassColors[k];
    strcpy(rec.name, gMemberNames[slot]);
    strcpy(rec.id, gPlayerIds[slot]);
    Plsel_AddChatLog(slot, (char *)text, (u8 *)rec.id, 0);
    if ((u32)Plsel_ChatLogLineCount() > 11) {
        w->logScroll++;
        if (w->logScroll > 0x3F) {
            w->logScroll = 0x3F;
        }
    }
    w->newMsg = 1;
    w->newMsgMine = 0;
    w->logShowTimer = 0x12C;
    Sound_PlaySeFlat(0, 0x18, 0);
}

u8 Plsel_SlotColor(s32 slot) {
    if (slot > 3) {
        slot = 4;
    }
    return ((u8 *)gPlselSlotColors)[slot];
}
