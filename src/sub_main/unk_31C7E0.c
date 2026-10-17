#include "common.h"
#include "scene.h"
#include "em.h"
#include "camera.h"
#include "player.h"
#include "model.h"

/* prototypes */
f32 Math_Pow(f32 x, f32 y);
void Mtx_Identity(f32 *m);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Vec_Copy(f32 *dst, f32 *src);
void CamFixed_UpdateTarget(CamCtl *c, CamView *v, CamFixed *w);

s32 Cam_PlayerHasFlag80000(Player *pl);
s32 CamFixed_AreaCamera(CamCtl *c, CamView *v, CamFixed *w);
s16 Player_IsState(void *obj, u8 state, u8 sub);
s32 Player_OnMountedGun(Player *p);
void Vec_RotateY(f32 *v, f32 a);
void Vec_Add(f32 *out, f32 *a, f32 *b);
extern u8 gAreaCameraIdx[];
extern f32 gCamMode4AreaPos[][6];
extern f32 gCamMode4AreaFovs[];
/* end of prototypes */


void CamFocus_Update(CamCtl *c, CamView *v, CamFocus *f);


/* The init handler of camera mode 2 (gCamModeInit): no roll, a 55-degree field of view. */
void CamRail_Init(CamCtl *c, CamView *v)
{
    v->roll = 0.0f;
    c->rail.roll = 0.0f;
    v->fov = 0.9599311f;
    c->rail.fov = 0.9599311f;
}

extern PathSeg gCamMode2Segs[];
s32 Math_Atan2Angle(f32 y, f32 x);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
f32 Vec_Distance(f32 *a, f32 *b);
f32 Vec_Length(f32 *v);
void Vec_RotateX(f32 *v, f32 a);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
void Vec_Lerp(f32 *out, f32 *a, f32 *b, f32 t);
void CamPoint_GetPos(CamCtl *c, f32 *out, CamArea *area);
void CamPoint_GetPosOr(CamCtl *c, f32 *out, CamArea *area, f32 *k);
void Path_PointAt(f32 *out, CamCtl *o, CamArea *area, PathSeg *segs);
s32 CamCtl_PathPlace(CamCtl *c, PathSeg *segs, f32 *pos, s32 flag);
void Vec_EvalCubic(f32 *out, PathSeg *k, f32 t);
f32 CamArea_FovScale(CamArea *area, f32 dist);
f32 PathKey_Blend(PathData *d, s32 i, f32 t);
f32 PathKey_BlendAngle(PathData *d, s32 i, f32 t);
void CamView_SetTransition(CamCtl *c, CamView *v, CamArea *area);
f32 CamBlend_EaseStep(CamView *v);

/* The update handler of camera mode 2 (c->areaMode == 2): follows the camera area's path (c->area) by its
 * kind (1: fixed, 3: from the player, 2: along the segments at gCamMode2Segs); from step 1 it
 * turns the view by at most the limits from the distance unless c->areaChanged is set; then
 * blends from the view it started from (v->blend 1..2). */
void CamRail_Update(CamCtl *c, CamView *v)
{
    CamRail *w;
    CamArea *area;
    s16 step;
    PathSeg *segs;
    PathData *d;
    f32 f;
    f32 t;
    s16 ay;
    s16 ax;
    s16 lim;
    s16 lim2;
    Player *pl;
    s16 diff;
    f32 vec[3];

    segs = gCamMode2Segs;
    v->active = 0;
    if (c->areaMode != 2) {
        *(u16 *)&v->step = 0;
        return;
    }
    area = c->area;
    if (area->kind == 0) {
        *(u16 *)&v->step = 0;
        c->areaMode = 0;
        return;
    }
    pl = c->player;
    w = &c->rail;
    switch (v->step) {
    case 0:
        v->step++;
        v->active = 1;
        v->blend = 0;
        switch (area->kind) {
        case 1:
            Vec_Copy(w->pos, area->fixed.eye);
            CamPoint_GetPos(c, w->at, area);
            f = area->fixed.fov;
            w->roll = area->fixed.roll;
            break;
        case 3:
            Vec_Add(w->pos, pl->pos, area->fixed.eye);
            CamPoint_GetPos(c, w->at, area);
            f = area->fixed.fov;
            w->roll = area->fixed.roll;
            break;
        case 2:
            d = &area->path.d;
            CamCtl_PathPlace(c, segs, pl->pos, 0);
            Vec_EvalCubic(vec, &segs[c->pathPos.k], c->pathPos.dist);
            CamPoint_GetPosOr(c, w->at, area, vec);
            Path_PointAt(w->pos, c, area, segs);
            f = PathKey_Blend(d, c->pathPos.k, c->pathPos.t);
            w->roll = PathKey_BlendAngle(d, c->pathPos.k, c->pathPos.t);
            CamPoint_GetPosOr(c, w->at, area, vec);
            Path_PointAt(w->pos, c, area, segs);
            break;
        }
        Vec_Diff(v->dir, w->at, w->pos);
        w->ry = Math_Atan2Angle(v->dir[0], v->dir[2]);
        w->rx = Math_Atan2Angle(-v->dir[1], Vec_DistanceXZ(w->at, w->pos));
        w->unk22 = 0;
        w->unk20 = 0;
        break;
    case 1:
        v->active = 1;
        switch (area->kind) {
        case 1:
            Vec_Copy(w->pos, area->fixed.eye);
            CamPoint_GetPos(c, w->at, area);
            f = area->fixed.fov;
            w->roll = area->fixed.roll;
            break;
        case 3:
            Vec_Add(w->pos, pl->pos, area->fixed.eye);
            CamPoint_GetPos(c, w->at, area);
            f = area->fixed.fov;
            w->roll = area->fixed.roll;
            break;
        case 2:
            d = &area->path.d;
            if (c->areaChanged == 0) {
                CamCtl_PathPlace(c, segs, pl->pos, 1);
            } else {
                CamCtl_PathPlace(c, segs, pl->pos, 0);
            }
            Vec_EvalCubic(vec, &segs[c->pathPos.k], c->pathPos.dist);
            f = PathKey_Blend(d, c->pathPos.k, c->pathPos.t);
            w->roll = PathKey_BlendAngle(d, c->pathPos.k, c->pathPos.t);
            CamPoint_GetPosOr(c, w->at, area, vec);
            Path_PointAt(w->pos, c, area, segs);
            break;
        }
        Vec_Diff(v->dir, w->at, w->pos);
        if (c->areaChanged != 0) {
            w->ry = Math_Atan2Angle(v->dir[0], v->dir[2]);
            w->rx = Math_Atan2Angle(-v->dir[1], Vec_DistanceXZ(w->at, w->pos));
            w->unk22 = 0;
            w->unk20 = 0;
        } else {
            ay = Math_Atan2Angle(v->dir[0], v->dir[2]);
            ax = Math_Atan2Angle(-v->dir[1], Vec_DistanceXZ(w->at, w->pos));
            step = 52.15189f * w->fov;
            lim = 521.5189f * w->fov;
            diff = ax - w->rx;
            if (lim < diff) {
                w->rx = ax - lim;
                w->unk20 = step;
            } else if (diff < -lim) {
                w->rx = ax + lim;
                w->unk20 = -step / 2;
            }
            diff = ay - w->ry;
            lim2 = 1.4285715f * (651.8986f * w->fov);
            if (lim2 < diff) {
                w->unk22 = w->ry;
                w->ry = ay - lim2;
                w->unk22 = w->ry - w->unk22;
                w->unk22 = w->unk22 - (s16)(w->unk22 >> 2);
            } else if (diff < -lim2) {
                w->unk22 = w->ry;
                w->ry = ay + lim2;
                w->unk22 = w->ry - w->unk22;
                w->unk22 = w->unk22 - (s16)(w->unk22 >> 2);
            }
        }
        vec[0] = vec[1] = 0.0f;
        vec[2] = Vec_Length(v->dir);
        Vec_RotateX(vec, ANGLE16_TO_RAD * w->rx);
        Vec_RotateY(vec, ANGLE16_TO_RAD * w->ry);
        Vec_Add(w->at, w->pos, vec);
        break;
    }
    w->fov = f * CamArea_FovScale(area, Vec_Distance(w->at, w->pos));
    CamView_SetTransition(c, v, area);
    switch (v->blend) {
    case 0:
        Vec_Copy(v->eye, w->pos);
        Vec_Copy(v->at, w->at);
        v->roll = w->roll;
        v->fov = w->fov;
        break;
    case 1:
        Vec_Copy(v->eye0, c->out.eye);
        Vec_Copy(v->at0, c->out.at);
        v->roll0 = c->out.roll;
        v->fov0 = c->out.fov;
        (v->blend)++;
    case 2:
        t = CamBlend_EaseStep(v);
        Vec_Lerp(v->eye, v->eye0, w->pos, t);
        Vec_Lerp(v->at, v->at0, w->at, t);
        v->roll = v->roll0 * t + w->roll * (1.0f - t);
        v->fov = v->fov0 * t + w->fov * (1.0f - t);
        break;
    }
}

/* The init handler of camera mode 3: a 45-degree field of view; the zoom from 15 to 60 degrees. */
void CamAim_Init(CamCtl *c, CamView *v)
{
    f32 f;

    v->fov = 0.7853982f;
    v->roll = 0.0f;
    c->aim.fovMin = 0.2617994f;
    c->aim.fovMax = 1.0471976f;
    c->aim.fovScale = 1.2732395f;
    f = v->fov;
    c->aim.fov1 = f;
    c->aim.fov0 = f;
}

extern f32 gCamMode3Offsets[][6];
void Mtx_Copy(f32 *dst, f32 *src);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
void Vec_RotateSelf(f32 *v, f32 *m);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
s32 Bowgun_HasScope(Player *p);
s32 CamAim_IsSpecialMotion(Player *p);
void Mtx_FromAngles16Pos(f32 *m, s16 x, s16 y, f32 *pos);
s32 CamAim_Situation(Player *pl);

/* The update handler of camera mode 3: by the player's class (CamAim_Situation) a view
 * fixed to the player (its field of view zoomed by the right stick), a rotation about it,
 * or the event position; eye and target then come from gCamMode3Offsets[class]. */
void CamAim_Update(CamCtl *c, CamView *v)
{
    CamAim *w;
    Player *pl;
    f32 *a;
    f32 f;
    u16 h;
    u16 ang;
    f32 d[4];
    f32 off[4];
    f32 base[4];

    pl = c->player;
    v->active = 0;
    if (pl->aimMode == 0) {
        *(u16 *)&v->step = 0;
        return;
    }
    w = &c->aim;
    w->kind = CamAim_Situation(pl);
    switch (v->step) {
    case 0:
        w->zoom = 0;
        switch (w->kind) {
        case 0:
            if (CamAim_IsSpecialMotion(pl) == 1) {
                return;
            }
            w->unk65 = 1;
            if (Bowgun_HasScope(pl) == 1) {
                w->zoom = 1;
                w->fovMin = 0.2617994f;
                w->fovMax = 1.0471976f;
                w->fovScale = 1.2732395f;
            }
            break;
        case 1:
            w->unk65 = 0;
            w->zoom = 1;
            w->fovMin = 0.2617994f;
            w->fovMax = 1.0471976f;
            w->fovScale = 1.2732395f;
            break;
        case 2:
            w->unk65 = 0x11;
            v->fov = 0.7853982f;
            break;
        case 3:
            w->unk65 = 0;
            v->fov = 0.890118f;
            break;
        }
        v->step++;
    case 1:
        if (w->zoom != 0) {
            h = c->padMagR;
            if (h > 0x50) {
                if (w->kind == 0) {
                    a = &w->fov0;
                } else {
                    a = &w->fov1;
                }
                if (h > 0x70) {
                    f = 0.030543262f;
                } else {
                    f = h - 0x50;
                    f *= 0.00095447694f;
                }
                ang = c->padDegR;
                if ((u16)(ang - 0x4000) <= 0x6000) {
                    *a -= f;
                    if (*a < w->fovMin) {
                        *a = w->fovMin;
                    }
                } else if ((u16)(ang - 0x8000 - 0x6000) <= 0x6000) {
                    *a += f;
                    if (*a > w->fovMax) {
                        *a = w->fovMax;
                    }
                }
            }
        }
        switch (w->kind) {
        case 0:
            if (CamAim_IsSpecialMotion(pl) == 0) {
                Vec_Copy(w->pos, pl->pos);
                Mtx_Copy(w->mtx, (f32 *)((u8 *) pl->partRecs[14] + 0x40));
            } else {
                Vec_Diff(d, pl->pos, w->pos);
                Vec_Copy(w->pos, pl->pos);
                Vec_Add(&w->mtx[12], &w->mtx[12], d);
            }
            v->fov = w->fov0;
            break;
        case 1:
            Mtx_FromAngles16Pos(w->mtx, pl->aimPitch, (s16)(u16)(pl->rot[1] + 0x8000), pl->pos);
            v->fov = w->fov1;
            break;
        case 2:
            Mtx_FromAngles16Pos(w->mtx, pl->aimPitch, (s16)(u16)(pl->rot[1] + 0x8000), pl->pos);
            break;
        case 3:
            Mtx_FromAngles16Pos(w->mtx, w->rx, w->ry, w->pos);
            break;
        }
    default:
        Mtx_GetTrans(base, w->mtx);
        off[0] = gCamMode3Offsets[w->kind][0];
        off[1] = 0.0f;
        off[2] = gCamMode3Offsets[w->kind][2];
        Vec_RotateSelf((f32 *) off, w->mtx);
        Vec_Add(v->eye, base, off);
        v->eye[1] += gCamMode3Offsets[w->kind][1];
        Vec_Rotate(off, &gCamMode3Offsets[w->kind][3], w->mtx);
        Vec_Add(v->at, base, off);
        v->at[1] += gCamMode3Offsets[w->kind][1];
        v->active = 1;
    }
}

/* 1 for the player motions 0x3EA, 0x3F2, 0x579, 0x57C, 0x57D, 0x580, 0x583, 0x58E, 0x58F. */
s32 CamAim_IsSpecialMotion(Player *p)
{
    switch (p->motionId[0]) {
    case 0x3EA:
    case 0x579:
    case 0x57C:
    case 0x583:
    case 0x57D:
    case 0x580:
    case 0x3F2:
    case 0x58E:
    case 0x58F:
        return 1;
    }
    return 0;
}

/* m = a rotation by the 16-bit angles x and y, and the translation pos. */
void Mtx_FromAngles16Pos(f32 *m, s16 x, s16 y, f32 *pos)
{
    Mtx_Identity(m);
    Mtx_RotateXYZ(m, ANGLE16_TO_RAD * (f32)x, ANGLE16_TO_RAD * (f32)y, 0.0f);
    Vec_Copy(&m[12], pos);
}

/* A class of the player's situation: 3 online, 1 in state 0/0x65 or
 * 0/0x66, 2 when Player_OnMountedGun is 1, 0 when +2 is 1 or 5, else -1. */
s32 CamAim_Situation(Player *pl)
{
    if (gScene.online != 0) {
        return 3;
    }
    if (Player_IsState(pl, 0, 0x65) != 0 || Player_IsState(pl, 0, 0x66) != 0) {
        return 1;
    }
    if (Player_OnMountedGun(pl) == 1) {
        return 2;
    }
    if (pl->weaponType == 1 || pl->weaponType == 5) {
        return 0;
    }
    return -1;
}

/* The aim camera's state: *a its unk65, *b whether it zooms; when it does, *t the zoom
 * (the aim view's field of view from fovMin, 0 to 1). Returns the class. */
s8 CamAim_GetState(s32 unused, u8 *a, u8 *b, f32 *t)
{
    CamView *g = &gCamCtl.modes[3];
    CamAim *e = (CamAim *)&gCamCtl.aim;

    *a = e->unk65;
    *b = e->zoom;
    if (e->zoom != 0) {
        *t = g->fov - e->fovMin;
        *t *= e->fovScale;
    }
    return e->kind;
}

/* Sets the aim camera's position and angles (class 3). */
void CamAim_Set(f32 *pos, s16 a, s16 b)
{
    CamAim *e = (CamAim *)&gCamCtl.aim;

    Vec_Copy(e->pos, pos);
    e->rx = a;
    e->ry = b;
}

/* The init handler of camera mode 4 (its work c->fixed and the focus; the bytes are cleared as words). */
void CamFixed_Init(CamCtl *c, CamView *v)
{
    v->fov = 0.9599311f;
    v->roll = 0.0f;
    *(s32 *)&c->fixed.started = 0;
    *(s32 *)&c->fixed.focus.state = 0;
    c->fixed.focus.on = 0;
}

/* The update handler of camera mode 4. */
void CamFixed_Update(CamCtl *c, CamView *v)
{
    CamFixed *w = &c->fixed;

    CamFixed_UpdateTarget(c, v, w);
    if (v->active == 0) {
        CamFocus_Update(c, v, &w->focus);
    }
}

/* Camera mode 4's target step: w->on gets Cam_PlayerHasFlag80000 of the player
 * (off: w->started = 0); the first time, w->spot from the player's action spot
 * and a 55-degree view; v->active is set when CamFixed_AreaCamera succeeds. */
void CamFixed_UpdateTarget(CamCtl *c, CamView *v, CamFixed *w)
{
    Player *pl;
    s32 r;

    v->active = 0;
    pl = c->player;
    r = Cam_PlayerHasFlag80000(pl);
    w->on = r;
    if (r == 0) {
        w->started = 0;
        return;
    }
    if (w->started == 0) {
        w->started++;
        w->spot = pl->spot;
        v->fov = 0.9599311f;
        v->roll = 0.0f;
    }
    if (CamFixed_AreaCamera(c, v, w) >= 0) {
        v->active = 1;
    }
}

/* The area's fixed camera (gAreaCameraIdx by area, 0xFF: none): eye and target from
 * gCamMode4AreaPos turned by the target object's heading, the fov from gCamMode4AreaFovs. */
s32 CamFixed_AreaCamera(CamCtl *c, CamView *v, CamFixed *w)
{
    f32 d[3];
    u8 *obj;
    f32 *pos;
    f32 a;
    u8 k;

    obj = (u8 *)c->player;
    if (w->spot == NULL) {
        return -1;
    }
    k = gAreaCameraIdx[gScene.area];
    if (k == 0xFF) {
        return -1;
    }
    pos = gCamMode4AreaPos[k];
    a = ANGLE16_TO_RAD * (s32)*(u16 *)(obj + 0xE);
    v->fov = gCamMode4AreaFovs[k];
    Vec_Copy(d, pos);
    Vec_RotateY(d, a);
    Vec_Add(v->eye, (f32 *)(obj + 0xAC), d);
    Vec_Copy(d, pos + 3);
    Vec_RotateY(d, a);
    Vec_Add(v->at, (f32 *)(obj + 0xAC), d);
    return 0;
}

/* Starts a camera focus on obj (its position, 20 degrees), or without one
 * at 120 and 300 (30 degrees). */
void CamFocus_Start(Em *obj)
{
    CamFocus *f = (CamFocus *)&gCamCtl.fixed.focus;

    f->state = 0;
    f->on = 1;
    f->obj = obj;
    f->aside = 0;
    if (obj != NULL) {
        f->kind = 0;
        Vec_Copy(f->pos, &obj->posX);
        f->angle = 0.34906587f;
    } else {
        f->kind = 1;
        f->height = 120.0f;
        f->dist = 300.0f;
        f->angle = 0.5235988f;
    }
}

void CamFocus_End(void)
{
    gCamCtl.fixed.focus.on = 0;
}

s32 CamFocus_IsOn(void)
{
    return gCamCtl.fixed.focus.on != 0;
}

void CamFocus_SetAside(s8 arg0)
{
    gCamCtl.fixed.focus.aside = arg0;
}

/* The camera focus state: 1 while it runs (state 1 before 20 frames, or
 * state 2), -1 after, 0 when idle. */
s32 CamFocus_GetState(void)
{
    CamFocus *f = (CamFocus *)&gCamCtl.fixed.focus;

    switch (f->state) {
    case 1:
        if (f->timer >= 20) {
            return -1;
        }
    case 2:
        return 1;
    }
    return 0;
}

/* Frame n of 20 as 0..1, eased: 1 out, 2 in (power 2.25), else linear. */
f32 CamFocus_Ease(s32 unused, s16 n, u8 ease)
{
    f32 t;

    t = 0.05f * (f32)n;
    switch (ease) {
    case 1:
        t = 1.0f - Math_Pow(1.0f - t, 2.25f);
        break;
    case 2:
        t = Math_Pow(t, 2.25f);
        break;
    }
    return t;
}

void Vec_Lerp(f32 *out, f32 *a, f32 *b, f32 t);
s32 Npc_IsAct4For50(Em *em);
s32 Chat_InInputMode(void);
u16 Pad_StickToDpad(u8 pad);
void Vec_Resize(f32 *out, f32 len, f32 *v);
f32 CamFocus_Ease(s32 unused, s16 n, u8 ease);

/* The camera focus: step 0 waits for a request (+0x2E), steps 1 and 2 ease in and out
 * over 20 frames; the view is lerped from the current mode's view to the focus (kind
 * +0x2F: 0 the player and an object or +0x00, 1 the player with pad-adjusted height and
 * distance, 2 the position +0x00). */
void CamFocus_Update(CamCtl *c, CamView *v, CamFocus *f)
{
    Player *pl;
    CamView *src;
    f32 t;
    u16 btn;
    f32 d[4];

    v->active = 0;
    switch (f->state) {
    case 0:
        if (f->on != 0) {
            f->timer = 0;
            f->state++;
            if (f->aside != 0) {
                f->asideTimer = 0x14;
            }
        }
        return;
    case 1:
        if (f->timer < 0x14) {
            f->timer++;
        }
        if (f->on == 0) {
            f->state++;
        }
        break;
    case 2:
        if (f->timer > 0) {
            f->timer--;
        }
        if (f->on != 0) {
            f->state = 1;
            break;
        }
        if (f->timer == 0) {
            f->state = 0;
            return;
        }
        break;
    }
    pl = c->player;
    src = &c->modes[c->areaMode];
    t = CamFocus_Ease((s32)f, f->timer, f->state);
    switch (f->kind) {
    case 0:
        if (Npc_IsAct4For50(f->obj) == 1) {
            Vec_Lerp(f->at, pl->pos, &f->obj->posX, 0.5f);
            f->at[1] += 64.0f;
        } else {
            Vec_Lerp(f->at, pl->pos, f->pos, 0.2f);
            f->at[1] += 100.0f;
            if (f->aside != 0) {
                if (f->asideTimer < 0x14) {
                    f->asideTimer++;
                }
            } else {
                if (f->asideTimer > 0) {
                    f->asideTimer--;
                }
            }
            if (f->asideTimer > 0) {
                Vec_Diff(d, f->pos, src->eye);
                Vec_RotateY(d, -(0.45f * v->fov));
                d[0] += src->eye[0];
                d[1] = 100.0f + f->pos[1];
                d[2] += src->eye[2];
                Vec_Lerp(f->at, d, f->at, CamFocus_Ease((s32)f, f->asideTimer, f->aside != 0 ? 1 : 2));
            }
        }
        Vec_Copy(v->eye, src->eye);
        break;
    case 1:
        if (Chat_InInputMode() != 1) {
            btn = Pad_StickToDpad(0);
            if (btn & 0x2000) {
                f->height += 2.0f;
                if (f->height > 160.0f) {
                    f->height = 160.0f;
                }
            }
            if (btn & 0x1000) {
                f->height -= 2.0f;
                if (f->height < 40.0f) {
                    f->height = 40.0f;
                }
            }
            if (btn & 0x800) {
                f->dist -= 8.0f;
                if (f->dist < 120.0f) {
                    f->dist = 120.0f;
                }
            }
            if (btn & 0x400) {
                f->dist += 8.0f;
                if (f->dist > 375.0f) {
                    f->dist = 375.0f;
                }
            }
        }
        Vec_Copy(f->at, pl->pos);
        f->at[1] += f->height;
        Vec_Diff(d, src->eye, f->at);
        Vec_Resize(d, f->dist, d);
        Vec_Add(d, f->at, d);
        Vec_Lerp(v->eye, d, src->eye, t);
        break;
    case 2:
        Vec_Lerp(v->eye, f->pos, src->eye, t);
        break;
    }
    Vec_Lerp(v->at, f->at, src->at, t);
    v->fov = f->angle * t + src->fov * (1.0f - t);
    v->roll = src->roll;
    v->active = 1;
}
