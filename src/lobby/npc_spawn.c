/*
 * The townspeople's placing. Npc_Spawn and Npc_SpawnArea place the current
 * area's townspeople from the per-area tables (gAreaNpcTables, gAreaNpcCounts,
 * gAreaNpcSpawns), Npc_Exists and Npc_Find look one up by id. The data holds
 * the area tables, the route lines, the town's area sets and the townspeople's
 * handler tables (gNpcHandlers, by body; the free townspeople's set runs their
 * motion sounds, Npc_RunMotionSounds). The townsperson's task and set-up, then
 * its motion sounds, come before this file.
 */
#include "common.h"
#include "lobby_npc.h"
#include "em.h"
#include "npc_motion.h"

u8 Area_GetSetIndex(u16 id);
/* unit's own declaration: the definition takes (u8 kind, s32 area, u8 force); with those types the compiler converts Npc_SpawnArea's arguments differently from the game's code */
s32 Npc_IsPresent(u8 kind, u16 area, s32 force);
extern u8 *gAreaNpcSpawns[];

extern NpcAreaEntry *gAreaNpcTables[];

/* Places townsperson kind (an NpcWork.id) from the current area's table: for the
 * first entry with that id it takes a free Em slot, marks it a townsperson task
 * (ownTask) of the current area with the entry's index, and sets it up from the
 * entry (Npc_SetupFromArea). The set-up is called even when no slot was free, as
 * in the game. */
void Npc_Spawn(s32 npcId)
{
    s32 area = Area_GetSetIndex(gScene.area);
    NpcAreaEntry *entry = gAreaNpcTables[area];
    s32 i;
    Em *em;

    for (i = 0; i < gAreaNpcCounts[area]; i++) {
        if (npcId == entry->id) {
            em = EmSlot_Alloc();
            if (em != NULL) {
                em->ownTask = 1;
                em->modelSlot = 0;
                em->index = i;
                em->area = gScene.area;
            }
            Npc_SetupFromArea(em, 1, gScene.area);
            return;
        }
        entry++;
    }
}

/* Returns 1 when townsperson kind (an NpcWork.id; 255: none) has an active Em
 * slot, else 0. */
s32 Npc_Exists(u8 npcId)
{
    Em *em;
    s32 i;

    if (npcId == 255) {
        return 0;
    }
    em = gEms;
    for (i = 0; i < 20; i++, em++) {
        if (em->active != 0 && ((NpcWork *)&em->work)->id == npcId) {
            return 1;
        }
    }
    return 0;
}

/* Returns the active Em slot of townsperson kind (an NpcWork.id), or NULL. */
Em *Npc_Find(u8 npcId)
{
    Em *em = gEms;
    s32 i;

    for (i = 0; i < 20; i++, em++) {
        if (em->active != 0 && ((NpcWork *)&em->work)->id == npcId) {
            return em;
        }
    }
    return NULL;
}

/* Places the current area's alternating townspeople (gAreaNpcSpawns[area]: pairs
 * of ids, 255 ends the list): when neither of a pair is placed, the first one
 * if it is present now (Npc_IsPresent), else the second if it is. */
void Npc_SpawnArea(void)
{
    NpcPair *pair = (NpcPair *)gAreaNpcSpawns[Area_GetSetIndex(gScene.area)];

    if (pair == NULL) {
        return;
    }
    while (pair->first != 255) {
        if (Npc_Exists(pair->first) == 0 && Npc_Exists(pair->second) == 0) {
            if (Npc_IsPresent(pair->first, gScene.area, 0) == 1) {
                Npc_Spawn(pair->first);
            } else if (Npc_IsPresent(pair->second, gScene.area, 0) == 1) {
                Npc_Spawn(pair->second);
            }
        }
        pair++;
    }
}

/* The file's data: the per-area townspeople tables and spawn pairs, the route
 * lines (gNpcSidePairs, gNpcSideLines), the partner pairs, the town's area
 * files and sets, and the townspeople's handler tables (gNpcHandlers by body). */
/* The routes and the route tables by time of day (townspeople.c) that the
 * area tables' entries start with (NpcAreaEntry.route). */
extern NpcRouteEnt *gNpc01Routes1[];
extern NpcRouteEnt gNpcRoute02[];
extern NpcRouteEnt gNpcRoute03Am[];
extern NpcRouteEnt gNpcRoute04Am[];
extern NpcRouteEnt gNpcRoute05Am[];
extern NpcRouteEnt *gNpc06Routes[];
extern NpcRouteEnt gNpcRoute07[];
extern NpcRouteEnt gNpcRoute08[];
extern NpcRouteEnt *gNpc09Routes1[];
extern NpcRouteEnt gNpcRoute0A[];
extern NpcRouteEnt gNpcRoute0B[];
extern NpcRouteEnt gNpcRoute0C[];
extern NpcRouteEnt gNpcRoute0D[];
extern NpcRouteEnt gSmithRouteBase[];
extern NpcRouteEnt gWorkshopHelperRoute[];
extern NpcRouteEnt *gNpc10Routes0[];
extern NpcRouteEnt gNamingTalkerRoute[];
extern NpcRouteEnt gNpcRoute13Mid[];
extern NpcRouteEnt gPoogieRouteVillage[];
extern NpcRouteEnt gPoogieRouteHouse0[];
extern NpcRouteEnt gPoogieRouteHouse1[];
extern NpcRouteEnt gPoogieRouteHouse2[];
extern NpcRouteEnt gNpcRoute15[];
extern NpcRouteEnt gNpcRoute16[];
extern NpcRouteEnt gNpcRoute17[];
extern NpcRouteEnt gNpcRoute18[];
extern NpcRouteEnt gFishingSpotRoute[];
extern NpcRouteEnt gNpcRoute1ABase[];
extern NpcRouteEnt gNpcRoute1B[];
extern NpcRouteEnt gNpcRoute2C[];
extern NpcRouteEnt gNpcRoute2D[];
extern NpcRouteEnt gNpcRoute2E[];
extern NpcRouteEnt gNpcRoute2F[];
extern NpcRouteEnt *gNpc30Routes[];
extern NpcRouteEnt *gNpc31Routes[];
extern NpcRouteEnt *gNpc32Routes[];
extern NpcRouteEnt gFishingSpotOnlineRoute[];
extern NpcRouteEnt gNpcRoute34[];
extern NpcRouteEnt gNpcRoute35[];
extern NpcRouteEnt gNpcRoute36[];
extern NpcRouteEnt gNpcRoute37[];
extern NpcRouteEnt gNpcRoute38[];
extern NpcRouteEnt gNpcRoute39[];
extern NpcRouteEnt gNpcRoute3A[];
extern NpcRouteEnt gNpcRoute3B[];
extern NpcRouteEnt gNpcRoute3C[];
extern NpcRouteEnt gNpcRoute3D[];
extern NpcRouteEnt gNpcRoute3E[];
extern NpcRouteEnt gNpcRoute3F[];
extern NpcRouteEnt gOfficialDeskLowRoute[];
extern NpcRouteEnt gNpcRoute41[];
extern NpcRouteEnt gNpcRoute42[];
extern NpcRouteEnt *gNpc45Routes[];
extern NpcRouteEnt *gNpc46Routes[];
extern NpcRouteEnt *gNpc47Routes[];
extern NpcRouteEnt *gNpc49Routes[];
extern NpcRouteEnt *gNpc4ARoutes[];
extern NpcRouteEnt *gNpc4BRoutes[];
extern NpcRouteEnt *gNpc4CRoutes[];
extern NpcRouteEnt gNpcRoute4D[];
extern NpcRouteEnt gOfficialDeskHighRoute[];
extern NpcRouteEnt gQuestDesk2Route[];
extern NpcRouteEnt gNpcRoute50[];
extern NpcRouteEnt gNpcRoute51[];
extern NpcRouteEnt gNpcRoute52[];
extern NpcRouteEnt gNpcRoute53[];
extern NpcRouteEnt gNpcRoute54[];
extern NpcRouteEnt gNpcRoute55[];
extern NpcRouteEnt gNpcRoute56[];
extern NpcRouteEnt gNpcRoute57[];
extern NpcRouteEnt gLessonRouteBHome[];
extern NpcRouteEnt gNpcRoute59[];
extern NpcRouteEnt gNpcRoute5A[];
extern NpcRouteEnt gNpcRoute5B[];
extern NpcRouteEnt gLessonRouteAHome[];
extern NpcRouteEnt gNpcRoute5D[];
extern NpcRouteEnt gNpcRoute5E[];
extern NpcRouteEnt *gNpc5FRoutes[];
extern NpcRouteEnt *gNpc60Routes[];
extern NpcRouteEnt *gNpc61Routes[];
extern NpcRouteEnt *gNpc62Routes[];
extern NpcRouteEnt gNpcRoute64[];
extern NpcRouteEnt gNpcRoute65[];
extern NpcRouteEnt gNpcRoute66[];
extern NpcRouteEnt *gNpc67Routes[];
extern NpcRouteEnt gNpcRoute68[];
extern NpcRouteEnt *gNpc69Routes[];
extern NpcRouteEnt gNpcRoute6A[];
extern NpcRouteEnt gNpcRoute6B[];
extern NpcRouteEnt gNpcRoute6C[];
extern NpcRouteEnt gHousePetRoute[];
extern NpcRouteEnt gNpcRoute6F[];
extern NpcRouteEnt gHairdresserRoute[];
extern NpcRouteEnt gNpcRouteKitchen_34[];
extern NpcRouteEnt gKitchenRoute0_44[];
extern NpcRouteEnt gNpcPlaceRoutes0_0[];
extern NpcRouteEnt gNpcPlaceRoutes0_1[];
extern NpcRouteEnt gNpcPlaceRoutes1_0[];
extern NpcRouteEnt gNpcPlaceRoutes1_1[];
extern NpcRouteEnt gNpcPlaceRoutes1_2[];
extern NpcRouteEnt gNpcPlaceRoutes1_3[];
extern NpcRouteEnt gNpcPlaceRoutes1_4[];
extern NpcRouteEnt gNpcPlaceRoutes1_5[];
extern NpcRouteEnt gNpcPlaceRoutes2_6[];
extern NpcRouteEnt gNpcPlaceRoutes2_7[];
extern NpcRouteEnt gNpcPlaceRoutes2_8[];
extern NpcRouteEnt gNpcPlaceRoutes2_9[];
extern NpcRandJumpRoute gNpcRoute6E;
extern u32 gPetHandlers[];
extern u32 gServiceNpcHandlers[];
extern u32 gStationedNpcHandlers[];
extern void *gNpcCommonHandlers[];
extern void *gNpcNopHandlers[];
u8 gAreaNpcSpawns_0[16] = {
#include "lobby/007FBEA0.inc"
};
NpcAreaEntry gAreaNpcTables_1[11] = {
#include "lobby/007FBEB0.inc"
};
u8 gAreaNpcSpawns_1[8] = {
#include "lobby/007FC1C8.inc"
};
NpcAreaEntry gAreaNpcTables_11[6] = {
#include "lobby/007FC1D0.inc"
};
NpcAreaEntry gAreaNpcTables_12[8] = {
#include "lobby/007FC380.inc"
};
NpcAreaEntry gAreaNpcTables_13[14] = {
#include "lobby/007FC5C0.inc"
};
NpcAreaEntry gAreaNpcTables_2[8] = {
#include "lobby/007FC9B0.inc"
};
NpcAreaEntry gAreaNpcTables_3[9] = {
#include "lobby/007FCBF0.inc"
};
NpcRouteEnt gNpcRoute48[2] = {
#include "lobby/007FCE80.inc"
};
NpcAreaEntry gAreaNpcTables_4[11] = {
#include "lobby/007FCEB0.inc"
};
u8 gAreaNpcSpawns_4[12] = {
#include "lobby/007FD1C8.inc"
};
NpcAreaEntry gAreaNpcTables_5[8] = {
#include "lobby/007FD1E0.inc"
};
NpcAreaEntry gAreaNpcTables_6[25] = {
#include "lobby/007FD420.inc"
};
NpcSidePair gNpcSidePairs[8] = {
#include "lobby/007FDB30.inc"
};
u8 gAreaNpcSpawns_6[16] = {
#include "lobby/007FDB50.inc"
};
NpcSideLine gNpcSideLines[4] = {
#include "lobby/007FDB60.inc"
};
NpcPair gNpcPartnerPairs[8] = {
#include "lobby/007FDBA0.inc"
};
NpcAreaEntry gAreaNpcTables_7 = {
#include "lobby/007FDBB0.inc"
};
NpcAreaEntry gAreaNpcTables_14[2] = {
#include "lobby/007FDC00.inc"
};
NpcAreaEntry gAreaNpcTables_10[19] = {
#include "lobby/007FDC90.inc"
};
u8 *gAreaNpcSpawns[16] = {
    gAreaNpcSpawns_0, gAreaNpcSpawns_1, NULL, NULL,
    gAreaNpcSpawns_4, NULL, gAreaNpcSpawns_6, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
NpcAreaEntry *gAreaNpcTables[16] = {
    gAreaNpcTables_0, gAreaNpcTables_1, gAreaNpcTables_2, gAreaNpcTables_3,
    gAreaNpcTables_4, gAreaNpcTables_5, gAreaNpcTables_6, &gAreaNpcTables_7,
    &gAreaNpcTables_7, &gAreaNpcTables_7, gAreaNpcTables_10, gAreaNpcTables_11,
    gAreaNpcTables_12, gAreaNpcTables_13, gAreaNpcTables_14, NULL,
};
u8 gAreaNpcCounts[16] = {
#include "lobby/007FE270.inc"
};
s32 gAreaFiles[16] = {
#include "lobby/007FE280.inc"
};
AreaModelSlot gTownAreaSets_0[4] = {
#include "lobby/007FE2C0.inc"
};
AreaModelSlot gTownAreaSets_1[4] = {
#include "lobby/007FE2E0.inc"
};
AreaModelSlot gTownAreaSets_2[4] = {
#include "lobby/007FE300.inc"
};
AreaModelSlot gTownAreaSets_3[4] = {
#include "lobby/007FE320.inc"
};
AreaModelSlot gTownAreaSets_4[4] = {
#include "lobby/007FE340.inc"
};
AreaModelSlot gTownAreaSets_5[4] = {
#include "lobby/007FE360.inc"
};
AreaModelSlot gTownAreaSets_6[4] = {
#include "lobby/007FE380.inc"
};
AreaModelSlot gTownAreaSets_10[4] = {
#include "lobby/007FE3A0.inc"
};
AreaModelSlot gTownAreaSets_11[4] = {
#include "lobby/007FE3C0.inc"
};
AreaModelSlot *gTownAreaSets[16] = {
    gTownAreaSets_0, gTownAreaSets_1, gTownAreaSets_2, gTownAreaSets_3,
    gTownAreaSets_4, gTownAreaSets_5, gTownAreaSets_6, gTownAreaSets_6,
    gTownAreaSets_6, gTownAreaSets_6, gTownAreaSets_10, gTownAreaSets_11,
    gTownAreaSets_11, gTownAreaSets_11, gTownAreaSets_11, 0,
};
u32 gNpcHandlers[12] = {
    (u32)gNpcCommonHandlers, (u32)gStationedNpcHandlers, (u32)gServiceNpcHandlers, (u32)gPetHandlers,
    (u32)gNpcNopHandlers, (u32)gNpcNopHandlers, (u32)gNpcNopHandlers, (u32)gNpcNopHandlers,
    (u32)gNpcNopHandlers, (u32)gNpcNopHandlers, 0, 0,
};
void *gNpcNopHandlers[8] = {
    Npc_NopHandlerA, Npc_NopHandlerA, Npc_NopHandlerB, Npc_NopHandlerB,
    Npc_NopHandlerB, NULL, NULL, NULL,
};
void *gNpcCommonHandlers[8] = {
    Npc_NopHandlerA, Npc_NopHandlerA, Npc_NopHandlerB, Npc_RunMotionSounds,
    Npc_NopHandlerB, NULL, NULL, NULL,
};
