/*
 * The drinking contest's mug: a stage object (kind LOBBY_OBJ_DRINK_MUG, its
 * work a DrinkMugWork) that a drinking townsperson holds. The townsperson's
 * set-up spawns it (DrinkMug_Spawn). While the contest is off it sits in the
 * hand and swivels now and then; when the contest starts (DrinkContest_IsOn)
 * it lifts off, flies out on a curve and circles the townsperson; when the
 * contest ends it spirals back to the hand, settles, turns to the
 * townsperson's heading and rests, and starts over. StageObj.type is the
 * phase (enum DrinkMugPhase). The contest's menu waits for it to be back
 * (DrinkMug_IsBack).
 *
 * The 16-bit angles are turned into radians as 2 pi * angle / 0x10000.
 */
#include "common.h"
#include "sprite.h"
#include "em.h"
#include "lobby_vobj.h"
#include "stageobj.h"
#include "lobby_globals.h"
#pragma optimization_level 4

void DrinkMug_Init(StageObj *o);
void DrinkMug_Step(StageObj *o);
void DrinkMug_End(StageObj *o);
void DrinkMug_Free(StageObj *o);
void DrawSlot_Free(s32);
u16 Rand_Next(s32 stream);
extern StageObj gStageObjs[];

/* The mug's update (StageObj.update): runs its set-up, its frame, its end or
 * its free by StageObj.state. */
void DrinkMug_Update(StageObj *obj)
{
    u8 state;

    state = obj->state;
    switch (state) {
    case LOBBY_OBJ_INIT:
        DrinkMug_Init(obj);
        break;
    case LOBBY_OBJ_RUN:
        DrinkMug_Step(obj);
        break;
    case LOBBY_OBJ_END:
        DrinkMug_End(obj);
        break;
    case LOBBY_OBJ_FREE:
        DrinkMug_Free(obj);
        break;
    }
}

struct EmBone *Em_Bone(void *obj, s16 bone);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateY(f32 *v, f32 a);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void DrinkMug_Draw(DrawSlot *);

/*
 * Sets the mug up in the townsperson's hand, DRINK_MUG_HAND_OFFSET ahead of
 * the hand along the heading, with a first wait of 5 to 14 frames before it
 * swivels, the flight's speed and curve, DrinkMug_IsBack's timeout (150
 * frames) and a draw slot. Frees the object when no draw slot is free.
 */
void DrinkMug_Init(StageObj *obj)
{
    DrinkMugWork *mug = (DrinkMugWork *)obj->buf;
    Vec3f pos;
    Vec3f ofs;

    obj->var0 = DRINK_MUG_WAIT;
    obj->var1 = 0;
    obj->timer = 0;
    obj->state++;
    obj->visible = 1;
    obj->draw = 0;
    Mtx_GetTrans((f32 *) &pos, Em_Bone(mug->npc, DRINK_MUG_BONE)->world);
    ofs.x = DRINK_MUG_HAND_OFFSET;
    ofs.y = 0.0f;
    ofs.z = 0.0f;
    Vec_RotateY((f32 *) &ofs, 6.2831855f * (f32)mug->npc->rotY / 65536.0f);
    pos.x += ofs.x;
    pos.y += ofs.y;
    pos.z += ofs.z;
    obj->pos[0] = pos.x;
    obj->pos[1] = pos.y;
    obj->pos[2] = pos.z;
    mug->idleWait = Rand_Next(1) % 10 + 5;
    mug->swivelFrames = 0;
    mug->swivelOffset = 0;
    mug->yaw = mug->swivelOffset + mug->npc->rotY;
    mug->heading = 6.2831855f * (f32)mug->yaw / 65536.0f;
    mug->atHand = 0;
    mug->returning = 0;
    mug->hopFrame = 0;
    mug->bobFrame = 0;
    mug->bobRise = 0;
    mug->turnFrames = 0;
    mug->turnDir = 0;
    /* the curve: a circle of 885.48 round, at 11.77518 a frame */
    mug->speed = -11.77518f;
    mug->curveRate = 65535.0f * mug->speed / 885.48004f;
    mug->speed = mug->speed * -1.0f;
    mug->backTimeout = 150; /* frames: 5 s */
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->node = DrawSlot_Get(obj->slot);
        obj->node->owner = obj;
        obj->node->draw = DrinkMug_Draw;
    } else {
        StageObj_Free((StageObj *) obj);
    }
}

void Em_BonePos(void *em, s16 bone, f32 *out);
f32 Rand_Signed(void);
f32 Math_Absf(f32 x);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Vec_Copy(f32 *dst, f32 *src);
f32 Vec_Length(f32 *v);
void Vec_Normalize(f32 *v);
s32 DrinkContest_IsOn(void);
f32 Math_Ceil(f32);

/*
 * The mug's frame, by its phase (StageObj.type, enum DrinkMugPhase). Held, it
 * follows the hand and swivels at random. When the contest starts it lifts
 * off (9 frames), flies out on a curve, rising, until it is
 * DRINK_MUG_ORBIT_RADIUS from the townsperson, and circles at that distance.
 * When the contest ends it spirals in to the hand, sinking, hovers there,
 * settles (9 frames), turns to the townsperson's heading with small hops,
 * rests 20 frames and is held again. Ends the object when the townsperson is
 * gone, and puts its draw slot in the sorted table each frame.
 */
void DrinkMug_Step(StageObj *obj)
{
    DrinkMugWork *mug = (DrinkMugWork *)obj->buf;
    Vec3f dir;
    Vec3f ofs;
    Vec3f hand;
    Vec3f toHand;
    f32 len;
    f32 diff;
    Em *npc;

    Vec_Copy(&mug->lastPos.x, &obj->pos[0]);
    if (mug->npc->active == 0) {
        obj->state++;
        return;
    }
    if (DrinkContest_IsOn() == 0 && obj->type > DRINK_MUG_HELD && mug->atHand == 0 && mug->returning == 0) {
        mug->returning = 1;
        obj->type = DRINK_MUG_RETURN;
        mug->frameTimer = 0;
        mug->animFrame = 0;
        mug->bobFrame = 0;
    }
    switch (obj->type) {
    case DRINK_MUG_HELD:
        if (DrinkContest_IsOn() == 1) {
            obj->type = DRINK_MUG_LIFT;
            mug->frameTimer = 2;
            mug->animFrame = 9;
        }
        mug->hand = Em_Bone(mug->npc, DRINK_MUG_BONE);
        Mtx_GetTrans((f32 *) &obj->pos, mug->hand->world);
        ofs.x = DRINK_MUG_HAND_OFFSET;
        ofs.y = 0.0f;
        ofs.z = 0.0f;
        Vec_RotateY((f32 *) &ofs, 6.2831855f * (f32)mug->npc->rotY / 65536.0f);
        obj->pos[0] += ofs.x;
        obj->pos[1] += ofs.y;
        obj->pos[2] += ofs.z;
        switch (obj->var0) {
        case DRINK_MUG_WAIT:
            if (mug->idleWait-- <= 0) {
                obj->var0 = DRINK_MUG_SWIVEL;
                mug->swivelGoal = (u16)(s32)(0.5f + 65536.0f * (f32)(Rand_Next(1) % 90) / 360.0f);
                mug->swivelFrames = Rand_Next(1) % 3 + 1;
                mug->swivelEnd = mug->swivelGoal - 0x2000;
                mug->swivelStep = (s16)(mug->swivelEnd - mug->swivelOffset) / mug->swivelFrames;
            } else {
                mug->swivelStep = 0.0f;
            }
            break;
        case DRINK_MUG_SWIVEL:
            if (mug->swivelFrames-- < 2) {
                obj->var0 = DRINK_MUG_WAIT;
                mug->idleWait = Rand_Next(1) % 10 + 5;
                mug->swivelStep = 0.0f;
            }
            break;
        }
        mug->swivelOffset += mug->swivelStep;
        mug->yaw = mug->swivelOffset + mug->npc->rotY;
        break;
    case DRINK_MUG_LIFT:
        if (mug->animFrame-- <= 0) {
            obj->type = DRINK_MUG_FLY_OUT;
            mug->frameTimer = 0;
            mug->animFrame = 0;
            /* the townsperson's heading, give or take 20 degrees */
            mug->flyHeading = 6.2831855f * (f32)mug->npc->rotY / 65536.0f + 0.34906587f * Rand_Signed();
        } else if (mug->frameTimer-- <= 0) {
            mug->frameTimer = 2;
        }
        break;
    case DRINK_MUG_FLY_OUT:
        if (!(mug->animFrame++ < 5.0f)) {
            mug->frameTimer = 0;
            mug->animFrame = 0;
        } else if (!(mug->frameTimer++ < 1.0f)) {
            mug->frameTimer = 0;
        }
        dir.x = -1.0f;
        dir.y = 0.0f;
        dir.z = 1.0f;
        Vec_RotateY((f32 *) &dir, mug->flyHeading);
        Vec_Normalize(&dir.x);
        mug->flyHeading += 6.2831855f * (f32)mug->curveRate / 65536.0f;
        obj->pos[0] += dir.x * mug->speed;
        obj->pos[1] += 7.0f;
        obj->pos[2] += dir.z * mug->speed;
        dir.x = mug->npc->posX - obj->pos[0];
        dir.y = 0.0f;
        dir.z = mug->npc->posZ - obj->pos[2];
        if (Vec_Length(&dir.x) >= DRINK_MUG_ORBIT_RADIUS) {
            obj->type = DRINK_MUG_CIRCLE;
            mug->frameTimer = 0;
            mug->animFrame = 0;
            mug->bobFrame = 0;
            npc = mug->npc;
            mug->orbit.x = obj->pos[0] - npc->posX;
            mug->orbit.y = 0.0f;
            mug->orbit.z = obj->pos[2] - npc->posZ;
            Vec_Normalize(&mug->orbit.x);
            mug->orbit.x *= DRINK_MUG_ORBIT_RADIUS;
            mug->orbit.z *= DRINK_MUG_ORBIT_RADIUS;
        }
        break;
    case DRINK_MUG_HOVER:
        Em_BonePos(mug->npc, DRINK_MUG_BONE, (f32 *)&hand);
        if (!(mug->animFrame++ < 5.0f) || obj->pos[1] - 2.0f < hand.y) {
            if (mug->atHand == 0) {
                obj->type = DRINK_MUG_CIRCLE;
                mug->frameTimer = 0;
                mug->animFrame = 0;
                mug->bobFrame = 0;
            } else if (obj->pos[1] - 2.0f < hand.y) {
                obj->type = DRINK_MUG_SETTLE;
                mug->frameTimer = 0;
                mug->animFrame = 0;
            } else {
                mug->frameTimer = 0;
                mug->animFrame = 0;
            }
        } else if (!(mug->frameTimer++ < 1.0f)) {
            mug->frameTimer = 0;
        }
        if (mug->atHand == 1) {
            if (mug->bobFrame++ < 6) {
                obj->pos[1] -= 2.0f;
            } else if (mug->bobRise++ < 5) {
                obj->pos[1] += 0.2f;
            } else {
                mug->bobFrame = 0;
                mug->bobRise = 0;
            }
        } else if (mug->bobFrame++ <= 12.0f) {
            if (mug->bobFrame <= 6.0f) {
                obj->pos[1] += 0.3f;
            } else {
                obj->pos[1] -= 0.3f;
            }
        } else {
            mug->bobFrame = 0;
        }
        break;
    case DRINK_MUG_CIRCLE:
    case DRINK_MUG_RETURN:
        if (!(mug->animFrame++ < 5.0f)) {
            mug->frameTimer = 0;
            mug->animFrame = 0;
        } else if (!(mug->frameTimer++ < 1.0f)) {
            mug->frameTimer = 0;
        }
        if (obj->type == DRINK_MUG_RETURN) {
            /* the orbit shrinks by 4 a frame round the hand */
            Mtx_GetTrans((f32 *) &hand, Em_Bone(mug->npc, DRINK_MUG_BONE)->world);
            ofs.x = DRINK_MUG_HAND_OFFSET;
            ofs.y = 0.0f;
            ofs.z = 0.0f;
            Vec_RotateY((f32 *) &ofs, 6.2831855f * (f32)mug->npc->rotY / 65536.0f);
            hand.x += ofs.x;
            hand.y += ofs.y;
            hand.z += ofs.z;
            toHand.x = obj->pos[0] - hand.x;
            toHand.y = 0.0f;
            toHand.z = obj->pos[2] - hand.z;
            len = Vec_Length(&toHand.x) - 4.0f;
            Vec_Normalize(&toHand.x);
            toHand.x *= len;
            toHand.y = 0.0f;
            toHand.z *= len;
            Vec_Copy(&mug->orbit.x, &toHand.x);
            if (len <= 4.0f) {
                mug->atHand = 1;
            }
        }
        if (obj->type == DRINK_MUG_RETURN) {
            if (mug->atHand == 1) {
                Mtx_GetTrans((f32 *) &hand, Em_Bone(mug->npc, DRINK_MUG_BONE)->world);
                ofs.x = DRINK_MUG_HAND_OFFSET;
                ofs.y = 0.0f;
                ofs.z = 0.0f;
                Vec_RotateY((f32 *) &ofs, 6.2831855f * (f32)mug->npc->rotY / 65536.0f);
                hand.x += ofs.x;
                hand.y += ofs.y;
                hand.z += ofs.z;
                obj->pos[0] = hand.x;
                obj->pos[2] = hand.z;
                obj->type = DRINK_MUG_HOVER;
                mug->frameTimer = 0;
                mug->animFrame = 0;
                mug->bobFrame = 0;
            } else {
                Vec_RotateY((f32 *) &mug->orbit, -0.049087387f); /* pi / 64 a frame */
                obj->pos[0] = hand.x + mug->orbit.x;
                obj->pos[1] -= 3.0f;
                obj->pos[2] = hand.z + mug->orbit.z;
            }
        } else {
            Vec_RotateY((f32 *) &mug->orbit, -0.049087387f); /* pi / 64 a frame */
            obj->pos[0] = mug->npc->posX + mug->orbit.x;
            obj->pos[2] = mug->npc->posZ + mug->orbit.z;
        }
        if (mug->bobFrame++ < 6.0f) {
            if (mug->bobFrame <= 3.0f) {
                obj->pos[1] += 0.75f;
            } else {
                obj->pos[1] -= 0.75f;
            }
        } else {
            mug->bobFrame = 0;
        }
        break;
    case DRINK_MUG_TURN:
        if (mug->turnFrames-- > 0) {
            mug->heading += 0.12271847f * (f32)mug->turnDir; /* pi / 25.6 a frame */
            if (mug->hopFrame >= 8.0f) {
                mug->hopFrame = 0;
            }
            if (mug->hopFrame++ < 8.0f) {
                if (mug->hopFrame <= 4.0f) {
                    obj->pos[1] += 2.0f;
                } else {
                    obj->pos[1] -= 2.0f;
                }
            }
        } else {
            Em_BonePos(mug->npc, DRINK_MUG_BONE, (f32 *)&hand);
            obj->pos[1] = hand.y;
            obj->type = DRINK_MUG_REST;
            obj->timer = 20;
        }
        break;
    case DRINK_MUG_REST:
        if (obj->timer-- <= 0) {
            obj->type = DRINK_MUG_RESET;
        }
        break;
    case DRINK_MUG_RESET:
        obj->type = DRINK_MUG_HELD;
        obj->var0 = DRINK_MUG_WAIT;
        obj->var1 = 0;
        obj->timer = 0;
        mug->idleWait = Rand_Next(1) % 10 + 5;
        mug->swivelFrames = 0;
        mug->swivelOffset = 0;
        mug->yaw = mug->swivelOffset + mug->npc->rotY;
        mug->atHand = 0;
        mug->returning = 0;
        mug->hopFrame = 0;
        mug->bobFrame = 0;
        mug->bobRise = 0;
        mug->turnFrames = 0;
        mug->turnDir = 0;
        mug->speed = -11.77518f;
        mug->curveRate = 65535.0f * mug->speed / 885.48004f;
        mug->speed = mug->speed * -1.0f;
        break;
    case DRINK_MUG_SETTLE:
        Em_BonePos(mug->npc, DRINK_MUG_BONE, (f32 *)&hand);
        if (!(mug->animFrame++ < 9.0f)) {
            /* the turn: 8 frames for each pi / 3.2 of it, rounded up */
            obj->type = DRINK_MUG_TURN;
            diff = 6.2831855f * (f32)mug->npc->rotY / 65536.0f - mug->heading;
            mug->turnFrames = Math_Ceil(Math_Absf(diff) / 0.98174775f);
            mug->turnFrames *= 8;
            if (diff < 0.0f) {
                mug->turnDir = -1;
            } else if (diff > 0.0f) {
                mug->turnDir = 1;
            } else {
                mug->turnDir = 0;
            }
            obj->pos[1] = hand.y;
        } else if (!(mug->frameTimer++ < 2.0f)) {
            mug->frameTimer = 0;
        }
        break;
    }
    if (obj->node != NULL) {
        Vec_Copy(obj->node->pos, &obj->pos[0]);
        OT_AddByDepth(gDrawOtSorted, obj->node, DRAW_OT_SORTED_LEN, 0);
    }
}

/* The end state: moves on to LOBBY_OBJ_FREE and frees the draw slot. */
void DrinkMug_End(StageObj *obj)
{
    obj->state++;
    if (obj->node != NULL) {
        DrawSlot_Free(obj->slot);
    }
}

/* The free state: frees the object. */
void DrinkMug_Free(StageObj *obj)
{
    StageObj_Free(obj);
}

u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);

/*
 * The mug's draw (its draw slot's callback): a material set of the common
 * model by the phase, at scale 0.7. The phases that animate show one material
 * of the set, picked by animFrame (lifting and settling: 1 + animFrame / 2),
 * and hide the others. The yaw is the held mug's yaw, the townsperson's
 * heading while it lifts, the direction it travels while it flies (kept in
 * heading; tipped back on the flight out), else heading. Draws nothing until
 * the model is loaded, or while the mug is hidden.
 */
void DrinkMug_Draw(DrawSlot *slot)
{
    StageObj *obj = slot->owner;
    Model *mdl = gCommonModels[0];
    DrinkMugWork *mug = (DrinkMugWork *)obj->buf;
    ModelMat *mats;
    ModelMatSet *matSet;
    f32 mtx[16];
    f32 scale[3];
    f32 rot[3];
    u8 shown;
    u8 i;

    if (mdl != NULL && mdl->active != 0 && obj->visible != 0) {
        mats = mdl->mats;
        switch (obj->type) {
        case DRINK_MUG_HELD:
        case DRINK_MUG_RESET:
            matSet = &mdl->matSets[DRINK_MUG_MATSET_HELD];
            rot[0] = 0.0f;
            rot[1] = 6.2831855f * (f32)mug->yaw / 65536.0f;
            rot[2] = 0.0f;
            break;
        case DRINK_MUG_LIFT:
        case DRINK_MUG_SETTLE:
            matSet = &mdl->matSets[DRINK_MUG_MATSET_LIFT];
            rot[0] = 0.0f;
            if (obj->type == DRINK_MUG_LIFT) {
                rot[1] = 6.2831855f * (f32)mug->npc->rotY / 65536.0f;
            } else {
                rot[1] = mug->heading;
            }
            rot[2] = 0.0f;
            shown = 1.0f + mug->animFrame / 2.0f;
            for (i = 1; i < matSet->count; i++) {
                ModelMat *mat = &mats[matSet->mat[i]];

                if (i == shown) {
                    mat->alpha = 1.0f;
                } else {
                    mat->alpha = 0.0f;
                }
            }
            break;
        case DRINK_MUG_FLY_OUT:
        case DRINK_MUG_CIRCLE:
        case DRINK_MUG_RETURN: {
            f32 dx = obj->pos[0] - mug->lastPos.x;
            f32 dy = obj->pos[1] - mug->lastPos.y;
            f32 dz = obj->pos[2] - mug->lastPos.z;

            matSet = &mdl->matSets[DRINK_MUG_MATSET_FLY];
            if (obj->type == DRINK_MUG_FLY_OUT) {
                rot[0] = -0.5f;
            } else {
                rot[0] = 0.0f;
            }
            if (dx != 0.0f || dy != 0.0f || dz != 0.0f) {
                mug->heading = 6.2831855f * (f32)(Math_Heading(dx, dz, 0.0f, 0.0f) + 0x4000) / 65536.0f;
            }
            rot[1] = mug->heading;
            rot[2] = 0.0f;
            shown = (f32)mug->animFrame;
            for (i = 1; i < matSet->count; i++) {
                ModelMat *mat = &mats[matSet->mat[i]];

                if (i == shown) {
                    mat->alpha = 1.0f;
                } else {
                    mat->alpha = 0.0f;
                }
            }
            break;
        }
        case DRINK_MUG_HOVER:
            matSet = &mdl->matSets[DRINK_MUG_MATSET_HOVER];
            rot[0] = 0.0f;
            rot[1] = mug->heading;
            rot[2] = 0.0f;
            shown = (f32)mug->animFrame;
            for (i = 1; i < matSet->count; i++) {
                ModelMat *mat = &mats[matSet->mat[i]];

                if (i == shown) {
                    mat->alpha = 1.0f;
                } else {
                    mat->alpha = 0.0f;
                }
            }
            break;
        case DRINK_MUG_TURN:
        case DRINK_MUG_REST:
            matSet = &mdl->matSets[DRINK_MUG_MATSET_HELD];
            rot[0] = 0.0f;
            rot[1] = mug->heading;
            rot[2] = 0.0f;
            break;
        }
        scale[0] = 0.7f;
        scale[1] = 0.7f;
        scale[2] = 0.7f;
        Mtx_FromScaleRotTrans((f32 *) scale, (f32 *) rot, slot->pos, 4, (f32 *) mtx);
        Effect_DrawModel(matSet, mtx, -1, 0, mats);
    }
}

/*
 * Gives a townsperson its mug: an object of one block, kind
 * LOBBY_OBJ_DRINK_MUG, held. Nothing when the pool is full. The
 * townspeople's set-up calls it for the drinking townspeople.
 */
void DrinkMug_Spawn(Em *npc)
{
    StageObj *obj = (StageObj *)StageObj_Alloc(1);

    if (obj != NULL) {
        obj->kind = LOBBY_OBJ_DRINK_MUG;
        obj->type = DRINK_MUG_HELD;
        obj->update = DrinkMug_Update;
        ((DrinkMugWork *)obj->buf)->npc = npc;
    }
}

StageObj *StageObj_FindKind(s32 kind);

/*
 * Returns 1 when the mug is back in the hand (within 5 on each axis of the
 * point DRINK_MUG_HAND_OFFSET ahead of the hand) or when its backTimeout has
 * run out, which each call counts down; else 0. With no mug running only the
 * timeout counts. The contest's menu waits on it once the contest is over.
 * The mug's work is read before the object is tested for NULL, as the game's
 * code does.
 */
s32 DrinkMug_IsBack(void)
{
    StageObj *obj = StageObj_FindKind(LOBBY_OBJ_DRINK_MUG);
    DrinkMugWork *mug = (DrinkMugWork *)obj->buf;
    Vec3f pos;
    Vec3f ofs;

    if (obj == NULL || obj->active != 1) {
        if (--mug->backTimeout < 0) {
            return 1;
        }
    } else {
        Em_BonePos(mug->npc, DRINK_MUG_BONE, (f32 *)&pos);
        ofs.x = DRINK_MUG_HAND_OFFSET;
        ofs.y = 0.0f;
        ofs.z = 0.0f;
        Vec_RotateY((f32 *) &ofs, 6.2831855f * (f32)mug->npc->rotY / 65536.0f);
        pos.x += ofs.x;
        pos.y += ofs.y;
        pos.z += ofs.z;
        if (--mug->backTimeout < 0 ||
            (pos.x - 5.0f <= obj->pos[0] && !(5.0f + pos.x < obj->pos[0]) &&
             pos.y - 5.0f <= obj->pos[1] && !(5.0f + pos.y < obj->pos[1]) &&
             pos.z - 5.0f <= obj->pos[2] && !(5.0f + pos.z < obj->pos[2]))) {
            return 1;
        }
    }
    return 0;
}

/* Returns the first object of the stage object pool (gStageObjs, 0x80
 * objects) whose kind is kind, or NULL. */
StageObj *StageObj_FindKind(s32 kind)
{
    StageObj *obj;
    s32 i;

    obj = gStageObjs;
    i = 0;
    for (;;) {
        if (obj->kind == (kind & 0xFF)) {
            return obj;
        }
        i = (i + 1) & 0xFF;
        obj++;
        if (i >= 0x80) {
            return NULL;
        }
    }
}
