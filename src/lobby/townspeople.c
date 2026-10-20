/*
 * The townspeople's drawing, set-up, presence, routes and per-frame task.
 *
 * The lobby's townspeople are Em slots whose work area is a NpcWork
 * (lobby_npc.h). Each area set has a table of townspeople entries
 * (gAreaNpcTables[set], gAreaNpcCounts[set] of them, 0x48 bytes each: the
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
#include "m2c_macros.h"
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
s32 Game_IsOnline(void);
/* unit's own declaration: an s32 return (the definition returns u8); the callers cut the level to a byte themselves, and with a u8 return Npc_IsPresent and Npc_SetRouteByKind compile differently from the game's code */
s32 Village_FacilityLevel(s32);
s32 Npc_IsMusicActor(Em *);

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
extern u8 gAreaNpcCounts[];
s32 Area_IsOnlineHouse(s32 arg0);
s32 LobbyTime_IsAm(void);
/* unit's own declaration: a u16 state (the definition takes s32): Npc_Init passes the route entry's state cut to 16 bits, as the game's code does */
void Actor_SetState(void *, s32, u16);
extern u8 gHouseAccess;

s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 EmMotion_InFrameRange(void *em, s32 a, f32 frame, f32 frame2);
s16 Effect_MapTexId(s16 id);
s32 Npc_IsFreeId0Aor2E(Em *arg0);
s32 Npc_IsFree(Em *arg0);
s32 Npc_IsType40(Em *arg0);
EmBone *Em_Bone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_Identity(f32 *m);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
s32 Gs_SetState(u8, u32);
void Draw_ApplyStates(u32);
s32 Model_Draw(u32, s32);
void Draw_SetDefaultState(void);
void Player_DrawHeldItem(void *obj);

/* Draws what a townsperson holds: model parts of the common models
 * (gCommonModels[0]), or of the area's models (gAreaSubModels[0], part 8: the
 * mug) for NPC_DRINK_CONTEST_HOST, each at a bone with an offset and a
 * rotation that depend on the motion and its frame. Townspeople 0xA and 0x2E,
 * while free, hold two props (bones 0x12 and 0xE) in motions 0x2AB-0x2AD; the
 * drinking host holds its mug at bone 0x12 in motions 0x35D-0x360, and after
 * frame 274 of motion 0x35D it sets it down in front of it (at bone 1, the
 * root); a townsperson of type 0x40 holds part 38 at bone 0xE. Last, the
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
            case 0x2AD:
                if (EmMotion_ReachedFrame(npc, 0, 80.0f) != 0) {
                    return;
                }
            case 0x2AB:
            case 0x2AC:
                texSlot = Effect_MapTexId(0x3E);
                if (texSlot == -1) {
                    return;
                }
                part = set->matSets + texSlot;
                bone[0] = 0x12;
                rot[0][0] = 2.51327419f;
                rot[0][1] = -0.488692224f;
                rot[0][2] = 0.0174532924f;
                ofs[0][0] = -9.39999962f;
                ofs[0][1] = -7.4000001f;
                ofs[0][2] = -3.0f;
                bone[1] = 0xE;
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
                case 0x35D:
                    if (EmMotion_InFrameRange(npc, 0, 172.0f, 274.0f) != 0) {
                        bone[0] = 0x12;
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
                case 0x35E:
                    if (EmMotion_ReachedFrame(npc, 0, 54.0f) != 0) {
                        bone[0] = 0x12;
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
                case 0x35F:
                    if (EmMotion_ReachedFrame(npc, 0, 206.0f) == 0) {
                        bone[0] = 0x12;
                        rot[0][0] = 1.71042275f;
                        rot[0][1] = 0.972148478f;
                        rot[0][2] = -1.48352981f;
                        ofs[0][0] = -25.0f;
                        ofs[0][1] = -8.30000019f;
                        ofs[0][2] = -22.7000008f;
                        propCount++;
                    }
                    break;
                case 0x360:
                    if (EmMotion_ReachedFrame(npc, 0, 486.0f) == 0) {
                        bone[0] = 0x12;
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
            bone[propCount] = 0xE;
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

s32 Npc_IsWorkshopSmith(Em *arg0);
s32 Npc_IsAct51to55(Em *arg0);
s32 Motion_ReachedFrame(void *, s32, f32);


/* Hides (alpha 0) material p, the part-th material of one of a
 * townsperson's model parts, where the townsperson's rules say so: the
 * workshop townspeople's materials 0xD and 6 by their motion phase, material
 * 2 of townsperson 0x1A outside AREA_HUB_0x89, material 5 of townsperson 6
 * at port level 2, in state 4 or in motions 0x292 and 1, material 6 of types
 * 0x53 and 0x55 except during parts of motions 0x43C and 0x43D, and, for
 * acts 0x51-0x55, materials 1-4 but one of 1-3, picked by NpcWork.partSet. */
void Npc_HidePart(Em *npc, ModelMat *mat, s32 matIdx)
{
    NpcWork *w = (NpcWork *)&npc->work;

    if (Npc_IsWorkshopSmith(npc) == 1 || w->id == NPC_WORKSHOP_HELPER || w->id == NPC_WORKSHOP_HELPER_ONLINE) {
        switch (w->motionPhase) {
        case 0:
            if (matIdx == 0xD) {
                mat->alpha = 0.0f;
            }
            break;
        case 1:
            if (matIdx == 6) {
                mat->alpha = 0.0f;
            }
            break;
        default:
            if (w->id != NPC_WORKSHOP_HELPER_ONLINE && (matIdx == 0xD || matIdx == 6)) {
                mat->alpha = 0.0f;
            }
            break;
        }
    }
    if (w->id == 0x1A && gScene.area != AREA_HUB_0x89 && matIdx == 2) {
        mat->alpha = 0.0f;
    }
    if (w->id == 6 &&
        ((u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) == 2 || npc->state == 4 || npc->motionId[0] == 0x292 ||
         npc->motionId[0] == 1) &&
        matIdx == 5) {
        mat->alpha = 0.0f;
    }
    if ((npc->variant == 0x53 || npc->variant == 0x55) && matIdx == 6) {
        mat->alpha = 0.0f;
        if (npc->motionId[0] == 0x43C) {
            if (Motion_ReachedFrame(npc, 0, 120.0f)) {
                mat->alpha = 1.0f;
            }
        } else if (npc->motionId[0] == 0x43D) {
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

void LightSet_SetColors(void *obj, s32 n);
void LightSet_FadeColors(void *obj);
void Vec_AnglesToRad(s32 *in, f32 *out);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_Copy(f32 *dst, f32 *src);
void Gfx_SetTexFilter(s32);
s32 Gs_SetState(u8, u32);
struct EmBone;
void Bone_StoreMatrices(struct EmBone *b);
void Mtx_Identity(f32 *m);
void Tex_Prepare(s32 n, s32 tex);
s32 Area_IsHouse(u16 area);
void Color_ArgbToFloat(f32 *rgba, u32 c);
void Draw_ApplyStates(u32);
s32 Model_Draw(u32, s32);
void Draw_SetDefaultState(void);
void Npc_DrawProps(Em *npc);

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
    Gfx_SetTexFilter(1);
    Gs_SetState(RS_ALPHA_REF, 0);
    parts = npcModel->matSets;
    mats = npcModel->mats;
    Bone_StoreMatrices(npcModel->bones);
    Mtx_Identity(identMtx);
    Gs_SetState(RS_UV_MTX, (s32)identMtx);
    Tex_Prepare(0xA, npc->type * 20 + 0xA3);
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
            if (j < 0 || j >= 0x20) {} /* an empty range check of the material slot (0 to 31) that the game's code has; it emits no instructions */
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
s32 Npc_IsType4B(Em *arg0);
s32 Npc_IsType3Cto3E(Em *arg0);
s32 Npc_IsType4D(Em *arg0);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void Npc_Draw(DrawSlot *ds);

/* Gives a townsperson its handler table and calls the table's init, then
 * takes a draw slot for it, drawn by Npc_Draw. The table is
 * gNpcHandlers[Em.type] (the entry's model set), but table 4 for type 0x4E,
 * table 0 for types 0x4B and 0x3C-0x3E and for act 0x4C, and table 1 for
 * type 0x4D. */
void Npc_SetupDraw(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    s8 handlerSet = npc->type;

    if (Npc_IsMusicActor(npc) == 1) {
        handlerSet = 4;
    } else if (Npc_IsType4B(npc) == 1 || w->look == 0x4C || Npc_IsType3Cto3E(npc) == 1) {
        handlerSet = 0;
    } else if (Npc_IsType4D(npc) != 0) {
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

void EmSlot_SetupModel(Em *e);
s32 Npc_IsMusicActor(Em *);
s32 Npc_IsFree(Em *arg0);
s32 Npc_IsFreeAct15to19(Em *arg0);
s32 Npc_IsType40(Em *);
s32 Npc_HasSecondHead(Em *arg0);
void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);
void Player_BeginLayerMotion(Player *p, s32 frame, s32 n, s32 part);
void LobbyBgm_SetActor(Em *actor);
void Actor_SetMotionOnLayer(void *, s32, s32, s32, s32);
s32 Player_UpdateMotion(Player *p);
void Npc_UpdateMotion(Em *);
void Npc_FollowFloor(Em *);
void Npc_UpdateAreaPos(Em *);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
extern s8 gPoogieByPlayer;
extern s8 gPoogiePetStreak;
extern u8 gPetFollows;

/* Sets a townsperson up: its model, its motion layers (two for a free
 * townsperson and type 0x4E, else one), its handlers and draw slot
 * (Npc_SetupDraw), then the motion for the state of its current route point
 * and that state. The house's Poogie takes NPC_STATE_HOUSE_OPEN while the
 * house is open to visitors, and a pet that follows the player
 * PET_STATE_FOLLOW. Last, a bone's position (NpcWork.markAnchor) and, for the
 * pets, their counters; a type 0x4E townsperson stays hidden.
 * The route point's state is read as *(s32 *)(offset + base + 0xC), that is
 * route[routeIdx].state: one s32 local holds the route's address and later
 * the motion id, as in the game's code, which the struct access would not
 * give. */
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
        Actor_SetMotion(npc, 0x3E9, 0, 0);
        Player_BeginLayerMotion((Player *) npc, npc->motionFrame[0], npc->motionBlend[0], 0);
        Player_BeginLayerMotion((Player *) npc, npc->motionFrame[1], npc->motionBlend[1], 1);
        LobbyBgm_SetActor(npc);
    } else if (Npc_IsFree((Em *)npc) == 1) {
        entryOfs = w->routeIdx * 0x14;
        routeOrMotion = (u32) w->route;
        npc->state = *(s32 *)(entryOfs + routeOrMotion + 0xC);
        entryOfs = w->routeIdx * 0x14;
        routeOrMotion = (u32) w->route;
        switch (*(s32 *)(entryOfs + routeOrMotion + 0xC)) {
        default:
            if (Npc_IsFreeAct15to19((Em *)npc) == 1) {
                routeOrMotion = 0x2A4;
                break;
            }
        case 0x1:
            if (Npc_IsType40(npc) != 0) {
                routeOrMotion = 0x364;
            } else {
                routeOrMotion = 1;
            }
            break;
        case 0x43:
            routeOrMotion = 0x291;
            break;
        case 0x35:
            routeOrMotion = 0x29E;
            break;
        case 0x36:
            routeOrMotion = 0x2A0;
            break;
        case 0x38:
            routeOrMotion = 0x29C;
            break;
        case 0x46:
        case 0x47:
            routeOrMotion = 0x295;
            break;
        case 0x44:
            routeOrMotion = 0x290;
            break;
        case 0x3A:
            routeOrMotion = 0x296;
            break;
        case 0x3B:
            routeOrMotion = 0x298;
            break;
        case 0x41:
            routeOrMotion = 0x261;
            break;
        case 0x48:
            routeOrMotion = 0x2A6;
            break;
        case 0x4B:
            routeOrMotion = 0x27E;
            break;
        case 0x4C:
            routeOrMotion = 0x285;
            break;
        case 0x4D:
            routeOrMotion = 0x284;
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
            routeOrMotion = 0x3E9;
        } else {
            switch (*(s32 *)&((NpcRouteEnt *)w->route)->state) {
            case 0x4E:
                routeOrMotion = 0x426;
                break;
            case 0x98:
                routeOrMotion = 0x43A;
                break;
            case 0x52:
            case NPC_STATE_HOUSE_OPEN:
                routeOrMotion = 0x432;
                break;
            case 0x57:
                if (gPetFollows == 1) {
                    Actor_SetState(npc, 0, PET_STATE_FOLLOW);
                    routeOrMotion = 0x3E9;
                } else {
                    routeOrMotion = 0x3E9;
                }
                break;
            default:
                routeOrMotion = 0x3E9;
                break;
            }
        }
        Actor_SetMotionOnLayer(npc, routeOrMotion, 0, 0, 0);
        Player_BeginLayerMotion((Player *) npc, npc->motionFrame[0], npc->motionBlend[0], 0);
    }
    entryOfs = w->routeIdx * 0x14;
    routeOrMotion = (u32) w->route;
    Actor_SetState(npc, 0, *(s32 *)(entryOfs + routeOrMotion + 0xC));
    Player_UpdateMotion((Player *) npc);
    Npc_UpdateMotion(npc);
    Npc_UpdateMotion(npc);
    Npc_FollowFloor(npc);
    Npc_UpdateAreaPos(npc);
    if (Npc_IsFree((Em *)npc) != 0) {
        Em_GetBonePos(npc, 0x14, w->markAnchor);
    } else if (Npc_IsAct51to55(npc) != 0) {
        Em_GetBonePos(npc, 0xA, w->markAnchor);
    } else if (Npc_HasSecondHead((Em *)npc) != 0) {
        Em_GetBonePos(npc, 0xC, w->markAnchor);
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
 * set (Area_GetSetIndex), or NULL; NULL also for kind 0xFF, no
 * townsperson. */
NpcAreaEntry *NpcTable_Find(u8 npcId, s32 areaId)
{
    u8 i;
    NpcAreaEntry *entry;

    if (npcId == 0xFF) {
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

s32 Cond_Check(u16 id);
s32 PspLink_HasFlag(s32);
s32 Save_TestEventFlag(s32 flag);
/* unit's own declaration: an s32 return (the definition returns u8); the callers cut the level to a byte themselves, and with a u8 return Npc_IsPresent and Npc_SetRouteByKind compile differently from the game's code */
s32 Village_FacilityLevel(s32);
u8 Village_TotalLevel(void);
s32 Npc_RollPresence(void);
/* unit's own declaration: an s32 return (the definition returns s16): Npc_IsPresent's tests of the result compile as the game's code only with the s32 */
s32 Village_MilestoneState(s32);
s32 ElderRaid_InState1(void);
u8 Npc_PairIdBySide(u8 kind);
s32 MyHouse_IsOwner(void);
s8 MyHouse_LocalSlot(void);
s32 Game_IsOnline(void);
extern u8 gPoogieLeft;
extern u8 gCarePetLeft;
extern u8 gHousePetLeft;
extern NetUser gNetUsers[];
extern u8 gPetFollows;

/* Whether townsperson kind (an id; its entry NpcTable_Find(kind, id), id
 * being the area) is in town now; 0 when it has no entry there. First its
 * own rule: event flags, conditions, the village facilities' levels (for
 * 0x16 and 0x17 the sum of the tavern's, the smithy's and the port's), the
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

    if (npcId == 0xFF) {
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
    case 0x1:
        if (Save_TestEventFlag(0x186) == 1 && Save_TestEventFlag(0x187) == 0) {
            return 1;
        }
        if (Cond_Check(0x1A) == 1 && Village_MilestoneState(5) == 0) {
            return 0;
        }
        break;
    case 0x1B:
        if (Save_TestEventFlag(0x186) == 1 && Save_TestEventFlag(0x187) == 0) {
            return 0;
        }
        if (Cond_Check(0x1A) == 0 || Village_MilestoneState(5) != 0) {
            return 0;
        }
        break;
    case 0x4:
    case 0x8:
        if (Cond_Check(3) == 0) {
            return 0;
        }
        break;
    case NPC_DRINK_CONTEST_HOST:
        if (Cond_Check(0xD) == 0) {
            return 0;
        }
        break;
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x3E:
        if (ElderRaid_InState1() == 1) {
            return 0;
        }
        break;
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x3F:
        if (ElderRaid_InState1() == 0) {
            return 0;
        }
        break;
    case 0x6:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) == 0) {
            return 0;
        }
        break;
    case 0xA:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) < 2) {
            return 0;
        }
        break;
    case 0xB:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) == 0) {
            return 0;
        }
        break;
    case 0xC:
    case 0x1A:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_SMITHY) == 0) {
            return 0;
        }
        break;
    case 0xD:
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
        if (Save_TestEventFlag(0x49) == 0) {
            return 0;
        }
        if (Save_TestEventFlag(0x4A) == 0) {
            return 1;
        }
        if (PspLink_HasFlag(0xB) != 0) {
            return 1;
        }
        if (Village_MilestoneState(2) == 0) {
            return 0;
        }
        break;
    case 0x13:
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
                     * NetUser's text): poogieOutfit, 0xFF while its Poogie
                     * has no name */
                    card = (HunterCard *)gNetUsers[slot].text;
                    return (card->poogieOutfit != 0xFF) ? 1 : 0;
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
        if (Npc_RollPresence() == 0 || gCarePetLeft == 1) {
            return 0;
        }
        break;
    case 0x6B:
        if (Cond_Check(0x53) == 0) {
            return 0;
        }
        break;
    case NPC_HOUSE_PET:
        if (gHousePetLeft == 1) {
            return 0;
        }
        break;
    case 0x15:
        if (Save_TestEventFlag(EVENT_FLAG_VILLAGE_LEVEL_2) == 0 || Save_TestEventFlag(EVENT_FLAG_TOUR_VILLAGE_LEVEL_2) == 0) {
            return 0;
        }
        break;
    case 0x16:
    case 0x17:
        if ((u8)facilityTotal < 4) {
            return 0;
        }
        break;
    case 0x18:
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
        if (ElderRaid_InState1() == 0) {
            return 0;
        }
        break;
    case 0x6F:
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
 * there is no start route. A start route ends by jumping (state 0x83) to
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

s32 Cond_Check(u16 id);
/* unit's own declaration: an s32 return (the definition returns u8); the callers cut the level to a byte themselves, and with a u8 return Npc_IsPresent and Npc_SetRouteByKind compile differently from the game's code */
s32 Village_FacilityLevel(s32);
void Npc_SetRouteByTime(Em *arg0, NpcRouteEnt **t);
s32 Area_IsHouse(u16 arg0);
s32 Game_IsOnline(void);
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
extern s8 gPoogieByPlayer;
extern u8 gPetFollows;

/* Puts a townsperson on its route for the time of day and the village's
 * state (Npc_SetRouteAmPm, Npc_SetRouteByTime), by its id: townsperson 1 by
 * condition 1 and the port's level, 4 by the smithy's level, 9 by the
 * port's, the drinking host by the sum of the tavern's, the smithy's and the
 * port's levels, 0x69 by condition 0x53, the others by their own tables. The
 * Poogie, when gPetFollows is set in AREA_HUB_0x89 and it is not digging,
 * takes the one-point route that follows the player (PET_STATE_FOLLOW).
 * Nothing changes offline in the house, except for townsperson 0x67. */
void Npc_SetRouteByKind(Em *npc)
{
    u8 i;
    NpcWork *w;
    s32 facilityTotal;
    s32 portLevel;
    u8 id;

    w = (NpcWork *)&npc->work;
    if ((Game_IsOnline() != 0) || (w->id == 0x67) || (Area_IsHouse(gScene.area) != 1)) {
        facilityTotal = 0;
        /* the levels of VILLAGE_FACILITY_TAVERN to VILLAGE_FACILITY_PORT */
        for (i = 1; i <= 3; i++) {
            facilityTotal = (u8)(facilityTotal + Village_FacilityLevel(i));
        }
        id = w->id;
        switch (id) {
        case 0x1:
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
        case 0x3:
            Npc_SetRouteAmPm(npc, gNpcRoute03Am, gNpcRoute03Pm);
            break;
        case 0x4:
            if (!(Village_FacilityLevel(VILLAGE_FACILITY_SMITHY) & 0xFF)) {
                Npc_SetRouteAmPm(npc, gNpcRoute04Am, gNpcRoute04Pm);
                break;
            }
            Npc_SetRouteAmPm(npc, gNpcRoute04AmLv, gNpcRoute04Pm);
            break;
        case 0x5:
            Npc_SetRouteAmPm(npc, gNpcRoute05Am, gNpcRoute05Pm);
            break;
        case 0x6:
            Npc_SetRouteByTime(npc, gNpc06Routes);
            break;
        case 0x9:
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
        case 0x30:
            Npc_SetRouteByTime(npc, gNpc30Routes);
            break;
        case 0x31:
            Npc_SetRouteByTime(npc, gNpc31Routes);
            break;
        case 0x32:
            Npc_SetRouteByTime(npc, gNpc32Routes);
            break;
        case 0x45:
            Npc_SetRouteByTime(npc, gNpc45Routes);
            break;
        case 0x46:
            Npc_SetRouteByTime(npc, gNpc46Routes);
            break;
        case 0x47:
            Npc_SetRouteByTime(npc, gNpc47Routes);
            break;
        case 0x49:
            Npc_SetRouteByTime(npc, gNpc49Routes);
            break;
        case 0x4A:
            Npc_SetRouteByTime(npc, gNpc4ARoutes);
            break;
        case 0x4B:
            Npc_SetRouteByTime(npc, gNpc4BRoutes);
            break;
        case 0x4C:
            Npc_SetRouteByTime(npc, gNpc4CRoutes);
            break;
        case 0x5F:
            Npc_SetRouteByTime(npc, gNpc5FRoutes);
            break;
        case 0x60:
            Npc_SetRouteByTime(npc, gNpc60Routes);
            break;
        case 0x61:
            Npc_SetRouteByTime(npc, gNpc61Routes);
            break;
        case 0x62:
            Npc_SetRouteByTime(npc, gNpc62Routes);
            break;
        case 0x67:
            Npc_SetRouteByTime(npc, gNpc67Routes);
            break;
        case 0x69:
            if (Cond_Check(0x53U) == 1) {
                Npc_SetRouteByTime(npc, gNpc69RoutesCond53);
                break;
            }
            Npc_SetRouteByTime(npc, gNpc69Routes);
            break;
        }
    }
}

/* During the elder raid (Npc_SetupFromArea calls it then): gives
 * townspeople 0x1D-0x1F, 0x21, 0x23, 0x24, 0x3A-0x3D and the official
 * desks' clerks their
 * raid route, and most of them a faster motion (1.3 times; 1.1 times for
 * 0x1C, which keeps its route; NPC_OFFICIAL_DESK_HIGH_HR's set to 1.3).
 * Townspeople 0x25-0x2B only move faster. 0x3A takes the route named
 * gNpcRaidRoute3A. */
void Npc_SetRaidRoute(Em *npc)
{
    u8 npcId;
    NpcWork *w;

    npcId = npc->work;
    w = (NpcWork *)&npc->work;
    switch (npcId) {
    case 0x1D:
        w->route = gNpcRaidRoute1D;
        break;
    case 0x1E:
        w->route = gNpcRaidRoute1E;
        break;
    case 0x1F:
        w->route = gNpcRaidRoute1F;
        npc->motionSpeed *= 1.3f;
        break;
    case 0x21:
        w->route = gNpcRaidRoute21;
        npc->motionSpeed *= 1.3f;
        break;
    case 0x1C:
        npc->motionSpeed *= 1.1f;
        break;
    case 0x23:
        w->route = gNpcRaidRoute23;
        npc->motionSpeed *= 1.3f;
        break;
    case 0x24:
        w->route = gNpcRaidRoute24;
        npc->motionSpeed *= 1.3f;
        break;
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2A:
    case 0x2B:
        npc->motionSpeed *= 1.3f;
        break;
    case 0x3A:
        w->route = gNpcRaidRoute3A;
        npc->motionSpeed *= 1.3f;
        break;
    case 0x3B:
        w->route = gNpcRaidRoute3B;
        npc->motionSpeed *= 1.3f;
        break;
    case 0x3C:
        w->route = gNpcRaidRoute3C;
        npc->motionSpeed *= 1.3f;
        break;
    case 0x3D:
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
s32 MyHouse_InOwnHouse(void);
extern NpcRouteEnt gNpcRouteHouse[];
extern NpcRouteEnt gNpcRouteHouse0A[];
extern NpcRouteEnt gNpcRoute07Lv2[];
extern NpcRouteEnt gNpcRoute08Lv2[];
extern NpcRouteEnt gNpcRoute0CLv2[];
extern NpcRouteEnt gSmithRoute[];
extern NpcRouteEnt gMinerRouteHigh[];
extern NpcRouteEnt gMinerRouteLow[];
extern NpcRouteEnt gMinerRouteMid[];
extern NpcRouteEnt gNpcRoute13High[];
extern NpcRouteEnt gNpcRoute13Low[];
extern NpcRouteEnt gNpcRoute13Mid[];
extern NpcRouteEnt gNpcRoute14Guest[];
extern NpcRouteEnt gNpcRoute1A[];

/* Replaces a townsperson's route (NpcWork.route) by the village's state.
 * Offline in the house, a resident with a meal visit waiting takes the
 * house's visit route (townsperson 0xA its own). Else by id: 7 and 8 at
 * port level 2, 0xC at smithy level 2 and the smith (NPC_WORKSHOP_SMITH)
 * once the smithy is built take other spots (0xC and the smith with a set
 * yaw); the miner (0x13) takes its route by the mine's level (three
 * ranges), another set while a dig runs (MINE_DIGGING); the Poogie visiting
 * another's house online takes its guest route; 0x1A moves (with a set yaw)
 * while NPC_WORKSHOP_HELPER is in town. */
void Npc_SetRouteByVillage(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    if (Game_IsOnline() == 0 && Area_IsHouse(gScene.area) == 1 && Village_HasMealVisit(w->id) == 1) {
        if (w->id != 0xA) {
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
    case 0xC:
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
    case 0x13:
        /* SaveData.mine.active: a dig runs */
        if (*(u8 *)((u8 *)&gSave + 0x37F3) == MINE_DIGGING) {
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
    case 0x1A:
        if (Npc_IsPresent(NPC_WORKSHOP_HELPER, gScene.area, 0) == 1) {
            w->route = gNpcRoute1A;
            npc->rotY = 0x6E39;
        }
        break;
    }
}

struct Em;
s16 OfferMenu_BuildList(struct Em *t, s32 b);
s32 Kitchen_SetCourseReady(void);
s32 ElderRaid_InState1(void);
s32 Poogie_PlaceByPlayer(void);
s32 Npc_HasDrinkMug(Em *npc);
void DrinkMug_Spawn(Em *npc);
s32 MyHouse_IsOwner(void);

/* Sets townsperson npc up from its entry (Em.index) in the table of area
 * id's set: yaw, type, model set, name (the Poogie's is
 * SaveData.poogieName), id, route (then Npc_SetRouteByKind,
 * Npc_SetRaidRoute during the elder raid, and Npc_SetRouteByVillage),
 * presence rule, motion speed, scale, and its offer mark from its offer
 * list (2 also for the house's cook with its set course ready). Unless keep
 * is set (always for presence rule 4), it follows its route's first jump
 * (state 0x83 or 0xB3) and starts at the first point with no wait (arg
 * -1), with the yaw of the last turn point (state 0x81) before it. Then it
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
    if (ElderRaid_InState1() == 1) {
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

s32 Npc_IsFreeAct2Bto30(Em *arg0);
struct TriHdr;
s32 Em_GetFloorInfoEx(Em *em, f32 *pos, struct TriHdr *info, f32 *out, f32 *depth);

/* Keeps a townsperson on the floor: the floor under it (Em_GetFloorInfoEx)
 * and, unless its stance is 2, its height set to the floor's. Not for
 * townspeople 7, 8, 0xE, 0x15, 0x1C-0x1E, 0x20, 0x2C, 0x2E, 0x40, 0x48 and
 * 0x4D, nor for 0xA and 0x1A outside AREA_HUB_0x89 (they keep the height
 * they were placed at), nor while a free townsperson's type is 0 or
 * 0x2B-0x30 (Npc_IsFreeAct2Bto30). */
void Npc_FollowFloor(Em *npc)
{
    switch (npc->work) {
    case 0x7:
    case 0x8:
    case 0xE:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x20:
    case 0x2C:
    case 0x2E:
    case 0x40:
    case 0x48:
    case 0x4D:
    case 0x15:
        return;
    case 0x1A:
    case 0xA:
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

f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Npc_UpdateMotion(Em *arg0);
s32 Npc_IsMusicActor(Em *arg0);
s32 Npc_UpdateState(Em *a);
void Actor_TickTimers(Player *arg0);
void Npc_StepMoveTo(Em *obj);
void Npc_UpdateHitStop(Em *arg0);
s32 LobbyBgm_IsActive(void);

/* A townsperson's per-frame task: its timers and hit stop, its move toward
 * a point (Npc_StepMoveTo), its state (Npc_UpdateState), its rotation
 * matrix, its motions at twice its motion speed, and its floor. In state
 * 0x60 (it has left) its task phase moves on. Returns 0, doing nothing, for
 * a type 0x4E townsperson while the lobby's world stream is active
 * (LobbyBgm_IsActive); else 1. */
s32 Npc_Run(Em *npc)
{
    if (Npc_IsMusicActor(npc) == 1 && LobbyBgm_IsActive()) {
        return 0;
    }
    npc->noPushTimer = 0xA;
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

void EmSlot_Free(struct Em *em);

/* Frees the townsperson's Em slot (Npc_Update's last task phase). */
void Npc_Free(Em *npc)
{
    EmSlot_Free(npc);
}
void Player_BeginLayerMotion(Player *p, s32 frame, s32 n, s32 part);
s32 Player_UpdateMotion(Player *p);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
s32 Debug_IsOn(s32 arg0);

/* Clears the talked-to mark (NpcWork.unk3) of all EM_COUNT slots, so that every
 * townsperson starts with its first line again: on the next day
 * (LobbyWorld_OnNextDay), when days pass (LobbyWorld_OnAddDays) and after a
 * talk's event step. */
void Npc_ClearAllTalked(void)
{
    s32 i;

    for (i = 0; i < EM_COUNT; i++) {
        ((NpcWork *)&gEms[i].work)->talked = 0;
    }
}

s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Npc_Init(Em *a);
s32 Npc_Run(Em *arg0);
void Npc_RunMotionFx(Em *npc);
void Npc_UpdateAreaPos(Em *npc);
s32 Game_IsOnline(void);

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
            OT_AddByDepth(gDrawOt32, npc->drawSlot, 0x20, 0);
        }
    }
    Npc_UpdateAreaPos(npc);
    return 0;
}

/* Unless debug switch 0x28 is on (Debug_IsOn answers 0 in the released game):
 * the townsperson's rotation matrix, then, while it is not in hit stop, starts
 * the motions waiting on layers 0 and 1 and advances them. These are the steps
 * of the player's Player_PrepareMotions; the townsperson goes to the player's
 * motion code as a Player, whose motion fields are at the same offsets. */
void Npc_UpdateMotion(Em *npc)
{
    if (Debug_IsOn(0x28) == 0) {
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

void Mtx_Scale(f32 *, f32, f32, f32);
void Mtx_Concat3x3(f32 *m, f32 *n);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Copy(f32 *dst, f32 *src);
f32 Math_Angle16ToRad(u16 a);
struct EmBone;
void Bone_UpdateWorldScaledRoot(struct EmBone *sk, f32 *m);
void Mtx_RotateY(f32 *, f32);
f32 *Em_BoneMatrixB(void *obj, s16 bone);
s32 Actor_HeadingDiff(void *obj, void *pos);
s32 Npc_HasSecondHead(Em *arg0);
s32 Npc_IsAct51to55(Em *arg0);
s32 Npc_IsPet(Em *arg0);
s32 Npc_IsType4D(Em *arg0);
s32 Npc_IsMusicActor(Em *arg0);
s32 Npc_IsType3Cto3E(Em *arg0);

/* The townsperson's model matrix (scale, angles, position, into Em.worldMtx),
 * then its bones' matrices. Its head bone turns toward the local player, eased
 * by a fifth of the gap a frame (NpcWork.headYaw), at most 10 degrees on the
 * second body (Npc_HasSecondHead: bone 0xC) or 20 on the others (bone 0x13).
 * The head stays still for townspeople 1, 0x21, 0x55 and 0x59-0x5B (0x55 and
 * 0x59-0x5B are the online workshop lessons' townspeople) and NPC_CONTEST_HOST,
 * for the looks 0x4D, 0x4E, 0x3C-0x3E and 0x4F-0x55 (the music actor and the
 * pets among them), and during the motions 0x284 and 0x286. */
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
    case 0x21:
    case 0x55:
    case 0x59:
    case 0x5A:
    case 0x5B:
        still = 1;
        break;
    default:
        still = 0;
        break;
    }
    if (Npc_IsType4D(npc) == 1) {
        still = 1;
    }
    if (Npc_IsType3Cto3E(npc) == 1) {
        still = 1;
    }
    if (Npc_IsMusicActor(npc) == 1) {
        still = 1;
    }
    switch (npc->motionId[0]) {
    case 0x284:
    case 0x286:
        still = 1;
        break;
    }
    if (Npc_IsAct51to55(npc) == 1 || Npc_IsPet(npc) == 1) {
        still = 1;
    }
    if (!still) {
        if (Npc_HasSecondHead(npc) != 0) {
            bone = Em_BoneMatrixB(npc, 0xC);
            limit = 10.0f;
        } else {
            bone = Em_BoneMatrixB(npc, 0x13);
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
 * for body 0, NpcSetB_Run and NpcSetA_Run for bodies 2 and 3, a no-op for 4-9. */
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
extern u32 gNpcRoute6ELoop[];
extern u32 gNpcRoute6E[];
NpcRouteEnt gNpc01Routes1_1[31] = {
#include "lobby/007F20B0.inc"
};
NpcRouteEnt gNpc01Routes2_1[31] = {
#include "lobby/007F2320.inc"
};
NpcRouteEnt gNpc01Routes1_3[7] = {
#include "lobby/007F2590.inc"
};
NpcRouteEnt gNpc01Routes1_0[6] = {
#include "lobby/007F2620.inc"
};
NpcRouteEnt gNpc01Routes2_0[6] = {
#include "lobby/007F26A0.inc"
};
NpcRouteEnt gNpc01Routes0_0[3] = {
#include "lobby/007F2720.inc"
};
NpcRouteEnt *gNpc01Routes0[4] = {
    gNpc01Routes0_0, gNpc01Routes0_0, NULL, gNpc01Routes0_0,
};
NpcRouteEnt *gNpc01Routes1[4] = {
    gNpc01Routes1_0, gNpc01Routes1_1, NULL, gNpc01Routes1_3,
};
NpcRouteEnt *gNpc01Routes2[4] = {
    gNpc01Routes2_0, gNpc01Routes2_1, NULL, gNpc01Routes1_3,
};
u32 gNpcRoute02[12] = {
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
u32 gNpcRoute07[12] = {
#include "lobby/007F2E20.inc"
};
NpcRouteEnt gNpcRoute07Lv2[2] = {
#include "lobby/007F2E50.inc"
};
u32 gNpcRoute08[12] = {
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
NpcRouteEnt gNpc09Routes1_2[6] = {
#include "lobby/007F32E0.inc"
};
NpcRouteEnt *gNpc09Routes1[4] = {
    gNpc09Routes1_0, NULL, gNpc09Routes1_2, gNpc09Routes1_3,
};
NpcRouteEnt *gNpc09Routes2[4] = {
    gNpc09Routes2_0, NULL, NULL, gNpc09Routes2_3,
};
u32 gNpcRoute0A[16] = {
#include "lobby/007F3380.inc"
};
u32 gNpcRoute0B[12] = {
#include "lobby/007F33C0.inc"
};
u32 gNpcRoute0C[20] = {
#include "lobby/007F33F0.inc"
};
NpcRouteEnt gNpcRoute0CLv2[3] = {
#include "lobby/007F3440.inc"
};
u32 gNpcRoute0D[16] = {
#include "lobby/007F3480.inc"
};
u32 gSmithRouteBase[16] = {
#include "lobby/007F34C0.inc"
};
NpcRouteEnt gSmithRoute[3] = {
#include "lobby/007F3500.inc"
};
u32 gWorkshopHelperRoute[20] = {
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
NpcRouteEnt gNpc10Routes0_0[16] = {
#include "lobby/007F38D0.inc"
};
NpcRouteEnt gNpc10Routes1_0[16] = {
#include "lobby/007F3A10.inc"
};
NpcRouteEnt *gNpc10Routes0[4] = {
    gNpc10Routes0_0, gNpc10Routes0_1, NULL, gNpc10Routes0_3,
};
NpcRouteEnt *gNpc10Routes1[4] = {
    gNpc10Routes1_0, gNpc10Routes1_1, NULL, gNpc10Routes0_3,
};
u32 gNamingTalkerRoute[56] = {
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
u32 gPoogieRouteVillage[12] = {
#include "lobby/007F3DD0.inc"
};
NpcRouteEnt gPoogieFollowRoute[2] = {
#include "lobby/007F3E00.inc"
};
NpcRouteEnt gNpcRoute14Guest[2] = {
#include "lobby/007F3E30.inc"
};
u32 gPoogieRouteHouse0[28] = {
#include "lobby/007F3E60.inc"
};
u32 gPoogieRouteHouse1[20] = {
#include "lobby/007F3ED0.inc"
};
u32 gPoogieRouteHouse2[36] = {
#include "lobby/007F3F20.inc"
};
u32 gNpcRoute15[20] = {
#include "lobby/007F3FB0.inc"
};
u32 gNpcRoute16[12] = {
#include "lobby/007F4000.inc"
};
u32 gNpcRoute17[12] = {
#include "lobby/007F4030.inc"
};
u32 gNpcRoute18[120] = {
#include "lobby/007F4060.inc"
};
u32 gFishingSpotRoute[32] = {
#include "lobby/007F4240.inc"
};
u32 gNpcRoute1ABase[20] = {
#include "lobby/007F42C0.inc"
};
NpcRouteEnt gNpcRoute1A[4] = {
#include "lobby/007F4310.inc"
};
u32 gNpcRoute1B[72] = {
#include "lobby/007F4360.inc"
};
u32 gNpcRoute1C[12] = {
#include "lobby/007F4480.inc"
};
u32 gNpcRoute1D[16] = {
#include "lobby/007F44B0.inc"
};
NpcRouteEnt gNpcRaidRoute1D[3] = {
#include "lobby/007F44F0.inc"
};
u32 gNpcRoute1E[12] = {
#include "lobby/007F4530.inc"
};
NpcRouteEnt gNpcRaidRoute1E[2] = {
#include "lobby/007F4560.inc"
};
u32 gNpcRoute1F[12] = {
#include "lobby/007F4590.inc"
};
NpcRouteEnt gNpcRaidRoute1F[3] = {
#include "lobby/007F45C0.inc"
};
u32 gNpcRoute20[12] = {
#include "lobby/007F4600.inc"
};
u32 gNpcRoute21[12] = {
#include "lobby/007F4630.inc"
};
NpcRouteEnt gNpcRaidRoute21[3] = {
#include "lobby/007F4660.inc"
};
u32 gCarePetRoute[20] = {
#include "lobby/007F46A0.inc"
};
u32 gNpcRoute23[12] = {
#include "lobby/007F46F0.inc"
};
NpcRouteEnt gNpcRaidRoute23[3] = {
#include "lobby/007F4720.inc"
};
u32 gNpcRoute24[108] = {
#include "lobby/007F4760.inc"
};
NpcRouteEnt gNpcRaidRoute24[3] = {
#include "lobby/007F4910.inc"
};
u32 gNpcRoute25[56] = {
#include "lobby/007F4950.inc"
};
u32 gNpcRoute26[40] = {
#include "lobby/007F4A30.inc"
};
u32 gNpcRoute27[32] = {
#include "lobby/007F4AD0.inc"
};
u32 gNpcRoute28[12] = {
#include "lobby/007F4B50.inc"
};
u32 gNpcRoute29[36] = {
#include "lobby/007F4B80.inc"
};
u32 gNpcRoute2A[48] = {
#include "lobby/007F4C10.inc"
};
u32 gNpcRoute2B[48] = {
#include "lobby/007F4CD0.inc"
};
u32 gNpcRoute2C[12] = {
#include "lobby/007F4D90.inc"
};
u32 gNpcRoute2D[12] = {
#include "lobby/007F4DC0.inc"
};
u32 gNpcRoute2E[12] = {
#include "lobby/007F4DF0.inc"
};
u32 gNpcRoute2F[12] = {
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
u32 gFishingSpotOnlineRoute[16] = {
#include "lobby/007F5420.inc"
};
u32 gNpcRoute34[32] = {
#include "lobby/007F5460.inc"
};
u32 gNpcRoute35[36] = {
#include "lobby/007F54E0.inc"
};
u32 gNpcRoute36[60] = {
#include "lobby/007F5570.inc"
};
u32 gNpcRoute37[12] = {
#include "lobby/007F5660.inc"
};
u32 gNpcRoute38[12] = {
#include "lobby/007F5690.inc"
};
u32 gNpcRoute39[12] = {
#include "lobby/007F56C0.inc"
};
u32 gNpcRoute3A[52] = {
#include "lobby/007F56F0.inc"
};
NpcRouteEnt gNpcRaidRoute3A[7] = {
#include "lobby/007F57C0.inc"
};
u32 gNpcRoute3B[12] = {
#include "lobby/007F5850.inc"
};
NpcRouteEnt gNpcRaidRoute3B[3] = {
#include "lobby/007F5880.inc"
};
u32 gNpcRoute3C[12] = {
#include "lobby/007F58C0.inc"
};
NpcRouteEnt gNpcRaidRoute3C[3] = {
#include "lobby/007F58F0.inc"
};
u32 gNpcRoute3D[12] = {
#include "lobby/007F5930.inc"
};
NpcRouteEnt gNpcRaidRoute3D[3] = {
#include "lobby/007F5960.inc"
};
u32 gNpcRoute3E[88] = {
#include "lobby/007F59A0.inc"
};
u32 gNpcRoute3F[88] = {
#include "lobby/007F5B00.inc"
};
u32 gOfficialDeskLowRoute[12] = {
#include "lobby/007F5C60.inc"
};
NpcRouteEnt gOfficialDeskLowRaidRoute[2] = {
#include "lobby/007F5C90.inc"
};
u32 gNpcRoute41[12] = {
#include "lobby/007F5CC0.inc"
};
u32 gNpcRoute42[12] = {
#include "lobby/007F5CF0.inc"
};
NpcRouteEnt gNpc45Routes_1[5] = {
#include "lobby/007F5D20.inc"
};
NpcRouteEnt gNpc45Routes_0[7] = {
#include "lobby/007F5D90.inc"
};
NpcRouteEnt gNpc45Routes_2[5] = {
#include "lobby/007F5E20.inc"
};
NpcRouteEnt *gNpc45Routes[4] = {
    gNpc45Routes_0, gNpc45Routes_1, gNpc45Routes_2, NULL,
};
NpcRouteEnt gNpc46Routes_1[5] = {
#include "lobby/007F5EA0.inc"
};
NpcRouteEnt gNpc46Routes_0[8] = {
#include "lobby/007F5F10.inc"
};
NpcRouteEnt gNpc46Routes_2[7] = {
#include "lobby/007F5FB0.inc"
};
NpcRouteEnt *gNpc46Routes[4] = {
    gNpc46Routes_0, gNpc46Routes_1, gNpc46Routes_2, NULL,
};
NpcRouteEnt gNpc47Routes_1[5] = {
#include "lobby/007F6050.inc"
};
NpcRouteEnt gNpc47Routes_0[11] = {
#include "lobby/007F60C0.inc"
};
NpcRouteEnt gNpc47Routes_2[10] = {
#include "lobby/007F61A0.inc"
};
NpcRouteEnt *gNpc47Routes[4] = {
    gNpc47Routes_0, gNpc47Routes_1, gNpc47Routes_2, NULL,
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
NpcRouteEnt gNpc4ARoutes_2[11] = {
#include "lobby/007F64B0.inc"
};
NpcRouteEnt gNpc4ARoutes_0[7] = {
#include "lobby/007F6590.inc"
};
NpcRouteEnt *gNpc4ARoutes[4] = {
    gNpc4ARoutes_0, NULL, gNpc4ARoutes_2, gNpc4ARoutes_3,
};
NpcRouteEnt gNpc4BRoutes_3[3] = {
#include "lobby/007F6630.inc"
};
NpcRouteEnt gNpc4BRoutes_2[11] = {
#include "lobby/007F6670.inc"
};
NpcRouteEnt gNpc4BRoutes_0[9] = {
#include "lobby/007F6750.inc"
};
NpcRouteEnt *gNpc4BRoutes[4] = {
    gNpc4BRoutes_0, NULL, gNpc4BRoutes_2, gNpc4BRoutes_3,
};
NpcRouteEnt gNpc4CRoutes_3[3] = {
#include "lobby/007F6820.inc"
};
NpcRouteEnt gNpc4CRoutes_2[8] = {
#include "lobby/007F6860.inc"
};
NpcRouteEnt gNpc4CRoutes_0[6] = {
#include "lobby/007F6900.inc"
};
NpcRouteEnt *gNpc4CRoutes[4] = {
    gNpc4CRoutes_0, NULL, gNpc4CRoutes_2, gNpc4CRoutes_3,
};
u32 gNpcRoute4D[12] = {
#include "lobby/007F6990.inc"
};
u32 gOfficialDeskHighRoute[12] = {
#include "lobby/007F69C0.inc"
};
NpcRouteEnt gOfficialDeskHighRaidRoute[7] = {
#include "lobby/007F69F0.inc"
};
u32 gQuestDesk2Route[12] = {
#include "lobby/007F6A80.inc"
};
u32 gNpcRoute50[12] = {
#include "lobby/007F6AB0.inc"
};
u32 gNpcRoute51[148] = {
#include "lobby/007F6AE0.inc"
};
u32 gNpcRoute52[12] = {
#include "lobby/007F6D30.inc"
};
u32 gNpcRoute53[12] = {
#include "lobby/007F6D60.inc"
};
u32 gNpcRoute54[12] = {
#include "lobby/007F6D90.inc"
};
u32 gNpcRoute55[12] = {
#include "lobby/007F6DC0.inc"
};
u32 gNpcRoute56[12] = {
#include "lobby/007F6DF0.inc"
};
u32 gNpcRoute57[96] = {
#include "lobby/007F6E20.inc"
};
u32 gLessonRouteBHome[16] = {
#include "lobby/007F6FA0.inc"
};
u32 gLessonRouteB1[28] = {
#include "lobby/007F6FE0.inc"
};
u32 gLessonRouteB2[32] = {
#include "lobby/007F7050.inc"
};
u32 gNpcRoute59[12] = {
#include "lobby/007F70D0.inc"
};
u32 gNpcRoute5A[12] = {
#include "lobby/007F7100.inc"
};
u32 gNpcRoute5B[12] = {
#include "lobby/007F7130.inc"
};
u32 gLessonRouteAHome[16] = {
#include "lobby/007F7160.inc"
};
u32 gLessonRouteA1[20] = {
#include "lobby/007F71A0.inc"
};
u32 gLessonRouteA2[28] = {
#include "lobby/007F71F0.inc"
};
u32 gNpcRoute5D[12] = {
#include "lobby/007F7260.inc"
};
u32 gNpcRoute5E[12] = {
#include "lobby/007F7290.inc"
};
NpcRouteEnt gNpc5FRoutes_1[2] = {
#include "lobby/007F72C0.inc"
};
NpcRouteEnt gNpc5FRoutes_3[12] = {
#include "lobby/007F72F0.inc"
};
NpcRouteEnt gNpc5FRoutes_0[7] = {
#include "lobby/007F73E0.inc"
};
NpcRouteEnt gNpc5FRoutes_2[8] = {
#include "lobby/007F7470.inc"
};
NpcRouteEnt *gNpc5FRoutes[4] = {
    gNpc5FRoutes_0, gNpc5FRoutes_1, gNpc5FRoutes_2, gNpc5FRoutes_3,
};
NpcRouteEnt gNpc60Routes_1[2] = {
#include "lobby/007F7520.inc"
};
NpcRouteEnt gNpc60Routes_3[2] = {
#include "lobby/007F7550.inc"
};
NpcRouteEnt gNpc60Routes_0[4] = {
#include "lobby/007F7580.inc"
};
NpcRouteEnt gNpc60Routes_2[3] = {
#include "lobby/007F75D0.inc"
};
NpcRouteEnt *gNpc60Routes[4] = {
    gNpc60Routes_0, gNpc60Routes_1, gNpc60Routes_2, gNpc60Routes_3,
};
NpcRouteEnt gNpc61Routes_1[2] = {
#include "lobby/007F7620.inc"
};
NpcRouteEnt gNpc61Routes_3[2] = {
#include "lobby/007F7650.inc"
};
NpcRouteEnt gNpc61Routes_0[5] = {
#include "lobby/007F7680.inc"
};
NpcRouteEnt gNpc61Routes_2[4] = {
#include "lobby/007F76F0.inc"
};
NpcRouteEnt *gNpc61Routes[4] = {
    gNpc61Routes_0, gNpc61Routes_1, gNpc61Routes_2, gNpc61Routes_3,
};
NpcRouteEnt gNpc62Routes_1[22] = {
#include "lobby/007F7750.inc"
};
NpcRouteEnt gNpc62Routes_3[3] = {
#include "lobby/007F7910.inc"
};
NpcRouteEnt gNpc62Routes_0[10] = {
#include "lobby/007F7950.inc"
};
NpcRouteEnt gNpc62Routes_2[13] = {
#include "lobby/007F7A20.inc"
};
NpcRouteEnt *gNpc62Routes[4] = {
    gNpc62Routes_0, gNpc62Routes_1, gNpc62Routes_2, gNpc62Routes_3,
};
u32 gNpcRoute64[12] = {
#include "lobby/007F7B40.inc"
};
u32 gNpcRoute65[12] = {
#include "lobby/007F7B70.inc"
};
u32 gNpcRoute66[40] = {
#include "lobby/007F7BA0.inc"
};
NpcRouteEnt gNpc67Routes_1[12] = {
#include "lobby/007F7C40.inc"
};
NpcRouteEnt gNpc67Routes_3[3] = {
#include "lobby/007F7D30.inc"
};
NpcRouteEnt gNpc67Routes_2[11] = {
#include "lobby/007F7D70.inc"
};
NpcRouteEnt gNpc67Routes_0[21] = {
#include "lobby/007F7E50.inc"
};
NpcRouteEnt *gNpc67Routes[4] = {
    gNpc67Routes_0, gNpc67Routes_1, gNpc67Routes_2, gNpc67Routes_3,
};
u32 gNpcRoute68[12] = {
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
NpcRouteEnt gNpc69Routes_0[11] = {
#include "lobby/007F8100.inc"
};
NpcRouteEnt gNpc69RoutesCond53_0[11] = {
#include "lobby/007F81E0.inc"
};
NpcRouteEnt gNpc69Routes_2[11] = {
#include "lobby/007F82C0.inc"
};
NpcRouteEnt *gNpc69Routes[4] = {
    gNpc69Routes_0, gNpc69Routes_1, gNpc69Routes_2, gNpc69Routes_3,
};
NpcRouteEnt *gNpc69RoutesCond53[4] = {
    gNpc69RoutesCond53_0, gNpc69RoutesCond53_1, gNpc69Routes_2, gNpc69Routes_3,
};
u32 gNpcRoute6A[40] = {
#include "lobby/007F83C0.inc"
};
u32 gNpcRoute6B[12] = {
#include "lobby/007F8460.inc"
};
u32 gNpcRoute6C[12] = {
#include "lobby/007F8490.inc"
};
u32 gHousePetRoute[20] = {
#include "lobby/007F84C0.inc"
};
u32 gNpcRoute6F[20] = {
#include "lobby/007F8510.inc"
};
u32 gHairdresserRoute[28] = {
#include "lobby/007F8560.inc"
};
u32 gNpcRouteToHairChair[20] = {
#include "lobby/007F85D0.inc"
};
u32 gNpcRouteFromHairChair[28] = {
#include "lobby/007F8620.inc"
};
u32 gNpcRouteKitchen_34[16] = {
#include "lobby/007F8690.inc"
};
u32 gNpcRouteKitchen[40] = {
#include "lobby/007F86D0.inc"
};
u32 gKitchenRoute0_44[16] = {
#include "lobby/007F8770.inc"
};
u32 gKitchenRoute0[52] = {
#include "lobby/007F87B0.inc"
};
u32 gKitchenRoute1[68] = {
#include "lobby/007F8880.inc"
};
u32 gKitchenRoute2[48] = {
#include "lobby/007F8990.inc"
};
u32 gNpcPlaceRoutes0_0[76] = {
#include "lobby/007F8A50.inc"
};
u32 gNpcPlaceRoutes0_1[68] = {
#include "lobby/007F8B80.inc"
};
u32 gNpcPlaceRoutes1_0[72] = {
#include "lobby/007F8C90.inc"
};
u32 gNpcPlaceRoutes1_1[68] = {
#include "lobby/007F8DB0.inc"
};
u32 gNpcPlaceRoutes1_2[68] = {
#include "lobby/007F8EC0.inc"
};
u32 gNpcPlaceRoutes1_3[60] = {
#include "lobby/007F8FD0.inc"
};
u32 gNpcPlaceRoutes1_4[76] = {
#include "lobby/007F90C0.inc"
};
u32 gNpcPlaceRoutes1_5[68] = {
#include "lobby/007F91F0.inc"
};
u32 gNpcPlaceRoutes2_0[68] = {
#include "lobby/007F9300.inc"
};
u32 gNpcPlaceRoutes2_1[68] = {
#include "lobby/007F9410.inc"
};
u32 gNpcPlaceRoutes2_6[88] = {
#include "lobby/007F9520.inc"
};
u32 gNpcPlaceRoutes2_7[96] = {
#include "lobby/007F9680.inc"
};
u32 gNpcPlaceRoutes2_8[96] = {
#include "lobby/007F9800.inc"
};
u32 gNpcPlaceRoutes2_9[96] = {
#include "lobby/007F9980.inc"
};
u32 gNpcPlaceRoutes2_10[108] = {
#include "lobby/007F9B00.inc"
};
u32 gNpcPlaceRoutes2_11[96] = {
#include "lobby/007F9CB0.inc"
};
u32 gNpcPlaceRoutes2_12[96] = {
#include "lobby/007F9E30.inc"
};
u32 gNpcPlaceRoutes2_13[88] = {
#include "lobby/007F9FB0.inc"
};
u32 gNpcPlaceRoutes2_14[88] = {
#include "lobby/007FA110.inc"
};
u32 gNpcPlaceRoutes2_15[88] = {
#include "lobby/007FA270.inc"
};
u32 gNpcPlaceRoutes3_0[56] = {
#include "lobby/007FA3D0.inc"
};
u32 gNpcPlaceRoutes0[4] = {
    (u32)gNpcPlaceRoutes0_0, (u32)gNpcPlaceRoutes0_1, 0, 0,
};
u32 gNpcPlaceRoutes1[8] = {
    (u32)gNpcPlaceRoutes1_0, (u32)gNpcPlaceRoutes1_1, (u32)gNpcPlaceRoutes1_2, (u32)gNpcPlaceRoutes1_3,
    (u32)gNpcPlaceRoutes1_4, (u32)gNpcPlaceRoutes1_5, 0, 0,
};
u32 gNpcPlaceRoutes2[16] = {
    (u32)gNpcPlaceRoutes2_0, (u32)gNpcPlaceRoutes2_1, (u32)gNpcPlaceRoutes1_2, (u32)gNpcPlaceRoutes1_3,
    (u32)gNpcPlaceRoutes1_4, (u32)gNpcPlaceRoutes1_5, (u32)gNpcPlaceRoutes2_6, (u32)gNpcPlaceRoutes2_7,
    (u32)gNpcPlaceRoutes2_8, (u32)gNpcPlaceRoutes2_9, (u32)gNpcPlaceRoutes2_10, (u32)gNpcPlaceRoutes2_11,
    (u32)gNpcPlaceRoutes2_12, (u32)gNpcPlaceRoutes2_13, (u32)gNpcPlaceRoutes2_14, (u32)gNpcPlaceRoutes2_15,
};
u32 gNpcPlaceRoutes3[4] = {
    (u32)gNpcPlaceRoutes3_0, 0, 0, 0,
};
u32 gNpcRoute6EOften[48] = {
#include "lobby/007FA530.inc"
};
u32 gNpcRoute6ERare[40] = {
#include "lobby/007FA5F0.inc"
};
u32 gNpcRoute6ELoop[60] = {
#include "lobby/007FA690.inc"
};
u32 gNpcRoute6E[20] = {
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
u32 gNpcTalkSe[116] = {
#include "lobby/007FB850.inc"
};
u32 gAreaNpcTables_0[288] = {
#include "lobby/007FBA20.inc"
};
