#include "common.h"
#include "quest.h"
#include "em.h"
#include "scene.h"
#include "area.h"

/* prototypes */
void Obj_GetPartLod(u8 i, f32 *pos, s32 obj);
AreaRecord *Stage_GetAreaRecord(u16 x);
f32 Vec_Dot(f32 *a, f32 *b);
s32 Tri_ContainsPoint(f32 *p, f32 *tri, f32 *n);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void Vec_Scale(f32 *out, f32 *in, f32 s);
f32 Math_Sqrtf(f32 x);
void Vec_Normalize(f32 *v);
f32 Vec_Length(f32 *v);
f32 Math_Acos(f32 x);
extern u32 gAreaGrid2CellX;
extern u32 gAreaGrid2CellZ;
extern u32 gAreaGrid2Cols;
extern s32 **gAreaGrid2Cells;
/* The area grid (AreaTable_Relocate): cell sizes, columns, the lists of the cells. */
extern u32 gAreaGridCellX;
extern u32 gAreaGridCellZ;
extern u32 gAreaGridCols;
extern u32 gAreaGridRows;
extern u32 gAreaGrid2Rows;
s32 Tri_Normal(f32 *a, f32 *b, f32 *c, f32 *out);
extern s32 **gAreaGridCells;
/* A triangle of the area collision (an entry of the grid lists). */
typedef struct AreaTri {
    u8 kind;            /* 0x00: its kind (gAreaKindFlags by stage) */
    u8 unk01;           /* 0x01 */
    u16 unk02;          /* 0x02 */
    f32 v[3][3];        /* 0x04: the corners */
    f32 n[3];           /* 0x28: the normal */
    f32 d;              /* 0x34: the plane distance */
} AreaTri;

/* Tables of 8-byte records, by the model's id (+0x680). */
typedef struct Rec8 {
    u8 b[8];
} Rec8;

extern Rec8 *gAreaKindFlags[];
s32 Tri_Contains2D(f32 *tri, f32 *p);
s32 AreaGrid2_Contains(f32 *pos);
s32 *AreaGrid2_GetCell(f32 *pos);
/* A level-of-detail record (24 bytes, gPartLodRecs and gPartLodRecs2 by stage). */
typedef struct LodRec {
    f32 pos[3];         /* 0x00 */
    f32 near;           /* 0x0C: nearer than this, nearModel */
    f32 far;            /* 0x10: farther than this, no model */
    u16 nearModel;      /* 0x14 */
    u16 farModel;       /* 0x16 */
} LodRec;
extern LodRec *gPartLodRecs[];
extern LodRec *gPartLodRecs2[];
extern u8 *gPartLodModels[];
void Vec_Copy(f32 *dst, f32 *src);
f32 Vec_Distance(f32 *a, f32 *b);
/* A sphere tested against the area (AreaHit_SpherePushOut). */
typedef struct HitSphere {
    u8 unk00[0x18];
    f32 r;              /* 0x18: the radius */
    u8 unk1C[0x20 - 0x1C];
    u16 unk20;          /* 0x20: 1: push out horizontally */
} HitSphere;
/* The first 4 bytes of an AreaTri, as the floor tests hand them out. */
typedef struct TriHdr {
    u8 kind;
    u8 unk01;
    u16 unk02;
} TriHdr;

/* The hits of the area tests (Capsule_TestTri; gAreaHitCount of them): the
 * triangle, the sphere center, the flags, the contact points. */
typedef struct Vec3 {
    f32 x, y, z;
} Vec3;
extern s8 gAreaHitPointKinds[];         /* 1: an edge or corner hit */
extern s8 gAreaHitFlags[];         /* flags: 1 behind the plane, 2 crossing it */
extern Vec3 gAreaHitPointsB[];
extern Vec3 gAreaHitPoints[];       /* the contact point */
extern Vec3 gAreaHitCapsulePos[];       /* the sphere center */
extern AreaTri gAreaHits[];
f32 Seg_NearestPointDist(f32 *out, f32 *a, f32 *b, f32 *p);
s32 Capsule_TestTri(f32 *cap, AreaTri *t);
void Vec_SnapSmall(f32 *dst, f32 *src);
u8 Tri_PushOut(f32 *tri, Em *obj, s32 *timer, f32 *out, f32 h);
void Vec_AngleToUp(f32 *v, s32 *out);
extern f32 D_0041D220[];
f32 Math_Absf(f32 x);
void Vec_Lerp(f32 *out, f32 *a, f32 *b, f32 t);
extern s8 gAreaHitCount;
/* the calls cast to u8: this unit's code takes the value as u8 */
s32 Obj_InCurrentArea(void *obj);
void Vec_Set(f32 *v, f32 x, f32 y, f32 z);
/* The moving sphere of the sweep tests (AreaGrid_SweepSphere): its end and start,
 * radius and length. */
typedef struct SweepCap {
    Vec3 a;             /* 0x00 */
    Vec3 b;             /* 0x0C */
    f32 r;              /* 0x18 */
    f32 len;            /* 0x1C */
    s16 unk20;          /* 0x20 */
    s16 unk22;          /* 0x22 */
} SweepCap;
extern f32 gAreaQuadOffsets[][2];
f32 Vec_DistanceXZ(f32 *a, f32 *b);
extern f32 gPlayerBodySpheres[];
extern f32 *D_8B9FC0[];
extern f32 *D_7F1980[];
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void AreaHit_CollideSweep(SweepCap *cap, u16 mask, s32 em, f32 r);
void AreaHit_CollideCapsule(SweepCap *cap, u16 mask, Em *em, f32 r);
s32 AreaHit_Resolve(f32 *cap, f32 *out, u8 *flag);
s32 AreaGrid_ClampSegment(f32 *out, f32 *to, f32 *from);
s32 AreaGrid_Quadrant(f32 *pos);
s32 AreaGrid_Contains(f32 *pos);
s32 *AreaGrid_GetCell(f32 *pos);
s32 List_Contains(s32 id, s32 *list, u32 n);
s32 AreaHitTris_Contains(s32 id, u32 n);
extern s32 gAreaHitTris[];
/* The records of the area hits by em slot (+0xC): 11 of 12 bytes, ended by flags == 0. */
typedef struct AreaHit {
    u16 flags;          /* 0x0: 1 a hit, 2 a special kind */
    u16 heading;        /* 0x2: to the contact point, relative to em */
    f32 *normal;        /* 0x4: the triangle normal */
    u8 kind;            /* 0x8: AreaTri.unk01 */
} AreaHit;
extern AreaHit gEmWallContacts[][11];
extern s8 gAreaHitIdCount;
extern s8 gAreaHitIds[];
extern Rec8 *gAreaTriKindFlags[];
s32 Player_ActionInCamRange_Unk(void *em);
s32 Capsule_TestTriLevel(f32 *cap, AreaTri *t, f32 *from);
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);

void Vec_Sub(f32 *out, f32 *a, f32 *b);
/* end of prototypes */

/* The last loop reuses i and has its own triangle pointer (one t for both loops swaps s0/s2). */
#pragma optimization_level 4
void AreaHit_CollideCapsule(SweepCap *seg, u16 mask, Em *em, f32 r)
{
    u8 flag;
    Vec3 pos;
    SweepCap cap;
    s32 seen[10];
    s32 i;
    s32 j;
    s32 nseen;
    s32 nhit;
    s32 idx;
    s32 e;
    s32 qx;
    s32 qz;
    s32 quad;
    u16 m;
    AreaTri *t;
    s32 *list;
    s32 *cell;
    s32 *hp;
    u8 n;
    u8 hit;
    u32 h;
    f32 *from;
    AreaTri *tri;

    flag = 0;
    if (AreaGrid_Contains((f32 *)&seg->b) == 0) {
        return;
    }
    gAreaHitCount = 0;
    gAreaHitIdCount = 0;
    from = &em->posX;
    nseen = 0;
    nhit = 0;
    cap.a = seg->b;
    cap.b = seg->a;
    cap.unk20 = 1;
    cap.r = r;
    cap.unk22 = 1;
    cap.len = Vec_Distance((f32 *)&cap.a, (f32 *)&cap.b);
    if (Player_ActionInCamRange_Unk(em) == 1) {
        m = 0x7FFF;
    } else {
        m = 0xFFFF;
    }
    quad = AreaGrid_Quadrant((f32 *)&seg->b);
    qx = seg->b.x / (f32)gAreaGridCellX;
    qz = seg->b.z / (f32)gAreaGridCellZ;
    pos = seg->b;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 2; j++) {
            pos.x = (f32)gAreaGridCellX * ((f32)qx + gAreaQuadOffsets[quad][0]);
            pos.z = (f32)gAreaGridCellZ * ((f32)qz + gAreaQuadOffsets[quad][1]);
            pos.x += (f32)((gAreaGridCellX >> 1) + gAreaGridCellX * i);
            pos.z += (f32)((gAreaGridCellZ >> 1) + gAreaGridCellZ * j);
            if (AreaGrid_Contains((f32 *)&pos) == 1) {
                cell = AreaGrid_GetCell((f32 *)&pos);
                idx = 0;
                if (cell[idx] != -1) {
                    do {
                        t = (AreaTri *)cell[idx];
                        if (!(mask & (t->unk02 & m))
                            && (nseen == 0 || List_Contains((s32)t, seen, nseen) != 1)
                            && (nhit == 0 || AreaHitTris_Contains((s32)t, nhit) != 1)) {
                            if (Capsule_TestTriLevel((f32 *)&cap, t, from)) {
                                gAreaHitTris[nhit++] = (s32)t;
                            }
                            seen[nseen++] = (s32)t;
                            if (nseen >= 10) {
                                nseen = 0;
                            }
                        }
                        idx++;
                    } while (cell[idx] != -1);
                }
            }
        }
    }
    AreaHit_Resolve((f32 *)&cap, from, &flag);
    if (flag) {
        flag = 0;
    }
    n = 0;
    gEmWallContacts[em->index][0].flags = 0;
    hit = 0;
    if (gAreaHitIdCount != 0) {
        for (i = 0, e = 0; i < gAreaHitIdCount; i++) {
            idx = gAreaHitIds[i];
            tri = (AreaTri *)gAreaHitTris[idx];
            h = (u16)(Math_Heading(gAreaHitPoints[idx].x, gAreaHitPoints[idx].z, from[0], from[2]) + 0x4000);
            h = (u16)(h - em->rotY);
            gEmWallContacts[em->index][e].flags = 1;
            if (gAreaTriKindFlags[gScene.area][tri->kind].b[0]) {
                hit = 1;
                gEmWallContacts[em->index][e].flags |= 2;
            }
            n++;
            gEmWallContacts[em->index][e].kind = tri->unk01;
            gEmWallContacts[em->index][e].heading = h;
            gEmWallContacts[em->index][e].normal = tri->n;
            e++;
            if (hit) {
                hit = 0;
                if (n != 1) {
                    u16 tf = gEmWallContacts[em->index][0].flags;
                    u8 tk = gEmWallContacts[em->index][0].kind;
                    u16 th = gEmWallContacts[em->index][0].heading;
                    f32 *tn = gEmWallContacts[em->index][0].normal;
                    gEmWallContacts[em->index][0].flags = gEmWallContacts[em->index][(u8)(n - 1)].flags;
                    gEmWallContacts[em->index][0].kind = gEmWallContacts[em->index][(u8)(n - 1)].kind;
                    gEmWallContacts[em->index][0].heading = gEmWallContacts[em->index][(u8)(n - 1)].heading;
                    gEmWallContacts[em->index][0].normal = gEmWallContacts[em->index][(u8)(n - 1)].normal;
                    gEmWallContacts[em->index][(u8)(n - 1)].flags = tf;
                    gEmWallContacts[em->index][(u8)(n - 1)].kind = tk;
                    gEmWallContacts[em->index][(u8)(n - 1)].heading = th;
                    gEmWallContacts[em->index][(u8)(n - 1)].normal = tn;
                }
            }
            em->wallHitDirs |= 1 << (h >> 11);
        }
        gEmWallContacts[em->index][n].flags = 0;
    }
}
#pragma optimization_level reset


void AreaHit_CollideSweep(SweepCap *seg, u16 mask, s32 em_in, f32 r)
{
    Em *em = (Em *)em_in;
    u8 flag;
    Vec3 pos;
    SweepCap cap;
    s32 seen[10];
    s32 i;
    s32 j;
    s32 nseen;
    s32 nhit;
    s32 idx;
    s32 cells;
    s32 qx;
    s32 qz;
    s32 quad;
    AreaTri *t;
    AreaTri *t2;
    s32 *list;
    s32 *cell;
    u32 h;
    s32 k;
    f32 *from;

    if (AreaGrid_Contains((f32 *)&seg->b) == 0) {
        return;
    }
    gAreaHitCount = 0;
    gAreaHitIdCount = 0;
    nseen = 0;
    nhit = 0;
    from = &em->posX;
    cap.a = seg->b;
    cap.b = seg->a;
    cap.unk20 = 1;
    cap.r = r;
    cap.unk22 = 1;
    cap.len = Vec_Distance((f32 *)&cap.a, (f32 *)&cap.b);
    if (gAreaGridCellX <= gAreaGridCellZ) {
        cells = (s32)(2.0f * r / (f32)gAreaGridCellX) + 2;
    } else {
        cells = (s32)(2.0f * r / (f32)gAreaGridCellZ) + 2;
    }
    if (cells == 2) {
        quad = AreaGrid_Quadrant((f32 *)&seg->b);
    }
    qx = seg->b.x / (f32)gAreaGridCellX;
    qz = seg->b.z / (f32)gAreaGridCellZ;
    pos = seg->b;
    for (i = 0; i < cells; i++) {
        for (j = 0; j < cells; j++) {
            if (cells == 2) {
                pos.x = (f32)gAreaGridCellX * ((f32)qx + gAreaQuadOffsets[quad][0]);
                pos.z = (f32)gAreaGridCellZ * ((f32)qz + gAreaQuadOffsets[quad][1]);
                pos.x += (f32)((gAreaGridCellX >> 1) + gAreaGridCellX * i);
                pos.z += (f32)((gAreaGridCellZ >> 1) + gAreaGridCellZ * j);
            } else {
                pos.x = (f32)qx * (f32)gAreaGridCellX - (f32)(cells / 2) * (f32)gAreaGridCellX;
                pos.z = (f32)qz * (f32)gAreaGridCellZ - (f32)(cells / 2) * (f32)gAreaGridCellZ;
                pos.x += (f32)((gAreaGridCellX >> 1) + gAreaGridCellX * i);
                pos.z += (f32)((gAreaGridCellZ >> 1) + gAreaGridCellZ * j);
            }
            if (AreaGrid_Contains((f32 *)&pos) == 1) {
                cell = AreaGrid_GetCell((f32 *)&pos);
                idx = 0;
                if (cell[idx] != -1) {
                    do {
                        t = (AreaTri *)cell[idx];
                        if (!(t->unk02 & mask)
                            && (nseen == 0 || List_Contains((s32)t, seen, nseen) != 1)
                            && (nhit == 0 || AreaHitTris_Contains((s32)t, nhit) != 1)) {
                            if (Capsule_TestTriLevel((f32 *)&cap, t, from)) {
                                gAreaHitTris[nhit++] = (s32)t;
                            }
                            seen[nseen++] = (s32)t;
                            if (nseen >= 10) {
                                nseen = 0;
                            }
                        }
                        idx++;
                    } while (cell[idx] != -1);
                }
            }
        }
    }
    AreaHit_Resolve((f32 *)&cap, from, &flag);
    em->wallHitSpecial = 0;
    for (i = 0; i < gAreaHitIdCount; i++) {
        idx = gAreaHitIds[i];
        t2 = (AreaTri *)gAreaHitTris[idx];
        h = (u16)(Math_Heading(gAreaHitPoints[idx].x, gAreaHitPoints[idx].z, from[0], from[2]) + 0x4000);
        em->wallHitDirs |= 1 << ((u32)(u16)(h - em->rotY) >> 11);
        if (gAreaTriKindFlags[gScene.area][t2->kind].b[1]) {
            em->wallHitSpecial = 1;
        }
    }
}

/* The sweep of AreaGrid_SweepSphere, testing each triangle with Capsule_TestTri; the result is
 * AreaHit_Resolve's, which resolves the hits into out. */
s32 AreaGrid_SweepCells(f32 *from, f32 *to, f32 *out, u16 mask, f32 r)
{
    u8 flag;
    Vec3 end;
    SweepCap cap;
    s32 seen[10];
    s32 cells;
    s32 quad;
    s32 i;
    s32 j;
    s32 nseen;
    s32 nhit;
    AreaTri *t;
    s32 idx;
    s32 *cell;
    f32 d;

    if (AreaGrid_ClampSegment((f32 *)&end, to, from) < 0) {
        return -1;
    }
    if (gScene.skipFlaggedTris) {
        mask |= 0x4000;
    }
    gAreaHitCount = 0;
    nseen = 0;
    nhit = 0;
    cap.a = end;
    cap.b = *(Vec3 *)from;
    cap.r = r;
    cap.unk20 = 0;
    cap.unk22 = 0;
    cap.len = Vec_Distance((f32 *)&cap.a, (f32 *)&cap.b);
    d = Vec_DistanceXZ((f32 *)&cap.a, (f32 *)&cap.b);
    if (gAreaGridCellX <= gAreaGridCellZ) {
        cells = (s32)((d + 2.0f * r) / (f32)gAreaGridCellX) + 2;
    } else {
        cells = (s32)((d + 2.0f * r) / (f32)gAreaGridCellZ) + 2;
    }
    if (cells == 2) {
        quad = AreaGrid_Quadrant(from);
    }
    for (i = 0; i < cells; i++) {
        for (j = 0; j < cells; j++) {
            if (cells == 2) {
                end = *(Vec3 *)from;
                end.x += (f32)gAreaGridCellX * gAreaQuadOffsets[quad][0];
                end.z += (f32)gAreaGridCellZ * gAreaQuadOffsets[quad][1];
                end.x += (f32)(gAreaGridCellX * i);
                end.z += (f32)(gAreaGridCellZ * j);
            } else {
                end = *(Vec3 *)from;
                end.x -= (f32)(cells / 2) * (f32)gAreaGridCellX;
                end.z -= (f32)(cells / 2) * (f32)gAreaGridCellZ;
                end.x += (f32)(gAreaGridCellX * i);
                end.z += (f32)(gAreaGridCellZ * j);
            }
            if (AreaGrid_Contains((f32 *)&end) == 1) {
                cell = AreaGrid_GetCell((f32 *)&end);
                idx = 0;
                if (cell[idx] != -1) {
                    do {
                        t = (AreaTri *)cell[idx];
                        if (!(t->unk02 & mask)
                            && (nseen == 0 || List_Contains((s32)t, seen, nseen) != 1)
                            && (nhit == 0 || AreaHitTris_Contains((s32)t, nhit) != 1)) {
                            if (Capsule_TestTri((f32 *)&cap, t)) {
                                gAreaHitTris[nhit++] = (s32)t;
                            }
                            seen[nseen++] = (s32)t;
                            if (nseen >= 10) {
                                nseen = 0;
                            }
                        }
                        idx++;
                    } while (cell[idx] != -1);
                }
            }
        }
    }
    return AreaHit_Resolve((f32 *)&cap, out, &flag);
}

/* 1 when id is one of the n entries of list. */
s32 List_Contains(s32 id, s32 *list, u32 n)
{
    u32 i;

    for (i = 0; i < n; i++, list++) {
        if (id == *list) {
            return 1;
        }
    }
    return 0;
}

extern s32 gAreaHitTris[];

s32 AreaHitTris_Contains(s32 id, u32 n)
{
    u32 i;

    for (i = 0; i < n; i++) {
        if (gAreaHitTris[i] == id) {
            return 1;
        }
    }
    return 0;
}

s32 AreaGrid_Quadrant(f32 *pos)
{
    s32 r;
    u32 sz;
    u32 sx;
    s32 row;
    s32 col;

    r = 0;
    sz = gAreaGridCellZ;
    row = pos[2] / (f32)sz;
    sx = gAreaGridCellX;
    col = pos[0] / (f32)sx;
    if (pos[0] >= (f32)(sx / 2 + col * sx)) {
        r = 1;
    }
    if (pos[2] >= (f32)(sz / 2 + row * sz)) {
        r += 2;
    }
    return r;
}

/* 1 when pos (x, z) is inside the second area grid, 8 units from its edges. */
s32 AreaGrid2_Contains(f32 *pos)
{
    if (pos[0] < 8.0f || pos[2] < 8.0f) {
        return 0;
    }
    if (pos[0] < (f32)(gAreaGrid2CellX * gAreaGrid2Rows) - 8.0f && pos[2] < (f32)(gAreaGrid2CellZ * gAreaGrid2Cols) - 8.0f) {
        return 1;
    }
    return 0;
}

/* 1 when pos (x, z) is inside the first area grid, 8 units from its edges. */
s32 AreaGrid_Contains(f32 *pos)
{
    if (pos[0] < 8.0f || pos[2] < 8.0f) {
        return 0;
    }
    if (pos[0] < (f32)(gAreaGridCellX * gAreaGridRows) - 8.0f && pos[2] < (f32)(gAreaGridCellZ * gAreaGridCols) - 8.0f) {
        return 1;
    }
    return 0;
}

/* 1 when pos (x, z) is inside the area id, 8 units from its edges. */
s32 Area_ContainsPos(s32 id, f32 *pos)
{
    AreaRecord *size = Stage_GetAreaRecord(id);

    if (pos[0] < 8.0f || pos[2] < 8.0f) {
        return 0;
    }
    if (pos[0] < size->sizeX - 8.0f && pos[2] < size->sizeZ - 8.0f) {
        return 1;
    }
    return 0;
}

/* Moves the point to back along the line from from until it is inside the
 * first area grid (8 units from its edges): -1 when from is outside, else 1
 * when to was moved (0 when not), the point in out. */
s32 AreaGrid_ClampSegment(f32 *out, f32 *to, f32 *from)
{
    f32 d[3];
    f32 p[3];
    f32 maxx;
    f32 maxz;
    s32 moved;

    if (from[0] < 8.0f || from[2] < 8.0f) {
        return -1;
    }
    maxx = (f32)(gAreaGridCellX * gAreaGridRows) - 8.0f;
    if (from[0] > maxx) {
        return -1;
    }
    maxz = (f32)(gAreaGridCellZ * gAreaGridCols) - 8.0f;
    if (from[2] > maxz) {
        return -1;
    }
    moved = 0;
    Vec_Copy(p, to);
    if (p[0] < 8.0f) {
        Vec_Diff(d, p, from);
        Vec_Scale(d, d, (8.0f - from[0]) / d[0]);
        Vec_Add(p, d, from);
        p[0] = 8.0f;
        moved = 1;
    }
    if (p[0] > maxx) {
        Vec_Diff(d, p, from);
        Vec_Scale(d, d, (maxx - from[0]) / d[0]);
        Vec_Add(p, d, from);
        p[0] = maxx;
        moved = 1;
    }
    if (p[2] < 8.0f) {
        Vec_Diff(d, p, from);
        Vec_Scale(d, d, (8.0f - from[2]) / d[2]);
        Vec_Add(p, d, from);
        p[2] = 8.0f;
        moved = 1;
    }
    if (p[2] > maxz) {
        Vec_Diff(d, p, from);
        Vec_Scale(d, d, (maxz - from[2]) / d[2]);
        Vec_Add(p, d, from);
        p[2] = maxz;
        moved = 1;
    }
    Vec_Copy(out, p);
    return moved;
}

/* The list of the area grid's cell that holds pos (x, z). */
s32 *AreaGrid_GetCell(f32 *pos)
{
    s32 row;
    s32 col;

    row = pos[2] / (f32)gAreaGridCellZ;
    col = pos[0] / (f32)gAreaGridCellX;
    return gAreaGridCells[col * gAreaGridCols + row];
}

/* Capsule_TestTri, skipped when the sphere keeps level (+0x22) and the
 * triangle is wholly below the point from. */
s32 Capsule_TestTriLevel(f32 *cap, AreaTri *t, f32 *from)
{
    f32 y;

    if (*(u16 *)((u8 *)cap + 0x22) != 0) {
        y = from[1];
        if (!(y < t->v[0][1]) && !(y < t->v[1][1]) && !(y < t->v[2][1])) {
            return 0;
        }
    }
    return Capsule_TestTri(cap, t);
}

/* Tests the sphere cap (two points, then a radius at +0x18) against the
 * triangle t and records a hit (gAreaHits.., gAreaHitCount hits, 9 at most): 1
 * when there is one. flags: 1 the start is behind the plane, 2 the segment
 * crosses it. */
s32 Capsule_TestTri(f32 *cap, AreaTri *t)
{
    f32 v[3];
    f32 w[3];
    f32 a[3];
    f32 b[3];
    f32 c[3];
    f32 *e[4];
    AreaTri *h;
    f32 dist;
    f32 d;
    s32 i;
    s8 flags;
    f32 **pe;


    if (gAreaHitCount >= 9) {
        return 0;
    }
    Vec_Diff(w, cap + 3, t->v[0]);
    if (Vec_Dot(w, t->n) < 0.0f) {
        return 0;
    }
    Vec_Diff(w, cap, t->v[0]);
    dist = Vec_Dot(w, t->n);
    if (dist > 0.0f) {
        flags = 0;
    } else {
        flags = 1;
    }
    if (dist > cap[6]) {
        return 0;
    }
    if (dist >= -cap[6]) {
        Vec_Scale(v, t->n, dist);
        Vec_Diff(v, cap, v);
    } else {
        Vec_Diff(w, cap, cap + 3);
        d = Vec_Dot(w, t->n);
        if (d == 0.0f) {
            return 0;
        }
        Vec_Scale(w, w, -(dist / d));
        Vec_Add(v, cap, w);
        flags |= 2;
    }
    if (Tri_ContainsPoint(v, t->v[0], t->n)) {
        h = &gAreaHits[gAreaHitCount];
        *h = *t;
        Vec_SnapSmall(h->n, h->n);
        gAreaHitCapsulePos[gAreaHitCount] = *(Vec3 *)cap;
        gAreaHitFlags[gAreaHitCount] = flags;
        gAreaHitPointKinds[gAreaHitCount] = 0;
        gAreaHitPoints[gAreaHitCount] = *(Vec3 *)v;
        gAreaHitCount++;
        return 1;
    }
    gAreaHitPointKinds[gAreaHitCount] = 1;
    Vec_Copy(b, v);
    e[0] = t->v[0];
    e[1] = t->v[1];
    e[2] = t->v[2];
    e[3] = t->v[0];
    pe = e;
    dist = 8000.0f;
    if (!(flags & 2)) {
        Vec_Copy(c, cap);
    } else {
        Vec_Copy(c, b);
    }
    for (i = 3; i > 0; i--, pe++) {
        d = Seg_NearestPointDist(v, pe[0], pe[1], c);
        if (d < dist) {
            dist = d;
            Vec_Copy(a, v);
        }
    }
    if (dist < cap[6]) {
        h = &gAreaHits[gAreaHitCount];
        *h = *t;
        Vec_SnapSmall(h->n, h->n);
        gAreaHitCapsulePos[gAreaHitCount] = *(Vec3 *)cap;
        gAreaHitFlags[gAreaHitCount] = flags;
        gAreaHitPoints[gAreaHitCount] = *(Vec3 *)a;
        gAreaHitPointsB[gAreaHitCount] = *(Vec3 *)b;
        gAreaHitCount++;
        return 1;
    }
    return 0;
}

/* Where the segment seg (two points, then a radius at +0x18) first comes
 * within that radius of the line through the two points of line: 1 and the
 * point in out when it does before the segment's end and between the two
 * points. */
s32 Capsule_HitLine(f32 *seg, f32 *line, f32 *out)
{
    f32 dir[3];
    f32 d1[3];
    f32 d2[3];
    f32 p[2][3];
    f32 *q;
    f32 disc;
    f32 s;
    f32 t1;
    f32 b;
    f32 a;
    f32 cc;
    f32 t0;

    Vec_Diff(dir, line + 3, line);
    Vec_Normalize(dir);
    Vec_Diff(p[0], seg, seg + 3);
    s = Vec_Dot(p[0], dir);
    q = p[1];
    Vec_Scale(q, dir, s);
    Vec_Diff(d1, q, p[0]);
    a = Vec_Dot(d1, d1);
    Vec_Diff(p[0], seg + 3, line);
    Vec_Scale(q, dir, Vec_Dot(p[0], dir));
    Vec_Diff(d2, q, p[0]);
    cc = Vec_Dot(d2, d2);
    b = Vec_Dot(d1, d2);
    if (a != 0.0f) {
        disc = b * b - a * (cc - seg[6] * seg[6]);
        if (disc >= 0.0f) {
            cc = 1.0f / a;
            s = Math_Sqrtf(disc);
            t1 = cc * (-b + s);
            t0 = cc * (-b - s);
            if (t0 > t1) {
                t0 = t1;
            }
            if (t0 < 1.0f) {
                Vec_Scale(p[0], seg, t0);
                Vec_Scale(q, seg + 3, 1.0f - t0);
                Vec_Add(out, p[0], q);
                Vec_Diff(p[0], out, line);
                t0 = Vec_Dot(p[0], dir);
                if (t0 >= 0.0f) {
                    Vec_Diff(q, line + 3, line);
                    if (t0 <= Vec_Length(q)) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

/* Where the segment seg (two points, then a radius at +0x18) first touches a
 * sphere of that radius around one of the three points c: 1 and the point in
 * out when it does before the segment's end. */
s32 Capsule_HitPoints(f32 *seg, f32 *c, f32 *out)
{
    f32 d1[3];
    f32 d2[3];
    f32 p[2][3];
    u32 i;
    f32 a;
    f32 b;
    f32 cc;
    f32 best;
    f32 disc;
    f32 s;
    f32 t0;
    f32 t1;

    best = 10.0f;
    for (i = 0; i < 3; i++, c += 3) {
        Vec_Diff(d1, seg, seg + 3);
        a = Vec_Dot(d1, d1);
        Vec_Diff(d2, seg + 3, c);
        cc = Vec_Dot(d2, d2);
        b = Vec_Dot(d1, d2);
        disc = b * b - a * (cc - seg[6] * seg[6]);
        if (disc >= 0.0f) {
            cc = 1.0f / a;
            s = Math_Sqrtf(disc);
            t1 = cc * (-b + s);
            t0 = cc * (-b - s);
            if (t0 > t1) {
                t0 = t1;
            }
            if (best > t0) {
                best = t0;
            }
        }
    }
    if (best < 1.0f) {
        Vec_Scale(p[0], seg, best);
        Vec_Scale(p[1], seg + 3, 1.0f - best);
        Vec_Add(out, p[0], p[1]);
        return 1;
    }
    return 0;
}

/* Where the segment seg (two points) crosses the plane through pt with normal
 * n: 0 when both ends are on one side, else 1 and the point in out. */
s32 Seg_HitPlane(f32 *seg, f32 *n, f32 *pt, f32 *out)
{
    f32 d[3];
    f32 t;
    f32 dd;
    u32 side;

    side = 0;
    Vec_Diff(d, seg + 3, pt);
    if (Vec_Dot(d, n) >= 0.0f) {
        side |= 2;
    }
    Vec_Diff(d, seg, pt);
    if ((t = Vec_Dot(d, n)) >= 0.0f) {
        side |= 1;
    }
    switch (side) {
    case 0:
    case 3:
        return 0;
    case 1:
    case 2:
        Vec_Diff(d, seg, seg + 3);
        dd = Vec_Dot(d, n);
        if (dd == 0.0f) {
            return 0;
        }
        Vec_Scale(d, d, -(t / dd));
        Vec_Add(out, seg, d);
        break;
    }
    return 1;
}

/* 1 when p is inside the triangle tri (three points) seen along n. */
s32 Tri_ContainsPoint(f32 *p, f32 *tri, f32 *n)
{
    f32 c[3];

    Tri_Normal(tri, tri + 3, p, c);
    if (Vec_Dot(c, n) < 0.0f) {
        return 0;
    }
    Tri_Normal(tri + 3, tri + 6, p, c);
    if (Vec_Dot(c, n) < 0.0f) {
        return 0;
    }
    Tri_Normal(tri + 6, tri, p, c);
    return !(Vec_Dot(c, n) < 0.0f);
}

extern u8 D_0041D210[];
s32 AreaHit_SpherePushOut(HitSphere *s, f32 *out, s8 id, s8 *flag);
void Box_Accumulate(f32 *v, f32 *box, f32 lim);

#pragma optimization_level 4
/* The mask of e's vertices (bit b for e->v[b]) that are also vertices of f. */
static inline u8 AreaHit_SharedVerts(f32 *f, AreaTri *e)
{
    f32 *g;
    s8 a;
    s8 b;
    u8 mask;
    f32 (*ev)[3];

    mask = 0;
    for (a = 0, ev = e->v; a < 3; a++, f += 3) {
        g = ev[0];
        for (b = 0; b < 3; b++, g += 3) {
            if (f[0] == g[0] && f[1] == g[1] && f[2] == g[2]) {
                mask += 1 << b;
                break;
            }
        }
    }
    return mask;
}

/* Resolves the area hits of the sweep (gAreaHitCount of them) into out: the face
 * hits first, then the edge and corner hits no face or other edge already
 * covers (skip), their push-outs summed per axis by Box_Accumulate; the
 * total is kept under 1.8 times the sweep's length. Returns the hit count. */
s32 AreaHit_Resolve(f32 *cap, f32 *out, u8 *flag)
{
    f32 v[3];
    s8 face[16];
    s8 edge[16];
    s8 kind[16];
    s8 skip[16];
    f32 d[4];
    f32 d2[4];
    f32 box[12];
    s32 i;
    s8 ns;
    s8 ne;
    s8 na;
    s32 nf;
    s32 j;
    s32 k;
    s8 found;
    s8 id;
    s8 id2;
    AreaTri *tf;
    AreaTri *tf2;
    AreaTri *te;
    f32 *p;
    f32 *p2;
    f32 (*va)[3];
    u8 mask;
    u8 mask2;
    f32 len;
    f32 max;
    f32 s;

    switch (gAreaHitCount) {
    case 0:
        break;
    case 1:
        if ((s8)AreaHit_SpherePushOut((HitSphere *)cap, v, 0, (s8 *)flag) != 0) {
            if (cap[7] != 0.0f) {
                len = Vec_Length(v);
                max = 1.8f * cap[7];
                if (!(len <= max)) {
                    s = max / len;
                    v[0] *= s;
                    v[1] *= s;
                    v[2] *= s;
                }
            }
            out[0] += v[0];
            out[1] += v[1];
            out[2] += v[2];
            gAreaHitIds[gAreaHitIdCount] = 0;
            gAreaHitIdCount++;
        }
        break;
    default:
        ns = 0;
        for (i = 0, na = 0, ne = 0; i < gAreaHitCount; i++) {
            if (gAreaHitPointKinds[i] == 0) {
                face[na] = i;
                na++;
            } else {
                edge[ne] = i;
                ne++;
            }
        }
        nf = na;
        for (i = 0; i < nf; i++) {
            for (j = 0; j < ne; j++) {
                found = 0;
                for (k = 0; k < ns; k++) {
                    if (edge[j] == skip[k]) {
                        found = 1;
                        break;
                    }
                }
                if (found == 1) {
                    continue;
                }
                te = &gAreaHits[edge[j]];
                tf = &gAreaHits[face[i]];
                if (tf->n[0] == te->n[0] && tf->n[1] == te->n[1] && tf->n[2] == te->n[2]) {
                    kind[ns] = 1;
                    skip[ns] = edge[j];
                    ns++;
                    continue;
                }
                if (tf->n[1] == 1.0f || tf->n[1] == -1.0f) {
                    continue;
                }
                if (te->n[1] == 1.0f || te->n[1] == -1.0f) {
                    continue;
                }
                va = tf->v;
                mask = AreaHit_SharedVerts(va[0], &gAreaHits[edge[j]]);
                switch (D_0041D210[mask]) {
                case 0:
                    p = te->v[0];
                    break;
                case 1:
                    p = te->v[1];
                    break;
                case 2:
                    p = te->v[2];
                    break;
                default:
                    continue;
                }
                Vec_Diff(d, p, va[0]);
                if (Vec_Dot(d, gAreaHits[face[i]].n) <= 0.0f) {
                    kind[ns] = 2;
                    skip[ns] = edge[j];
                    ns++;
                }
            }
        }
        if (ne >= 2) {
            for (i = 0; i < ne - 1; i++) {
                found = 0;
                for (k = 0; k < ns; k++) {
                    if (edge[i] == skip[k]) {
                        found = 1;
                        break;
                    }
                }
                if (found == 1) {
                    continue;
                }
                for (j = 1; j < ne; j++) {
                    if (edge[i] == edge[j]) {
                        continue;
                    }
                    found = 0;
                    for (k = 0; k < ns; k++) {
                        if (edge[j] == skip[k]) {
                            found = 1;
                            break;
                        }
                    }
                    if (found == 1) {
                        continue;
                    }
                    if (gAreaHitPoints[edge[i]].x == gAreaHitPoints[edge[j]].x && gAreaHitPoints[edge[i]].y == gAreaHitPoints[edge[j]].y
                        && gAreaHitPoints[edge[i]].z == gAreaHitPoints[edge[j]].z) {
                        kind[ns] = 3;
                        skip[ns] = edge[j];
                        ns++;
                    }
                }
            }
        }
        if (ns > 0 && ns < ne) {
            for (i = 0; i < ns; i++) {
                if (kind[i] != 1) {
                    continue;
                }
                for (j = 0; j < ne; j++) {
                    if (edge[j] == skip[i]) {
                        continue;
                    }
                    tf2 = &gAreaHits[skip[i]];
                    if (tf2->n[1] == 1.0f || tf2->n[1] == -1.0f) {
                        continue;
                    }
                    te = &gAreaHits[edge[j]];
                    if (te->n[1] == 1.0f || te->n[1] == -1.0f) {
                        continue;
                    }
                    va = tf2->v;
                    mask2 = AreaHit_SharedVerts(va[0], &gAreaHits[edge[j]]);
                    switch (D_0041D210[mask2]) {
                    case 0:
                        p2 = te->v[0];
                        break;
                    case 1:
                        p2 = te->v[1];
                        break;
                    case 2:
                        p2 = te->v[2];
                        break;
                    default:
                        continue;
                    }
                    Vec_Diff(d2, p2, va[0]);
                    if (Vec_Dot(d2, gAreaHits[skip[i]].n) <= 0.0f) {
                        kind[ns] = 2;
                        skip[ns] = edge[j];
                        ns++;
                    }
                }
            }
        }
        box[0] = box[3] = box[6] = box[9] = 0.0f;
        box[1] = box[4] = box[7] = box[10] = 0.0f;
        box[2] = box[5] = box[8] = box[11] = 0.0f;
        for (i = 0; i < nf; i++) {
            id = face[i];
            if ((s8)AreaHit_SpherePushOut((HitSphere *)cap, v, id, (s8 *)flag) != 0) {
                Box_Accumulate(v, box, cap[6]);
                gAreaHitIds[gAreaHitIdCount] = id;
                gAreaHitIdCount++;
            }
        }
        for (i = 0; i < ne; i++) {
            id2 = edge[i];
            k = 0;
            for (j = 0; j < ns; j++) {
                if (skip[j] == id2) {
                    k = 1;
                    break;
                }
            }
            if (k == 0 && (s8)AreaHit_SpherePushOut((HitSphere *)cap, v, id2, (s8 *)flag) != 0) {
                Box_Accumulate(v, box, cap[6]);
                gAreaHitIds[gAreaHitIdCount] = id2;
                gAreaHitIdCount++;
            }
        }
        if (box[3] != 0.0f) {
            if (box[0] != 0.0f) {
                v[0] = 0.5f * (box[0] + box[3]) + box[6] + box[9];
            } else {
                v[0] = box[3] + box[9];
            }
        } else {
            v[0] = box[0] + box[6];
        }
        if (box[4] != 0.0f) {
            if (box[1] != 0.0f) {
                v[1] = 0.5f * (box[1] + box[4]) + box[7] + box[10];
            } else {
                v[1] = box[4] + box[10];
            }
        } else {
            v[1] = box[1] + box[7];
        }
        if (box[5] != 0.0f) {
            if (box[2] != 0.0f) {
                v[2] = 0.5f * (box[2] + box[5]) + box[8] + box[11];
            } else {
                v[2] = box[5] + box[11];
            }
        } else {
            v[2] = box[2] + box[8];
        }
        if (cap[7] != 0.0f) {
            len = Vec_Length(v);
            max = 1.8f * cap[7];
            if (!(len <= max)) {
                s = max / len;
                v[0] *= s;
                v[1] *= s;
                v[2] *= s;
            }
        }
        out[0] += v[0];
        out[1] += v[1];
        out[2] += v[2];
        break;
    }
    return gAreaHitCount;
}
#pragma optimization_level reset

#pragma optimization_level 4
/* The gAreaHitPointKinds test is a switch with its default written first (the game leaves every
 * conditional delay slot unfilled); the second flag test compares (s8)f as the game sign-extends there. */
s32 AreaHit_SpherePushOut(HitSphere *s, f32 *out, s8 id, s8 *flag)
{
    f32 n[3];
    f32 t[3];
    f32 d[3];
    f32 c[3];
    Vec3 *p;
    s32 i;
    s32 f;

    i = id;
    switch (gAreaHitPointKinds[i]) {
    default:
        Vec_Copy(n, gAreaHits[i].n);
        f = 0;
        if (s->unk20 == 1) {
            if (!(Math_Absf(n[0]) < 0.001f) || !(Math_Absf(n[2]) < 0.001f)) {
                n[1] = 0.0f;
                f = 1;
                Vec_Normalize(n);
            }
        }
        Vec_Scale(t, n, s->r);
        Vec_Add(d, &gAreaHitPoints[i].x, t);
        if (f != 0) {
            d[1] = gAreaHitCapsulePos[i].y;
        }
        Vec_Diff(out, d, &gAreaHitCapsulePos[i].x);
        break;
    case 1:
        switch (gAreaHitFlags[i]) {
        case 0:
            p = &gAreaHitPoints[i];
            Vec_Diff(d, &gAreaHitCapsulePos[i].x, &p->x);
            break;
        case 1:
        default:
            p = &gAreaHitPoints[i];
            Vec_Diff(d, &gAreaHitPointsB[i].x, &p->x);
            break;
        }
        f = 0;
        if (s->unk20 == 1 && (d[0] != 0.0f || d[1] == 0.0f || d[2] != 0.0f)) {
            d[1] = 0.0f;
            f = 1;
        }
        Vec_Normalize(d);
        Vec_Scale(t, d, s->r);
        Vec_Add(d, &p->x, t);
        Vec_Set(c, gAreaHitCapsulePos[i].x, gAreaHitCapsulePos[i].y, gAreaHitCapsulePos[i].z);
        if ((s8)f == 1) {
            c[1] = d[1];
        }
        Vec_Diff(t, d, c);
        if (Vec_Length(t) == 0.0f) {
            return 0;
        }
        Vec_Copy(out, t);
        break;
    }
    if (gAreaHitFlags[i] == 1) {
        *flag = 1;
    }
    return 1;
}
#pragma optimization_level reset

/* Accumulates v into box: the positive (+0x00) and negative (+0x0C) parts of
 * each axis up to lim, what goes past it in +0x18 and +0x24. */
void Box_Accumulate(f32 *v, f32 *box, f32 lim)
{
    if (v[0] >= 0.0f) {
        if (box[0] + box[6] < v[0]) {
            if (v[0] <= lim) {
                box[0] = v[0];
            } else {
                box[0] = lim;
                box[6] = v[0] - lim;
            }
        }
    } else {
        if (box[3] + box[9] > v[0]) {
            if (-v[0] <= lim) {
                box[3] = v[0];
            } else {
                box[3] = -lim;
                box[9] = v[0] + lim;
            }
        }
    }
    if (v[1] >= 0.0f) {
        if (box[1] + box[7] < v[1]) {
            if (v[1] <= lim) {
                box[1] = v[1];
            } else {
                box[1] = lim;
                box[7] = v[1] - lim;
            }
        }
    } else {
        if (box[4] + box[10] > v[1]) {
            if (-v[1] <= lim) {
                box[4] = v[1];
            } else {
                box[4] = -lim;
                box[10] = v[1] + lim;
            }
        }
    }
    if (v[2] >= 0.0f) {
        if (box[2] + box[8] < v[2]) {
            if (v[2] <= lim) {
                box[2] = v[2];
            } else {
                box[2] = lim;
                box[8] = v[2] - lim;
            }
        }
    } else {
        if (box[5] + box[11] > v[2]) {
            if (-v[2] <= lim) {
                box[5] = v[2];
            } else {
                box[5] = -lim;
                box[11] = v[2] + lim;
            }
        }
    }
}

/* The floor height under pos in the second area grid: of the upward
 * triangles at (x, z) (5 at most), the highest one at most 50 above pos, else
 * the lowest one; pos[1] when there is none. */
f32 AreaGrid2_GetFloorY(f32 *pos)
{
    f32 p[3];
    f32 hs[5];
    f32 q[3][3];
    f32 below[5];
    s32 *cell;
    s8 n;
    AreaTri *t;
    AreaTri **list;
    f32 *h;
    s8 found;
    s8 j;
    s8 k;
    f32 r;

    n = 0;
    hs[0] = pos[1];
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            h = hs;
            list = (AreaTri **)cell;
            do {
                t = *list;
                p[0] = pos[0];
                p[1] = pos[2];
                q[0][0] = t->v[0][0];
                q[0][1] = t->v[0][2];
                q[1][0] = t->v[1][0];
                q[1][1] = t->v[1][2];
                q[2][0] = t->v[2][0];
                q[2][1] = t->v[2][2];
                if (t->n[1] > 0.0f && Tri_Contains2D((f32 *) q, (f32 *) p) == 1 && n < 5) {
                    n++;
                    *h++ = -(t->d + (t->n[0] * pos[0] + t->n[2] * pos[2])) / t->n[1];
                }
                list++;
            } while (*(s32 *)list != -1);
        }
    }
    if (n != 1) {
        found = 0;
        k = 0;
        for (j = 0; j < n; j++) {
            if (hs[j] <= pos[1] + 50.0f) {
                found = 1;
                below[k++] = hs[j];
            }
        }
        if (found == 1) {
            if (k == 1) {
                r = below[0];
            } else {
                hs[0] = below[0];
                for (j = 1; j < k; j++) {
                    if (hs[0] < below[j]) {
                        hs[0] = below[j];
                    }
                }
                r = hs[0];
            }
        } else {
            below[0] = hs[0];
            for (j = 1; j < n; j++) {
                if (below[0] > hs[j]) {
                    below[0] = hs[j];
                }
            }
            r = below[0];
        }
    } else {
        r = hs[0];
    }
    if (r == -0.0f) {
        r = 0.0f;
    }
    return r;
}

/* The same with 100 above pos, and only the triangles whose kind has neither
 * flag b[4] nor b[5] (gAreaKindFlags). */
f32 AreaGrid2_GetFloorYFiltered(f32 *pos)
{
    f32 p[3];
    f32 hs[5];
    f32 q[3][3];
    f32 below[5];
    s32 *cell;
    s8 n;
    AreaTri *t;
    AreaTri **list;
    f32 *h;
    s8 found;
    s8 j;
    s8 k;
    f32 r;
    f32 y;
    Rec8 *r8;
    u8 f4;
    u8 f5;

    n = 0;
    hs[0] = pos[1];
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            h = hs;
            list = (AreaTri **)cell;
            do {
                t = *list;
                p[0] = pos[0];
                p[1] = pos[2];
                q[0][0] = t->v[0][0];
                q[0][1] = t->v[0][2];
                q[1][0] = t->v[1][0];
                q[1][1] = t->v[1][2];
                q[2][0] = t->v[2][0];
                q[2][1] = t->v[2][2];
                if (t->n[1] > 0.0f && Tri_Contains2D((f32 *) q, (f32 *) p) == 1 && n < 5) {
                    y = -(t->d + (t->n[0] * pos[0] + t->n[2] * pos[2])) / t->n[1];
                    r8 = &gAreaKindFlags[gScene.area][t->kind];
                    f5 = r8->b[5];
                    f4 = r8->b[4];
                    if (!f4 && !f5) {
                        n++;
                        *h++ = y;
                    }
                }
                list++;
            } while (*(s32 *)list != -1);
        }
    }
    if (n != 1) {
        found = 0;
        k = 0;
        for (j = 0; j < n; j++) {
            if (hs[j] <= pos[1] + 100.0f) {
                found = 1;
                below[k++] = hs[j];
            }
        }
        if (found == 1) {
            if (k == 1) {
                r = below[0];
            } else {
                for (j = 0; j < k; j++) {
                    if (j == 0) {
                        hs[0] = below[j];
                    } else if (hs[0] < below[j]) {
                        hs[0] = below[j];
                    }
                }
                r = hs[0];
            }
        } else {
            for (j = 0; j < n; j++) {
                if (j == 0) {
                    below[0] = hs[j];
                } else if (below[0] > hs[j]) {
                    below[0] = hs[j];
                }
            }
            r = below[0];
        }
    } else {
        r = hs[0];
    }
    if (r == -0.0f) {
        r = 0.0f;
    }
    return r;
}

/* The floor height under the monster em at pos, in out: -1 (and the area's
 * default height) when it isn't in the current area, 0 (the stage default)
 * outside the grid, else 1. Also sets em +0x735 from flag b[6] of the floor. */
s32 Em_GetFloorY(Em *em, f32 *pos, f32 *out)
{
    f32 p[3];
    f32 hs[5];
    f32 q[3][3];
    f32 below[5];
    s32 *cell;
    s8 n;
    AreaTri *t;
    AreaTri **list;
    f32 *h;
    s8 found;
    s8 j;
    s8 k;
    f32 y;
    Rec8 *r8;
    u8 f4;
    u8 f6;

    n = 0;
    if ((u8)Obj_InCurrentArea(em) == 0) {
        *out = Stage_GetAreaRecord(em->area)->bottom;
        return -1;
    }
    hs[0] = pos[1];
    em->surfaceKind = 0;
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            h = hs;
            list = (AreaTri **)cell;
            do {
                t = *list;
                p[0] = pos[0];
                p[1] = pos[2];
                q[0][0] = t->v[0][0];
                q[0][1] = t->v[0][2];
                q[1][0] = t->v[1][0];
                q[1][1] = t->v[1][2];
                q[2][0] = t->v[2][0];
                q[2][1] = t->v[2][2];
                if (t->n[1] > 0.0f && Tri_Contains2D((f32 *) q, (f32 *) p) == 1 && n < 5) {
                    y = -(t->d + (t->n[0] * pos[0] + t->n[2] * pos[2])) / t->n[1];
                    r8 = &gAreaKindFlags[em->area][t->kind];
                    f4 = r8->b[4];
                    f6 = r8->b[6];
                    if (f6) {
                        em->surfaceKind = f6;
                    }
                    if (!f4) {
                        n++;
                        *h++ = y;
                    }
                }
                list++;
            } while (*(s32 *)list != -1);
        }
    } else {
        *out = gAreaState.record->top;
        return 0;
    }
    if (n != 1) {
        found = 0;
        k = 0;
        for (j = 0; j < n; j++) {
            if (hs[j] <= pos[1] + 50.0f) {
                found = 1;
                below[k++] = hs[j];
            }
        }
        if (found == 1) {
            if (k == 1) {
                *out = below[0];
            } else {
                for (j = 0; j < k; j++) {
                    if (j == 0) {
                        hs[0] = below[j];
                    } else if (hs[0] < below[j]) {
                        hs[0] = below[j];
                    }
                }
                *out = hs[0];
            }
        } else {
            for (j = 0; j < n; j++) {
                if (j == 0) {
                    below[0] = hs[j];
                } else if (below[0] > hs[j]) {
                    below[0] = hs[j];
                }
            }
            *out = below[0];
        }
    } else {
        *out = hs[0];
    }
    if (*out == -0.0f) {
        *out = 0.0f;
    }
    return 1;
}

/* The same, but of the heights above pos the lowest one, else the highest
 * one. */
s32 Em_GetFloorYAbove(Em *em, f32 *pos, f32 *out)
{
    f32 p[3];
    f32 hs[5];
    f32 q[3][3];
    f32 below[5];
    s32 *cell;
    s8 n;
    AreaTri *t;
    AreaTri **list;
    f32 *h;
    s8 found;
    s8 j;
    s8 k;
    f32 y;
    Rec8 *r8;
    u8 f4;
    u8 f6;

    n = 0;
    if ((u8)Obj_InCurrentArea(em) == 0) {
        *out = Stage_GetAreaRecord(em->area)->bottom;
        return -1;
    }
    hs[0] = pos[1];
    em->surfaceKind = 0;
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            h = hs;
            list = (AreaTri **)cell;
            do {
                t = *list;
                p[0] = pos[0];
                p[1] = pos[2];
                q[0][0] = t->v[0][0];
                q[0][1] = t->v[0][2];
                q[1][0] = t->v[1][0];
                q[1][1] = t->v[1][2];
                q[2][0] = t->v[2][0];
                q[2][1] = t->v[2][2];
                if (t->n[1] > 0.0f && Tri_Contains2D((f32 *) q, (f32 *) p) == 1 && n < 5) {
                    y = -(t->d + (t->n[0] * pos[0] + t->n[2] * pos[2])) / t->n[1];
                    r8 = &gAreaKindFlags[em->area][t->kind];
                    f4 = r8->b[4];
                    f6 = r8->b[6];
                    if (f6) {
                        em->surfaceKind = f6;
                    }
                    if (!f4) {
                        n++;
                        *h++ = y;
                    }
                }
                list++;
            } while (*(s32 *)list != -1);
        }
    } else {
        *out = gAreaState.record->top;
        return 0;
    }
    if (n != 1) {
        found = 0;
        k = 0;
        for (j = 0; j < n; j++) {
            if (hs[j] > pos[1]) {
                found = 1;
                below[k++] = hs[j];
            }
        }
        if (found == 0) {
            for (j = 0; j < n; j++) {
                if (j == 0) {
                    below[0] = hs[j];
                } else if (below[0] < hs[j]) {
                    below[0] = hs[j];
                }
            }
            *out = below[0];
        } else if (k == 1) {
            *out = below[0];
        } else {
            for (j = 0; j < k; j++) {
                if (j == 0) {
                    hs[0] = below[j];
                } else if (hs[0] > below[j]) {
                    hs[0] = below[j];
                }
            }
            *out = hs[0];
        }
    } else {
        *out = hs[0];
    }
    if (*out == -0.0f) {
        *out = 0.0f;
    }
    return 1;
}

/* The floor under the monster em at pos (as Em_GetFloorY), with the header of
 * its triangle in info (the monster's own at +0x660 when there is none): -1
 * when it isn't in the current area, 0 outside the grid, else 1. */
s32 Em_GetFloorInfo(Em *em, f32 *pos, TriHdr *info, f32 *out, s32 *arg4)
{
    f32 p[3];
    TriHdr hdrs[5];
    f32 hs[5];
    f32 q[3][3];
    f32 below[5];
    TriHdr belowHdr[5];
    s32 *cell;
    s8 n;
    AreaTri **list;
    AreaTri *t;
    f32 *ph;
    TriHdr *pt;
    s8 found;
    s8 j;
    s8 k;
    f32 y;

    n = 0;
    em->floorUnderSurface = 0;
    if ((u8)Obj_InCurrentArea(em) == 0) {
        *out = Stage_GetAreaRecord(em->area)->bottom;
        *arg4 = 0;
        info->kind = 0;
        info->unk01 = 0;
        info->unk02 = 0;
        return -1;
    }
    hs[0] = pos[1];
    hdrs[0].kind = em->floorKind;
    hdrs[0].unk01 = em->floorArg;
    hdrs[0].unk02 = em->floorAttr;
    em->surfaceKind = 0;
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            ph = hs;
            pt = hdrs;
            list = (AreaTri **)cell;
            do {
                t = *list;
                p[0] = pos[0];
                p[1] = pos[2];
                q[0][0] = t->v[0][0];
                q[0][1] = t->v[0][2];
                q[1][0] = t->v[1][0];
                q[1][1] = t->v[1][2];
                q[2][0] = t->v[2][0];
                q[2][1] = t->v[2][2];
                if (t->n[1] > 0.0f && Tri_Contains2D((f32 *) q, (f32 *) p) == 1 && n < 5) {
                    y = -(t->d + (t->n[0] * pos[0] + t->n[2] * pos[2])) / t->n[1];
                    if (gAreaKindFlags[em->area][t->kind].b[4] == 0) {
                        n++;
                        *ph++ = y;
                        pt->kind = t->kind;
                        pt->unk01 = t->unk01;
                        pt->unk02 = t->unk02;
                        pt++;
                    }
                }
                list++;
            } while (*(s32 *)list != -1);
        }
    } else {
        *out = gAreaState.record->top;
        *arg4 = 0;
        info->kind = 0;
        info->unk01 = 0;
        info->unk02 = 0;
        return 0;
    }
    if (n != 1) {
        found = 0;
        k = 0;
        for (j = 0; j < n; j++) {
            if (hs[j] <= pos[1] + 50.0f) {
                found = 1;
                belowHdr[k] = hdrs[j];
                below[k] = hs[j];
                k++;
            }
        }
        if (found == 1) {
            if (k == 1) {
                *out = below[0];
                info->kind = belowHdr[0].kind;
                info->unk01 = belowHdr[0].unk01;
                info->unk02 = belowHdr[0].unk02;
            } else {
                for (j = 0; j < k; j++) {
                    if (j == 0) {
                        hs[0] = below[j];
                        hdrs[0].kind = belowHdr[j].kind;
                        hdrs[0].unk01 = belowHdr[j].unk01;
                        hdrs[0].unk02 = belowHdr[j].unk02;
                    } else if (hs[0] < below[j]) {
                        hs[0] = below[j];
                        hdrs[0].kind = belowHdr[j].kind;
                        hdrs[0].unk01 = belowHdr[j].unk01;
                        hdrs[0].unk02 = belowHdr[j].unk02;
                    }
                }
                *out = hs[0];
                info->kind = hdrs[0].kind;
                info->unk01 = hdrs[0].unk01;
                info->unk02 = hdrs[0].unk02;
            }
        } else {
            for (j = 0; j < n; j++) {
                if (j == 0) {
                    below[0] = hs[j];
                    belowHdr[0].kind = hdrs[j].kind;
                    belowHdr[0].unk01 = hdrs[j].unk01;
                    belowHdr[0].unk02 = hdrs[j].unk02;
                } else if (below[0] > hs[j]) {
                    below[0] = hs[j];
                    belowHdr[0].kind = hdrs[j].kind;
                    belowHdr[0].unk01 = hdrs[j].unk01;
                    belowHdr[0].unk02 = hdrs[j].unk02;
                }
            }
            *out = below[0];
            info->kind = belowHdr[0].kind;
            info->unk01 = belowHdr[0].unk01;
            info->unk02 = belowHdr[0].unk02;
        }
    } else {
        *out = hs[0];
        info->kind = hdrs[0].kind;
        info->unk01 = hdrs[0].unk01;
        info->unk02 = hdrs[0].unk02;
    }
    if (*out == -0.0f) {
        *out = 0.0f;
    }
    if (em->rideState) {
        info->unk01 = 2;
    }
    return 1;
}

/* Em_GetFloorInfo with the special floors: a triangle whose kind has flag b[6]
 * sets em +0x735 and gives its height in depth; one with flag b[4] (not a
 * floor) gives its height less em +0x72C in depth, and sets em +0x734 when
 * it lies above the floor found. Monsters 0x15 and 0x2E use the area's
 * other default height. */
s32 Em_GetFloorInfoEx(Em *em, f32 *pos, TriHdr *info, f32 *out, f32 *depth)
{
    f32 p[3];
    TriHdr hdrs[5];
    f32 hs[5];
    f32 q[3][3];
    f32 below[5];
    TriHdr belowHdr[5];
    s32 *cell;
    s8 n;
    AreaTri **list;
    AreaTri *t;
    f32 *ph;
    TriHdr *pt;
    s8 found;
    s8 j;
    s8 k;
    f32 y;
    AreaRecord *a;
    Rec8 *r8;
    u8 f4;
    u8 f6;
    u8 low;
    u8 on;
    u8 kind6;
    u8 u6;
    u16 w6;
    u8 kind4;
    u8 u4;
    u16 w4;

    n = 0;
    em->floorUnderSurface = 0;
    em->surfaceKind = 0;
    low = 0;
    if ((u8)Obj_InCurrentArea(em) == 0) {
        a = Stage_GetAreaRecord(em->area);
        if (em->isEm && (em->type == 0x15 || em->type == 0x2E)) {
            *out = a->top;
        } else {
            *out = a->bottom;
        }
        *(s32 *)depth = 0;
        info->kind = 0;
        info->unk01 = 0;
        info->unk02 = 0;
        return -1;
    }
    hs[0] = pos[1];
    hdrs[0].kind = em->floorKind;
    hdrs[0].unk01 = em->floorArg;
    hdrs[0].unk02 = em->floorAttr;
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            ph = hs;
            pt = hdrs;
            list = (AreaTri **)cell;
            do {
                t = *list;
                p[0] = pos[0];
                p[1] = pos[2];
                q[0][0] = t->v[0][0];
                q[0][1] = t->v[0][2];
                q[1][0] = t->v[1][0];
                q[1][1] = t->v[1][2];
                q[2][0] = t->v[2][0];
                q[2][1] = t->v[2][2];
                if (t->n[1] > 0.0f && Tri_Contains2D((f32 *) q, (f32 *) p) == 1 && n < 5) {
                    y = -(t->d + (t->n[0] * pos[0] + t->n[2] * pos[2])) / t->n[1];
                    r8 = &gAreaKindFlags[em->area][t->kind];
                    f4 = r8->b[4];
                    f6 = r8->b[6];
                    if (f6) {
                        em->surfaceKind = f6;
                        *depth = y;
                        kind6 = t->kind;
                        u6 = t->unk01;
                        w6 = t->unk02;
                    }
                    if (f4) {
                        low = 1;
                        y -= *(f32 *)&em->surfaceDepth;
                        *depth = y;
                        kind4 = t->kind;
                        u4 = t->unk01;
                        w4 = t->unk02;
                    } else {
                        *ph++ = y;
                        pt->kind = t->kind;
                        pt->unk01 = t->unk01;
                        pt->unk02 = t->unk02;
                        pt++;
                        n++;
                    }
                }
                list++;
            } while (*(s32 *)list != -1);
        }
    } else {
        *out = gAreaState.record->top;
        *(s32 *)depth = 0;
        info->kind = 0;
        info->unk01 = 0;
        info->unk02 = 0;
        return 0;
    }
    if (n != 1) {
        found = 0;
        k = 0;
        for (j = 0; j < n; j++) {
            if (em->surfaceKind) {
                if (hs[j] <= pos[1] + 100.0f) {
                    found = 1;
                    belowHdr[k] = hdrs[j];
                    below[k] = hs[j];
                    k++;
                }
            } else {
                if (hs[j] <= pos[1] + 50.0f) {
                    found = 1;
                    belowHdr[k] = hdrs[j];
                    below[k] = hs[j];
                    k++;
                }
            }
        }
        if (found == 1) {
            if (k == 1) {
                *out = below[0];
                info->kind = belowHdr[0].kind;
                info->unk01 = belowHdr[0].unk01;
                info->unk02 = belowHdr[0].unk02;
            } else {
                for (j = 0; j < k; j++) {
                    if (j == 0) {
                        hs[0] = below[j];
                        hdrs[0].kind = belowHdr[j].kind;
                        hdrs[0].unk01 = belowHdr[j].unk01;
                        hdrs[0].unk02 = belowHdr[j].unk02;
                    } else if (hs[0] < below[j]) {
                        hs[0] = below[j];
                        hdrs[0].kind = belowHdr[j].kind;
                        hdrs[0].unk01 = belowHdr[j].unk01;
                        hdrs[0].unk02 = belowHdr[j].unk02;
                    }
                }
                *out = hs[0];
                info->kind = hdrs[0].kind;
                info->unk01 = hdrs[0].unk01;
                info->unk02 = hdrs[0].unk02;
            }
        } else {
            for (j = 0; j < n; j++) {
                if (j == 0) {
                    below[0] = hs[j];
                    belowHdr[0].kind = hdrs[j].kind;
                    belowHdr[0].unk01 = hdrs[j].unk01;
                    belowHdr[0].unk02 = hdrs[j].unk02;
                } else if (below[0] > hs[j]) {
                    below[0] = hs[j];
                    belowHdr[0].kind = hdrs[j].kind;
                    belowHdr[0].unk01 = hdrs[j].unk01;
                    belowHdr[0].unk02 = hdrs[j].unk02;
                }
            }
            *out = below[0];
            info->kind = belowHdr[0].kind;
            info->unk01 = belowHdr[0].unk01;
            info->unk02 = belowHdr[0].unk02;
        }
    } else {
        *out = hs[0];
        info->kind = hdrs[0].kind;
        info->unk01 = hdrs[0].unk01;
        info->unk02 = hdrs[0].unk02;
    }
    on = 0;
    if (low && *out < *depth) {
        on = 1;
        info->kind = kind4;
        info->unk01 = u4;
        info->unk02 = w4;
    }
    if (em->surfaceKind) {
        info->kind = kind6;
        info->unk01 = u6;
        info->unk02 = w6;
    }
    em->floorUnderSurface = on;
    if (*out == -0.0f) {
        *out = 0.0f;
    }
    return 1;
}

/* The height at pos (x, z) of a triangle of the second area grid that faces
 * up and whose kind has flag b[4] (gAreaKindFlags): 1 and the height in y. */
u8 AreaGrid2_GetFloorHeight(f32 *pos, f32 *y)
{
    f32 p[3];
    f32 q[3][3];
    u8 hit;
    AreaTri *t;
    AreaTri **list;
    s32 *cell;
    f32 h;

    hit = 0;
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            list = (AreaTri **)cell;
            do {
                t = *list;
                p[0] = pos[0];
                p[1] = pos[2];
                q[0][0] = t->v[0][0];
                q[0][1] = t->v[0][2];
                q[1][0] = t->v[1][0];
                q[1][1] = t->v[1][2];
                q[2][0] = t->v[2][0];
                q[2][1] = t->v[2][2];
                if (t->n[1] > 0.0f && Tri_Contains2D((f32 *) q, (f32 *) p) == 1) {
                    h = -(t->d + (t->n[0] * pos[0] + t->n[2] * pos[2])) / t->n[1];
                    if (gAreaKindFlags[gScene.area][t->kind].b[4] != 0) {
                        *y = h;
                        hit = 1;
                    }
                }
                list++;
            } while (*(s32 *)list != -1);
        }
    }
    if (*y == -0.0f) {
        *y = 0.0f;
    }
    return hit;
}

/* The same for the triangles that face down (a ceiling): 1, the height in y
 * and the kind of the triangle in info. */
u8 AreaGrid2_GetCeiling(f32 *pos, f32 *y, AreaTri *info)
{
    f32 p[3];
    f32 q[3][3];
    u8 hit;
    AreaTri *t;
    AreaTri **list;
    s32 *cell;

    hit = 0;
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            list = (AreaTri **)cell;
            do {
                t = *list;
                p[0] = pos[0];
                p[1] = pos[2];
                q[0][0] = t->v[0][0];
                q[0][1] = t->v[0][2];
                q[1][0] = t->v[1][0];
                q[1][1] = t->v[1][2];
                q[2][0] = t->v[2][0];
                q[2][1] = t->v[2][2];
                if (t->n[1] < 0.0f && Tri_Contains2D((f32 *) q, (f32 *) p) == 1) {
                    hit = 1;
                    *y = -(t->d + (t->n[0] * pos[0] + t->n[2] * pos[2])) / t->n[1];
                    info->kind = t->kind;
                    info->unk01 = t->unk01;
                    info->unk02 = t->unk02;
                }
                list++;
            } while (*(s32 *)list != -1);
        }
    }
    if (*y == -0.0f) {
        *y = 0.0f;
    }
    return hit;
}

/* 1 when pos (x, z) is over a triangle of the second area grid that faces up
 * and whose kind has flag b[3] (gAreaKindFlags). */
u8 AreaGrid2_OnFlaggedFloor(f32 *pos)
{
    f32 p[3];
    f32 q[3][3];
    u8 hit;
    AreaTri *t;
    AreaTri **list;
    s32 *cell;

    hit = 0;
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            list = (AreaTri **)cell;
            do {
                t = *list;
                p[0] = pos[0];
                p[1] = pos[2];
                q[0][0] = t->v[0][0];
                q[0][1] = t->v[0][2];
                q[1][0] = t->v[1][0];
                q[1][1] = t->v[1][2];
                q[2][0] = t->v[2][0];
                q[2][1] = t->v[2][2];
                if (t->n[1] > 0.0f && Tri_Contains2D((f32 *) q, (f32 *) p) == 1
                    && gAreaKindFlags[gScene.area][t->kind].b[3] != 0) {
                    hit = 1;
                }
                list++;
            } while (*(s32 *)list != -1);
        }
    }
    return hit;
}

/* The list of the area grid's cell that holds pos (x, z). */
s32 *AreaGrid2_GetCell(f32 *pos)
{
    s32 row;
    s32 col;

    row = pos[2] / (f32)gAreaGrid2CellZ;
    col = pos[0] / (f32)gAreaGrid2CellX;
    return gAreaGrid2Cells[col * gAreaGrid2Cols + row];
}


#pragma optimization_level 4
u8 AreaHit_Raycast(f32 *a, f32 *b, f32 *out, u16 mask)
{
    f32 hp[3];
    Vec3 d;
    Vec3 dir;
    Vec3 pos;
    Vec3 q;
    Vec3 seg[2];
    s32 hits[10];
    s32 cx0;
    s32 j;
    s32 cz0;
    s32 nx;
    s32 sx;
    s32 sz;
    s32 nz;
    s32 i;
    s32 xo;
    s32 n;
    s32 hit;
    s32 *cell;
    s32 m;
    s32 *first;
    AreaTri *t;
    s32 *w;
    s32 zj;
    s32 k;
    f32 best;
    f32 lim;
    f32 u;
    f32 dist;

    hit = 0;
    best = Vec_Distance(a, b);
    if (AreaGrid_Contains(a) == 0) {
        Vec_Set(out, a[0], a[1], a[2]);
        return 1;
    }
    if (AreaGrid_Contains(b) == 0) {
        Vec_Set(out, b[0], b[1], b[2]);
        return 1;
    }
    if (gScene.skipFlaggedTris) {
        mask |= 0x4000;
    }
    cx0 = a[0] / (f32)gAreaGrid2CellX;
    cz0 = a[2] / (f32)gAreaGrid2CellZ;
    nx = (s32)(b[0] / (f32)gAreaGrid2CellX) - cx0;
    nz = (s32)(b[2] / (f32)gAreaGrid2CellZ) - cz0;
    if (nx < 0) {
        sx = -1;
        nx = 1 - nx;
    } else {
        sx = 1;
        nx = nx + 1;
    }
    if (nz < 0) {
        sz = -1;
        nz = 1 - nz;
    } else {
        sz = 1;
        nz = nz + 1;
    }
    Vec_Set((f32 *)&seg[1], a[0], a[1], a[2]);
    Vec_Set((f32 *)&seg[0], b[0], b[1], b[2]);
    Vec_Sub((f32 *)&d, b, a);
    Vec_Set((f32 *)&dir, d.x, 0.0f, d.z);
    Vec_Normalize((f32 *)&dir);
    if (dir.x == -0.0f) {
        dir.x = 0.0f;
    }
    if (dir.z == -0.0f) {
        dir.z = 0.0f;
    }
    n = 0;
    lim = Math_Sqrtf((f32)gAreaGrid2CellX * (f32)gAreaGrid2CellX + (f32)(gAreaGrid2CellZ * gAreaGrid2CellZ))
        + (f32)(gAreaGrid2CellX / 10) + (f32)(gAreaGrid2CellZ / 10);
    i = 0;
    if (i < nx) {
        xo = 0;
        m = mask;
        w = hits;
        first = w;
        do {
            for (j = 0; j < nz; j++) {
                pos.x = (f32)(gAreaGrid2CellX >> 1) + (f32)gAreaGrid2CellX * (f32)(cx0 + xo);
                pos.z = (f32)(gAreaGrid2CellZ >> 1) + (f32)gAreaGrid2CellZ * (f32)(cz0 + j * sz);
                pos.y = 0.0f;
                if (dir.x == 0.0f && dir.z == 0.0f) {
                    u = -dir.x * (a[0] - pos.x) - dir.z * (a[2] - pos.z);
                } else {
                    u = (-dir.x * (a[0] - pos.x) - dir.z * (a[2] - pos.z)) / (dir.x * dir.x + dir.z * dir.z);
                }
                q.x = a[0] + dir.x * u;
                q.z = a[2] + dir.z * u;
                q.y = 0.0f;
                if (Vec_Distance((f32 *)&pos, (f32 *)&q) <= lim) {
                    cell = AreaGrid_GetCell((f32 *)&pos);
                    if (cell != (s32 *)-1) {
                        for (k = 0; cell[k] != -1; k++) {
                            t = (AreaTri *)cell[k];
                            if (t->unk02 & m) {
                                continue;
                            }
                            if (n != 0 && 0 < n && (s32)t == *first) {
                                continue;
                            }
                            if (Seg_HitPlane((f32 *)seg, t->n, t->v[0], hp)) {
                                if (!Tri_ContainsPoint(hp, t->v[0], t->n)) {
                                    continue;
                                }
                                hit = 1;
                                dist = Vec_Distance(a, hp);
                                if (!(best <= dist)) {
                                    best = dist;
                                    Vec_Set(out, hp[0], hp[1], hp[2]);
                                }
                            }
                            if (n < 10) {
                                *w++ = (s32)t;
                                n++;
                            }
                            if (hit) {
                                return 1;
                            }
                        }
                    }
                }
            }
            xo += sx;
        } while (++i < nx);
    }
    Vec_Set(out, b[0], b[1], b[2]);
    return 0;
}
#pragma optimization_level reset

#pragma optimization_level 4
/*
 * Tests the segment a -> b against the collision triangles of both area grids in the cells
 * along it (in the first grid skipping kinds flagged in gAreaKindFlags[em->area] b[4], or b[6]
 * for em types 8, 0x22, 0xE and 0x1A; in the second those with unk02 & mask): returns 1 with
 * the nearest hit in out, else 0 with out = b. An end outside a grid gives 1 and that end.
 */
s32 AreaGrid_TestSegment(Em *em, f32 *a, f32 *b, f32 *out, u16 mask)
{
    f32 hp[3];
    Vec3 d;
    Vec3 dir;
    Vec3 pos;
    s32 hit;
    Vec3 seg[2];
    u8 f;
    s32 cx0;
    s32 cz0;
    s32 nx;
    s32 sx;
    s32 sz;
    s32 nz;
    s32 i;
    s32 j;
    s32 zj;
    s32 n;
    s32 k;
    Vec3 q;
    s32 *cell;
    Rec8 *r;
    s32 hits[10];
    s32 *w;
    AreaTri *t;
    AreaTri *t2;
    f32 best;
    f32 lim;
    f32 u;
    f32 dist;

    hit = 0;
    best = Vec_Distance(a, b);
    if (AreaGrid2_Contains(a) == 0) {
        Vec_Set(out, a[0], a[1], a[2]);
        return 1;
    }
    if (AreaGrid2_Contains(b) == 0) {
        Vec_Set(out, b[0], b[1], b[2]);
        return 1;
    }
    if (AreaGrid_Contains(a) == 0) {
        Vec_Set(out, a[0], a[1], a[2]);
        return 1;
    }
    if (AreaGrid_Contains(b) == 0) {
        Vec_Set(out, b[0], b[1], b[2]);
        return 1;
    }
    cx0 = a[0] / (f32)gAreaGrid2CellX;
    cz0 = a[2] / (f32)gAreaGrid2CellZ;
    nx = (s32)(b[0] / (f32)gAreaGrid2CellX) - cx0;
    nz = (s32)(b[2] / (f32)gAreaGrid2CellZ) - cz0;
    if (nx < 0) {
        sx = -1;
        nx = 1 - nx;
    } else {
        sx = 1;
        nx = nx + 1;
    }
    if (nz < 0) {
        sz = -1;
        nz = 1 - nz;
    } else {
        sz = 1;
        nz = nz + 1;
    }
    Vec_Set((f32 *)&seg[1], a[0], a[1], a[2]);
    Vec_Set((f32 *)&seg[0], b[0], b[1], b[2]);
    Vec_Sub((f32 *)&d, b, a);
    Vec_Set((f32 *)&dir, d.x, 0.0f, d.z);
    Vec_Normalize((f32 *)&dir);
    if (dir.x == -0.0f) {
        dir.x = 0.0f;
    }
    if (dir.z == -0.0f) {
        dir.z = 0.0f;
    }
    n = 0;
    lim = Math_Sqrtf((f32)gAreaGrid2CellX * (f32)gAreaGrid2CellX + (f32)(gAreaGrid2CellZ * gAreaGrid2CellZ))
        + (f32)(gAreaGrid2CellX / 10) + (f32)(gAreaGrid2CellZ / 10);
    for (i = 0; i < nx; i++) {
        for (j = 0; j < nz; j++) {
            pos.x = (f32)(gAreaGrid2CellX >> 1) + (f32)gAreaGrid2CellX * (f32)(cx0 + i * sx);
            pos.z = (f32)(gAreaGrid2CellZ >> 1) + (f32)gAreaGrid2CellZ * (f32)(cz0 + j * sz);
            pos.y = 0.0f;
            if (dir.x == 0.0f && dir.z == 0.0f) {
                u = -dir.x * (a[0] - pos.x) - dir.z * (a[2] - pos.z);
            } else {
                u = (-dir.x * (a[0] - pos.x) - dir.z * (a[2] - pos.z)) / (dir.x * dir.x + dir.z * dir.z);
            }
            q.x = a[0] + dir.x * u;
            q.z = a[2] + dir.z * u;
            q.y = 0.0f;
            if (Vec_Distance((f32 *)&pos, (f32 *)&q) <= lim) {
                cell = AreaGrid2_GetCell((f32 *)&pos);
                if (cell != (s32 *)-1) {
                    for (k = 0; cell[k] != -1; k++) {
                        t = (AreaTri *)cell[k];
                        r = &gAreaKindFlags[em->area][t->kind];
                        f = r->b[4];
                        if (em->type == 8 || em->type == 0x22 || em->type == 0xE || em->type == 0x1A) {
                            f |= r->b[6];
                        }
                        if (f) {
                            continue;
                        }
                        if (n != 0) {
                            w = hits;
                            if (0 < n && (s32)t == *w) {
                                continue;
                            }
                        }
                        if (Seg_HitPlane((f32 *)seg, t->n, t->v[0], hp)) {
                            if (!Tri_ContainsPoint(hp, t->v[0], t->n)) {
                                continue;
                            }
                            hit = 1;
                            dist = Vec_Distance(a, hp);
                            if (!(best <= dist)) {
                                best = dist;
                                Vec_Set(out, hp[0], hp[1], hp[2]);
                            }
                        }
                        if (n < 10) {
                            hits[n++] = (s32)t;
                        }
                        if (hit) {
                            return 1;
                        }
                    }
                }
                cell = AreaGrid_GetCell((f32 *)&pos);
                if (cell != (s32 *)-1) {
                    for (k = 0; cell[k] != -1; k++) {
                        t2 = (AreaTri *)cell[k];
                        if (t2->unk02 & mask) {
                            continue;
                        }
                        if (n != 0) {
                            w = hits;
                            if (0 < n && (s32)t2 == *w) {
                                continue;
                            }
                        }
                        if (Seg_HitPlane((f32 *)seg, t2->n, t2->v[0], hp)) {
                            if (!Tri_ContainsPoint(hp, t2->v[0], t2->n)) {
                                continue;
                            }
                            hit = 1;
                            dist = Vec_Distance(a, hp);
                            if (!(best <= dist)) {
                                best = dist;
                                Vec_Set(out, hp[0], hp[1], hp[2]);
                            }
                        }
                        if (n < 10) {
                            hits[n++] = (s32)t2;
                        }
                        if (hit) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    Vec_Set(out, b[0], b[1], b[2]);
    return 0;
}
#pragma optimization_level reset

/* The floor triangle under em (the highest of those at most 50 above it, else the
 * lowest): -1 outside the grid or with none, 1 when em is below it, else 0 with out
 * the Tri_PushOut offset (added to the position when apply). */
s32 Em_FindFloorTri(Em *em, f32 *out, s32 apply)
{
    s32 timer;
    f32 p[3];
    AreaTri *ts[5];
    f32 hs[5];
    f32 q[3][3];
    AreaTri *belowT[5];
    f32 below[5];
    s32 *cell;
    s8 n;
    f32 *pos;
    AreaTri *t;
    AreaTri **list;
    f32 *h;
    AreaTri **tp;
    s8 found;
    s8 j;
    s8 k;
    f32 y;
    u8 r;

    n = 0;
    pos = &em->posX;
    timer = 0;
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
    if (AreaGrid2_Contains(pos) == 0) {
        return -1;
    }
    cell = AreaGrid2_GetCell(pos);
    if (*cell != -1) {
        h = hs;
        tp = ts;
        list = (AreaTri **)cell;
        do {
            t = *list;
            p[0] = pos[0];
            p[1] = pos[2];
            q[0][0] = t->v[0][0];
            q[0][1] = t->v[0][2];
            q[1][0] = t->v[1][0];
            q[1][1] = t->v[1][2];
            q[2][0] = t->v[2][0];
            q[2][1] = t->v[2][2];
            if (t->n[1] > 0.0f && Tri_Contains2D((f32 *) q, (f32 *) p) == 1 && n < 5) {
                n++;
                *h++ = -(t->d + (t->n[0] * pos[0] + t->n[2] * pos[2])) / t->n[1];
                *tp++ = t;
            }
            list++;
        } while (*(s32 *)list != -1);
    }
    if (n) {
    if (n != 1) {
        found = 0;
        k = 0;
        for (j = 0; j < n; j++) {
            if (hs[j] <= 50.0f + pos[1]) {
                below[k] = hs[j];
                found = 1;
                belowT[k] = ts[j];
                k++;
            }
        }
        if (found == 1) {
            if (k == 1) {
                y = below[0];
                t = belowT[0];
            } else {
                for (j = 0; j < k; j++) {
                    if (j == 0) {
                        hs[0] = below[j];
                        ts[0] = belowT[j];
                    } else if (hs[0] < below[j]) {
                        hs[0] = below[j];
                        ts[0] = belowT[j];
                    }
                }
                t = ts[0];
                y = hs[0];
            }
        } else {
            for (j = 0; j < n; j++) {
                if (j == 0) {
                    below[0] = hs[j];
                    belowT[0] = ts[j];
                } else if (below[0] > hs[j]) {
                    below[0] = hs[j];
                    belowT[0] = ts[j];
                }
            }
            t = belowT[0];
            y = below[0];
        }
    } else {
        y = hs[0];
        t = ts[0];
    }
    if (!(y < pos[1])) {
        Vec_AngleToUp(t->n, &timer);
        r = Tri_PushOut((f32 *)t, em, &timer, out, D_0041D220[0]);
        if (apply && r) {
            pos[0] += out[0];
            pos[1] += out[1];
            pos[2] += out[2];
        }
        return 0;
    }
    return 1;
    }
    return -1;
}

/* The offset that brings the point pos - (0, timer / 10000 * h, 0) of the
 * object onto the plane at +0x28 of tri (normal, then distance), once the
 * timer reached 0x1500; 1 when it did. */
u8 Tri_PushOut(f32 *tri, Em *obj, s32 *timer, f32 *out, f32 h)
{
    f32 y;
    f32 len2;
    f32 dist;

    if (*timer < 0x1500) {
        return 0;
    }
    if (*timer == 0x4000) {
        *timer = 0;
    }
    y = obj->posY - (f32)*timer / 10000.0f * h;
    len2 = tri[10] * tri[10] + tri[11] * tri[11] + tri[12] * tri[12];
    dist = tri[13] + (tri[10] * obj->posX + tri[11] * y + tri[12] * obj->posZ);
    out[0] = -(tri[10] * dist / len2);
    out[1] = -(tri[11] * dist / len2);
    out[2] = -(tri[12] * dist / len2);
    return 1;
}

/* The angle between v and the up axis, 0x10000 per turn. */
void Vec_AngleToUp(f32 *v, s32 *out)
{
    f32 up[3];
    f32 d;

    up[0] = 0.0f;
    up[1] = 1.0f;
    up[2] = 0.0f;
    d = Vec_Dot(v, up);
    if (d > 1.0f) {
        d = 1.0f;
    }
    *out = (u16)(s32)(65536.0f * Math_Acos(d) / 6.2831855f + 0.5f);
}

/* Where the moving sphere cap (two points, then a radius at +0x18) first
 * touches the triangle t (its face, then its edges, then its corners): 1 and
 * the sphere center in out (gAreaHitCount counts the hits). */
s32 Capsule_HitTri(f32 *cap, AreaTri *t, f32 *out)
{
    f32 d[3];
    f32 e[3];
    f32 seg[3][3];
    f32 tmp[3][3];
    f32 l[2][3];
    f32 *c;
    f32 *p1;
    f32 dn;
    f32 s;

    Vec_Diff(tmp[0], cap + 3, t->v[0]);
    if (Vec_Dot(tmp[0], t->n) < 0.0f) {
        return 0;
    }
    c = seg[0];
    Vec_Copy(c, cap);
    p1 = seg[1];
    Vec_Copy(p1, cap + 3);
    seg[2][0] = cap[6];
    Vec_Diff(d, c, p1);
    if (d[0] == 0.0f && d[1] == 0.0f && d[2] == 0.0f) {
        Vec_Add((c + 3), c, t->n);
        Vec_Diff(d, c, (c + 3));
        dn = -1.0f;
    } else {
        dn = Vec_Dot(d, t->n);
        if (dn > 0.0f) {
            return 0;
        }
    }
    if (dn == 0.0f) {
        return 0;
    }
    Vec_Diff(tmp[0], t->v[0], (c + 3));
    s = Vec_Dot(tmp[0], t->n);
    s = (s + c[6]) / dn;
    if (s >= 1.0f) {
        return 0;
    }
    if (s < 0.0f) {
        return 0;
    }
    Vec_Lerp(tmp[0], c, (c + 3), s);
    Vec_Scale(tmp[1], t->n, c[6]);
    Vec_Diff(tmp[1], tmp[0], tmp[1]);
    if (Tri_ContainsPoint(tmp[1], t->v[0], t->n)) {
        Vec_Copy(out, tmp[0]);
        gAreaHitCount++;
        return 1;
    }
    if (Capsule_HitLine(c, t->v[0], e) == 1) {
        Vec_Copy(out, e);
        gAreaHitCount++;
        return 1;
    }
    if (Capsule_HitLine(c, t->v[1], e) == 1) {
        Vec_Copy(out, e);
        gAreaHitCount++;
        return 1;
    }
    Vec_Copy(l[0], t->v[2]);
    Vec_Copy(l[1], t->v[0]);
    if (Capsule_HitLine(c, l[0], e) == 1) {
        Vec_Copy(out, e);
        gAreaHitCount++;
        return 1;
    }
    if (Capsule_HitPoints(c, t->v[0], tmp[0]) == 1) {
        Vec_Copy(out, tmp[0]);
        gAreaHitCount++;
        return 1;
    }
    return 0;
}

/* Sweeps the sphere of radius r from from to to against the area grid cells
 * around it (the triangles not masked by mask; 0x4000 added while
 * gScene.skipFlaggedTris is set): the hits go to gAreaHits.. (Capsule_TestTri), the
 * triangles hit to gAreaHitTris; out gets the last contact. 1 when there was
 * a hit. */
s32 AreaGrid_SweepSphere(f32 *from, f32 *to, f32 *out, u16 mask, f32 r)
{
    Vec3 end;
    SweepCap cap;
    s32 seen[10];
    s32 cells;
    s32 quad;
    s32 i;
    s32 j;
    s32 nseen;
    s32 nhit;
    AreaTri *t;
    s32 *list;
    s32 *cell;
    f32 d;

    if (AreaGrid_ClampSegment((f32 *)&end, to, from) < 0) {
        return 0;
    }
    if (gScene.skipFlaggedTris) {
        mask |= 0x4000;
    }
    gAreaHitCount = 0;
    nseen = 0;
    nhit = 0;
    cap.a = end;
    cap.b = *(Vec3 *)from;
    cap.r = r;
    cap.unk20 = 0;
    cap.unk22 = 0;
    cap.len = Vec_Distance((f32 *)&cap.a, (f32 *)&cap.b);
    d = Vec_DistanceXZ((f32 *)&cap.a, (f32 *)&cap.b);
    if (gAreaGridCellX <= gAreaGridCellZ) {
        cells = (s32)((d + 2.0f * r) / (f32)gAreaGridCellX) + 2;
    } else {
        cells = (s32)((d + 2.0f * r) / (f32)gAreaGridCellZ) + 2;
    }
    if (cells == 2) {
        quad = AreaGrid_Quadrant(from);
    }
    for (i = 0; i < cells; i++) {
        for (j = 0; j < cells; j++) {
            if (cells == 2) {
                end = *(Vec3 *)from;
                end.x += (f32)gAreaGridCellX * gAreaQuadOffsets[quad][0];
                end.z += (f32)gAreaGridCellZ * gAreaQuadOffsets[quad][1];
                end.x += (f32)(gAreaGridCellX * i);
                end.z += (f32)(gAreaGridCellZ * j);
            } else {
                end = *(Vec3 *)from;
                end.x -= (f32)(cells / 2) * (f32)gAreaGridCellX;
                end.z -= (f32)(cells / 2) * (f32)gAreaGridCellZ;
                end.x += (f32)(gAreaGridCellX * i);
                end.z += (f32)(gAreaGridCellZ * j);
            }
            if (AreaGrid_Contains((f32 *)&end) == 1) {
                cell = AreaGrid_GetCell((f32 *)&end);
                if (*cell != -1) {
                    list = cell;
                    do {
                        t = (AreaTri *)*list;
                        if (!(t->unk02 & mask)
                            && (nseen == 0 || List_Contains((s32)t, seen, nseen) != 1)
                            && (nhit == 0 || AreaHitTris_Contains((s32)t, nhit) != 1)) {
                            if (Capsule_HitTri((f32 *)&cap, t, out)) {
                                cap.a = *(Vec3 *)out;
                                gAreaHitTris[nhit++] = (s32)t;
                            }
                            seen[nseen++] = (s32)t;
                            if (nseen >= 10) {
                                nseen = 0;
                            }
                        }
                        list++;
                    } while (*list != -1);
                }
            }
        }
    }
    return gAreaHitCount != 0;
}

/* The model LOD of object obj's part i at pos: its parts table (gAreaKindFlags)
 * names a near/far record (24 bytes: a position, two distances, two model
 * indices) in one of two tables; the chosen model of gPartLodModels (32 bytes
 * each) goes to obj +0x664, or 0 when pos is too far. */
void Obj_GetPartLod(u8 i, f32 *pos, s32 obj)
{
    u8 *base;
    u8 a;
    u8 b;
    u16 id;
    void **dst;
    LodRec *t;
    LodRec *e;
    f32 d;

    dst = (void **)(obj + 0x664);
    id = *(u16 *)(obj + 0x680);
    a = gAreaKindFlags[id][i].b[0];
    b = gAreaKindFlags[id][i].b[1];
    base = gPartLodModels[id];

    if (base == NULL) {
        *dst = NULL;
        return;
    }
    if (a && (t = gPartLodRecs[id]) != NULL) {
        e = &t[a];
        d = Vec_Distance(e->pos, pos);
        if (d <= e->far) {
            if (d <= e->near) {
                *dst = base + e->nearModel * 32;
            } else {
                *dst = base + e->farModel * 32;
            }
            return;
        }
    } else if (b && (t = gPartLodRecs2[id]) != NULL) {
        e = &t[b];
        d = Vec_Distance(e->pos, pos);
        if (d <= e->far) {
            if (d <= e->near) {
                *dst = base + e->nearModel * 32;
            } else {
                *dst = base + e->farModel * 32;
            }
            return;
        }
    }
    *dst = NULL;
}

void func_00330D60(u8 i, void *unused, u8 *p)
{
    u8 *flag = &p[0x668];

    if (gAreaKindFlags[*(u16 *)(p + 0x680)][i].b[2] == 0) {
        *flag = 0;
    } else {
        *flag = 1;
    }
}

void func_00330DB0(u8 i, void *unused, u8 *p)
{
    p[0x669] = gAreaKindFlags[*(u16 *)(p + 0x680)][i].b[7];
}

void func_00330DE0(u8 *p)
{
    Obj_GetPartLod(p[0x660], (f32 *)(p + 0xAC), (s32)p);
    func_00330D60(p[0x660], p + 0xAC, p);
    func_00330DB0(p[0x660], p + 0xAC, p);
}

void func_00330E30(u8 *obj)
{
    u8 temp_a0;

    temp_a0 = *(u8 *)((u8 *)obj + 2);
    if (temp_a0 != 9) {
        if (temp_a0 == 0x17) {
            goto block_3;
        }
    } else {
block_3:
        Obj_GetPartLod(obj[0x660], (f32 *)(obj + 0xAC), (s32)obj);
    }
}

u8 func_00330E80(void *obj)
{
    return gAreaKindFlags[*(u16 *)((u8 *)obj + 0x680)][*(u8 *)((u8 *)obj + 0x660)].b[4];
}

/* The loop reads em's collision mask through a raw-offset view at the member's offset
 * (the headers have no offsetof), which reads exactly what the game reads: a read
 * through a cast pointer makes MWCC hoist the address (s0 = em + 0x8B2) and put the
 * loop set-up after the loop, entered at its test, as the game does; a plain
 * em->collMask gives a direct lhu (146 words off). */
/* Collides the body spheres of em (records {x, y, z, r} ended by r == -1:
 * gPlayerBodySpheres, or per monster kind from the game overlay) with the area,
 * sweeping each from the last position (+0x568) to the current one. */
void Em_CollideField(Em *em, u8 keep)
{
    s32 angles[3];
    f32 rot[3];
    Vec3 cap[2];
    f32 m[4][4];
    f32 *sph;
    f32 r;
    f32 sx;
    f32 sy;
    f32 sz;
    u16 mask;

    if (keep == 0) {
        em->wallHitDirs = 0;
    }
    if (em->fieldCollide == 0) {
        return;
    }
    if ((u8)Obj_InCurrentArea(em) == 0) {
        return;
    }
    em->collideFlags &= 0xFE;
    sx = em->scaleX;
    sy = em->scaleY;
    sz = em->scaleZ;
    sph = gPlayerBodySpheres;
    if (em->isEm) {
        if (gScene.online == 0) {
            sph = D_8B9FC0[em->type];
            if (em->sizePct >= 60 && em->sizePct <= 200) {
                sx = em->bodyScale;
                sy = sx;
                sz = sx;
            }
        } else {
            sph = D_7F1980[em->type];
        }
    }
    for (; sph[3] != -1.0f; sph += 4) {
        r = sph[3] * sx;
        if (em->isEm && (em->type == 7 || em->type == 0x32)) {
            angles[0] = 0;
            angles[1] = em->rotY;
            angles[2] = 0;
            Mtx_FromAnglesYXZ(angles, (f32 *)m);
            Vec_Rotate(rot, sph, (f32 *)m);
            cap[0].x = em->prevPosX + rot[0] * sx;
            cap[0].y = em->prevPosY + rot[1] * sy;
            cap[0].z = em->prevPosZ + rot[2] * sz;
            cap[1].x = em->posX + rot[0] * sx;
            cap[1].y = em->posY + rot[1] * sy;
            cap[1].z = em->posZ + rot[2] * sz;
        } else {
            cap[0].x = em->prevPosX + sph[0] * sx;
            cap[0].y = em->prevPosY + sph[1] * sy;
            cap[0].z = em->prevPosZ + sph[2] * sz;
            cap[1].x = em->posX + sph[0] * sx;
            cap[1].y = em->posY + sph[1] * sy;
            cap[1].z = em->posZ + sph[2] * sz;
        }
        if (em->isEm) {
            mask = *(u16 *)((u8 *)em + (u32)&((Em *)0)->collMask);
            if (em->stance == 2) {
                mask |= 0x200;
            }
            if (gScene.skipFlaggedTris) {
                mask |= 0x4000;
            }
            AreaHit_CollideSweep((SweepCap *)cap, mask, (s32)em, r);
        } else if (gScene.skipFlaggedTris) {
            AreaHit_CollideCapsule((SweepCap *)cap, 0xC004, em, r);
        } else {
            AreaHit_CollideCapsule((SweepCap *)cap, 0x8004, em, r);
        }
    }
}

/* unk_328CD0's .data (0x41D1F0-0x41D2F0) */
f32 gAreaQuadOffsets[4][2] = {
#include "sub_main/0041D1F0.inc"
};
u8 D_0041D210[16] = {
#include "sub_main/0041D210.inc"
};
f32 D_0041D220[44] = {
#include "sub_main/0041D220.inc"
};
f32 gPlayerBodySpheres[8] = {
#include "sub_main/0041D2D0.inc"
};
