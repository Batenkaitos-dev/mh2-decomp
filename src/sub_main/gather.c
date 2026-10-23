/*
 * Gathering points and the gathering item roll.
 *
 * The current map's gathering points come from the stage data
 * (Gather_GetPoints(area)): 24-byte records, ended by x == -1.0f. The player
 * gathers at a point when his height is within [y - 200, y + 100) and his
 * horizontal distance from it is at most its radius (Player_GetGatherPoint
 * finds the point he stands at; Gather_AtPoint gathers). Each gather rolls an
 * item from the point's table (Gather_RollItem), then may use the point up:
 * a point has `count` gathers left (GATHER_UNLIMITED: unlimited); after its `bonus`
 * gathers, each gather empties it with chance (c + 1) / 32, where c is 4,
 * or with a skill of the gathering group 0, 2, 8 or 12 (SKILL_GATHER_*).
 *
 * The action spots (Player_FindActionSpot, Player_RecordActionSpot) are the
 * places in an area where the player can act (seen: kinds 2 and 11 in the
 * player's states): the same kind of list, from Stage_GetActionSpots.
 * The records (GatherPoint, GatherEntry, ActionSpot) are in quest.h.
 */
#include "common.h"
#include "player.h"
#include "item.h"
#include "quest.h"
#include "mine_point.h"
#include "engine_math.h"
#include "rand.h"
#include "stage_data.h"

u8 Player_GetSkill(Player *p, u8 group);

/* The gathering point the player stands at (within its radius, from 200
 * below it to 100 above): returns its item table, with its action
 * (MINE_ACTION_*) in *action and its position in pos; GATHER_NO_ITEM when
 * there is none. */
u16 Player_GetGatherPoint(Player *pl, u16 *action, f32 *pos)
{
    GatherPoint *point;
    f32 dx;
    f32 dz;
    u16 table;

    point = Gather_GetPoints(pl->area);
    if (point == NULL) {
        return GATHER_NO_ITEM;
    }
    for (; point->x != -1.0f; point++) {
        if (pl->pos[1] < point->y - 200.0f) {
            continue;
        }
        if (!(pl->pos[1] < point->y + 100.0f)) {
            continue;
        }
        dx = pl->pos[0] - point->x;
        dz = pl->pos[2] - point->z;
        if (!(Math_Sqrtf(dx * dx + dz * dz) <= point->radius)) {
            continue;
        }
        table = point->table;
        *action = point->action;
        pos[0] = point->x;
        pos[1] = point->y;
        pos[2] = point->z;
        return table;
    }
    return GATHER_NO_ITEM;
}

/*
 * Gathers at the point the player stands at: the item (0 or GATHER_NO_ITEM
 * for nothing), GATHER_USED_UP if the point is used up, GATHER_NO_ITEM if
 * the map has no points, 0 when the player is at none.
 */
u16 Gather_AtPoint(Player *pl)
{
    GatherPoint *point;
    u16 item;
    u16 chance;
    f32 dx;
    f32 dz;

    point = Gather_GetPoints(pl->area);
    if (point == NULL) {
        return GATHER_NO_ITEM;
    }
    for (; point->x != -1.0f; point++) {
        if (pl->pos[1] < point->y - 200.0f) {
            continue;
        }
        if (!(pl->pos[1] < point->y + 100.0f)) {
            continue;
        }
        dx = pl->pos[0] - point->x;
        dz = pl->pos[2] - point->z;
        if (!(Math_Sqrtf(dx * dx + dz * dz) <= point->radius)) {
            continue;
        }
        if (point->count > 0) {
            item = Gather_RollItem(point->table & GATHER_TABLE_MASK);
            if (item != 0 && item != GATHER_NO_ITEM) {
                chance = 4;
                if (Player_GetSkill(pl, SKILL_GROUP_GATHER) != 0) {
                    if (Player_HasSkill(pl, SKILL_GATHER_PLUS2) == 1) {
                        chance = 0;
                    } else if (Player_HasSkill(pl, SKILL_GATHER_PLUS1) == 1) {
                        chance = 2;
                    } else if (Player_HasSkill(pl, SKILL_GATHER_MINUS1) == 1) {
                        chance = 8;
                    } else if (Player_HasSkill(pl, SKILL_GATHER_MINUS2) == 1) {
                        chance = 12;
                    }
                }
                if (point->count != GATHER_UNLIMITED) {
                    if (point->bonus > 0) {
                        point->bonus--;
                    }
                    if (point->bonus > 0) {
                        point->count--;
                    } else if ((Rand_Next(1) & 0x1F) > chance) {
                        point->count--;
                    } else {
                        point->count = 0;
                    }
                }
            }
        } else {
            return GATHER_USED_UP;
        }
        return item;
    }
    return 0;
}

/*
 * The action spot the player stands at (within 50 up or down and its
 * radius): copies its position to pos, its angle to *angle and its type to
 * *type, and returns its kind (0 if none).
 */
u16 Player_FindActionSpot(Player *pl, f32 *pos, u16 *angle, u8 *type)
{
    ActionSpot *spot;
    f32 dx;
    f32 dz;

    spot = Stage_GetActionSpots(pl->area);
    if (spot == NULL) {
        return 0;
    }
    for (; spot->x != -1.0f; spot++) {
        if (pl->pos[1] < spot->y - 50.0f) {
            continue;
        }
        if (!(pl->pos[1] < spot->y + 50.0f)) {
            continue;
        }
        dx = pl->pos[0] - spot->x;
        dz = pl->pos[2] - spot->z;
        if (!(Math_Sqrtf(dx * dx + dz * dz) <= spot->radius)) {
            continue;
        }
        pos[0] = spot->x;
        pos[1] = spot->y;
        pos[2] = spot->z;
        *angle = spot->angle;
        *type = spot->type;
        return spot->kind;
    }
    return 0;
}

/* Records the action spot the player stands at in pl->spot, or NULL. A
 * kind 2 spot reaches 30 further while the player fishes (PLF_FISHING). */
void Player_RecordActionSpot(Player *pl)
{
    ActionSpot *spot;
    f32 dx;
    f32 dz;
    f32 dist;

    spot = Stage_GetActionSpots(pl->area);
    if (spot == NULL) {
        pl->spot = NULL;
        return;
    }
    for (; spot->x != -1.0f; spot++) {
        if (pl->pos[1] < spot->y - 50.0f) {
            continue;
        }
        if (!(pl->pos[1] < spot->y + 50.0f)) {
            continue;
        }
        dx = pl->pos[0] - spot->x;
        dz = pl->pos[2] - spot->z;
        dist = Math_Sqrtf(dx * dx + dz * dz);
        if (spot->kind == 2 && Player_TestFlag(pl, PLF_FISHING) != 0) {
            dist -= 30.0f;
        }
        if (dist <= spot->radius) {
            pl->spot = spot;
            return;
        }
    }
    pl->spot = NULL;
}

/*
 * Rolls an item from gathering table `table`: {weight, item} pairs ended by
 * weight GATHER_END, drawn with random stream 0. GATHER_NO_ITEM if the roll
 * falls past the last.
 */
u16 Gather_RollItem(u16 table)
{
    GatherEntry *entry;
    u16 sum;
    u16 acc;
    u16 roll;

    sum = 0;
    for (entry = Gather_GetTable(table); entry->weight != GATHER_END; entry++) {
        sum += entry->weight;
    }
    roll = Rand_Next(0) % sum;
    acc = 0;
    for (entry = Gather_GetTable(table); entry->weight != GATHER_END; entry++) {
        acc += entry->weight;
        if (roll < acc) {
            return entry->item;
        }
    }
    return GATHER_NO_ITEM;
}
