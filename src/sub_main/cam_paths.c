/*
 * The camera areas' look points and paths, which the rail camera follows:
 * an area's look point (CamPoint_GetPos, CamPoint_GetPosOr), the point at
 * the camera's place on an area's path (Path_PointAt), and the place
 * itself (PathPos_*: the nearest segment, the walk along the keys,
 * CamCtl_PathPlace). A path's segments are cubics through its keys
 * (Path_BuildSpline, solving a tridiagonal system); the nearest point of
 * a segment comes from the real roots of a quintic (PathSeg_Nearest,
 * Quintic_Roots on complex numbers, Cubic_Roots). CamArea_FovScale gives
 * an area's fov scale by distance, and PathKey_Blend and
 * PathKey_BlendAngle a path's values and angles between two keys. The
 * data: the quintic solver's starting points.
 */

#include "common.h"
#include "camera.h"
#include "em.h"
#include "player.h"
#include "engine_math.h"
#include "vec_math.h"

/*
 * Puts the look point of camera area `area` (CAM_AREA_FIXED and CAM_AREA_OFFSET, CamAreaFixed)
 * into out. By area->lookSpace: 0 fixed.at in the world, 1 fixed.atLocal
 * through the player's worldMtx, 2 the player's pos plus fixed.atOffset.
 * Then by area->lookLock: 0 takes fixed.at's y, 1 its x and z, 2 none.
 * The player is ctl->player. Seen: CamRail_Update.
 */
void CamPoint_GetPos(CamCtl *ctl, f32 *out, CamArea *area)
{
    Player *pl;
    u8 space;
    u8 lockMode;

    pl = ctl->player;
    space = area->lookSpace;
    switch (space) {
    case CAM_LOOK_SPACE_WORLD:
        Vec_Copy(out, area->fixed.at);
        break;
    case CAM_LOOK_SPACE_PLAYER:
        Vec_Transform(out, area->fixed.atLocal, pl->worldMtx[0]);
        break;
    case CAM_LOOK_SPACE_OFFSET:
        Vec_Add(out, pl->pos, area->fixed.atOffset);
        break;
    }
    lockMode = area->lookLock;
    switch (lockMode) {
    case CAM_LOOK_LOCK_Y:
        out[1] = area->fixed.at[1];
        break;
    case CAM_LOOK_LOCK_XZ:
        out[0] = area->fixed.at[0];
        out[2] = area->fixed.at[2];
        break;
    }
}

/*
 * CamPoint_GetPos for a camera area of kind CAM_AREA_PATH (CamAreaPath), with
 * pathPoint in place of the area's world point: lookSpace 0 copies
 * pathPoint, 1 is path.atLocal through the player's worldMtx, 2 the
 * player's pos plus path.atOffset; lookLock 0 takes pathPoint's y, 1 its
 * x and z. Seen: CamRail_Update passes the point of the area's second
 * key set (keys2) where the player is placed.
 */
void CamPoint_GetPosOr(CamCtl *ctl, f32 *out, CamArea *area, f32 *pathPoint)
{
    Player *pl;
    u8 space;
    u8 lockMode;

    pl = ctl->player;
    space = area->lookSpace;
    switch (space) {
    case 0:
        Vec_Copy(out, pathPoint);
        break;
    case 1:
        Vec_Transform(out, area->path.atLocal, pl->worldMtx[0]);
        break;
    case 2:
        Vec_Add(out, pl->pos, area->path.atOffset);
        break;
    }
    lockMode = area->lookLock;
    switch (lockMode) {
    case 0:
        out[1] = pathPoint[1];
        break;
    case 1:
        out[0] = pathPoint[0];
        out[2] = pathPoint[2];
        break;
    }
}

/*
 * Puts into out the point of area's eye path (path.d.keys) at ctl's path
 * position: builds that path's segments into segs (Path_BuildSpline) and
 * evaluates segment ctl->pathPos.k at pathPos.t times the key's len
 * (Vec_EvalCubic). pathPos is placed on the second key set (keys2), so t
 * carries the place over from that path to this one.
 */
void Path_PointAt(f32 *out, CamCtl *ctl, CamArea *area, PathSeg *segs)
{
    u8 segNo;

    Path_BuildSpline(segs, area->path.d.keys, area->path.d.count);
    segNo = ctl->pathPos.k;
    Vec_EvalCubic(out, &segs[segNo], ctl->pathPos.t * area->path.d.keys[segNo].len);
}

/*
 * The segment of path nearest target: from the key of keys2 nearest to
 * it, or the one before when the key before is nearer than the key after;
 * the last key gives count - 2. The walk steps 12 bytes through the
 * 16-byte keys2 records (as the game's code does), so only key 0's position
 * is read whole.
 */
s32 Path_NearestSeg(PathData *path, f32 *target)
{
    f32 dist[16];
    f32 *keyPos = path->keys2[0].pos;
    f32 *distPtr = dist;
    s32 i;
    s32 best = 0;
    f32 minDist = 1e7f;

    for (i = 0; i < path->count; i++) {
        *distPtr = Vec_Distance(target, keyPos);
        if (minDist > *distPtr) {
            minDist = *distPtr;
            best = i;
        }
        keyPos += 3;
        distPtr++;
    }
    if (best != 0 && best < path->count - 1) {
        if (dist[best - 1] < dist[best + 1]) {
            best--;
        }
    } else if (best >= path->count - 1) {
        best = path->count - 2;
    }
    return best;
}

/*
 * Places pos->dist on the segment that starts at key (its length
 * key->len) from the candCount candidate distances cands (PathSeg_Nearest's
 * roots), in order: the first one within 0..len is taken (returns 0);
 * else pos->dist is clamped to the end nearest the closest candidate, 0
 * (returns -1) or len (returns 1). seg is not read.
 */
s32 PathPos_PlaceOnSeg(PathPos *pos, PathSeg *seg, PathKey *key, f32 *cands, s32 candCount)
{
    f32 best;
    f32 cand;
    f32 first;
    s32 side;

    first = *cands;
    if (first < 0.0f) {
        pos->dist = 0.0f;
        side = -1;
        best = -first;
    } else if (!(first <= key->len)) {
        best = first - key->len;
        side = 1;
        pos->dist = key->len;
    } else {
        side = 0;
        pos->dist = first;
        return side;
    }
    if (candCount == 1) {
        return side;
    }
    for (candCount--; candCount != 0; candCount--) {
        cand = *++cands;
        if (cand < 0.0f) {
            if (!(best <= -cand)) {
                best = -cand;
                side = -1;
                pos->dist = 0.0f;
            }
        } else if (!(cand <= key->len)) {
            cand -= key->len;
            if (!(best <= cand)) {
                pos->dist = key->len;
                side = 1;
                best = cand;
            }
        } else {
            side = 0;
            pos->dist = cand;
            return side;
        }
    }
    return side;
}

/*
 * Builds the segments of path's second key set (keys2) into segs and
 * places pos->dist on segment pos->k at the point nearest target
 * (PathSeg_Nearest, with path->nearest3D as its axes, then
 * PathPos_PlaceOnSeg). Past the segment's end it moves to the next
 * segment (while pos->k < count - 2), past its start to the one before
 * (pos->k > 0), when that segment has candidates. Returns 1; 0 with
 * pos->dist cleared when segment pos->k has no candidate.
 */
s32 PathPos_Build(PathPos *pos, PathSeg *segs, PathData *path, f32 *target)
{
    f32 cands[8];
    s32 candCount;
    s32 side;

    Path_BuildSpline(segs, path->keys2, path->count);
    candCount = PathSeg_Nearest(cands, &segs[pos->k], target, path->nearest3D);
    if (candCount == 0) {
        pos->dist = 0.0f;
        return 0;
    }
    side = PathPos_PlaceOnSeg(pos, &segs[pos->k], &path->keys2[pos->k], cands, candCount);
    if (side > 0) {
        if (pos->k < path->count - 2) {
            candCount = PathSeg_Nearest(cands, &segs[pos->k + 1], target, path->nearest3D);
            if (candCount != 0) {
                pos->k++;
                PathPos_PlaceOnSeg(pos, &segs[pos->k], &path->keys2[pos->k], cands, candCount);
            }
        }
    } else if (side < 0) {
        if (pos->k > 0) {
            candCount = PathSeg_Nearest(cands, &segs[pos->k - 1], target, path->nearest3D);
            if (candCount != 0) {
                pos->k--;
                PathPos_PlaceOnSeg(pos, &segs[pos->k], &path->keys2[pos->k], cands, candCount);
            }
        }
    }
    return 1;
}

/*
 * Moves pos along path's second key set (keys2) by at most 0.5 toward
 * distance goalDist on segment goalSeg, crossing segment ends on the way.
 * Going back, each segment entered starts at keys2[goalSeg].len, the
 * goal segment's length, as the code reads it. Seen: only
 * PathPos_MoveToward calls it, when goalSeg is another segment.
 */
void PathPos_MoveOnSeg(PathPos *pos, PathData *path, s32 goalSeg, f32 goalDist)
{
    f32 step = 0.5f;
    f32 rem;
    f32 cur;

    if (pos->k < goalSeg) {
        do {
            cur = pos->dist;
            rem = path->keys2[pos->k].len - cur;
            if (rem <= step) {
                step -= rem;
                pos->k++;
                pos->dist = 0.0f;
            } else {
                pos->dist = cur + step;
                return;
            }
        } while (pos->k < goalSeg);
        if (goalDist - pos->dist <= step) {
            pos->dist = goalDist;
        } else {
            pos->dist = pos->dist + step;
        }
    } else {
        do {
            if (pos->dist <= step) {
                step -= pos->dist;
                pos->k--;
                pos->dist = path->keys2[goalSeg].len;
            } else {
                pos->dist = pos->dist - step;
                return;
            }
        } while (pos->k > goalSeg);
        if (pos->dist - goalDist <= step) {
            pos->dist = goalDist;
        } else {
            pos->dist = pos->dist - step;
        }
    }
}

/*
 * Moves pos along path toward target: finds the place nearest target
 * (Path_NearestSeg, PathPos_Build), then steps pos->dist toward it by at
 * most 0.5 on the same segment, or with PathPos_MoveOnSeg across
 * segments, and sets pos->t to dist over the segment's keys2 len.
 * Returns 0; -1 with pos unchanged when no place is found.
 */
s32 PathPos_MoveToward(PathPos *pos, PathData *path, PathSeg *segs, f32 *target)
{
    PathPos goal;

    goal.k = Path_NearestSeg(path, target);
    if (PathPos_Build(&goal, segs, path, target) != 0) {
        if (pos->k == goal.k) {
            if (pos->dist < goal.dist) {
                if (goal.dist - pos->dist <= 0.5f) {
                    pos->dist = goal.dist;
                } else {
                    pos->dist = pos->dist + 0.5f;
                }
            } else if (!(pos->dist <= goal.dist)) {
                if (pos->dist - goal.dist <= 0.5f) {
                    pos->dist = goal.dist;
                } else {
                    pos->dist = pos->dist - 0.5f;
                }
            }
        } else {
            PathPos_MoveOnSeg(pos, path, goal.k, goal.dist);
        }
    } else {
        return -1;
    }
    pos->t = pos->dist / path->keys2[pos->k].len;
    return 0;
}

/*
 * Places pos on path at the place nearest target: the segment from
 * Path_NearestSeg, the distance from PathPos_Build, t the distance over
 * the segment's keys2 len. Returns 0; -1 with t and dist cleared when
 * PathPos_Build finds no place.
 */
s32 PathPos_PlaceNear(PathPos *pos, PathData *path, PathSeg *segs, f32 *target)
{
    pos->k = Path_NearestSeg(path, target);
    if (PathPos_Build(pos, segs, path, target) != 0) {
        pos->t = pos->dist / path->keys2[pos->k].len;
        return 0;
    }
    pos->t = 0.0f;
    pos->dist = 0.0f;
    return -1;
}

/*
 * Places ctl->pathPos on the current area's path (area->path.d) near
 * pos: at once when moveToward is 0 (PathPos_PlaceNear), else by one step
 * toward it (PathPos_MoveToward); segs receives the path's segments.
 * Returns their result; -1 when ctl->area is NULL. Seen: CamRail_Update
 * passes the player's position, moveToward 1 while the area is unchanged.
 */
s32 CamCtl_PathPlace(CamCtl *ctl, PathSeg *segs, f32 *pos, s32 moveToward)
{
    CamArea *area;
    s32 ret;

    area = ctl->area;
    if (area == NULL) {
        return -1;
    }
    if (moveToward == 0) {
        ret = PathPos_PlaceNear(&ctl->pathPos, &area->path.d, segs, pos);
    } else {
        ret = PathPos_MoveToward(&ctl->pathPos, &area->path.d, segs, pos);
    }
    return ret;
}

/*
 * out = ((c[0] t + c[1]) t + c[2]) t + c[3]: the point of segment seg at
 * t. Path_BuildSpline's segments take t from 0 to the key's len (seen:
 * PathPos.dist; Path_PointAt's t times len).
 */
void Vec_EvalCubic(f32 *out, PathSeg *seg, f32 frac)
{
    Vec_Scale(out, seg->c[0], frac);
    Vec_Add(out, out, seg->c[1]);
    Vec_Scale(out, out, frac);
    Vec_Add(out, out, seg->c[2]);
    Vec_Scale(out, out, frac);
    Vec_Add(out, out, seg->c[3]);
}

/* lhs . rhs in the x-z plane. */
static f32 PathVec_DotXZ(f32 *lhs, f32 *rhs)
{
    return lhs[2] * rhs[2] + lhs[0] * rhs[0];
}

/* lhs . rhs. */
static f32 PathVec_Dot(f32 *lhs, f32 *rhs)
{
    return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}

#define LEN2(v) ((v)[0] * (v)[0] + (v)[1] * (v)[1] + (v)[2] * (v)[2])

/*
 * The parameters t where segment seg comes nearest target: the real roots
 * of the derivative of the squared distance (halved), into cands; returns
 * how many (0 to 5). axes 0 measures in the x-z plane (PathVec_DotXZ),
 * else in 3D (PathVec_Dot). Coefficients with a squared length up to
 * 1e-10 (always in 3D) lower the degree: a quintic (Quintic_Roots, the
 * roots with an imaginary part under 0.001 kept), a cubic (Cubic_Roots)
 * or a line (one root); 0 when the segment is a point.
 */
s32 PathSeg_Nearest(f32 *cands, PathSeg *seg, f32 *target, s32 axes)
{
    f32 coefs[6];
    Complex roots[5];
    f32 offset[3];
    f32 point[3];
    f32 termA;
    f32 termB;
    s32 count;
    u32 i;

    Vec_Copy(point, target);
    if (axes == 0) {
        point[1] = 0.0f;
        Vec_Diff(offset, seg->c[3], point);
        if (LEN2(seg->c[0]) <= 1e-10f) {
            if (LEN2(seg->c[1]) <= 1e-10f) {
                if (LEN2(seg->c[2]) <= 1e-10f) {
                    return 0;
                }
                termA = PathVec_DotXZ(seg->c[2], seg->c[2]);
                cands[0] = -(PathVec_DotXZ(seg->c[2], offset) / termA);
                return 1;
            }
            coefs[0] = 2.0f * PathVec_DotXZ(seg->c[1], seg->c[1]);
            coefs[1] = 3.0f * PathVec_DotXZ(seg->c[1], seg->c[2]);
            termA = 2.0f * PathVec_DotXZ(seg->c[1], offset);
            coefs[2] = termA + PathVec_DotXZ(seg->c[2], seg->c[2]);
            coefs[3] = PathVec_DotXZ(seg->c[2], offset);
            return Cubic_Roots(cands, coefs);
        }
        coefs[0] = 3.0f * PathVec_DotXZ(seg->c[0], seg->c[0]);
        coefs[1] = 5.0f * PathVec_DotXZ(seg->c[0], seg->c[1]);
        termA = 4.0f * PathVec_DotXZ(seg->c[0], seg->c[2]);
        coefs[2] = termA + 2.0f * PathVec_DotXZ(seg->c[1], seg->c[1]);
        termA = PathVec_DotXZ(seg->c[1], seg->c[2]);
        termB = PathVec_DotXZ(seg->c[0], offset);
        coefs[3] = 3.0f * (termB + termA);
        termA = 2.0f * PathVec_DotXZ(seg->c[1], offset);
        coefs[4] = termA + PathVec_DotXZ(seg->c[2], seg->c[2]);
        coefs[5] = PathVec_DotXZ(seg->c[2], offset);
        Quintic_Roots(roots, coefs);
        for (i = count = 0; i < 5; i++) {
            if (Math_Absf(roots[i].y) < 0.001f) {
                count++;
                *cands++ = roots[i].x;
            }
        }
    } else {
        Vec_Diff(offset, seg->c[3], point);
        if (LEN2(seg->c[0]) <= 1e-10f) {
            if (LEN2(seg->c[1]) <= 1e-10f) {
                if (LEN2(seg->c[2]) <= 1e-10f) {
                    return 0;
                }
                termA = PathVec_Dot(seg->c[2], seg->c[2]);
                cands[0] = -(PathVec_Dot(seg->c[2], offset) / termA);
                return 1;
            }
            coefs[0] = 2.0f * PathVec_Dot(seg->c[1], seg->c[1]);
            coefs[1] = 3.0f * PathVec_Dot(seg->c[1], seg->c[2]);
            termA = 2.0f * PathVec_Dot(seg->c[1], offset);
            coefs[2] = termA + PathVec_Dot(seg->c[2], seg->c[2]);
            coefs[3] = PathVec_Dot(seg->c[2], offset);
            return Cubic_Roots(cands, coefs);
        }
        coefs[0] = 3.0f * PathVec_Dot(seg->c[0], seg->c[0]);
        coefs[1] = 5.0f * PathVec_Dot(seg->c[0], seg->c[1]);
        termA = 4.0f * PathVec_Dot(seg->c[0], seg->c[2]);
        coefs[2] = termA + 2.0f * PathVec_Dot(seg->c[1], seg->c[1]);
        termA = PathVec_Dot(seg->c[1], seg->c[2]);
        termB = PathVec_Dot(seg->c[0], offset);
        coefs[3] = 3.0f * (termB + termA);
        termA = 2.0f * PathVec_Dot(seg->c[1], offset);
        coefs[4] = termA + PathVec_Dot(seg->c[2], seg->c[2]);
        coefs[5] = PathVec_Dot(seg->c[2], offset);
        Quintic_Roots(roots, coefs);
        for (i = count = 0; i < 5; i++) {
            if (Math_Absf(roots[i].y) < 0.001f) {
                count++;
                *cands++ = roots[i].x;
            }
        }
    }
    return count;
}

/*
 * The fov's scale for area at distance dist (seen: the eye's distance from
 * the target, CamRail_Update): fovScale[0] up to fovDist[0], fovScale[1]
 * from fovDist[1], linear between. The mean of the two for equal
 * distances is reached only by a NaN dist (the tests before catch the
 * rest).
 */
f32 CamArea_FovScale(CamArea *area, f32 dist)
{
    f32 scale;
    f32 farDist;
    f32 nearDist;

    nearDist = area->fovDist[0];
    if (dist <= nearDist) {
        return area->fovScale[0];
    }
    farDist = area->fovDist[1];
    if (!(dist < farDist)) {
        return area->fovScale[1];
    }
    if (farDist == nearDist) {
        return 0.5f * (area->fovScale[0] + area->fovScale[1]);
    }
    scale = (area->fovScale[1] - area->fovScale[0]) * (dist - nearDist);
    scale /= farDist - nearDist;
    scale += area->fovScale[0];
    return scale;
}

/*
 * path->vals[keyNo] blended with vals[keyNo + 1] by frac (0 to 1). Seen:
 * CamRail_Update's fov on a path, at pathPos.k and pathPos.t.
 */
f32 PathKey_Blend(PathData *path, s32 keyNo, f32 frac)
{
    return path->vals[keyNo] * (1.0f - frac) + path->vals[keyNo + 1] * frac;
}

/*
 * path->angles[keyNo] blended with angles[keyNo + 1] by frac, in radians
 * (ANGLE16_TO_RAD): the difference times frac in 16.16 fixed point, whose
 * upper half (through prodHigh) gets angles[keyNo] added, so the result
 * wraps as a 16-bit angle. Seen: CamRail_Update's roll on a path.
 */
f32 PathKey_BlendAngle(PathData *path, s32 keyNo, f32 frac)
{
    s32 prod;
    s16 *prodHigh = (s16 *)&prod + 1;

    prod = (s32)(65536.0f * frac) * (path->angles[keyNo + 1] - path->angles[keyNo]);
    *prodHigh += path->angles[keyNo];
    return ANGLE16_TO_RAD * *prodHigh;
}

/*
 * Solves the tridiagonal system sub[i] sol[i-1] + diag[i] sol[i] +
 * super[i] sol[i+1] = rhs[i] for i < count (count <= 64) into sol (the
 * Thomas algorithm); a zero pivot is kept as 0 instead of inverted.
 */
static void Tridiag_Solve(f32 *sol, f32 *sub, f32 *diag, f32 *super, f32 *rhs, s32 count)
{
    f32 gamma[64];
    f32 invPivot;
    s32 i;

    invPivot = diag[0];
    if (invPivot != 0.0f) {
        invPivot = 1.0f / invPivot;
    }
    sol[0] = rhs[0] * invPivot;
    for (i = 1; i < count; i++) {
        gamma[i - 1] = invPivot * super[i - 1];
        invPivot = diag[i] - sub[i] * gamma[i - 1];
        if (invPivot != 0.0f) {
            invPivot = 1.0f / invPivot;
        }
        sol[i] = invPivot * (rhs[i] - sub[i] * sol[i - 1]);
    }
    for (i = count - 2; i >= 0; i--) {
        sol[i] -= gamma[i] * sol[i + 1];
    }
}

/*
 * Builds the keyCount - 1 cubic segments through the path keys into seg:
 * a cubic Hermite spline whose parameter runs from 0 to each key's len,
 * with natural ends. mat's rows 0-2 hold the tridiagonal system (sub,
 * diag, super), 3-5 the right sides for x, y and z, 6-8 the tangents
 * Tridiag_Solve finds, one axis at a time. Segment i: c[3] the key's pos,
 * c[2] its tangent, c[1] and c[0] from the next key and its tangent.
 */
void Path_BuildSpline(PathSeg *seg, PathKey *keys, s32 keyCount)
{
    f32 mat[9][16]; /* rows 0-2 the system (a, b, c), 3-5 the right sides and 6-8 the tangents (x, y, z) */
    f32 *col;
    s32 i;
    f32 ratioIn;
    f32 ratioOut;
    f32 invLen;
    f32 invLenSq;
    f32 invLenCube;

    col = mat[0];
    col[0] = 0.0f;
    col[1 * 16] = keys[0].len + keys[0].len;
    col[2 * 16] = keys[0].len;
    col[3 * 16] = 3.0f * (keys[1].pos[0] - keys[0].pos[0]);
    col[4 * 16] = 3.0f * (keys[1].pos[1] - keys[0].pos[1]);
    col[5 * 16] = 3.0f * (keys[1].pos[2] - keys[0].pos[2]);
    for (i = 1; i < keyCount - 1; i++) {
        ratioIn = keys[i].len / keys[i - 1].len;
        ratioOut = keys[i - 1].len / keys[i].len;
        col[i] = keys[i].len;
        col[1 * 16 + i] = 2.0f * (keys[i - 1].len + keys[i].len);
        col[2 * 16 + i] = keys[i - 1].len;
        col[3 * 16 + i] = 3.0f * (ratioIn * (keys[i].pos[0] - keys[i - 1].pos[0]) + ratioOut * (keys[i + 1].pos[0] - keys[i].pos[0]));
        col[4 * 16 + i] = 3.0f * (ratioIn * (keys[i].pos[1] - keys[i - 1].pos[1]) + ratioOut * (keys[i + 1].pos[1] - keys[i].pos[1]));
        col[5 * 16 + i] = 3.0f * (ratioIn * (keys[i].pos[2] - keys[i - 1].pos[2]) + ratioOut * (keys[i + 1].pos[2] - keys[i].pos[2]));
    }
    col[keyCount - 1] = keys[keyCount - 2].len;
    col[1 * 16 + keyCount - 1] = keys[keyCount - 2].len + keys[keyCount - 2].len;
    col[2 * 16 + keyCount - 1] = 0.0f;
    col[3 * 16 + keyCount - 1] = 3.0f * (keys[keyCount - 1].pos[0] - keys[keyCount - 2].pos[0]);
    col[4 * 16 + keyCount - 1] = 3.0f * (keys[keyCount - 1].pos[1] - keys[keyCount - 2].pos[1]);
    col[5 * 16 + keyCount - 1] = 3.0f * (keys[keyCount - 1].pos[2] - keys[keyCount - 2].pos[2]);
    Tridiag_Solve(col + 6 * 16, col, col + 16, col + 2 * 16, col + 3 * 16, keyCount);
    Tridiag_Solve(col + 7 * 16, col, col + 16, col + 2 * 16, col + 4 * 16, keyCount);
    Tridiag_Solve(col + 8 * 16, col, col + 16, col + 2 * 16, col + 5 * 16, keyCount);
    for (i = 0; i < keyCount - 1; i++) {
        invLen = 1.0f / keys->len;
        invLenSq = invLen * invLen;
        invLenCube = 1.0f / Math_Pow(keys->len, 3.0f);
        seg->c[0][0] = invLenSq * (col[6 * 16] + col[6 * 16 + 1]) + invLenCube * (2.0f * (keys[0].pos[0] - keys[1].pos[0]));
        seg->c[1][0] = invLenSq * (3.0f * (keys[1].pos[0] - keys[0].pos[0])) - invLen * (2.0f * col[6 * 16] + col[6 * 16 + 1]);
        seg->c[2][0] = col[6 * 16];
        seg->c[3][0] = keys[0].pos[0];
        seg->c[0][1] = invLenSq * (col[7 * 16] + col[7 * 16 + 1]) + invLenCube * (2.0f * (keys[0].pos[1] - keys[1].pos[1]));
        seg->c[1][1] = invLenSq * (3.0f * (keys[1].pos[1] - keys[0].pos[1])) - invLen * (2.0f * col[7 * 16] + col[7 * 16 + 1]);
        seg->c[2][1] = col[7 * 16];
        seg->c[3][1] = keys[0].pos[1];
        seg->c[0][2] = invLenSq * (col[8 * 16] + col[8 * 16 + 1]) + invLenCube * (2.0f * (keys[0].pos[2] - keys[1].pos[2]));
        seg->c[1][2] = invLenSq * (3.0f * (keys[1].pos[2] - keys[0].pos[2])) - invLen * (2.0f * col[8 * 16] + col[8 * 16 + 1]);
        seg->c[2][2] = col[8 * 16];
        seg->c[3][2] = keys[0].pos[2];
        col++;
        keys++;
        seg++;
    }
}

/* out = x + iy. */
static void Cplx_Set(Complex *out, f32 x, f32 y)
{
    out->x = x;
    out->y = y;
}

/* out = lhs - rhs as complex numbers. */
static void Cplx_Sub(Complex *out, Complex *lhs, Complex *rhs)
{
    out->x = lhs->x - rhs->x;
    out->y = lhs->y - rhs->y;
}

/*
 * out = lhs * rhs as complex numbers (lhs turned by rhs's angle and
 * scaled by its length); out may be lhs or rhs.
 */
static void Cplx_Mul(Complex *out, Complex *lhs, Complex *rhs)
{
    Complex prod;

    prod.x = lhs->x * rhs->x - lhs->y * rhs->y;
    prod.y = lhs->x * rhs->y + lhs->y * rhs->x;
    *out = prod;
}

/*
 * out = num / den as complex numbers; num itself when den is 0. out may
 * be num or den.
 */
static void Cplx_Div(Complex *out, Complex *num, Complex *den)
{
    Complex quot;
    f32 numRe;
    f32 numIm;
    f32 denIm;
    f32 denRe;
    f32 denNorm;

    denRe = den->x;
    denIm = den->y;
    denNorm = denRe * denRe + denIm * denIm;
    if (0.0f != denNorm) {
        denNorm = 1.0f / denNorm;
        numRe = num->x;
        numIm = num->y;
        quot.x = denNorm * (numRe * denRe + numIm * denIm);
        quot.y = denNorm * (numIm * denRe - numRe * denIm);
        *out = quot;
    } else {
        *out = *num;
    }
}

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

/*
 * The five complex roots of coefs[0] x^5 + ... + coefs[5] into roots
 * (Durand-Kerner). The coefficients are divided by coefs[0] (monic[1..5]).
 * The start points are gQuinticStart0..4 (x and y) scaled by 5 times the
 * largest abs(monic[i]) ^ gQuinticBoundExps[i - 2], i 2 to 5 (monic[1] is
 * not used). Each of the 26 passes moves every root in place: it
 * subtracts P(root) over the product of its differences to the others.
 */
void Quintic_Roots(Complex *roots, f32 *coefs)
{
    Complex den;
    Complex val;
    Complex est;
    Complex step;
    Complex diff;
    f32 monic[6];
    s32 i;
    f32 *boundExp;
    f32 *boundCoef;
    f32 invLead;
    f32 bound;
    f32 term;
    s32 k;
    s32 pass;
    Complex *root;
    f32 *monicCoef;
    Complex *other;
    s32 j;

    invLead = 1.0f / coefs[0];
    for (i = 1; i < 6; i++) {
        monic[i] = invLead * coefs[i];
    }
    bound = 0.0f;
    for (i = 2, boundExp = gQuinticBoundExps, boundCoef = &monic[2]; i <= 5; i++, boundExp++, boundCoef++) {
        term = Math_Pow(Math_Absf(*boundCoef), *boundExp);
        if (!(term <= bound)) {
            bound = term;
        }
    }
    bound *= 5.0f;
    roots[0].x = bound * gQuinticStart0[0];
    roots[0].y = bound * gQuinticStart0Y;
    roots[1].x = bound * gQuinticStart1[0];
    roots[1].y = bound * gQuinticStart1Y;
    roots[2].x = bound * gQuinticStart2[0];
    roots[2].y = bound * gQuinticStart2Y;
    roots[3].x = bound * gQuinticStart3[0];
    roots[3].y = bound * gQuinticStart3Y;
    roots[4].x = bound * gQuinticStart4[0];
    roots[4].y = bound * gQuinticStart4Y[0];
    for (pass = 25; pass >= 0; pass--) {
        for (j = 0, root = roots; j < 5; j++, root++) {
            Cplx_Set(&den, 1.0f, 0.0f);
            Cplx_Set(&val, 1.0f, 0.0f);
            est = *root;
            for (k = 0, monicCoef = monic, other = roots; k < 5; k++, monicCoef++, other++) {
                Cplx_Mul(&val, &val, &est);
                val.x += monicCoef[1];
                if (k != j) {
                    Cplx_Sub(&diff, &est, other);
                    Cplx_Mul(&den, &den, &diff);
                }
            }
            Cplx_Div(&step, &val, &den);
            Cplx_Sub(root, &est, &step);
        }
    }
}

/*
 * The real roots of coefs[0] x^3 + coefs[1] x^2 + coefs[2] x + coefs[3]
 * into out (Cardano: shift the cubic by shift, then depP and depQ);
 * returns how many: 1 (a triple root, or one real root), 2 (a double
 * root) or 3 (the trigonometric form).
 */
s32 Cubic_Roots(f32 *out, f32 *coefs)
{
    f32 invLead;
    f32 shift;
    f32 depP;
    f32 depQ;
    f32 disc;
    f32 depPSq;
    f32 termU;
    f32 termV;
    f32 quadCoef;
    f32 linCoef;

    invLead = 1.0f / coefs[0];
    quadCoef = coefs[1] * invLead;
    linCoef = coefs[2] * invLead;
    shift = 0.33333334f * quadCoef;
    depP = 0.33333334f * linCoef - shift * shift;
    depQ = 0.5f * (coefs[3] * invLead + shift * (shift * shift + shift * shift - linCoef));
    if (Math_Absf(depP) < 1e-6f && Math_Absf(depQ) < 1e-6f) {
        out[0] = -shift;
        return 1;
    }
    depPSq = depP * depP;
    disc = depPSq * depP + depQ * depQ;
    if (!(disc <= 0.0f)) {
        if (depQ >= 0.0f) {
            termU = Math_Pow(depQ + Math_Sqrtf(disc), 0.33333334f);
        } else {
            termU = -Math_Pow(-depQ + Math_Sqrtf(disc), 0.33333334f);
        }
        termV = depP / termU;
        if (depP < 0.0f) {
            out[0] = termV - termU;
        } else {
            termV = termU * termU;
            out[0] = -2.0f * depQ * termV / (depPSq + termV * (termV + depP));
        }
        out[0] -= shift;
        return 1;
    }
    if (depQ < 0.0f) {
        depP = Math_Sqrtf(-depP);
    } else {
        depP = -Math_Sqrtf(-depP);
    }
    if (Math_Absf(disc) < 1e-6f) {
        out[0] = depP + depP - shift;
        out[1] = -depP - shift;
        return 2;
    }
    depQ = 0.33333334f * Math_Atan2(Math_Sqrtf(-disc), -depQ);
    termU = depP * Math_Cos(depQ);
    termV = 1.7320508f * (depP * Math_Sin(depQ));
    out[0] = termU + termU - shift;
    out[1] = -termU - termV - shift;
    out[2] = -termU + termV - shift;
    return 3;
}

/* This file's initialised data. */
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
