/*
 * Where an object stands: the floor ahead of the player, above or below his
 * height (Player_FloorAheadAbove, Player_FloorAheadBelow, which the player's
 * states use), whether an object is in the current area or two are in the
 * same one, and an object's map position: its position in the area plus the
 * area's origin on the map (Player_UpdateAreaPos, Area_ToWorld). The
 * players, the monsters and the attack objects all use these, read through
 * the shared object header.
 */
#include "common.h"
#include "quest.h"
#include "player.h"
#include "em.h"
#include "objhead.h"
#include "engine_math.h"
#include "stage_data.h"
#include "vec_math.h"
#include "area.h"
/* end of prototypes */

/* Looks for the floor `fwd` ahead of the player (along its heading). Returns
 * 1 with its height in *out when it lies between `rise` and `rise + range`
 * above the player's height; 0 when it lies elsewhere; 1, *out unchanged,
 * when there is no floor there. */
s32 Player_FloorAheadAbove(Player *pl, f32 *out, f32 fwd, f32 rise, f32 range)
{
    f32 mtx[16];
    f32 point[3];
    f32 offset[3];
    s32 angles[3];
    f32 floorY;
    f32 *pointY;
    f32 *pointZ;
    f32 minY;

    pointY = &point[1];
    pointZ = &point[2];
    point[0] = 0.0f;
    *pointY = 0.0f;
    *pointZ = fwd;
    angles[0] = 0;
    angles[1] = pl->rot[1];
    angles[2] = 0;
    Mtx_FromAnglesXYZ(angles, mtx);
    Vec_Rotate(offset, point, mtx);
    point[0] = pl->pos[0] + offset[0];
    *pointY = pl->pos[1] + offset[1];
    *pointZ = pl->pos[2] + offset[2];
    minY = *pointY + rise;
    if (Em_GetFloorYAbove((struct Em *)pl, point, &floorY) != 1) {
        return 1;
    }
    if (!(floorY < minY) && floorY <= minY + range) {
        *out = floorY;
        return 1;
    }
    return 0;
}

/* Compares the floor `fwd` ahead of the player (along its heading) with the
 * player's height plus `rise`: 1 when the floor is below it (above it, with
 * `above`), 0 when not or when there is no floor there. */
s32 Player_FloorAheadBelow(Player *pl, u16 above, f32 fwd, f32 rise)
{
    f32 mtx[16];
    f32 point[3];
    f32 offset[3];
    s32 angles[3];
    f32 floorY;
    f32 *pointY;
    f32 *pointZ;
    f32 refY;

    pointY = &point[1];
    pointZ = &point[2];
    point[0] = 0.0f;
    *pointY = 0.0f;
    *pointZ = fwd;
    angles[0] = 0;
    angles[1] = pl->rot[1];
    angles[2] = 0;
    Mtx_FromAnglesXYZ(angles, mtx);
    Vec_Rotate(offset, point, mtx);
    point[0] = pl->pos[0] + offset[0];
    *pointY = pl->pos[1] + offset[1];
    *pointZ = pl->pos[2] + offset[2];
    refY = pl->pos[1] + rise;
    if (Em_GetFloorYAbove((struct Em *)pl, point, &floorY) != 1) {
        return 0;
    }
    if (above == 0) {
        if (floorY < refY) {
            return 1;
        }
    } else if (!(floorY <= refY)) {
        return 1;
    }
    return 0;
}

/* Nonzero when the object is in the current area (gScene.area). */
s32 Obj_InCurrentArea(void *obj)
{
    ObjHead *head = obj;
    return head->area == gScene.area;
}

/* The same as Obj_InCurrentArea, as a byte. */
u8 Player_InCurrentArea(void *obj)
{
    ObjHead *head = obj;
    return head->area == gScene.area;
}

/* Nonzero when two players (or a player and a monster) are in the same area. */
s32 Obj_SameArea(void *obj, Player *other)
{
    ObjHead *head = obj;
    return head->area == other->area;
}

/* Sets the object's map position: its position in the area plus the area's
 * origin on the map (AreaRecord), the height from the middle of the area's
 * vertical range. */
void Player_UpdateAreaPos(void *obj)
{
    ObjHead *head = obj;
    AreaRecord *rec = Stage_GetAreaRecord(head->area);

    head->mapPos[0] = head->pos[0] + rec->originX;
    head->mapPos[1] = head->pos[1] + (rec->bottom + rec->top) / 2.0f;
    head->mapPos[2] = head->pos[2] + rec->originZ;
}

/* Player_UpdateAreaPos for a position in area `area`, into out. */
void Area_ToWorld(s32 area, f32 *pos, f32 *out)
{
    AreaRecord *rec = Stage_GetAreaRecord(area);

    out[0] = pos[0] + rec->originX;
    out[1] = pos[1] + (rec->bottom + rec->top) / 2.0f;
    out[2] = pos[2] + rec->originZ;
}

