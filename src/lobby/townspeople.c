/*
 * The townspeople's drawing, set-up, presence, routes and per-frame task.
 *
 * The lobby's townspeople are Em slots whose work area is a NpcWork
 * (lobby_npc.h). Each area set has a table of townspeople entries
 * (gAreaNpcTables[set], gAreaNpcCounts[set] of them, 72 bytes each: the
 * townsperson's id, its type, start yaw, scale, motion speed, route, model
 * set, presence rule and name). Npc_IsPresent decides from the entry and
 * the village's state (event flags, conditions, facility levels, the time
 * of day, online or not) whether a townsperson is in town.
 * Npc_SetupFromArea copies its entry into the Em, and the route pickers
 * (Npc_SetRouteByKind, Npc_SetRaidRoute, Npc_SetRouteByVillage) replace
 * the entry's route by the time of day, the elder raid or the village's
 * levels. Npc_Init starts its motion for the first route point,
 * Npc_SetupDraw gives it its handler table and its draw slot, and Npc_Draw
 * draws its model with the colours of its type (gNpcColorSets), hiding
 * some materials (Npc_HidePart) and adding held props (Npc_DrawProps).
 * Each frame the town's loop calls Npc_Update for every townsperson slot:
 * by the task phase it sets the townsperson up (Npc_Init), runs it (Npc_Run)
 * or frees its slot, and puts its draw slot into the ordering table.
 * Npc_UpdateMotion advances its motions, Npc_UpdateMatrix builds its model
 * matrix and turns its head toward the local player, and Npc_RunMotionFx
 * calls its handler table's motion sounds and effects (Npc_NopHandlerA and
 * B are the tables' empty entries).
 *
 * The data holds the routes (NpcRouteEnt lists ended by state 0, many
 * reached only from the area tables), the tables of four routes for the
 * morning and the afternoon, the colour sets, the sound each townsperson
 * plays when talked to, and area set 0's table. The townspeople's states
 * and motions are run by the files that follow this one.
 */
#include "common.h"
#include "lobby_npc.h"
#include "stageobj.h"
#include "lobby_vobj.h"
#include "scene.h"
#include "player.h"
#include "item.h"
#include "em.h"
#include "save.h"
#include "pad.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "area.h"
#include "quest.h"
#include "render_state.h"
#include "sprite.h"
#include "bgm.h"
#include "place_rules.h"
#include "card.h"
#include "light.h"
#include "text.h"
#include "village_cond.h"
#include "draw_slots.h"
#include "drink_mug.h"
#include "engine_math.h"
#include "game_util.h"
#include "kitchen.h"
#include "lobby_queries.h"
#include "lobby_world.h"
#include "my_house.h"
#include "offer_menu.h"
#include "town_area_change.h"
#include "vec_math.h"
/* unit's own declaration: an s32 return (the definition returns u8); the callers cut the level to a byte themselves, and with a u8 return Npc_IsPresent and Npc_SetRouteByKind compile differently from the game's code */
s32 Village_FacilityLevel(s32);

/* prototypes */
extern NpcRouteEnt gNpcRaidRoute1D[];
extern NpcRouteEnt gNpcRaidRoute1E[];
extern NpcRouteEnt gNpcRaidRoute1F[];
extern NpcRouteEnt gNpcRaidRoute21[];
extern NpcRouteEnt gNpcRaidRoute23[];
extern NpcRouteEnt gNpcRaidRoute24[];
extern NpcRouteEnt gNpcRaidRoute3A[];
extern NpcRouteEnt gNpcRaidRoute3B[];
extern NpcRouteEnt gNpcRaidRoute3C[];
extern NpcRouteEnt gNpcRaidRoute3D[];
extern NpcRouteEnt gOfficialDeskLowRaidRoute[];
extern NpcRouteEnt gOfficialDeskHighRaidRoute[];
/* end of prototypes */

/* unit's own declaration: an s32 area (the definition takes a u16); with a u16 parameter NpcTable_Find, AreaNpc_IsPresent and Npc_SetupFromArea cut the area to 16 bits before the call, which the game's code doesn't */
u8 Area_GetSetIndex(s32 id);
s32 Npc_IsPresent(u8 kind, s32 id, u8 force);

extern NpcAreaEntry *gAreaNpcTables[];
/* unit's own declaration: a u16 state (the definition takes s32): Npc_Init passes the route entry's state cut to 16 bits, as the game's code does */
void Actor_SetState(void *, s32, u16);
extern u8 gHouseAccess;

EmBone *Em_Bone(void *obj, s16 bone);

/* Draws what a townsperson holds: model parts of the common models
 * (gCommonModels[0]), or of the area's models (gAreaSubModels[0], part 8: the
 * mug) for NPC_DRINK_CONTEST_HOST, each at a bone with an offset and a
 * rotation that depend on the motion and its frame. Townspeople 10 and 46,
 * while free, hold two props (bones 18 and 14) in motions 683-685; the
 * drinking host holds its mug at bone 18 in motions 861-864, and after
 * frame 274 of motion 861 it sets it down in front of it (at bone 1, the
 * root); a townsperson of type 64 holds part 38 at bone 14. Last, the
 * item the townsperson holds (Player_DrawHeldItem). */
void Npc_DrawProps(Em *npc)
{
    s16 bone[2];
    f32 pos[3];
    f32 propMtx[16];
    f32 boneMtx[16];
    f32 ofs[2][3];
    f32 rot[2][3];
    ModelMat *mats;
    s32 drawCount;
    Model *set;
    s16 propCount;
    NpcWork *w;
    s16 texSlot;
    s16 i;
    ModelMatSet *part;
    s32 j;
    f32 *mat;

    set = gCommonModels[0];
    w = (NpcWork *)&npc->work;
    if (set != NULL && set->active != 0) {
        mats = set->mats;
        Gs_SetState(RS_COLOR2, -1);
        propCount = 0;
        if (Npc_IsFreeId0Aor2E((Em *)npc) == 1) {
            switch (npc->motionId[0]) {
            case 685:
                if (EmMotion_ReachedFrame(npc, 0, 80.0f) != 0) {
                    return;
                }
            case 683:
            case 684:
                texSlot = Effect_MapTexId(62);
                if (texSlot == -1) {
                    return;
                }
                part = set->matSets + texSlot;
                bone[0] = 18;
                rot[0][0] = 2.51327419f;
                rot[0][1] = -0.488692224f;
                rot[0][2] = 0.0174532924f;
                ofs[0][0] = -9.39999962f;
                ofs[0][1] = -7.4000001f;
                ofs[0][2] = -3.0f;
                bone[1] = 14;
                rot[1][0] = 1.18682396f;
                rot[1][1] = 0.558505356f;
                rot[1][2] = -2.02458191f;
                ofs[1][0] = 11.0f;
                ofs[1][1] = -6.4000001f;
                ofs[1][2] = -1.60000002f;
                propCount += 2;
                break;
            }
        } else if (Npc_IsFree((Em *)npc) == 1) {
            switch (w->id) {
            case NPC_DRINK_CONTEST_HOST:
                set = gAreaSubModels[0];
                part = set->matSets + 8;
                switch (npc->motionId[0]) {
                case 861:
                    if (EmMotion_InFrameRange(npc, 0, 172.0f, 274.0f) != 0) {
                        bone[0] = 18;
                        rot[0][0] = 1.71042275f;
                        rot[0][1] = 0.972148478f;
                        rot[0][2] = -1.48352981f;
                        ofs[0][0] = -25.0f;
                        ofs[0][1] = -8.30000019f;
                        ofs[0][2] = -22.7000008f;
                        propCount++;
                    } else if (EmMotion_ReachedFrame(npc, 0, 274.0f) != 0) {
                        propCount = 1;
                        rot[0][0] = 0.0f;
                        ofs[0][0] = -0.340000004f;
                        bone[0] = 1;
                        ofs[0][1] = 75.0f;
                        rot[0][1] = 0.0f;
                        ofs[0][2] = 92.0f;
                        rot[0][2] = 0.0f;
                    }
                    break;
                case 862:
                    if (EmMotion_ReachedFrame(npc, 0, 54.0f) != 0) {
                        bone[0] = 18;
                        rot[0][0] = 1.71042275f;
                        rot[0][1] = 0.972148478f;
                        rot[0][2] = -1.48352981f;
                        ofs[0][0] = -25.0f;
                        ofs[0][1] = -8.30000019f;
                        ofs[0][2] = -22.7000008f;
                        propCount++;
                    } else {
                        propCount = 1;
                        rot[0][0] = 0.0f;
                        ofs[0][0] = -0.340000004f;
                        bone[0] = 1;
                        ofs[0][1] = 75.0f;
                        rot[0][1] = 0.0f;
                        ofs[0][2] = 92.0f;
                        rot[0][2] = 0.0f;
                    }
                    break;
                case 863:
                    if (EmMotion_ReachedFrame(npc, 0, 206.0f) == 0) {
                        bone[0] = 18;
                        rot[0][0] = 1.71042275f;
                        rot[0][1] = 0.972148478f;
                        rot[0][2] = -1.48352981f;
                        ofs[0][0] = -25.0f;
                        ofs[0][1] = -8.30000019f;
                        ofs[0][2] = -22.7000008f;
                        propCount++;
                    }
                    break;
                case 864:
                    if (EmMotion_ReachedFrame(npc, 0, 486.0f) == 0) {
                        bone[0] = 18;
                        rot[0][0] = 1.71042275f;
                        rot[0][1] = 0.972148478f;
                        rot[0][2] = -1.48352981f;
                        ofs[0][0] = -25.0f;
                        ofs[0][1] = -8.30000019f;
                        ofs[0][2] = -22.7000008f;
                        propCount++;
                    }
                    break;
                }
                break;
            }
        }
        if (Npc_IsType40(npc) != 0) {
            part = set->matSets + 38;
            bone[propCount] = 14;
            rot[propCount][0] = 0.0f;
            rot[propCount][1] = 0.34906587f;
            rot[propCount][2] = 0.0f;
            ofs[propCount][0] = 2.4000001f;
            ofs[propCount][1] = -3.0f;
            ofs[propCount][2] = -10.0f;
            propCount++;
        }
        drawCount = propCount;
        for (i = 0; i < drawCount; i++) {
            if (part != NULL && part->id != -1) {
                Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_Bone(npc, bone[i])));
                Mtx_GetTrans(pos, boneMtx);
                Vec_RotateSelf(ofs[i], boneMtx);
                pos[0] += ofs[i][0];
                pos[1] += ofs[i][1];
                pos[2] += ofs[i][2];
                Mtx_Identity(propMtx);
                Mtx_RotateXYZ(propMtx, rot[i][0], rot[i][1], rot[i][2]);
                Mtx_Concat3x3(propMtx, boneMtx);
                Mtx_SetTrans(propMtx, pos[0], pos[1], pos[2]);
                Gs_SetState(RS_MTX_SLOT(0), (s32)propMtx);
                Gs_SetState(RS_COLOR2, -1);
                for (j = 0; j < part->count; j++) {
                    mat = (f32 *)&mats[part->mat[j]];
                    mat[4] = npc->alpha; /* the material's alpha */
                    Gs_SetState((u8)RS_MATERIAL(j), (s32)mat);
                }
                Draw_ApplyStates(part->flags);
                Model_Draw(part->id, 0);
            }
        }
        Draw_SetDefaultState();
        Player_DrawHeldItem(npc);
    }
}


/* Hides (alpha 0) material p, the part-th material of one of a
 * townsperson's model parts, where the townsperson's rules say so: the
 * workshop townspeople's materials 13 and 6 by their motion phase, material
 * 2 of townsperson 26 outside AREA_HUB_0x89, material 5 of townsperson 6
 * at port level 2, in state 4 or in motions 658 and 1, material 6 of types
 * 83 and 85 except during parts of motions 1084 and 1085, and, for
 * acts 81-85, materials 1-4 but one of 1-3, picked by NpcWork.partSet. */
void Npc_HidePart(Em *npc, ModelMat *mat, s32 matIdx)
{
    NpcWork *w = (NpcWork *)&npc->work;

    if (Npc_IsWorkshopSmith(npc) == 1 || w->id == NPC_WORKSHOP_HELPER || w->id == NPC_WORKSHOP_HELPER_ONLINE) {
        switch (w->motionPhase) {
        case 0:
            if (matIdx == 13) {
                mat->alpha = 0.0f;
            }
            break;
        case 1:
            if (matIdx == 6) {
                mat->alpha = 0.0f;
            }
            break;
        default:
            if (w->id != NPC_WORKSHOP_HELPER_ONLINE && (matIdx == 13 || matIdx == 6)) {
                mat->alpha = 0.0f;
            }
            break;
        }
    }
    if (w->id == 26 && gScene.area != AREA_HUB_0x89 && matIdx == 2) {
        mat->alpha = 0.0f;
    }
    if (w->id == 6 &&
        ((u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) == 2 || npc->state == 4 || npc->motionId[0] == 658 ||
         npc->motionId[0] == 1) &&
        matIdx == 5) {
        mat->alpha = 0.0f;
    }
    if ((npc->variant == 83 || npc->variant == 85) && matIdx == 6) {
        mat->alpha = 0.0f;
        if (npc->motionId[0] == 1084) {
            if (Motion_ReachedFrame(npc, 0, 120.0f)) {
                mat->alpha = 1.0f;
            }
        } else if (npc->motionId[0] == 1085) {
            if (!Motion_ReachedFrame(npc, 0, 54.0f)) {
                mat->alpha = 1.0f;
            }
        }
    }
    if (Npc_IsAct51to55(npc) == 1) {
        switch (w->partSet) {
        case 0:
            switch (matIdx) {
            case 2:
            case 3:
            case 4:
                mat->alpha = 0.0f;
                break;
            }
            break;
        case 1:
            switch (matIdx) {
            case 1:
            case 3:
            case 4:
                mat->alpha = 0.0f;
                break;
            }
            break;
        case 2:
            switch (matIdx) {
            case 1:
            case 2:
            case 4:
                mat->alpha = 0.0f;
                break;
            }
            break;
        }
    }
}

struct EmBone;

extern NpcColor *gNpcColorSets[];

/* The draw callback of a townsperson (DrawSlot.draw; DrawSlot.owner is the
 * townsperson): its model, each part's materials coloured from its type's
 * colour set (gNpcColorSets[NpcWork.look]: a list of parts, ended by
 * part -1, each colouring all its materials or followed by entries for single
 * materials), with the hidden materials (Npc_HidePart), then its props
 * (Npc_DrawProps). The offline house's Poogie writes its outfit
 * (SaveData.poogieOutfit) into its colour set's first part. */
void Npc_Draw(DrawSlot *slot)
{
    Em *npc;
    f32 mtx[16];
    f32 identMtx[16];
    f32 scaleMtx[16];
    ModelMat *mats;
    ModelMatSet *parts;
    s32 entryCount;
    s32 j;
    ModelMat *mat;
    NpcColor *colorEnt;
    Model *npcModel;
    f32 rad[3];
    NpcWork *w;

    npc = slot->owner; /* the townsperson (Npc_SetupDraw) */
    npcModel = npc->model;
    w = (NpcWork *)&npc->work;
    if (npc->active == 0 || npc->visible == 0) {
        return;
    }
    LightSet_SetColors(npc, 1);
    LightSet_FadeColors(npc);
    Vec_AnglesToRad(&npc->rotX, rad);
    Mtx_Scale(scaleMtx, npc->scaleX, npc->scaleY, npc->scaleZ);
    Mtx_FromAnglesYXZ(&npc->rotX, mtx);
    Mtx_SetTrans(mtx, npc->posX, npc->posY, npc->posZ);
    Mtx_Concat3x3(mtx, scaleMtx);
    Mtx_Copy((f32 *) npc->worldMtx, mtx);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    parts = npcModel->matSets;
    mats = npcModel->mats;
    Bone_StoreMatrices(npcModel->bones);
    Mtx_Identity(identMtx);
    Gs_SetState(RS_UV_MTX, (s32)identMtx);
    Tex_Prepare(10, npc->type * 20 + 163);
    colorEnt = gNpcColorSets[w->look];
    if (w->id == NPC_POOGIE && Area_IsHouse(gScene.area) == 1 && Game_IsOnline() == 0) {
        colorEnt->part = gSave.poogieOutfit;
    }
    while (colorEnt->part != -1) {
        Gs_SetState(RS_COLOR2, -1);
        if (colorEnt->sub == -1) {
            entryCount = 1;
        } else {
            entryCount = 0;
        }
        for (j = 0; j < parts[colorEnt->part].count; j++) {
            mat = &mats[parts[colorEnt->part].mat[j]];
            if (j < 0 || j >= 32) {} /* an empty range check of the material slot (0 to 31) that the game's code has; it emits no instructions */
            if (colorEnt->part == colorEnt[entryCount].part && j == colorEnt[entryCount].sub) {
                Color_ArgbToFloat(mat->rgb, colorEnt[entryCount].color);
                entryCount++;
            } else if (colorEnt->sub == -1) {
                Color_ArgbToFloat(mat->rgb, colorEnt->color);
            } else {
                Color_ArgbToFloat(mat->rgb, 0);
            }
            Npc_HidePart(npc, mat, j);
            Gs_SetState((u8)RS_MATERIAL(j), (s32)mat);
        }
        Draw_ApplyStates(parts[colorEnt->part].flags);
        Model_Draw(parts[colorEnt->part].id, 0);
        if (entryCount == 0) {
            colorEnt++;
        } else {
            colorEnt += entryCount;
        }
    }
    Draw_SetDefaultState();
    Npc_DrawProps(npc);
    Gs_SetState(RS_ALPHA_REF, 0);
}

typedef void (*NpcFunc)(void *);
extern NpcFunc *gNpcHandlers[];

/* Gives a townsperson its handler table and calls the table's init, then
 * takes a draw slot for it, drawn by Npc_Draw. The table is
 * gNpcHandlers[Em.type] (the entry's model set), but table 4 for type 78,
 * table 0 for types 75 and 60-62 and for act 76, and table 1 for
 * type 77. */
void Npc_SetupDraw(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    s8 handlerSet = npc->type;

    if (Npc_IsMusicActor(npc) == 1) {
        handlerSet = 4;
    } else if (Npc_IsType4B(npc) == 1 || w->look == 76 || Npc_IsType3Cto3E(npc) == 1) {
        handlerSet = 0;
    } else if (Npc_IsType77(npc) != 0) {
        handlerSet = 1;
    }
    (npc)->handlers = (EmVt *)gNpcHandlers[handlerSet];
    (npc)->handlers->init(npc);
    npc->drawSlotId = DrawSlot_Alloc();
    if (npc->drawSlotId != -1) {
        npc->drawSlot = DrawSlot_Get(npc->drawSlotId);
        npc->drawSlot->owner = npc;
        npc->drawSlot->draw = Npc_Draw;
    }
}

void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);
void Player_BeginLayerMotion(Player *p, s32 frame, s32 n, s32 part);
void Actor_SetMotionOnLayer(void *, s32, s32, s32, s32);
s32 Player_UpdateMotion(Player *p);
extern s8 gPoogieByPlayer;
extern s8 gPoogiePetStreak;
extern u8 gPetFollows;

/* Sets a townsperson up: its model, its motion layers (two for a free
 * townsperson and type 78, else one), its handlers and draw slot
 * (Npc_SetupDraw), then the motion for the state of its current route point
 * and that state. The house's Poogie takes NPC_STATE_HOUSE_OPEN while the
 * house is open to visitors, and a pet that follows the player
 * PET_STATE_FOLLOW. Last, a bone's position (NpcWork.markAnchor) and, for the
 * pets, their counters; a type 78 townsperson stays hidden.
 * The route point's state is read as
 * *(s32 *)(entryOfs + routeOrMotion + 12), that is route[routeIdx].state:
 * one s32 local holds the route's address and later the motion id, as in
 * the game's code, which the struct access would not give. */
void Npc_Init(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    s32 routeOrMotion;
    s32 entryOfs;

    EmSlot_SetupModel(npc);
    npc->taskPhase++;
    npc->isEm = 0;
    npc->ownTask = 1;
    npc->hitPhase = 0;
    npc->visible = 1;
    npc->motion[0].step = 2.0f;
    npc->motion[1].step = 2.0f;
    npc->motion[2].step = 2.0f;
    npc->motion[3].step = 2.0f;
    npc->motionTime = 0;
    npc->area = gScene.area;
    if (Npc_IsMusicActor(npc) == 1) {
        npc->motionLayerCount = 2;
        npc->isEm = 1;
    } else if (Npc_IsFree((Em *)npc) == 1) {
        npc->isEm = 0;
        npc->motionLayerCount = 2;
    } else {
        npc->isEm = 1;
        npc->motionLayerCount = 1;
    }
    npc->alpha = 1.0f;
    npc->hideShadow = 0;
    npc->fieldCollide = 1;
    npc->motionId[0] = 0;
    npc->motionId[1] = 0;
    npc->motionId[2] = 0;
    npc->motionId[3] = 0;
    Npc_SetupDraw(npc);
    npc->state = 1;
    if (Npc_IsMusicActor(npc) == 1) {
        Actor_SetMotion(npc, 1001, 0, 0);
        Player_BeginLayerMotion((Player *) npc, npc->motionFrame[0], npc->motionBlend[0], 0);
        Player_BeginLayerMotion((Player *) npc, npc->motionFrame[1], npc->motionBlend[1], 1);
        LobbyBgm_SetActor(npc);
    } else if (Npc_IsFree((Em *)npc) == 1) {
        entryOfs = w->routeIdx * 20;
        routeOrMotion = (u32) w->route;
        npc->state = *(s32 *)(entryOfs + routeOrMotion + 12);
        entryOfs = w->routeIdx * 20;
        routeOrMotion = (u32) w->route;
        switch (*(s32 *)(entryOfs + routeOrMotion + 12)) {
        default:
            if (Npc_IsFreeAct15to19((Em *)npc) == 1) {
                routeOrMotion = 676;
                break;
            }
        case 1:
            if (Npc_IsType40(npc) != 0) {
                routeOrMotion = 868;
            } else {
                routeOrMotion = 1;
            }
            break;
        case 67:
            routeOrMotion = 657;
            break;
        case 53:
            routeOrMotion = 670;
            break;
        case 54:
            routeOrMotion = 672;
            break;
        case 56:
            routeOrMotion = 668;
            break;
        case 70:
        case 71:
            routeOrMotion = 661;
            break;
        case 68:
            routeOrMotion = 656;
            break;
        case 58:
            routeOrMotion = 662;
            break;
        case 59:
            routeOrMotion = 664;
            break;
        case 65:
            routeOrMotion = 609;
            break;
        case 72:
            routeOrMotion = 678;
            break;
        case 75:
            routeOrMotion = 638;
            break;
        case 76:
            routeOrMotion = 645;
            break;
        case 77:
            routeOrMotion = 644;
            break;
        }
        Actor_SetMotion(npc, routeOrMotion, 0, 0);
        Player_BeginLayerMotion((Player *) npc, npc->motionFrame[0], npc->motionBlend[0], 0);
        Player_BeginLayerMotion((Player *) npc, npc->motionFrame[1], npc->motionBlend[1], 1);
    } else {
        if (gHouseAccess != HOUSE_ACCESS_CLOSED && w->id == NPC_POOGIE) {
            Actor_SetState(npc, 0, NPC_STATE_HOUSE_OPEN);
        }
        if (((NpcRouteEnt *)w->route) == NULL) {
            routeOrMotion = 1001;
        } else {
            switch (*(s32 *)&((NpcRouteEnt *)w->route)->state) {
            case 78:
                routeOrMotion = 1062;
                break;
            case 152:
                routeOrMotion = 1082;
                break;
            case 82:
            case NPC_STATE_HOUSE_OPEN:
                routeOrMotion = 1074;
                break;
            case 87:
                if (gPetFollows == 1) {
                    Actor_SetState(npc, 0, PET_STATE_FOLLOW);
                    routeOrMotion = 1001;
                } else {
                    routeOrMotion = 1001;
                }
                break;
            default:
                routeOrMotion = 1001;
                break;
            }
        }
        Actor_SetMotionOnLayer(npc, routeOrMotion, 0, 0, 0);
        Player_BeginLayerMotion((Player *) npc, npc->motionFrame[0], npc->motionBlend[0], 0);
    }
    entryOfs = w->routeIdx * 20;
    routeOrMotion = (u32) w->route;
    Actor_SetState(npc, 0, *(s32 *)(entryOfs + routeOrMotion + 12));
    Player_UpdateMotion((Player *) npc);
    Npc_UpdateMotion(npc);
    Npc_UpdateMotion(npc);
    Npc_FollowFloor(npc);
    Npc_UpdateAreaPos(npc);
    if (Npc_IsFree((Em *)npc) != 0) {
        Em_GetBonePos(npc, 20, w->markAnchor);
    } else if (Npc_IsAct51to55(npc) != 0) {
        Em_GetBonePos(npc, 10, w->markAnchor);
    } else if (Npc_HasSecondHead((Em *)npc) != 0) {
        Em_GetBonePos(npc, 12, w->markAnchor);
    } else {
        w->markAnchor[0] = npc->posX;
        w->markAnchor[1] = npc->posY + 200.0f * npc->scaleY;
        w->markAnchor[2] = npc->posZ;
    }
    if (w->id == NPC_POOGIE || w->id == NPC_POOGIE_CARE || w->id == NPC_HOUSE_PET) {
        if (Area_IsOnlineHouse(gScene.area) == 1) {
            gPetFollows = 0;
            if (gPoogieErrand == POOGIE_ERRAND_BACK) {
                w->offerMark = 1;
            }
        }
        w->petAct = 0;
        gPoogiePetStreak = 0;
    }
    if (Npc_IsMusicActor(npc) == 1) {
        npc->visible = 0;
    }
}

/* Returns the entry of townsperson kind (an id) in the table of area id's
 * set (Area_GetSetIndex), or NULL; NULL also for kind 255, no
 * townsperson. */
NpcAreaEntry *NpcTable_Find(u8 npcId, s32 areaId)
{
    u8 i;
    NpcAreaEntry *entry;

    if (npcId == 255) {
        return NULL;
    }
    areaId = Area_GetSetIndex(areaId);
    entry = gAreaNpcTables[areaId];
    for (i = 0; i < gAreaNpcCounts[areaId]; i++) {
        if (entry->id == npcId) {
            return entry;
        }
        entry++;
    }
    return NULL;
}


/* Whether the townsperson of entry idx of the table of area id's set is in
 * town now (Npc_IsPresent, not forced): Town_SpawnNpcs asks it for each
 * entry. */
s32 AreaNpc_IsPresent(s32 entryIdx, s32 areaId)
{
    NpcAreaEntry *entry = gAreaNpcTables[Area_GetSetIndex(areaId)];

    entry += entryIdx;
    return Npc_IsPresent(entry->id, areaId, 0);
}

/* unit's own declaration: an s32 return (the definition returns u8); the callers cut the level to a byte themselves, and with a u8 return Npc_IsPresent and Npc_SetRouteByKind compile differently from the game's code */
s32 Pet_RollPresence(void);
/* unit's own declaration: an s32 return (the definition returns s16): Npc_IsPresent's tests of the result compile as the game's code only with the s32 */
s32 Village_MilestoneState(s32);
extern u8 gPoogieLeft;
extern u8 gCarePetLeft;
extern u8 gHousePetLeft;
extern NetUser gNetUsers[];

/* Whether townsperson kind (an id; its entry NpcTable_Find(kind, id), id
 * being the area) is in town now; 0 when it has no entry there. First its
 * own rule: event flags, conditions, the village facilities' levels (for
 * 22 and 23 the sum of the tavern's, the smithy's and the port's), the
 * elder raid, the pets' absences, and for the Poogie whether it has a name
 * and where the player is (in another's house online: whether the owner's
 * hunter card shows a named Poogie). Then, unless force is 1, the entry's
 * presence rule: 1 mornings only, 2 afternoons only (townsperson 9 also
 * mornings while the port is at level 0), 4 only when it is the one of its
 * pair on the player's side (Npc_PairIdBySide). Returns 1 or 0. */
s32 Npc_IsPresent(u8 npcId, s32 areaId, u8 force)
{
    NpcAreaEntry *entry;
    s32 facilityTotal;
    u8 i;
    s8 slot;
    HunterCard *card;

    if (npcId == 255) {
        return 0;
    }
    facilityTotal = 0;
    /* the levels of VILLAGE_FACILITY_TAVERN to VILLAGE_FACILITY_PORT */
    for (i = 1; i <= 3; i++) {
        facilityTotal = (u8)(facilityTotal + Village_FacilityLevel(i));
    }
    entry = NpcTable_Find(npcId, areaId);
    if (entry == NULL) {
        return 0;
    }
    switch (npcId) {
    case 1:
        if (Save_TestEventFlag(390) == 1 && Save_TestEventFlag(391) == 0) {
            return 1;
        }
        if (Cond_Check(26) == 1 && Village_MilestoneState(5) == 0) {
            return 0;
        }
        break;
    case 27:
        if (Save_TestEventFlag(390) == 1 && Save_TestEventFlag(391) == 0) {
            return 0;
        }
        if (Cond_Check(26) == 0 || Village_MilestoneState(5) != 0) {
            return 0;
        }
        break;
    case 4:
    case 8:
        if (Cond_Check(3) == 0) {
            return 0;
        }
        break;
    case NPC_DRINK_CONTEST_HOST:
        if (Cond_Check(13) == 0) {
            return 0;
        }
        break;
    case 48:
    case 49:
    case 50:
    case 62:
        if (ElderRaid_IsOn() == 1) {
            return 0;
        }
        break;
    case 52:
    case 53:
    case 54:
    case 63:
        if (ElderRaid_IsOn() == 0) {
            return 0;
        }
        break;
    case 6:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) == 0) {
            return 0;
        }
        break;
    case 10:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) < 2) {
            return 0;
        }
        break;
    case 11:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) == 0) {
            return 0;
        }
        break;
    case 12:
    case 26:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_SMITHY) == 0) {
            return 0;
        }
        break;
    case 13:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_SMITHY) < 2) {
            return 0;
        }
        break;
    case NPC_WORKSHOP_HELPER:
        if (Village_MilestoneState(4) == 0) {
            return 0;
        }
        break;
    case NPC_NAMING_TALKER:
        if (Save_TestEventFlag(73) == 0) {
            return 0;
        }
        if (Save_TestEventFlag(74) == 0) {
            return 1;
        }
        if (PspLink_HasFlag(11) != 0) {
            return 1;
        }
        if (Village_MilestoneState(2) == 0) {
            return 0;
        }
        break;
    case 19:
        if (Save_TestEventFlag(EVENT_FLAG_VILLAGE_LEVEL_2) == 0 || Save_TestEventFlag(EVENT_FLAG_TOUR_VILLAGE_LEVEL_2) == 0) {
            return 0;
        }
        break;
    case NPC_POOGIE:
        if (Game_IsOnline() == 1) {
            if (Area_IsOnlineHouse(gScene.area) == 1) {
                if (MyHouse_IsOwner() == 0) {
                    slot = MyHouse_LocalSlot();
                    if (slot == -1) {
                        return 0;
                    }
                    /* the house owner's hunter card (the start of its
                     * NetUser's text): poogieOutfit, POOGIE_OUTFIT_NONE while its Poogie
                     * has no name */
                    card = (HunterCard *)gNetUsers[slot].text;
                    return (card->poogieOutfit != 255) ? 1 : 0;
                }
                if (*(s8 *)&gSave.poogieName[0] != 0) {
                    if (gPoogieErrand == POOGIE_ERRAND_AWAY || gPoogieLeft == 1) {
                        return 0;
                    }
                    return 1;
                }
            }
            return 0;
        }
        if (*(s8 *)&gSave.poogieName[0] != 0) {
            if (Area_IsOnlineHouse(gScene.area) == 0 && gPetFollows == 0) {
                return 0;
            }
            if (gPoogieErrand == POOGIE_ERRAND_AWAY || gPoogieLeft == 1) {
                return 0;
            }
            break;
        }
        if (gScene.area != AREA_HUB_0x89) {
            return 0;
        }
        if (Npc_IsPresent(NPC_NAMING_TALKER, gScene.area, 0) == 0) {
            return 0;
        }
        break;
    case NPC_POOGIE_CARE:
        if (Pet_RollPresence() == 0 || gCarePetLeft == 1) {
            return 0;
        }
        break;
    case 107:
        if (Cond_Check(83) == 0) {
            return 0;
        }
        break;
    case NPC_HOUSE_PET:
        if (gHousePetLeft == 1) {
            return 0;
        }
        break;
    case 21:
        if (Save_TestEventFlag(EVENT_FLAG_VILLAGE_LEVEL_2) == 0 || Save_TestEventFlag(EVENT_FLAG_TOUR_VILLAGE_LEVEL_2) == 0) {
            return 0;
        }
        break;
    case 22:
    case 23:
        if ((u8)facilityTotal < 4) {
            return 0;
        }
        break;
    case 24:
        if ((u8)Village_TotalLevel() <= 0) {
            return 0;
        }
        break;
    case NPC_FISHING_SPOT:
        if (Cond_Check(3) == 0) {
            return 0;
        }
        break;
    case NPC_WORKSHOP_HELPER_ONLINE:
        if (ElderRaid_IsOn() == 0) {
            return 0;
        }
        break;
    case 111:
        if (Game_IsOnline() == 1 || Village_MilestoneState(1) != 1) {
            return 0;
        }
        break;
    case NPC_HAIRDRESSER:
        if (Game_IsOnline() == 1 || Village_MilestoneState(1) != 2) {
            return 0;
        }
        break;
    }
    if (force == 1) {
        return 1;
    }
    switch (entry->presence) {
    case NPC_PRESENCE_AM:
        if (LobbyTime_IsAm() == 0) {
            return 0;
        }
        break;
    case NPC_PRESENCE_PM:
        if (LobbyTime_IsAm() == 1 && (npcId != 9 || (u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) != 0)) {
            return 0;
        }
        break;
    case NPC_PRESENCE_SIDE:
        if (entry->id != Npc_PairIdBySide(entry->id)) {
            return 0;
        }
        break;
    }
    return 1;
}

/* Puts a townsperson on route a in the morning and on route b in the
 * afternoon (LobbyTime_IsAm), at the route's first point and in its state,
 * unless it is on that route already. */
void Npc_SetRouteAmPm(Em *npc, NpcRouteEnt *amRoute, NpcRouteEnt *pmRoute)
{
    NpcWork *w = (NpcWork *)&npc->work;
    NpcRouteEnt *cur = w->route;
    s32 isPm;

    isPm = LobbyTime_IsAm() == 0;
    if (isPm == 1) {
        if (cur != pmRoute) {
            w->route = pmRoute;
            w->routeIdx = 0;
            Actor_SetState(npc, 0, (u16)w->route[w->routeIdx].state);
        }
    } else if (cur != amRoute) {
        w->route = amRoute;
        w->routeIdx = 0;
        Actor_SetState(npc, 0, (u16)w->route[w->routeIdx].state);
    }
}

/* Puts a townsperson on the route of the time of day from its table t of
 * four: [0] the morning's start route (or NULL) and [1] the morning's
 * route, [2] the afternoon's start route (or NULL) and [3] the afternoon's
 * route. Nothing changes while it is on either route of the time of day;
 * else it goes to the first point of the start route, or of the route when
 * there is no start route. A start route ends by jumping (NPC_STATE_ROUTE_JUMP) to
 * its route. */
void Npc_SetRouteByTime(Em *npc, NpcRouteEnt **routes)
{
    NpcWork *w = (NpcWork *)&npc->work;
    NpcRouteEnt *cur = w->route;
    s32 isPm;

    isPm = LobbyTime_IsAm() == 0;
    if (isPm == 1) {
        if (cur != routes[3] && cur != routes[2]) {
            if (routes[2] == NULL) {
                w->route = routes[3];
            } else {
                w->route = routes[2];
            }
            w->routeIdx = 0;
            Actor_SetState(npc, 0, (u16)w->route[w->routeIdx].state);
        }
    } else if (cur != routes[1] && cur != routes[0]) {
        if (routes[0] == NULL) {
            w->route = routes[1];
        } else {
            w->route = routes[0];
        }
        w->routeIdx = 0;
        Actor_SetState(npc, 0, (u16)w->route[w->routeIdx].state);
    }
}

/* unit's own declaration: an s32 return (the definition returns u8); the callers cut the level to a byte themselves, and with a u8 return Npc_IsPresent and Npc_SetRouteByKind compile differently from the game's code */
void Npc_SetRouteByTime(Em *arg0, NpcRouteEnt **t);
extern NpcRouteEnt *gNpc01Routes0[];
extern NpcRouteEnt *gNpc01Routes1[];
extern NpcRouteEnt *gNpc01Routes2[];
extern NpcRouteEnt gNpcRoute03Am[];
extern NpcRouteEnt gNpcRoute03Pm[];
extern NpcRouteEnt gNpcRoute04Am[];
extern NpcRouteEnt gNpcRoute04AmLv[];
extern NpcRouteEnt gNpcRoute04Pm[];
extern NpcRouteEnt gNpcRoute05Am[];
extern NpcRouteEnt gNpcRoute05Pm[];
extern NpcRouteEnt *gNpc06Routes[];
extern NpcRouteEnt gNpc09Routes1_3[];
extern NpcRouteEnt *gNpc09Routes1[];
extern NpcRouteEnt *gNpc09Routes2[];
extern NpcRouteEnt *gNpc10Routes0[];
extern NpcRouteEnt *gNpc10Routes1[];
extern NpcRouteEnt gPoogieFollowRoute[];
extern NpcRouteEnt *gNpc30Routes[];
extern NpcRouteEnt *gNpc31Routes[];
extern NpcRouteEnt *gNpc32Routes[];
extern NpcRouteEnt *gNpc45Routes[];
extern NpcRouteEnt *gNpc46Routes[];
extern NpcRouteEnt *gNpc47Routes[];
extern NpcRouteEnt *gNpc49Routes[];
extern NpcRouteEnt *gNpc4ARoutes[];
extern NpcRouteEnt *gNpc4BRoutes[];
extern NpcRouteEnt *gNpc4CRoutes[];
extern NpcRouteEnt *gNpc5FRoutes[];
extern NpcRouteEnt *gNpc60Routes[];
extern NpcRouteEnt *gNpc61Routes[];
extern NpcRouteEnt *gNpc62Routes[];
extern NpcRouteEnt *gNpc67Routes[];
extern NpcRouteEnt *gNpc69Routes[];
extern NpcRouteEnt *gNpc69RoutesCond53[];

/* Puts a townsperson on its route for the time of day and the village's
 * state (Npc_SetRouteAmPm, Npc_SetRouteByTime), by its id: townsperson 1 by
 * condition 1 and the port's level, 4 by the smithy's level, 9 by the
 * port's, the drinking host by the sum of the tavern's, the smithy's and the
 * port's levels, 105 by condition 83, the others by their own tables. The
 * Poogie, when gPetFollows is set in AREA_HUB_0x89 and it is not digging,
 * takes the one-point route that follows the player (PET_STATE_FOLLOW).
 * Nothing changes offline in the house, except for townsperson 103. */
void Npc_SetRouteByKind(Em *npc)
{
    u8 i;
    NpcWork *w;
    s32 facilityTotal;
    s32 portLevel;
    u8 id;

    w = (NpcWork *)&npc->work;
    if ((Game_IsOnline() != 0) || (w->id == 103) || (Area_IsHouse(gScene.area) != 1)) {
        facilityTotal = 0;
        /* the levels of VILLAGE_FACILITY_TAVERN to VILLAGE_FACILITY_PORT */
        for (i = 1; i <= 3; i++) {
            facilityTotal = (u8)(facilityTotal + Village_FacilityLevel(i));
        }
        id = w->id;
        switch (id) {
        case 1:
            if (Cond_Check(1U) == 0) {
                Npc_SetRouteByTime(npc, gNpc01Routes0);
                break;
            }
            if ((Village_FacilityLevel(VILLAGE_FACILITY_PORT) & 0xFF) == 2) {
                Npc_SetRouteByTime(npc, gNpc01Routes2);
                break;
            }
            Npc_SetRouteByTime(npc, gNpc01Routes1);
            break;
        case 3:
            Npc_SetRouteAmPm(npc, gNpcRoute03Am, gNpcRoute03Pm);
            break;
        case 4:
            if (!(Village_FacilityLevel(VILLAGE_FACILITY_SMITHY) & 0xFF)) {
                Npc_SetRouteAmPm(npc, gNpcRoute04Am, gNpcRoute04Pm);
                break;
            }
            Npc_SetRouteAmPm(npc, gNpcRoute04AmLv, gNpcRoute04Pm);
            break;
        case 5:
            Npc_SetRouteAmPm(npc, gNpcRoute05Am, gNpcRoute05Pm);
            break;
        case 6:
            Npc_SetRouteByTime(npc, gNpc06Routes);
            break;
        case 9:
            portLevel = Village_FacilityLevel(VILLAGE_FACILITY_PORT) & 0xFF;
            switch (portLevel) {
            case 0:
                w->route = gNpc09Routes1_3;
                break;
            case 1:
                Npc_SetRouteByTime(npc, gNpc09Routes1);
                break;
            default:
                Npc_SetRouteByTime(npc, gNpc09Routes2);
            }
            break;
        case NPC_DRINK_CONTEST_HOST:
            if ((u8)facilityTotal < 2) {
                Npc_SetRouteByTime(npc, gNpc10Routes1);
                break;
            }
            Npc_SetRouteByTime(npc, gNpc10Routes0);
            break;
        case NPC_POOGIE:
            if ((npc->state != PET_STATE_DIG) && (gPetFollows == 1) && (gScene.area == AREA_HUB_0x89)) {
                w->route = gPoogieFollowRoute;
                gPoogieByPlayer = 1;
            }
            break;
        case 48:
            Npc_SetRouteByTime(npc, gNpc30Routes);
            break;
        case 49:
            Npc_SetRouteByTime(npc, gNpc31Routes);
            break;
        case 50:
            Npc_SetRouteByTime(npc, gNpc32Routes);
            break;
        case 69:
            Npc_SetRouteByTime(npc, gNpc45Routes);
            break;
        case 70:
            Npc_SetRouteByTime(npc, gNpc46Routes);
            break;
        case 71:
            Npc_SetRouteByTime(npc, gNpc47Routes);
            break;
        case 73:
            Npc_SetRouteByTime(npc, gNpc49Routes);
            break;
        case 74:
            Npc_SetRouteByTime(npc, gNpc4ARoutes);
            break;
        case 75:
            Npc_SetRouteByTime(npc, gNpc4BRoutes);
            break;
        case 76:
            Npc_SetRouteByTime(npc, gNpc4CRoutes);
            break;
        case 95:
            Npc_SetRouteByTime(npc, gNpc5FRoutes);
            break;
        case 96:
            Npc_SetRouteByTime(npc, gNpc60Routes);
            break;
        case 97:
            Npc_SetRouteByTime(npc, gNpc61Routes);
            break;
        case 98:
            Npc_SetRouteByTime(npc, gNpc62Routes);
            break;
        case 103:
            Npc_SetRouteByTime(npc, gNpc67Routes);
            break;
        case 105:
            if (Cond_Check(83U) == 1) {
                Npc_SetRouteByTime(npc, gNpc69RoutesCond53);
                break;
            }
            Npc_SetRouteByTime(npc, gNpc69Routes);
            break;
        }
    }
}

/* During the elder raid (Npc_SetupFromArea calls it then): gives
 * townspeople 29-31, 33, 35, 36, 58-61 and the official
 * desks' clerks their
 * raid route, and most of them a faster motion (1.3 times; 1.1 times for
 * 28, which keeps its route; NPC_OFFICIAL_DESK_HIGH_HR's set to 1.3).
 * Townspeople 37-43 only move faster. 58 takes the route named
 * gNpcRaidRoute3A. */
void Npc_SetRaidRoute(Em *npc)
{
    u8 npcId;
    NpcWork *w;

    npcId = npc->work;
    w = (NpcWork *)&npc->work;
    switch (npcId) {
    case 29:
        w->route = gNpcRaidRoute1D;
        break;
    case 30:
        w->route = gNpcRaidRoute1E;
        break;
    case 31:
        w->route = gNpcRaidRoute1F;
        npc->motionSpeed *= 1.3f;
        break;
    case 33:
        w->route = gNpcRaidRoute21;
        npc->motionSpeed *= 1.3f;
        break;
    case 28:
        npc->motionSpeed *= 1.1f;
        break;
    case 35:
        w->route = gNpcRaidRoute23;
        npc->motionSpeed *= 1.3f;
        break;
    case 36:
        w->route = gNpcRaidRoute24;
        npc->motionSpeed *= 1.3f;
        break;
    case 37:
    case 38:
    case 39:
    case 40:
    case 41:
    case 42:
    case 43:
        npc->motionSpeed *= 1.3f;
        break;
    case 58:
        w->route = gNpcRaidRoute3A;
        npc->motionSpeed *= 1.3f;
        break;
    case 59:
        w->route = gNpcRaidRoute3B;
        npc->motionSpeed *= 1.3f;
        break;
    case 60:
        w->route = gNpcRaidRoute3C;
        npc->motionSpeed *= 1.3f;
        break;
    case 61:
        w->route = gNpcRaidRoute3D;
        npc->motionSpeed *= 1.3f;
        break;
    case NPC_OFFICIAL_DESK_LOW_HR:
        w->route = gOfficialDeskLowRaidRoute;
        break;
    case NPC_OFFICIAL_DESK_HIGH_HR:
        w->route = gOfficialDeskHighRaidRoute;
        npc->motionSpeed = 1.3f;
        break;
    }
}

s32 Village_HasMealVisit(u16 id);
extern NpcRouteEnt gNpcRouteHouse[];
extern NpcRouteEnt gNpcRouteHouse0A[];
extern NpcRouteEnt gNpcRoute07Lv2[];
extern NpcRouteEnt gNpcRoute08Lv2[];
extern NpcRouteEnt gNpcRoute0CLv2[];
extern NpcRouteEnt gSmithRoute[];
extern NpcRouteEnt gNpcRoute13High[];
extern NpcRouteEnt gNpcRoute13Low[];
extern NpcRouteEnt gNpcRoute13Mid[];
extern NpcRouteEnt gNpcRoute14Guest[];
extern NpcRouteEnt gNpcRoute1A[];

/* Replaces a townsperson's route (NpcWork.route) by the village's state.
 * Offline in the house, a resident with a meal visit waiting takes the
 * house's visit route (townsperson 10 its own). Else by id: 7 and 8 at
 * port level 2, 12 at smithy level 2 and the smith (NPC_WORKSHOP_SMITH)
 * once the smithy is built take other spots (12 and the smith with a set
 * yaw); the miner (19) takes its route by the mine's level (three
 * ranges), another set while a dig runs (MINE_DIGGING); the Poogie visiting
 * another's house online takes its guest route; 26 moves (with a set yaw)
 * while NPC_WORKSHOP_HELPER is in town. */
void Npc_SetRouteByVillage(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    if (Game_IsOnline() == 0 && Area_IsHouse(gScene.area) == 1 && Village_HasMealVisit(w->id) == 1) {
        if (w->id != 10) {
            w->route = gNpcRouteHouse;
        } else {
            w->route = gNpcRouteHouse0A;
        }
        return;
    }
    switch (w->id) {
    case 7:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) == 2) {
            w->route = gNpcRoute07Lv2;
        }
        break;
    case 8:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) == 2) {
            w->route = gNpcRoute08Lv2;
        }
        break;
    case 12:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_SMITHY) == 2) {
            w->route = gNpcRoute0CLv2;
            npc->rotY = 0x149F;
        }
        break;
    case NPC_WORKSHOP_SMITH:
        if (0 < (u8)Village_FacilityLevel(VILLAGE_FACILITY_SMITHY)) {
            w->route = gSmithRoute;
            npc->rotY = 0x6E39;
        }
        break;
    case 19:
        /* SaveData.mine.active: a dig runs */
        if (*(u8 *)((u8 *)&gSave + 14323) == MINE_DIGGING) {
            if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_MINE) >= 6) {
                w->route = gMinerRouteHigh;
            } else if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_MINE) <= 2) {
                w->route = gMinerRouteLow;
            } else {
                w->route = gMinerRouteMid;
            }
        } else {
            if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_MINE) >= 6) {
                w->route = gNpcRoute13High;
            } else if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_MINE) <= 2) {
                w->route = gNpcRoute13Low;
            } else {
                w->route = gNpcRoute13Mid;
            }
        }
        break;
    case NPC_POOGIE:
        if (Game_IsOnline() == 1 && MyHouse_InOwnHouse() == 0) {
            w->route = gNpcRoute14Guest;
        }
        break;
    case 26:
        if (Npc_IsPresent(NPC_WORKSHOP_HELPER, gScene.area, 0) == 1) {
            w->route = gNpcRoute1A;
            npc->rotY = 0x6E39;
        }
        break;
    }
}

struct Em;

/* Sets townsperson npc up from its entry (Em.index) in the table of area
 * id's set: yaw, type, model set, name (the Poogie's is
 * SaveData.poogieName), id, route (then Npc_SetRouteByKind,
 * Npc_SetRaidRoute during the elder raid, and Npc_SetRouteByVillage),
 * presence rule, motion speed, scale, and its offer mark from its offer
 * list (2 also for the house's cook with its set course ready). Unless keep
 * is set (always for presence rule 4), it follows its route's first jump
 * (NPC_STATE_ROUTE_JUMP or NPC_STATE_ROUTE_JUMP_RANDOM) and starts at the first point with no wait (arg
 * -1), with the yaw of the last turn point (NPC_STATE_ROUTE_TURN) before it. Then it
 * is placed at its route point, and the drinking townspeople get their mugs
 * (DrinkMug_Spawn). The loops read the route through w->route at each use,
 * as the game's code does. */

void Npc_SetupFromArea(Em *npc, u8 keep, s32 areaId)
{
    NpcAreaEntry *entry;
    NpcWork *w;
    u8 areaSet;
    s32 i;
    u16 yaw;
    SaveData *save;

    w = (NpcWork *)&npc->work;
    areaSet = Area_GetSetIndex(areaId);
    save = &gSave;
    entry = gAreaNpcTables[areaSet];
    if (entry == NULL) {
        return;
    }
    entry += npc->index;
    npc->rotZ = 0;
    npc->rotX = 0;
    npc->rotY = entry->yaw;
    npc->variant = entry->type;
    npc->modelSlot = npc->type = entry->modelSet;
    w->leaving = 0;
    w->name = entry->name;
    if (entry->id == NPC_POOGIE) {
        w->name = (char *)save->poogieName;
    }
    w->id = entry->id;
    w->route = entry->route;
    Npc_SetRouteByKind(npc);
    w->presence = entry->presence;
    w->bodyKind = entry->modelSet;
    w->look = entry->type;
    npc->motionSpeed = entry->motionSpeed;
    npc->scaleX = npc->scaleY = npc->scaleZ = entry->scale;
    w->holdRoute = 0;
    switch (OfferMenu_BuildList((struct Em *) npc, 1)) {
    default:
        w->offerMark = 1;
        break;
    case 0:
        w->offerMark = 0;
        break;
    case 4:
        w->offerMark = 2;
        break;
    }
    if (w->id == NPC_HOUSE_COOK && MyHouse_IsOwner() != 0 && Kitchen_SetCourseReady() == 1) {
        w->offerMark = 2;
    }
    if (ElderRaid_IsOn() == 1) {
        Npc_SetRaidRoute(npc);
    }
    Npc_SetRouteByVillage(npc);
    if (keep == 0 || entry->presence == NPC_PRESENCE_SIDE) {
        for (i = 0; w->route[i].state != 0; i++) {
            if (w->route[i].state == NPC_STATE_ROUTE_JUMP || w->route[i].state == NPC_STATE_ROUTE_JUMP_RANDOM) {
                w->route = (NpcRouteEnt *) w->route[i].arg;
                break;
            }
        }
        yaw = entry->yaw;
        for (i = 0; w->route[i].state != 0; i++) {
            if (w->route[i].state == NPC_STATE_ROUTE_TURN) {
                yaw = w->route[i].arg;
            }
            if (w->route[i].arg == -1) {
                npc->rotY = yaw;
                w->routeIdx = i;
                break;
            }
        }
    }
    *(NpcPos *)&npc->posX = w->route[w->routeIdx].pos;
    if (w->id == NPC_POOGIE && Poogie_PlaceByPlayer() == 1 && Area_IsOnlineHouse(gScene.area) == 1) {
        gPoogieByPlayer = 0;
    }
    if (Npc_HasDrinkMug(npc)) {
        DrinkMug_Spawn(npc);
    }
}

struct TriHdr;

/* Keeps a townsperson on the floor: the floor under it (Em_GetFloorInfoEx)
 * and, unless its stance is 2, its height set to the floor's. Not for
 * townspeople 7, 8, NPC_WORKSHOP_SMITH, 21, 28-30, 32, 44, 46, NPC_OFFICIAL_DESK_LOW_HR, 72 and
 * 77, nor for 10 and 26 outside AREA_HUB_0x89 (they keep the height
 * they were placed at), nor while a free townsperson's type is 0 or
 * 43-48 (Npc_IsFreeAct2Bto30). */
void Npc_FollowFloor(Em *npc)
{
    switch (npc->work) {
    case 7:
    case 8:
    case NPC_WORKSHOP_SMITH:
    case 28:
    case 29:
    case 30:
    case 32:
    case 44:
    case 46:
    case NPC_OFFICIAL_DESK_LOW_HR:
    case 72:
    case 77:
    case 21:
        return;
    case 26:
    case 10:
        if (gScene.area == AREA_HUB_0x89) {
            break;
        }
    default:
        if (Npc_IsFreeAct2Bto30((Em *)npc) != 1) {
            Em_GetFloorInfoEx(npc, &npc->posX, (struct TriHdr *) &npc->floorKind, &npc->floorY, &npc->surfaceY);
            if (npc->stance != 2) {
                npc->posY = npc->floorY;
            }
        }
        break;
    }
}

void Actor_TickTimers(Player *arg0);
void Npc_StepMoveTo(Em *obj);
void Npc_UpdateHitStop(Em *arg0);

/* A townsperson's per-frame task: its timers and hit stop, its move toward
 * a point (Npc_StepMoveTo), its state (Npc_UpdateState), its rotation
 * matrix, its motions at twice its motion speed, and its floor. In state
 * NPC_STATE_GONE (it has left) its task phase moves on. Returns 0, doing nothing, for
 * a type 78 townsperson while the lobby's world stream is active
 * (LobbyBgm_IsActive); else 1. */
s32 Npc_Run(Em *npc)
{
    if (Npc_IsMusicActor(npc) == 1 && LobbyBgm_IsActive()) {
        return 0;
    }
    npc->noPushTimer = 10;
    Actor_TickTimers((Player *) npc);
    Npc_UpdateHitStop(npc);
    npc->prevPosX = npc->posX;
    npc->prevPosY = npc->posY;
    npc->prevPosZ = npc->posZ;
    Npc_StepMoveTo(npc);
    Npc_UpdateState(npc);
    npc->rotX = (u16)npc->rotX;
    npc->rotY = (u16)npc->rotY;
    npc->rotZ = (u16)npc->rotZ;
    Mtx_FromAnglesYXZ(&npc->rotX, &npc->rotMtx[0][0]);
    npc->motion[0].step = 2.0f * npc->motionSpeed;
    npc->motion[1].step = 2.0f * npc->motionSpeed;
    npc->motion[2].step = 2.0f * npc->motionSpeed;
    npc->motion[3].step = 2.0f * npc->motionSpeed;
    Npc_UpdateMotion(npc);
    Npc_FollowFloor(npc);
    if (npc->state == NPC_STATE_GONE) {
        npc->taskPhase++;
    }
    return 1;
}

/* Frees the townsperson's Em slot (Npc_Update's last task phase). */
void Npc_Free(Em *npc)
{
    EmSlot_Free(npc);
}

/* Clears the talked-to mark (NpcWork.unk3) of all EM_COUNT slots, so that every
 * townsperson starts with its first line again: on the next day
 * (LobbyWorld_OnNewSeason), when days pass (LobbyWorld_OnHalfDay) and after a
 * talk's event step. */
void Npc_ClearAllTalked(void)
{
    s32 i;

    for (i = 0; i < EM_COUNT; i++) {
        ((NpcWork *)&gEms[i].work)->talked = 0;
    }
}

/* A townsperson's task for this frame, by its task phase: 0 sets it up
 * (Npc_Init) and returns, 1 runs it (Npc_Run), 2 frees its slot. Each frame it
 * also calls the handler table's motion effects (Npc_RunMotionFx) and updates its
 * area position; while it is shown and in the current area (offline: in any
 * area) its draw slot goes into the 32-entry ordering table at its position.
 * Returns 0 when the caller should build its model matrix (Npc_UpdateMatrix),
 * 1 when it was freed or Npc_Run left it alone (the music actor while the lobby
 * music plays: the music's scripts move it). */
s32 Npc_Update(Em *npc)
{
    switch (npc->taskPhase) {
    case 0:
        Npc_Init(npc);
        Npc_RunMotionFx(npc);
        Npc_UpdateAreaPos(npc);
        return 0;
    case 1:
        if (Npc_Run(npc) == 0) {
            return 1;
        }
        break;
    case 2:
        Npc_Free(npc);
        return 1;
    }
    Npc_RunMotionFx(npc);
    if (npc->visible != 0) {
        if (npc->area == gScene.area || Game_IsOnline() == 0) {
            npc->drawSlot->pos[0] = npc->posX;
            npc->drawSlot->pos[1] = npc->posY;
            npc->drawSlot->pos[2] = npc->posZ;
            OT_AddByDepth(gDrawOt32, npc->drawSlot, 32, 0);
        }
    }
    Npc_UpdateAreaPos(npc);
    return 0;
}

/* Unless debug switch 40 is on (Debug_IsOn answers 0 in the released game):
 * the townsperson's rotation matrix, then, while it is not in hit stop, starts
 * the motions waiting on layers 0 and 1 and advances them. These are the steps
 * of the player's Player_PrepareMotions; the townsperson goes to the player's
 * motion code as a Player, whose motion fields are at the same offsets. */
void Npc_UpdateMotion(Em *npc)
{
    if (Debug_IsOn(40) == 0) {
        Mtx_FromAnglesXYZ(&npc->rotX, (f32 *) npc->rotMtx);
        if (npc->hitStop == 0) {
            if (npc->motionStarted[0] == 0) {
                npc->motionStarted[0]++;
                Player_BeginLayerMotion((Player *) npc, npc->motionFrame[0], npc->motionBlend[0], 0);
            }
            if (npc->motionStarted[1] == 0 && npc->motionLayerCount > 1) {
                npc->motionStarted[1]++;
                Player_BeginLayerMotion((Player *) npc, npc->motionFrame[1], npc->motionBlend[1], 1);
            }
            Player_UpdateMotion((Player *) npc);
        }
    }
}

/* The townsperson's model matrix (scale, angles, position, into Em.worldMtx),
 * then its bones' matrices. Its head bone turns toward the local player, eased
 * by a fifth of the gap a frame (NpcWork.headYaw), at most 10 degrees on the
 * second body (Npc_HasSecondHead: bone 12) or 20 on the others (bone 19).
 * The head stays still for townspeople 1, 33, 85 and 89-91 (85 and
 * 89-91 are the online workshop lessons' townspeople) and NPC_CONTEST_HOST,
 * for the looks 77, 78, 60-62 and 79-85 (the music actor and the
 * pets among them), and during the motions 644 and 646. */
void Npc_UpdateMatrix(Em *npc)
{
    f32 mtx[16];
    f32 scale[16];
    Model *model;
    NpcWork *npcWork;
    u16 still;
    f32 *bone;
    f32 limit;
    f32 yaw;

    npcWork = (NpcWork *)&npc->work;
    model = (npc)->model;
    Mtx_Scale(scale, npc->scaleX, npc->scaleY, npc->scaleZ);
    Mtx_FromAnglesYXZ(&npc->rotX, mtx);
    Mtx_SetTrans(mtx, npc->posX, npc->posY, npc->posZ);
    Mtx_Concat3x3(mtx, scale);
    Mtx_Copy(&npc->worldMtx[0][0], mtx);
    switch (npcWork->id) {
    case 1:
    case NPC_CONTEST_HOST:
    case 33:
    case 85:
    case 89:
    case 90:
    case 91:
        still = 1;
        break;
    default:
        still = 0;
        break;
    }
    if (Npc_IsType77(npc) == 1) {
        still = 1;
    }
    if (Npc_IsType3Cto3E(npc) == 1) {
        still = 1;
    }
    if (Npc_IsMusicActor(npc) == 1) {
        still = 1;
    }
    switch (npc->motionId[0]) {
    case 644:
    case 646:
        still = 1;
        break;
    }
    if (Npc_IsAct51to55(npc) == 1 || Npc_IsPet(npc) == 1) {
        still = 1;
    }
    if (!still) {
        if (Npc_HasSecondHead(npc) != 0) {
            bone = Em_BoneMatrixB(npc, 12);
            limit = 10.0f;
        } else {
            bone = Em_BoneMatrixB(npc, 19);
            limit = 20.0f;
        }
        yaw = 180.0f * Math_Angle16ToRad(Actor_HeadingDiff(npc, gPlayers[gScene.localPlayer].pos)) / 3.1415927f;
        if (!(yaw <= 180.0f)) {
            yaw = -(yaw - 180.0f);
        }
        if (!(yaw <= -90.0f) || yaw < 90.0f) {
            if (yaw < -limit) {
                yaw = -limit;
            } else if (!(yaw <= limit)) {
                yaw = limit;
            }
            yaw = npcWork->headYaw + 0.2f * (yaw - npcWork->headYaw);
            Mtx_RotateY(bone, 3.1415927f * yaw / 180.0f);
            npcWork->headYaw = yaw;
        }
    }
    Bone_UpdateWorldScaledRoot(model->bones, mtx);
}

/* Calls the motion-effects entry (EmVt.motionFx) of the townsperson's handler table
 * (Em.handlers, from gNpcHandlers by body, Npc_SetupDraw): Npc_RunMotionSounds
 * for body 0, ServiceNpc_RunMotionFx and Pet_RunMotionFx for bodies 2 and 3, a no-op for 4-9. */
void Npc_RunMotionFx(Em *npc)
{
    (npc)->handlers->motionFx(npc);
}

/* An empty handler: the init and update entries of the no-op handler table
 * gNpcNopHandlers and of gNpcCommonHandlers. */
void Npc_NopHandlerA(void)
{
}

/* An empty handler: the other entries of gNpcNopHandlers and gNpcCommonHandlers's
 * third and fifth. */
void Npc_NopHandlerB(void)
{
}

/* The townspeople's data: the routes (many reached only from the area
 * tables), the tables of four routes for the time of day
 * (Npc_SetRouteByTime), the colour sets (gNpcColorSets), the sound each
 * townsperson plays when talked to (gNpcTalkSe) and area set 0's table
 * (gAreaNpcTables_0). */
extern NpcJumpRoute10End gNpcRoute6ELoop;
extern NpcRandJumpRoute gNpcRoute6E;
NpcRouteEnt gNpc01Routes1_1[31] = {
#include "lobby/007F20B0.inc"
};
NpcRouteEnt gNpc01Routes2_1[31] = {
#include "lobby/007F2320.inc"
};
NpcRouteEnt gNpc01Routes1_3[7] = {
#include "lobby/007F2590.inc"
};
NpcJumpRoute4End gNpc01Routes1_0 = {
#include "lobby/007F2620.inc"
};
NpcJumpRoute4End gNpc01Routes2_0 = {
#include "lobby/007F26A0.inc"
};
NpcRouteEnt gNpc01Routes0_0[3] = {
#include "lobby/007F2720.inc"
};
NpcRouteEnt *gNpc01Routes0[4] = {
    gNpc01Routes0_0, gNpc01Routes0_0, NULL, gNpc01Routes0_0,
};
NpcRouteEnt *gNpc01Routes1[4] = {
    gNpc01Routes1_0.pt, gNpc01Routes1_1, NULL, gNpc01Routes1_3,
};
NpcRouteEnt *gNpc01Routes2[4] = {
    gNpc01Routes2_0.pt, gNpc01Routes2_1, NULL, gNpc01Routes1_3,
};
NpcRouteEnt gNpcRoute02[2] = {
#include "lobby/007F2790.inc"
};
NpcRouteEnt gNpcRoute03Am[6] = {
#include "lobby/007F27C0.inc"
};
NpcRouteEnt gNpcRoute03Pm[4] = {
#include "lobby/007F2840.inc"
};
NpcRouteEnt gNpcRoute04Am[13] = {
#include "lobby/007F2890.inc"
};
NpcRouteEnt gNpcRoute04AmLv[14] = {
#include "lobby/007F29A0.inc"
};
NpcRouteEnt gNpcRoute04Pm[5] = {
#include "lobby/007F2AC0.inc"
};
NpcRouteEnt gNpcRoute05Am[7] = {
#include "lobby/007F2B30.inc"
};
NpcRouteEnt gNpcRoute05Pm[7] = {
#include "lobby/007F2BC0.inc"
};
NpcRouteEnt gNpc06Routes_1[20] = {
#include "lobby/007F2C50.inc"
};
NpcRouteEnt gNpc06Routes_2[2] = {
#include "lobby/007F2DE0.inc"
};
NpcRouteEnt *gNpc06Routes[4] = {
    NULL, gNpc06Routes_1, gNpc06Routes_2, NULL,
};
NpcRouteEnt gNpcRoute07[2] = {
#include "lobby/007F2E20.inc"
};
NpcRouteEnt gNpcRoute07Lv2[2] = {
#include "lobby/007F2E50.inc"
};
NpcRouteEnt gNpcRoute08[2] = {
#include "lobby/007F2E80.inc"
};
NpcRouteEnt gNpcRoute08Lv2[3] = {
#include "lobby/007F2EB0.inc"
};
NpcRouteEnt gNpc09Routes1_3[18] = {
#include "lobby/007F2EF0.inc"
};
NpcRouteEnt gNpc09Routes2_3[23] = {
#include "lobby/007F3060.inc"
};
NpcRouteEnt gNpc09Routes1_0[6] = {
#include "lobby/007F3230.inc"
};
NpcRouteEnt gNpc09Routes2_0[2] = {
#include "lobby/007F32B0.inc"
};
NpcJumpRoute4End gNpc09Routes1_2 = {
#include "lobby/007F32E0.inc"
};
NpcRouteEnt *gNpc09Routes1[4] = {
    gNpc09Routes1_0, NULL, gNpc09Routes1_2.pt, gNpc09Routes1_3,
};
NpcRouteEnt *gNpc09Routes2[4] = {
    gNpc09Routes2_0, NULL, NULL, gNpc09Routes2_3,
};
NpcRouteEnt gNpcRoute0A[3] = {
#include "lobby/007F3380.inc"
};
NpcRouteEnt gNpcRoute0B[2] = {
#include "lobby/007F33C0.inc"
};
NpcRouteEnt gNpcRoute0C[4] = {
#include "lobby/007F33F0.inc"
};
NpcRouteEnt gNpcRoute0CLv2[3] = {
#include "lobby/007F3440.inc"
};
NpcRouteEnt gNpcRoute0D[3] = {
#include "lobby/007F3480.inc"
};
NpcRouteEnt gSmithRouteBase[3] = {
#include "lobby/007F34C0.inc"
};
NpcRouteEnt gSmithRoute[3] = {
#include "lobby/007F3500.inc"
};
NpcRouteEnt gWorkshopHelperRoute[4] = {
#include "lobby/007F3540.inc"
};
NpcRouteEnt gNpc10Routes1_1[15] = {
#include "lobby/007F3590.inc"
};
NpcRouteEnt gNpc10Routes0_1[15] = {
#include "lobby/007F36C0.inc"
};
NpcRouteEnt gNpc10Routes0_3[11] = {
#include "lobby/007F37F0.inc"
};
NpcJumpRoute14End gNpc10Routes0_0 = {
#include "lobby/007F38D0.inc"
};
NpcJumpRoute14End gNpc10Routes1_0 = {
#include "lobby/007F3A10.inc"
};
NpcRouteEnt *gNpc10Routes0[4] = {
    gNpc10Routes0_0.pt, gNpc10Routes0_1, NULL, gNpc10Routes0_3,
};
NpcRouteEnt *gNpc10Routes1[4] = {
    gNpc10Routes1_0.pt, gNpc10Routes1_1, NULL, gNpc10Routes0_3,
};
NpcRouteEnt gNamingTalkerRoute[11] = {
#include "lobby/007F3B70.inc"
};
NpcRouteEnt gNpcRoute13Mid[2] = {
#include "lobby/007F3C50.inc"
};
NpcRouteEnt gMinerRouteMid[4] = {
#include "lobby/007F3C80.inc"
};
NpcRouteEnt gNpcRoute13Low[2] = {
#include "lobby/007F3CD0.inc"
};
NpcRouteEnt gMinerRouteLow[4] = {
#include "lobby/007F3D00.inc"
};
NpcRouteEnt gNpcRoute13High[2] = {
#include "lobby/007F3D50.inc"
};
NpcRouteEnt gMinerRouteHigh[4] = {
#include "lobby/007F3D80.inc"
};
NpcRouteEnt gPoogieRouteVillage[2] = {
#include "lobby/007F3DD0.inc"
};
NpcRouteEnt gPoogieFollowRoute[2] = {
#include "lobby/007F3E00.inc"
};
NpcRouteEnt gNpcRoute14Guest[2] = {
#include "lobby/007F3E30.inc"
};
NpcRouteEnt gPoogieRouteHouse0[5] = {
#include "lobby/007F3E60.inc"
};
NpcRouteEnt gPoogieRouteHouse1[4] = {
#include "lobby/007F3ED0.inc"
};
NpcRouteEnt gPoogieRouteHouse2[7] = {
#include "lobby/007F3F20.inc"
};
NpcRouteEnt gNpcRoute15[4] = {
#include "lobby/007F3FB0.inc"
};
NpcRouteEnt gNpcRoute16[2] = {
#include "lobby/007F4000.inc"
};
NpcRouteEnt gNpcRoute17[2] = {
#include "lobby/007F4030.inc"
};
NpcRouteEnt gNpcRoute18[24] = {
#include "lobby/007F4060.inc"
};
NpcRouteEnt gFishingSpotRoute[6] = {
#include "lobby/007F4240.inc"
};
NpcRouteEnt gNpcRoute1ABase[4] = {
#include "lobby/007F42C0.inc"
};
NpcRouteEnt gNpcRoute1A[4] = {
#include "lobby/007F4310.inc"
};
NpcRouteEnt gNpcRoute1B[14] = {
#include "lobby/007F4360.inc"
};
NpcRouteEnt gNpcRoute1C[2] = {
#include "lobby/007F4480.inc"
};
NpcRouteEnt gNpcRoute1D[3] = {
#include "lobby/007F44B0.inc"
};
NpcRouteEnt gNpcRaidRoute1D[3] = {
#include "lobby/007F44F0.inc"
};
NpcRouteEnt gNpcRoute1E[2] = {
#include "lobby/007F4530.inc"
};
NpcRouteEnt gNpcRaidRoute1E[2] = {
#include "lobby/007F4560.inc"
};
NpcRouteEnt gNpcRoute1F[2] = {
#include "lobby/007F4590.inc"
};
NpcRouteEnt gNpcRaidRoute1F[3] = {
#include "lobby/007F45C0.inc"
};
NpcRouteEnt gNpcRoute20[2] = {
#include "lobby/007F4600.inc"
};
NpcRouteEnt gNpcRoute21[2] = {
#include "lobby/007F4630.inc"
};
NpcRouteEnt gNpcRaidRoute21[3] = {
#include "lobby/007F4660.inc"
};
NpcRouteEnt gCarePetRoute[4] = {
#include "lobby/007F46A0.inc"
};
NpcRouteEnt gNpcRoute23[2] = {
#include "lobby/007F46F0.inc"
};
NpcRouteEnt gNpcRaidRoute23[3] = {
#include "lobby/007F4720.inc"
};
NpcRouteEnt gNpcRoute24[21] = {
#include "lobby/007F4760.inc"
};
NpcRouteEnt gNpcRaidRoute24[3] = {
#include "lobby/007F4910.inc"
};
NpcRouteEnt gNpcRoute25[11] = {
#include "lobby/007F4950.inc"
};
NpcRouteEnt gNpcRoute26[8] = {
#include "lobby/007F4A30.inc"
};
NpcRouteEnt gNpcRoute27[6] = {
#include "lobby/007F4AD0.inc"
};
NpcRouteEnt gNpcRoute28[2] = {
#include "lobby/007F4B50.inc"
};
NpcRouteEnt gNpcRoute29[7] = {
#include "lobby/007F4B80.inc"
};
NpcRouteEnt gNpcRoute2A[9] = {
#include "lobby/007F4C10.inc"
};
NpcRouteEnt gNpcRoute2B[9] = {
#include "lobby/007F4CD0.inc"
};
NpcRouteEnt gNpcRoute2C[2] = {
#include "lobby/007F4D90.inc"
};
NpcRouteEnt gNpcRoute2D[2] = {
#include "lobby/007F4DC0.inc"
};
NpcRouteEnt gNpcRoute2E[2] = {
#include "lobby/007F4DF0.inc"
};
NpcRouteEnt gNpcRoute2F[2] = {
#include "lobby/007F4E20.inc"
};
NpcRouteEnt gNpc30Routes_1[20] = {
#include "lobby/007F4E50.inc"
};
NpcRouteEnt gNpc30Routes_3[22] = {
#include "lobby/007F4FE0.inc"
};
NpcRouteEnt *gNpc30Routes[4] = {
    NULL, gNpc30Routes_1, NULL, gNpc30Routes_3,
};
NpcRouteEnt gNpc31Routes_1[8] = {
#include "lobby/007F51B0.inc"
};
NpcRouteEnt gNpc31Routes_3[6] = {
#include "lobby/007F5250.inc"
};
NpcRouteEnt *gNpc31Routes[4] = {
    NULL, gNpc31Routes_1, NULL, gNpc31Routes_3,
};
NpcRouteEnt gNpc32Routes_1[7] = {
#include "lobby/007F52E0.inc"
};
NpcRouteEnt gNpc32Routes_3[8] = {
#include "lobby/007F5370.inc"
};
NpcRouteEnt *gNpc32Routes[4] = {
    NULL, gNpc32Routes_1, NULL, gNpc32Routes_3,
};
NpcRouteEnt gFishingSpotOnlineRoute[3] = {
#include "lobby/007F5420.inc"
};
NpcRouteEnt gNpcRoute34[6] = {
#include "lobby/007F5460.inc"
};
NpcRouteEnt gNpcRoute35[7] = {
#include "lobby/007F54E0.inc"
};
NpcRouteEnt gNpcRoute36[12] = {
#include "lobby/007F5570.inc"
};
NpcRouteEnt gNpcRoute37[2] = {
#include "lobby/007F5660.inc"
};
NpcRouteEnt gNpcRoute38[2] = {
#include "lobby/007F5690.inc"
};
NpcRouteEnt gNpcRoute39[2] = {
#include "lobby/007F56C0.inc"
};
NpcRouteEnt gNpcRoute3A[10] = {
#include "lobby/007F56F0.inc"
};
NpcRouteEnt gNpcRaidRoute3A[7] = {
#include "lobby/007F57C0.inc"
};
NpcRouteEnt gNpcRoute3B[2] = {
#include "lobby/007F5850.inc"
};
NpcRouteEnt gNpcRaidRoute3B[3] = {
#include "lobby/007F5880.inc"
};
NpcRouteEnt gNpcRoute3C[2] = {
#include "lobby/007F58C0.inc"
};
NpcRouteEnt gNpcRaidRoute3C[3] = {
#include "lobby/007F58F0.inc"
};
NpcRouteEnt gNpcRoute3D[2] = {
#include "lobby/007F5930.inc"
};
NpcRouteEnt gNpcRaidRoute3D[3] = {
#include "lobby/007F5960.inc"
};
NpcRouteEnt gNpcRoute3E[17] = {
#include "lobby/007F59A0.inc"
};
NpcRouteEnt gNpcRoute3F[17] = {
#include "lobby/007F5B00.inc"
};
NpcRouteEnt gOfficialDeskLowRoute[2] = {
#include "lobby/007F5C60.inc"
};
NpcRouteEnt gOfficialDeskLowRaidRoute[2] = {
#include "lobby/007F5C90.inc"
};
NpcRouteEnt gNpcRoute41[2] = {
#include "lobby/007F5CC0.inc"
};
NpcRouteEnt gNpcRoute42[2] = {
#include "lobby/007F5CF0.inc"
};
NpcRouteEnt gNpc45Routes_1[5] = {
#include "lobby/007F5D20.inc"
};
NpcJumpRoute5End gNpc45Routes_0 = {
#include "lobby/007F5D90.inc"
};
NpcRouteEnt gNpc45Routes_2[5] = {
#include "lobby/007F5E20.inc"
};
NpcRouteEnt *gNpc45Routes[4] = {
    gNpc45Routes_0.pt, gNpc45Routes_1, gNpc45Routes_2, NULL,
};
NpcRouteEnt gNpc46Routes_1[5] = {
#include "lobby/007F5EA0.inc"
};
NpcJumpRoute6End gNpc46Routes_0 = {
#include "lobby/007F5F10.inc"
};
NpcRouteEnt gNpc46Routes_2[7] = {
#include "lobby/007F5FB0.inc"
};
NpcRouteEnt *gNpc46Routes[4] = {
    gNpc46Routes_0.pt, gNpc46Routes_1, gNpc46Routes_2, NULL,
};
NpcRouteEnt gNpc47Routes_1[5] = {
#include "lobby/007F6050.inc"
};
NpcJumpRoute9End gNpc47Routes_0 = {
#include "lobby/007F60C0.inc"
};
NpcRouteEnt gNpc47Routes_2[10] = {
#include "lobby/007F61A0.inc"
};
NpcRouteEnt *gNpc47Routes[4] = {
    gNpc47Routes_0.pt, gNpc47Routes_1, gNpc47Routes_2, NULL,
};
NpcRouteEnt gNpc49Routes_0[9] = {
#include "lobby/007F6280.inc"
};
NpcRouteEnt gNpc49Routes_3[3] = {
#include "lobby/007F6340.inc"
};
NpcRouteEnt gNpc49Routes_2[11] = {
#include "lobby/007F6380.inc"
};
NpcRouteEnt *gNpc49Routes[4] = {
    gNpc49Routes_0, NULL, gNpc49Routes_2, gNpc49Routes_3,
};
NpcRouteEnt gNpc4ARoutes_3[3] = {
#include "lobby/007F6470.inc"
};
NpcJumpRoute9End gNpc4ARoutes_2 = {
#include "lobby/007F64B0.inc"
};
NpcRouteEnt gNpc4ARoutes_0[7] = {
#include "lobby/007F6590.inc"
};
NpcRouteEnt *gNpc4ARoutes[4] = {
    gNpc4ARoutes_0, NULL, gNpc4ARoutes_2.pt, gNpc4ARoutes_3,
};
NpcRouteEnt gNpc4BRoutes_3[3] = {
#include "lobby/007F6630.inc"
};
NpcJumpRoute9End gNpc4BRoutes_2 = {
#include "lobby/007F6670.inc"
};
NpcRouteEnt gNpc4BRoutes_0[9] = {
#include "lobby/007F6750.inc"
};
NpcRouteEnt *gNpc4BRoutes[4] = {
    gNpc4BRoutes_0, NULL, gNpc4BRoutes_2.pt, gNpc4BRoutes_3,
};
NpcRouteEnt gNpc4CRoutes_3[3] = {
#include "lobby/007F6820.inc"
};
NpcJumpRoute6End gNpc4CRoutes_2 = {
#include "lobby/007F6860.inc"
};
NpcRouteEnt gNpc4CRoutes_0[6] = {
#include "lobby/007F6900.inc"
};
NpcRouteEnt *gNpc4CRoutes[4] = {
    gNpc4CRoutes_0, NULL, gNpc4CRoutes_2.pt, gNpc4CRoutes_3,
};
NpcRouteEnt gNpcRoute4D[2] = {
#include "lobby/007F6990.inc"
};
NpcRouteEnt gOfficialDeskHighRoute[2] = {
#include "lobby/007F69C0.inc"
};
NpcRouteEnt gOfficialDeskHighRaidRoute[7] = {
#include "lobby/007F69F0.inc"
};
NpcRouteEnt gQuestDesk2Route[2] = {
#include "lobby/007F6A80.inc"
};
NpcRouteEnt gNpcRoute50[2] = {
#include "lobby/007F6AB0.inc"
};
NpcRouteEnt gNpcRoute51[29] = {
#include "lobby/007F6AE0.inc"
};
NpcRouteEnt gNpcRoute52[2] = {
#include "lobby/007F6D30.inc"
};
NpcRouteEnt gNpcRoute53[2] = {
#include "lobby/007F6D60.inc"
};
NpcRouteEnt gNpcRoute54[2] = {
#include "lobby/007F6D90.inc"
};
NpcRouteEnt gNpcRoute55[2] = {
#include "lobby/007F6DC0.inc"
};
NpcRouteEnt gNpcRoute56[2] = {
#include "lobby/007F6DF0.inc"
};
NpcRouteEnt gNpcRoute57[19] = {
#include "lobby/007F6E20.inc"
};
NpcRouteEnt gLessonRouteBHome[3] = {
#include "lobby/007F6FA0.inc"
};
NpcRouteEnt gLessonRouteB1[5] = {
#include "lobby/007F6FE0.inc"
};
NpcJumpRoute4End gLessonRouteB2 = {
#include "lobby/007F7050.inc"
};
NpcRouteEnt gNpcRoute59[2] = {
#include "lobby/007F70D0.inc"
};
NpcRouteEnt gNpcRoute5A[2] = {
#include "lobby/007F7100.inc"
};
NpcRouteEnt gNpcRoute5B[2] = {
#include "lobby/007F7130.inc"
};
NpcRouteEnt gLessonRouteAHome[3] = {
#include "lobby/007F7160.inc"
};
NpcRouteEnt gLessonRouteA1[4] = {
#include "lobby/007F71A0.inc"
};
NpcJumpRoute3End gLessonRouteA2 = {
#include "lobby/007F71F0.inc"
};
NpcRouteEnt gNpcRoute5D[2] = {
#include "lobby/007F7260.inc"
};
NpcRouteEnt gNpcRoute5E[2] = {
#include "lobby/007F7290.inc"
};
NpcRouteEnt gNpc5FRoutes_1[2] = {
#include "lobby/007F72C0.inc"
};
NpcRouteEnt gNpc5FRoutes_3[12] = {
#include "lobby/007F72F0.inc"
};
NpcJumpRoute6 gNpc5FRoutes_0 = {
#include "lobby/007F73E0.inc"
};
NpcJumpRoute7 gNpc5FRoutes_2 = {
#include "lobby/007F7470.inc"
};
NpcRouteEnt *gNpc5FRoutes[4] = {
    gNpc5FRoutes_0.pt, gNpc5FRoutes_1, gNpc5FRoutes_2.pt, gNpc5FRoutes_3,
};
NpcRouteEnt gNpc60Routes_1[2] = {
#include "lobby/007F7520.inc"
};
NpcRouteEnt gNpc60Routes_3[2] = {
#include "lobby/007F7550.inc"
};
NpcJumpRoute3 gNpc60Routes_0 = {
#include "lobby/007F7580.inc"
};
NpcJumpRoute2 gNpc60Routes_2 = {
#include "lobby/007F75D0.inc"
};
NpcRouteEnt *gNpc60Routes[4] = {
    gNpc60Routes_0.pt, gNpc60Routes_1, gNpc60Routes_2.pt, gNpc60Routes_3,
};
NpcRouteEnt gNpc61Routes_1[2] = {
#include "lobby/007F7620.inc"
};
NpcRouteEnt gNpc61Routes_3[2] = {
#include "lobby/007F7650.inc"
};
NpcJumpRoute4 gNpc61Routes_0 = {
#include "lobby/007F7680.inc"
};
NpcJumpRoute3 gNpc61Routes_2 = {
#include "lobby/007F76F0.inc"
};
NpcRouteEnt *gNpc61Routes[4] = {
    gNpc61Routes_0.pt, gNpc61Routes_1, gNpc61Routes_2.pt, gNpc61Routes_3,
};
NpcRouteEnt gNpc62Routes_1[22] = {
#include "lobby/007F7750.inc"
};
NpcRouteEnt gNpc62Routes_3[3] = {
#include "lobby/007F7910.inc"
};
NpcJumpRoute9 gNpc62Routes_0 = {
#include "lobby/007F7950.inc"
};
NpcJumpRoute12 gNpc62Routes_2 = {
#include "lobby/007F7A20.inc"
};
NpcRouteEnt *gNpc62Routes[4] = {
    gNpc62Routes_0.pt, gNpc62Routes_1, gNpc62Routes_2.pt, gNpc62Routes_3,
};
NpcRouteEnt gNpcRoute64[2] = {
#include "lobby/007F7B40.inc"
};
NpcRouteEnt gNpcRoute65[2] = {
#include "lobby/007F7B70.inc"
};
NpcRouteEnt gNpcRoute66[8] = {
#include "lobby/007F7BA0.inc"
};
NpcRouteEnt gNpc67Routes_1[12] = {
#include "lobby/007F7C40.inc"
};
NpcRouteEnt gNpc67Routes_3[3] = {
#include "lobby/007F7D30.inc"
};
NpcJumpRoute9End gNpc67Routes_2 = {
#include "lobby/007F7D70.inc"
};
NpcJumpRoute19End gNpc67Routes_0 = {
#include "lobby/007F7E50.inc"
};
NpcRouteEnt *gNpc67Routes[4] = {
    gNpc67Routes_0.pt, gNpc67Routes_1, gNpc67Routes_2.pt, gNpc67Routes_3,
};
NpcRouteEnt gNpcRoute68[2] = {
#include "lobby/007F8010.inc"
};
NpcRouteEnt gNpc69Routes_1[3] = {
#include "lobby/007F8040.inc"
};
NpcRouteEnt gNpc69RoutesCond53_1[3] = {
#include "lobby/007F8080.inc"
};
NpcRouteEnt gNpc69Routes_3[3] = {
#include "lobby/007F80C0.inc"
};
NpcJumpRoute9End gNpc69Routes_0 = {
#include "lobby/007F8100.inc"
};
NpcJumpRoute9End gNpc69RoutesCond53_0 = {
#include "lobby/007F81E0.inc"
};
NpcJumpRoute9End gNpc69Routes_2 = {
#include "lobby/007F82C0.inc"
};
NpcRouteEnt *gNpc69Routes[4] = {
    gNpc69Routes_0.pt, gNpc69Routes_1, gNpc69Routes_2.pt, gNpc69Routes_3,
};
NpcRouteEnt *gNpc69RoutesCond53[4] = {
    gNpc69RoutesCond53_0.pt, gNpc69RoutesCond53_1, gNpc69Routes_2.pt, gNpc69Routes_3,
};
NpcRouteEnt gNpcRoute6A[8] = {
#include "lobby/007F83C0.inc"
};
NpcRouteEnt gNpcRoute6B[2] = {
#include "lobby/007F8460.inc"
};
NpcRouteEnt gNpcRoute6C[2] = {
#include "lobby/007F8490.inc"
};
NpcRouteEnt gHousePetRoute[4] = {
#include "lobby/007F84C0.inc"
};
NpcRouteEnt gNpcRoute6F[4] = {
#include "lobby/007F8510.inc"
};
NpcRouteEnt gHairdresserRoute[5] = {
#include "lobby/007F8560.inc"
};
NpcRouteEnt gNpcRouteToHairChair[4] = {
#include "lobby/007F85D0.inc"
};
NpcRouteEnt gNpcRouteFromHairChair[5] = {
#include "lobby/007F8620.inc"
};
NpcRouteEnt gNpcRouteKitchen_34[3] = {
#include "lobby/007F8690.inc"
};
NpcJumpRoute6End gNpcRouteKitchen = {
#include "lobby/007F86D0.inc"
};
NpcRouteEnt gKitchenRoute0_44[3] = {
#include "lobby/007F8770.inc"
};
NpcJumpRoute8End gKitchenRoute0 = {
#include "lobby/007F87B0.inc"
};
NpcJumpRoute11End gKitchenRoute1 = {
#include "lobby/007F8880.inc"
};
NpcJumpRoute7End gKitchenRoute2 = {
#include "lobby/007F8990.inc"
};
NpcRouteEnt gNpcPlaceRoutes0_0[15] = {
#include "lobby/007F8A50.inc"
};
NpcRouteEnt gNpcPlaceRoutes0_1[13] = {
#include "lobby/007F8B80.inc"
};
NpcRouteEnt gNpcPlaceRoutes1_0[14] = {
#include "lobby/007F8C90.inc"
};
NpcRouteEnt gNpcPlaceRoutes1_1[13] = {
#include "lobby/007F8DB0.inc"
};
NpcRouteEnt gNpcPlaceRoutes1_2[13] = {
#include "lobby/007F8EC0.inc"
};
NpcRouteEnt gNpcPlaceRoutes1_3[12] = {
#include "lobby/007F8FD0.inc"
};
NpcRouteEnt gNpcPlaceRoutes1_4[15] = {
#include "lobby/007F90C0.inc"
};
NpcRouteEnt gNpcPlaceRoutes1_5[13] = {
#include "lobby/007F91F0.inc"
};
NpcRouteEnt gNpcPlaceRoutes2_0[13] = {
#include "lobby/007F9300.inc"
};
NpcRouteEnt gNpcPlaceRoutes2_1[13] = {
#include "lobby/007F9410.inc"
};
NpcRouteEnt gNpcPlaceRoutes2_6[17] = {
#include "lobby/007F9520.inc"
};
NpcRouteEnt gNpcPlaceRoutes2_7[19] = {
#include "lobby/007F9680.inc"
};
NpcRouteEnt gNpcPlaceRoutes2_8[19] = {
#include "lobby/007F9800.inc"
};
NpcRouteEnt gNpcPlaceRoutes2_9[19] = {
#include "lobby/007F9980.inc"
};
NpcRouteEnt gNpcPlaceRoutes2_10[21] = {
#include "lobby/007F9B00.inc"
};
NpcRouteEnt gNpcPlaceRoutes2_11[19] = {
#include "lobby/007F9CB0.inc"
};
NpcRouteEnt gNpcPlaceRoutes2_12[19] = {
#include "lobby/007F9E30.inc"
};
NpcRouteEnt gNpcPlaceRoutes2_13[17] = {
#include "lobby/007F9FB0.inc"
};
NpcRouteEnt gNpcPlaceRoutes2_14[17] = {
#include "lobby/007FA110.inc"
};
NpcRouteEnt gNpcPlaceRoutes2_15[17] = {
#include "lobby/007FA270.inc"
};
NpcRouteEnt gNpcPlaceRoutes3_0[11] = {
#include "lobby/007FA3D0.inc"
};
NpcRouteEnt *gNpcPlaceRoutes0[4] = {
    gNpcPlaceRoutes0_0, gNpcPlaceRoutes0_1, NULL, NULL,
};
NpcRouteEnt *gNpcPlaceRoutes1[8] = {
    gNpcPlaceRoutes1_0, gNpcPlaceRoutes1_1, gNpcPlaceRoutes1_2, gNpcPlaceRoutes1_3,
    gNpcPlaceRoutes1_4, gNpcPlaceRoutes1_5, NULL, NULL,
};
NpcRouteEnt *gNpcPlaceRoutes2[16] = {
    gNpcPlaceRoutes2_0, gNpcPlaceRoutes2_1, gNpcPlaceRoutes1_2, gNpcPlaceRoutes1_3,
    gNpcPlaceRoutes1_4, gNpcPlaceRoutes1_5, gNpcPlaceRoutes2_6, gNpcPlaceRoutes2_7,
    gNpcPlaceRoutes2_8, gNpcPlaceRoutes2_9, gNpcPlaceRoutes2_10, gNpcPlaceRoutes2_11,
    gNpcPlaceRoutes2_12, gNpcPlaceRoutes2_13, gNpcPlaceRoutes2_14, gNpcPlaceRoutes2_15,
};
NpcRouteEnt *gNpcPlaceRoutes3[4] = {
    gNpcPlaceRoutes3_0, NULL, NULL, NULL,
};
NpcJumpRoute7End gNpcRoute6EOften = {
#include "lobby/007FA530.inc"
};
NpcJumpRoute6End gNpcRoute6ERare = {
#include "lobby/007FA5F0.inc"
};
NpcJumpRoute10End gNpcRoute6ELoop = {
#include "lobby/007FA690.inc"
};
NpcRandJumpRoute gNpcRoute6E = {
#include "lobby/007FA780.inc"
};
NpcRouteEnt gNpcRouteHouse[3] = {
#include "lobby/007FA7D0.inc"
};
NpcRouteEnt gNpcRouteHouse0A[3] = {
#include "lobby/007FA810.inc"
};
NpcColor gNpcColorSets_0[6] = {
#include "lobby/007FA850.inc"
};
NpcColor gNpcColorSets_1[6] = {
#include "lobby/007FA880.inc"
};
NpcColor gNpcColorSets_2[6] = {
#include "lobby/007FA8B0.inc"
};
NpcColor gNpcColorSets_3[6] = {
#include "lobby/007FA8E0.inc"
};
NpcColor gNpcColorSets_4[6] = {
#include "lobby/007FA910.inc"
};
NpcColor gNpcColorSets_5[6] = {
#include "lobby/007FA940.inc"
};
NpcColor gNpcColorSets_6[6] = {
#include "lobby/007FA970.inc"
};
NpcColor gNpcColorSets_7[6] = {
#include "lobby/007FA9A0.inc"
};
NpcColor gNpcColorSets_8[6] = {
#include "lobby/007FA9D0.inc"
};
NpcColor gNpcColorSets_9[6] = {
#include "lobby/007FAA00.inc"
};
NpcColor gNpcColorSets_10[4] = {
#include "lobby/007FAA30.inc"
};
NpcColor gNpcColorSets_11[6] = {
#include "lobby/007FAA50.inc"
};
NpcColor gNpcColorSets_65[8] = {
#include "lobby/007FAA80.inc"
};
NpcColor gNpcColorSets_66[6] = {
#include "lobby/007FAAC0.inc"
};
NpcColor gNpcColorSets_67[6] = {
#include "lobby/007FAAF0.inc"
};
NpcColor gNpcColorSets_68[8] = {
#include "lobby/007FAB20.inc"
};
NpcColor gNpcColorSets_69[8] = {
#include "lobby/007FAB60.inc"
};
NpcColor gNpcColorSets_70[6] = {
#include "lobby/007FABA0.inc"
};
NpcColor gNpcColorSets_71[10] = {
#include "lobby/007FABD0.inc"
};
NpcColor gNpcColorSets_72[12] = {
#include "lobby/007FAC20.inc"
};
NpcColor gNpcColorSets_73[8] = {
#include "lobby/007FAC80.inc"
};
NpcColor gNpcColorSets_74[8] = {
#include "lobby/007FACC0.inc"
};
NpcColor gNpcColorSets_79[2] = {
#include "lobby/007FAD00.inc"
};
NpcColor gNpcColorSets_80[2] = {
#include "lobby/007FAD10.inc"
};
NpcColor gNpcColorSets_81[8] = {
#include "lobby/007FAD20.inc"
};
NpcColor gNpcColorSets_82[8] = {
#include "lobby/007FAD60.inc"
};
NpcColor gNpcColorSets_83[10] = {
#include "lobby/007FADA0.inc"
};
NpcColor gNpcColorSets_84[8] = {
#include "lobby/007FADF0.inc"
};
NpcColor gNpcColorSets_85[8] = {
#include "lobby/007FAE30.inc"
};
NpcColor gNpcColorSets_12[4] = {
#include "lobby/007FAE70.inc"
};
NpcColor gNpcColorSets_13[6] = {
#include "lobby/007FAE90.inc"
};
NpcColor gNpcColorSets_14[4] = {
#include "lobby/007FAEC0.inc"
};
NpcColor gNpcColorSets_15[6] = {
#include "lobby/007FAEE0.inc"
};
NpcColor gNpcColorSets_16[6] = {
#include "lobby/007FAF10.inc"
};
NpcColor gNpcColorSets_17[6] = {
#include "lobby/007FAF40.inc"
};
NpcColor gNpcColorSets_18[6] = {
#include "lobby/007FAF70.inc"
};
NpcColor gNpcColorSets_19[6] = {
#include "lobby/007FAFA0.inc"
};
NpcColor gNpcColorSets_20[6] = {
#include "lobby/007FAFD0.inc"
};
NpcColor gNpcColorSets_21[4] = {
#include "lobby/007FB000.inc"
};
NpcColor gNpcColorSets_22[6] = {
#include "lobby/007FB020.inc"
};
NpcColor gNpcColorSets_23[6] = {
#include "lobby/007FB050.inc"
};
NpcColor gNpcColorSets_24[6] = {
#include "lobby/007FB080.inc"
};
NpcColor gNpcColorSets_25[4] = {
#include "lobby/007FB0B0.inc"
};
NpcColor gNpcColorSets_26[6] = {
#include "lobby/007FB0D0.inc"
};
NpcColor gNpcColorSets_27[6] = {
#include "lobby/007FB100.inc"
};
NpcColor gNpcColorSets_28[6] = {
#include "lobby/007FB130.inc"
};
NpcColor gNpcColorSets_29[6] = {
#include "lobby/007FB160.inc"
};
NpcColor gNpcColorSets_30[6] = {
#include "lobby/007FB190.inc"
};
NpcColor gNpcColorSets_31[6] = {
#include "lobby/007FB1C0.inc"
};
NpcColor gNpcColorSets_32[6] = {
#include "lobby/007FB1F0.inc"
};
NpcColor gNpcColorSets_33[6] = {
#include "lobby/007FB220.inc"
};
NpcColor gNpcColorSets_34[4] = {
#include "lobby/007FB250.inc"
};
NpcColor gNpcColorSets_35[4] = {
#include "lobby/007FB270.inc"
};
NpcColor gNpcColorSets_36[4] = {
#include "lobby/007FB290.inc"
};
NpcColor gNpcColorSets_37[4] = {
#include "lobby/007FB2B0.inc"
};
NpcColor gNpcColorSets_38[4] = {
#include "lobby/007FB2D0.inc"
};
NpcColor gNpcColorSets_39[4] = {
#include "lobby/007FB2F0.inc"
};
NpcColor gNpcColorSets_40[6] = {
#include "lobby/007FB310.inc"
};
NpcColor gNpcColorSets_41[2] = {
#include "lobby/007FB340.inc"
};
NpcColor gNpcColorSets_42[4] = {
#include "lobby/007FB350.inc"
};
NpcColor gNpcColorSets_43[4] = {
#include "lobby/007FB370.inc"
};
NpcColor gNpcColorSets_44[4] = {
#include "lobby/007FB390.inc"
};
NpcColor gNpcColorSets_45[4] = {
#include "lobby/007FB3B0.inc"
};
NpcColor gNpcColorSets_46[4] = {
#include "lobby/007FB3D0.inc"
};
NpcColor gNpcColorSets_47[4] = {
#include "lobby/007FB3F0.inc"
};
NpcColor gNpcColorSets_48[4] = {
#include "lobby/007FB410.inc"
};
NpcColor gNpcColorSets_49[4] = {
#include "lobby/007FB430.inc"
};
NpcColor gNpcColorSets_50[4] = {
#include "lobby/007FB450.inc"
};
NpcColor gNpcColorSets_51[4] = {
#include "lobby/007FB470.inc"
};
NpcColor gNpcColorSets_52[4] = {
#include "lobby/007FB490.inc"
};
NpcColor gNpcColorSets_53[4] = {
#include "lobby/007FB4B0.inc"
};
NpcColor gNpcColorSets_54[4] = {
#include "lobby/007FB4D0.inc"
};
NpcColor gNpcColorSets_55[4] = {
#include "lobby/007FB4F0.inc"
};
NpcColor gNpcColorSets_56[8] = {
#include "lobby/007FB510.inc"
};
NpcColor gNpcColorSets_57[6] = {
#include "lobby/007FB550.inc"
};
NpcColor gNpcColorSets_58[6] = {
#include "lobby/007FB580.inc"
};
NpcColor gNpcColorSets_59[6] = {
#include "lobby/007FB5B0.inc"
};
NpcColor gNpcColorSets_60[4] = {
#include "lobby/007FB5E0.inc"
};
NpcColor gNpcColorSets_61[4] = {
#include "lobby/007FB600.inc"
};
NpcColor gNpcColorSets_62[4] = {
#include "lobby/007FB620.inc"
};
NpcColor gNpcColorSets_63[2] = {
#include "lobby/007FB640.inc"
};
NpcColor gNpcColorSets_64[6] = {
#include "lobby/007FB650.inc"
};
NpcColor gNpcColorSets_75[4] = {
#include "lobby/007FB680.inc"
};
NpcColor gNpcColorSets_76[4] = {
#include "lobby/007FB6A0.inc"
};
NpcColor gNpcColorSets_77[2] = {
#include "lobby/007FB6C0.inc"
};
NpcColor gNpcColorSets_78[4] = {
#include "lobby/007FB6D0.inc"
};
NpcColor *gNpcColorSets[88] = {
    gNpcColorSets_0, gNpcColorSets_1, gNpcColorSets_2, gNpcColorSets_3,
    gNpcColorSets_4, gNpcColorSets_5, gNpcColorSets_6, gNpcColorSets_7,
    gNpcColorSets_8, gNpcColorSets_9, gNpcColorSets_10, gNpcColorSets_11,
    gNpcColorSets_12, gNpcColorSets_13, gNpcColorSets_14, gNpcColorSets_15,
    gNpcColorSets_16, gNpcColorSets_17, gNpcColorSets_18, gNpcColorSets_19,
    gNpcColorSets_20, gNpcColorSets_21, gNpcColorSets_22, gNpcColorSets_23,
    gNpcColorSets_24, gNpcColorSets_25, gNpcColorSets_26, gNpcColorSets_27,
    gNpcColorSets_28, gNpcColorSets_29, gNpcColorSets_30, gNpcColorSets_31,
    gNpcColorSets_32, gNpcColorSets_33, gNpcColorSets_34, gNpcColorSets_35,
    gNpcColorSets_36, gNpcColorSets_37, gNpcColorSets_38, gNpcColorSets_39,
    gNpcColorSets_40, gNpcColorSets_41, gNpcColorSets_42, gNpcColorSets_43,
    gNpcColorSets_44, gNpcColorSets_45, gNpcColorSets_46, gNpcColorSets_47,
    gNpcColorSets_48, gNpcColorSets_49, gNpcColorSets_50, gNpcColorSets_51,
    gNpcColorSets_52, gNpcColorSets_53, gNpcColorSets_54, gNpcColorSets_55,
    gNpcColorSets_56, gNpcColorSets_57, gNpcColorSets_58, gNpcColorSets_59,
    gNpcColorSets_60, gNpcColorSets_61, gNpcColorSets_62, gNpcColorSets_63,
    gNpcColorSets_64, gNpcColorSets_65, gNpcColorSets_66, gNpcColorSets_67,
    gNpcColorSets_68, gNpcColorSets_69, gNpcColorSets_70, gNpcColorSets_71,
    gNpcColorSets_72, gNpcColorSets_73, gNpcColorSets_74, gNpcColorSets_75,
    gNpcColorSets_76, gNpcColorSets_77, gNpcColorSets_78, gNpcColorSets_79,
    gNpcColorSets_80, gNpcColorSets_81, gNpcColorSets_82, gNpcColorSets_83,
    gNpcColorSets_84, gNpcColorSets_85, NULL, NULL,
};
s32 gNpcTalkSe[116] = {
#include "lobby/007FB850.inc"
};
NpcAreaEntry gAreaNpcTables_0[16] = {
#include "lobby/007FBA20.inc"
};
