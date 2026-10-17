#include "common.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"
#include "sprite.h"

void StageObj_Free(StageObj *o);
void DrawSlot_Free(s32 id);
void Effect1A_Setup(StageObj *o);
void Effect1A_Run(StageObj *o);
void Effect1A_End(StageObj *o);
void Effect1A_Delete(StageObj *o);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Obj_InCurrentArea(void *obj); /* unit's own declaration: u8, as at its marked declaration below (s32 changes this unit's callers) */
StageObj *StageObj_Alloc(s32 nblk);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void Mtx_GetTrans(f32 *out, f32 *m);
f32 Math_Sin(f32 x);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
s32 Gs_SetState(u8 reg, u32 v);
s32 Effect_DrawModelAdd(void *list, f32 *v, void *base);
void Mtx_RotateY(f32 *m, f32 a);
s16 Effect_MapTexId(s16 id);
void StageObj_K11_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
s8 Effect_SpawnGround(void *owner, f32 *pos, s16 k, f32 scale);

void Effect08_Spawn(f32 *pos, s32 k, s32 b, f32 scale);
void Effect08_SpawnAtEm(Em *em, s32 k, s32 b, f32 scale, f32 y);
/* The ground effects of kind k at pos, raising pos 5 (with a landing sound, louder above scale 4); 0 when there are none. */
s8 Effect_SpawnGround(void *owner, f32 *pos, s16 k, f32 scale)
{
    s32 sound = 1;
    f32 s;

    switch (k) {
    case 0:
        return 0;
    case 1:
    default:
        s = 1.2f * scale;
        Effect08_Spawn(pos, 3, 0, s);
        pos[1] += 5.0f;
        Effect08_Spawn(pos, 0, 0, s);
        Sound_PlaySeAtPosChecked(1, 0x2E, 0, pos, 1, 0);
        return 1;
    case 2:
        if (gScene.frame & 2) {
            Effect08_Spawn(pos, 4, 0, 1.5f * scale);
        }
        pos[1] += 5.0f;
        s = 2.0f * scale;
        Effect08_Spawn(pos, 2, 0, s);
        break;
    case 3:
        s = 1.5f * scale;
        Effect08_Spawn(pos, 4, 0, s);
        pos[1] += 5.0f;
        Effect08_Spawn(pos, 5, 0, s);
        break;
    case 4:
        sound = 0;
    case 9:
        s = 2.7f * scale;
        pos[1] += 5.0f;
        Effect08_Spawn(pos, 1, 0, s);
        break;
    case 5:
        if (owner == NULL) {
            return 0;
        }
        s = 2.0f * scale;
        pos[1] += 5.0f;
        Effect08_SpawnAtEm(owner, 6, 0, s, pos[1]);
        if (!(gScene.frame & 7)) {
            sound = 0;
        }
        break;
    case 6:
        s = 2.5f * scale;
        Effect08_Spawn(pos, 4, 0, s);
        pos[1] += 5.0f;
        Effect08_Spawn(pos, 5, 0, s);
        break;
    case 7:
        s = 3.0f * scale;
        Effect08_Spawn(pos, 4, 0, s);
        pos[1] += 5.0f;
        Effect08_Spawn(pos, 5, 0, s);
        break;
    case 8:
        if (gScene.frame & 4) {
            Effect08_Spawn(pos, 4, 0, 1.5f * scale);
        }
        s = 2.0f * scale;
        pos[1] += 5.0f;
        Effect08_Spawn(pos, 2, 0, s);
        break;
    }
    if (sound != 0) {
        if (s > 4.0f) {
            Sound_PlaySeAtPosChecked(1, 0x76, 0, pos, 1, 0);
        } else {
            Sound_PlaySeAtPosChecked(1, 0x70, 0, pos, 1, 0);
        }
    }
    return 1;
}

/* the calls cast to u8: this unit's code takes the value as u8 */
s32 Obj_InCurrentArea(void *obj);
void Mtx_GetTrans(f32 *out, f32 *m);
void Effect0D_SpawnOnGround(f32 *pos, s32 type, f32 scale);
void StageObj_K11_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
/* An effect of the given kind at a player's bone, on the ground (kind 0x13 adds two more). */
void Player_SpawnGroundEffect(Player *p, s32 kind, f32 scale)
{
    f32 pos[3];

    if ((u8)Obj_InCurrentArea(p) != 0) {
        Mtx_GetTrans(pos, p->weaponModel->partBones[0][3].world);
        pos[1] = p->floorY;
        Effect0D_SpawnOnGround(pos, (u8)kind, scale);
        if ((u8)kind == 0x13) {
            StageObj_K11_CreateAtScaled(pos, *(u16 *)&p->rot[1], 10, 0.3f);
            Effect0D_SpawnOnGround(pos, 0xE, 0.9f);
        }
    }
}

void Effect1A_Update(StageObj *o);

/* A kind 0x1A object for a player (Effect1A_Update updates it). */
void Effect1A_Spawn(Player *p)
{
    StageObj *o;

    if ((u8)Obj_InCurrentArea(p) == 0) {
        return;
    }
    o = StageObj_Alloc(0);
    if (o == NULL) {
        return;
    }
    o->kind = 0x1A;
    o->update = Effect1A_Update;
    o->draw = NULL;
    o->owner = p;
    o->ownerIndex = p->index;
}

/* The update of the kind 0x1A objects (Effect1A_Spawn), by state: set up, run, end, free. */
void Effect1A_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        Effect1A_Setup(o);
        break;
    case 1:
        Effect1A_Run(o);
        break;
    case 2:
        Effect1A_End(o);
        break;
    case 3:
        Effect1A_Delete(o);
        break;
    }
}

s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void Effect1A_Draw(DrawSlot *s);
/* Sets up a kind 0x1A object: a draw slot drawn by Effect1A_Draw (freed at once when none is left). */
void Effect1A_Setup(StageObj *o)
{
    o->state++;
    o->var0 = 0;
    o->var1 = 0;
    o->visible = 1;
    o->draw = NULL;
    o->timer = 0;
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        *(DrawSlot **)&o->node = DrawSlot_Get(o->slot);
        (*(DrawSlot **)&o->node)->owner = o;
        (*(DrawSlot **)&o->node)->draw = Effect1A_Draw;
        return;
    }
    *(DrawSlot **)&o->node = NULL;
    Effect1A_Delete(o);
}

s32 Npc_IsBusyNotAct3C4B(Em *em);
s32 Npc_IsFree(Em *em);
s32 Npc_IsAct51to55(Em *em);
s32 Npc_IsAct4For50(Em *em);
s32 Npc_IsType4D(Em *em);
s32 Npc_IsType49(Em *em);
s32 LobbyTime_IsAm(void);
f32 Math_Sin(f32 x);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
extern f32 gLockOnMarkHeights[];
/* The run of a kind 0x1A object: its draw slot bobs above the player's +0x3B4 object, at a height by that object's state (lobby tests), and joins an ordering table. */
void Effect1A_Run(StageObj *o)
{
    Player *p = o->owner;
    Player *q;
    s16 k;
    f32 bob;

    if (((Player *)p->target) != NULL) {
        if (p->active == 0 || p->visible == 0 || (u8)Obj_InCurrentArea(p) == 0) {
            return;
        }
        q = ((Player *)p->target);
        o->timer++;
        if (Npc_IsType4D((Em *)q) == 1) {
            k = 4;
        } else if (Npc_IsType49((Em *)q) == 1) {
            k = 5;
        } else if (Npc_IsFree((Em *)q) == 1) {
            k = 0;
        } else if (Npc_IsBusyNotAct3C4B((Em *)q) == 1) {
            k = 1;
        } else if (Npc_IsAct51to55((Em *)q) == 1) {
            k = 2;
        } else if (Npc_IsAct4For50((Em *)q) == 1) {
            k = 3;
        } else {
            k = 0;
        }
        bob = 4.0f * Math_Sin(6.2831855f * (u16)(o->timer << 10) / 65536.0f);
        (*(DrawSlot **)&o->node)->pos[0] = q->pos[0];
        (*(DrawSlot **)&o->node)->pos[1] = bob + (q->pos[1] + gLockOnMarkHeights[k]);
        (*(DrawSlot **)&o->node)->pos[2] = q->pos[2];
        if (gScene.online != 0) {
            if (LobbyTime_IsAm() == 0) {
                OT_AddByDepth(gDrawListSky, *(DrawSlot **)&o->node, 1, 0);
                return;
            }
            OT_AddByDepth(gDrawOt32, *(DrawSlot **)&o->node, 0x20, 0);
            return;
        }
        OT_AddByDepth(gDrawOt32, *(DrawSlot **)&o->node, 0x20, 0);
    }
}

void Effect1A_End(StageObj *o)
{
    o->state++;
    if (*(s32 *)&o->node != 0) {
        DrawSlot_Free(o->slot);
    }
}

void Effect1A_Delete(StageObj *o)
{
    StageObj_Free(o);
}

s16 Effect_MapTexId(s16 id);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
s32 Effect_DrawModelAdd(void *list, f32 *v, void *base);
extern s32 gLockOnMarkColors[];
extern s8 gLockOnMarkSkip;
/* Draws a kind 0x1A object: model 0x86's entry turning with the object's timer, 7 times bigger, at the slot (not for the local player's own when it is busy online). */
void Effect1A_Draw(DrawSlot *s)
{
    StageObj *o = s->owner;
    Player *p = o->owner;
    Player *q = ((Player *)p->target);
    Model *set = gCommonModels[0];
    s16 k;
    ModelMat *arg;
    ModelMatSet *model;
    f32 m[4][4];

    if (q == NULL) {
        return;
    }
    if (gScene.online != 0) {
        if (p->stateGroup == PL_GROUP_WEAPON) {
            return;
        }
        if (gLockOnMarkSkip != 0) {
            gLockOnMarkSkip = 0;
            return;
        }
        if (gPlayers[gScene.localPlayer].action == 1) {
            return;
        }
    }
    k = Effect_MapTexId(0x86);
    if (k == -1) {
        return;
    }
    model = &set->matSets[k];
    arg = set->mats;
    Mtx_Scale(m[0], 7.0f, 7.0f, 7.0f);
    Mtx_RotateY(m[0], 6.2831855f * (o->timer << 8) / 65536.0f);
    Mtx_SetTrans(m[0], s->pos[0], s->pos[1], s->pos[2]);
    Gs_SetState(0x67, gLockOnMarkColors[q->bowgunReady]);
    Effect_DrawModelAdd(model, m[0], arg);
}

/* unk_3D2230's .data (0x467C40-0x467C78) */
f32 gLockOnMarkHeights[8] = {
#include "sub_main/00467C40.inc"
};
s32 gLockOnMarkColors[6] = {
#include "sub_main/00467C60.inc"
};
