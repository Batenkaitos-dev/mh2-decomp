/*
 * Script accessors of a monster (Em): the resume position chosen by the flag byte at
 * +0x78F (Em.scriptFlags), and entries of the tables of its script set (Em.script; NULL when a
 * table is missing).
 */
#include "common.h"
#include "em.h"

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

void EmScript_SetPos(Em *em, u8 *value)
{
    u8 flags = em->scriptFlags;

    if (flags & 0x20) {
        em->trigger20Pos = value;
    } else if (flags & 0x10) {
        em->trigger10Pos = value;
    } else if (flags & 0x01) {
        em->loopScriptPos = value;
    } else {
        em->mainScriptPos = value;
    }
}

void *EmScript_GetTable08(Em *em, u8 i)
{
    if (em->script[2] == NULL) {
        return NULL;
    }
    return em->script[2][i];
}

void *EmScript_Trigger10Script(Em *em, u8 i)
{
    if (em->script[3] == NULL) {
        return NULL;
    }
    return em->script[3][i];
}

void *EmScript_Trigger20Script(Em *em, u8 i)
{
    if (em->script[4] == NULL) {
        return NULL;
    }
    return em->script[4][i];
}

void *EmScript_Trigger02Script(Em *em, u8 i)
{
    if (em->script[8] == NULL) {
        return NULL;
    }
    return em->script[8][i];
}

void *EmScript_Trigger04Script(Em *em, u8 i)
{
    if (em->script[10] == NULL) {
        return NULL;
    }
    return em->script[10][i];
}

void *EmScript_Trigger08Script(Em *em, u8 i)
{
    if (em->script[11] == NULL) {
        return NULL;
    }
    return em->script[11][i];
}

void *EmScript_SubScript(Em *em, u8 i)
{
    if (em->script[9] == NULL) {
        return NULL;
    }
    return em->script[9][i];
}

void *EmScript_GetTable14(Em *em, u8 i)
{
    if (em->script[5] == NULL) {
        return NULL;
    }
    return em->script[5][i];
}

void *EmScript_GetTable18(Em *em, u8 i)
{
    return em->script[6][i];
}

void *EmScript_GetTable08b(Em *em, u8 i)
{
    if (em->script[2] == NULL) {
        return NULL;
    }
    return em->script[2][i];
}

void *EmScript_GetTable30(Em *em, u8 i)
{
    if (em->script[12] == NULL) {
        return NULL;
    }
    return em->script[12][i];
}

void *EmScript_Trigger80Script(Em *em)
{
    if (em->script[13] == NULL) {
        return NULL;
    }
    return *em->script[13];
}

void *EmScript_Trigger40Script(Em *em)
{
    if (em->script[14] == NULL) {
        return NULL;
    }
    return *em->script[14];
}
