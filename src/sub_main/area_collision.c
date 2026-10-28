/*
 * The area's collision. The area's table file gives two grids of triangle
 * lists (loaded by Area_LoadTables when an area loads; AreaTable_Relocate
 * makes gAreaGridCells and gAreaGrid2Cells, one list of AreaTri pointers per
 * cell, each ended by -1), and area.h describes the triangles and the rows of
 * their kinds.
 *
 * The sweeps test moving spheres (SweepCap) against the first grid: the
 * players' and monsters' bodies each frame (Em_CollideField, which runs
 * AreaHit_CollideCapsule or AreaHit_CollideSweep) and the camera
 * (AreaGrid_SweepSphere, AreaGrid_SweepCells). Capsule_TestTri records the
 * hits in gAreaHits and the arrays beside it, AreaHit_Resolve turns them into
 * one push, and the walls touched go to gEmWallContacts and Em.wallHitDirs.
 *
 * The floor tests read the second grid: the floor or ceiling height at a point
 * (AreaGrid2_GetFloorY, Em_GetFloorInfo and the others), each triangle's kind
 * looked up in the area's rows of gAreaKindFlags. The segment tests
 * (AreaHit_Raycast, AreaGrid_TestSegment) serve the monsters' sight, the
 * camera and the game's other line tests. Last come the floor's effects on an object: its own light colours
 * by place (gAreaLightZones), and a player's mustCrouch and floorHazard.
 */
#include "common.h"
#include "syssettings.h"
#include "quest.h"
#include "em.h"
#include "scene.h"
#include "area.h"
#include "player.h"
#include "objhead.h"
#include "camera.h"
#include "capsule.h"
#include "engine_math.h"
#include "equip_shop.h"
#include "file_load.h"
#include "stage_data.h"
#include "packs.h"
#include "libc.h"
#include "cam_areas.h"
#include "vec_math.h"

/* prototypes */
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
extern s32 **gAreaGridCells;

extern AreaKindRow *gAreaKindFlags[];
extern AreaLightZone *gAreaLightZones[];
extern AreaLightZone *gAreaLightZones2[];
extern u8 *gAreaLightColors[];

/* The hits of the area tests (Capsule_TestTri; gAreaHitCount of them): the
 * triangle, the sphere center, the flags, the contact points. */
typedef struct Vec3 {
    f32 x, y, z;
} Vec3;
extern s8 gAreaHitPointKinds[];         /* 1: an edge or corner hit */
extern s8 gAreaHitFlags[];         /* flags: 1 behind the plane, 2 crossing it */
extern Vec3 gAreaHitPlanePoints[];
extern Vec3 gAreaHitPoints[];       /* the contact point */
extern Vec3 gAreaHitCapsulePos[];       /* the sphere center */
extern AreaTri gAreaHits[];
extern f32 gFloorSlopeDrop[];
extern s8 gAreaHitCount;
/* The calls cast Obj_InCurrentArea's result to u8: the game's code here uses
 * only its low byte. */
/* The moving sphere of the sweep tests (AreaGrid_SweepSphere): its end and start,
 * radius and length. */
typedef struct SweepCap {
    Vec3 a;             /* 0x00 */
    Vec3 b;             /* 0x0C */
    f32 r;              /* 0x18 */
    f32 len;            /* 0x1C */
    u16 flatPush;       /* 0x20: 1: push out horizontally (the bodies' sweeps; 0 for the camera) */
    u16 skipBelow;      /* 0x22: 1: skip a triangle with no corner above the start (the bodies' sweeps) */
} SweepCap;
extern f32 gAreaQuadOffsets[][2];
void AreaHit_CollideSweep(SweepCap *cap, u16 mask, s32 em, f32 r);
void AreaHit_CollideCapsule(SweepCap *cap, u16 mask, Em *em, f32 r);
extern s32 gAreaHitTris[];
extern WallContact gEmWallContacts[][WALL_CONTACTS];
extern s8 gAreaHitIdCount;
extern s8 gAreaHitIds[];
extern AreaWallKindRow *gAreaTriKindFlags[];
extern u8 *gAreaGrid2Data;
extern u8 *gAreaGridData;
extern s32 gAreaTableFiles[];
extern u8 *gLoadBuf;
extern void *gWorkArea;

/* end of prototypes */

#pragma optimization_level 4
/*
 * Loads an area's two grids from its table file (gAreaTableFiles): part 0
 * into gWorkArea, part 1 into gSysSettings.extraBuf, each relocated in place.
 */
void Area_LoadTables(s32 id)
{
    u8 *buf;
    void *dst;

    buf = gLoadBuf;
    LoadCompressedFile(buf, gAreaTableFiles[id]);
    dst = gWorkArea;
    memcpy(dst, (void *)Pack_GetPart(buf, 0), Pack_PartSize(buf, 0));
    AreaTable_Relocate(dst);
    dst = gSysSettings.extraBuf;
    memcpy(dst, (void *)Pack_GetPart(buf, 1), Pack_PartSize(buf, 1));
    AreaTable2_Relocate(dst);
}

/*
 * Relocates the first grid (the header 8 bytes into the part): each cell's
 * list of offsets, ended by -1, becomes pointers, and the grid's globals
 * gAreaGrid* are set.
 */
void AreaTable_Relocate(u8 *file)
{
    AreaGridHeader *hdr = (AreaGridHeader *)(file + 8);
    u32 i;
    u32 j;
    s32 k;
    s32 **cell;

    gAreaGridCellX = hdr->cellX;
    gAreaGridCellZ = hdr->cellZ;
    gAreaGridRows = hdr->rows;
    gAreaGridCols = hdr->cols;
    gAreaGridCells = (s32 **)((u8 *)hdr + hdr->tblOff);
    gAreaGridData = (u8 *)hdr + hdr->dataOff;
    for (i = 0; i < hdr->rows; i++) {
        for (j = 0; j < hdr->cols; j++) {
            cell = &gAreaGridCells[i * hdr->cols + j];
            *cell = (s32 *)((u8 *)*cell + (u32)hdr);
            for (k = 0; (*cell)[k] != -1; k++) {
                (*cell)[k] += (u32)gAreaGridData;
            }
        }
    }
}

/* The same for the second grid (gAreaGrid2*). */
void AreaTable2_Relocate(u8 *file)
{
    AreaGridHeader *hdr = (AreaGridHeader *)(file + 8);
    u32 i;
    u32 j;
    s32 k;
    s32 **cell;

    gAreaGrid2CellX = hdr->cellX;
    gAreaGrid2CellZ = hdr->cellZ;
    gAreaGrid2Rows = hdr->rows;
    gAreaGrid2Cols = hdr->cols;
    gAreaGrid2Cells = (s32 **)((u8 *)hdr + hdr->tblOff);
    gAreaGrid2Data = (u8 *)hdr + hdr->dataOff;
    for (i = 0; i < hdr->rows; i++) {
        for (j = 0; j < hdr->cols; j++) {
            cell = &gAreaGrid2Cells[i * hdr->cols + j];
            *cell = (s32 *)((u8 *)*cell + (u32)hdr);
            for (k = 0; (*cell)[k] != -1; k++) {
                (*cell)[k] += (u32)gAreaGrid2Data;
            }
        }
    }
}

/* Collides a player's sphere of the given radius, moved from seg->a to seg->b, with the
 * walls of the first area grid: the 2x2 cells nearest seg->b, skipping the
 * triangles whose attributes meet mask (AREA_TRI_ATTR_PLAYER_ON_WALL is not tested while
 * Player_IsOnWall), each one tested only where it rises above the object
 * (Capsule_TestTriLevel). AreaHit_Resolve pushes the object's position out of
 * the hits; each hit it kept becomes a record of gEmWallContacts[em->index]
 * (heading, normal, the triangle's seArg; WALL_CONTACT_SPECIAL when its kind
 * is special in gAreaTriKindFlags, swapped to the front; flags 0 ends the list)
 * and sets a bit of em->wallHitDirs.
 * The last loop reuses i and has its own triangle pointer: with one pointer
 * for both loops the compiler assigns two registers the other way round from
 * the game's code. */
void AreaHit_CollideCapsule(SweepCap *seg, u16 mask, Em *em, f32 radius)
{
    u8 flag;
    Vec3 pos;
    SweepCap cap;
    s32 seen[AREA_SEEN_TRIS];
    s32 i;
    s32 j;
    s32 nseen;
    s32 nhit;
    s32 idx;
    s32 contact;
    s32 cellX;
    s32 cellZ;
    s32 quad;
    u16 attrMask;
    AreaTri *cellTri;
    s32 *list;
    s32 *cell;
    s32 *unusedPtr;
    u8 contactCount;
    u8 hit;
    u32 contactYaw;
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
    cap.flatPush = 1;
    cap.r = radius;
    cap.skipBelow = 1;
    cap.len = Vec_Distance((f32 *)&cap.a, (f32 *)&cap.b);
    if (Player_IsOnWall(em) == 1) {
        attrMask = (u16)~AREA_TRI_ATTR_PLAYER_ON_WALL;
    } else {
        attrMask = 65535; /* every bit */
    }
    quad = AreaGrid_Quadrant((f32 *)&seg->b);
    cellX = seg->b.x / (f32)gAreaGridCellX;
    cellZ = seg->b.z / (f32)gAreaGridCellZ;
    pos = seg->b;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 2; j++) {
            pos.x = (f32)gAreaGridCellX * ((f32)cellX + gAreaQuadOffsets[quad][0]);
            pos.z = (f32)gAreaGridCellZ * ((f32)cellZ + gAreaQuadOffsets[quad][1]);
            pos.x += (f32)((gAreaGridCellX >> 1) + gAreaGridCellX * i);
            pos.z += (f32)((gAreaGridCellZ >> 1) + gAreaGridCellZ * j);
            if (AreaGrid_Contains((f32 *)&pos) == 1) {
                cell = AreaGrid_GetCell((f32 *)&pos);
                idx = 0;
                if (cell[idx] != -1) {
                    do {
                        cellTri = (AreaTri *)cell[idx];
                        if (!(mask & (cellTri->attr & attrMask))
                            && (nseen == 0 || List_Contains((s32)cellTri, seen, nseen) != 1)
                            && (nhit == 0 || AreaHitTris_Contains((s32)cellTri, nhit) != 1)) {
                            if (Capsule_TestTriLevel((f32 *)&cap, cellTri, from)) {
                                gAreaHitTris[nhit++] = (s32)cellTri;
                            }
                            seen[nseen++] = (s32)cellTri;
                            if (nseen >= AREA_SEEN_TRIS) {
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
    contactCount = 0;
    gEmWallContacts[em->index][0].flags = 0;
    hit = 0;
    if (gAreaHitIdCount != 0) {
        for (i = 0, contact = 0; i < gAreaHitIdCount; i++) {
            idx = gAreaHitIds[i];
            tri = (AreaTri *)gAreaHitTris[idx];
            contactYaw = (u16)(Math_Heading(gAreaHitPoints[idx].x, gAreaHitPoints[idx].z, from[0], from[2]) + ANGLE_TURN / 4);
            contactYaw = (u16)(contactYaw - em->rotY);
            gEmWallContacts[em->index][contact].flags = WALL_CONTACT_HIT;
            if (gAreaTriKindFlags[gScene.area][tri->kind].special) {
                hit = 1;
                gEmWallContacts[em->index][contact].flags |= WALL_CONTACT_SPECIAL;
            }
            contactCount++;
            gEmWallContacts[em->index][contact].seArg = tri->seArg;
            gEmWallContacts[em->index][contact].heading = contactYaw;
            gEmWallContacts[em->index][contact].normal = tri->n;
            contact++;
            if (hit) {
                hit = 0;
                if (contactCount != 1) {
                    u16 savedFlags = gEmWallContacts[em->index][0].flags;
                    u8 savedSeArg = gEmWallContacts[em->index][0].seArg;
                    u16 savedYaw = gEmWallContacts[em->index][0].heading;
                    f32 *savedNormal = gEmWallContacts[em->index][0].normal;
                    gEmWallContacts[em->index][0].flags = gEmWallContacts[em->index][(u8)(contactCount - 1)].flags;
                    gEmWallContacts[em->index][0].seArg = gEmWallContacts[em->index][(u8)(contactCount - 1)].seArg;
                    gEmWallContacts[em->index][0].heading = gEmWallContacts[em->index][(u8)(contactCount - 1)].heading;
                    gEmWallContacts[em->index][0].normal = gEmWallContacts[em->index][(u8)(contactCount - 1)].normal;
                    gEmWallContacts[em->index][(u8)(contactCount - 1)].flags = savedFlags;
                    gEmWallContacts[em->index][(u8)(contactCount - 1)].seArg = savedSeArg;
                    gEmWallContacts[em->index][(u8)(contactCount - 1)].heading = savedYaw;
                    gEmWallContacts[em->index][(u8)(contactCount - 1)].normal = savedNormal;
                }
            }
            em->wallHitDirs |= 1 << (contactYaw >> 11);
        }
        gEmWallContacts[em->index][contactCount].flags = 0;
    }
}
#pragma optimization_level reset


/* The monsters' AreaHit_CollideCapsule: the cells searched around seg->b grow
 * with radius, the triangles whose attributes meet mask are skipped, AreaHit_Resolve
 * pushes em's position out, and each hit it kept sets a bit of em->wallHitDirs
 * and, when the triangle's kind is emSpecial in gAreaTriKindFlags,
 * em->wallHitSpecial. */
void AreaHit_CollideSweep(SweepCap *seg, u16 mask, s32 em_in, f32 radius)
{
    Em *em = (Em *)em_in;
    u8 flag;
    Vec3 pos;
    SweepCap cap;
    s32 seen[AREA_SEEN_TRIS];
    s32 i;
    s32 j;
    s32 nseen;
    s32 nhit;
    s32 idx;
    s32 cells;
    s32 cellX;
    s32 cellZ;
    s32 quad;
    AreaTri *cellTri;
    AreaTri *hitTri;
    s32 *list;
    s32 *cell;
    u32 contactYaw;
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
    cap.flatPush = 1;
    cap.r = radius;
    cap.skipBelow = 1;
    cap.len = Vec_Distance((f32 *)&cap.a, (f32 *)&cap.b);
    if (gAreaGridCellX <= gAreaGridCellZ) {
        cells = (s32)(2.0f * radius / (f32)gAreaGridCellX) + 2;
    } else {
        cells = (s32)(2.0f * radius / (f32)gAreaGridCellZ) + 2;
    }
    if (cells == 2) {
        quad = AreaGrid_Quadrant((f32 *)&seg->b);
    }
    cellX = seg->b.x / (f32)gAreaGridCellX;
    cellZ = seg->b.z / (f32)gAreaGridCellZ;
    pos = seg->b;
    for (i = 0; i < cells; i++) {
        for (j = 0; j < cells; j++) {
            if (cells == 2) {
                pos.x = (f32)gAreaGridCellX * ((f32)cellX + gAreaQuadOffsets[quad][0]);
                pos.z = (f32)gAreaGridCellZ * ((f32)cellZ + gAreaQuadOffsets[quad][1]);
                pos.x += (f32)((gAreaGridCellX >> 1) + gAreaGridCellX * i);
                pos.z += (f32)((gAreaGridCellZ >> 1) + gAreaGridCellZ * j);
            } else {
                pos.x = (f32)cellX * (f32)gAreaGridCellX - (f32)(cells / 2) * (f32)gAreaGridCellX;
                pos.z = (f32)cellZ * (f32)gAreaGridCellZ - (f32)(cells / 2) * (f32)gAreaGridCellZ;
                pos.x += (f32)((gAreaGridCellX >> 1) + gAreaGridCellX * i);
                pos.z += (f32)((gAreaGridCellZ >> 1) + gAreaGridCellZ * j);
            }
            if (AreaGrid_Contains((f32 *)&pos) == 1) {
                cell = AreaGrid_GetCell((f32 *)&pos);
                idx = 0;
                if (cell[idx] != -1) {
                    do {
                        cellTri = (AreaTri *)cell[idx];
                        if (!(cellTri->attr & mask)
                            && (nseen == 0 || List_Contains((s32)cellTri, seen, nseen) != 1)
                            && (nhit == 0 || AreaHitTris_Contains((s32)cellTri, nhit) != 1)) {
                            if (Capsule_TestTriLevel((f32 *)&cap, cellTri, from)) {
                                gAreaHitTris[nhit++] = (s32)cellTri;
                            }
                            seen[nseen++] = (s32)cellTri;
                            if (nseen >= AREA_SEEN_TRIS) {
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
        hitTri = (AreaTri *)gAreaHitTris[idx];
        contactYaw = (u16)(Math_Heading(gAreaHitPoints[idx].x, gAreaHitPoints[idx].z, from[0], from[2]) + ANGLE_TURN / 4);
        em->wallHitDirs |= 1 << ((u32)(u16)(contactYaw - em->rotY) >> 11);
        if (gAreaTriKindFlags[gScene.area][hitTri->kind].emSpecial) {
            em->wallHitSpecial = 1;
        }
    }
}

/* The sweep of AreaGrid_SweepSphere, testing each triangle with Capsule_TestTri; the result is
 * AreaHit_Resolve's, which resolves the hits into out. */
s32 AreaGrid_SweepCells(f32 *from, f32 *target, f32 *out, u16 mask, f32 radius)
{
    u8 flag;
    Vec3 end;
    SweepCap cap;
    s32 seen[AREA_SEEN_TRIS];
    s32 cells;
    s32 quad;
    s32 i;
    s32 j;
    s32 nseen;
    s32 nhit;
    AreaTri *cellTri;
    s32 idx;
    s32 *cell;
    f32 distXZ;

    if (AreaGrid_ClampSegment((f32 *)&end, target, from) < 0) {
        return -1;
    }
    if (gScene.skipFlaggedTris) {
        mask |= AREA_TRI_ATTR_FLAGGED;
    }
    gAreaHitCount = 0;
    nseen = 0;
    nhit = 0;
    cap.a = end;
    cap.b = *(Vec3 *)from;
    cap.r = radius;
    cap.flatPush = 0;
    cap.skipBelow = 0;
    cap.len = Vec_Distance((f32 *)&cap.a, (f32 *)&cap.b);
    distXZ = Vec_DistanceXZ((f32 *)&cap.a, (f32 *)&cap.b);
    if (gAreaGridCellX <= gAreaGridCellZ) {
        cells = (s32)((distXZ + 2.0f * radius) / (f32)gAreaGridCellX) + 2;
    } else {
        cells = (s32)((distXZ + 2.0f * radius) / (f32)gAreaGridCellZ) + 2;
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
                        cellTri = (AreaTri *)cell[idx];
                        if (!(cellTri->attr & mask)
                            && (nseen == 0 || List_Contains((s32)cellTri, seen, nseen) != 1)
                            && (nhit == 0 || AreaHitTris_Contains((s32)cellTri, nhit) != 1)) {
                            if (Capsule_TestTri((f32 *)&cap, cellTri)) {
                                gAreaHitTris[nhit++] = (s32)cellTri;
                            }
                            seen[nseen++] = (s32)cellTri;
                            if (nseen >= AREA_SEEN_TRIS) {
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


/* 1 when id is one of the first n entries of gAreaHitTris, else 0. */
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

/* The quarter of its first-grid cell that pos (x, z) lies in: +1 in the upper
 * half in x, +2 in the upper half in z. The sweeps take it as the row of
 * gAreaQuadOffsets that places the 2x2 cells nearest pos. */
s32 AreaGrid_Quadrant(f32 *pos)
{
    s32 quadrant;
    u32 cellZ;
    u32 cellX;
    s32 row;
    s32 col;

    quadrant = 0;
    cellZ = gAreaGridCellZ;
    row = pos[2] / (f32)cellZ;
    cellX = gAreaGridCellX;
    col = pos[0] / (f32)cellX;
    if (pos[0] >= (f32)(cellX / 2 + col * cellX)) {
        quadrant = 1;
    }
    if (pos[2] >= (f32)(cellZ / 2 + row * cellZ)) {
        quadrant += 2;
    }
    return quadrant;
}

/* 1 when pos (x, z) is inside the second area grid, 8 units from its edges. */
s32 AreaGrid2_Contains(f32 *pos)
{
    if (pos[0] < AREA_EDGE_MARGIN || pos[2] < AREA_EDGE_MARGIN) {
        return 0;
    }
    if (pos[0] < (f32)(gAreaGrid2CellX * gAreaGrid2Rows) - AREA_EDGE_MARGIN && pos[2] < (f32)(gAreaGrid2CellZ * gAreaGrid2Cols) - AREA_EDGE_MARGIN) {
        return 1;
    }
    return 0;
}

/* 1 when pos (x, z) is inside the first area grid, 8 units from its edges. */
s32 AreaGrid_Contains(f32 *pos)
{
    if (pos[0] < AREA_EDGE_MARGIN || pos[2] < AREA_EDGE_MARGIN) {
        return 0;
    }
    if (pos[0] < (f32)(gAreaGridCellX * gAreaGridRows) - AREA_EDGE_MARGIN && pos[2] < (f32)(gAreaGridCellZ * gAreaGridCols) - AREA_EDGE_MARGIN) {
        return 1;
    }
    return 0;
}

/* 1 when pos (x, z) is inside the area id, AREA_EDGE_MARGIN from its edges. */
s32 Area_ContainsPos(s32 id, f32 *pos)
{
    AreaRecord *size = Stage_GetAreaRecord(id);

    if (pos[0] < AREA_EDGE_MARGIN || pos[2] < AREA_EDGE_MARGIN) {
        return 0;
    }
    if (pos[0] < size->sizeX - AREA_EDGE_MARGIN && pos[2] < size->sizeZ - AREA_EDGE_MARGIN) {
        return 1;
    }
    return 0;
}

/* Moves the point target back along the line from from until it is inside
 * the first area grid (AREA_EDGE_MARGIN from its edges): -1 when from is
 * outside, else 1 when target was moved (0 when not), the point in out. */
s32 AreaGrid_ClampSegment(f32 *out, f32 *target, f32 *from)
{
    f32 step[3];
    f32 point[3];
    f32 maxx;
    f32 maxz;
    s32 moved;

    if (from[0] < AREA_EDGE_MARGIN || from[2] < AREA_EDGE_MARGIN) {
        return -1;
    }
    maxx = (f32)(gAreaGridCellX * gAreaGridRows) - AREA_EDGE_MARGIN;
    if (from[0] > maxx) {
        return -1;
    }
    maxz = (f32)(gAreaGridCellZ * gAreaGridCols) - AREA_EDGE_MARGIN;
    if (from[2] > maxz) {
        return -1;
    }
    moved = 0;
    Vec_Copy(point, target);
    if (point[0] < AREA_EDGE_MARGIN) {
        Vec_Diff(step, point, from);
        Vec_Scale(step, step, (AREA_EDGE_MARGIN - from[0]) / step[0]);
        Vec_Add(point, step, from);
        point[0] = AREA_EDGE_MARGIN;
        moved = 1;
    }
    if (point[0] > maxx) {
        Vec_Diff(step, point, from);
        Vec_Scale(step, step, (maxx - from[0]) / step[0]);
        Vec_Add(point, step, from);
        point[0] = maxx;
        moved = 1;
    }
    if (point[2] < AREA_EDGE_MARGIN) {
        Vec_Diff(step, point, from);
        Vec_Scale(step, step, (AREA_EDGE_MARGIN - from[2]) / step[2]);
        Vec_Add(point, step, from);
        point[2] = AREA_EDGE_MARGIN;
        moved = 1;
    }
    if (point[2] > maxz) {
        Vec_Diff(step, point, from);
        Vec_Scale(step, step, (maxz - from[2]) / step[2]);
        Vec_Add(point, step, from);
        point[2] = maxz;
        moved = 1;
    }
    Vec_Copy(out, point);
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

/* Capsule_TestTri, skipped (0) when the sweep's skipBelow is set and no
 * corner of the triangle is above the point from. */
s32 Capsule_TestTriLevel(f32 *cap, AreaTri *tri, f32 *from)
{
    f32 y;

    if (((SweepCap *)cap)->skipBelow != 0) {
        y = from[1];
        if (!(y < tri->v[0][1]) && !(y < tri->v[1][1]) && !(y < tri->v[2][1])) {
            return 0;
        }
    }
    return Capsule_TestTri(cap, tri);
}

/* Tests the sphere cap (a SweepCap: two points, then the radius) against the
 * triangle t and records a hit (gAreaHits.., gAreaHitCount hits, 9 at most): 1
 * when there is one. flags: 1 the start is behind the plane, 2 the segment
 * crosses it. */
s32 Capsule_TestTri(f32 *cap, AreaTri *tri)
{
    f32 point[3];
    f32 w[3];
    f32 nearest[3];
    f32 planePoint[3];
    f32 query[3];
    f32 *edges[4];
    AreaTri *hit;
    f32 dist;
    f32 scalar;
    s32 i;
    s8 flags;
    f32 **edge;


    if (gAreaHitCount >= AREA_HITS_MAX) {
        return 0;
    }
    Vec_Diff(w, cap + 3, tri->v[0]);
    if (Vec_Dot(w, tri->n) < 0.0f) {
        return 0;
    }
    Vec_Diff(w, cap, tri->v[0]);
    dist = Vec_Dot(w, tri->n);
    if (dist > 0.0f) {
        flags = 0;
    } else {
        flags = 1;
    }
    if (dist > cap[6]) {
        return 0;
    }
    if (dist >= -cap[6]) {
        Vec_Scale(point, tri->n, dist);
        Vec_Diff(point, cap, point);
    } else {
        Vec_Diff(w, cap, cap + 3);
        scalar = Vec_Dot(w, tri->n);
        if (scalar == 0.0f) {
            return 0;
        }
        Vec_Scale(w, w, -(dist / scalar));
        Vec_Add(point, cap, w);
        flags |= 2;
    }
    if (Tri_ContainsPoint(point, tri->v[0], tri->n)) {
        hit = &gAreaHits[gAreaHitCount];
        *hit = *tri;
        Vec_SnapSmall(hit->n, hit->n);
        gAreaHitCapsulePos[gAreaHitCount] = *(Vec3 *)cap;
        gAreaHitFlags[gAreaHitCount] = flags;
        gAreaHitPointKinds[gAreaHitCount] = 0;
        gAreaHitPoints[gAreaHitCount] = *(Vec3 *)point;
        gAreaHitCount++;
        return 1;
    }
    gAreaHitPointKinds[gAreaHitCount] = 1;
    Vec_Copy(planePoint, point);
    edges[0] = tri->v[0];
    edges[1] = tri->v[1];
    edges[2] = tri->v[2];
    edges[3] = tri->v[0];
    edge = edges;
    dist = 8000.0f;
    if (!(flags & 2)) {
        Vec_Copy(query, cap);
    } else {
        Vec_Copy(query, planePoint);
    }
    for (i = 3; i > 0; i--, edge++) {
        scalar = Seg_NearestPointDist(point, edge[0], edge[1], query);
        if (scalar < dist) {
            dist = scalar;
            Vec_Copy(nearest, point);
        }
    }
    if (dist < cap[6]) {
        hit = &gAreaHits[gAreaHitCount];
        *hit = *tri;
        Vec_SnapSmall(hit->n, hit->n);
        gAreaHitCapsulePos[gAreaHitCount] = *(Vec3 *)cap;
        gAreaHitFlags[gAreaHitCount] = flags;
        gAreaHitPoints[gAreaHitCount] = *(Vec3 *)nearest;
        gAreaHitPlanePoints[gAreaHitCount] = *(Vec3 *)planePoint;
        gAreaHitCount++;
        return 1;
    }
    return 0;
}

/* Where the segment seg (a SweepCap: two points, then the radius) first comes
 * within that radius of the line through the two points of line: 1 and the
 * point in out when it does before the segment's end and between the two
 * points. */
s32 Capsule_HitLine(f32 *seg, f32 *line, f32 *out)
{
    f32 dir[3];
    f32 perpDir[3];
    f32 perpOfs[3];
    f32 p[2][3];
    f32 *proj;
    f32 disc;
    f32 scalar;
    f32 rootHi;
    f32 coefB;
    f32 coefA;
    f32 coefC;
    f32 rootLo;

    Vec_Diff(dir, line + 3, line);
    Vec_Normalize(dir);
    Vec_Diff(p[0], seg, seg + 3);
    scalar = Vec_Dot(p[0], dir);
    proj = p[1];
    Vec_Scale(proj, dir, scalar);
    Vec_Diff(perpDir, proj, p[0]);
    coefA = Vec_Dot(perpDir, perpDir);
    Vec_Diff(p[0], seg + 3, line);
    Vec_Scale(proj, dir, Vec_Dot(p[0], dir));
    Vec_Diff(perpOfs, proj, p[0]);
    coefC = Vec_Dot(perpOfs, perpOfs);
    coefB = Vec_Dot(perpDir, perpOfs);
    if (coefA != 0.0f) {
        disc = coefB * coefB - coefA * (coefC - seg[6] * seg[6]);
        if (disc >= 0.0f) {
            coefC = 1.0f / coefA;
            scalar = Math_Sqrtf(disc);
            rootHi = coefC * (-coefB + scalar);
            rootLo = coefC * (-coefB - scalar);
            if (rootLo > rootHi) {
                rootLo = rootHi;
            }
            if (rootLo < 1.0f) {
                Vec_Scale(p[0], seg, rootLo);
                Vec_Scale(proj, seg + 3, 1.0f - rootLo);
                Vec_Add(out, p[0], proj);
                Vec_Diff(p[0], out, line);
                rootLo = Vec_Dot(p[0], dir);
                if (rootLo >= 0.0f) {
                    Vec_Diff(proj, line + 3, line);
                    if (rootLo <= Vec_Length(proj)) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

/* Where the segment seg (a SweepCap: two points, then the radius) first touches a
 * sphere of that radius around one of the three points corners: 1 and the point in
 * out when it does before the segment's end. */
s32 Capsule_HitPoints(f32 *seg, f32 *corners, f32 *out)
{
    f32 segDir[3];
    f32 ofs[3];
    f32 p[2][3];
    u32 i;
    f32 coefA;
    f32 coefB;
    f32 coefC;
    f32 best;
    f32 disc;
    f32 sqrtDisc;
    f32 rootLo;
    f32 rootHi;

    best = 10.0f;
    for (i = 0; i < 3; i++, corners += 3) {
        Vec_Diff(segDir, seg, seg + 3);
        coefA = Vec_Dot(segDir, segDir);
        Vec_Diff(ofs, seg + 3, corners);
        coefC = Vec_Dot(ofs, ofs);
        coefB = Vec_Dot(segDir, ofs);
        disc = coefB * coefB - coefA * (coefC - seg[6] * seg[6]);
        if (disc >= 0.0f) {
            coefC = 1.0f / coefA;
            sqrtDisc = Math_Sqrtf(disc);
            rootHi = coefC * (-coefB + sqrtDisc);
            rootLo = coefC * (-coefB - sqrtDisc);
            if (rootLo > rootHi) {
                rootLo = rootHi;
            }
            if (best > rootLo) {
                best = rootLo;
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
s32 Seg_HitPlane(f32 *seg, f32 *n, f32 *planePoint, f32 *out)
{
    f32 diff[3];
    f32 startDist;
    f32 denom;
    u32 side;

    side = 0;
    Vec_Diff(diff, seg + 3, planePoint);
    if (Vec_Dot(diff, n) >= 0.0f) {
        side |= 2;
    }
    Vec_Diff(diff, seg, planePoint);
    if ((startDist = Vec_Dot(diff, n)) >= 0.0f) {
        side |= 1;
    }
    switch (side) {
    case 0:
    case 3:
        return 0;
    case 1:
    case 2:
        Vec_Diff(diff, seg, seg + 3);
        denom = Vec_Dot(diff, n);
        if (denom == 0.0f) {
            return 0;
        }
        Vec_Scale(diff, diff, -(startDist / denom));
        Vec_Add(out, seg, diff);
        break;
    }
    return 1;
}

/* 1 when point is inside the triangle tri (three points) seen along n. */
s32 Tri_ContainsPoint(f32 *point, f32 *tri, f32 *n)
{
    f32 cross[3];

    Tri_Normal(tri, tri + 3, point, cross);
    if (Vec_Dot(cross, n) < 0.0f) {
        return 0;
    }
    Tri_Normal(tri + 3, tri + 6, point, cross);
    if (Vec_Dot(cross, n) < 0.0f) {
        return 0;
    }
    Tri_Normal(tri + 6, tri, point, cross);
    return !(Vec_Dot(cross, n) < 0.0f);
}

extern u8 gTriUnsharedCorner[];
s32 AreaHit_SpherePushOut(SweepCap *s, f32 *out, s8 id, s8 *flag);

#pragma optimization_level 4
/* The mask of e's vertices (bit b for e->v[b]) that are also vertices of f. */
static inline u8 AreaHit_SharedVerts(f32 *faceVert, AreaTri *edgeTri)
{
    f32 *edgeVert;
    s8 i;
    s8 j;
    u8 mask;
    f32 (*ev)[3];

    mask = 0;
    for (i = 0, ev = edgeTri->v; i < 3; i++, faceVert += 3) {
        edgeVert = ev[0];
        for (j = 0; j < 3; j++, edgeVert += 3) {
            if (faceVert[0] == edgeVert[0] && faceVert[1] == edgeVert[1] && faceVert[2] == edgeVert[2]) {
                mask += 1 << j;
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
    f32 push[3];
    s8 face[16];
    s8 edge[16];
    s8 kind[16];
    s8 skip[16];
    f32 ofs[4];
    f32 ofs2[4];
    f32 box[12];
    s32 i;
    s8 nskip;
    s8 nedge;
    s8 nface;
    s32 faceTotal;
    s32 j;
    s32 k;
    s8 found;
    s8 id;
    s8 edgeId;
    AreaTri *faceTri;
    AreaTri *skipTri;
    AreaTri *edgeTri;
    f32 *vert;
    f32 *vert2;
    f32 (*va)[3];
    u8 mask;
    u8 shared2;
    f32 len;
    f32 max;
    f32 scale;

    switch (gAreaHitCount) {
    case 0:
        break;
    case 1:
        if ((s8)AreaHit_SpherePushOut((SweepCap *)cap, push, 0, (s8 *)flag) != 0) {
            if (cap[7] != 0.0f) {
                len = Vec_Length(push);
                max = 1.8f * cap[7];
                if (!(len <= max)) {
                    scale = max / len;
                    push[0] *= scale;
                    push[1] *= scale;
                    push[2] *= scale;
                }
            }
            out[0] += push[0];
            out[1] += push[1];
            out[2] += push[2];
            gAreaHitIds[gAreaHitIdCount] = 0;
            gAreaHitIdCount++;
        }
        break;
    default:
        nskip = 0;
        for (i = 0, nface = 0, nedge = 0; i < gAreaHitCount; i++) {
            if (gAreaHitPointKinds[i] == 0) {
                face[nface] = i;
                nface++;
            } else {
                edge[nedge] = i;
                nedge++;
            }
        }
        faceTotal = nface;
        for (i = 0; i < faceTotal; i++) {
            for (j = 0; j < nedge; j++) {
                found = 0;
                for (k = 0; k < nskip; k++) {
                    if (edge[j] == skip[k]) {
                        found = 1;
                        break;
                    }
                }
                if (found == 1) {
                    continue;
                }
                edgeTri = &gAreaHits[edge[j]];
                faceTri = &gAreaHits[face[i]];
                if (faceTri->n[0] == edgeTri->n[0] && faceTri->n[1] == edgeTri->n[1] && faceTri->n[2] == edgeTri->n[2]) {
                    kind[nskip] = 1;
                    skip[nskip] = edge[j];
                    nskip++;
                    continue;
                }
                if (faceTri->n[1] == 1.0f || faceTri->n[1] == -1.0f) {
                    continue;
                }
                if (edgeTri->n[1] == 1.0f || edgeTri->n[1] == -1.0f) {
                    continue;
                }
                va = faceTri->v;
                mask = AreaHit_SharedVerts(va[0], &gAreaHits[edge[j]]);
                switch (gTriUnsharedCorner[mask]) {
                case 0:
                    vert = edgeTri->v[0];
                    break;
                case 1:
                    vert = edgeTri->v[1];
                    break;
                case 2:
                    vert = edgeTri->v[2];
                    break;
                default:
                    continue;
                }
                Vec_Diff(ofs, vert, va[0]);
                if (Vec_Dot(ofs, gAreaHits[face[i]].n) <= 0.0f) {
                    kind[nskip] = 2;
                    skip[nskip] = edge[j];
                    nskip++;
                }
            }
        }
        if (nedge >= 2) {
            for (i = 0; i < nedge - 1; i++) {
                found = 0;
                for (k = 0; k < nskip; k++) {
                    if (edge[i] == skip[k]) {
                        found = 1;
                        break;
                    }
                }
                if (found == 1) {
                    continue;
                }
                for (j = 1; j < nedge; j++) {
                    if (edge[i] == edge[j]) {
                        continue;
                    }
                    found = 0;
                    for (k = 0; k < nskip; k++) {
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
                        kind[nskip] = 3;
                        skip[nskip] = edge[j];
                        nskip++;
                    }
                }
            }
        }
        if (nskip > 0 && nskip < nedge) {
            for (i = 0; i < nskip; i++) {
                if (kind[i] != 1) {
                    continue;
                }
                for (j = 0; j < nedge; j++) {
                    if (edge[j] == skip[i]) {
                        continue;
                    }
                    skipTri = &gAreaHits[skip[i]];
                    if (skipTri->n[1] == 1.0f || skipTri->n[1] == -1.0f) {
                        continue;
                    }
                    edgeTri = &gAreaHits[edge[j]];
                    if (edgeTri->n[1] == 1.0f || edgeTri->n[1] == -1.0f) {
                        continue;
                    }
                    va = skipTri->v;
                    shared2 = AreaHit_SharedVerts(va[0], &gAreaHits[edge[j]]);
                    switch (gTriUnsharedCorner[shared2]) {
                    case 0:
                        vert2 = edgeTri->v[0];
                        break;
                    case 1:
                        vert2 = edgeTri->v[1];
                        break;
                    case 2:
                        vert2 = edgeTri->v[2];
                        break;
                    default:
                        continue;
                    }
                    Vec_Diff(ofs2, vert2, va[0]);
                    if (Vec_Dot(ofs2, gAreaHits[skip[i]].n) <= 0.0f) {
                        kind[nskip] = 2;
                        skip[nskip] = edge[j];
                        nskip++;
                    }
                }
            }
        }
        box[0] = box[3] = box[6] = box[9] = 0.0f;
        box[1] = box[4] = box[7] = box[10] = 0.0f;
        box[2] = box[5] = box[8] = box[11] = 0.0f;
        for (i = 0; i < faceTotal; i++) {
            id = face[i];
            if ((s8)AreaHit_SpherePushOut((SweepCap *)cap, push, id, (s8 *)flag) != 0) {
                Box_Accumulate(push, box, cap[6]);
                gAreaHitIds[gAreaHitIdCount] = id;
                gAreaHitIdCount++;
            }
        }
        for (i = 0; i < nedge; i++) {
            edgeId = edge[i];
            k = 0;
            for (j = 0; j < nskip; j++) {
                if (skip[j] == edgeId) {
                    k = 1;
                    break;
                }
            }
            if (k == 0 && (s8)AreaHit_SpherePushOut((SweepCap *)cap, push, edgeId, (s8 *)flag) != 0) {
                Box_Accumulate(push, box, cap[6]);
                gAreaHitIds[gAreaHitIdCount] = edgeId;
                gAreaHitIdCount++;
            }
        }
        if (box[3] != 0.0f) {
            if (box[0] != 0.0f) {
                push[0] = 0.5f * (box[0] + box[3]) + box[6] + box[9];
            } else {
                push[0] = box[3] + box[9];
            }
        } else {
            push[0] = box[0] + box[6];
        }
        if (box[4] != 0.0f) {
            if (box[1] != 0.0f) {
                push[1] = 0.5f * (box[1] + box[4]) + box[7] + box[10];
            } else {
                push[1] = box[4] + box[10];
            }
        } else {
            push[1] = box[1] + box[7];
        }
        if (box[5] != 0.0f) {
            if (box[2] != 0.0f) {
                push[2] = 0.5f * (box[2] + box[5]) + box[8] + box[11];
            } else {
                push[2] = box[5] + box[11];
            }
        } else {
            push[2] = box[2] + box[8];
        }
        if (cap[7] != 0.0f) {
            len = Vec_Length(push);
            max = 1.8f * cap[7];
            if (!(len <= max)) {
                scale = max / len;
                push[0] *= scale;
                push[1] *= scale;
                push[2] *= scale;
            }
        }
        out[0] += push[0];
        out[1] += push[1];
        out[2] += push[2];
        break;
    }
    return gAreaHitCount;
}
#pragma optimization_level reset

#pragma optimization_level 4
/* The push that moves the sphere of cap out of area hit id, in out: 1, or 0
 * when an edge or corner hit gives none. A face hit pushes along the
 * triangle's normal to the radius from the contact point; an edge or corner
 * hit pushes from the contact point toward the sphere's centre (gAreaHitFlags
 * 0) or toward its point on the plane (gAreaHitPlanePoints). With cap's flatPush
 * set, a push off a slanted normal or direction is made horizontal. Sets *flag
 * when the hit's gAreaHitFlags is 1 (the start behind the plane).
 * The point-kind test is a switch with its default written first, and the
 * second test of f casts it to s8: written so, the compiler lays out the
 * branches and the sign extension as the game's code does. */
s32 AreaHit_SpherePushOut(SweepCap *cap, f32 *out, s8 id, s8 *flag)
{
    f32 n[3];
    f32 step[3];
    f32 target[3];
    f32 center[3];
    Vec3 *contact;
    s32 i;
    s32 flat;

    i = id;
    switch (gAreaHitPointKinds[i]) {
    default:
        Vec_Copy(n, gAreaHits[i].n);
        flat = 0;
        if (cap->flatPush == 1) {
            if (!(Math_Absf(n[0]) < 0.001f) || !(Math_Absf(n[2]) < 0.001f)) {
                n[1] = 0.0f;
                flat = 1;
                Vec_Normalize(n);
            }
        }
        Vec_Scale(step, n, cap->r);
        Vec_Add(target, &gAreaHitPoints[i].x, step);
        if (flat != 0) {
            target[1] = gAreaHitCapsulePos[i].y;
        }
        Vec_Diff(out, target, &gAreaHitCapsulePos[i].x);
        break;
    case 1:
        switch (gAreaHitFlags[i]) {
        case 0:
            contact = &gAreaHitPoints[i];
            Vec_Diff(target, &gAreaHitCapsulePos[i].x, &contact->x);
            break;
        case 1:
        default:
            contact = &gAreaHitPoints[i];
            Vec_Diff(target, &gAreaHitPlanePoints[i].x, &contact->x);
            break;
        }
        flat = 0;
        if (cap->flatPush == 1 && (target[0] != 0.0f || target[1] == 0.0f || target[2] != 0.0f)) {
            target[1] = 0.0f;
            flat = 1;
        }
        Vec_Normalize(target);
        Vec_Scale(step, target, cap->r);
        Vec_Add(target, &contact->x, step);
        Vec_Set(center, gAreaHitCapsulePos[i].x, gAreaHitCapsulePos[i].y, gAreaHitCapsulePos[i].z);
        if ((s8)flat == 1) {
            center[1] = target[1];
        }
        Vec_Diff(step, target, center);
        if (Vec_Length(step) == 0.0f) {
            return 0;
        }
        Vec_Copy(out, step);
        break;
    }
    if (gAreaHitFlags[i] == 1) {
        *flag = 1;
    }
    return 1;
}
#pragma optimization_level reset

/* Accumulates push into box: the positive (box[0..2]) and negative (box[3..5])
 * parts of each axis up to lim, what goes past it in box[6..8] and box[9..11]. */
void Box_Accumulate(f32 *push, f32 *box, f32 lim)
{
    if (push[0] >= 0.0f) {
        if (box[0] + box[6] < push[0]) {
            if (push[0] <= lim) {
                box[0] = push[0];
            } else {
                box[0] = lim;
                box[6] = push[0] - lim;
            }
        }
    } else {
        if (box[3] + box[9] > push[0]) {
            if (-push[0] <= lim) {
                box[3] = push[0];
            } else {
                box[3] = -lim;
                box[9] = push[0] + lim;
            }
        }
    }
    if (push[1] >= 0.0f) {
        if (box[1] + box[7] < push[1]) {
            if (push[1] <= lim) {
                box[1] = push[1];
            } else {
                box[1] = lim;
                box[7] = push[1] - lim;
            }
        }
    } else {
        if (box[4] + box[10] > push[1]) {
            if (-push[1] <= lim) {
                box[4] = push[1];
            } else {
                box[4] = -lim;
                box[10] = push[1] + lim;
            }
        }
    }
    if (push[2] >= 0.0f) {
        if (box[2] + box[8] < push[2]) {
            if (push[2] <= lim) {
                box[2] = push[2];
            } else {
                box[2] = lim;
                box[8] = push[2] - lim;
            }
        }
    } else {
        if (box[5] + box[11] > push[2]) {
            if (-push[2] <= lim) {
                box[5] = push[2];
            } else {
                box[5] = -lim;
                box[11] = push[2] + lim;
            }
        }
    }
}

/* The floor height under pos in the second area grid: of the upward
 * triangles at (x, z) (5 at most), the highest one at most 50 above pos, else
 * the lowest one; pos[1] when there is none. */
f32 AreaGrid2_GetFloorY(f32 *pos)
{
    f32 pointXZ[3];
    f32 heights[AREA_FLOOR_CANDIDATES];
    f32 triXZ[3][3];
    f32 below[AREA_FLOOR_CANDIDATES];
    s32 *cell;
    s8 n;
    AreaTri *tri;
    AreaTri **list;
    f32 *heightPtr;
    s8 found;
    s8 j;
    s8 k;
    f32 result;

    n = 0;
    heights[0] = pos[1];
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            heightPtr = heights;
            list = (AreaTri **)cell;
            do {
                tri = *list;
                pointXZ[0] = pos[0];
                pointXZ[1] = pos[2];
                triXZ[0][0] = tri->v[0][0];
                triXZ[0][1] = tri->v[0][2];
                triXZ[1][0] = tri->v[1][0];
                triXZ[1][1] = tri->v[1][2];
                triXZ[2][0] = tri->v[2][0];
                triXZ[2][1] = tri->v[2][2];
                if (tri->n[1] > 0.0f && Tri_Contains2D((f32 *) triXZ, (f32 *) pointXZ) == 1 && n < AREA_FLOOR_CANDIDATES) {
                    n++;
                    *heightPtr++ = -(tri->d + (tri->n[0] * pos[0] + tri->n[2] * pos[2])) / tri->n[1];
                }
                list++;
            } while (*(s32 *)list != -1);
        }
    }
    if (n != 1) {
        found = 0;
        k = 0;
        for (j = 0; j < n; j++) {
            if (heights[j] <= pos[1] + 50.0f) {
                found = 1;
                below[k++] = heights[j];
            }
        }
        if (found == 1) {
            if (k == 1) {
                result = below[0];
            } else {
                heights[0] = below[0];
                for (j = 1; j < k; j++) {
                    if (heights[0] < below[j]) {
                        heights[0] = below[j];
                    }
                }
                result = heights[0];
            }
        } else {
            below[0] = heights[0];
            for (j = 1; j < n; j++) {
                if (below[0] > heights[j]) {
                    below[0] = heights[j];
                }
            }
            result = below[0];
        }
    } else {
        result = heights[0];
    }
    if (result == -0.0f) {
        result = 0.0f;
    }
    return result;
}

/* The same with 100 above pos, and only the triangles whose kind has neither
 * passThrough nor noLanding (gAreaKindFlags). */
f32 AreaGrid2_GetFloorYFiltered(f32 *pos)
{
    f32 pointXZ[3];
    f32 heights[AREA_FLOOR_CANDIDATES];
    f32 triXZ[3][3];
    f32 below[AREA_FLOOR_CANDIDATES];
    s32 *cell;
    s8 n;
    AreaTri *tri;
    AreaTri **list;
    f32 *heightPtr;
    s8 found;
    s8 j;
    s8 k;
    f32 result;
    f32 y;
    AreaKindRow *kindRow;
    u8 passThrough;
    u8 filtered;

    n = 0;
    heights[0] = pos[1];
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            heightPtr = heights;
            list = (AreaTri **)cell;
            do {
                tri = *list;
                pointXZ[0] = pos[0];
                pointXZ[1] = pos[2];
                triXZ[0][0] = tri->v[0][0];
                triXZ[0][1] = tri->v[0][2];
                triXZ[1][0] = tri->v[1][0];
                triXZ[1][1] = tri->v[1][2];
                triXZ[2][0] = tri->v[2][0];
                triXZ[2][1] = tri->v[2][2];
                if (tri->n[1] > 0.0f && Tri_Contains2D((f32 *) triXZ, (f32 *) pointXZ) == 1 && n < AREA_FLOOR_CANDIDATES) {
                    y = -(tri->d + (tri->n[0] * pos[0] + tri->n[2] * pos[2])) / tri->n[1];
                    kindRow = &gAreaKindFlags[gScene.area][tri->kind];
                    filtered = kindRow->noLanding;
                    passThrough = kindRow->passThrough;
                    if (!passThrough && !filtered) {
                        n++;
                        *heightPtr++ = y;
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
            if (heights[j] <= pos[1] + 100.0f) {
                found = 1;
                below[k++] = heights[j];
            }
        }
        if (found == 1) {
            if (k == 1) {
                result = below[0];
            } else {
                for (j = 0; j < k; j++) {
                    if (j == 0) {
                        heights[0] = below[j];
                    } else if (heights[0] < below[j]) {
                        heights[0] = below[j];
                    }
                }
                result = heights[0];
            }
        } else {
            for (j = 0; j < n; j++) {
                if (j == 0) {
                    below[0] = heights[j];
                } else if (below[0] > heights[j]) {
                    below[0] = heights[j];
                }
            }
            result = below[0];
        }
    } else {
        result = heights[0];
    }
    if (result == -0.0f) {
        result = 0.0f;
    }
    return result;
}

/* The floor height under the monster em at pos, in out: -1 (and its area
 * record's bottom) when it isn't in the current area, 0 (the current area
 * record's top) outside the second grid, else 1. Triangles whose kind has
 * passThrough are left out; one with surfaceKind sets em->surfaceKind. */
s32 Em_GetFloorY(Em *em, f32 *pos, f32 *out)
{
    f32 pointXZ[3];
    f32 heights[AREA_FLOOR_CANDIDATES];
    f32 triXZ[3][3];
    f32 below[AREA_FLOOR_CANDIDATES];
    s32 *cell;
    s8 n;
    AreaTri *tri;
    AreaTri **list;
    f32 *heightPtr;
    s8 found;
    s8 j;
    s8 k;
    f32 y;
    AreaKindRow *kindRow;
    u8 passThrough;
    u8 surface;

    n = 0;
    if ((u8)Obj_InCurrentArea(em) == 0) {
        *out = Stage_GetAreaRecord(em->area)->bottom;
        return -1;
    }
    heights[0] = pos[1];
    em->surfaceKind = 0;
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            heightPtr = heights;
            list = (AreaTri **)cell;
            do {
                tri = *list;
                pointXZ[0] = pos[0];
                pointXZ[1] = pos[2];
                triXZ[0][0] = tri->v[0][0];
                triXZ[0][1] = tri->v[0][2];
                triXZ[1][0] = tri->v[1][0];
                triXZ[1][1] = tri->v[1][2];
                triXZ[2][0] = tri->v[2][0];
                triXZ[2][1] = tri->v[2][2];
                if (tri->n[1] > 0.0f && Tri_Contains2D((f32 *) triXZ, (f32 *) pointXZ) == 1 && n < AREA_FLOOR_CANDIDATES) {
                    y = -(tri->d + (tri->n[0] * pos[0] + tri->n[2] * pos[2])) / tri->n[1];
                    kindRow = &gAreaKindFlags[em->area][tri->kind];
                    passThrough = kindRow->passThrough;
                    surface = kindRow->surfaceKind;
                    if (surface) {
                        em->surfaceKind = surface;
                    }
                    if (!passThrough) {
                        n++;
                        *heightPtr++ = y;
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
            if (heights[j] <= pos[1] + 50.0f) {
                found = 1;
                below[k++] = heights[j];
            }
        }
        if (found == 1) {
            if (k == 1) {
                *out = below[0];
            } else {
                for (j = 0; j < k; j++) {
                    if (j == 0) {
                        heights[0] = below[j];
                    } else if (heights[0] < below[j]) {
                        heights[0] = below[j];
                    }
                }
                *out = heights[0];
            }
        } else {
            for (j = 0; j < n; j++) {
                if (j == 0) {
                    below[0] = heights[j];
                } else if (below[0] > heights[j]) {
                    below[0] = heights[j];
                }
            }
            *out = below[0];
        }
    } else {
        *out = heights[0];
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
    f32 pointXZ[3];
    f32 heights[AREA_FLOOR_CANDIDATES];
    f32 triXZ[3][3];
    f32 below[AREA_FLOOR_CANDIDATES];
    s32 *cell;
    s8 n;
    AreaTri *tri;
    AreaTri **list;
    f32 *heightPtr;
    s8 found;
    s8 j;
    s8 k;
    f32 y;
    AreaKindRow *kindRow;
    u8 passThrough;
    u8 surface;

    n = 0;
    if ((u8)Obj_InCurrentArea(em) == 0) {
        *out = Stage_GetAreaRecord(em->area)->bottom;
        return -1;
    }
    heights[0] = pos[1];
    em->surfaceKind = 0;
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            heightPtr = heights;
            list = (AreaTri **)cell;
            do {
                tri = *list;
                pointXZ[0] = pos[0];
                pointXZ[1] = pos[2];
                triXZ[0][0] = tri->v[0][0];
                triXZ[0][1] = tri->v[0][2];
                triXZ[1][0] = tri->v[1][0];
                triXZ[1][1] = tri->v[1][2];
                triXZ[2][0] = tri->v[2][0];
                triXZ[2][1] = tri->v[2][2];
                if (tri->n[1] > 0.0f && Tri_Contains2D((f32 *) triXZ, (f32 *) pointXZ) == 1 && n < AREA_FLOOR_CANDIDATES) {
                    y = -(tri->d + (tri->n[0] * pos[0] + tri->n[2] * pos[2])) / tri->n[1];
                    kindRow = &gAreaKindFlags[em->area][tri->kind];
                    passThrough = kindRow->passThrough;
                    surface = kindRow->surfaceKind;
                    if (surface) {
                        em->surfaceKind = surface;
                    }
                    if (!passThrough) {
                        n++;
                        *heightPtr++ = y;
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
            if (heights[j] > pos[1]) {
                found = 1;
                below[k++] = heights[j];
            }
        }
        if (found == 0) {
            for (j = 0; j < n; j++) {
                if (j == 0) {
                    below[0] = heights[j];
                } else if (below[0] < heights[j]) {
                    below[0] = heights[j];
                }
            }
            *out = below[0];
        } else if (k == 1) {
            *out = below[0];
        } else {
            for (j = 0; j < k; j++) {
                if (j == 0) {
                    heights[0] = below[j];
                } else if (heights[0] > below[j]) {
                    heights[0] = below[j];
                }
            }
            *out = heights[0];
        }
    } else {
        *out = heights[0];
    }
    if (*out == -0.0f) {
        *out = 0.0f;
    }
    return 1;
}

/* The floor under the monster em at pos (as Em_GetFloorY), with the header of
 * its triangle in info (em's own floorKind, floorArg and floorAttr when there
 * is none; info's seArg is 2 while em->rideState is set): -1 when it isn't in
 * the current area, 0 outside the grid (both clear info and *surfaceY), else
 * 1. Clears em->floorUnderSurface and em->surfaceKind. */
s32 Em_GetFloorInfo(Em *em, f32 *pos, TriHdr *info, f32 *out, s32 *surfaceY)
{
    f32 pointXZ[3];
    TriHdr hdrs[AREA_FLOOR_CANDIDATES];
    f32 heights[AREA_FLOOR_CANDIDATES];
    f32 triXZ[3][3];
    f32 below[AREA_FLOOR_CANDIDATES];
    TriHdr belowHdr[AREA_FLOOR_CANDIDATES];
    s32 *cell;
    s8 n;
    AreaTri **list;
    AreaTri *tri;
    f32 *heightPtr;
    TriHdr *hdrPtr;
    s8 found;
    s8 j;
    s8 k;
    f32 y;

    n = 0;
    em->floorUnderSurface = 0;
    if ((u8)Obj_InCurrentArea(em) == 0) {
        *out = Stage_GetAreaRecord(em->area)->bottom;
        *surfaceY = 0;
        info->kind = 0;
        info->seArg = 0;
        info->attr = 0;
        return -1;
    }
    heights[0] = pos[1];
    hdrs[0].kind = em->floorKind;
    hdrs[0].seArg = em->floorArg;
    hdrs[0].attr = em->floorAttr;
    em->surfaceKind = 0;
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            heightPtr = heights;
            hdrPtr = hdrs;
            list = (AreaTri **)cell;
            do {
                tri = *list;
                pointXZ[0] = pos[0];
                pointXZ[1] = pos[2];
                triXZ[0][0] = tri->v[0][0];
                triXZ[0][1] = tri->v[0][2];
                triXZ[1][0] = tri->v[1][0];
                triXZ[1][1] = tri->v[1][2];
                triXZ[2][0] = tri->v[2][0];
                triXZ[2][1] = tri->v[2][2];
                if (tri->n[1] > 0.0f && Tri_Contains2D((f32 *) triXZ, (f32 *) pointXZ) == 1 && n < AREA_FLOOR_CANDIDATES) {
                    y = -(tri->d + (tri->n[0] * pos[0] + tri->n[2] * pos[2])) / tri->n[1];
                    if (gAreaKindFlags[em->area][tri->kind].passThrough == 0) {
                        n++;
                        *heightPtr++ = y;
                        hdrPtr->kind = tri->kind;
                        hdrPtr->seArg = tri->seArg;
                        hdrPtr->attr = tri->attr;
                        hdrPtr++;
                    }
                }
                list++;
            } while (*(s32 *)list != -1);
        }
    } else {
        *out = gAreaState.record->top;
        *surfaceY = 0;
        info->kind = 0;
        info->seArg = 0;
        info->attr = 0;
        return 0;
    }
    if (n != 1) {
        found = 0;
        k = 0;
        for (j = 0; j < n; j++) {
            if (heights[j] <= pos[1] + 50.0f) {
                found = 1;
                belowHdr[k] = hdrs[j];
                below[k] = heights[j];
                k++;
            }
        }
        if (found == 1) {
            if (k == 1) {
                *out = below[0];
                info->kind = belowHdr[0].kind;
                info->seArg = belowHdr[0].seArg;
                info->attr = belowHdr[0].attr;
            } else {
                for (j = 0; j < k; j++) {
                    if (j == 0) {
                        heights[0] = below[j];
                        hdrs[0].kind = belowHdr[j].kind;
                        hdrs[0].seArg = belowHdr[j].seArg;
                        hdrs[0].attr = belowHdr[j].attr;
                    } else if (heights[0] < below[j]) {
                        heights[0] = below[j];
                        hdrs[0].kind = belowHdr[j].kind;
                        hdrs[0].seArg = belowHdr[j].seArg;
                        hdrs[0].attr = belowHdr[j].attr;
                    }
                }
                *out = heights[0];
                info->kind = hdrs[0].kind;
                info->seArg = hdrs[0].seArg;
                info->attr = hdrs[0].attr;
            }
        } else {
            for (j = 0; j < n; j++) {
                if (j == 0) {
                    below[0] = heights[j];
                    belowHdr[0].kind = hdrs[j].kind;
                    belowHdr[0].seArg = hdrs[j].seArg;
                    belowHdr[0].attr = hdrs[j].attr;
                } else if (below[0] > heights[j]) {
                    below[0] = heights[j];
                    belowHdr[0].kind = hdrs[j].kind;
                    belowHdr[0].seArg = hdrs[j].seArg;
                    belowHdr[0].attr = hdrs[j].attr;
                }
            }
            *out = below[0];
            info->kind = belowHdr[0].kind;
            info->seArg = belowHdr[0].seArg;
            info->attr = belowHdr[0].attr;
        }
    } else {
        *out = heights[0];
        info->kind = hdrs[0].kind;
        info->seArg = hdrs[0].seArg;
        info->attr = hdrs[0].attr;
    }
    if (*out == -0.0f) {
        *out = 0.0f;
    }
    if (em->rideState) {
        info->seArg = 2;
    }
    return 1;
}

/* Em_GetFloorInfo with the special floors: a triangle whose kind has a surfaceKind
 * sets em->surfaceKind, gives its height in depth and its header in info, and
 * lets floors up to 100 above pos count; one with passThrough (not stood on)
 * gives its height less em->surfaceDepth in depth, and when that lies above
 * the floor found sets em->floorUnderSurface and gives its header. Outside the
 * current area EM_KIND_PLESIOTH and EM_KIND_GREEN_PLESIOTH take the area
 * record's top, the others its bottom. */
s32 Em_GetFloorInfoEx(Em *em, f32 *pos, TriHdr *info, f32 *out, f32 *depth)
{
    f32 pointXZ[3];
    TriHdr hdrs[AREA_FLOOR_CANDIDATES];
    f32 heights[AREA_FLOOR_CANDIDATES];
    f32 triXZ[3][3];
    f32 below[AREA_FLOOR_CANDIDATES];
    TriHdr belowHdr[AREA_FLOOR_CANDIDATES];
    s32 *cell;
    s8 n;
    AreaTri **list;
    AreaTri *tri;
    f32 *heightPtr;
    TriHdr *hdrPtr;
    s8 found;
    s8 j;
    s8 k;
    f32 y;
    AreaRecord *areaRec;
    AreaKindRow *kindRow;
    u8 passThrough;
    u8 surface;
    u8 low;
    u8 belowSurface;
    u8 surfaceTriKind;
    u8 surfaceTriSeArg;
    u16 surfaceTriAttr;
    u8 passTriKind;
    u8 passTriSeArg;
    u16 passTriAttr;

    n = 0;
    em->floorUnderSurface = 0;
    em->surfaceKind = 0;
    low = 0;
    if ((u8)Obj_InCurrentArea(em) == 0) {
        areaRec = Stage_GetAreaRecord(em->area);
        if (em->isEm && (em->type == EM_KIND_PLESIOTH || em->type == EM_KIND_GREEN_PLESIOTH)) {
            *out = areaRec->top;
        } else {
            *out = areaRec->bottom;
        }
        *(s32 *)depth = 0;
        info->kind = 0;
        info->seArg = 0;
        info->attr = 0;
        return -1;
    }
    heights[0] = pos[1];
    hdrs[0].kind = em->floorKind;
    hdrs[0].seArg = em->floorArg;
    hdrs[0].attr = em->floorAttr;
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            heightPtr = heights;
            hdrPtr = hdrs;
            list = (AreaTri **)cell;
            do {
                tri = *list;
                pointXZ[0] = pos[0];
                pointXZ[1] = pos[2];
                triXZ[0][0] = tri->v[0][0];
                triXZ[0][1] = tri->v[0][2];
                triXZ[1][0] = tri->v[1][0];
                triXZ[1][1] = tri->v[1][2];
                triXZ[2][0] = tri->v[2][0];
                triXZ[2][1] = tri->v[2][2];
                if (tri->n[1] > 0.0f && Tri_Contains2D((f32 *) triXZ, (f32 *) pointXZ) == 1 && n < AREA_FLOOR_CANDIDATES) {
                    y = -(tri->d + (tri->n[0] * pos[0] + tri->n[2] * pos[2])) / tri->n[1];
                    kindRow = &gAreaKindFlags[em->area][tri->kind];
                    passThrough = kindRow->passThrough;
                    surface = kindRow->surfaceKind;
                    if (surface) {
                        em->surfaceKind = surface;
                        *depth = y;
                        surfaceTriKind = tri->kind;
                        surfaceTriSeArg = tri->seArg;
                        surfaceTriAttr = tri->attr;
                    }
                    if (passThrough) {
                        low = 1;
                        y -= *(f32 *)&em->surfaceDepth;
                        *depth = y;
                        passTriKind = tri->kind;
                        passTriSeArg = tri->seArg;
                        passTriAttr = tri->attr;
                    } else {
                        *heightPtr++ = y;
                        hdrPtr->kind = tri->kind;
                        hdrPtr->seArg = tri->seArg;
                        hdrPtr->attr = tri->attr;
                        hdrPtr++;
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
        info->seArg = 0;
        info->attr = 0;
        return 0;
    }
    if (n != 1) {
        found = 0;
        k = 0;
        for (j = 0; j < n; j++) {
            if (em->surfaceKind) {
                if (heights[j] <= pos[1] + 100.0f) {
                    found = 1;
                    belowHdr[k] = hdrs[j];
                    below[k] = heights[j];
                    k++;
                }
            } else {
                if (heights[j] <= pos[1] + 50.0f) {
                    found = 1;
                    belowHdr[k] = hdrs[j];
                    below[k] = heights[j];
                    k++;
                }
            }
        }
        if (found == 1) {
            if (k == 1) {
                *out = below[0];
                info->kind = belowHdr[0].kind;
                info->seArg = belowHdr[0].seArg;
                info->attr = belowHdr[0].attr;
            } else {
                for (j = 0; j < k; j++) {
                    if (j == 0) {
                        heights[0] = below[j];
                        hdrs[0].kind = belowHdr[j].kind;
                        hdrs[0].seArg = belowHdr[j].seArg;
                        hdrs[0].attr = belowHdr[j].attr;
                    } else if (heights[0] < below[j]) {
                        heights[0] = below[j];
                        hdrs[0].kind = belowHdr[j].kind;
                        hdrs[0].seArg = belowHdr[j].seArg;
                        hdrs[0].attr = belowHdr[j].attr;
                    }
                }
                *out = heights[0];
                info->kind = hdrs[0].kind;
                info->seArg = hdrs[0].seArg;
                info->attr = hdrs[0].attr;
            }
        } else {
            for (j = 0; j < n; j++) {
                if (j == 0) {
                    below[0] = heights[j];
                    belowHdr[0].kind = hdrs[j].kind;
                    belowHdr[0].seArg = hdrs[j].seArg;
                    belowHdr[0].attr = hdrs[j].attr;
                } else if (below[0] > heights[j]) {
                    below[0] = heights[j];
                    belowHdr[0].kind = hdrs[j].kind;
                    belowHdr[0].seArg = hdrs[j].seArg;
                    belowHdr[0].attr = hdrs[j].attr;
                }
            }
            *out = below[0];
            info->kind = belowHdr[0].kind;
            info->seArg = belowHdr[0].seArg;
            info->attr = belowHdr[0].attr;
        }
    } else {
        *out = heights[0];
        info->kind = hdrs[0].kind;
        info->seArg = hdrs[0].seArg;
        info->attr = hdrs[0].attr;
    }
    belowSurface = 0;
    if (low && *out < *depth) {
        belowSurface = 1;
        info->kind = passTriKind;
        info->seArg = passTriSeArg;
        info->attr = passTriAttr;
    }
    if (em->surfaceKind) {
        info->kind = surfaceTriKind;
        info->seArg = surfaceTriSeArg;
        info->attr = surfaceTriAttr;
    }
    em->floorUnderSurface = belowSurface;
    if (*out == -0.0f) {
        *out = 0.0f;
    }
    return 1;
}

/* The height at pos (x, z) of a triangle of the second area grid that faces
 * up and whose kind is passThrough (gAreaKindFlags): 1 and the height in y. */
u8 AreaGrid2_GetFloorHeight(f32 *pos, f32 *y)
{
    f32 pointXZ[3];
    f32 triXZ[3][3];
    u8 hit;
    AreaTri *tri;
    AreaTri **list;
    s32 *cell;
    f32 height;

    hit = 0;
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            list = (AreaTri **)cell;
            do {
                tri = *list;
                pointXZ[0] = pos[0];
                pointXZ[1] = pos[2];
                triXZ[0][0] = tri->v[0][0];
                triXZ[0][1] = tri->v[0][2];
                triXZ[1][0] = tri->v[1][0];
                triXZ[1][1] = tri->v[1][2];
                triXZ[2][0] = tri->v[2][0];
                triXZ[2][1] = tri->v[2][2];
                if (tri->n[1] > 0.0f && Tri_Contains2D((f32 *) triXZ, (f32 *) pointXZ) == 1) {
                    height = -(tri->d + (tri->n[0] * pos[0] + tri->n[2] * pos[2])) / tri->n[1];
                    if (gAreaKindFlags[gScene.area][tri->kind].passThrough != 0) {
                        *y = height;
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
    f32 pointXZ[3];
    f32 triXZ[3][3];
    u8 hit;
    AreaTri *tri;
    AreaTri **list;
    s32 *cell;

    hit = 0;
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            list = (AreaTri **)cell;
            do {
                tri = *list;
                pointXZ[0] = pos[0];
                pointXZ[1] = pos[2];
                triXZ[0][0] = tri->v[0][0];
                triXZ[0][1] = tri->v[0][2];
                triXZ[1][0] = tri->v[1][0];
                triXZ[1][1] = tri->v[1][2];
                triXZ[2][0] = tri->v[2][0];
                triXZ[2][1] = tri->v[2][2];
                if (tri->n[1] < 0.0f && Tri_Contains2D((f32 *) triXZ, (f32 *) pointXZ) == 1) {
                    hit = 1;
                    *y = -(tri->d + (tri->n[0] * pos[0] + tri->n[2] * pos[2])) / tri->n[1];
                    info->kind = tri->kind;
                    info->seArg = tri->seArg;
                    info->attr = tri->attr;
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
 * and whose kind has unk3 set (gAreaKindFlags). */
u8 AreaGrid2_OnFlaggedFloor(f32 *pos)
{
    f32 pointXZ[3];
    f32 triXZ[3][3];
    u8 hit;
    AreaTri *tri;
    AreaTri **list;
    s32 *cell;

    hit = 0;
    if (AreaGrid2_Contains(pos) == 1) {
        cell = AreaGrid2_GetCell(pos);
        if (*cell != -1) {
            list = (AreaTri **)cell;
            do {
                tri = *list;
                pointXZ[0] = pos[0];
                pointXZ[1] = pos[2];
                triXZ[0][0] = tri->v[0][0];
                triXZ[0][1] = tri->v[0][2];
                triXZ[1][0] = tri->v[1][0];
                triXZ[1][1] = tri->v[1][2];
                triXZ[2][0] = tri->v[2][0];
                triXZ[2][1] = tri->v[2][2];
                if (tri->n[1] > 0.0f && Tri_Contains2D((f32 *) triXZ, (f32 *) pointXZ) == 1
                    && gAreaKindFlags[gScene.area][tri->kind].unk3 != 0) {
                    hit = 1;
                }
                list++;
            } while (*(s32 *)list != -1);
        }
    }
    return hit;
}

/* The list of the second area grid's cell that holds pos (x, z). */
s32 *AreaGrid2_GetCell(f32 *pos)
{
    s32 row;
    s32 col;

    row = pos[2] / (f32)gAreaGrid2CellZ;
    col = pos[0] / (f32)gAreaGrid2CellX;
    return gAreaGrid2Cells[col * gAreaGrid2Cols + row];
}


#pragma optimization_level 4
/* Tests the segment a -> b against the triangles of the first area grid in the
 * cells along it (stepped by the second grid's cell size; a cell is taken when
 * its centre is within about a cell's diagonal of the line), skipping those
 * whose attributes meet mask (AREA_TRI_ATTR_FLAGGED added while gScene.skipFlaggedTris is
 * set). Returns 1 at the first triangle the segment crosses, with the crossing
 * in out, else 0 with out = b. An end outside the grid gives 1 and that end. */
u8 AreaHit_Raycast(f32 *start, f32 *end, f32 *out, u16 mask)
{
    f32 hitPos[3];
    Vec3 delta;
    Vec3 dir;
    Vec3 pos;
    Vec3 nearest;
    Vec3 seg[2];
    s32 hits[AREA_RAY_TRIS];
    s32 startCellX;
    s32 j;
    s32 startCellZ;
    s32 cellsX;
    s32 stepX;
    s32 stepZ;
    s32 cellsZ;
    s32 i;
    s32 offsetX;
    s32 n;
    s32 hit;
    s32 *cell;
    s32 attrMask;
    s32 *first;
    AreaTri *tri;
    s32 *writePtr;
    s32 unusedZ;
    s32 k;
    f32 best;
    f32 lim;
    f32 along;
    f32 dist;

    hit = 0;
    best = Vec_Distance(start, end);
    if (AreaGrid_Contains(start) == 0) {
        Vec_Set(out, start[0], start[1], start[2]);
        return 1;
    }
    if (AreaGrid_Contains(end) == 0) {
        Vec_Set(out, end[0], end[1], end[2]);
        return 1;
    }
    if (gScene.skipFlaggedTris) {
        mask |= AREA_TRI_ATTR_FLAGGED;
    }
    startCellX = start[0] / (f32)gAreaGrid2CellX;
    startCellZ = start[2] / (f32)gAreaGrid2CellZ;
    cellsX = (s32)(end[0] / (f32)gAreaGrid2CellX) - startCellX;
    cellsZ = (s32)(end[2] / (f32)gAreaGrid2CellZ) - startCellZ;
    if (cellsX < 0) {
        stepX = -1;
        cellsX = 1 - cellsX;
    } else {
        stepX = 1;
        cellsX = cellsX + 1;
    }
    if (cellsZ < 0) {
        stepZ = -1;
        cellsZ = 1 - cellsZ;
    } else {
        stepZ = 1;
        cellsZ = cellsZ + 1;
    }
    Vec_Set((f32 *)&seg[1], start[0], start[1], start[2]);
    Vec_Set((f32 *)&seg[0], end[0], end[1], end[2]);
    Vec_Sub((f32 *)&delta, end, start);
    Vec_Set((f32 *)&dir, delta.x, 0.0f, delta.z);
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
    if (i < cellsX) {
        offsetX = 0;
        attrMask = mask;
        writePtr = hits;
        first = writePtr;
        do {
            for (j = 0; j < cellsZ; j++) {
                pos.x = (f32)(gAreaGrid2CellX >> 1) + (f32)gAreaGrid2CellX * (f32)(startCellX + offsetX);
                pos.z = (f32)(gAreaGrid2CellZ >> 1) + (f32)gAreaGrid2CellZ * (f32)(startCellZ + j * stepZ);
                pos.y = 0.0f;
                if (dir.x == 0.0f && dir.z == 0.0f) {
                    along = -dir.x * (start[0] - pos.x) - dir.z * (start[2] - pos.z);
                } else {
                    along = (-dir.x * (start[0] - pos.x) - dir.z * (start[2] - pos.z)) / (dir.x * dir.x + dir.z * dir.z);
                }
                nearest.x = start[0] + dir.x * along;
                nearest.z = start[2] + dir.z * along;
                nearest.y = 0.0f;
                if (Vec_Distance((f32 *)&pos, (f32 *)&nearest) <= lim) {
                    cell = AreaGrid_GetCell((f32 *)&pos);
                    if (cell != (s32 *)-1) {
                        for (k = 0; cell[k] != -1; k++) {
                            tri = (AreaTri *)cell[k];
                            if (tri->attr & attrMask) {
                                continue;
                            }
                            if (n != 0 && 0 < n && (s32)tri == *first) {
                                continue;
                            }
                            if (Seg_HitPlane((f32 *)seg, tri->n, tri->v[0], hitPos)) {
                                if (!Tri_ContainsPoint(hitPos, tri->v[0], tri->n)) {
                                    continue;
                                }
                                hit = 1;
                                dist = Vec_Distance(start, hitPos);
                                if (!(best <= dist)) {
                                    best = dist;
                                    Vec_Set(out, hitPos[0], hitPos[1], hitPos[2]);
                                }
                            }
                            if (n < AREA_RAY_TRIS) {
                                *writePtr++ = (s32)tri;
                                n++;
                            }
                            if (hit) {
                                return 1;
                            }
                        }
                    }
                }
            }
            offsetX += stepX;
        } while (++i < cellsX);
    }
    Vec_Set(out, end[0], end[1], end[2]);
    return 0;
}
#pragma optimization_level reset

#pragma optimization_level 4
/*
 * Tests the segment a -> b against the collision triangles of both area grids in the cells
 * along it (in the second grid skipping the passThrough kinds of gAreaKindFlags[em->area], or
 * also those with a surfaceKind for EM_KIND_CEPHADROME, EM_KIND_CEPHALOS, EM_KIND_DIABLOS and EM_KIND_MONOBLOS;
 * in the first grid those whose attributes meet mask): returns 1 with
 * the nearest hit in out, else 0 with out = b. An end outside a grid gives 1 and that end.
 */
s32 AreaGrid_TestSegment(Em *em, f32 *start, f32 *end, f32 *out, u16 mask)
{
    f32 hitPos[3];
    Vec3 delta;
    Vec3 dir;
    Vec3 pos;
    s32 hit;
    Vec3 seg[2];
    u8 skip;
    s32 startCellX;
    s32 startCellZ;
    s32 cellsX;
    s32 stepX;
    s32 stepZ;
    s32 cellsZ;
    s32 i;
    s32 j;
    s32 unusedZ;
    s32 n;
    s32 k;
    Vec3 nearest;
    s32 *cell;
    AreaKindRow *kindRow;
    s32 hits[AREA_RAY_TRIS];
    s32 *firstHit;
    AreaTri *grid2Tri;
    AreaTri *gridTri;
    f32 best;
    f32 lim;
    f32 along;
    f32 dist;

    hit = 0;
    best = Vec_Distance(start, end);
    if (AreaGrid2_Contains(start) == 0) {
        Vec_Set(out, start[0], start[1], start[2]);
        return 1;
    }
    if (AreaGrid2_Contains(end) == 0) {
        Vec_Set(out, end[0], end[1], end[2]);
        return 1;
    }
    if (AreaGrid_Contains(start) == 0) {
        Vec_Set(out, start[0], start[1], start[2]);
        return 1;
    }
    if (AreaGrid_Contains(end) == 0) {
        Vec_Set(out, end[0], end[1], end[2]);
        return 1;
    }
    startCellX = start[0] / (f32)gAreaGrid2CellX;
    startCellZ = start[2] / (f32)gAreaGrid2CellZ;
    cellsX = (s32)(end[0] / (f32)gAreaGrid2CellX) - startCellX;
    cellsZ = (s32)(end[2] / (f32)gAreaGrid2CellZ) - startCellZ;
    if (cellsX < 0) {
        stepX = -1;
        cellsX = 1 - cellsX;
    } else {
        stepX = 1;
        cellsX = cellsX + 1;
    }
    if (cellsZ < 0) {
        stepZ = -1;
        cellsZ = 1 - cellsZ;
    } else {
        stepZ = 1;
        cellsZ = cellsZ + 1;
    }
    Vec_Set((f32 *)&seg[1], start[0], start[1], start[2]);
    Vec_Set((f32 *)&seg[0], end[0], end[1], end[2]);
    Vec_Sub((f32 *)&delta, end, start);
    Vec_Set((f32 *)&dir, delta.x, 0.0f, delta.z);
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
    for (i = 0; i < cellsX; i++) {
        for (j = 0; j < cellsZ; j++) {
            pos.x = (f32)(gAreaGrid2CellX >> 1) + (f32)gAreaGrid2CellX * (f32)(startCellX + i * stepX);
            pos.z = (f32)(gAreaGrid2CellZ >> 1) + (f32)gAreaGrid2CellZ * (f32)(startCellZ + j * stepZ);
            pos.y = 0.0f;
            if (dir.x == 0.0f && dir.z == 0.0f) {
                along = -dir.x * (start[0] - pos.x) - dir.z * (start[2] - pos.z);
            } else {
                along = (-dir.x * (start[0] - pos.x) - dir.z * (start[2] - pos.z)) / (dir.x * dir.x + dir.z * dir.z);
            }
            nearest.x = start[0] + dir.x * along;
            nearest.z = start[2] + dir.z * along;
            nearest.y = 0.0f;
            if (Vec_Distance((f32 *)&pos, (f32 *)&nearest) <= lim) {
                cell = AreaGrid2_GetCell((f32 *)&pos);
                if (cell != (s32 *)-1) {
                    for (k = 0; cell[k] != -1; k++) {
                        grid2Tri = (AreaTri *)cell[k];
                        kindRow = &gAreaKindFlags[em->area][grid2Tri->kind];
                        skip = kindRow->passThrough;
                        if (em->type == EM_KIND_CEPHADROME || em->type == EM_KIND_CEPHALOS
                            || em->type == EM_KIND_DIABLOS || em->type == EM_KIND_MONOBLOS) {
                            skip |= kindRow->surfaceKind;
                        }
                        if (skip) {
                            continue;
                        }
                        if (n != 0) {
                            firstHit = hits;
                            if (0 < n && (s32)grid2Tri == *firstHit) {
                                continue;
                            }
                        }
                        if (Seg_HitPlane((f32 *)seg, grid2Tri->n, grid2Tri->v[0], hitPos)) {
                            if (!Tri_ContainsPoint(hitPos, grid2Tri->v[0], grid2Tri->n)) {
                                continue;
                            }
                            hit = 1;
                            dist = Vec_Distance(start, hitPos);
                            if (!(best <= dist)) {
                                best = dist;
                                Vec_Set(out, hitPos[0], hitPos[1], hitPos[2]);
                            }
                        }
                        if (n < AREA_RAY_TRIS) {
                            hits[n++] = (s32)grid2Tri;
                        }
                        if (hit) {
                            return 1;
                        }
                    }
                }
                cell = AreaGrid_GetCell((f32 *)&pos);
                if (cell != (s32 *)-1) {
                    for (k = 0; cell[k] != -1; k++) {
                        gridTri = (AreaTri *)cell[k];
                        if (gridTri->attr & mask) {
                            continue;
                        }
                        if (n != 0) {
                            firstHit = hits;
                            if (0 < n && (s32)gridTri == *firstHit) {
                                continue;
                            }
                        }
                        if (Seg_HitPlane((f32 *)seg, gridTri->n, gridTri->v[0], hitPos)) {
                            if (!Tri_ContainsPoint(hitPos, gridTri->v[0], gridTri->n)) {
                                continue;
                            }
                            hit = 1;
                            dist = Vec_Distance(start, hitPos);
                            if (!(best <= dist)) {
                                best = dist;
                                Vec_Set(out, hitPos[0], hitPos[1], hitPos[2]);
                            }
                        }
                        if (n < AREA_RAY_TRIS) {
                            hits[n++] = (s32)gridTri;
                        }
                        if (hit) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    Vec_Set(out, end[0], end[1], end[2]);
    return 0;
}
#pragma optimization_level reset

/* The floor triangle under em in the second grid (the highest of those at most
 * 50 above it, else the lowest): -1 outside the grid or with none; 1 when the
 * floor is below em; else 0, with out the Tri_PushOut offset for the floor's
 * slope (zero when it isn't steep enough), added to em's position when apply. */
s32 Em_FindFloorTri(Em *em, f32 *out, s32 apply)
{
    s32 slope;
    f32 pointXZ[3];
    AreaTri *tris[AREA_FLOOR_CANDIDATES];
    f32 heights[AREA_FLOOR_CANDIDATES];
    f32 q[3][3];
    AreaTri *belowT[AREA_FLOOR_CANDIDATES];
    f32 below[AREA_FLOOR_CANDIDATES];
    s32 *cell;
    s8 n;
    f32 *pos;
    AreaTri *tri;
    AreaTri **list;
    f32 *heightPtr;
    AreaTri **triPtr;
    s8 found;
    s8 j;
    s8 k;
    f32 y;
    u8 pushed;

    n = 0;
    pos = &em->posX;
    slope = 0;
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
    if (AreaGrid2_Contains(pos) == 0) {
        return -1;
    }
    cell = AreaGrid2_GetCell(pos);
    if (*cell != -1) {
        heightPtr = heights;
        triPtr = tris;
        list = (AreaTri **)cell;
        do {
            tri = *list;
            pointXZ[0] = pos[0];
            pointXZ[1] = pos[2];
            q[0][0] = tri->v[0][0];
            q[0][1] = tri->v[0][2];
            q[1][0] = tri->v[1][0];
            q[1][1] = tri->v[1][2];
            q[2][0] = tri->v[2][0];
            q[2][1] = tri->v[2][2];
            if (tri->n[1] > 0.0f && Tri_Contains2D((f32 *) q, (f32 *) pointXZ) == 1 && n < AREA_FLOOR_CANDIDATES) {
                n++;
                *heightPtr++ = -(tri->d + (tri->n[0] * pos[0] + tri->n[2] * pos[2])) / tri->n[1];
                *triPtr++ = tri;
            }
            list++;
        } while (*(s32 *)list != -1);
    }
    if (n) {
    if (n != 1) {
        found = 0;
        k = 0;
        for (j = 0; j < n; j++) {
            if (heights[j] <= 50.0f + pos[1]) {
                below[k] = heights[j];
                found = 1;
                belowT[k] = tris[j];
                k++;
            }
        }
        if (found == 1) {
            if (k == 1) {
                y = below[0];
                tri = belowT[0];
            } else {
                for (j = 0; j < k; j++) {
                    if (j == 0) {
                        heights[0] = below[j];
                        tris[0] = belowT[j];
                    } else if (heights[0] < below[j]) {
                        heights[0] = below[j];
                        tris[0] = belowT[j];
                    }
                }
                tri = tris[0];
                y = heights[0];
            }
        } else {
            for (j = 0; j < n; j++) {
                if (j == 0) {
                    below[0] = heights[j];
                    belowT[0] = tris[j];
                } else if (below[0] > heights[j]) {
                    below[0] = heights[j];
                    belowT[0] = tris[j];
                }
            }
            tri = belowT[0];
            y = below[0];
        }
    } else {
        y = heights[0];
        tri = tris[0];
    }
    if (!(y < pos[1])) {
        Vec_AngleToUp(tri->n, &slope);
        pushed = Tri_PushOut((f32 *)tri, em, &slope, out, gFloorSlopeDrop[0]);
        if (apply && pushed) {
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

/* For a floor whose slope (the angle of its normal from up, ANGLE_TURN to a turn,
 * as Vec_AngleToUp gives it) is at least FLOOR_SLIDE_SLOPE: the offset that brings the
 * point (obj's x, its y less slope / 10000 * dropScale, its z) onto the plane of tri
 * (its normal and distance), in out, and 1. A vertical slope (ANGLE_TURN / 4, a quarter turn) is taken
 * as 0. Below FLOOR_SLIDE_SLOPE it returns 0. */
u8 Tri_PushOut(f32 *tri, Em *obj, s32 *slope, f32 *out, f32 dropScale)
{
    f32 y;
    f32 normalLenSq;
    f32 dist;

    if (*slope < FLOOR_SLIDE_SLOPE) {
        return 0;
    }
    if (*slope == 16384) { /* a quarter turn */
        *slope = 0;
    }
    y = obj->posY - (f32)*slope / 10000.0f * dropScale;
    normalLenSq = tri[10] * tri[10] + tri[11] * tri[11] + tri[12] * tri[12];
    dist = tri[13] + (tri[10] * obj->posX + tri[11] * y + tri[12] * obj->posZ);
    out[0] = -(tri[10] * dist / normalLenSq);
    out[1] = -(tri[11] * dist / normalLenSq);
    out[2] = -(tri[12] * dist / normalLenSq);
    return 1;
}

/* The angle between dir and the up axis, ANGLE_TURN to a turn. */
void Vec_AngleToUp(f32 *dir, s32 *out)
{
    f32 upAxis[3];
    f32 cosAngle;

    upAxis[0] = 0.0f;
    upAxis[1] = 1.0f;
    upAxis[2] = 0.0f;
    cosAngle = Vec_Dot(dir, upAxis);
    if (cosAngle > 1.0f) {
        cosAngle = 1.0f;
    }
    *out = (u16)(s32)(65536.0f * Math_Acos(cosAngle) / 6.2831855f + 0.5f);
}

/* Where the moving sphere cap (a SweepCap: two points, then the radius) first
 * touches the triangle tri (its face, then its edges, then its corners): 1 and
 * the sphere center in out (gAreaHitCount counts the hits). */
s32 Capsule_HitTri(f32 *cap, AreaTri *tri, f32 *out)
{
    f32 move[3];
    f32 edgeHit[3];
    f32 seg[3][3];
    f32 tmp[3][3];
    f32 l[2][3];
    f32 *sweep;
    f32 *sweepB;
    f32 moveDotN;
    f32 frac;

    Vec_Diff(tmp[0], cap + 3, tri->v[0]);
    if (Vec_Dot(tmp[0], tri->n) < 0.0f) {
        return 0;
    }
    sweep = seg[0];
    Vec_Copy(sweep, cap);
    sweepB = seg[1];
    Vec_Copy(sweepB, cap + 3);
    seg[2][0] = cap[6];
    Vec_Diff(move, sweep, sweepB);
    if (move[0] == 0.0f && move[1] == 0.0f && move[2] == 0.0f) {
        Vec_Add((sweep + 3), sweep, tri->n);
        Vec_Diff(move, sweep, (sweep + 3));
        moveDotN = -1.0f;
    } else {
        moveDotN = Vec_Dot(move, tri->n);
        if (moveDotN > 0.0f) {
            return 0;
        }
    }
    if (moveDotN == 0.0f) {
        return 0;
    }
    Vec_Diff(tmp[0], tri->v[0], (sweep + 3));
    frac = Vec_Dot(tmp[0], tri->n);
    frac = (frac + sweep[6]) / moveDotN;
    if (frac >= 1.0f) {
        return 0;
    }
    if (frac < 0.0f) {
        return 0;
    }
    Vec_Lerp(tmp[0], sweep, (sweep + 3), frac);
    Vec_Scale(tmp[1], tri->n, sweep[6]);
    Vec_Diff(tmp[1], tmp[0], tmp[1]);
    if (Tri_ContainsPoint(tmp[1], tri->v[0], tri->n)) {
        Vec_Copy(out, tmp[0]);
        gAreaHitCount++;
        return 1;
    }
    if (Capsule_HitLine(sweep, tri->v[0], edgeHit) == 1) {
        Vec_Copy(out, edgeHit);
        gAreaHitCount++;
        return 1;
    }
    if (Capsule_HitLine(sweep, tri->v[1], edgeHit) == 1) {
        Vec_Copy(out, edgeHit);
        gAreaHitCount++;
        return 1;
    }
    Vec_Copy(l[0], tri->v[2]);
    Vec_Copy(l[1], tri->v[0]);
    if (Capsule_HitLine(sweep, l[0], edgeHit) == 1) {
        Vec_Copy(out, edgeHit);
        gAreaHitCount++;
        return 1;
    }
    if (Capsule_HitPoints(sweep, tri->v[0], tmp[0]) == 1) {
        Vec_Copy(out, tmp[0]);
        gAreaHitCount++;
        return 1;
    }
    return 0;
}

/* Sweeps the sphere of the given radius from from to target against the area grid cells
 * around it (the triangles not masked by mask; AREA_TRI_ATTR_FLAGGED added while
 * gScene.skipFlaggedTris is set): the hits go to gAreaHits.. (Capsule_TestTri), the
 * triangles hit to gAreaHitTris; out gets the last contact. 1 when there was
 * a hit. */
s32 AreaGrid_SweepSphere(f32 *from, f32 *target, f32 *out, u16 mask, f32 radius)
{
    Vec3 end;
    SweepCap cap;
    s32 seen[AREA_SEEN_TRIS];
    s32 cells;
    s32 quad;
    s32 i;
    s32 j;
    s32 nseen;
    s32 nhit;
    AreaTri *cellTri;
    s32 *list;
    s32 *cell;
    f32 distXZ;

    if (AreaGrid_ClampSegment((f32 *)&end, target, from) < 0) {
        return 0;
    }
    if (gScene.skipFlaggedTris) {
        mask |= AREA_TRI_ATTR_FLAGGED;
    }
    gAreaHitCount = 0;
    nseen = 0;
    nhit = 0;
    cap.a = end;
    cap.b = *(Vec3 *)from;
    cap.r = radius;
    cap.flatPush = 0;
    cap.skipBelow = 0;
    cap.len = Vec_Distance((f32 *)&cap.a, (f32 *)&cap.b);
    distXZ = Vec_DistanceXZ((f32 *)&cap.a, (f32 *)&cap.b);
    if (gAreaGridCellX <= gAreaGridCellZ) {
        cells = (s32)((distXZ + 2.0f * radius) / (f32)gAreaGridCellX) + 2;
    } else {
        cells = (s32)((distXZ + 2.0f * radius) / (f32)gAreaGridCellZ) + 2;
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
                        cellTri = (AreaTri *)*list;
                        if (!(cellTri->attr & mask)
                            && (nseen == 0 || List_Contains((s32)cellTri, seen, nseen) != 1)
                            && (nhit == 0 || AreaHitTris_Contains((s32)cellTri, nhit) != 1)) {
                            if (Capsule_HitTri((f32 *)&cap, cellTri, out)) {
                                cap.a = *(Vec3 *)out;
                                gAreaHitTris[nhit++] = (s32)cellTri;
                            }
                            seen[nseen++] = (s32)cellTri;
                            if (nseen >= AREA_SEEN_TRIS) {
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

/* Picks the object's own light colours (its lightColors, which
 * LightSet_SetColors gives lights 0 and 1) for the floor kind floorKind at pos:
 * the kind's lightZone in the object's area names an AreaLightZone of
 * gAreaLightZones, else its lightZone2 one of gAreaLightZones2. Within the
 * zone's far distance of its position the colours are entry nearColors of
 * gAreaLightColors (32 bytes: two rows of four floats) when within the near
 * one, else entry farColors; otherwise NULL (the area's). */
void Obj_UpdateLightColors(u8 floorKind, f32 *pos, ObjHead *obj)
{
    u8 *base;
    u8 zone;
    u8 zone2;
    u16 id;
    void **dst;
    AreaLightZone *zones;
    AreaLightZone *zoneRec;
    f32 dist;

    dst = (void **)&obj->lightColors;
    id = obj->area;
    zone = gAreaKindFlags[id][floorKind].lightZone;
    zone2 = gAreaKindFlags[id][floorKind].lightZone2;
    base = gAreaLightColors[id];

    if (base == NULL) {
        *dst = NULL;
        return;
    }
    if (zone && (zones = gAreaLightZones[id]) != NULL) {
        zoneRec = &zones[zone];
        dist = Vec_Distance(zoneRec->pos, pos);
        if (dist <= zoneRec->far) {
            if (dist <= zoneRec->near) {
                *dst = base + zoneRec->nearColors * 32;
            } else {
                *dst = base + zoneRec->farColors * 32;
            }
            return;
        }
    } else if (zone2 && (zones = gAreaLightZones2[id]) != NULL) {
        zoneRec = &zones[zone2];
        dist = Vec_Distance(zoneRec->pos, pos);
        if (dist <= zoneRec->far) {
            if (dist <= zoneRec->near) {
                *dst = base + zoneRec->nearColors * 32;
            } else {
                *dst = base + zoneRec->farColors * 32;
            }
            return;
        }
    }
    *dst = NULL;
}

/* Sets the object's mustCrouch from the mustCrouch of floorKind's row of
 * gAreaKindFlags in its area: 1 when set, else 0. */
void Obj_UpdateMustCrouch(u8 floorKind, void *unused, ObjHead *obj)
{
    u8 *flag = &obj->mustCrouch;

    if (gAreaKindFlags[obj->area][floorKind].mustCrouch == 0) {
        *flag = 0;
    } else {
        *flag = 1;
    }
}

/* Sets the object's floorHazard to the hazard of floorKind's row of
 * gAreaKindFlags in its area. */
void Obj_UpdateFloorHazard(u8 floorKind, void *unused, ObjHead *obj)
{
    obj->floorHazard = gAreaKindFlags[obj->area][floorKind].hazard;
}

/* A player's floor effects, each frame in the current area (Player_Update,
 * TownPc_Update): from the floor kind under it, its own light colours at its
 * position (Obj_UpdateLightColors), its mustCrouch and its floorHazard. */
void Obj_UpdateFloorEffects(ObjHead *obj)
{
    Obj_UpdateLightColors(obj->floorKind, obj->pos, obj);
    Obj_UpdateMustCrouch(obj->floorKind, obj->pos, obj);
    Obj_UpdateFloorHazard(obj->floorKind, obj->pos, obj);
}

/* For EM_KIND_FELYNE and EM_KIND_MELYNX, each frame (GmEm_Update): the
 * monster's own light colours from the floor kind under it (Obj_UpdateLightColors). */
void Em_UpdateLightColors(Em *em)
{
    if (em->type == EM_KIND_FELYNE || em->type == EM_KIND_MELYNX) {
        Obj_UpdateLightColors(em->floorKind, &em->posX, (ObjHead *)em);
    }
}

/* Nonzero when the kind of the floor under the object (its floorKind, in its
 * area) is passThrough in gAreaKindFlags: a floor the monsters are not
 * stood on (Em_GetFloorInfo), which effects still use. */
u8 Obj_OnPassThroughFloor(void *obj)
{
    return gAreaKindFlags[((ObjHead *)obj)->area][((ObjHead *)obj)->floorKind].passThrough;
}

/* Collides the body spheres of em (records {x, y, z, r} ended by r == -1:
 * gPlayerBodySpheres for a player; for a monster gEmWallSpheres by kind, or
 * gEmPushSpheresLobby in the lobby) with the walls of the area, sweeping each
 * from em's prevPos to its position: AreaHit_CollideSweep for a monster,
 * AreaHit_CollideCapsule for a player. keep 0 clears em->wallHitDirs first.
 * The loop reads em->collMask through a pointer cast at the member's offset:
 * read that way the compiler computes the field's address once before the
 * loop and lays the loop out as the game's code does; a plain em->collMask
 * read does not. */
void Em_CollideField(Em *em, u8 keep)
{
    s32 angles[3];
    f32 rot[3];
    Vec3 cap[2];
    f32 m[4][4];
    f32 *sph;
    f32 radius;
    f32 sizeX;
    f32 sizeY;
    f32 sizeZ;
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
    em->collideFlags &= 0xFE; /* bit 0 cleared */
    sizeX = em->scaleX;
    sizeY = em->scaleY;
    sizeZ = em->scaleZ;
    sph = gPlayerBodySpheres;
    if (em->isEm) {
        if (gScene.inLobby == 0) {
            sph = gEmWallSpheres[em->type];
            if (em->sizePct >= 60 && em->sizePct <= 200) { /* percent */
                sizeX = em->bodyScale;
                sizeY = sizeX;
                sizeZ = sizeX;
            }
        } else {
            sph = gEmPushSpheresLobby[em->type];
        }
    }
    for (; sph[3] != -1.0f; sph += 4) {
        radius = sph[3] * sizeX;
        if (em->isEm && (em->type == EM_KIND_LAO_SHAN_LUNG || em->type == EM_KIND_ASHEN_LAO_SHAN_LUNG)) {
            angles[0] = 0;
            angles[1] = em->rotY;
            angles[2] = 0;
            Mtx_FromAnglesYXZ(angles, (f32 *)m);
            Vec_Rotate(rot, sph, (f32 *)m);
            cap[0].x = em->prevPosX + rot[0] * sizeX;
            cap[0].y = em->prevPosY + rot[1] * sizeY;
            cap[0].z = em->prevPosZ + rot[2] * sizeZ;
            cap[1].x = em->posX + rot[0] * sizeX;
            cap[1].y = em->posY + rot[1] * sizeY;
            cap[1].z = em->posZ + rot[2] * sizeZ;
        } else {
            cap[0].x = em->prevPosX + sph[0] * sizeX;
            cap[0].y = em->prevPosY + sph[1] * sizeY;
            cap[0].z = em->prevPosZ + sph[2] * sizeZ;
            cap[1].x = em->posX + sph[0] * sizeX;
            cap[1].y = em->posY + sph[1] * sizeY;
            cap[1].z = em->posZ + sph[2] * sizeZ;
        }
        if (em->isEm) {
            mask = *(u16 *)((u8 *)em + (u32)&((Em *)0)->collMask);
            if (em->stance == EM_STANCE_AIR) {
                mask |= AREA_TRI_ATTR_NOT_AIR;
            }
            if (gScene.skipFlaggedTris) {
                mask |= AREA_TRI_ATTR_FLAGGED;
            }
            AreaHit_CollideSweep((SweepCap *)cap, mask, (s32)em, radius);
        } else if (gScene.skipFlaggedTris) {
            AreaHit_CollideCapsule((SweepCap *)cap, AREA_TRI_ATTR_PLAYER_ON_WALL | AREA_TRI_ATTR_FLAGGED | AREA_TRI_ATTR_NO_PLAYER, em, radius);
        } else {
            AreaHit_CollideCapsule((SweepCap *)cap, AREA_TRI_ATTR_PLAYER_ON_WALL | AREA_TRI_ATTR_NO_PLAYER, em, radius);
        }
    }
}

/* The file's data, from gAreaQuadOffsets (0x41D1F0) to gPlayerBodySpheres. */
f32 gAreaQuadOffsets[4][2] = {
#include "sub_main/0041D1F0.inc"
};
u8 gTriUnsharedCorner[16] = {
#include "sub_main/0041D210.inc"
};
f32 gFloorSlopeDrop[44] = {
#include "sub_main/0041D220.inc"
};
f32 gPlayerBodySpheres[8] = {
#include "sub_main/0041D2D0.inc"
};
