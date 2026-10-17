#include "common.h"
#include "syssettings.h"
#include "quest.h"
#include "scene.h"
#include "camera.h"
#include "em.h"
#include "pad.h"
#include "player.h"
#include "objhead.h"
#include "capsule.h"
#include "model.h"
#include "area.h"

/* prototypes */
u32 Player_TestFlag(void *obj, u32 flag);
void Vec_Copy(f32 *dst, f32 *src);
f32 Math_Pow(f32 x, f32 y);
void Cam_CopyDefaults(CamDistSet *dst);
s32 Player_LocalActionInCamRange_Unk(void);
s32 CamArea_IsEnabled(CamArea *area);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void Vec_Scale(f32 *, f32 *, f32);
void Vec_Transform(f32 *out, f32 *v, f32 *m);
s32 Hud_InputBlocked(void);
s32 Quest_PhaseIsOver(s32 a);
/* end of prototypes */

/* A complex number (the root finder Quintic_Roots). */
typedef struct Vec2f {
    f32 x;
    f32 y;
} Vec2f;

s32 PathPos_MoveToward(PathPos *pos, PathData *d, PathSeg *segs, f32 *pt);
s32 PathPos_PlaceNear(PathPos *pos, PathData *d, PathSeg *segs, f32 *pt);
s32 Path_NearestSeg(PathData *d, f32 *pt);
s32 PathPos_Build(PathPos *pos, PathSeg *segs, PathData *d, f32 *pt);
void PathPos_MoveOnSeg(PathPos *pos, PathData *d, s32 k, f32 dist);

extern u32 gDmcamFileIds[];
u8 *Dmcam_Find(s32 idx);
s32 CamScript_Step(CamCtl *cam, CamView *obj, CamScript *w);

/* Starts (step 0: the script w->id, cam's view copied into obj and w) and runs
 * (step 1, CamScript_Step) obj's camera script: 1 once the script step reports done (or when
 * it may be skipped, obj->flags bit 1, and Start is pressed), else CamScript_Step's result (the
 * game's code falls off the end with it in v0; for a state above 2, r is unset: the game
 * returns whatever v0 held on entry). */
s32 CamScript_Run(CamCtl *cam, CamView *obj)
{
    CamScript *w = &cam->script;
    s32 r;

    if ((obj->flags & 2) && (cam->padPressed & 0x4000)) {
        return 1;
    }
    switch (obj->blend) {
    case 0:
        obj->blend++;
        w->pc = (s8 *)gDmcamFileIds[w->id];
        if ((u32)w->pc < 0x80) {
            w->pc = (s8 *)Dmcam_Find(w->id);
        }
        w->mark = w->pc;
        Vec_Copy(obj->eye, cam->out.eye);
        Vec_Copy(obj->at, cam->out.at);
        obj->roll = cam->out.roll;
        obj->fov = cam->out.fov;
        Vec_Copy(w->eye[0], obj->eye);
        Vec_Copy(w->at[0], obj->at);
        w->roll[0] = RAD_TO_ANGLE16 * obj->roll;
        w->fov[0] = RAD_TO_ANGLE16 * obj->fov;
        w->swing[3].speed = 0;
        w->swing[2].speed = 0;
        w->swing[1].speed = 0;
        w->swing[0].speed = 0;
        *(s32 *)w->swingRand = 0;
        w->ease = 0;
        *(s32 *)&w->easePow = 0;
        obj->length = -1;
        obj->left = -1;
    case 1:
        if ((r = CamScript_Step(cam, obj, w)) <= 0) {
            break;
        }
        obj->blend++;
    case 2:
        return 1;
    }
    return r;
}

static void *CamScript_ObjMtx(CamView *v, CamScript *w)
{
    Em *obj;

    obj = w->obj;
    if ((obj != NULL) && (obj->active != 0)) {
        return obj->worldMtx;
    }
    v->flags |= 1;
    return NULL;
}


/* An eye point of the script (command 2): src[1..3] (fixed point, 4096 = 1) moved into the space
 * w->eyeSpace names: the local player's body (0), position (1), part w->eyePart (2), the request
 * object's matrix (3, CamScript_ObjMtx) or the target at[0] (5). */
void CamScript_EyePoint(f32 *out, CamView *v, CamScript *w, s32 *src)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    void *m;

    out[0] = 0.00024414062f * src[1];
    out[1] = 0.00024414062f * src[2];
    out[2] = 0.00024414062f * src[3];
    switch (w->eyeSpace) {
    case 2:
        Vec_Transform(out, out, pl->partRecs[w->eyePart]->m1);
        break;
    case 0:
        Vec_Transform(out, out, pl->worldMtx[0]);
        break;
    case 1:
        Vec_Add(out, out, pl->pos);
        break;
    case 3:
        m = CamScript_ObjMtx(v, w);
        if (m != NULL) {
            Vec_Transform(out, out, m);
        }
        break;
    case 4:
        break;
    case 5:
        Vec_Add(out, out, w->at[0]);
        break;
    }
}

/* A target point of the script (command 4): as CamScript_EyePoint in w->atSpace with w->atPart (no
 * space 5). */
void CamScript_AtPoint(f32 *out, CamView *v, CamScript *w, s32 *src)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    void *m;

    out[0] = 0.00024414062f * src[1];
    out[1] = 0.00024414062f * src[2];
    out[2] = 0.00024414062f * src[3];
    switch (w->atSpace) {
    case 2:
        Vec_Transform(out, out, pl->partRecs[w->atPart]->m1);
        break;
    case 0:
        Vec_Transform(out, out, pl->worldMtx[0]);
        break;
    case 1:
        Vec_Add(out, out, pl->pos);
        break;
    case 3:
        m = CamScript_ObjMtx(v, w);
        if (m != NULL) {
            Vec_Transform(out, out, m);
        }
        break;
    case 4:
        break;
    }
}

void Vec_Copy(f32 *dst, f32 *src);

void CamScript_CopyToEnd(CamScript *w, s32 mask)
{
    if (mask & 1) {
        Vec_Copy(w->eye[1], w->eye[0]);
    }
    if (mask & 2) {
        Vec_Copy(w->at[1], w->at[0]);
    }
    if (mask & 4) {
        w->roll[1] = w->roll[0];
    }
    if (mask & 8) {
        w->fov[1] = w->fov[0];
    }
    if (mask & 0x10) {
        w->swing[0].amp[1] = w->swing[0].amp[0];
    }
    if (mask & 0x20) {
        w->swing[1].amp[1] = w->swing[1].amp[0];
    }
    if (mask & 0x40) {
        w->swing[2].amp[1] = w->swing[2].amp[0];
    }
    if (mask & 0x80) {
        w->swing[3].amp[1] = w->swing[3].amp[0];
    }
}

/* The camera script's orbit angles (CamScript.angle) at the script view's time: eased from
 * [0] to [1] by left / length (modes[5]); [0] when there is no move (length not positive). */
void CamScript_OrbitAngles(s16 *out, CamCtl *c)
{
    CamView *v = &c->modes[5];
    CamScript *w = &c->script;
    s16 *t;

    if (c->modes[5].length > 0) {
        out[0] = w->angle[0][0] - w->angle[1][0];
        t = &v->left;
        out[0] = w->angle[1][0] + out[0] * t[0] / t[1];
        out[1] = w->angle[0][1] - w->angle[1][1];
        out[1] = w->angle[1][1] + out[1] * t[0] / t[1];
    } else {
        out[0] = w->angle[0][0];
        out[1] = w->angle[0][1];
    }
}

void CamScript_OrbitAngles(s16 *out, CamCtl *c);
void Mtx_Identity(f32 *m);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Vec_Lerp(f32 *, f32 *, f32 *, f32 t);
void Vec_RotateSelf(f32 *v, f32 *m);
void Vec_Normalize(f32 *v);
void Vec_Cross(f32 *out, f32 *a, f32 *b);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
f32 Math_CosAngle(s32 a);
u16 Rand_Next(s32 stream);
/* unit's own declaration: wider than the definition's u8 (unk_328CD0.c): with u8 CamScript_Place masks the value before its == 1 (6 of 534 words differ) */
s32 AreaHit_Raycast(f32 *from, f32 *to, f32 *out, u16 mask);
f32 AreaGrid2_GetFloorY(f32 *pos);
void Quat_Set(f32 *axis, f32 *q, f32 angle);
void Quat_ToMatrix(f32 *q, f32 (*m)[4]);

/* Places view obj for the camera script w (CamScript_Step) at the eased time obj->left of
 * obj->length: eye and target eased between w's pairs (or, w->place 0, one of them orbiting the
 * other by w->angle at w->dist), roll and fov eased, then the swings and sways. */
void CamScript_Place(CamCtl *cam, CamView *obj, CamScript *w)
{
    f32 mtx[16];
    f32 mtx2[16];
    f32 q[4];
    f32 off[3];
    f32 sway[3];
    f32 hit[3];
    f32 side[3];
    f32 up[3];
    f32 dir[3];
    s16 ang[2];
    Player *pl;
    void *m;
    f32 t;
    f32 it;
    f32 a;
    f32 y;
    u8 flags;

    if (obj->length > 0) {
        t = (f32)obj->left / (f32)obj->length;
        switch (w->ease) {
        case 1:
            t = Math_Pow(t, w->easePow);
            break;
        case 2:
            t = 1.0f - Math_Pow(1.0f - t, w->easePow);
            break;
        case 3:
            if (t < 0.5f) {
                t = 0.5f * Math_Pow(2.0f * t, w->easePow);
            } else {
                t = 1.0f - 0.5f * Math_Pow(2.0f * (1.0f - t), w->easePow);
            }
            break;
        }
    } else {
        t = 1.0f;
    }
    it = 1.0f - t;
    switch (w->place) {
    case 0:
        CamScript_OrbitAngles(ang, cam);
        Mtx_Identity(mtx);
        Mtx_RotateXYZ(mtx, ANGLE16_TO_RAD * ang[0], ANGLE16_TO_RAD * ang[1], 0.0f);
        off[0] = 0.0f;
        off[1] = 0.0f;
        off[2] = 500.0f;
        pl = &gPlayers[gScene.localPlayer];
        if (w->pivotEye == 1) {
            Vec_Lerp(obj->eye, w->eye[0], w->eye[1], t);
            Vec_RotateSelf(off, (f32 *)mtx);
            switch (w->eyeSpace) {
            case 2:
                Vec_RotateSelf(off, pl->partRecs[w->atPart]->m1);
                break;
            case 0:
                Vec_RotateSelf(off, (f32 *)pl->worldMtx[0]);
                break;
            case 3:
                m = CamScript_ObjMtx(obj, w);
                if (m != NULL) {
                    Vec_RotateSelf(off, (f32 *)m);
                }
                break;
            case 1:
            case 4:
            case 5:
                break;
            }
            Vec_Add(obj->at, obj->eye, off);
        } else {
            Vec_Lerp(obj->at, w->at[0], w->at[1], t);
            off[2] = w->dist[0] * t + w->dist[1] * it;
            Vec_RotateSelf(off, (f32 *)mtx);
            switch (w->atSpace) {
            case 2:
                Vec_RotateSelf(off, pl->partRecs[w->eyePart]->m1);
                break;
            case 0:
                Vec_RotateSelf(off, (f32 *)pl->worldMtx[0]);
                break;
            case 3:
                m = CamScript_ObjMtx(obj, w);
                if (m != NULL) {
                    Vec_RotateSelf(off, (f32 *)m);
                }
                break;
            case 1:
            case 4:
                break;
            }
            Vec_Add(obj->eye, obj->at, off);
        }
        break;
    case 1:
        Vec_Lerp(obj->at, w->at[0], w->at[1], t);
        Vec_Lerp(obj->eye, w->eye[0], w->eye[1], t);
        break;
    }
    if (obj->flags & 4) {
        if (AreaHit_Raycast(obj->at, obj->eye, hit, 0x8009) == 1) {
            Vec_Copy(obj->eye, hit);
        }
        hit[1] = AreaGrid2_GetFloorY(obj->eye);
        y = 30.0f + hit[1];
        if (!(y <= obj->eye[1])) {
            obj->eye[1] = y;
        }
    }
    obj->roll = ANGLE16_TO_RAD * w->roll[0] * t + ANGLE16_TO_RAD * w->roll[1] * it;
    obj->fov = ANGLE16_TO_RAD * w->fov[0] * t + ANGLE16_TO_RAD * w->fov[1] * it;
    if (w->swing[2].speed != 0) {
        a = (f32)w->swing[2].amp[0] * t + (f32)w->swing[2].amp[1] * it;
        a *= Math_CosAngle(w->swing[2].phase);
        w->swing[2].phase += w->swing[2].speed;
        if (w->swingRand[2] != 0) {
            a += 3.0637257e-5f * (a * w->swingRand[2] * (f32)((Rand_Next(1) & 0xFF) - 0x80));
        }
        obj->roll += ANGLE16_TO_RAD * a;
    }
    if (w->swing[3].speed != 0) {
        a = (f32)w->swing[3].amp[0] * t + (f32)w->swing[3].amp[1] * it;
        a *= Math_CosAngle(w->swing[3].phase);
        w->swing[3].phase += w->swing[3].speed;
        if (w->swingRand[3] != 0) {
            a += 3.0637257e-5f * (a * w->swingRand[3] * (f32)((Rand_Next(1) & 0xFF) - 0x80));
        }
        obj->fov += ANGLE16_TO_RAD * a;
    }
    sway[1] = 0.0f;
    sway[0] = 0.0f;
    flags = 0;
    if (w->swing[0].speed != 0) {
        a = (f32)w->swing[0].amp[0] * t + (f32)w->swing[0].amp[1] * it;
        sway[0] = a * Math_CosAngle(w->swing[0].phase);
        flags |= 1;
        w->swing[0].phase += w->swing[0].speed;
        if (w->swingRand[0] != 0) {
            Rand_Next(1);
        }
    }
    if (w->swing[1].speed != 0) {
        a = (f32)w->swing[1].amp[0] * t + (f32)w->swing[1].amp[1] * it;
        sway[1] = a * Math_CosAngle(w->swing[1].phase);
        flags |= 1;
        w->swing[1].phase += w->swing[1].speed;
        if (w->swingRand[1] != 0) {
            Rand_Next(1);
        }
    }
    if (flags & 1) {
        Vec_Diff(dir, obj->at, obj->eye);
        up[1] = 1.0f;
        up[2] = 0.0f;
        up[0] = 0.0f;
        Vec_Cross(side, up, dir);
        Vec_Normalize(side);
        Vec_Cross(up, dir, side);
        Vec_Normalize(up);
        Vec_Scale(side, side, sway[0]);
        Vec_Scale(up, up, sway[1]);
        Vec_Add(sway, side, up);
        Quat_Set(dir, q, obj->roll);
        Quat_ToMatrix(q, (f32 (*)[4])mtx2);
        Vec_Rotate(off, sway, mtx2);
        Vec_Add(obj->eye, obj->eye, off);
        Vec_Add(obj->at, obj->at, off);
    }
}

void CamScript_Place(CamCtl *cam, CamView *obj, CamScript *w);

/* Runs obj's camera script (w = the controller's script work, w->pc the next command): each
 * command is op[0] (the kind), op[1] (its length in words) and arguments from op + 2. Kinds 0-35
 * set the camera's parameters; 36-38 loop (36 marks the start, 37 and 38 run CamScript_Place and go
 * back); 40 ends with -1, 41-43 flag and branch. Returns 1 when obj->flags bit 0 gets set or on an
 * unknown kind, else the loop's result. */
s32 CamScript_Step(CamCtl *cam, CamView *obj, CamScript *w)
{
    s8 *next;
    s32 run;
    s8 *op;
    s32 ret;
    s32 *arg;

    run = 1;
    obj->flags &= 0xFE;
    next = w->pc;
    do {
        op = next;
        arg = (s32 *)next;
        next += op[1] * 4;
        switch (op[0]) {
        case 0:
            w->place = op[2];
            break;
        case 1:
            w->eyeSpace = op[2];
            w->eyePart = op[3];
            break;
        case 2:
            CamScript_EyePoint(w->eye[op[2]], obj, w, arg);
            break;
        case 3:
            w->atSpace = op[2];
            w->atPart = op[3];
            break;
        case 4:
            CamScript_AtPoint(w->at[op[2]], obj, w, arg);
            break;
        case 5:
            w->pivotEye = op[2];
            break;
        case 6:
            w->angle[0][0] = *(s16 *)(op + 2);
            break;
        case 7:
            w->angle[1][0] = *(s16 *)(op + 2);
            break;
        case 8:
            w->angle[0][1] = *(s16 *)(op + 2);
            break;
        case 9:
            w->angle[1][1] = *(s16 *)(op + 2);
            break;
        case 10:
            w->dist[0] = 0.0625f * arg[1];
            break;
        case 11:
            w->dist[1] = 0.0625f * arg[1];
            break;
        case 12:
            w->roll[0] = *(s16 *)(op + 2);
            break;
        case 13:
            w->roll[1] = *(s16 *)(op + 2);
            break;
        case 14:
            w->fov[0] = *(s16 *)(op + 2);
            break;
        case 15:
            w->fov[1] = *(s16 *)(op + 2);
            break;
        case 16:
            obj->left = obj->length = *(s16 *)(op + 2);
            break;
        case 17:
            w->swing[0].phase = *(s16 *)(op + 2);
            break;
        case 18:
            w->swing[0].speed = *(s16 *)(op + 2);
            break;
        case 19:
            w->swing[0].amp[0] = *(s16 *)(op + 2);
            break;
        case 20:
            w->swing[0].amp[1] = *(s16 *)(op + 2);
            break;
        case 21:
            w->swing[1].phase = *(s16 *)(op + 2);
            break;
        case 22:
            w->swing[1].speed = *(s16 *)(op + 2);
            break;
        case 23:
            w->swing[1].amp[0] = *(s16 *)(op + 2);
            break;
        case 24:
            w->swing[1].amp[1] = *(s16 *)(op + 2);
            break;
        case 25:
            w->swing[2].phase = *(s16 *)(op + 2);
            break;
        case 26:
            w->swing[2].speed = *(s16 *)(op + 2);
            break;
        case 27:
            w->swing[2].amp[0] = *(s16 *)(op + 2);
            break;
        case 28:
            w->swing[2].amp[1] = *(s16 *)(op + 2);
            break;
        case 29:
            w->swing[3].phase = *(s16 *)(op + 2);
            break;
        case 30:
            w->swing[3].speed = *(s16 *)(op + 2);
            break;
        case 31:
            w->swing[3].amp[0] = *(s16 *)(op + 2);
            break;
        case 32:
            w->swing[3].amp[1] = *(s16 *)(op + 2);
            break;
        case 33:
            w->swingRand[op[2]] = op[3];
            break;
        case 34:
            CamScript_CopyToEnd(w, op[2]);
            break;
        case 35:
            w->ease = (*(s16 *)(op + 2) >> 14) & 3;
            w->easePow = 0.0009765625f * (*(s16 *)(op + 2) & 0x3FFF);
            break;
        case 36:
            obj->left = obj->length = *(s16 *)(op + 2);
            w->loop = next;
            break;
        case 37:
            if (op[2] != 0) {
                CamScript_Place(cam, obj, w);
            }
            (obj->left)--;
            if (obj->length <= 0 || obj->left >= 0) {
                next = w->loop;
                run = 0;
                ret = 0;
            } else {
                Vec_Copy(w->eye[0], obj->eye);
                Vec_Copy(w->at[0], obj->at);
                w->angle[0][0] = w->angle[1][0];
                w->angle[0][1] = w->angle[1][1];
                w->roll[0] = w->roll[1];
                w->fov[0] = w->fov[1];
                w->swing[0].speed = 0;
                w->swing[1].speed = 0;
                w->swing[2].speed = 0;
                w->swing[3].speed = 0;
                w->swing[0].amp[0] = w->swing[0].amp[1];
                w->swing[1].amp[0] = w->swing[1].amp[1];
                w->swing[2].amp[0] = w->swing[2].amp[1];
                w->swing[3].amp[0] = w->swing[3].amp[1];
                w->ease = 0;
                w->easePow = 0.0f;
                obj->length = obj->left = -1;
            }
            break;
        case 38:
            CamScript_Place(cam, obj, w);
            if (op[2] != 0) {
                next = w->loop;
                run = 0;
                ret = 0;
            }
            break;
        case 41:
            obj->flags |= 4;
            break;
        case 43:
            w->mark = next;
            break;
        case 42:
            if (w->mark != NULL) {
                next = w->mark;
            }
            break;
        default:
            return 1;
        case 40:
            run = 0;
            ret = -1;
            break;
        }
        if (obj->flags & 1) {
            return 1;
        }
        w->pc = next;
    } while (run);
    return ret;
}

void Camera_SetRoll(f32 roll);
void Camera_StoreFov(f32 fov);
void CamFollow_SetYawFrom(s32 v);
void CamShake_Apply(CamShake *t);

/* Ticks the controller's three shakes, copies the current mode's view (c->areaMode) to
 * c->out and hands gCamera the view that overrides it (5, 3 or 4 when that view is active;
 * c->override, -1: none, then c->out). */
void CamCtl_RunMode(CamCtl *c)
{
    CamView *v;
    CamShake *t;
    s32 i;

    t = &c->shakeA;
    i = 3;
    do {
        if (t->active != 0) {
            if (--t->length <= 0) {
                t->active = 0;
            }
        }
        i--;
        t++;
    } while (i > 0);
    v = &c->modes[c->areaMode];
    Vec_Copy(c->out.eye, v->eye);
    Vec_Copy(c->out.at, v->at);
    c->out.roll = v->roll;
    c->out.fov = v->fov;
    c->override = -1;
    if (c->modes[5].active != 0) {
        c->override = 5;
    } else if (c->modes[3].active != 0) {
        c->override = 3;
    } else if (c->modes[4].active != 0) {
        c->override = 4;
    }
    if (c->override >= 0) {
        v = &c->modes[c->override];
        Vec_Copy(gCamera->eye, v->eye);
        Vec_Copy(gCamera->at, v->at);
        if (c->override == 5) {
            CamShake_Apply(&c->shakeC);
        } else if (c->override == 3) {
            CamShake_Apply(&c->shakeB);
        }
        Camera_SetRoll(v->roll);
        Camera_StoreFov(v->fov);
        c->followView = 0;
    } else {
        Vec_Copy(gCamera->eye, c->out.eye);
        Vec_Copy(gCamera->at, c->out.at);
        CamShake_Apply(&c->shakeA);
        Camera_SetRoll(c->out.roll);
        Camera_StoreFov(c->out.fov);
    }
    CamFollow_SetYawFrom(c->override);
}

s32 CamCtl_IsRailMode(void)
{
    return gCamCtl.areaMode == 2;
}


s32 CamCtl_IsAiming(Player *pl)
{
    CamCtl *c = (CamCtl *)&gCamCtl;

    if (gCamCtl.modes[5].active != 0) {
        return 0;
    }
    if (pl->aimLockTimer != 0) {
        return 1;
    }
    return c->modes[3].active != 0;
}


/* *out = gCamCtl.playerFade and 1 when gCamCtl.followView is set; else 1.0 and 0. */
s32 CamCtl_GetPlayerFade(f32 *out)
{
    CamCtl *c = (CamCtl *)&gCamCtl;

    if (gCamCtl.followView == 0) {
        *out = 1.0f;
        return 0;
    }
    *out = c->playerFade;
    return 1;
}

extern f32 gCamMatrix[];
void Frustum_Build(f32 *out, void *m, f32 fov, f32 k);
s32 Frustum_ClipSegment(f32 *a, f32 *b);
void Cam_GetEmBoneSegment(f32 *v, s32 em);

/* gCamCtl.emInView: whether the segment between two bones of the monster
 * at gCamCtl.player (Cam_GetEmBoneSegment) is in view (Frustum_Build, Frustum_ClipSegment); when it
 * is, playerFade = 0.3. */
void CamCtl_CheckEmInView(void)
{
    CamCtl *c = (CamCtl *)&gCamCtl;
    f32 frustum[16];
    f32 seg[6];

    Frustum_Build(frustum, gCamMatrix, gCamera->fov, gCamCtl.emInView == 0 ? 45.0f : 50.0f);
    Cam_GetEmBoneSegment(seg, (s32)c->player);
    if (Frustum_ClipSegment(frustum, seg) == 1) {
        c->playerFade = 0.3f;
        c->emInView = 1;
    } else {
        c->emInView = 0;
    }
}

EmBone * Em_GetBone(void *obj, s16 bone);
void Vec_Lerp(f32 *, f32 *, f32 *, f32 t);

void Cam_GetEmBoneSegment(f32 *v, s32 em)
{
    f32 *w;

    Vec_Copy(v, (f32 *)((u8 *)Em_GetBone((Em *)em, 10) + 0x30));
    Vec_Copy(v + 3, (f32 *)((u8 *)Em_GetBone((Em *)em, 2) + 0x30));
    Vec_Lerp((f32 *)v, (f32 *)v, (f32 *)(v + 3), 2.0f);
    w = v + 3;
    Vec_Lerp((f32 *)w, (f32 *)w, (f32 *)v, 1.0f);
}

void Math_GetSinCos(f32 *s, f32 *c, f32 a);
void Vec4_TransformSelf(f32 *v, f32 *m);
void Vec_RotateSelf(f32 *v, f32 *m);
void Vec_Normalize(f32 *v);

/*
 * The view pyramid of matrix m for a field of view fov (radians): out[0..2]
 * its apex (1.25 k / tan(fov / 2) behind the origin, through m), out[3..14]
 * the normals of its four sides (a 640 x 480 screen), rotated by m.
 */
void Frustum_Build(f32 *out, void *m, f32 fov, f32 k)
{
    f32 s;
    f32 c;
    f32 v[4];
    f32 t;
    f32 w;
    f32 hw = 320.0f;
    f32 hh = 240.0f;

    Math_GetSinCos(&s, &c, 0.5f * fov);
    t = c / s;
    w = hw * t;
    v[0] = v[1] = 0.0f;
    v[2] = -(1.25f * k * t);
    v[3] = 1.0f;
    Vec4_TransformSelf(v, m);
    out[0] = v[0];
    out[1] = v[1];
    out[2] = v[2];
    out[3] = w;
    out[4] = 0.0f;
    out[5] = -hw;
    out[6] = -w;
    out[7] = 0.0f;
    out[8] = -hw;
    out[9] = 0.0f;
    out[10] = w;
    out[11] = -hh;
    out[12] = 0.0f;
    out[13] = -w;
    out[14] = -hh;
    Vec_RotateSelf(out + 3, (f32 *)m);
    Vec_RotateSelf(out + 6, (f32 *)m);
    Vec_RotateSelf(out + 9, (f32 *)m);
    Vec_RotateSelf(out + 12, (f32 *)m);
    Vec_Normalize(out + 3);
    Vec_Normalize(out + 6);
    Vec_Normalize(out + 9);
    Vec_Normalize(out + 12);
}

/*
 * Clips the segment seg (two points) to the view pyramid f (Frustum_Build):
 * 0 when it lies wholly outside one of the four sides, else 1 with the
 * outside ends moved onto the sides.
 */
s32 Frustum_ClipSegment(f32 *f, f32 *seg)
{
    f32 *n = f + 3;
    s32 i;

    for (i = 4; i != 0; i--) {
        s32 in = 0;
        f32 d0 = n[0] * (f[0] - seg[0]) + n[1] * (f[1] - seg[1]) + n[2] * (f[2] - seg[2]);
        f32 d1;
        f32 dx;
        f32 dy;
        f32 inv;
        f32 dz;

        if (d0 >= 0.0f) {
            in |= 1;
        }
        d1 = n[0] * (f[0] - seg[3]) + n[1] * (f[1] - seg[4]) + n[2] * (f[2] - seg[5]);
        if (d1 >= 0.0f) {
            in |= 2;
        }
        switch (in) {
        case 0:
            return 0;
        case 1:
            dx = seg[3] - seg[0];
            dy = seg[4] - seg[1];
            dz = seg[5] - seg[2];
            inv = 1.0f / (dx * n[0] + dy * n[1] + dz * n[2]);
            seg[3] += inv * (dx * d1);
            seg[4] += inv * (dy * d1);
            seg[5] += inv * (dz * d1);
            break;
        case 2:
            dx = seg[0] - seg[3];
            dy = seg[1] - seg[4];
            dz = seg[2] - seg[5];
            inv = 1.0f / (dx * n[0] + dy * n[1] + dz * n[2]);
            seg[0] += inv * (dx * d0);
            seg[1] += inv * (dy * d0);
            seg[2] += inv * (dz * d0);
            break;
        }
        n += 3;
    }
    return 1;
}

void Vec_Diff(f32 *out, f32 *a, f32 *b);
f32 Vec_Dot(f32 *a, f32 *b);
s32 Cam_GetEmLowPoint(f32 *out, Em *em, u8 *plane);
void Camera_ComputeUp(f32 *up, f32 *eye, f32 *at, f32 roll);
void Mtx_LookAt(f32 *m, f32 *eye, f32 *at, f32 *up);
void Mtx_Inverse(f32 *out, f32 *m);
void Vec_Cross(f32 *out, f32 *a, f32 *b);
void Mtx_Identity(f32 *m);
void Mtx_FromAxisAngle(f32 *m, f32 *axis, f32 angle);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
f32 Math_Atan2(f32 y, f32 x);

/* Turns view v (eye, target +0xC, up +0x18, direction +0x24, roll +0x60, fov +0x64) so that the
 * point Cam_GetEmLowPoint finds for the controller's object (c + 0x384) is not below the bottom edge
 * of the screen. */
void CamView_KeepTargetOnScreen(CamCtl *c, CamView *v)
{
    f32 look[16];
    f32 inv[16];
    f32 target[3];
    f32 eye[3];
    f32 e1[3];
    f32 e2[3];
    f32 axis[3];
    f32 d[3];
    f32 s;
    f32 co;
    f32 h;
    f32 a;
    f32 b;

    Vec_Diff(v->dir, v->at, v->eye);
    if (Cam_GetEmLowPoint(target, (Em *)c->player, (u8 *)v) == 1) {
        Camera_ComputeUp(v->up, v->eye, v->at, v->roll);
        Mtx_LookAt(look, v->eye, v->at, v->up);
        Mtx_Inverse(inv, look);
        Math_GetSinCos(&s, &co, 0.5f * v->fov);
        h = 224.0f * (co / s);
        Vec_Copy(eye, v->eye);
        e1[0] = 0.0f;
        e1[1] = -h;
        e1[2] = 224.0f;
        Vec_RotateSelf(e1, (f32 *)inv);
        Vec_Normalize(e1);
        e2[0] = 0.0f;
        e2[1] = -224.0f;
        e2[2] = -h;
        Vec_RotateSelf(e2, (f32 *)inv);
        Vec_Normalize(e2);
        Vec_Diff(d, target, eye);
        a = Vec_Dot(d, e1);
        if (!(a <= 0.0f)) {
            b = Vec_Dot(d, e2);
            Vec_Cross(axis, e2, e1);
            Mtx_Identity(look);
            Mtx_FromAxisAngle(look, axis, Math_Atan2(a, b));
            Vec_Rotate(axis, v->dir, look);
            Vec_Add(v->at, v->eye, axis);
        }
    }
}

EmBone *Em_GetBone(void *obj, s16 bone);
void Vec_Copy(f32 *dst, f32 *src);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
f32 Vec_Dot(f32 *a, f32 *b);
/* The monster's bone 0x14 (else bone 2) position into out; 1 when it is below plane's point and in front of its normal (+0x24). */
s32 Cam_GetEmLowPoint(f32 *out, Em *em, u8 *plane)
{
    f32 d[3];

    Vec_Copy(out, (f32 *)((u8 *)Em_GetBone(em, 0x14) + 0x30));
    if (!(out[1] < ((f32 *)plane)[1])) {
        return 0;
    }
    Vec_Diff(d, out, (f32 *)plane);
    if (!(Vec_Dot((f32 *)(plane + 0x24), d) <= 0.0f)) {
        return 1;
    }
    Vec_Copy(out, (f32 *)((u8 *)Em_GetBone(em, 2) + 0x30));
    Vec_Diff(d, out, (f32 *)plane);
    return Vec_Dot((f32 *)(plane + 0x24), d) > 0.0f;
}

extern s16 gCamShakeLengths[];

void CamShake_SetA(s32 i, f32 *pos)
{
    CamShake *c = &gCamCtl.shakeA;

    c->active = 1;
    c->kind = i;
    c->length = gCamShakeLengths[i];
    c->pos[0] = pos[0];
    c->pos[1] = pos[1];
    c->pos[2] = pos[2];
    c->range = 0;
}

void CamShake_SetAEverywhere(s32 i)
{
    CamShake *c = &gCamCtl.shakeA;

    c->active = 1;
    c->kind = i | 0x80;
    c->length = gCamShakeLengths[i];
}

s32 Obj_InCurrentArea(void *obj);

/* Request i (gCamShakeLengths[i]) of the request slot gCamShakeA at obj's position, when obj is in the current area. */
void CamShake_ReqA(void *obj, s32 i)
{
    CamShake *c = &gCamCtl.shakeA;

    if ((u8)Obj_InCurrentArea(obj)) {
        c->active = 1;
        c->kind = i;
        c->length = gCamShakeLengths[i];
        c->pos[0] = ((ObjHead *)obj)->pos[0];
        c->pos[1] = ((ObjHead *)obj)->pos[1];
        c->pos[2] = ((ObjHead *)obj)->pos[2];
        c->range = 0;
    }
}

/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
u8 Player_InCurrentArea(void *obj);

/* Request i (gCamShakeLengths[i]) of the request slot gCamShakeA at obj's position, when obj is passes Player_InCurrentArea. */
void CamShake_ReqAIf(Em *obj, s32 i)
{
    CamShake *c = &gCamCtl.shakeA;

    if ((u8)(s32)Player_InCurrentArea(obj)) {
        c->active = 1;
        c->kind = i;
        c->length = gCamShakeLengths[i];
        c->pos[0] = obj->posX;
        c->pos[1] = obj->posY;
        c->pos[2] = obj->posZ;
        c->range = 0;
    }
}

/* Request i (gCamShakeLengths[i]) of the request slot gCamShakeA at obj's position, when obj is passes Player_InCurrentArea. */
void CamShake_ReqARange(Em *obj, s32 i, s32 flag)
{
    CamShake *c = &gCamCtl.shakeA;

    if ((u8)(s32)Player_InCurrentArea(obj)) {
        c->active = 1;
        c->kind = i;
        c->length = gCamShakeLengths[i];
        c->pos[0] = obj->posX;
        c->pos[1] = obj->posY;
        c->pos[2] = obj->posZ;
        c->range = flag;
    }
}


/* Request i (gCamShakeLengths[i]) of the request slot gCamShakeC at obj's position, when obj is passes Player_InCurrentArea. */
void CamShake_ReqC(Em *obj, s32 i)
{
    CamShake *c = &gCamCtl.shakeC;

    if ((u8)(s32)Player_InCurrentArea(obj)) {
        c->active = 1;
        c->kind = i;
        c->length = gCamShakeLengths[i];
        c->pos[0] = obj->posX;
        c->pos[1] = obj->posY;
        c->pos[2] = obj->posZ;
        c->range = 0;
    }
}


/* Request i (gCamShakeLengths[i]) of the request slot gCamShakeB at obj's position, when obj is in the current area. */
void CamShake_ReqB(void *obj, s32 i)
{
    CamShake *c = &gCamCtl.shakeB;

    if ((u8)Obj_InCurrentArea(obj)) {
        c->active = 1;
        c->kind = i;
        c->length = gCamShakeLengths[i];
        c->pos[0] = ((ObjHead *)obj)->pos[0];
        c->pos[1] = ((ObjHead *)obj)->pos[1];
        c->pos[2] = ((ObjHead *)obj)->pos[2];
        c->range = 0;
    }
}

f32 Math_Sin(f32 x);
f32 Vec_Distance(f32 *a, f32 *b);
extern f32 gCamShakeRanges[];

/* Applies shake request s (its kind's gCamShakeLengths, and gCamShakeRanges by its range unless
 * felt everywhere): moves gCamera's
 * eye and target up or down by an amplitude that fades with the time and the distance. */
void CamShake_Apply(CamShake *s)
{
    s32 kind;
    f32 a;
    f32 amp;
    f32 k;
    f32 d;

    if (s->active != 0) {
        kind = s->kind & 0x7F;
        a = 90.0f * ((f32)s->length / (f32)gCamShakeLengths[kind]);
        if (kind < 2) {
            amp = 10.0f;
        } else {
            amp = 12.0f;
        }
        amp *= Math_Sin(6.2831855f * a / 360.0f);
        if (s->kind & 0x80) {
            k = 1.0f;
        } else {
            d = Vec_Distance(s->pos, gCamera->eye);
            if (d > gCamShakeRanges[s->range]) {
                k = 0.0f;
            } else {
                k = 1.0f - 0.0005f * d;
            }
        }
        amp *= k;
        if ((s->length >> 1) & 1) {
            amp = -amp;
        }
        gCamera->eye[1] += amp;
        gCamera->at[1] += amp;
    }
}

s32 CamCtl_PadAllowed(void *cam)
{
    if ((gScene.online == 0) && (Quest_PhaseIsOver(1) == 1)) {
        return 0;
    }
    if (*(u8 *)((u8 *)(*(void **)((u8 *)cam + 0x384)) + 0x81E) != 0) {
        return 0;
    }
    return Hud_InputBlocked() != 1;
}

void CamCtl_ReadPad(void *cam)
{
    if (CamCtl_PadAllowed((void *) cam) == 0) {
        ((CamCtl *)cam)->padDir = 0U;
        ((CamCtl *)cam)->padPressed = 0U;
        ((CamCtl *)cam)->padHeld = 0U;
        ((CamCtl *)cam)->padMagR = 0U;
        ((CamCtl *)cam)->padDegR = 0U;
        return;
    }
    ((CamCtl *)cam)->padHeld = (u16) gPadState->held;
    ((CamCtl *)cam)->padPressed = (u16) gPadState->pressed;
    ((CamCtl *)cam)->padDir = (u16) gPadState->dir;
    ((CamCtl *)cam)->padDegR = (u16) gPadState->degR;
    ((CamCtl *)cam)->padMagR = (u16) gPadState->magR;
}

extern CamDistSet gCamFollowDefaults;

/* dst = the default follow settings gCamFollowDefaults. */
void Cam_CopyDefaults(CamDistSet *dst)
{
    *dst = gCamFollowDefaults;
}

void CamCtl_InitDefaultParams(CamCtl *c)
{
    CamArea *a;
    CamDataHdr *prm;

    prm = (CamDataHdr *)gSysSettings.camData;
    prm->version = 0x102;
    a = (CamArea *)(prm + 1);
    prm->areaCount = 0;
    prm->gridW = 1;
    prm->gridH = 1;
    prm->unk08 = 20000;
    prm->unk0A = 20000;
    prm->unk0C = 0;
    prm->unk10 = 0;
    prm->unk14 = 20000;
    prm->unk18 = 20000;
    prm->cells = NULL;
    prm->areaLists = NULL;
    prm->unk24 = 0;
    prm->areas = a;
    a->unk00 = 0;
    a->id = 0;
    a->kind = 0;
    a->lookLock = 2;
    a->lookSpace = 0;
    a->faceCount = 0;
    a->fovDist[0] = 400.0f;
    a->fovDist[1] = 2400.0f;
    a->fovScale[0] = 1.0f;
    a->fovScale[1] = 0.75f;
    a->faces = NULL;
    a->trans = NULL;
    Cam_CopyDefaults(&a->follow);
    c->data = prm;
}

void CamCtl_LoadParams(CamCtl *c)
{
    CamDataHdr *s;

    if (c->data == NULL) {
        CamCtl_InitDefaultParams(c);
    }
    s = c->data;
    c->gridW = s->gridW;
    c->gridH = s->gridH;
    c->unk394 = s->unk08;
    c->unk396 = s->unk0A;
    c->unk398 = s->unk0C;
    c->unk39C = s->unk10;
    c->unk3A0 = s->unk14;
    c->unk3A4 = s->unk18;
}

s32 CamArea_TestObj(CamArea *area, Player *obj, u8 mask);

/* Looks up the area the local player is in, from the grid cell (cellX, cellZ) of the map at
 * c->data: stores it in c->area and returns 1 (0: none found or c->unk3AC set, -1: no map). */
s32 Cam_FindPlayerArea(CamCtl *c)
{
    CamCell *grid;
    CamCell *cell;
    CamArea **list;
    s32 n;
    Player *pl = &gPlayers[gScene.localPlayer];
    CamDataHdr *map = c->data;

    if (map == NULL) {
        c->area = NULL;
        return -1;
    }
    c->area = map->areas;
    if (c->unk3AC != 0) {
        return 0;
    }
    grid = map->cells;
    if (grid == NULL) {
        return 0;
    }
    cell = &grid[c->cellX + c->gridW * c->cellZ];
    n = cell->count;
    if (n == 0) {
        return 0;
    }
    list = cell->areas;
    while (n != 0) {
        if (CamArea_TestObj(*list, pl, 1) == 0) {
            c->area = *list;
            return 1;
        }
        n--;
        list++;
    }
    return 0;
}

/* 2 of 71 words: the original loads face in the delay slot of the n test, ours before it (with the mask in a local of its own, the registers match) */
s32 CamFace_ContainsXZ(CamFace *face, f32 *pos);

/* 0 when obj (its position at +0xAC) is in front of one of area's faces whose flags are not
 * in mask, no farther than its depth, and CamFace_ContainsXZ accepts it; 2 otherwise. */
s32 CamArea_TestObj(CamArea *area, Player *obj, u8 mask)
{
    f32 d[3];
    f32 t;
    CamFace *face;
    s32 n;
    s32 m;
    f32 *dy;
    f32 *dz;

    if (CamArea_IsEnabled(area) == 0) {
        return 2;
    }
    n = area->faceCount;
    face = area->faces;
    if (n != 0) {
        dy = &d[1];
        dz = &d[2];
        m = mask;
        do {
            if (!(face->flags & m)) {
                d[0] = obj->pos[0] - face->pos[0];
                *dy = obj->pos[1] - face->pos[1];
                *dz = obj->pos[2] - face->pos[2];
                t = Vec_Dot(face->normal, d);
                if (!(t < 0.0f) && t <= face->depth && CamFace_ContainsXZ(face, obj->pos) == 0) {
                    return 0;
                }
            }
            n--;
            face++;
        } while (n != 0);
    }
    return 2;
}

s32 Player_ActionInCamRange_Unk(void *em)
{
    u8 group;
    u8 state;
    u8 state2;

    group = *(u8 *)((u8 *)em + 0x14);
    if (group == 0) {
        state = *(u8 *)((u8 *)em + 0x15);
        if (((s32) state >= 0x27) && ((s32) state < 0x2C)) {
            return 1;
        }
    }
    if (group == 0) {
        state2 = *(u8 *)((u8 *)em + 0x15);
        if (((s32) state2 >= 0x77) && ((s32) state2 < 0x79)) {
            return 1;
        }
    }
    return 0;
}

s32 Player_LocalActionInCamRange_Unk(void)
{
    return Player_ActionInCamRange_Unk(&gPlayers[gScene.localPlayer]);
}

s32 CamArea_IsEnabled(CamArea *area)
{
    if ((area->flags & 0x80) && (Player_LocalActionInCamRange_Unk() == 0)) {
        return 0;
    }
    return 1;
}

/* a b - c d: a 2D cross product (the game used an inline helper here; plain C orders the
 * mula operands the other way). */
static inline f32 Cross2(f32 a, f32 b, f32 c, f32 d)
{
    return a * b - c * d;
}

/* 0 when pos (x at [0], z at [2]) is on the inner side of the face's four edges in the x-z plane
 * (corners (f[0], f[2]), (f[4], f[6]), (f[1], f[3]) and (f[5], f[7])), 2 when outside. */
s32 CamFace_ContainsXZ(CamFace *face, f32 *pos)
{
    f32 *f = face->quad;
    f32 dx;
    f32 dz;

    dx = pos[0] - f[0];
    dz = pos[2] - f[2];
    if (Cross2(f[2] - f[6], dx, dz, f[0] - f[4]) < 0.0f) {
        return 2;
    }
    if (Cross2(f[7] - f[2], dx, dz, f[5] - f[0]) < 0.0f) {
        return 2;
    }
    dx = pos[0] - f[1];
    dz = pos[2] - f[3];
    if (Cross2(f[6] - f[3], dx, dz, f[4] - f[1]) < 0.0f) {
        return 2;
    }
    if (Cross2(f[3] - f[7], dx, dz, f[1] - f[5]) < 0.0f) {
        return 2;
    }
    return 0;
}

void CamView_StartBlend(CamView *p, s32 n);

/* When p->areaChanged is set: the length of t's transition for p->prevAreaId, from the
 * 16 {area id, frames} pairs of r->trans (15 without r, its table or the key);
 * 0 or 1 frames stop the transition, more start it (CamView_StartBlend). */
void CamView_SetTransition(CamCtl *p, CamView *t, CamArea *r)
{
    s32 n;
    u8 *e;
    s32 i;

    if (p->areaChanged != 0) {
        n = 0xF;
        if (r != NULL) {
            e = r->trans;
            if (e != NULL) {
                for (i = 0x10; i != 0; i--, e += 2) {
                    if (e[0] == p->prevAreaId) {
                        n = e[1];
                        break;
                    }
                }
            }
        }
        switch (n) {
        default:
            CamView_StartBlend(t, n);
            break;
        case 0:
        case 1:
            t->blend = 0;
            break;
        }
    }
}

f32 CamBlend_EaseStep(CamView *v)
{
    f32 t;
    f32 ret;

    t = (f32) v->left * v->rate;
    if (t < 0.5f) {
        ret = 0.5f * Math_Pow(2.0f * t, 2.0f);
    } else {
        ret = 1.0f - (0.5f * Math_Pow(2.0f * (1.0f - t), 2.0f));
    }
    v->left = (s16) (v->left - 1);
    if (v->left == 0) {
        v->blend = 0;
    }
    return ret;
}

void CamView_StartBlend(CamView *p, s32 n)
{
    p->blend = 1;
    p->left = n - 1;
    p->rate = 1.0f / n;
}

f32 Math_Atan2(f32 y, f32 x);

/* Angle (0x10000 per turn) of the direction from b to a in the x-z plane. */
s16 Vec_AngleXZ16(f32 *a, f32 *b)
{
    return (u16)(s32)(RAD_TO_ANGLE16 * Math_Atan2(a[0] - b[0], a[2] - b[2]));
}

/* A bump over the 16-bit angle a: 0 outside lo..hi, amp * ((a - lo)(hi - a) / 65536)^2 inside, and the same mirrored (negative) over 0x10000 - hi .. 0x10000 - lo. */
s32 Angle_Bump(u32 a, f32 lo, f32 hi, f32 amp)
{
    f32 x;
    f32 d;

    if (a <= lo) {
        return 0;
    }
    if (!(a < hi)) {
        if (a <= 65536.0f - hi) {
            return 0;
        }
        if (a < 65536.0f - lo) {
            a = 0x10000 - a;
            amp = -amp;
        } else {
            return 0;
        }
    }
    x = (s32)a;
    d = x - lo;
    x = hi - x;
    d = 0.000015258789f * (d * d);
    x = 0.000015258789f * (x * x);
    return amp * (d * x);
}

s32 Cam_PlayerHasFlag80000(Player *pl)
{
    return Player_TestFlag(pl, 0x80000U) != 0;
}

s32 CamFollow_IsBlownAway(void);
s16 Player_IsState(void *obj, u8 state, u8 sub);

/* 1 when player pl (+0x12 set) is in one of the states that let the camera follow (weapon kinds
 * 1 and 5 with +0x36C bit 3, kind 10 with bit 7), 0 when not, -1 otherwise. */
s32 Cam_PlayerCanFollow(Player *pl)
{
    s32 r;

    if (CamFollow_IsBlownAway() == 1) {
        return -1;
    }
    if (pl->weaponOut != 0) {
        if (pl->weaponType == 1 || pl->weaponType == 5) {
            if (pl->input.held & 8) {
                if (Player_IsState(pl, 0, 0) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, 1, 0) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, 1, 4) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, 1, 5) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, 1, 1) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, 1, 6) != 0) {
                    return 1;
                }
                r = Player_IsState(pl, 1, 2) != 0;
                return r;
            }
        } else if (pl->weaponType == 10) {
            if (pl->input.held & 0x80) {
                if (Player_IsState(pl, 1, 1) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, 1, 9) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, 1, 0xB) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, 1, 0xF) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, 1, 5) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, 1, 7) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, 1, 0) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, 1, 0xE) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, 1, 8) != 0) {
                    return 1;
                }
                r = Player_IsState(pl, 1, 0xA) != 0;
                return r;
            }
        }
    }
    return -1;
}

s32 CamFollow_ShouldTurnBehind(Player *pl, CamFollow *w)
{
    u16 flags;
    u8 group;

    if ((Player_LocalActionInCamRange_Unk() == 1) && (w->unk75 != 0)) {
        return 1;
    }
    flags = w->pressed;
    if (flags & 8) {
        return 1;
    }
    if ((pl->weaponType == 0xA) && (flags & 0x80) && (pl->input.held & 0x80) && (pl->weaponOut != 0)) {
        group = pl->stateGroup;
        if ((group != 2) && (group != 6)) {
            return 1;
        }
    }
    return 0;
}

s8 CamFollow_GetCanFollow(void);
/* 1 when the player is free (none of its +0x536, +0x81A, +0x8D5 input lock, +0x8B1, +0x6AF set) and CamFollow_GetCanFollow is negative. */
s32 Cam_PlayerFree(Player *pl)
{
    if (Hud_InputBlocked() == 1) {
        return 0;
    }
    if (pl->ammoMenuOpen != 0) {
        return 0;
    }
    if (pl->supplyBoxOpen != 0) {
        return 0;
    }
    if (pl->deliveryOpen != 0) {
        return 0;
    }
    if (pl->deliveryMenu != 0) {
        return 0;
    }
    if (pl->aimLockTimer != 0) {
        return 0;
    }
    if (Cam_PlayerHasFlag80000(pl) != 0) {
        return 0;
    }
    return CamFollow_GetCanFollow() < 0;
}



s32 Player_LocalAreaOk_Unk(void)
{
    Player *p = &gPlayers[gScene.localPlayer];

    switch (p->area) {
    case AREA_BATTLE_TOWN_0:
    case AREA_BATTLE_TOWN_NIGHT_0:
        return 0;
    }
    return 1;
}

void CamPoint_GetPos(CamCtl *arg0, f32 *out, CamArea *area)
{
    Player *pl;
    u8 temp_a0;
    u8 temp_a0_2;

    pl = arg0->player;
    temp_a0 = area->lookSpace;
    switch (temp_a0) {
    case 0:
        Vec_Copy(out, area->fixed.at);
        break;
    case 1:
        Vec_Transform(out, area->fixed.atLocal, pl->worldMtx[0]);
        break;
    case 2:
        Vec_Add(out, pl->pos, area->fixed.atOffset);
        break;
    }
    temp_a0_2 = area->lookLock;
    switch (temp_a0_2) {
    case 0:
        out[1] = area->fixed.at[1];
        break;
    case 1:
        out[0] = area->fixed.at[0];
        out[2] = area->fixed.at[2];
        break;
    }
}

void CamPoint_GetPosOr(CamCtl *arg0, f32 *out, CamArea *area, f32 *k)
{
    Player *pl;
    u8 temp_a0;
    u8 temp_a0_2;

    pl = arg0->player;
    temp_a0 = area->lookSpace;
    switch (temp_a0) {
    case 0:
        Vec_Copy(out, k);
        break;
    case 1:
        Vec_Transform(out, area->path.atLocal, pl->worldMtx[0]);
        break;
    case 2:
        Vec_Add(out, pl->pos, area->path.atOffset);
        break;
    }
    temp_a0_2 = area->lookLock;
    switch (temp_a0_2) {
    case 0:
        out[1] = k[1];
        break;
    case 1:
        out[0] = k[0];
        out[2] = k[2];
        break;
    }
}

void Path_BuildSpline(PathSeg *seg, PathKey *k, s32 n);
void Vec_EvalCubic(f32 *out, PathSeg *k, f32 t);

/* out = the point of area's path at o's position: segment o->pathPos.k, at o->pathPos.t
 * (0 to 1) of its length; segs receives the path's segments. */
void Path_PointAt(f32 *out, CamCtl *o, CamArea *area, PathSeg *segs)
{
    u8 k;

    Path_BuildSpline(segs, area->path.d.keys, area->path.d.count);
    k = o->pathPos.k;
    Vec_EvalCubic(out, &segs[k], o->pathPos.t * area->path.d.keys[k].len);
}

f32 Vec_Distance(f32 *a, f32 *b);

/* The segment of the path d nearest to pt: the nearest key, or the one before
 * it when that one is nearer than the one after, kept below count - 1. The
 * walk steps 12 bytes through the 16-byte keys2 (as the original does). */
s32 Path_NearestSeg(PathData *d, f32 *pt)
{
    f32 dist[16];
    f32 *p = d->keys2[0].pos;
    f32 *dp = dist;
    s32 i;
    s32 best = 0;
    f32 min = 1e7f;

    for (i = 0; i < d->count; i++) {
        *dp = Vec_Distance(pt, p);
        if (min > *dp) {
            min = *dp;
            best = i;
        }
        p += 3;
        dp++;
    }
    if (best != 0 && best < d->count - 1) {
        if (dist[best - 1] < dist[best + 1]) {
            best--;
        }
    } else if (best >= d->count - 1) {
        best = d->count - 2;
    }
    return best;
}

/* Places pos on segment key from the n candidate distances ts: the first
 * one within the segment (0), else the one nearest to it, clamped to the
 * start (-1) or the end (1). */
s32 PathPos_PlaceOnSeg(PathPos *pos, PathSeg *seg, PathKey *key, f32 *ts, s32 n)
{
    f32 best;
    f32 t;
    f32 t0;
    s32 r;

    t0 = *ts;
    if (t0 < 0.0f) {
        pos->dist = 0.0f;
        r = -1;
        best = -t0;
    } else if (!(t0 <= key->len)) {
        best = t0 - key->len;
        r = 1;
        pos->dist = key->len;
    } else {
        r = 0;
        pos->dist = t0;
        return r;
    }
    if (n == 1) {
        return r;
    }
    for (n--; n != 0; n--) {
        t = *++ts;
        if (t < 0.0f) {
            if (!(best <= -t)) {
                best = -t;
                r = -1;
                pos->dist = 0.0f;
            }
        } else if (!(t <= key->len)) {
            t -= key->len;
            if (!(best <= t)) {
                pos->dist = key->len;
                r = 1;
                best = t;
            }
        } else {
            r = 0;
            pos->dist = t;
            return r;
        }
    }
    return r;
}

s32 PathSeg_Nearest(f32 *ts, PathSeg *seg, f32 *pt, s32 axes);

/* Builds d's segments into x and places pos on segment pos->k at the point nearest pt
 * (PathSeg_Nearest, PathPos_PlaceOnSeg), moving to the next or the previous segment when the point is
 * past an end; 0 (and pos->dist cleared) when there is none. */
s32 PathPos_Build(PathPos *pos, PathSeg *segs, PathData *d, f32 *pt)
{
    f32 ts[8];
    s32 n;
    s32 r;

    Path_BuildSpline(segs, d->keys2, d->count);
    n = PathSeg_Nearest(ts, &segs[pos->k], pt, d->unk261);
    if (n == 0) {
        pos->dist = 0.0f;
        return 0;
    }
    r = PathPos_PlaceOnSeg(pos, &segs[pos->k], &d->keys2[pos->k], ts, n);
    if (r > 0) {
        if (pos->k < d->count - 2) {
            n = PathSeg_Nearest(ts, &segs[pos->k + 1], pt, d->unk261);
            if (n != 0) {
                pos->k++;
                PathPos_PlaceOnSeg(pos, &segs[pos->k], &d->keys2[pos->k], ts, n);
            }
        }
    } else if (r < 0) {
        if (pos->k > 0) {
            n = PathSeg_Nearest(ts, &segs[pos->k - 1], pt, d->unk261);
            if (n != 0) {
                pos->k--;
                PathPos_PlaceOnSeg(pos, &segs[pos->k], &d->keys2[pos->k], ts, n);
            }
        }
    }
    return 1;
}

/* Moves pos by at most 0.5 toward distance dist on segment k, crossing
 * segment ends on the way (PathPos_MoveToward when k is another segment). */
void PathPos_MoveOnSeg(PathPos *pos, PathData *d, s32 k, f32 dist)
{
    f32 step = 0.5f;
    f32 rem;
    f32 cur;

    if (pos->k < k) {
        do {
            cur = pos->dist;
            rem = d->keys2[pos->k].len - cur;
            if (rem <= step) {
                step -= rem;
                pos->k++;
                pos->dist = 0.0f;
            } else {
                pos->dist = cur + step;
                return;
            }
        } while (pos->k < k);
        if (dist - pos->dist <= step) {
            pos->dist = dist;
        } else {
            pos->dist = pos->dist + step;
        }
    } else {
        do {
            if (pos->dist <= step) {
                step -= pos->dist;
                pos->k--;
                pos->dist = d->keys2[k].len;
            } else {
                pos->dist = pos->dist - step;
                return;
            }
        } while (pos->k > k);
        if (pos->dist - dist <= step) {
            pos->dist = dist;
        } else {
            pos->dist = pos->dist - step;
        }
    }
}

/* Moves pos along the path d toward the point near pt, by at most 0.5 when
 * both are on the same segment (PathPos_MoveOnSeg otherwise); -1 when there is
 * no such point. */
s32 PathPos_MoveToward(PathPos *pos, PathData *d, PathSeg *segs, f32 *pt)
{
    PathPos to;

    to.k = Path_NearestSeg(d, pt);
    if (PathPos_Build(&to, segs, d, pt) != 0) {
        if (pos->k == to.k) {
            if (pos->dist < to.dist) {
                if (to.dist - pos->dist <= 0.5f) {
                    pos->dist = to.dist;
                } else {
                    pos->dist = pos->dist + 0.5f;
                }
            } else if (!(pos->dist <= to.dist)) {
                if (pos->dist - to.dist <= 0.5f) {
                    pos->dist = to.dist;
                } else {
                    pos->dist = pos->dist - 0.5f;
                }
            }
        } else {
            PathPos_MoveOnSeg(pos, d, to.k, to.dist);
        }
    } else {
        return -1;
    }
    pos->t = pos->dist / d->keys2[pos->k].len;
    return 0;
}


/* Places pos on the path d near pt (the segment from Path_NearestSeg, the
 * distance from PathPos_Build); -1 and pos cleared when that fails. */
s32 PathPos_PlaceNear(PathPos *pos, PathData *d, PathSeg *segs, f32 *pt)
{
    pos->k = Path_NearestSeg(d, pt);
    if (PathPos_Build(pos, segs, d, pt) != 0) {
        pos->t = pos->dist / d->keys2[pos->k].len;
        return 0;
    }
    pos->t = 0.0f;
    pos->dist = 0.0f;
    return -1;
}

s32 CamCtl_PathPlace(CamCtl *arg0, PathSeg *segs, f32 *pos, s32 flag)
{
    CamArea *area;
    s32 ret;

    area = arg0->area;
    if (area == NULL) {
        return -1;
    }
    if (flag == 0) {
        ret = PathPos_PlaceNear(&arg0->pathPos, &area->path.d, segs, pos);
    } else {
        ret = PathPos_MoveToward(&arg0->pathPos, &area->path.d, segs, pos);
    }
    return ret;
}

void Vec_EvalCubic(f32 *out, PathSeg *k, f32 fparg0)
{
    Vec_Scale(out, k->c[0], fparg0);
    Vec_Add(out, out, k->c[1]);
    Vec_Scale(out, out, fparg0);
    Vec_Add(out, out, k->c[2]);
    Vec_Scale(out, out, fparg0);
    Vec_Add(out, out, k->c[3]);
}

/* a . b in the x-z plane. */
static f32 PathVec_DotXZ(f32 *a, f32 *b)
{
    return a[2] * b[2] + a[0] * b[0];
}

/* a . b. */
static f32 PathVec_Dot(f32 *a, f32 *b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

f32 Math_Absf(f32 x);
void Quintic_Roots(Vec2f *roots, f32 *p);

s32 Cubic_Roots(f32 *out, f32 *c);

#define LEN2(v) ((v)[0] * (v)[0] + (v)[1] * (v)[1] + (v)[2] * (v)[2])

/* The parameters t of segment seg where it comes nearest pt (the roots of the derivative of the
 * squared distance; in the x-z plane when axes is 0) into ts; returns how many. */
s32 PathSeg_Nearest(f32 *ts, PathSeg *seg, f32 *pt, s32 axes)
{
    f32 q[6];
    Vec2f roots[5];
    f32 d[3];
    f32 p[3];
    f32 t;
    f32 u;
    s32 n;
    u32 i;

    Vec_Copy(p, pt);
    if (axes == 0) {
        p[1] = 0.0f;
        Vec_Diff(d, seg->c[3], p);
        if (LEN2(seg->c[0]) <= 1e-10f) {
            if (LEN2(seg->c[1]) <= 1e-10f) {
                if (LEN2(seg->c[2]) <= 1e-10f) {
                    return 0;
                }
                t = PathVec_DotXZ(seg->c[2], seg->c[2]);
                ts[0] = -(PathVec_DotXZ(seg->c[2], d) / t);
                return 1;
            }
            q[0] = 2.0f * PathVec_DotXZ(seg->c[1], seg->c[1]);
            q[1] = 3.0f * PathVec_DotXZ(seg->c[1], seg->c[2]);
            t = 2.0f * PathVec_DotXZ(seg->c[1], d);
            q[2] = t + PathVec_DotXZ(seg->c[2], seg->c[2]);
            q[3] = PathVec_DotXZ(seg->c[2], d);
            return Cubic_Roots(ts, q);
        }
        q[0] = 3.0f * PathVec_DotXZ(seg->c[0], seg->c[0]);
        q[1] = 5.0f * PathVec_DotXZ(seg->c[0], seg->c[1]);
        t = 4.0f * PathVec_DotXZ(seg->c[0], seg->c[2]);
        q[2] = t + 2.0f * PathVec_DotXZ(seg->c[1], seg->c[1]);
        t = PathVec_DotXZ(seg->c[1], seg->c[2]);
        u = PathVec_DotXZ(seg->c[0], d);
        q[3] = 3.0f * (u + t);
        t = 2.0f * PathVec_DotXZ(seg->c[1], d);
        q[4] = t + PathVec_DotXZ(seg->c[2], seg->c[2]);
        q[5] = PathVec_DotXZ(seg->c[2], d);
        Quintic_Roots(roots, q);
        for (i = n = 0; i < 5; i++) {
            if (Math_Absf(roots[i].y) < 0.001f) {
                n++;
                *ts++ = roots[i].x;
            }
        }
    } else {
        Vec_Diff(d, seg->c[3], p);
        if (LEN2(seg->c[0]) <= 1e-10f) {
            if (LEN2(seg->c[1]) <= 1e-10f) {
                if (LEN2(seg->c[2]) <= 1e-10f) {
                    return 0;
                }
                t = PathVec_Dot(seg->c[2], seg->c[2]);
                ts[0] = -(PathVec_Dot(seg->c[2], d) / t);
                return 1;
            }
            q[0] = 2.0f * PathVec_Dot(seg->c[1], seg->c[1]);
            q[1] = 3.0f * PathVec_Dot(seg->c[1], seg->c[2]);
            t = 2.0f * PathVec_Dot(seg->c[1], d);
            q[2] = t + PathVec_Dot(seg->c[2], seg->c[2]);
            q[3] = PathVec_Dot(seg->c[2], d);
            return Cubic_Roots(ts, q);
        }
        q[0] = 3.0f * PathVec_Dot(seg->c[0], seg->c[0]);
        q[1] = 5.0f * PathVec_Dot(seg->c[0], seg->c[1]);
        t = 4.0f * PathVec_Dot(seg->c[0], seg->c[2]);
        q[2] = t + 2.0f * PathVec_Dot(seg->c[1], seg->c[1]);
        t = PathVec_Dot(seg->c[1], seg->c[2]);
        u = PathVec_Dot(seg->c[0], d);
        q[3] = 3.0f * (u + t);
        t = 2.0f * PathVec_Dot(seg->c[1], d);
        q[4] = t + PathVec_Dot(seg->c[2], seg->c[2]);
        q[5] = PathVec_Dot(seg->c[2], d);
        Quintic_Roots(roots, q);
        for (i = n = 0; i < 5; i++) {
            if (Math_Absf(roots[i].y) < 0.001f) {
                n++;
                *ts++ = roots[i].x;
            }
        }
    }
    return n;
}

/* The fov's scale for area at the eye's distance x: on the segment (fovDist[0], fovScale[0]) -
 * (fovDist[1], fovScale[1]), clamped to its ends (CamRail_Update). */
f32 CamArea_FovScale(CamArea *area, f32 x)
{
    f32 r;
    f32 x1;
    f32 x0;

    x0 = area->fovDist[0];
    if (x <= x0) {
        return area->fovScale[0];
    }
    x1 = area->fovDist[1];
    if (!(x < x1)) {
        return area->fovScale[1];
    }
    if (x1 == x0) {
        return 0.5f * (area->fovScale[0] + area->fovScale[1]);
    }
    r = (area->fovScale[1] - area->fovScale[0]) * (x - x0);
    r /= x1 - x0;
    r += area->fovScale[0];
    return r;
}

/* Value i of the path keys d blended with value i + 1 by t. */
f32 PathKey_Blend(PathData *d, s32 i, f32 t)
{
    return d->vals[i] * (1.0f - t) + d->vals[i + 1] * t;
}

/* Angle i of the path keys d blended with angle i + 1 by t (in 16.16 fixed
 * point, keeping the upper half), in radians. */
f32 PathKey_BlendAngle(PathData *d, s32 i, f32 t)
{
    s32 w;
    s16 *hi = (s16 *)&w + 1;

    w = (s32)(65536.0f * t) * (d->angles[i + 1] - d->angles[i]);
    *hi += d->angles[i];
    return 9.58738e-05f * *hi;
}

/* Solves the tridiagonal system a[i] x[i-1] + b[i] x[i] + c[i] x[i+1] = d[i] (i < n, n <= 64). */
static void Tridiag_Solve(f32 *x, f32 *a, f32 *b, f32 *c, f32 *d, s32 n)
{
    f32 g[64];
    f32 bet;
    s32 i;

    bet = b[0];
    if (bet != 0.0f) {
        bet = 1.0f / bet;
    }
    x[0] = d[0] * bet;
    for (i = 1; i < n; i++) {
        g[i - 1] = bet * c[i - 1];
        bet = b[i] - a[i] * g[i - 1];
        if (bet != 0.0f) {
            bet = 1.0f / bet;
        }
        x[i] = bet * (d[i] - a[i] * x[i - 1]);
    }
    for (i = n - 2; i >= 0; i--) {
        x[i] -= g[i] * x[i + 1];
    }
}

/* Builds the n - 1 cubic segments through the path keys k (a spline with the key lengths as the
 * parameter steps): solves for the tangents with Tridiag_Solve, one axis at a time. */
void Path_BuildSpline(PathSeg *seg, PathKey *k, s32 n)
{
    f32 m[9][16]; /* rows 0-2 the system (a, b, c), 3-5 the right sides and 6-8 the tangents (x, y, z) */
    f32 *p;
    s32 i;
    f32 r0;
    f32 r1;
    f32 ih;
    f32 ih2;
    f32 ih3;

    p = m[0];
    p[0] = 0.0f;
    p[1 * 16] = k[0].len + k[0].len;
    p[2 * 16] = k[0].len;
    p[3 * 16] = 3.0f * (k[1].pos[0] - k[0].pos[0]);
    p[4 * 16] = 3.0f * (k[1].pos[1] - k[0].pos[1]);
    p[5 * 16] = 3.0f * (k[1].pos[2] - k[0].pos[2]);
    for (i = 1; i < n - 1; i++) {
        r0 = k[i].len / k[i - 1].len;
        r1 = k[i - 1].len / k[i].len;
        p[i] = k[i].len;
        p[1 * 16 + i] = 2.0f * (k[i - 1].len + k[i].len);
        p[2 * 16 + i] = k[i - 1].len;
        p[3 * 16 + i] = 3.0f * (r0 * (k[i].pos[0] - k[i - 1].pos[0]) + r1 * (k[i + 1].pos[0] - k[i].pos[0]));
        p[4 * 16 + i] = 3.0f * (r0 * (k[i].pos[1] - k[i - 1].pos[1]) + r1 * (k[i + 1].pos[1] - k[i].pos[1]));
        p[5 * 16 + i] = 3.0f * (r0 * (k[i].pos[2] - k[i - 1].pos[2]) + r1 * (k[i + 1].pos[2] - k[i].pos[2]));
    }
    p[n - 1] = k[n - 2].len;
    p[1 * 16 + n - 1] = k[n - 2].len + k[n - 2].len;
    p[2 * 16 + n - 1] = 0.0f;
    p[3 * 16 + n - 1] = 3.0f * (k[n - 1].pos[0] - k[n - 2].pos[0]);
    p[4 * 16 + n - 1] = 3.0f * (k[n - 1].pos[1] - k[n - 2].pos[1]);
    p[5 * 16 + n - 1] = 3.0f * (k[n - 1].pos[2] - k[n - 2].pos[2]);
    Tridiag_Solve(p + 6 * 16, p, p + 16, p + 2 * 16, p + 3 * 16, n);
    Tridiag_Solve(p + 7 * 16, p, p + 16, p + 2 * 16, p + 4 * 16, n);
    Tridiag_Solve(p + 8 * 16, p, p + 16, p + 2 * 16, p + 5 * 16, n);
    for (i = 0; i < n - 1; i++) {
        ih = 1.0f / k->len;
        ih2 = ih * ih;
        ih3 = 1.0f / Math_Pow(k->len, 3.0f);
        seg->c[0][0] = ih2 * (p[6 * 16] + p[6 * 16 + 1]) + ih3 * (2.0f * (k[0].pos[0] - k[1].pos[0]));
        seg->c[1][0] = ih2 * (3.0f * (k[1].pos[0] - k[0].pos[0])) - ih * (2.0f * p[6 * 16] + p[6 * 16 + 1]);
        seg->c[2][0] = p[6 * 16];
        seg->c[3][0] = k[0].pos[0];
        seg->c[0][1] = ih2 * (p[7 * 16] + p[7 * 16 + 1]) + ih3 * (2.0f * (k[0].pos[1] - k[1].pos[1]));
        seg->c[1][1] = ih2 * (3.0f * (k[1].pos[1] - k[0].pos[1])) - ih * (2.0f * p[7 * 16] + p[7 * 16 + 1]);
        seg->c[2][1] = p[7 * 16];
        seg->c[3][1] = k[0].pos[1];
        seg->c[0][2] = ih2 * (p[8 * 16] + p[8 * 16 + 1]) + ih3 * (2.0f * (k[0].pos[2] - k[1].pos[2]));
        seg->c[1][2] = ih2 * (3.0f * (k[1].pos[2] - k[0].pos[2])) - ih * (2.0f * p[8 * 16] + p[8 * 16 + 1]);
        seg->c[2][2] = p[8 * 16];
        seg->c[3][2] = k[0].pos[2];
        p++;
        k++;
        seg++;
    }
}

/* z = x + iy. */
static void Cplx_Set(Vec2f *z, f32 x, f32 y)
{
    z->x = x;
    z->y = y;
}

/* out = a - b. */
static void Cplx_Sub(Vec2f *out, Vec2f *a, Vec2f *b)
{
    out->x = a->x - b->x;
    out->y = a->y - b->y;
}

/* out = a * b as complex numbers (a rotated by the angle of b, scaled by its length). */
static void Cplx_Mul(Vec2f *out, Vec2f *a, Vec2f *b)
{
    Vec2f r;

    r.x = a->x * b->x - a->y * b->y;
    r.y = a->x * b->y + a->y * b->x;
    *out = r;
}

/* out = a / b as complex numbers (a when b is zero). */
static void Cplx_Div(Vec2f *out, Vec2f *a, Vec2f *b)
{
    Vec2f r;
    f32 ax;
    f32 ay;
    f32 by;
    f32 bx;
    f32 d;

    bx = b->x;
    by = b->y;
    d = bx * bx + by * by;
    if (0.0f != d) {
        d = 1.0f / d;
        ax = a->x;
        ay = a->y;
        r.x = d * (ax * bx + ay * by);
        r.y = d * (ay * bx - ax * by);
        *out = r;
    } else {
        *out = *a;
    }
}

f32 Math_Absf(f32 x);
extern f32 gQuinticBoundExps[];
extern f32 gQuinticStart0[];
extern f32 gQuinticStart0Y;
extern f32 gQuinticStart1[];
extern f32 gQuinticStart1Y;
extern f32 gQuinticStart2[];
extern f32 gQuinticStart2Y;
extern f32 gQuinticStart3[];
extern f32 gQuinticStart3Y;
extern f32 gQuinticStart4[];
extern f32 gQuinticStart4Y[];

/* The five complex roots of the quintic p[0] x^5 + ... + p[5] (Durand-Kerner, 26 rounds from
 * points on a circle of the roots' bound). */
void Quintic_Roots(Vec2f *roots, f32 *p)
{
    Vec2f den;
    Vec2f val;
    Vec2f z;
    Vec2f step;
    Vec2f d;
    f32 a[6];
    s32 i;
    f32 *e;
    f32 *q;
    f32 inv;
    f32 r;
    f32 t;
    s32 k;
    s32 n;
    Vec2f *rj;
    f32 *ak;
    Vec2f *rk;
    s32 j;

    inv = 1.0f / p[0];
    for (i = 1; i < 6; i++) {
        a[i] = inv * p[i];
    }
    r = 0.0f;
    for (i = 2, e = gQuinticBoundExps, q = &a[2]; i <= 5; i++, e++, q++) {
        t = Math_Pow(Math_Absf(*q), *e);
        if (!(t <= r)) {
            r = t;
        }
    }
    r *= 5.0f;
    roots[0].x = r * gQuinticStart0[0];
    roots[0].y = r * gQuinticStart0Y;
    roots[1].x = r * gQuinticStart1[0];
    roots[1].y = r * gQuinticStart1Y;
    roots[2].x = r * gQuinticStart2[0];
    roots[2].y = r * gQuinticStart2Y;
    roots[3].x = r * gQuinticStart3[0];
    roots[3].y = r * gQuinticStart3Y;
    roots[4].x = r * gQuinticStart4[0];
    roots[4].y = r * gQuinticStart4Y[0];
    for (n = 25; n >= 0; n--) {
        for (j = 0, rj = roots; j < 5; j++, rj++) {
            Cplx_Set(&den, 1.0f, 0.0f);
            Cplx_Set(&val, 1.0f, 0.0f);
            z = *rj;
            for (k = 0, ak = a, rk = roots; k < 5; k++, ak++, rk++) {
                Cplx_Mul(&val, &val, &z);
                val.x += ak[1];
                if (k != j) {
                    Cplx_Sub(&d, &z, rk);
                    Cplx_Mul(&den, &den, &d);
                }
            }
            Cplx_Div(&step, &val, &den);
            Cplx_Sub(rj, &z, &step);
        }
    }
}

f32 Math_Sqrtf(f32 x);
f32 Math_Cos(f32 x);

/* The real roots of the cubic c[0] x^3 + c[1] x^2 + c[2] x + c[3] into out (Cardano); returns
 * how many (1 to 3). */
s32 Cubic_Roots(f32 *out, f32 *c)
{
    f32 inv;
    f32 h;
    f32 p;
    f32 q;
    f32 d;
    f32 p3;
    f32 u;
    f32 v;
    f32 a;
    f32 b;

    inv = 1.0f / c[0];
    a = c[1] * inv;
    b = c[2] * inv;
    h = 0.33333334f * a;
    p = 0.33333334f * b - h * h;
    q = 0.5f * (c[3] * inv + h * (h * h + h * h - b));
    if (Math_Absf(p) < 1e-6f && Math_Absf(q) < 1e-6f) {
        out[0] = -h;
        return 1;
    }
    p3 = p * p;
    d = p3 * p + q * q;
    if (!(d <= 0.0f)) {
        if (q >= 0.0f) {
            u = Math_Pow(q + Math_Sqrtf(d), 0.33333334f);
        } else {
            u = -Math_Pow(-q + Math_Sqrtf(d), 0.33333334f);
        }
        v = p / u;
        if (p < 0.0f) {
            out[0] = v - u;
        } else {
            v = u * u;
            out[0] = -2.0f * q * v / (p3 + v * (v + p));
        }
        out[0] -= h;
        return 1;
    }
    if (q < 0.0f) {
        p = Math_Sqrtf(-p);
    } else {
        p = -Math_Sqrtf(-p);
    }
    if (Math_Absf(d) < 1e-6f) {
        out[0] = p + p - h;
        out[1] = -p - h;
        return 2;
    }
    q = 0.33333334f * Math_Atan2(Math_Sqrtf(-d), -q);
    u = p * Math_Cos(q);
    v = 1.7320508f * (p * Math_Sin(q));
    out[0] = u + u - h;
    out[1] = -u - v - h;
    out[2] = -u + v - h;
    return 3;
}

s32 CamCtl_GetMode(void);
void Vec_Set(f32 *v, f32 x, f32 y, f32 z);
f32 Vec_Distance(f32 *a, f32 *b);
/* the calls cast to u8: this unit's code takes the value as u8 */
s32 AreaGrid_SweepCells(f32 *from, f32 *to, f32 *out, u16 mask, f32 r);
extern f32 gCamCollideSphere[];
extern f32 D_0041CE44[];
extern f32 D_0041CE48[];
extern f32 D_0041CE4C[];

/* The distance from `from` to the first hit of a line test from `to` (both raised by
 * gCamCollideSphere[0]..48), 0 when nothing is hit; returns 0. */
f32 Cam_LineHitDist(f32 *from, f32 *to, f32 *dist)
{
    f32 a[3];
    f32 b[3];
    f32 hit[3];

    if (CamCtl_GetMode() & 2) {
        return 0.0f;
    }
    /* FAKEMATCH: the comma in the subscript (a permuter find) gives the offsets the
     * original's float registers (f1, f3, f2); every plain spelling gives f3, f2, f1. */
    a[0] = to[0] + gCamCollideSphere[0, 0];
    a[1] = to[1] + D_0041CE44[0];
    a[2] = to[2] + D_0041CE48[0];
    b[0] = from[0] + gCamCollideSphere[0];
    b[1] = from[1] + D_0041CE44[0];
    b[2] = from[2] + D_0041CE48[0];
    Vec_Set(hit, from[0], from[1], from[2]);
    if ((u8)AreaGrid_SweepCells(a, b, from, 0x8001, D_0041CE4C[0]) > 0) {
        *dist = Vec_Distance(hit, from);
    } else {
        *dist = 0.0f;
    }
    return 0.0f;
}

s32 CamCtl_GetMode(void);
s32 BoneHit_Place(ObjHead *o, HitShape *def, HitCapsule *c, HitSphere *s);
void Capsule_Copy(struct HitCapsule *src, struct Capsule *dst);
s32 Sphere_PushOut(f32 ra, f32 rb, f32 *a, f32 *b, f32 *out);
s32 Capsule_PushSphere(struct Capsule *cap, f32 *p, f32 *out, f32 r);
void HitList_Next(Em *p, HitShape **cur);
f32 AreaGrid2_GetFloorY(f32 *pos);
extern HitShape *gEmBoneHits[];
extern HitShape *D_7F1950[];

/* Adds push v to the push acc: the larger one when their signs differ, else their sum. */
#define PUSH_ADD(acc, v)                                                                      \
    if ((!(acc < 0.0f) && v < 0.0f) || (acc < 0.0f && !(v < 0.0f)) ||                         \
        (!(acc < -0.0f) && v < -0.0f) || (acc < -0.0f && !(v < -0.0f))) {                     \
        if (Math_Absf(acc) < Math_Absf(v)) {                                                  \
            acc = -v;                                                                         \
        }                                                                                     \
    } else {                                                                                  \
        acc -= v;                                                                             \
    }

/* Pushes the point pos (raised by gCamCollideSphere[0]..48) out of the colliders of the monsters in the
 * current area, then keeps it 10 above the floor. */
void Cam_PushOutOfEms(f32 *pos, f32 *unused1)
{
    f32 cap[8];
    f32 sph[4];
    f32 work[16];
    f32 p[3];
    f32 push[3];
    HitShape *c;
    Em *em;
    f32 *o;
    s32 i;
    s32 k;
    f32 y;
    f32 az;
    f32 ay;
    f32 ax;

    if (CamCtl_GetMode() & 1) {
        return;
    }
    em = gEms;
    o = gCamCollideSphere;
    ax = ay = az = 0.0f;
    for (i = 0; i < 20; i++, em++) {
        if (em->active != 0 && em->isEm != 0 && em->visible != 0 && gScene.area == em->area) {
            p[0] = pos[0] + o[0];
            p[1] = pos[1] + o[1];
            p[2] = pos[2] + o[2];
            if (gScene.online == 0) {
                c = gEmBoneHits[em->type];
            } else {
                c = D_7F1950[em->type];
            }
            while ((k = c->bone) != -1) {
                if ((c->atkFlags & 4) || (c->atkFlags2 & 2) || (c->openFlags & 8) || (c->openFlags & 2) || (c->offFlags & 2)) {
                    c++;
                    continue;
                }
                if (k == 0x7D) {
                    HitList_Next(em, &c);
                    continue;
                }
                switch ((s16)BoneHit_Place((ObjHead *)em, c, (struct HitCapsule *) cap, (struct HitSphere *) sph)) {
                case 0:
                    if (Sphere_PushOut(sph[3], o[3], sph, p, push) != 0) {
                        PUSH_ADD(ax, push[0]);
                        PUSH_ADD(ay, push[1]);
                        PUSH_ADD(az, push[2]);
                    }
                    break;
                case 1:
                    Capsule_Copy((struct HitCapsule *) cap, (struct Capsule *) work);
                    if (Capsule_PushSphere((struct Capsule *) work, p, push, o[3]) != 0) {
                        PUSH_ADD(ax, push[0]);
                        PUSH_ADD(ay, push[1]);
                        PUSH_ADD(az, push[2]);
                    }
                    break;
                }
                c++;
            }
        }
    }
    if (ax != 0.0f || ay != 0.0f || az != 0.0f) {
        pos[0] += ax;
        pos[1] += ay;
        pos[2] += az;
        y = 10.0f + AreaGrid2_GetFloorY(pos);
        if (pos[1] < y) {
            pos[1] = y;
        }
    }
}

u16 Area_DayId(u16 a);
u8 MonsterGroup_First(u8 id);
void CamReq_Start(s32 req, void *arg);
void CamReq_Change(s32 req, void *arg);
extern u8 gQuestTargetCamIds[];

/* Starts the camera request for the monster the quest is about ((*(u8 * *)&gQuest.downEm), else (*(u8 * *)&gQuest.spawnedEm) for
 * some kinds) when it is in the current area: CamReq_Start (flag 0) or CamReq_Change. */
void CamReq_QuestTarget(s32 flag)
{
    Em *e;
    s32 i;
    u8 *p;
    u8 k;
    s32 id;

    for (i = 2, p = ((u8 *)&gQuest + 2); i >= 0; i--, p--) {
        if (p[0x541] != 0xFF) {
            k = gQuest.goalIdx[i];
            if (!(gQuest.goals[k].flags & 5)) {
                return;
            }
            if (gQuest.downEm != NULL && gQuest.downEm->type != gQuest.goals[k].target) {
                return;
            }
            break;
        }
    }
    e = gQuest.downEm;
    if (e == NULL) {
        e = gQuest.spawnedEm;
        if (e == NULL) {
            return;
        }
        if (e->type != 7 && e->type != 0x32 && e->type != 2 && e->type != 0x24 && e->type != 0x47 && e->type != 0x37) {
            return;
        }
    }
    if (gScene.area != e->area) {
        return;
    }
    switch (e->type) {
    case 7:
    case 0x32:
        if (Area_DayId(gScene.area) != AREA_FORTRESS_5) {
            return;
        }
        if (gQuest.emLeft == 0) {
            id = 0x86;
        } else {
            return;
        }
        break;
    case 2:
    case 0x24:
    case 0x47:
        if (gQuest.emLeft == 0) {
            id = 0x8A;
        } else {
            return;
        }
        break;
    case 0x37:
        if (gQuest.emLeft == 0) {
            id = 0x87;
        } else {
            return;
        }
        break;
    case 0x36:
    case 0x3C:
    case 0x3B:
        if (gQuest.emLeft == 0) {
            id = 7;
        } else {
            return;
        }
        break;
    case 0x40:
    case 0x41:
        if (gQuest.emLeft == 0) {
            id = 7;
        } else {
            return;
        }
        break;
    case 0x15:
    case 0x2E:
        if (e->stateGroup == 5 && e->state == 2) {
            return;
        }
    default:
        id = gQuestTargetCamIds[MonsterGroup_First(e->type)];
        if (id == 0xFF) {
            return;
        }
        break;
    }
    if (flag == 0) {
        CamReq_Start(id, e);
    } else {
        CamReq_Change(id, e);
    }
}

/* unk_31E190's .data (0x4187B0-0x41CEB0) */
void CamFollow_Init(CamCtl *c, CamView *v);
void CamFollow_Update(CamCtl *c, CamView *v);
void CamRail_Init(CamCtl *c, CamView *v);
void CamRail_Update(CamCtl *c, CamView *v);
void CamAim_Init(CamCtl *c, CamView *v);
void CamAim_Update(CamCtl *c, CamView *v);
void CamFixed_Init(CamCtl *c, CamView *v);
void CamFixed_Update(CamCtl *c, CamView *v);
void CamScript_Init(CamCtl *c, CamView *v);
void CamScript_Update(CamCtl *c, CamView *v);
void CamOrbit_Init(CamCtl *c, CamView *v);
void CamOrbit_Update(CamCtl *c, CamView *v);
u32 gDmcamFileIds_1[92] = {
#include "sub_main/004187B0.inc"
};
u32 gDmcamFileIds_2[20] = {
#include "sub_main/00418920.inc"
};
u32 gDmcamFileIds_3[20] = {
#include "sub_main/00418970.inc"
};
u32 gDmcamFileIds_4[20] = {
#include "sub_main/004189C0.inc"
};
u32 gDmcamFileIds_5[84] = {
#include "sub_main/00418A10.inc"
};
u32 gDmcamFileIds_6[52] = {
#include "sub_main/00418B60.inc"
};
u32 gDmcamFileIds_7[76] = {
#include "sub_main/00418C30.inc"
};
u32 gDmcamFileIds_8[100] = {
#include "sub_main/00418D60.inc"
};
u32 gDmcamFileIds_9[100] = {
#include "sub_main/00418EF0.inc"
};
u32 gDmcamFileIds_10[36] = {
#include "sub_main/00419080.inc"
};
u32 gDmcamFileIds_11[112] = {
#include "sub_main/00419110.inc"
};
u32 gDmcamFileIds_12[68] = {
#include "sub_main/004192D0.inc"
};
u32 gDmcamFileIds_13[68] = {
#include "sub_main/004193E0.inc"
};
u32 gDmcamFileIds_14[104] = {
#include "sub_main/004194F0.inc"
};
u32 gDmcamFileIds_15[52] = {
#include "sub_main/00419690.inc"
};
u32 gDmcamFileIds_16[68] = {
#include "sub_main/00419760.inc"
};
u32 gDmcamFileIds_17[68] = {
#include "sub_main/00419870.inc"
};
u32 gDmcamFileIds_18[68] = {
#include "sub_main/00419980.inc"
};
u32 gDmcamFileIds_19[84] = {
#include "sub_main/00419A90.inc"
};
u32 gDmcamFileIds_20[100] = {
#include "sub_main/00419BE0.inc"
};
u32 gDmcamFileIds_21[152] = {
#include "sub_main/00419D70.inc"
};
u32 gDmcamFileIds_22[148] = {
#include "sub_main/00419FD0.inc"
};
u32 gDmcamFileIds_23[144] = {
#include "sub_main/0041A220.inc"
};
u32 gDmcamFileIds_24[20] = {
#include "sub_main/0041A460.inc"
};
u32 gDmcamFileIds_25[40] = {
#include "sub_main/0041A4B0.inc"
};
u32 gDmcamFileIds_26[20] = {
#include "sub_main/0041A550.inc"
};
u32 gDmcamFileIds_27[20] = {
#include "sub_main/0041A5A0.inc"
};
u32 gDmcamFileIds_28[20] = {
#include "sub_main/0041A5F0.inc"
};
u32 gDmcamFileIds_29[20] = {
#include "sub_main/0041A640.inc"
};
u32 gDmcamFileIds_30[20] = {
#include "sub_main/0041A690.inc"
};
u32 gDmcamFileIds_31[20] = {
#include "sub_main/0041A6E0.inc"
};
u32 gDmcamFileIds_32[20] = {
#include "sub_main/0041A730.inc"
};
u32 gDmcamFileIds_33[76] = {
#include "sub_main/0041A780.inc"
};
u32 gDmcamFileIds_34[76] = {
#include "sub_main/0041A8B0.inc"
};
u32 gDmcamFileIds_35[76] = {
#include "sub_main/0041A9E0.inc"
};
u32 gDmcamFileIds_36[68] = {
#include "sub_main/0041AB10.inc"
};
u32 gDmcamFileIds_37[44] = {
#include "sub_main/0041AC20.inc"
};
u32 gDmcamFileIds_38[28] = {
#include "sub_main/0041ACD0.inc"
};
u32 gDmcamFileIds_39[48] = {
#include "sub_main/0041AD40.inc"
};
u32 gDmcamFileIds_40[64] = {
#include "sub_main/0041AE00.inc"
};
u32 gDmcamFileIds_41[48] = {
#include "sub_main/0041AF00.inc"
};
u32 gDmcamFileIds_42[84] = {
#include "sub_main/0041AFC0.inc"
};
u32 gDmcamFileIds_43[72] = {
#include "sub_main/0041B110.inc"
};
u32 gDmcamFileIds_44[48] = {
#include "sub_main/0041B230.inc"
};
u32 gDmcamFileIds_45[64] = {
#include "sub_main/0041B2F0.inc"
};
u32 gDmcamFileIds_46[60] = {
#include "sub_main/0041B3F0.inc"
};
u32 gDmcamFileIds_47[144] = {
#include "sub_main/0041B4E0.inc"
};
u32 gDmcamFileIds_48[44] = {
#include "sub_main/0041B720.inc"
};
u32 gDmcamFileIds_49[72] = {
#include "sub_main/0041B7D0.inc"
};
u32 gDmcamFileIds_50[64] = {
#include "sub_main/0041B8F0.inc"
};
u32 gDmcamFileIds_51[24] = {
#include "sub_main/0041B9F0.inc"
};
u32 gDmcamFileIds_52[20] = {
#include "sub_main/0041BA50.inc"
};
u32 gDmcamFileIds_53[40] = {
#include "sub_main/0041BAA0.inc"
};
u32 gDmcamFileIds_54[40] = {
#include "sub_main/0041BB40.inc"
};
u32 gDmcamFileIds_55[80] = {
#include "sub_main/0041BBE0.inc"
};
u32 gDmcamFileIds_56[64] = {
#include "sub_main/0041BD20.inc"
};
u32 gDmcamFileIds_57[24] = {
#include "sub_main/0041BE20.inc"
};
u32 gDmcamFileIds_58[48] = {
#include "sub_main/0041BE80.inc"
};
u32 gDmcamFileIds_59[72] = {
#include "sub_main/0041BF40.inc"
};
u32 gDmcamFileIds_60[44] = {
#include "sub_main/0041C060.inc"
};
u32 gDmcamFileIds_61[48] = {
#include "sub_main/0041C110.inc"
};
u32 gDmcamFileIds_62[24] = {
#include "sub_main/0041C1D0.inc"
};
u32 gDmcamFileIds_63[64] = {
#include "sub_main/0041C230.inc"
};
u32 gDmcamFileIds_64[48] = {
#include "sub_main/0041C330.inc"
};
u32 gDmcamFileIds_65[48] = {
#include "sub_main/0041C3F0.inc"
};
u32 gDmcamFileIds_66[48] = {
#include "sub_main/0041C4B0.inc"
};
u32 gDmcamFileIds_67[26] = {
#include "sub_main/0041C570.inc"
};
u32 gDmcamFileIds_0[2] = {
#include "sub_main/0041C5D8.inc"
};
u32 gDmcamFileIds[140] = {
#include "sub_main/0041C5E0.inc"
};
s16 gCamShakeLengths[8] = {
#include "sub_main/0041C810.inc"
};
f32 gCamShakeRanges[4] = {
#include "sub_main/0041C820.inc"
};
CamDistSet gCamFollowDefaults = {
#include "sub_main/0041C830.inc"
};
CamDist gCamBlockedDists[2] = {
#include "sub_main/0041C930.inc"
};
CamDist gCamFallDist = {
#include "sub_main/0041C970.inc"
};
u32 gCamMode3Offsets[24] = {
#include "sub_main/0041C990.inc"
};
u32 gCamMode4AreaPos[152] = {
#include "sub_main/0041C9F0.inc"
};
u32 gCamMode4AreaFovs[28] = {
#include "sub_main/0041CC50.inc"
};
u32 gAreaCameraIdx[52] = {
#include "sub_main/0041CCC0.inc"
};
CamEntFn gCamModeInit[8] = {
    CamFollow_Init, CamOrbit_Init, CamRail_Init, CamAim_Init,
    CamFixed_Init, CamScript_Init, NULL, NULL,
};
CamEntFn gCamModeUpdate[6] = {
    CamFollow_Update, CamOrbit_Update, CamRail_Update, CamAim_Update,
    CamFixed_Update, CamScript_Update,
};
u32 gCamOrbitExitBlend[2] = {
#include "sub_main/0041CDC8.inc"
};
u32 gCamOrbitEyeOfs[4] = {
#include "sub_main/0041CDD0.inc"
};
u32 gCamOrbitAimOfs[4] = {
#include "sub_main/0041CDE0.inc"
};
f32 gQuinticStart0[2] = {
#include "sub_main/0041CDF0.inc"
};
f32 gQuinticStart1[2] = {
#include "sub_main/0041CDF8.inc"
};
f32 gQuinticStart2[2] = {
#include "sub_main/0041CE00.inc"
};
f32 gQuinticStart3[2] = {
#include "sub_main/0041CE08.inc"
};
f32 gQuinticStart4[12] = {
#include "sub_main/0041CE10.inc"
};
f32 gCamCollideSphere[8] = {
#include "sub_main/0041CE40.inc"
};
u8 gQuestTargetCamIds[80] = {
#include "sub_main/0041CE60.inc"
};
