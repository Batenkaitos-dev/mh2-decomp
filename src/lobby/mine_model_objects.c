/*
 * The village mine's pickaxe list and the lobby's model objects.
 * - The mine (TOWN_MODE_MINE, run by Mine_UpdateMenu): the input and draw
 *   of the list of pickaxes held (MineMenu in gMenuWork), and Mine_Dig,
 *   which takes the pickaxe given and rolls the save's MineState: the
 *   digs it lasts and the find, from this file's tables of finds.
 * - The model objects (stage objects of kind LOBBY_OBJ_MODEL): short
 *   effects of up to 8 parts (ModelObjPart, in the object's buffer), each
 *   a draw slot that ModelObj_DrawPart draws. They are spawned on a
 *   townsperson, the player or a point (enum ModelObjType) and animated
 *   from key curves (gModelObjKeys) and per-type tables.
 */
#include "common.h"
#include "sprite.h"
#include "save.h"
#include "em.h"
#include "lobby_vobj.h"
#include "item.h"
#include "pad.h"
#include "lobby_menu.h"
#include "objhead.h"
#include "render_state.h"
#include "lobby_labels.h"
#include "bgm.h"
#include "sprite_queue.h"
#include "text.h"
#include "village_cond.h"
#include "mine_model_objects.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "font.h"
#include "item_shop.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "offer_menu.h"
#include "pets.h"
#include "rand.h"
#include "ship_mine_menus.h"
#pragma optimization_level 4

extern u16 gMineFindsWeak[];
extern u16 gMineFindsMid[];
extern u16 gMineFindsStrong[];
extern u16 gMineFindsWeakOpen[];
extern u16 gMineFindsMidOpen[];
extern u16 gMineFindsStrongOpen[];
extern u16 gMineFindsStrongEvent[];
extern s16 gMineBonusByLevel[];
u8 Village_FacilityLevel(s32 k);

/* Takes the pickaxe chosen and starts the miner's dig: rolls the digs it
 * lasts (1-2, 1-4 or 2-6, by pickaxe) and one of five finds from that
 * pickaxe's table (the richer tables while Mine_IsOpen, and the strong
 * pickaxe's own once event flag 0x89 is set), and sets the mine's bonus
 * from the mine's level when none is left. */
void Mine_Dig(MineMenu *menu)
{
    u16 *finds;
    u16 item = menu->items[menu->cursor];
    s16 bonus;
    s32 foundItem;

    Items_Take(item, 1);
    switch (item) {
    case ITEM_ID_MINE_TOOL_WEAK:
        gSave.mine.digs = (Rand_Next(1) & 1) + 1;
        if (Mine_IsOpen() == 1) {
            finds = gMineFindsWeakOpen;
        } else {
            finds = gMineFindsWeak;
        }
        break;
    case ITEM_ID_MINE_TOOL_MID:
        gSave.mine.digs = Rand_Next(1) % 4 + 1;
        if (Mine_IsOpen() == 1) {
            finds = gMineFindsMidOpen;
        } else {
            finds = gMineFindsMid;
        }
        break;
    case ITEM_ID_MINE_TOOL_STRONG:
        gSave.mine.digs = Rand_Next(1) % 5 + 2;
        if (Mine_IsOpen() == 1) {
            finds = gMineFindsStrongOpen;
        } else if (Save_TestEventFlag(137) == 0) {
            finds = gMineFindsStrong;
        } else {
            finds = gMineFindsStrongEvent;
        }
        break;
    }
    gSave.mine.active = MINE_DIGGING;
    foundItem = finds[Rand_Next(1) % 5];
    bonus = gSave.mine.bonus;
    gSave.mine.find = foundItem;
    if (bonus == 0) {
        gSave.mine.bonus = gMineBonusByLevel[(u8)Village_FacilityLevel(VILLAGE_FACILITY_MINE)];
    }
}

/* The pickaxe list's input: PAD_CIRCLE gives the pickaxe under the
 * cursor (when one is held), PAD_CROSS leaves, PAD_SQUARE shows or hides
 * its description, up and down move the cursor. Returns MENU_DONE,
 * MENU_CANCEL or MENU_RUNNING. */
s32 Mine_PickaxeInput(MineMenu *menu, u16 pad)
{
    if (pad & PAD_CIRCLE) {
        if (menu->count != 0) {
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_DONE;
        }
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    } else if (pad & PAD_SQUARE) {
        if (menu->count != 0) {
            menu->toggle ^= 1;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
    } else {
        menu->cursor = Menu_WrapUpDown(menu->cursor, menu->count);
    }
    return MENU_RUNNING;
}

/* unit's own declaration: returning an int, as this file's code uses the result; with the definition's s16 return, Mine_DrawPickaxes compiles differently */
s32 Text_CenterX(char *s, s16 x, s32 size);
/* unit's own declaration: with s16 coordinates, as this file's code passes them; with the definition's s32 ones, Mine_DrawPickaxes compiles differently */
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k);

/* Draws the pickaxe window at (x, y): its title, a row per pickaxe held
 * with its count and the cursor's bar, the description of the one under
 * the cursor, and the help text; only a centred line when none is held. */
void Mine_DrawPickaxes(MineMenu *menu, s16 x, s16 y)
{
    s16 rowY = y;
    s16 i;
    u16 item;
    u16 cursorItem;

    Win_Draw(x, y, 340, 288, 0xBDB59F6C, 0xBB402010, 8);
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(280, 18, 200, 0xFFB0B000, gListTitles[LIST_TITLE_PICKAXE], 2, 1);
    if (menu->count == 0) {
        Font_SetSpacing(18, 18);
        Font_SetColor(0x802B7F7F);
        Font_SetPos(Text_CenterX(gTownLabels[TOWN_LABEL_NO_PICKAXE], x + 170, 18), rowY + 90);
        Text_PrintfColored(gTownLabels[TOWN_LABEL_NO_PICKAXE]);
        return;
    }
    Font_SetColor(0x807F7F7F);
    rowY += 30;
    Font_SetSpacing(20, 20);
    for (i = 0; i < 7; i++) {
        if (i <= menu->count) {
            item = menu->items[i];
            if (i == menu->cursor) {
                cursorItem = item;
                Menu_DrawRowHighlight(x + 10, rowY - 1, 320, 24, 0);
            }
            Menu_DrawItemCountRow(x + 60, rowY, item, 0x807F7F7F);
        }
        rowY += 22;
    }
    Gfx_DrawLine(x + 20, rowY + 2, 300, 2, 0xBDB59F6C);
    ItemInfo_Draw((s16)(x + 60) - 50, rowY + 4, 340, menu->toggle, cursorItem, 0, -1, 0x807F7F7F);
    Win_DrawText(280, 356, 340, 80, 0xBDB59F6C, 0xC0101010, gTownLabels[TOWN_LABEL_PICKAXE_HELP]);
}

extern u8 gMenuWork[];
/* The mine menu's draw: the pickaxe window, at its slide position, while
 * it slides in and while a pickaxe is chosen. */
void Mine_DrawMenu(MenuTask *task)
{
    MineMenu *menu = (MineMenu *)gMenuWork;

    Text_SetSize(task->textSize);
    switch (menu->state) {
    case MINE_MENU_SLIDE_IN:
    case MINE_MENU_CHOOSE:
        Mine_DrawPickaxes(menu, menu->win.pos, 58);
        break;
    }
}

/* Spawns a model object of the given type on owner (a townsperson or a
 * player) at scale 1. */
void ModelObj_SpawnOn(void *owner, s32 type)
{
    ModelObj_SpawnOnScaled((void *)owner, type, 1.0f);
}

/* Spawns a model object of the given type at pos, with no owner;
 * colorIndex picks MODEL_OBJ_SPARKLE's colour (gModelObjPartColors). */
void ModelObj_SpawnAt(f32 *pos, s32 type, s32 colorIndex, f32 scale)
{
    StageObj *obj = StageObj_Alloc(1);

    if (obj != NULL) {
        obj->kind = LOBBY_OBJ_MODEL;
        obj->update = ModelObj_Update;
        obj->draw = 0;
        obj->owner = NULL;
        obj->type = type;
        obj->var2 = colorIndex;
        obj->node = NULL;
        obj->scale = scale;
        Vec_Copy((f32 *) &obj->pos, pos);
    }
}

/* Spawns a model object of the given type on owner at a scale, facing
 * the owner's way; ModelObj_Init places it at one of the owner's bones. */
void ModelObj_SpawnOnScaled(u8 *owner, s32 type, f32 scale)
{
    StageObj *obj = StageObj_Alloc(1);

    if (obj != NULL) {
        obj->kind = LOBBY_OBJ_MODEL;
        obj->update = ModelObj_Update;
        obj->draw = 0;
        obj->owner = owner;
        obj->type = type;
        obj->angle = ((ObjHead *)owner)->rot[1];
        obj->node = NULL;
        obj->scale = scale;
    }
}

/* A model object's update (StageObj.update): its state's step. */
void ModelObj_Update(StageObj *obj)
{
    u8 state;

    state = obj->state;
    switch (state) {
    case LOBBY_OBJ_INIT:
        ModelObj_Init(obj);
        break;
    case LOBBY_OBJ_RUN:
        ModelObj_Run(obj);
        break;
    case LOBBY_OBJ_END:
        ModelObj_End(obj);
        break;
    case LOBBY_OBJ_FREE:
        ModelObj_Free(obj);
        break;
    }
}

extern s16 gModelObjPartCounts[];
extern s16 gModelObjBurstDelays[];

/* The set-up (LOBBY_OBJ_INIT): places the object at one of its owner's
 * bones by type (with an offset turned by the bone), sets up each of its
 * parts (start frame, angle, spin, scale and drift by type and part), and
 * gives each a draw slot that ModelObj_DrawPart draws (none free: the
 * part isn't drawn). The bones' numbers are the owners' skeletons'. */
void ModelObj_Init(StageObj *obj)
{
    f32 boneMtx[4][4];
    Vec3f offset;
    ModelObjPart *curPart = (ModelObjPart *)obj->buf;
    s16 partCount;
    s16 i;

    obj->state++;
    obj->var0 = 0;
    obj->var1 = 0;
    obj->visible = 1;
    obj->draw = 0;
    obj->timer = 0;
    partCount = gModelObjPartCounts[obj->type];
    switch (obj->type) {
    case MODEL_OBJ_NPC_BURST:
        obj->angle = Rand_Next(1);
    case MODEL_OBJ_NPC_PUFF:
        Mtx_Copy(boneMtx[0], (f32 *)Em_GetBone(obj->owner, 10));
        offset.x = 0.0f;
        offset.y = 50.0f;
        offset.z = 25.0f;
        Vec_RotateSelf(&offset.x, boneMtx[0]);
        obj->pos[0] = boneMtx[3][0] + offset.x;
        obj->pos[1] = boneMtx[3][1] + offset.y;
        obj->pos[2] = boneMtx[3][2] + offset.z;
        break;
    case MODEL_OBJ_SPARKLE:
        if (obj->owner != NULL) {
            Mtx_Copy(boneMtx[0], (f32 *)Em_GetBone(obj->owner, 11));
            offset.x = -26.4f;
            offset.y = 2.8f;
            offset.z = 7.7f;
            Vec_RotateSelf(&offset.x, boneMtx[0]);
            obj->pos[0] = boneMtx[3][0] + offset.x;
            obj->pos[1] = boneMtx[3][1] + offset.y;
            obj->pos[2] = boneMtx[3][2] + offset.z;
        }
        break;
    case MODEL_OBJ_PLAYER_FX_A:
        Mtx_Copy(boneMtx[0], (f32 *)Em_GetBone(obj->owner, 18));
        offset.x = -8.0f;
        offset.y = 0.0f;
        offset.z = 70.0f;
        Vec_RotateSelf(&offset.x, boneMtx[0]);
        obj->pos[0] = boneMtx[3][0] + offset.x;
        obj->pos[1] = boneMtx[3][1] + offset.y;
        obj->pos[2] = boneMtx[3][2] + offset.z;
        break;
    case MODEL_OBJ_PET_JOY:
        Mtx_Copy(boneMtx[0], (f32 *)Em_GetBone(obj->owner, 20));
        offset.x = 0.0f;
        offset.y = 40.0f;
        offset.z = 0.0f;
        Vec_RotateSelf(&offset.x, boneMtx[0]);
        obj->pos[0] = boneMtx[3][0] + offset.x;
        obj->pos[1] = boneMtx[3][1] + offset.y;
        obj->pos[2] = boneMtx[3][2] + offset.z;
        break;
    }
    for (i = 0; i < partCount; curPart++, i++) {
        curPart->frame = 0;
        curPart->scale = obj->scale;
        Vec_Copy((f32 *) &curPart->pos, (f32 *) &obj->pos);
        curPart->index = i;
        switch (obj->type) {
        case MODEL_OBJ_NPC_BURST:
            curPart->frame = gModelObjBurstDelays[i] - 10;
            switch (curPart->index) {
            case 0:
                curPart->angle = 0;
                curPart->spin = 0;
                break;
            case 1:
                curPart->angle = 0;
                curPart->spin = 0;
                curPart->scale *= 3.0f;
                break;
            case 2:
                curPart->angle = Rand_Next(1);
                curPart->spin = (Rand_Next(1) & 0x3ff) - 512;
                break;
            case 3:
                curPart->angle = Rand_Next(1);
                curPart->spin = (Rand_Next(1) & 0xFF) - 128;
                curPart->scale *= 3.0f;
                break;
            case 4:
            case 5:
            case 6:
                curPart->angle = Rand_Next(1);
                curPart->spin = (Rand_Next(1) & 0x3ff) - 512;
                curPart->scale *= 0.5f + 0.0003f * ((Rand_Next(1) & 0x3ff));
                break;
            }
            break;
        case MODEL_OBJ_NPC_PUFF:
            curPart->angle = 0;
            curPart->spin = (Rand_Next(1) & 0x7ff) - 1024;
            curPart->scale *= 0.8f + 0.00040000002f * ((Rand_Next(1) & 0x3ff));
            curPart->driftSeed = Rand_Next(1);
            break;
        case MODEL_OBJ_SPARKLE:
            curPart->angle = Rand_Next(1);
            curPart->spin = (Rand_Next(1) & 0x1ff) - 256;
            curPart->driftSeed = Rand_Next(1);
            break;
        case MODEL_OBJ_PLAYER_FX_A:
            switch (curPart->index) {
            case 3:
                curPart->angle = ANGLE_TURN / 4;
                break;
            case 1:
            default:
                curPart->angle = 0;
                break;
            }
            curPart->spin = 0;
            break;
        case MODEL_OBJ_PLAYER_FX_B:
            curPart->frame = -i * 5 - 10;
            curPart->angle = ANGLE_TURN / 8;
            curPart->spin = 0;
            break;
        case MODEL_OBJ_PET_JOY:
            curPart->frame = 0;
            curPart->angle = 0;
            curPart->spin = 0;
            curPart->size.x = 0.3f;
            curPart->size.y = 0.3f;
            curPart->size.z = 0.3f;
            break;
        }
        curPart->slot = DrawSlot_Alloc();
        if (curPart->slot != -1) {
            curPart->draw = DrawSlot_Get(curPart->slot);
            curPart->draw->owner = obj;
            curPart->draw->part = i;
            curPart->draw->draw = ModelObj_DrawPart;
        } else {
            curPart->draw = NULL;
        }
    }
}

extern f32 gCamMatrix[];
extern u16 gModelObjLifetimes[];
extern s16 gModelObjFirstKey[];
extern s16 gModelObjKeysPerPart[];
extern u16 gModelObjPartFrames[];
extern void *gModelObjKeys[];

#pragma optimization_level 3
/*
 * The run (LOBBY_OBJ_RUN): counts the object's frames and ends it after
 * its lifetime (MODEL_OBJ_PET_JOY: the cooking's time) or when its
 * owner is gone, follows the owner's bone where the type does, and steps
 * each part: a part waits while its frame is 0 or less, then lives its
 * frames, its size, offset and alpha read from its key curves; each part
 * drawn goes into the sorted ordering table.
 * Built at optimization level 3. partFrameTable[i] and i * 0x5555 (an angle) are
 * written as indexing, not as pointers stepped by the loop: the compiler
 * then sets up its own stepping after the loop's first test, as the
 * game's code does.
 */
void ModelObj_Run(StageObj *obj)
{
    f32 boneMtx[4][4];
    Vec3f offset;
    Vec3f curveOffset;
    ModelObjPart *part = (ModelObjPart *)obj->buf;
    s16 partCount = gModelObjPartCounts[obj->type];
    u16 lifetime;
    s16 keyIdx;
    s16 keysPerPart;
    u16 partFrames;
    u16 *partFrameTable;
    s16 i;
    s32 curveIdx;
    void *curve;

    if (obj->type == MODEL_OBJ_PET_JOY) {
        lifetime = Poogie_JoyFrames();
    } else {
        lifetime = gModelObjLifetimes[obj->type];
    }
    keyIdx = gModelObjFirstKey[obj->type];
    keysPerPart = gModelObjKeysPerPart[obj->type];
    partFrames = gModelObjPartFrames[obj->type];
    partFrameTable = gModelObjTables.partFrames[obj->type];
    if (++obj->timer >= lifetime) {
        obj->state++;
        obj->visible = 0;
        return;
    }
    if (obj->type == MODEL_OBJ_PLAYER_FX_A) {
        if (((ObjHead *)obj->owner)->active == 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        Mtx_Copy(boneMtx[0], (f32 *)Em_GetBone(obj->owner, 18));
        offset.x = -8.0f;
        offset.y = 0.0f;
        offset.z = 70.0f;
        Vec_RotateSelf(&offset.x, boneMtx[0]);
        obj->pos[0] = boneMtx[3][0] + offset.x;
        obj->pos[1] = boneMtx[3][1] + offset.y;
        obj->pos[2] = boneMtx[3][2] + offset.z;
    } else if (obj->type == MODEL_OBJ_PLAYER_FX_B) {
        if (((ObjHead *)obj->owner)->active == 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
    } else if (obj->type == MODEL_OBJ_PET_JOY) {
        if (((ObjHead *)obj->owner)->active == 0) {
            obj->state++;
            obj->visible = 0;
            return;
        }
        Mtx_Copy(boneMtx[0], (f32 *)Em_GetBone(obj->owner, 20));
        offset.x = 0.0f;
        offset.y = 40.0f;
        offset.z = 0.0f;
        Vec_RotateSelf(&offset.x, boneMtx[0]);
        obj->pos[0] = boneMtx[3][0] + offset.x;
        obj->pos[1] = boneMtx[3][1] + offset.y;
        obj->pos[2] = boneMtx[3][2] + offset.z;
    }
    for (i = 0; i < partCount; i++, part++) {
        if (partFrameTable != NULL) {
            partFrames = partFrameTable[i];
        } else {
            keyIdx = gModelObjFirstKey[obj->type];
        }
        if (++part->frame <= 0) {
            keyIdx += keysPerPart;
            continue;
        }
        if (part->frame == 1) {
            if (obj->type == MODEL_OBJ_PLAYER_FX_B) {
                Mtx_Copy(boneMtx[0], (f32 *)Em_GetBone(obj->owner, 18));
                offset.x = -8.0f + ((Rand_Next(1) & 0x1F) - 16);
                offset.y = (Rand_Next(1) & 0xF) - 8;
                offset.z = 70.0f + ((Rand_Next(1) & 0x3F) - 32);
                Vec_RotateSelf(&offset.x, boneMtx[0]);
                part->pos.x = boneMtx[3][0] + offset.x;
                part->pos.y = boneMtx[3][1] + offset.y;
                part->pos.z = boneMtx[3][2] + offset.z;
            }
        } else if (partFrames < part->frame) {
            keyIdx += keysPerPart;
            continue;
        }
        offset.x = 0.0f;
        offset.y = 0.0f;
        offset.z = 0.0f;
        switch (obj->type) {
        case MODEL_OBJ_NPC_BURST:
            curveIdx = keyIdx;
            curve = gModelObjKeys[curveIdx];
            Key_LerpVec3(curve, &part->size.x, part->frame);
            Key_LerpVec3(gModelObjKeys[(s16)(curveIdx + 1)], &curveOffset.x, part->frame);
            switch (part->index) {
            default:
                Vec_RotateY((f32 *) &curveOffset, 6.2831855f * obj->angle / 65536.0f);
                break;
            case 4:
            case 5:
            case 6:
                Vec_RotateY((f32 *) &curveOffset, 6.2831855f * (obj->angle + i * 0x5555) / 65536.0f);
                break;
            }
            offset.x += curveOffset.x;
            offset.y += curveOffset.y;
            offset.z += curveOffset.z;
            keyIdx += 3;
            curve = gModelObjKeys[(s16)(curveIdx + 2)];
            Key_LerpFloat(curve, &part->alpha, part->frame);
            break;
        case MODEL_OBJ_NPC_PUFF:
            curveIdx = keyIdx;
            curve = gModelObjKeys[curveIdx];
            Key_LerpVec3(curve, &part->size.x, part->frame);
            keyIdx += 2;
            Key_LerpVec3(gModelObjKeys[(s16)(curveIdx + 1)], &curveOffset.x, part->frame);
            Vec_RotateY((f32 *) &curveOffset, 6.2831855f * obj->angle / 65536.0f);
            offset.x = part->frame * (0.030000001f * ((part->driftSeed & 0x3FF) - 512)) / partFrames;
            offset.y = 0.0f;
            offset.z = 0.0f;
            Vec_RotateSelf(&offset.x, gCamMatrix);
            offset.x += curveOffset.x;
            offset.y += curveOffset.y;
            offset.z += curveOffset.z;
            break;
        case MODEL_OBJ_SPARKLE:
            curveIdx = keyIdx;
            curve = gModelObjKeys[curveIdx];
            Key_LerpVec3(curve, &part->size.x, part->frame);
            curve = gModelObjKeys[(s16)(curveIdx + 1)];
            Key_LerpVec3(curve, &curveOffset.x, part->frame);
            Vec_RotateY((f32 *) &curveOffset, 6.2831855f * obj->angle / 65536.0f);
            keyIdx += 3;
            Key_LerpFloat(gModelObjKeys[(s16)(curveIdx + 2)], &part->alpha, part->frame);
            offset.x = part->frame * (0.010000001f * ((part->driftSeed & 0x3FF) - 512)) / partFrames;
            offset.y = 0.0f;
            offset.z = 0.0f;
            Vec_RotateSelf(&offset.x, gCamMatrix);
            offset.x += curveOffset.x;
            offset.y += curveOffset.y;
            offset.z += curveOffset.z;
            offset.x *= obj->scale;
            offset.y *= obj->scale;
            offset.z *= obj->scale;
            break;
        case MODEL_OBJ_PLAYER_FX_B:
            curve = gModelObjKeys[keyIdx++];
            Key_LerpVec3(curve, &part->size.x, part->frame);
            offset.z = 5.0f * i;
            Vec_RotateSelf(&offset.x, gCamMatrix);
            break;
        case MODEL_OBJ_PLAYER_FX_A:
            Vec_Copy((f32 *) &part->pos, (f32 *) &obj->pos);
            curveIdx = keyIdx;
            curve = gModelObjKeys[curveIdx];
            Key_LerpVec3(curve, &part->size.x, part->frame);
            keyIdx += 2;
            Key_LerpFloat(gModelObjKeys[(s16)(curveIdx + 1)], &part->alpha, part->frame);
            offset.z = 5.0f * i;
            Vec_RotateSelf(&offset.x, gCamMatrix);
            break;
        case MODEL_OBJ_PET_JOY:
            Vec_Copy((f32 *) &part->pos, (f32 *) &obj->pos);
            break;
        }
        part->angle += part->spin;
        if (part->draw != NULL) {
            part->draw->pos[0] = part->pos.x + offset.x;
            part->draw->pos[1] = part->pos.y + offset.y;
            part->draw->pos[2] = part->pos.z + offset.z;
            OT_AddByDepth(gDrawOtSorted, part->draw, 64, 0);
        }
    }
}
#pragma optimization_level reset

/* The end (LOBBY_OBJ_END): frees the object's own draw slot, if it has
 * one, and each part's. */
void ModelObj_End(StageObj *obj)
{
    ModelObjPart *part = (ModelObjPart *)obj->buf;
    s16 partCount;
    s16 i;

    obj->state++;
    partCount = gModelObjPartCounts[obj->type];
    if (obj->node != NULL) {
        DrawSlot_Free(obj->slot);
    }
    for (i = 0; i < partCount; i++, part++) {
        if (part->draw != NULL) {
            DrawSlot_Free(part->slot);
        }
    }
}

/* The end of a model object (LOBBY_OBJ_FREE): frees it. */
void ModelObj_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

extern u32 gModelObjColorKeys[];
extern u32 gModelObjPartColors[];
/* gModelObjPartColors' bytes, each colour's red, green, blue and alpha
 * (at colorIndex * 4), each named for the linker at its own address: the
 * game's code reads them so */
extern u8 gModelObjPartRed[];
extern u8 gModelObjPartGreen[];
extern u8 gModelObjPartBlue[];
extern u8 gModelObjPartAlpha[];

/*
 * Draws one part of a model object (its draw slot's callback): the
 * part's texture and tint by the object's type and the part's index,
 * scaled by the part's scale and size, at the slot's position facing the
 * camera. MODEL_OBJ_SPARKLE takes its colour from gModelObjPartColors
 * (red, green, blue, alpha at colorIndex * 4); MODEL_OBJ_PET_JOY is
 * drawn with flag 2 while the cooking is in its best window.
 */
void ModelObj_DrawPart(DrawSlot *slot)
{
    s32 color;
    Vec3f scale;
    Vec3f rot;
    f32 mtx[4][4];
    f32 uvMtx[4][4];
    u8 red;
    u8 green;
    u8 blue;
    ModelMatSet *mdl;
    StageObj *obj = slot->owner;
    Model *set = gStageModelSet;
    ModelObjPart *curPart = &((ModelObjPart *)obj->buf)[slot->part];
    u16 flags = 0;
    s32 order;
    s16 id;
    ModelMat *tex;
    s32 colorIdx;

    if (set != NULL && set->active != 0) {
        tex = set->mats;
        scale.x = curPart->scale * curPart->size.x;
        scale.y = curPart->scale * curPart->size.y;
        scale.z = curPart->scale * curPart->size.z;
        switch (obj->type) {
        case MODEL_OBJ_NPC_BURST:
            blue = green = red = 255;
            id = Effect_MapTexId(gModelObjTables.burstTexIds[curPart->index]);
            switch (curPart->index) {
            case 0:
                order = 0;
                break;
            case 1:
                flags |= 2;
                order = 0;
                green = 234;
                blue = 148;
                break;
            case 2:
                flags |= 2;
                order = 3;
                green = 124;
                blue = 118;
                break;
            case 3:
                flags |= 2;
                rot.z = 6.2831855f * curPart->angle / 65536.0f;
                order = 3;
                red = 255;
                green = 57;
                blue = 118;
                break;
            case 4:
            case 5:
            case 6:
                rot.z = 6.2831855f * curPart->angle / 65536.0f;
                order = 3;
                break;
            }
            color = ((u8)(255.0f * curPart->alpha) << 24) | (red << 16) | (green << 8) | blue;
            break;
        case MODEL_OBJ_NPC_PUFF:
            id = Effect_MapTexId(74);
            order = 0;
            Color_EvalKeys((s32 *) gModelObjColorKeys, curPart->frame, (u32 *) &color);
            break;
        case MODEL_OBJ_SPARKLE:
            id = Effect_MapTexId(72);
            rot.z = 6.2831855f * curPart->angle / 65536.0f;
            order = 3;
            colorIdx = obj->var2 * 4;
            color = ((u8)(gModelObjPartAlpha[colorIdx] * curPart->alpha) << 24) | (gModelObjPartRed[colorIdx] << 16) |
                    (gModelObjPartGreen[colorIdx] << 8) | gModelObjPartBlue[colorIdx];
            break;
        case MODEL_OBJ_PLAYER_FX_A:
            switch (curPart->index) {
            case 0:
            case 1:
            case 2:
            case 3:
                id = Effect_MapTexId(64);
                break;
            case 4:
                id = Effect_MapTexId(37);
                Mtx_Translation(uvMtx[0], 0.0f, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (u32)uvMtx);
                break;
            case 5:
            case 6:
            default:
                id = Effect_MapTexId(88);
                break;
            }
            flags |= 2;
            rot.z = 6.2831855f * curPart->angle / 65536.0f;
            order = 3;
            color = ((u8)(255.0f * curPart->alpha) << 24) | 0x7FDFFF;
            break;
        case MODEL_OBJ_PLAYER_FX_B:
            id = Effect_MapTexId(37);
            flags |= 2;
            rot.z = 6.2831855f * curPart->angle / 65536.0f;
            order = 3;
            color = -1;
            Mtx_Translation(uvMtx[0], 0.0f, 0.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (u32)uvMtx);
            break;
        case MODEL_OBJ_PET_JOY:
            id = Effect_MapTexId(23);
            order = 0;
            if (Poogie_AtGreatJoyStart() == 1) {
                flags |= 2;
                color = -1;
            } else {
                color = -1;
            }
            break;
        }
        if (id != -1) {
            mdl = &set->matSets[id];
            Mtx_FromScaleRotTrans(&scale.x, &rot.x, slot->pos, order, mtx[0]);
            Mtx_Concat3x3(mtx[0], gCamMatrix);
            Effect_DrawModel(mdl, mtx[0], color, flags, tex);
        }
    }
}

/* The file's data: the mine's tables of finds and bonus, then the model
 * objects' key curves and per-type tables. */
u16 gMineFindsWeak[8] = {
#include "lobby/007DBED0.inc"
};
u16 gMineFindsMid[8] = {
#include "lobby/007DBEE0.inc"
};
u16 gMineFindsStrong[8] = {
#include "lobby/007DBEF0.inc"
};
u16 gMineFindsWeakOpen[8] = {
#include "lobby/007DBF00.inc"
};
u16 gMineFindsMidOpen[8] = {
#include "lobby/007DBF10.inc"
};
u16 gMineFindsStrongOpen[8] = {
#include "lobby/007DBF20.inc"
};
u16 gMineFindsStrongEvent[8] = {
#include "lobby/007DBF30.inc"
};
s16 gMineBonusByLevel[16] = {
#include "lobby/007DBF40.inc"
};
Vec3Key gModelObjKeys_0[19] = {
#include "lobby/007DBF60.inc"
};
Vec3Key gModelObjKeys_1[24] = {
#include "lobby/007DC090.inc"
};
FloatKey gModelObjKeys_2[4] = {
#include "lobby/007DC210.inc"
};
Vec3Key gModelObjKeys_3[3] = {
#include "lobby/007DC230.inc"
};
Vec3Key gModelObjKeys_6[5] = {
#include "lobby/007DC260.inc"
};
Vec3Key gModelObjKeys_9[5] = {
#include "lobby/007DC2B0.inc"
};
Vec3Key gModelObjKeys_12[4] = {
#include "lobby/007DC300.inc"
};
Vec3Key gModelObjKeys_4[3] = {
#include "lobby/007DC340.inc"
};
Vec3Key gModelObjKeys_7[3] = {
#include "lobby/007DC370.inc"
};
Vec3Key gModelObjKeys_10[3] = {
#include "lobby/007DC3A0.inc"
};
Vec3Key gModelObjKeys_13[4] = {
#include "lobby/007DC3D0.inc"
};
FloatKey gModelObjKeys_5[4] = {
#include "lobby/007DC410.inc"
};
FloatKey gModelObjKeys_8[4] = {
#include "lobby/007DC430.inc"
};
FloatKey gModelObjKeys_11[4] = {
#include "lobby/007DC450.inc"
};
FloatKey gModelObjKeys_14[4] = {
#include "lobby/007DC470.inc"
};
u16 gModelObjBurstPartFrames[8] = {
#include "lobby/007DC490.inc"
};
s16 gModelObjBurstDelays[8] = {
#include "lobby/007DC4A0.inc"
};
Vec3Key gModelObjKeys_21[5] = {
#include "lobby/007DC4B0.inc"
};
Vec3Key gModelObjKeys_22[4] = {
#include "lobby/007DC500.inc"
};
u32 gModelObjColorKeys[8] = {
#include "lobby/007DC540.inc"
};
Vec3Key gModelObjKeys_23[3] = {
#include "lobby/007DC560.inc"
};
Vec3Key gModelObjKeys_24[3] = {
#include "lobby/007DC590.inc"
};
FloatKey gModelObjKeys_25[6] = {
#include "lobby/007DC5C0.inc"
};
u32 gModelObjPartColors[4] = {
#include "lobby/007DC5F0.inc"
};
Vec3Key gModelObjKeys_26[5] = {
#include "lobby/007DC600.inc"
};
Vec3Key gModelObjKeys_28[5] = {
#include "lobby/007DC650.inc"
};
Vec3Key gModelObjKeys_30[5] = {
#include "lobby/007DC6A0.inc"
};
Vec3Key gModelObjKeys_34[5] = {
#include "lobby/007DC6F0.inc"
};
Vec3Key gModelObjKeys_36[5] = {
#include "lobby/007DC740.inc"
};
Vec3Key gModelObjKeys_38[5] = {
#include "lobby/007DC790.inc"
};
FloatKey gModelObjKeys_27[4] = {
#include "lobby/007DC7E0.inc"
};
FloatKey gModelObjKeys_29[4] = {
#include "lobby/007DC800.inc"
};
FloatKey gModelObjKeys_31[4] = {
#include "lobby/007DC820.inc"
};
FloatKey gModelObjKeys_35[4] = {
#include "lobby/007DC840.inc"
};
FloatKey gModelObjKeys_37[4] = {
#include "lobby/007DC860.inc"
};
FloatKey gModelObjKeys_39[4] = {
#include "lobby/007DC880.inc"
};
u16 gModelObjFxAPartFrames[8] = {
#include "lobby/007DC8A0.inc"
};
Vec3Key gModelObjKeys_40[6] = {
#include "lobby/007DC8B0.inc"
};
void *gModelObjKeys[44] = {
    (void *)gModelObjKeys_0, (void *)gModelObjKeys_1, (void *)gModelObjKeys_2, (void *)gModelObjKeys_3,
    (void *)gModelObjKeys_4, (void *)gModelObjKeys_5, (void *)gModelObjKeys_6, (void *)gModelObjKeys_7,
    (void *)gModelObjKeys_8, (void *)gModelObjKeys_9, (void *)gModelObjKeys_10, (void *)gModelObjKeys_11,
    (void *)gModelObjKeys_12, (void *)gModelObjKeys_13, (void *)gModelObjKeys_14, (void *)gModelObjKeys_12,
    (void *)gModelObjKeys_13, (void *)gModelObjKeys_14, (void *)gModelObjKeys_12, (void *)gModelObjKeys_13,
    (void *)gModelObjKeys_14, (void *)gModelObjKeys_21, (void *)gModelObjKeys_22, (void *)gModelObjKeys_23,
    (void *)gModelObjKeys_24, (void *)gModelObjKeys_25, (void *)gModelObjKeys_26, (void *)gModelObjKeys_27,
    (void *)gModelObjKeys_28, (void *)gModelObjKeys_29, (void *)gModelObjKeys_30, (void *)gModelObjKeys_31,
    (void *)gModelObjKeys_30, (void *)gModelObjKeys_31, (void *)gModelObjKeys_34, (void *)gModelObjKeys_35,
    (void *)gModelObjKeys_36, (void *)gModelObjKeys_37, (void *)gModelObjKeys_38, (void *)gModelObjKeys_39,
    (void *)gModelObjKeys_40, NULL, NULL, NULL,
};
s16 gModelObjPartCounts[16] = {
#include "lobby/007DC9C0.inc"
};
u16 gModelObjLifetimes[16] = {
#include "lobby/007DC9E0.inc"
};
s16 gModelObjFirstKey[16] = {
#include "lobby/007DCA00.inc"
};
s16 gModelObjKeysPerPart[16] = {
#include "lobby/007DCA20.inc"
};
u16 gModelObjPartFrames[16] = {
#include "lobby/007DCA40.inc"
};
ModelObjTables gModelObjTables = {
#include "lobby/007DCA60.inc"
};
f32 gGalleryTargetSwingAmps[8] = {
#include "lobby/007DCAA0.inc"
};
