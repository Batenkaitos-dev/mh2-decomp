/*
 * The online house's props (HouseProp), a sub_main object (Obj_Alloc) with
 * draw slots of its own: on the first visit to the online house at house
 * level 1 or 2, its props (3 or 5 model parts of the area's sub-model,
 * placed by the house's position and angle tables) stand 120 frames, tip
 * over backwards a quarter turn, faster each frame, raise an effect at each
 * place and sink 500 units into the floor over 80 frames.
 *
 * The file's data: the props' places, angles and model parts.
 */
#include "common.h"
#include "em.h"
#include "lobby_vobj.h"
#include "obj.h"
#include "area.h"
#include "render_state.h"
#include "house_props.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "part_effects.h"

extern s16 gAreaAEModels[];                /* AREA_ONLINE_HOUSE_1's 3 models: part, angle, position */
extern u16 gHouse1PropRot[];
extern f32 gHouse1PropPos[];
extern s16 gAreaAFModels[];                /* AREA_ONLINE_HOUSE_2's 5 */
extern u16 gHouse2PropRot[];
extern f32 gHouse2PropPos[];

/* spawns the online house's props (an Obj of kind 15; HouseProp_Run) */
void HouseProp_Spawn(void)
{
    Obj *prop;

    prop = Obj_Alloc(0);
    if (prop != NULL) {
        prop->kind = 15;
        prop->update = (void *)HouseProp_Run;
    }
}

/* the props' update, by state */
void HouseProp_Run(HouseProp *prop)
{
    switch (prop->state) {
    case LOBBY_OBJ_INIT:
        HouseProp_Init(prop);
        break;
    case LOBBY_OBJ_RUN:
        HouseProp_Update(prop);
        break;
    case LOBBY_OBJ_END:
        HouseProp_End(prop);
        break;
    case LOBBY_OBJ_FREE:
        HouseProp_Free(prop);
        break;
    }
}

/* sets the props up, standing, at the house's first place; outside
 * AREA_ONLINE_HOUSE_1 and _2, or with no draw slot free, they are freed */
void HouseProp_Init(HouseProp *prop)
{
    prop->state++;
    prop->visible = 1;
    prop->draw = NULL;
    prop->phase = 0;
    prop->tilt = 0;
    prop->timer = 0;
    switch (gScene.area) {
    case AREA_ONLINE_HOUSE_1:
        Vec_Copy(prop->pos, gHouse1PropPos);
        break;
    case AREA_ONLINE_HOUSE_2:
        Vec_Copy(prop->pos, gHouse2PropPos);
        break;
    default:
        Obj_Free((Obj *)prop);
        return;
    }
    prop->slot = DrawSlot_Alloc();
    if (prop->slot != -1) {
        prop->ds = DrawSlot_Get(prop->slot);
        prop->ds->owner = prop;
        prop->ds->draw = HouseProp_Draw;
        HouseProp_Update(prop);
    } else {
        Obj_Free((Obj *)prop);
    }
}

/* the props stand 120 frames, tip over backwards (faster each frame) to a
 * quarter turn, raise an effect at each place, and end after 80 frames
 * down (sinking, HouseProp_Draw) */
void HouseProp_Update(HouseProp *prop)
{
    s16 i;
    s16 count;
    f32 *pos;

    prop->timer++;
    switch (prop->phase) {
    case 0:
        if (prop->timer > 120) {
            prop->phase++;
            prop->timer = 0;
        }
        break;
    case 1:
        prop->tilt -= (s16)(prop->timer * 32);
        if ((u16)prop->tilt < ANGLE_TURN * 3 / 4) {
            prop->phase++;
            prop->tilt = -(ANGLE_TURN / 4);
            prop->timer = 0;
            switch (gScene.area) {
            case AREA_ONLINE_HOUSE_1:
                count = 3;
                pos = gHouse1PropPos;
                break;
            case AREA_ONLINE_HOUSE_2:
                count = 5;
                pos = gHouse2PropPos;
                break;
            default:
                count = 0;
                break;
            }
            for (i = 0; i < count; i++) {
                PartFx_SpawnAt(pos, 7, 8.0f);
                pos += 3;
            }
        }
        break;
    case 2:
        if (prop->timer > 80) {
            prop->state++;
            return;
        }
        break;
    }
    OT_AddByDepth(gDrawOt32, prop->ds, 32, 1);
}

/* ends a house prop: frees its draw slot and moves it to its next state, hidden */
void HouseProp_End(HouseProp *prop)
{
    if (prop->ds != NULL) {
        DrawSlot_Free(prop->slot);
    }
    prop->state++;
    prop->visible = 0;
}

void HouseProp_Free(void *prop)
{
    Obj_Free((Obj *)prop);
}

/* draws the house's props (3 or 5 model parts of the area's sub-model) at
 * their places and angles, tilted, and sunk while down */
void HouseProp_Draw(DrawSlot *slot)
{
    f32 mtx[16];
    Model *scene;
    HouseProp *prop;
    ModelMatSet *part;
    s16 i;
    s16 count;
    s16 *models;
    u16 *rot;
    f32 *pos;

    prop = slot->owner;
    scene = gAreaSubModels[0];
    if (prop->visible != 0 && scene != NULL && scene->active != 0) {
        switch (gScene.area) {
        case AREA_ONLINE_HOUSE_1:
            count = 3;
            models = gAreaAEModels;
            rot = gHouse1PropRot;
            pos = gHouse1PropPos;
            break;
        case AREA_ONLINE_HOUSE_2:
            count = 5;
            models = gAreaAFModels;
            rot = gHouse2PropRot;
            pos = gHouse2PropPos;
            break;
        default:
            return;
        }
        Gs_SetState(RS_ALPHA_REF, 128);
        Gs_SetState(RS_COLOR2, -1);         /* opaque white */
        for (i = 0; i < count; i++) {
            part = &scene->matSets[*models];
            Mtx_Translation(mtx, pos[0], pos[1], pos[2]);
            if (prop->phase == 2) {
                mtx[13] -= 500.0f * (prop->timer / 80.0f);
            }
            Mtx_RotateX(mtx, 6.2831855f * prop->tilt / 65536.0f);
            Mtx_RotateY(mtx, 6.2831855f * *rot / 65536.0f);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            if (part != NULL && part->id != -1) {
                Draw_ApplyStates(part->flags);
                Model_Draw(part->id, 0);
            }
            models++;
            pos += 3;
            rot++;
        }
        Draw_SetDefaultState();
    }
}

f32 gHouse1PropPos[10] = {
#include "lobby/0081BC00.inc"
};
u16 gHouse1PropRot[4] = {
#include "lobby/0081BC28.inc"
};
s16 gAreaAEModels[8] = {
#include "lobby/0081BC30.inc"
};
f32 gHouse2PropPos[16] = {
#include "lobby/0081BC40.inc"
};
u16 gHouse2PropRot[8] = {
#include "lobby/0081BC80.inc"
};
s16 gAreaAFModels[8] = {
#include "lobby/0081BC90.inc"
};
