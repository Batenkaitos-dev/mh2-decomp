/*
 * The accessors of a monster's script set (Em.script, loaded with the
 * monster's data by GmLoad_GetEmData), for the script interpreter in
 * unk_70A290.c (EmScript_*) and the trigger picker EmScript_PickTrigger
 * (unk_733EA0.c).
 *
 * A script set is an array of tables of script pointers, one table per kind:
 *   [0]  the main scripts (Em.scriptNo; Em_StartScript)
 *   [1]  the subscripts of command 0x81 (Em.scriptCallNo)
 *   [2]  the loop scripts (Em.loopScript, Em.loopEndScript), also read as
 *        the loop's point lists (Em.loopPointTable)
 *   [3]  the scripts of trigger 0x10, [4] of trigger 0x20
 *   [5]  the area sequences' area lists, [6] their scripts
 *   [7]  the weighted area choices (EmScript_PickWeighted)
 *   [8]  the scripts of trigger 2
 *   [9]  the scripts EmScript_Call runs (Em.subScriptNo)
 *   [10] the scripts of trigger 4, [11] of trigger 8
 *   [12] the route points' scripts (Em.routePointScript)
 *   [13] the script of trigger 0x80, [14] of trigger 0x40 (one each)
 *   [15 + k] the jump tables of command 0x82
 * A table the monster has no use for is NULL, and its accessor returns NULL;
 * EmScript_AreaSeqScript is the one that doesn't check.
 *
 * Em.scriptFlags says which script thread runs (0x20 or 0x10 a trigger's
 * script, 0x01 the loop, none the main script); EmScript_GetPos and
 * EmScript_SetPos keep each thread's position apart.
 */
#include "common.h"
#include "em.h"

/* Returns the position of the running script thread. */
u8 *EmScript_GetPos(Em *em)
{
    u8 flags = em->scriptFlags;

    if (flags & 0x20) {
        return em->trigger20Pos;
    }
    if (flags & 0x10) {
        return em->trigger10Pos;
    }
    if (flags & 0x01) {
        return em->loopScriptPos;
    }
    return em->mainScriptPos;
}

/* Stores pos as the position of the running script thread. */
void EmScript_SetPos(Em *em, u8 *pos)
{
    u8 flags = em->scriptFlags;

    if (flags & 0x20) {
        em->trigger20Pos = pos;
    } else if (flags & 0x10) {
        em->trigger10Pos = pos;
    } else if (flags & 0x01) {
        em->loopScriptPos = pos;
    } else {
        em->mainScriptPos = pos;
    }
}

/* Returns loop script `entry` (table 2), or NULL. */
void *EmScript_LoopScript(Em *em, u8 entry)
{
    if (em->script[2] == NULL) {
        return NULL;
    }
    return em->script[2][entry];
}

/* Returns the script of trigger 0x10 numbered `entry` (table 3), or NULL. */
void *EmScript_Trigger10Script(Em *em, u8 entry)
{
    if (em->script[3] == NULL) {
        return NULL;
    }
    return em->script[3][entry];
}

/* Returns the script of trigger 0x20 numbered `entry` (table 4), or NULL. */
void *EmScript_Trigger20Script(Em *em, u8 entry)
{
    if (em->script[4] == NULL) {
        return NULL;
    }
    return em->script[4][entry];
}

/* Returns the script of trigger 2 numbered `entry` (table 8), or NULL. */
void *EmScript_Trigger02Script(Em *em, u8 entry)
{
    if (em->script[8] == NULL) {
        return NULL;
    }
    return em->script[8][entry];
}

/* Returns the script of trigger 4 numbered `entry` (table 10), or NULL. */
void *EmScript_Trigger04Script(Em *em, u8 entry)
{
    if (em->script[10] == NULL) {
        return NULL;
    }
    return em->script[10][entry];
}

/* Returns the script of trigger 8 numbered `entry` (table 11), or NULL. */
void *EmScript_Trigger08Script(Em *em, u8 entry)
{
    if (em->script[11] == NULL) {
        return NULL;
    }
    return em->script[11][entry];
}

/* Returns the script EmScript_Call runs, `entry` of table 9, or NULL. */
void *EmScript_SubScript(Em *em, u8 entry)
{
    if (em->script[9] == NULL) {
        return NULL;
    }
    return em->script[9][entry];
}

/* Returns area sequence `entry`'s list of areas (u16 ids, table 5), or NULL. */
void *EmScript_AreaSeqList(Em *em, u8 entry)
{
    if (em->script[5] == NULL) {
        return NULL;
    }
    return em->script[5][entry];
}

/* Returns area sequence `entry`'s script (table 6); the table is not checked. */
void *EmScript_AreaSeqScript(Em *em, u8 entry)
{
    return em->script[6][entry];
}

/* Returns the loop's point list `entry` (bytes: area point numbers; table 2,
 * the table of the loop scripts), or NULL. */
void *EmScript_LoopPointList(Em *em, u8 entry)
{
    if (em->script[2] == NULL) {
        return NULL;
    }
    return em->script[2][entry];
}

/* Returns route point script `entry` (table 12), or NULL. */
void *EmScript_RoutePointScript(Em *em, u8 entry)
{
    if (em->script[12] == NULL) {
        return NULL;
    }
    return em->script[12][entry];
}

/* Returns the script of trigger 0x80 (table 13 holds one), or NULL. */
void *EmScript_Trigger80Script(Em *em)
{
    if (em->script[13] == NULL) {
        return NULL;
    }
    return *em->script[13];
}

/* Returns the script of trigger 0x40 (table 14 holds one), or NULL. */
void *EmScript_Trigger40Script(Em *em)
{
    if (em->script[14] == NULL) {
        return NULL;
    }
    return *em->script[14];
}
