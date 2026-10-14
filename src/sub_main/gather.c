/*
 * Gathering points and the gathering item roll.
 *
 * The current map's gathering points come from the stage data
 * (Gather_GetPoints(area)): 24-byte records, ended by x == -1.0f. The player
 * gathers at a point when his height is within [y - 200, y + 100) and his
 * horizontal distance from it is at most its radius. Each gather rolls an
 * item from the point's table (Gather_RollItem), then may use the point up:
 * a point has `count` gathers left (0xFF: unlimited); after its `bonus`
 * gathers, each gather empties it with chance (c + 1) / 32, where c is 4,
 * or 0, 2, 8, 12 with skills 0x94, 0x93, 0x95, 0x96.
 */
#include "common.h"
#include "player.h"

typedef struct GatherPoint {
    f32 x;              /* 0x00; -1.0f ends the list */
    f32 y;
    f32 z;
    f32 radius;         /* 0x0C */
    u16 table;          /* 0x10: item table id (bit 15 is a flag) */
    u16 count;          /* 0x12: gathers left, 0xFF = unlimited */
    u16 unk14;
    u16 bonus;          /* 0x16: gathers before the point can run out */
} GatherPoint;

typedef struct GatherEntry {
    u16 weight;         /* 0xFFFF ends the table */
    u16 item;
} GatherEntry;

typedef struct ActionSpot {
    u16 type;           /* 0x00 */
    u16 kind;           /* 0x02 */
    f32 x;              /* 0x04; -1.0f ends the list */
    f32 y;
    f32 z;
    f32 radius;         /* 0x10 */
    u16 unk14;
} ActionSpot;

void *Gather_GetPoints(s32 area);
void *Stage_GetActionSpots(s32 area);
void *Gather_GetTable(s32 table);
u32 Player_TestFlag(void *obj, u32 flag);
s32 Player_HasSkill(Player *p, u8 skill);
u8 Player_GetSkill(Player *p, u8 group);
f32 Math_Sqrtf(f32 x);   /* sqrtf */
u16 Rand_Next(s32 stream);
u16 Gather_RollItem(u16 table);

/*
 * Gathers at the point the player stands at: the item (0 or 0xFFFF for
 * nothing), 0xFFFE if the point is used up, 0xFFFF if the map has none.
 */
u16 Gather_AtPoint(Player *p)
{
    GatherPoint *pt;
    u16 item;
    u16 chance;
    f32 dx;
    f32 dz;

    pt = Gather_GetPoints(p->area);
    if (pt == NULL) {
        return 0xFFFF;
    }
    for (; pt->x != -1.0f; pt++) {
        if (p->pos[1] < pt->y - 200.0f) {
            continue;
        }
        if (!(p->pos[1] < pt->y + 100.0f)) {
            continue;
        }
        dx = p->pos[0] - pt->x;
        dz = p->pos[2] - pt->z;
        if (!(Math_Sqrtf(dx * dx + dz * dz) <= pt->radius)) {
            continue;
        }
        if (pt->count > 0) {
            item = Gather_RollItem(pt->table & 0x7FFF);
            if (item != 0 && item != 0xFFFF) {
                chance = 4;
                if (Player_GetSkill(p, 0x16) != 0) {
                    if (Player_HasSkill(p, 0x94) == 1) {
                        chance = 0;
                    } else if (Player_HasSkill(p, 0x93) == 1) {
                        chance = 2;
                    } else if (Player_HasSkill(p, 0x95) == 1) {
                        chance = 8;
                    } else if (Player_HasSkill(p, 0x96) == 1) {
                        chance = 12;
                    }
                }
                if (pt->count != 0xFF) {
                    if (pt->bonus > 0) {
                        pt->bonus--;
                    }
                    if (pt->bonus > 0) {
                        pt->count--;
                    } else if ((Rand_Next(1) & 0x1F) > chance) {
                        pt->count--;
                    } else {
                        pt->count = 0;
                    }
                }
            }
        } else {
            return 0xFFFE;
        }
        return item;
    }
    return 0;
}

/*
 * The action spot the player stands at: copies its position, returns its
 * kind (0 if none).
 */
u16 Player_FindActionSpot(Player *p, f32 *pos, u16 *arg, u8 *type)
{
    ActionSpot *s;
    f32 dx;
    f32 dz;

    s = Stage_GetActionSpots(p->area);
    if (s == NULL) {
        return 0;
    }
    for (; s->x != -1.0f; s++) {
        if (p->pos[1] < s->y - 50.0f) {
            continue;
        }
        if (!(p->pos[1] < s->y + 50.0f)) {
            continue;
        }
        dx = p->pos[0] - s->x;
        dz = p->pos[2] - s->z;
        if (!(Math_Sqrtf(dx * dx + dz * dz) <= s->radius)) {
            continue;
        }
        pos[0] = s->x;
        pos[1] = s->y;
        pos[2] = s->z;
        *arg = s->unk14;
        *type = s->type;
        return s->kind;
    }
    return 0;
}

/* Records the action spot the player stands at in p->spot (kind 2 spots
 * reach 30 further with flag 0x80000). */
void Player_RecordActionSpot(Player *p)
{
    ActionSpot *s;
    f32 dx;
    f32 dz;
    f32 d;

    s = Stage_GetActionSpots(p->area);
    if (s == NULL) {
        p->spot = NULL;
        return;
    }
    for (; s->x != -1.0f; s++) {
        if (p->pos[1] < s->y - 50.0f) {
            continue;
        }
        if (!(p->pos[1] < s->y + 50.0f)) {
            continue;
        }
        dx = p->pos[0] - s->x;
        dz = p->pos[2] - s->z;
        d = Math_Sqrtf(dx * dx + dz * dz);
        if (s->kind == 2 && Player_TestFlag(p, 0x80000) != 0) {
            d -= 30.0f;
        }
        if (d <= s->radius) {
            p->spot = s;
            return;
        }
    }
    p->spot = NULL;
}

/*
 * Rolls an item from gathering table `table`: {weight, item} pairs ended by
 * weight 0xFFFF, drawn with random stream 0.
 */
u16 Gather_RollItem(u16 table)
{
    GatherEntry *e;
    u16 sum;
    u16 acc;
    u16 r;

    sum = 0;
    for (e = Gather_GetTable(table); e->weight != 0xFFFF; e++) {
        sum += e->weight;
    }
    r = Rand_Next(0) % sum;
    acc = 0;
    for (e = Gather_GetTable(table); e->weight != 0xFFFF; e++) {
        acc += e->weight;
        if (r < acc) {
            return e->item;
        }
    }
    return 0xFFFF;
}
