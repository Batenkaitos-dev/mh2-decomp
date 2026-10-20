/*
 * The rest of the shooting gallery's objects, the town's particles thrown
 * from an actor, and the house meal scene. Each object is a lobby stage
 * object (StageObj) with its own kind, an update that runs its four states
 * (init, run, end, free) and a draw slot callback:
 * - the target board's last queries and its target draw (a target is its
 *   kind's picture on a square, swinging after a hit that leaves it up);
 * - the gallery's shot (GalleryShot): the bullet flying from the gun, and
 *   the flash and puffs where a shot hits a wall;
 * - the results' score shown digit by digit (GalleryScore);
 * - particles thrown from an actor (ActorParticles): a spray, the hair
 *   falling at the barber's, the barber's hand effects;
 * - the points rising from a downed target (GalleryPoints);
 * - the meal scene (TOWN_MODE_MEAL_SCENE): the house's cook offers a meal,
 *   both sit, the cooking runs as a tutorial lesson, and the meal's bonuses
 *   are given (MealScene_*, Meal_ApplyEffects);
 * - the gun's muzzle flash (GalleryMuzzle): three flashes spawned together
 *   as the gun fires, each drawn with its own model and keys.
 * The particles and the meal sit between the gallery's objects in the
 * game's code and share their data block, so the file keeps them.
 */
#include "common.h"
#include "sprite.h"
#include "em.h"
#include "lobby_vobj.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "save.h"
#include "area.h"
#include "render_state.h"
#include "prim.h"
#include "player.h"
#include "lobby_state.h"
#include "lobby_npc.h"
#include "scene.h"
#include "quest.h"
#include "texture.h"
#include "award.h"
#include "lobby_labels.h"
/* Built at optimisation level 4: the compiler gives the target's draw the
 * game's code only at this level (the others match at either). */
#pragma optimization_level 4
void *memset(void *, s32, u32);

void ActorParticles_Init(StageObj *);
void ActorParticles_Run(StageObj *o);
void ActorParticles_End(StageObj *);
void ActorParticles_Free(StageObj *o);
void GalleryScore_Init(StageObj *o);
void GalleryScore_Run(StageObj *o);
void GalleryScore_End(StageObj *);
void GalleryScore_Free(StageObj *o);
void GalleryShot_Init(StageObj *o);
void GalleryShot_Run(StageObj *o);
void GalleryShot_End(StageObj *o);
void GalleryShot_Free(StageObj *o);
void GalleryShot_SetHit(StageObj *o);
void GalleryScore_Spawn(s16 arg0, u16 arg1);
void ActorParticles_SpawnSpray(void *arg0);
void GalleryPoints_Init(StageObj *o);
void GalleryPoints_Run(StageObj *o);
void GalleryPoints_End(StageObj *o);
void GalleryPoints_Free(StageObj *o);
void MealScene_Close(void);
void GalleryMuzzle_Init(StageObj *o);
void GalleryMuzzle_Run(StageObj *o);
void GalleryMuzzle_End(StageObj *o);
void GalleryMuzzle_Free(StageObj *o);
void DrawSlot_Free(s32);
void GalleryScore_Update(StageObj *);
void ActorParticles_Update(StageObj *);
void CamFocus_End(void);
void CamReq_Cancel(void);
void Npc_EnterRoutePoint(Em *npc);
void LobbyMenu_Init(void);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern u8 gMenuWork[];
s32 OT_AddAt(u32 *table, void *node, s32 idx, s32 n);
s32 Gallery_InMenuOrClose(void);
s32 Gallery_InResults(void);
void LobbySe_Play(s32);

/* Returns 1 once the board has flipped down and waited: the round refills
 * it. */
u8 GalleryBoard_IsCleared(StageObj *obj)
{
    return ((GalleryBoard *)obj->buf)->cleared;
}

/* Gives target i the hits of kind k (the round sets the kinds as it fills
 * the board). */
void GalleryBoard_SetTargetKind(StageObj *obj, u8 i, u8 kind)
{
    GalleryBoard *board = (GalleryBoard *)obj->buf;

    board->hitsLeft[i] = gGalleryTargetKinds[kind].hits;
}

/* Starts target i's swing, after a hit that leaves it standing. */
void GalleryBoard_SwingTarget(StageObj *obj, u8 i)
{
    GalleryBoard *board = (GalleryBoard *)obj->buf;

    board->swing[i] = GALLERY_SWING_FRAMES;
}

void Mtx_Identity(f32 *m);
s32 Gs_SetState(u8, u32);
void Camera_ProjectPoint(f32 *out, f32 *v);
void Gfx_SetTexFilter(s32);
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32);
f32 Math_Sin(f32 a);
void Vec_RotateX(f32 *v, f32 angle);
void Vec_RotateY(f32 *v, f32 a);
s32 EntryPicture_GetUVf(f32 *uv0, f32 *uv1, u16 id);
s32 Prim_DrawTexTriF(PrimTexTriF *t);
extern f32 gGalleryTargetSwingAmps[];
extern f32 gGalleryTargetSizes[];

/* Draws a target: its kind's picture (a cell of the pictures' texture) on
 * a square of its size, centred on pos, its top edge tilted about x by tilt
 * and the whole turned to yaw (both 0x10000 a turn), as two triangles.
 * While *swing runs, the tilt is replaced by a swing that fades
 * (gGalleryTargetSwingAmps, a fifth of the frames each) and *swing counts
 * down. */
void GalleryBoard_DrawTarget(Vec3f *pos, u8 kind, u32 tilt, u32 yaw, s8 *timer)
{
    Vec3f topLeft;
    Vec3f topRight;
    Vec3f bottomLeft;
    Vec3f bottomRight;
    f32 uvTopLeft[2];
    f32 uvBottomRight[2];
    Mtx44 mtx;
    PrimTexTriF prim;
    f32 yawRad;
    s8 swingLeft;
    f32 maxEdge;
    f32 tiltRad;
    f32 minEdge;
    f32 w;
    f32 swingRad;

    Gfx_SetTexFilter(1);
    Tex_Prepare(1, TEX_SLOT_PICTURES);
    Tex_Bind(TEX_SLOT_PICTURES);
    w = gGalleryTargetSizes[gGalleryTargetKinds[kind].size];
    minEdge = -w / 2.0f;
    maxEdge = w / 2.0f;
    topLeft.x = minEdge;
    topLeft.y = maxEdge;
    topLeft.z = 0.0f;
    swingRad = 0.0f;
    swingLeft = *timer;
    if (swingLeft <= 0) {
        *timer = 0;
    } else {
        tilt = 0;
        /* 80 degrees by the amplitude, a full swing every 5 frames */
        swingRad = 1.3962635f * (-1.0f * gGalleryTargetSwingAmps[(u8)((swingLeft - 1) / 5.0f)]) *
              Math_Sin(6.2831855f * (72.0f * (GALLERY_SWING_FRAMES - swingLeft)) / 360.0f);
        (*timer)--;
    }
    tiltRad = 6.2831855f * tilt / 65536.0f + swingRad;
    Vec_RotateX(&topLeft.x, tiltRad);
    yawRad = 6.2831855f * yaw / 65536.0f;
    Vec_RotateY((f32 *)&topLeft, yawRad);
    topLeft.x += pos->x;
    topLeft.y += pos->y;
    topLeft.z += pos->z;
    topRight.x = maxEdge;
    topRight.y = maxEdge;
    topRight.z = 0.0f;
    Vec_RotateX(&topRight.x, tiltRad);
    Vec_RotateY((f32 *)&topRight, yawRad);
    topRight.x += pos->x;
    topRight.y += pos->y;
    topRight.z += pos->z;
    bottomLeft.x = minEdge;
    bottomLeft.y = minEdge;
    bottomLeft.z = 0.0f;
    Vec_RotateY((f32 *)&bottomLeft, yawRad);
    bottomLeft.x += pos->x;
    bottomLeft.y += pos->y;
    bottomLeft.z += pos->z;
    bottomRight.x = maxEdge;
    bottomRight.y = minEdge;
    bottomRight.z = 0.0f;
    Vec_RotateY((f32 *)&bottomRight, yawRad);
    bottomRight.x += pos->x;
    bottomRight.y += pos->y;
    bottomRight.z += pos->z;
    prim.color[0] = -1;
    prim.color[1] = -1;
    prim.color[2] = -1;
    EntryPicture_GetUVf(uvTopLeft, uvBottomRight, gGalleryTargetKinds[kind].picture);
    prim.st[0][0] = uvTopLeft[0];
    prim.st[0][1] = uvTopLeft[1];
    prim.st[1][0] = uvTopLeft[0] + 0.1640625f;     /* a cell's side: 42 of 256 texels */
    prim.st[1][1] = uvTopLeft[1];
    prim.st[2][0] = uvTopLeft[0];
    prim.st[2][1] = uvTopLeft[1] + 0.1640625f;
    Mtx_Identity((f32 *) &mtx);
    Gs_SetState(RS_MTX_SLOT(0), (u32)&mtx);
    Camera_ProjectPoint((f32 *) &prim.v[0], (f32 *) &topLeft);
    prim.v[0].q = 1.0f;
    Camera_ProjectPoint((f32 *) &prim.v[1], (f32 *) &topRight);
    prim.v[1].q = 1.0f;
    Camera_ProjectPoint((f32 *) &prim.v[2], (f32 *) &bottomLeft);
    prim.v[2].q = 1.0f;
    Prim_DrawTexTriF(&prim);
    prim.st[0][0] = uvBottomRight[0];
    prim.st[0][1] = uvBottomRight[1];
    Camera_ProjectPoint((f32 *) &prim.v[0], (f32 *) &bottomRight);
    prim.v[0].q = 1.0f;
    Prim_DrawTexTriF(&prim);
}

/* The shot's update: runs the object's state. */
void GalleryShot_Update(StageObj *obj)
{
    u8 state;

    state = obj->state;
    switch (state) {
    case LOBBY_OBJ_INIT:
        GalleryShot_Init(obj);
        break;
    case LOBBY_OBJ_RUN:
        GalleryShot_Run(obj);
        break;
    case LOBBY_OBJ_END:
        GalleryShot_End(obj);
        break;
    case LOBBY_OBJ_FREE:
        GalleryShot_Free(obj);
        break;
    }
}

extern s8 gGalleryShotDelays[];
f32 Rand_Signed(void);
void Vec_RotateY(f32 *v, f32 a);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
u16 Rand_Next(s32 stream);
void GalleryShot_Draw(DrawSlot *);

/* The shot's init state. The bullet starts not hit. The flash and the
 * puffs take a random roll and their start delays; the puffs also move a
 * random step sideways (across the shot's yaw) and up, and rise 1 a frame.
 * Takes a draw slot; without one the object frees itself. */
void GalleryShot_Init(StageObj *obj)
{
    GalleryShot *shot = (GalleryShot *)obj->buf;
    Vec3f offset;
    f32 side;

    obj->var0 = 0;
    obj->var1 = 0;
    obj->var2 = 0;
    obj->state = LOBBY_OBJ_RUN;
    obj->visible = 1;
    obj->draw = 0;
    obj->timer = 0;
    switch (obj->type) {
    case GALLERY_SHOT_BULLET:
        shot->hit = 0;
        break;
    case GALLERY_SHOT_FLASH:
    case GALLERY_SHOT_PUFF1:
    case GALLERY_SHOT_PUFF2:
    case GALLERY_SHOT_PUFF3:
        shot->roll = Rand_Next(1);
        obj->timer = gGalleryShotDelays[obj->type - GALLERY_SHOT_FLASH];   /* the puffs start one after another */
        if (obj->type > GALLERY_SHOT_FLASH) {
            offset.x = 1.0f;
            offset.y = 0.0f;
            offset.z = 0.0f;
            Vec_RotateY((f32 *)&offset, (f32)(u32)obj->angle2 * 6.2831855f / 65536.0f);
            side = 15.0f * Rand_Signed();
            obj->pos[0] += offset.x * side;
            obj->pos[1] += 15.0f * Rand_Signed();
            shot->speed = 1.0f;
        }
        break;
    }
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->node = DrawSlot_Get(obj->slot);
        obj->node->owner = obj;
        obj->node->draw = GalleryShot_Draw;
    } else {
        StageObj_Free(obj);
    }
}

extern u8 gGalleryShotAlphaKeys0[];
extern u8 gGalleryShotScaleKeys0[];
extern u8 gGalleryShotAlphaKeys1[];
extern u8 gGalleryShotScaleKeys1[];
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
s32 Gallery_AddObj(StageObj *, u8 mode);
void Vec_Copy(f32 *dst, f32 *src);

/* The shot's run state. The bullet moves by vel, keeping its last
 * position, is drawn and put on the gallery's list of shots, and ends once
 * it hit something or after GALLERY_SHOT_FRAMES. The flash grows and fades
 * by its keys for 5 frames, the puffs for 30, rising. */
void GalleryShot_Run(StageObj *obj)
{
    GalleryShot *shot = (GalleryShot *)obj->buf;
    u8 *alphaKeys;
    u8 *scaleKeys;

    switch (obj->type) {
    case GALLERY_SHOT_BULLET:
        if (shot->hit == 0) {
            shot->prev = shot->cur;
            shot->cur.x += shot->vel.x;
            shot->cur.y += shot->vel.y;
            shot->cur.z += shot->vel.z;
            Vec_Copy(obj->node->pos, (f32 *)&shot->cur);
            if (obj->timer++ > GALLERY_SHOT_FRAMES) {
                obj->state = LOBBY_OBJ_END;
            }
            OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 0);
            Gallery_AddObj(obj, GALLERY_OBJ_SHOT);
        } else {
            obj->state = LOBBY_OBJ_END;
        }
        break;
    case GALLERY_SHOT_FLASH:
        alphaKeys = gGalleryShotAlphaKeys0;
        scaleKeys = gGalleryShotScaleKeys0;
        if (obj->timer++ > 4) {       /* frames */
            obj->state++;
        } else {
            Key_LerpVec3((f32 *) scaleKeys, (f32 *) &shot->scale, obj->timer);
            Key_LerpFloat((f32 *) alphaKeys, (f32 *) &shot->alpha, obj->timer);
            Vec_Copy(obj->node->pos, (f32 *)&obj->pos);
            OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 0);
        }
        break;
    case GALLERY_SHOT_PUFF1:
    case GALLERY_SHOT_PUFF2:
    case GALLERY_SHOT_PUFF3:
        if (++obj->timer < 0) {
            break;
        }
        alphaKeys = gGalleryShotAlphaKeys1;
        scaleKeys = gGalleryShotScaleKeys1;
        if (obj->timer > 30) {        /* frames */
            obj->state++;
        } else {
            Key_LerpVec3((f32 *) scaleKeys, (f32 *) &shot->scale, obj->timer);
            Key_LerpFloat((f32 *) alphaKeys, (f32 *) &shot->alpha, obj->timer);
            obj->pos[1] += shot->speed;
            Vec_Copy(obj->node->pos, (f32 *)&obj->pos);
            OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 0);
        }
        break;
    }
}

/* The shot's end state: frees its draw slot. */
void GalleryShot_End(StageObj *obj)
{
    obj->state = LOBBY_OBJ_FREE;
    if (obj->node != NULL) {
        DrawSlot_Free(obj->slot);
    }
}

/* The shot's free state: frees the object. */
void GalleryShot_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
extern f32 gCamMatrix[];

/* The shot's draw slot callback: the bullet's model at its full size; the
 * flash white and the puffs grey, scaled, faded and rolled, all facing the
 * camera. */
void GalleryShot_Draw(DrawSlot *slot)
{
    StageObj *obj = slot->owner;
    Model *mdl = gCommonModels[0];
    GalleryShot *shot = (GalleryShot *)obj->buf;
    ModelMat *tex;
    ModelMatSet *set;
    u16 flags = 0;
    u32 color;
    Vec3f scale;
    Vec3f rot;
    f32 *rollZ;
    f32 mtx[16];

    if (mdl != NULL && mdl->active != 0 && obj->visible != 0) {
        tex = mdl->mats;
        rollZ = &rot.z;
        rot.x = 0.0f;
        rot.y = 0.0f;
        *rollZ = 0.0f;
        switch (obj->type) {
        case GALLERY_SHOT_BULLET:
            color = -1;
            set = &mdl->matSets[39];
            scale.x = 1.0f;
            scale.y = 1.0f;
            scale.z = 1.0f;
            break;
        case GALLERY_SHOT_FLASH:
            set = &mdl->matSets[1];
            flags |= 2;
            scale.x = shot->scale.x;
            scale.y = shot->scale.y;
            scale.z = shot->scale.z;
            *rollZ = 6.2831855f * shot->roll / 65536.0f;
            color = ((u8)(255.0f * shot->alpha) << 24) | 0xFFFFFF;
            break;
        case GALLERY_SHOT_PUFF1:
        case GALLERY_SHOT_PUFF2:
        case GALLERY_SHOT_PUFF3:
            set = &mdl->matSets[6];
            flags |= 4;
            scale.x = shot->scale.x;
            scale.y = shot->scale.y;
            scale.z = shot->scale.z;
            *rollZ = 6.2831855f * shot->roll / 65536.0f;
            color = ((u8)(255.0f * shot->alpha) << 24) | 0x606060;
            break;
        }
        Mtx_FromScaleRotTrans((f32 *) &scale.x, (f32 *) &rot.x, slot->pos, 6, (f32 *) mtx);
        Mtx_Concat3x3(mtx, gCamMatrix);
        Effect_DrawModel(set, mtx, color, flags, tex);
    }
}

void Vec_Normalize(f32 *v);

/* Fires a bullet from 155 units ahead of from (the gun's muzzle) towards
 * to, at 50 units a frame. */
void GalleryShot_Spawn(Vec3f *muzzle, Vec3f *target)
{
    StageObj *obj;
    GalleryShot *shot;
    Vec3f dir;
    Vec3f aimCopy;
    f32 startAhead = 155.0f;
    f32 *dirY;
    f32 *dirZ;

    aimCopy.x = target->x;
    aimCopy.y = target->y;
    aimCopy.z = target->z;
    if ((obj = StageObj_Alloc(1)) != NULL) {
        obj->kind = LOBBY_OBJ_GALLERY_SHOT;
        obj->type = GALLERY_SHOT_BULLET;
        obj->update = GalleryShot_Update;
        dirY = &dir.y;
        dirZ = &dir.z;
        shot = (GalleryShot *)obj->buf;
        dir.x = target->x - muzzle->x;
        *dirY = target->y - muzzle->y;
        *dirZ = target->z - muzzle->z;
        Vec_Normalize((f32 *)&dir);
        shot->start.x = muzzle->x + dir.x * startAhead;
        shot->start.y = muzzle->y + *dirY * startAhead;
        shot->start.z = muzzle->z + *dirZ * startAhead;
        shot->aim = aimCopy;
        shot->cur = shot->start;
        shot->prev = shot->cur;
        shot->speed = 50.0f;
        shot->vel.x = dir.x * shot->speed;
        shot->vel.y = *dirY * shot->speed;
        shot->vel.z = *dirZ * shot->speed;
    }
}

void Vec_Normalize(f32 *v);
struct HitCapsule;
struct Capsule;
void Capsule_Copy(struct HitCapsule *src, struct Capsule *dst);
/* Writes the stretch the bullet crossed this frame (from its last position
 * to its new one) as a capsule of the given radius to out, and sets
 * capsule up from it for the gallery's hit tests. */
void GalleryShot_GetCapsule(f32 *out, f32 *capsule, StageObj *obj, f32 radius)
{
    GalleryShot *shot = (GalleryShot *)obj->buf;
    Vec3f dir = shot->vel;

    Vec_Normalize((f32 *)&dir);
    out[6] = radius;
    out[0] = shot->prev.x;
    out[1] = shot->prev.y;
    out[2] = shot->prev.z;
    out[3] = shot->cur.x;
    out[4] = shot->cur.y;
    out[5] = shot->cur.z;
    Capsule_Copy((struct HitCapsule *) out, (struct Capsule *) capsule);
}

/* Marks the bullet as having hit something: it ends on its next frame. */
void GalleryShot_SetHit(StageObj *obj)
{
    ((GalleryShot *)obj->buf)->hit = 1;
}

u16 Math_Heading(f32 x, f32 z, f32 a, f32 b);
void Vec_RotateY(f32 *v, f32 a);

/* Spawns the flash and the three puffs where a shot hits a wall: at pos,
 * moved 25 units along the shot's yaw (dir's heading). */
void GalleryShot_SpawnWallHit(Vec3f *pos, Vec3f *shotDir)
{
    StageObj *obj;
    u8 k;
    Vec3f offset;
    f32 *offsetY = &offset.y;
    f32 *offsetZ = &offset.z;

    for (k = GALLERY_SHOT_FLASH; k < GALLERY_SHOT_TYPES; k++) {
        if ((obj = StageObj_Alloc(1)) != NULL) {
            obj->kind = LOBBY_OBJ_GALLERY_SHOT;
            obj->type = k;
            obj->update = GalleryShot_Update;
            *(Vec3f *)obj->pos = *pos;
            obj->angle2 = Math_Heading(shotDir->x, shotDir->z, 0.0f, 0.0f);
            offset.x = 25.0f;
            *offsetY = 0.0f;
            *offsetZ = 0.0f;
            Vec_RotateY((f32 *)&offset, (f32)obj->angle2 * 6.2831855f / 65536.0f);
            obj->pos[0] += offset.x;
            obj->pos[1] += *offsetY;
            obj->pos[2] += *offsetZ;
        }
    }
}

/* The score's update: runs the object's state. */
void GalleryScore_Update(StageObj *obj)
{
    switch (obj->state) {
    case LOBBY_OBJ_INIT:
        GalleryScore_Init(obj);
        break;
    case LOBBY_OBJ_RUN:
        GalleryScore_Run(obj);
        break;
    case LOBBY_OBJ_END:
        GalleryScore_End(obj);
        break;
    case LOBBY_OBJ_FREE:
        GalleryScore_Free(obj);
        break;
    }
}

s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void GalleryScore_Draw(DrawSlot *slot);
/* The score's init state: splits the value into its digits and takes a
 * draw slot (without one the object frees itself). */
void GalleryScore_Init(StageObj *obj)
{
    GalleryScore *score = (GalleryScore *)obj->buf;
    s32 i;
    s32 rest;

    obj->var0 = 0;
    obj->var1 = 0;
    obj->var2 = 0;
    obj->state = LOBBY_OBJ_RUN;
    obj->visible = 1;
    obj->draw = 0;
    obj->timer = 0;
    obj->slot = DrawSlot_Alloc();
    rest = score->value;
    if (rest > 0) {
        for (i = 0; i < GALLERY_SCORE_DIGITS; i++) {
            score->digit[i] = rest % 10;
            rest -= score->digit[i];
            rest /= 10;
            if (rest == 0) {
                break;
            }
        }
        score->count = i + 1;
    } else {
        score->digit[0] = 0;
        score->count = 1;
    }
    score->shown = 0;
    score->rolling = 0;
    score->rollTimer = 0;
    if (obj->slot != -1) {
        obj->node = DrawSlot_Get(obj->slot);
        obj->node->owner = obj;
        obj->node->draw = GalleryScore_Draw;
    } else {
        StageObj_Free(obj);
    }
}

/* The score's run state: a digit settles every 30 frames (with sound 0xA)
 * while the next place rolls through 0 to 9, a step every 2 frames; every
 * digit shows at once while Gallery_InResults answers 1. Once all are
 * shown, the score ends when the gallery goes back to its menu or
 * closes. */
void GalleryScore_Run(StageObj *obj)
{
    s16 timer;
    s16 rollFrames;
    GalleryScore *score;

    score = (GalleryScore *)obj->buf;
    if (Gallery_InResults() == 1) {
        score->shown = (s16) score->count;
        obj->timer = 0;
    }
    if (score->shown < score->count) {
        timer = obj->timer + 1;
        obj->timer = timer;
        if (timer > 30) {
            score->shown = (s16) (score->shown + 1);
            obj->timer = 0;
            score->rolling = 0;
            LobbySe_Play(0xA);
        } else {
            rollFrames = score->rollTimer + 1;
            score->rollTimer = rollFrames;
            if (rollFrames >= 2) {
                score->rolling = (s16) (score->rolling + 1);
                score->rolling = (s16) ((s16) score->rolling % 10);
                LobbySe_Play(LOBBY_SE_SCROLL);
                score->rollTimer = 0;
            }
        }
        OT_AddAt(gDrawOtSorted, obj->node, 0, 1);
        return;
    }
    if (Gallery_InMenuOrClose() == 1) {
        obj->state = LOBBY_OBJ_END;
        return;
    }
    OT_AddAt(gDrawOtSorted, obj->node, 0, 1);
}

/* The score's end state: frees its draw slot. */
void GalleryScore_End(StageObj *obj)
{
    obj->state = LOBBY_OBJ_FREE;
    if (obj->node != NULL) {
        DrawSlot_Free(obj->slot);
    }
}

/* The score's free state: frees the object. */
void GalleryScore_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

void Text_SetSize(s32 size);
void Font_SetColor(s32 color);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(char *s, ...);
/* The score's draw slot callback: prints the settled digits from the
 * right (x 0x1B3, 15 apart), then the rolling one in the next place. */
void GalleryScore_Draw(DrawSlot *slot)
{
    StageObj *obj = slot->owner;
    GalleryScore *score = (GalleryScore *)obj->buf;
    s32 i;

    if (obj->visible) {
        Text_SetSize(0);
        Font_SetColor(0x80101010);
        Font_SetSpacing(0x14, 0x14);
        if (obj->timer <= 0) {
            s32 rest;
            rest = score->shown;
            if (rest >= score->count) {
                rest--;
            }
            for (i = 0; i <= rest; i++) {
                Font_SetPos(0x1B3 - i * 15, score->y);
                Text_PrintfColored("%d", score->digit[i]);
            }
        } else {

            for (i = 0; i < score->shown; i++) {
                Font_SetPos(0x1B3 - i * 15, score->y);
                Text_PrintfColored("%d", score->digit[i]);
            }
            Font_SetPos(0x1B3 - score->shown * 15, score->y);
            Text_PrintfColored("%d", score->rolling);
        }
    }
}

/* Shows points as the results' score at screen row y. */
void GalleryScore_Spawn(s16 points, u16 y)
{
    StageObj *obj;
    GalleryScore *score;

    obj = StageObj_Alloc(1);
    if (obj != NULL) {
        obj->kind = LOBBY_OBJ_GALLERY_SCORE;
        obj->type = 0;
        obj->update = GalleryScore_Update;
        score = (GalleryScore *)obj->buf;
        score->value = points;
        score->y = y;
    }
}

/* The particles' update: runs the object's state. */
void ActorParticles_Update(StageObj *obj)
{
    switch (obj->state) {
    case LOBBY_OBJ_INIT:
        ActorParticles_Init(obj);
        break;
    case LOBBY_OBJ_RUN:
        ActorParticles_Run(obj);
        break;
    case LOBBY_OBJ_END:
        ActorParticles_End(obj);
        break;
    case LOBBY_OBJ_FREE:
        ActorParticles_Free(obj);
        break;
    }
}

void Em_GetBonePos(void *obj, s16 bone, f32 *out);
s32 Npc_HasSecondHead(Em *arg0);
f32 Rand_Unit(void);
s32 Motion_PassedFrame(void *, s32 layer, f32 frame);
void Vec_RotateX(f32 *v, f32 angle);

EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
void ActorParticles_Draw(DrawSlot *slot);

/* The particles' init state: throws the six parts (SPRAY and HAIR from the
 * owner's bone, BARBER at the barber's bones 6 and 9, three a side), gives
 * each a draw slot, then runs the first frame. */
void ActorParticles_Init(StageObj *obj)
{
    f32 boneMtx[4][4];
    Vec3f offset;
    ActorParticles *parts = (ActorParticles *)obj->buf;
    u8 i;
    s32 pitch;
    s32 yaw;
    s8 sign;
    s8 coin;
    f32 spinX;
    f32 spinY;
    f32 spinZ;

    obj->state = LOBBY_OBJ_RUN;
    obj->visible = 1;
    obj->draw = 0;
    if (obj->type == ACTOR_PARTICLES_BARBER) {
        obj->timer = 30;
    } else {
        obj->timer = (u16)Rand_Next(1) % 20 + 10;
    }
    parts->on = 1;
    for (i = 0; i < ACTOR_PARTICLES; i++) {
        switch (obj->type) {
        case ACTOR_PARTICLES_SPRAY:
            if (Npc_HasSecondHead(obj->owner) != 0) {
                Em_GetBonePos(obj->owner, 0xC, &parts->pos[i].x);
            } else {
                Em_GetBonePos(obj->owner, 0x14, &parts->pos[i].x);
            }
            parts->timer[i] = (u16)Rand_Next(1) % 3 + 6;
            parts->speed[i] = 12.0f + (f32)((u16)Rand_Next(1) % 4);
            parts->dir[i].x = 0.0f;
            parts->dir[i].y = 0.0f;
            parts->dir[i].z = 1.0f;
            pitch = (u16)Rand_Next(1) % 32767 + 0x8000;
            yaw = (u16)Rand_Next(1);
            Vec_RotateX(&parts->dir[i].x, 6.2831855f * pitch / 65536.0f);
            Vec_RotateY((f32 *)&parts->dir[i], 6.2831855f * yaw / 65536.0f);
            break;
        case ACTOR_PARTICLES_HAIR:
            if (Npc_HasSecondHead(obj->owner) != 0) {
                Em_GetBonePos(obj->owner, 0xC, &parts->pos[i].x);
            } else {
                Em_GetBonePos(obj->owner, 0x14, &parts->pos[i].x);
            }
            parts->vy[i] = 8.0f + 6.0f * Rand_Unit();
            parts->pos[i].y += 15.0f;
            parts->timer[i] = 60.0f * Rand_Unit();
            parts->speed[i] = 3.0f + (f32)((u16)Rand_Next(1) % 3);
            parts->rot[i].x = 360.0f * Rand_Unit();
            parts->rot[i].y = 360.0f * Rand_Unit();
            parts->rot[i].z = 360.0f * Rand_Unit();
            parts->frame[i] = 0;
            coin = (s8)Rand_Next(1) % 2;
            sign = coin == 0 ? 1 : -1;
            spinX = 30.0f + 15.0f * (sign * Rand_Unit());
            spinY = 30.0f + 15.0f * (sign * Rand_Unit());
            spinZ = 30.0f + 15.0f * (sign * Rand_Unit());
            parts->spin[i].x = spinX / parts->timer[i];
            parts->spin[i].y = spinY / parts->timer[i];
            parts->spin[i].z = spinZ / parts->timer[i];
            parts->dir[i].x = 0.0f;
            parts->dir[i].y = 0.0f;
            parts->dir[i].z = 1.0f;
            pitch = (u16)Rand_Next(1) % 32767 + 0x8000;
            yaw = (u16)Rand_Next(1);
            Vec_RotateX(&parts->dir[i].x, 6.2831855f * pitch / 65536.0f);
            Vec_RotateY((f32 *)&parts->dir[i], 6.2831855f * yaw / 65536.0f);
            break;
        case ACTOR_PARTICLES_BARBER:
            parts->frame[i] = -1;
            switch (i) {
            case 0:
            case 2:
            case 4:
                Mtx_Copy(boneMtx[0], (f32 *)Em_GetBone(parts->em, 6));
                offset.x = 20.0f;
                offset.y = 0.0f;
                offset.z = 0.0f;
                break;
            case 1:
            case 3:
            case 5:
                offset.x = -20.0f;
                offset.y = 0.0f;
                offset.z = 0.0f;
                Mtx_Copy(boneMtx[0], (f32 *)Em_GetBone(parts->em, 9));
                break;
            }
            Vec_RotateSelf(&offset.x, boneMtx[0]);
            Mtx_GetTrans(&parts->pos[i].x, boneMtx[0]);
            parts->pos[i].x += offset.x;
            parts->pos[i].y += offset.y;
            parts->pos[i].z += offset.z;
            switch (i) {
            case 0:
            case 1:
                parts->timer[i] = 24;
                break;
            case 2:
            case 3:
                parts->timer[i] = 30;
                parts->rot[i].z = 360.0f * Rand_Unit();
                parts->spin[i].z = 120.0f * Rand_Signed() / parts->timer[i];
                break;
            case 4:
            case 5:
                parts->timer[i] = 16;
                break;
            }
            break;
        }
        parts->slot[i] = DrawSlot_Alloc();
        if (parts->slot[i] != -1) {
            parts->part[i] = DrawSlot_Get(parts->slot[i]);
            parts->part[i]->owner = obj;
            parts->part[i]->part = i;
            parts->part[i]->draw = ActorParticles_Draw;
        } else {
            parts->part[i] = NULL;
        }
    }
    ActorParticles_Run(obj);
}

/* The particles' run state. The spray turns on or off every 10 to 29
 * frames. While on, SPRAY and HAIR parts move (the hair falling, slowing
 * and tumbling) and are thrown again from the bone when their timers run
 * out; BARBER parts age, the middle pair rolling, and each shows for its
 * life. The object ends when the owner leaves state 0x6F (SPRAY), when the
 * barber passes frame 50 of motion 0x43D (HAIR), or after its 30 frames
 * (BARBER). */
void ActorParticles_Run(StageObj *obj)
{
    ActorParticles *parts = (ActorParticles *)obj->buf;
    u8 i;
    s32 pitch;
    s32 yaw;
    s8 sign;
    s8 coin;
    f32 spinX;
    f32 spinY;
    f32 spinZ;

    if (obj->type == ACTOR_PARTICLES_SPRAY) {
        if (obj->timer-- <= 0) {
            parts->on ^= 1;
            obj->timer = (u16)Rand_Next(1) % 20 + 10;
        }
    }
    if (parts->on == 1) {
        for (i = 0; i < ACTOR_PARTICLES; i++) {
            if (parts->part[i] != NULL) {
                switch (obj->type) {
                case ACTOR_PARTICLES_SPRAY:
                case ACTOR_PARTICLES_HAIR:
                    parts->pos[i].x += parts->dir[i].x * parts->speed[i];
                    parts->pos[i].z += parts->dir[i].z * parts->speed[i];
                    parts->frame[i]++;
                    if (obj->type == ACTOR_PARTICLES_HAIR) {
                        if (parts->vy[i] <= 0.0f) {
                            parts->vy[i] = -2.0f;
                            if (!(parts->speed[i] < 3.0f)) {
                                parts->speed[i] -= 2.0f;
                            }
                        } else {
                            parts->vy[i] += -1.0f;
                        }
                        parts->pos[i].y += parts->vy[i];
                        parts->rot[i].x += parts->spin[i].x;
                        parts->rot[i].y += parts->spin[i].y;
                        parts->rot[i].z += parts->spin[i].z;
                    } else {
                        parts->pos[i].y += parts->dir[i].y * parts->speed[i];
                    }
                    if (parts->timer[i]-- <= 0) {
                        parts->dir[i].x = 0.0f;
                        parts->dir[i].y = 0.0f;
                        parts->dir[i].z = 1.0f;
                        pitch = (u16)Rand_Next(1) % 32767 + 0x8000;
                        yaw = (u16)Rand_Next(1);
                        Vec_RotateX(&parts->dir[i].x, 6.2831855f * pitch / 65536.0f);
                        Vec_RotateY((f32 *)&parts->dir[i], 6.2831855f * yaw / 65536.0f);
                        if (Npc_HasSecondHead(obj->owner) != 0) {
                            Em_GetBonePos(obj->owner, 0xC, &parts->pos[i].x);
                        } else {
                            Em_GetBonePos(obj->owner, 0x14, &parts->pos[i].x);
                        }
                        switch (obj->type) {
                        case ACTOR_PARTICLES_SPRAY:
                            parts->speed[i] = 12.0f + (f32)((u16)Rand_Next(1) % 4);
                            parts->timer[i] = 6.0f + (f32)((u16)Rand_Next(1) % 3);
                            break;
                        case ACTOR_PARTICLES_HAIR:
                            parts->frame[i] = 0;
                            parts->speed[i] = 3.0f + (f32)((u16)Rand_Next(1) % 3);
                            parts->timer[i] = 60.0f * Rand_Unit();
                            parts->pos[i].y += 15.0f;
                            parts->vy[i] = 8.0f + 6.0f * Rand_Unit();
                            parts->rot[i].x = 360.0f * Rand_Unit();
                            parts->rot[i].y = 360.0f * Rand_Unit();
                            parts->rot[i].z = 360.0f * Rand_Unit();
                            coin = (s8)Rand_Next(1) % 2; sign = coin == 0 ? 1 : -1;
                            spinX = 30.0f + 15.0f * (sign * Rand_Unit());
                            spinY = 30.0f + 15.0f * (sign * Rand_Unit());
                            spinZ = 30.0f + 15.0f * (sign * Rand_Unit());
                            parts->spin[i].x = spinX / 60.0f;
                            parts->spin[i].y = spinY / 60.0f;
                            parts->spin[i].z = spinZ / 60.0f;
                            break;
                        }
                    }
                    break;
                case ACTOR_PARTICLES_BARBER:
                    parts->frame[i]++;
                    if (i == 2 || i == 3) {
                        parts->rot[i].z += parts->spin[i].z;
                    }
                    break;
                }
                if (obj->type != ACTOR_PARTICLES_BARBER || parts->frame[i] <= parts->timer[i]) {
                    Vec_Copy(parts->part[i]->pos, (f32 *)&parts->pos[i]);
                    OT_AddByDepth(gDrawOtSorted, parts->part[i], 0x40, 0);
                }
            }
        }
    }
    switch (obj->type) {
    case ACTOR_PARTICLES_SPRAY:
        if (((Em *)obj->owner)->state != 0x6F) {
            obj->state = LOBBY_OBJ_END;
        }
        break;
    case ACTOR_PARTICLES_HAIR:
        if (parts->em->motionId[0] == 0x43D && Motion_PassedFrame(parts->em, 0, 50.0f) != 0) {
            obj->state = LOBBY_OBJ_END;
        }
        break;
    case ACTOR_PARTICLES_BARBER:
        if (obj->timer-- <= 0) {
            obj->state = LOBBY_OBJ_END;
        }
        break;
    }
}

/* The particles' end state: frees the parts' draw slots. */
void ActorParticles_End(StageObj *obj)
{
    ActorParticles *parts = (ActorParticles *)obj->buf;
    u8 i;

    obj->state++;
    for (i = 0; i < ACTOR_PARTICLES; i++) {
        if (parts->part[i] != NULL) {
            DrawSlot_Free(parts->slot[i]);
        }
    }
}

/* The particles' free state: frees the object. */
void ActorParticles_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

extern u8 gActorParticleHairAlphaKeys[];
extern u8 gActorParticleBarberScaleKeys0[];
extern u8 gActorParticleBarberColorKeys0[];
extern u8 gActorParticleBarberScaleKeys1[];
extern u8 gActorParticleBarberColorKeys1[];
extern u8 gActorParticleBarberScaleKeys2[];
extern u8 gActorParticleBarberColorKeys2[];
void Color_EvalKeys(s32 *k, s32 t, u32 *out);

/* The particles' draw slot callback, for one part: the spray as small
 * translucent models facing the camera; the hair tumbling in the owner's
 * hair colour, fading by its frame; the barber's effects scaled and
 * coloured by their pair's keys. */
void ActorParticles_Draw(DrawSlot *slot)
{
    StageObj *obj = slot->owner;
    Model *mdl = gCommonModels[0];
    ActorParticles *parts = (ActorParticles *)obj->buf;
    ModelMat *tex;
    ModelMatSet *set;
    u16 flags = 0;
    u8 *scaleKeys;
    u8 *colorKeys;
    u32 rgb;
    f32 boneMtx[16];
    Vec3f rot;
    u32 color;
    Vec3f scale;
    f32 alpha;

    if (mdl != NULL && mdl->active != 0 && obj->visible != 0) {
        tex = mdl->mats;
        switch (obj->type) {
        case ACTOR_PARTICLES_SPRAY:
            rot.x = 0.0f;
            rot.y = 0.0f;
            rot.z = 0.0f;
            scale.x = 0.3f;
            scale.y = 0.3f;
            scale.z = 0.3f;
            set = &mdl->matSets[37];
            color = 0xB2FFFFFF;
            flags |= 2;
            Mtx_FromScaleRotTrans((f32 *) &scale.x, (f32 *) &rot.x, slot->pos, 6, (f32 *) boneMtx);
            Mtx_Concat3x3(boneMtx, gCamMatrix);
            break;
        case ACTOR_PARTICLES_HAIR:
            rot.x = 6.2831855f * parts->rot[slot->part].x / 360.0f;
            rot.y = 6.2831855f * parts->rot[slot->part].y / 360.0f;
            rot.z = 6.2831855f * parts->rot[slot->part].z / 360.0f;
            scale.x = 2.0f;
            scale.y = 2.0f;
            scale.z = 2.0f;
            set = &mdl->matSets[35];
            Key_LerpFloat((f32 *) gActorParticleHairAlphaKeys, (f32 *) &alpha, parts->frame[slot->part]);
            rgb = ((Player *)obj->owner)->hairColor;
            color = ((u8)(255.0f * alpha) << 24) | ((u8)(rgb >> 16) << 16) | ((u8)(rgb >> 8) << 8) | (u8)rgb;
            Mtx_FromScaleRotTrans((f32 *) &scale.x, (f32 *) &rot.x, slot->pos, 6, (f32 *) boneMtx);
            break;
        case ACTOR_PARTICLES_BARBER:
            switch (slot->part) {
            case 0:
            case 1:
                scaleKeys = gActorParticleBarberScaleKeys0;
                colorKeys = gActorParticleBarberColorKeys0;
                set = &mdl->matSets[5];
                rot.x = 0.0f;
                rot.y = 0.0f;
                rot.z = 0.0f;
                break;
            case 2:
            case 3:
                scaleKeys = gActorParticleBarberScaleKeys1;
                colorKeys = gActorParticleBarberColorKeys1;
                set = &mdl->matSets[13];
                rot.x = 0.0f;
                rot.y = 0.0f;
                rot.z = 6.2831855f * parts->rot[slot->part].z / 360.0f;
                break;
            case 4:
            case 5:
                scaleKeys = gActorParticleBarberScaleKeys2;
                colorKeys = gActorParticleBarberColorKeys2;
                set = &mdl->matSets[18];
                rot.x = 0.0f;
                rot.y = 0.0f;
                rot.z = 0.0f;
                break;
            }
            flags |= 2;
            Key_LerpVec3((f32 *) scaleKeys, (f32 *) &scale, parts->frame[slot->part]);
            Color_EvalKeys((s32 *) colorKeys, parts->frame[slot->part], &color);
            Mtx_FromScaleRotTrans((f32 *) &scale.x, (f32 *) &rot.x, slot->pos, 6, (f32 *) boneMtx);
            Mtx_Concat3x3(boneMtx, gCamMatrix);
            break;
        }
        Effect_DrawModel(set, boneMtx, color, flags, tex);
    }
}

/* Starts the spray on owner (an actor entering state 0x6F). */
void ActorParticles_SpawnSpray(void *obj)
{
    StageObj *spray;

    spray = StageObj_Alloc(1);
    if (spray != NULL) {
        spray->kind = LOBBY_OBJ_ACTOR_PARTICLES;
        spray->update = ActorParticles_Update;
        spray->owner = obj;
        spray->type = ACTOR_PARTICLES_SPRAY;
    }
}

/* Starts particles of type k on owner a, following the barber b. */
void ActorParticles_Spawn(void *actor, void *barber, u8 k)
{
    StageObj *obj = StageObj_Alloc(1);

    if (obj != NULL) {
        obj->kind = LOBBY_OBJ_ACTOR_PARTICLES;
        obj->update = ActorParticles_Update;
        obj->owner = actor;
        obj->type = k;
        ((ActorParticles *)obj->buf)->em = barber;
    }
}

/* The points' update: runs the object's state. */
void GalleryPoints_Update(StageObj *obj)
{
    u8 state;

    state = obj->state;
    switch (state) {
    case LOBBY_OBJ_INIT:
        GalleryPoints_Init(obj);
        break;
    case LOBBY_OBJ_RUN:
        GalleryPoints_Run(obj);
        break;
    case LOBBY_OBJ_END:
        GalleryPoints_End(obj);
        break;
    case LOBBY_OBJ_FREE:
        GalleryPoints_Free(obj);
        break;
    }
}

void GalleryPoints_Draw(DrawSlot *slot);
/* The points' init state: 6 frames to show, the position and age
 * cleared, and a draw slot (without one the object frees itself). */
void GalleryPoints_Init(StageObj *obj)
{
    GalleryPoints *pts = (GalleryPoints *)obj->buf;

    obj->var0 = 0;
    obj->var1 = 0;
    obj->var2 = 0;
    obj->state = LOBBY_OBJ_RUN;
    obj->visible = 1;
    obj->draw = 0;
    obj->timer = 6;       /* frames */
    pts->age = 0;
    pts->x = 0.0f;
    pts->y = 0.0f;
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->node = DrawSlot_Get(obj->slot);
        obj->node->owner = obj;
        obj->node->draw = GalleryPoints_Draw;
    } else {
        StageObj_Free(obj);
    }
}

/* The points' run state: each frame the object's position is projected
 * to the screen and lifted 20 a frame of age; ends after its frames. */
void GalleryPoints_Run(StageObj *obj)
{
    Mtx44 mtx;
    GalleryPoints *pts = (GalleryPoints *)obj->buf;

    if (obj->timer-- <= 0) {
        obj->state = LOBBY_OBJ_END;
    } else {
        pts->age++;
        Mtx_Identity((f32 *) &mtx);
        Gs_SetState(RS_MTX_SLOT(0), (u32)&mtx);
        Camera_ProjectPoint((f32 *) pts, (f32 *) &obj->pos);
        pts->y -= pts->age * 20;
        OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 0);
    }
}

/* The points' end state: frees the draw slot. */
void GalleryPoints_End(StageObj *obj)
{
    obj->state = LOBBY_OBJ_FREE;
    if (obj->node != NULL) {
        DrawSlot_Free(obj->slot);
    }
}

/* The points' free state: frees the object. */
void GalleryPoints_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

void Text_PrintfAt(s16 x, s16 y, s16 color, const char *fmt, ...);
/* The points' format: the game keeps it apart from the score's string. */
const char gGalleryPointsFmt[3] = {
#include "lobby/00825EE8.inc"
};
/* The points' draw slot callback: prints them at the projected position
 * (from the 512-wide projection to the 640-wide screen). */
void GalleryPoints_Draw(DrawSlot *slot)
{
    StageObj *obj = slot->owner;
    GalleryPoints *pts = (GalleryPoints *)obj->buf;
    f32 x;

    if (obj->visible) {
        x = 640.0f * pts->x / 512.0f;
        Font_SetSpacing(0x28, 0x28);
        Text_PrintfAt(-15.0f + x, 7.0f + pts->y, 5, gGalleryPointsFmt, pts->value);
    }
}

void GalleryPoints_Update(StageObj *o);
/* Shows the points n of a downed target, rising from pos. */
void GalleryPoints_Spawn(Vec3f *pos, s16 points)
{
    StageObj *obj;

    if ((obj = StageObj_Alloc(1)) != NULL) {
        obj->kind = LOBBY_OBJ_GALLERY_POINTS;
        obj->type = 0;
        obj->update = GalleryPoints_Update;
        Vec_Copy((f32 *)&obj->pos, (f32 *)pos);
        ((GalleryPoints *)obj->buf)->value = points;
    }
}

s16 Village_MealVisitTalk(s32 id);
void Talk_Say(s32 msg, s8 a);
void Npc_HoldRoute(Em *npc);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s8 gMenuInputLock;

/* Opens the meal scene with the house's cook npc: the cook's offer, and
 * the two seats by the house (AREA_ONLINE_HOUSE_0 to _2). If a meal was
 * eaten already, only the cook's line for that. */
void MealScene_Open(Em *npc)
{
    MealScene *scene = (MealScene *)gMenuWork;

    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_MEAL_SCENE;
    memset(scene, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    gMenuInputLock = 6;
    scene->npc = npc;
    scene->pl = &gPlayers[gScene.localPlayer];
    scene->talk = Village_MealVisitTalk(((NpcWork *)&npc->work)->id);
    Talk_Say(scene->talk, 0);
    Npc_HoldRoute(npc);
    switch (gScene.area) {
    case AREA_ONLINE_HOUSE_0:
        scene->plSpot = 0;
        scene->npcSpot = 1;
        break;
    case AREA_ONLINE_HOUSE_1:
        scene->plSpot = 3;
        scene->npcSpot = 4;
        break;
    case AREA_ONLINE_HOUSE_2:
        scene->plSpot = 9;
        scene->npcSpot = 10;
        break;
    }
    scene->seatMask = 0;
    scene->seatMask |= 1 << scene->plSpot;
    scene->seatMask |= 1 << scene->npcSpot;
    if (gLobby->mealEaten) {
        scene->step = MEAL_STEP_DECLINED;
        Talk_Say(scene->talk + 4, 0);
    }
}

/* Closes the meal scene: back to TOWN_MODE_FREE, the camera released, the
 * cook (if still there) back on its route. */
void MealScene_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    LobbyMenu_Init();
    CamReq_Cancel();
    CamFocus_End();
    if (((MealScene *)gMenuWork)->npc != NULL) {
        Npc_EnterRoutePoint(((MealScene *)gMenuWork)->npc);
    }
}

extern MealEffect gMealEffects[][3];
extern char **gMealTasteTexts[];
u8 Notice_PushItem(char *);
u16 Rand_Next(s32 stream);
/* unit's own declaration: declared returning s32 as the game's callers use it; with the definition's u8 the calls compile differently */
s32 Village_FacilityLevel(s32 sub);
void Village_ClearMealVisit(u16 id);
s32 RateFlag_SetAndNotify(s32 a);
s32 Award_Grant(s32 a);

/* Gives the meal's bonuses, by the cook's group (three groups of
 * townspeople) and the house's level, with a notice for each and a random
 * line on the taste; marks the meal eaten, adds meal points (the level
 * times 12, while below 0x136) and counts it in the meal record (two a
 * meal), which at 100 sets rate flag 10 and grants award 0x21. arg is the
 * scene, read as its cook. */
void Meal_ApplyEffects(u8 **scene)
{
    u8 *kind = *scene + 0x438;    /* the cook's NpcWork.id (Em.work) */
    s8 cookGroup;
    s8 houseLevel;
    MealEffect *effect;
    char **tasteTexts;

    switch (*kind) {
    case 1:
    case 5:
    case 11:
        cookGroup = 0;
        break;
    case 3:
    case 10:
    case 26:
        cookGroup = 1;
        break;
    default:
        cookGroup = 2;
        break;
    }
    houseLevel = Village_FacilityLevel(VILLAGE_FACILITY_HOUSE);
    effect = &gMealEffects[houseLevel][cookGroup];
    gScene.mealStamina = effect->stamina;
    gScene.mealHealth = effect->health;
    gScene.mealAttack = effect->attack;
    gScene.mealDefense = effect->defense;
    if (gScene.mealStamina > 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_STAMINA_UP]);
    }
    if (gScene.mealHealth > 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_HEALTH_UP]);
    }
    if (gScene.mealAttack > 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_ATTACK_UP]);
    }
    if (gScene.mealDefense > 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_DEFENSE_UP]);
    }
    gLobby->mealEaten = 1;
    Village_ClearMealVisit(*kind);
    if (gSave.mealPoints < 0x136) {
        gSave.mealPoints += houseLevel * 12;
    }
    if (gSave.recordCounts[RECORD_MEALS] < 0xFDE8) {
        gSave.recordCounts[RECORD_MEALS] += 2;
        if (gSave.recordCounts[RECORD_MEALS] >= 100) {
            if (!(gSave.rateFlags & (1 << 10))) {
                RateFlag_SetAndNotify(RATE_FLAG_MEALS);
            }
            Award_Grant(AWARD_MEALS);
        }
    }
    tasteTexts = gMealTasteTexts[0];
    Notice_PushItem(tasteTexts[(u16)Rand_Next(1) % 4]);
}

s32 Talk_Update(void);
void FadeA_Start(s32 a);
u8 FadeA_GetState(void);
void Actor_SetState(void *, s32, s32);
void Actor_MoveToSceneSpot(Player *p, s16 id);
/* unit's own declaration: declared with the argument types the game's callers pass; with the definition's (s8, s32, s32) the call compiles differently */
void NpcScene_Start(s32 a, s32 mask, u8 level);
s32 NpcScene_IsDone(void);
void Npc_HideAndAdvance(Em *npc);
void CamReq_Kitchen(s32 a);
u16 LobbyPad_Get(s32 a);

/* TOWN_MODE_MEAL_SCENE's update: one MEAL_STEP_* at a time (enum
 * MealStep). Talk_Update's answer 0 is taken as accepting, 3 as
 * declining. */
void MealScene_Update(void)
{
    MealScene *scene = (MealScene *)gMenuWork;
    s32 camKind;

    LobbyPad_Get(0);
    switch (scene->step) {
    case MEAL_STEP_OFFER:
        switch (Talk_Update()) {
        case 0:
            scene->step++;
            Talk_Say(scene->talk + 1, 0);
            break;
        case 3:
            scene->step = MEAL_STEP_DECLINED;
            Talk_Say(scene->talk + 2, 0);
            break;
        }
        break;
    case MEAL_STEP_ACCEPTED:
        switch (Talk_Update()) {
        case 0:
        case 3:
            scene->step++;
            FadeA_Start(1);
            break;
        }
        break;
    case MEAL_STEP_SEAT:
        if (FadeA_GetState() != 1) {
            scene->step++;
            scene->timer = 3;
            CamFocus_End();
            Actor_MoveToSceneSpot(scene->pl, scene->plSpot);
            Actor_MoveToSceneSpot((Player *)scene->npc, scene->npcSpot);
            Actor_SetState(scene->pl, 0, 0x25);
            Actor_SetState(scene->npc, 0, 0x25);
        }
        break;
    case MEAL_STEP_SEATED:
        if (--scene->timer <= 0) {
            scene->step++;
            FadeA_Start(2);
            switch (gScene.area) {
            case AREA_ONLINE_HOUSE_0:
            default:
                camKind = 0;
                break;
            case AREA_ONLINE_HOUSE_1:
                camKind = 1;
                break;
            case AREA_ONLINE_HOUSE_2:
                camKind = 2;
                break;
            case AREA_HUB_0x57:
                camKind = 3;
                break;
            }
            CamReq_Kitchen(camKind);
            NpcScene_Start(NPC_SCENE_COOKING, scene->seatMask, Village_FacilityLevel(VILLAGE_FACILITY_HOUSE));
        }
        break;
    case MEAL_STEP_COOKING:
        if (NpcScene_IsDone() == 1) {
            scene->step++;
            Actor_SetState(scene->pl, 0, 0x29);
            Actor_SetState(scene->npc, 0, 0x29);
        }
        break;
    case MEAL_STEP_SERVE:
        if (scene->pl->motionId[0] == 0x261) {
            scene->step++;
            Actor_SetState(scene->pl, 0, 0x18);
            Actor_SetState(scene->npc, 0, 0x18);
            Meal_ApplyEffects((u8 **)scene);
        }
        break;
    case MEAL_STEP_EATING:
        if (scene->pl->motion[0].playing <= 0) {
            scene->step++;
        }
        break;
    case MEAL_STEP_EATEN:
        if (scene->pl->motion[0].playing <= 0) {
            scene->step++;
            Talk_Say(scene->talk + 3, 0);
        }
        break;
    case MEAL_STEP_THANKS:
        switch (Talk_Update()) {
        case 0:
            scene->step++;
            FadeA_Start(1);
            break;
        }
        break;
    case MEAL_STEP_LEAVE:
        if (FadeA_GetState() != 1) {
            Npc_HideAndAdvance(scene->npc);
            scene->npc = NULL;
            FadeA_Start(2);
            MealScene_Close();
        }
        break;
    case MEAL_STEP_DECLINED:
        switch (Talk_Update()) {
        case 0:
        case 3:
            MealScene_Close();
            break;
        }
        break;
    }
}

extern ActionSpot *gAreaActionSpots[];
u8 Area_GetSetIndex(u16 id);
/* Places an actor (a Player, or a townsperson read as one) on the current
 * area's action spot of kind 10 with this id: its position and facing,
 * and its move target and target yaw, so that it stays there. */
void Actor_MoveToSceneSpot(Player *actor, s16 id)
{
    ActionSpot *spot = gAreaActionSpots[(u8)Area_GetSetIndex(gScene.area)];

    while (spot->kind != 0) {
        if (spot->kind == 10 && spot->type == id) {
            actor->pos[0] = spot->x;
            actor->pos[1] = spot->y;
            actor->pos[2] = spot->z;
            actor->rot[1] = spot->angle;
            *(Vec3f *)actor->moveTo = *(Vec3f *)actor->pos;
            actor->targetYaw = actor->rot[1];
            return;
        }
        spot++;
    }
}

/* The muzzle flash's update: runs the object's state. */
void GalleryMuzzle_Update(StageObj *obj)
{
    u8 state;

    state = obj->state;
    switch (state) {
    case LOBBY_OBJ_INIT:
        GalleryMuzzle_Init(obj);
        break;
    case LOBBY_OBJ_RUN:
        GalleryMuzzle_Run(obj);
        break;
    case LOBBY_OBJ_END:
        GalleryMuzzle_End(obj);
        break;
    case LOBBY_OBJ_FREE:
        GalleryMuzzle_Free(obj);
        break;
    }
}

void GalleryMuzzle_Draw(DrawSlot *slot);
/* The muzzle flash's init state: its roll (none for type 0, random for the
 * others) and its life by type, and a draw slot (without one the object
 * frees itself). */
void GalleryMuzzle_Init(StageObj *obj)
{
    GalleryMuzzle *muzzle = (GalleryMuzzle *)obj->buf;

    obj->var0 = 0;
    obj->var1 = 0;
    obj->var2 = 0;
    obj->timer = 0;
    obj->state++;
    obj->visible = 1;
    obj->draw = 0;
    switch (obj->type) {
    case GALLERY_MUZZLE_BLAST:
        muzzle->roll = 0;
        muzzle->life = 8;
        break;
    case GALLERY_MUZZLE_FLASH:
        muzzle->roll = Rand_Next(1);
        muzzle->life = 4;
        break;
    case GALLERY_MUZZLE_PUFF:
        muzzle->roll = Rand_Next(1);
        muzzle->life = 22;
        break;
    }
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->node = DrawSlot_Get(obj->slot);
        obj->node->owner = obj;
        obj->node->draw = GalleryMuzzle_Draw;
    } else {
        StageObj_Free(obj);
    }
}

/* The muzzle flash's run state: shows at its position for its life. */
void GalleryMuzzle_Run(StageObj *obj)
{
    GalleryMuzzle *muzzle = (GalleryMuzzle *)obj->buf;

    if (++obj->timer > muzzle->life) {
        obj->state++;
    } else if (obj->node != NULL) {
        Vec_Copy(obj->node->pos, (f32 *)&obj->pos);
        OT_AddByDepth(gDrawOtSorted, obj->node, 0x40, 0);
    }
}

/* The muzzle flash's end state: frees its draw slot. */
void GalleryMuzzle_End(StageObj *obj)
{
    obj->state++;
    if (obj->node != NULL) {
        DrawSlot_Free(obj->slot);
    }
}

/* The muzzle flash's free state: frees the object. */
void GalleryMuzzle_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

extern u8 gGalleryMuzzleScaleKeys0[];
extern u8 gGalleryMuzzleColorKeys0[];
extern u8 gGalleryMuzzleScaleKeys1[];
extern u8 gGalleryMuzzleColorKeys1[];
extern u8 gGalleryMuzzleScaleKeys2[];
extern u8 gGalleryMuzzleColorKeys2[];

/* The muzzle flash's draw (its draw slot's): its type's model set of the
 * common effect models, scaled and coloured by the type's keys at its frame
 * (StageObj.timer), rolled by its roll and turned to face the camera, drawn
 * without writing the depth. Draws nothing while hidden or while the common
 * model isn't loaded. */
void GalleryMuzzle_Draw(DrawSlot *slot)
{
    StageObj *obj = slot->owner;
    Vec3f scale;
    Vec3f rot;
    u8 *scaleKeys;
    u32 color;
    f32 mtx[16];
    GalleryMuzzle *muzzle = (GalleryMuzzle *)obj->buf;
    Model *model = gCommonModels[0];
    ModelMat *mats;
    ModelMatSet *set;
    u8 *colorKeys;
    u16 flags = 0;

    if (model != NULL && model->active != 0 && obj->visible != 0) {
        mats = model->mats;
        switch (obj->type) {
        case GALLERY_MUZZLE_BLAST:
            scaleKeys = gGalleryMuzzleScaleKeys0;
            colorKeys = gGalleryMuzzleColorKeys0;
            set = &model->matSets[18];
            flags |= 2;
            break;
        case GALLERY_MUZZLE_FLASH:
            scaleKeys = gGalleryMuzzleScaleKeys1;
            colorKeys = gGalleryMuzzleColorKeys1;
            set = &model->matSets[1];
            flags |= 2;
            break;
        case GALLERY_MUZZLE_PUFF:
            scaleKeys = gGalleryMuzzleScaleKeys2;
            colorKeys = gGalleryMuzzleColorKeys2;
            set = &model->matSets[6];
            break;
        }
        rot.x = 0.0f;
        rot.y = 0.0f;
        rot.z = 6.2831855f * muzzle->roll / 65536.0f;
        Key_LerpVec3((f32 *) scaleKeys, (f32 *) &scale, obj->timer);
        Mtx_FromScaleRotTrans((f32 *) &scale.x, (f32 *) &rot.x, slot->pos, 6, (f32 *) mtx);
        Mtx_Concat3x3(mtx, gCamMatrix);
        Color_EvalKeys((s32 *) colorKeys, (f32)obj->timer, &color);
        Gs_SetState(RS_ZWRITE, 0);
        Effect_DrawModel(set, mtx, color, flags, mats);
        Gs_SetState(RS_ZWRITE, 1);
    }
}

void GalleryMuzzle_Update(StageObj *obj);

/* Spawns the muzzle flash as the gallery's gun fires: its three types at
 * one point 75 units ahead of pos along the camera's view (the reverse of
 * gCamMatrix's third row). */
void GalleryMuzzle_Spawn(f32 *pos)
{
    f32 flashPos[3];
    f32 dir[3];
    u8 i;
    StageObj *obj;

    dir[0] = -gCamMatrix[8];
    dir[1] = -gCamMatrix[9];
    dir[2] = -gCamMatrix[10];
    Vec_Normalize(dir);
    flashPos[0] = pos[0] + 75.0f * dir[0];
    flashPos[1] = pos[1] + 75.0f * dir[1];
    flashPos[2] = pos[2] + 75.0f * dir[2];
    for (i = 0; i < GALLERY_MUZZLE_TYPES; i++) {
        obj = StageObj_Alloc(1);
        if (obj != NULL) {
            obj->kind = LOBBY_OBJ_GALLERY_MUZZLE;
            obj->type = i;
            obj->update = GalleryMuzzle_Update;
            Vec_Copy(obj->pos, flashPos);
        }
    }
}

/* The shot's keys (0 the flash, 1 the puffs) and the puffs' start delays,
 * the particles' keys (the hair's alpha, the barber's three pairs), the
 * meals' bonuses, and the muzzle flash's scale and colour keys by type. */
u8 gGalleryShotAlphaKeys0[32] = {
#include "lobby/007DCDF0.inc"
};
u8 gGalleryShotScaleKeys0[48] = {
#include "lobby/007DCE10.inc"
};
u8 gGalleryShotAlphaKeys1[32] = {
#include "lobby/007DCE40.inc"
};
u8 gGalleryShotScaleKeys1[56] = {
#include "lobby/007DCE60.inc"
};
s8 gGalleryShotDelays[8] = {
#include "lobby/007DCE98.inc"
};
u8 gActorParticleHairAlphaKeys[32] = {
#include "lobby/007DCEA0.inc"
};
u8 gActorParticleBarberScaleKeys0[64] = {
#include "lobby/007DCEC0.inc"
};
u8 gActorParticleBarberColorKeys0[32] = {
#include "lobby/007DCF00.inc"
};
u8 gActorParticleBarberScaleKeys1[80] = {
#include "lobby/007DCF20.inc"
};
u8 gActorParticleBarberColorKeys1[32] = {
#include "lobby/007DCF70.inc"
};
u8 gActorParticleBarberScaleKeys2[64] = {
#include "lobby/007DCF90.inc"
};
u8 gActorParticleBarberColorKeys2[32] = {
#include "lobby/007DCFD0.inc"
};
MealEffect gMealEffects[3][3] = {
#include "lobby/007DCFF0.inc"
};
u8 gGalleryMuzzleScaleKeys0[64] = {
#include "lobby/007DD030.inc"
};
u8 gGalleryMuzzleColorKeys0[32] = {
#include "lobby/007DD070.inc"
};
u8 gGalleryMuzzleScaleKeys1[48] = {
#include "lobby/007DD090.inc"
};
u8 gGalleryMuzzleColorKeys1[32] = {
#include "lobby/007DD0C0.inc"
};
u8 gGalleryMuzzleScaleKeys2[96] = {
#include "lobby/007DD0E0.inc"
};
u8 gGalleryMuzzleColorKeys2[48] = {
#include "lobby/007DD140.inc"
};
