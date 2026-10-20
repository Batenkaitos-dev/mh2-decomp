/*
 * The field's small shared systems:
 * - the draw slot pools (DrawSlot): gDrawSlotPool, gDrawSlotsB and gHudDrawHooks, each
 *   with a first-free hint, and the ordering tables the slots go into (OT_AddByDepth
 *   sorts by view depth, OT_AddAt puts at a bucket, Task_RunList runs a list);
 * - pad vibration (Vib_*), skipped while the vibration option is off;
 * - the camera (gCamera, a Camera): eye, target, roll, FOV, the presets (CamPreset),
 *   the projection and the view matrix; the camera controller (CamCtl) hands it its view;
 * - the monster slots (gEms, EM_COUNT Ems): allocation, model set-up and freeing;
 * - five lists of EmObj that the monsters check: the area objects, sight blocks,
 *   flashes, sensed objects, and one list that is only cleared;
 * - the area state (gAreaState): entering an area, its fog, its objects (gAreaInfo,
 *   then per area), its exit spots (AreaSpot), and the per-area and per-map tables
 *   (day/night ids, temperatures, the areas of each map).
 */
#include "common.h"
#include "quest.h"
#include "sprite.h"
#include "camera.h"
#include "player.h"
#include "em.h"
#include "scene.h"
#include "syssave.h"
#include "lobby_state.h"
#include "area.h"
#include "sound.h"
#include "objattach.h"
#include "qsync.h"
#include "overlay.h"
#include "render_state.h"
#include "sound.h"



/* A vibration pattern: the two values given to Pad_SetVibration (probably the
 * pad actuators). */
typedef struct VibPattern {
    s32 a;
    s32 b;
} VibPattern;

/* A camera preset (gCameraPresets, Camera_SetPreset). */
typedef struct CamPreset {
    f32 farZ;           /* copied to Camera.farZ */
    f32 nearZ;          /* copied to Camera.nearZ */
    f32 fov;
    f32 aspect;         /* seen: the aspect the code passes; nothing reads it */
} CamPreset;

/* A spot of an area's data (0x34 bytes; Stage_GetAreaData): where the player can act;
 * the stage's own action spots, from Stage_GetActionSpots, are ActionSpot (quest.h). */
typedef struct AreaSpot {
    u16 id;             /* 0x00: 0xFFFF ends the list */
    s16 type;           /* 0x02: 0: a cylinder (radius, height), 1: Slab_Contains */
    f32 pos[3];         /* 0x04 */
    f32 radius;         /* 0x10 */
    f32 height;         /* 0x14 */
    f32 pos2[3];        /* 0x18 */
    f32 target[3];      /* 0x24: where the player is put */
    u16 angle;          /* 0x30: his angle, less 0x4000 */
    u8 unk32[2];
} AreaSpot;

/* Per area id (4 bytes): four setup ids for Area_StartObjects (0xFF: none). atmosType
 * is also a kind: 8, 0xD and 0xE get Area_MonsterState's state. */
typedef struct AreaInfo {
    u8 atmosType;       /* AreaAtmos_Spawn's type; Area_MonsterState switches on it */
    u8 lifeSet;         /* AmbientLife_Create's set */
    u8 scenerySet;      /* AnimatedScenery_Create's set */
    u8 flyerPoint;      /* AreaFlyer_Create's point; 0xFF clears AreaState.flyer */
} AreaInfo;

extern DrawSlot gHudDrawHooks[4];
extern DrawSlot gDrawSlotsB[0x100];
extern DrawSlot gDrawSlotPool[0x200];
extern s32 gDrawSlotBNext;          /* first free entry of gDrawSlotsB */
extern s32 gDrawSlotNext;          /* first free entry of gDrawSlotPool */
extern s32 gTexHandles[];
extern LobbyState gLobbyState;
extern VibPattern gVibPatterns[];
extern f32 gCamDefaultEye[];       /* the default eye, then (gCamDefaultAt) the default target */
extern f32 gCamDefaultAt[3];       /* default target */
extern f32 gCamDefaultDir[3];    /* the default view direction (Camera.unk24) */
extern CamPreset gCameraPresets[];
extern u8 gCameraPreset;           /* the current camera preset */
extern f32 gFrustumPlanes[4][4];    /* the projection matrix */
extern f32 gCamYawMtx[4][4];    /* rotation by the camera's yaw */
extern f32 gCamMatrix[];
extern f32 gViewMatrix[4][4];
extern u8 gModelUsed[];
extern EmObj *gEmSenseObjs[32];
extern EmObj *gUnusedObjs[32];
extern EmObj *gFlashObjs[32];
extern EmObj *gSightBlockObjs[32];
extern EmObj *gAreaObjList[32];
extern s8 gEmSenseObjCount;           /* entries used in gEmSenseObjs */
extern s8 gUnusedObjCount;           /* ... in gUnusedObjs (only ObjListAF0_Clear touches the two) */
extern s8 gFlashObjCount;           /* ... in gFlashObjs */
extern s8 gSightBlockObjCount;           /* ... in gSightBlockObjs */
extern s8 gAreaObjCount;           /* ... in gAreaObjList */
extern FogParam gFogDefault;     /* the default fog */
extern FogParam gFogHouse;     /* the fog of AREA_HOUSE */
extern AreaInfo gAreaInfo[];
extern s16 gAreaFade;
extern s16 gAreaFadeTimer;
extern s16 gAreaTemperatures[];
extern u16 gMapDayIds[];           /* per map: its day map, 0xFFFF: itself */
extern s16 gAreaTimeOfDay[];        /* per area id: its time of day (AREA_TIME_*) */
extern u16 gAreaOtherTime[];        /* per area id: the other time's id, 0xFFFF: none */
extern u16 gMapAreaCount[];         /* per map: the number of its areas */
extern u16 *gMapAreas[];            /* per map: its area ids, in order */

void *memset(void *p, s32 c, u32 n);
void *memcpy(void *dst, void *src, u32 n);
s32 Gs_SetState(u8 reg, u32 v);
s32 Pad_SetVibration(s32 port, s32 level, s32 on);
s32 Overlay_IsLoaded(s32 a);
u8 Daytime_Get(void);
void Camera_SetPerspective(s32 a, f32 b, f32 c, f32 d, f32 e);
void Mtx_Identity(f32 *m);
void Mtx_SetRotXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_LookAt(f32 *m, f32 *eye, f32 *at, f32 *up);
void Mtx_Inverse(f32 *dst, f32 *src);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void Vec4_Transform(f32 *out, f32 *v, f32 *m);
f32 Vec_Distance(f32 *a, f32 *b);
void Vec_Copy(f32 *dst, f32 *src);
struct MtxSlot;
void MtxSlot_Load(struct MtxSlot *m, s32 i);
f32 Math_Atan2(f32 y, f32 x);
f32 Math_Sqrtf(f32 x);
void Quat_ToMatrix(f32 *q, f32 (*m)[4]);
void Quat_Set(f32 *axis, f32 *q, f32 angle);
struct EmBone;
void Bone_InitTreeRoot(struct EmBone *bone, s32 a);
void Frustum_BuildPlanes(f32 *planes, f32 fov, f32 aspect, f32 near, f32 far);
void TagList_Push(u32 *node, u32 *link);
void *TagList_Pop(u32 *p);
u16 Mtx_Heading(f32 *m);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
void Model_FreeCopy(s32 i);
s32 Bone_Alloc(s32 n);
struct EmBone *Bone_Get(s32 i);
void Bone_Reserve(s32 i, s32 n);
s32 Model_Alloc(s32 n);
Model *Model_Get(s32 i);
void Model_Reserve(s32 i, s32 a);
void Model_Release(s32 i, s32 a);
void AreaProp_Spawn(s16 i);
void SoundObj_Start(s32 bank, s32 se, s32 k, f32 *pos, s16 n, u8 quiet);
void AreaAtmos_Spawn(s32 a);
void Player_AddSpawnOffset(Player *p, f32 *pos, u16 angle);
void *Stage_GetAreaData(s32 area);
/* unit's own declaration: the area as u32; with the definition's u16, Area_Enter compiles differently from the game's code */
AreaRecord *Stage_GetAreaRecord(u32 id);
s32 Quest_GetOutcome(s32 withLoss);
s32 Quest_GetTime(s32 a);
s32 Player_IsLocal(void *obj);
u8 Village_FacilityLevel(s32 i);
/* unit's own declaration: the result as s32; with the definition's u8, Player_CheckActionSpot compiles differently from the game's code */
s32 Slab_Contains(f32 *pos, f32 *a, f32 *b, f32 height, f32 radius);
void Player_UpdateWorldMatrix(Player *pl);
void BirdFlock_Spawn(s8 type);
void PlacedItem_Create(f32 *pos, u8 kind, u16 area, Player *pl);
void LightShaft_Create(void);
void SkyStreak_Create(u8 a);
void AreaDrop_Create(void);
void AreaFlag_Create(void);
void AreaWeapon_Create(u8 a);
void AnimatedProps_Create(void);
void AnimatedGroundTiles_Create(void);
void AmbientLife_Create(u8 a);
void FortressTremor_Create(void);
void BreakableStructure_Create(void);
void ScrollingOverlay_Create(void);
void TurningProp_Create(void);
void GroundTileGrid_Create(void);
void AnimatedScenery_Create(u8 a);
void CastleGate_Create(u8 kind);
void DistantLightning_Create(void);
void AreaObj_K19_Create(void);
u8 EmObj_AlertMonsters(EmObj *obj);
s32 Town_IsIdleOnline(void);
u8 MyHouse_Level(void);
s32 LobbyTime_IsAm(void);
s32 LobbyWorld_GetSeason(void);
void LobbyFlock_Spawn(void);
void FireBlast_CreateAt(f32 *pos, s16 k);
void AreaFlyer_Create(u8 a);
void QSync_SendPlayer(Player *p, s32 kind, u16 mode);

void OT_InsertSorted(DrawSlot *node, u32 *link);
void DrawPool_Clear(DrawSlot *p, s32 n);
DrawSlot *DrawSlot_Get(s32 i);
DrawSlot *DrawSlotB_Get(s32 i);
void DrawSlot_Free(s32 i);
void Vib_Stop(s32 port);
void Camera_Reset(void);
void Camera_ApplyProjection(void);
void Camera_ComputeUp(f32 *up, f32 *eye, f32 *at, f32 roll);
void EmSlot_Free(Em *e);
void SightBlock_Remove(EmObj *obj);
void FlashObj_Remove(EmObj *obj);
void Area_ResetFade(void);
s16 Area_MonsterState(u16 area);
u16 Area_OtherTimeId(u16 area);
u16 Area_IdForCurrentTime(u16 area);

/* Runs Player_UpdateWorldMatrix on the active players (10 slots from gPlayers). */
void Players_UpdateMatrices(void)
{
    Player *pl = gPlayers;
    s32 i;

    for (i = 0; i < PLAYER_SLOTS; i++, pl++) {
        /* the lobby is loaded: its town players are drawn once their model is (pcLoaded, read as s8: the game's code loads it signed) */
        if ((Overlay_IsLoaded(OVERLAY_LOBBY) != 1 || (s8)gLobbyState.pcLoaded[pl->index] == 1) && pl->active != 0) {
            Player_UpdateWorldMatrix(pl);
        }
    }
}

/* Counts the nonzero entries of gTexHandles[start .. start + 29]. */
s32 Tex_CountLoaded(s32 start)
{
    s32 i;
    s32 count;
    s32 *handle;

    for (i = 0, count = 0, handle = &gTexHandles[start]; i < 30; i++, handle++) {
        if (*handle != 0) {
            count++;
        }
    }
    return count;
}

/* Hands the render states the materials of a set: its material i goes to RS_MATERIAL(i) (its ModelMat in mats). */
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set)
{
    s32 i;

    for (i = 0; i < set->count; i++) {
        Gs_SetState(RS_MATERIAL(i), (u32)&mats[set->mat[i]]);
    }
}

/* Inserts node into the list at link, before the first node whose key is
 * not greater (the list is kept in decreasing depth). */
void OT_InsertSorted(DrawSlot *node, u32 *link)
{
    u32 next;
    DrawSlot *nextSlot;

    for (;;) {
        next = *link;
        if (next == 0 || !(next & 1)) {
            *link = (u32)node | 1;
            node->next = next;
            return;
        }
        nextSlot = (DrawSlot *)(next & ~1);
        if (nextSlot->key <= node->key) {
            *link = (u32)node | 1;
            node->next = next;
            return;
        }
        link = &nextSlot->next;
    }
}

/* Clears count draw slots, from slot on. */
void DrawPool_Clear(DrawSlot *slot, s32 count)
{
    s32 i;

    for (i = 0; i < count; i++, slot++) {
        memset(slot, 0, sizeof(DrawSlot));
    }
}

void DrawSlot_ResetAll(void)
{
    DrawPool_Clear(gDrawSlotPool, 0x200);
    gDrawSlotNext = 0;
}

void DrawSlotB_ResetAll(void)
{
    DrawPool_Clear(gDrawSlotsB, 0x100);
    gDrawSlotBNext = 0;
}

void DrawPool_ClearHud(void)
{
    DrawPool_Clear(gHudDrawHooks, 4);
}

/* The address of entry index of gDrawSlotPool; NULL for a negative index. */
DrawSlot *DrawSlot_Get(s32 index)
{
    DrawSlot *slot = NULL;

    if (index >= 0) {
        slot = &gDrawSlotPool[index];
    }
    return slot;
}

/* The address of entry index of gDrawSlotsB; NULL for a negative index. */
DrawSlot *DrawSlotB_Get(s32 index)
{
    DrawSlot *slot = NULL;

    if (index >= 0) {
        slot = &gDrawSlotsB[index];
    }
    return slot;
}

/* Takes the first free entry of gDrawSlotPool and moves the hint to the next
 * free one; -1 when full. */
s16 DrawSlot_Alloc(void)
{
    s32 index = gDrawSlotNext;
    DrawSlot *slot;
    s32 j;

    if (index >= 0x200) {
        return -1;
    }
    slot = DrawSlot_Get(index + 1);
    for (j = gDrawSlotNext + 1; j < 0x200; j++, slot++) {
        if (slot->draw == 0) {
            break;
        }
    }
    gDrawSlotNext = j;
    slot = DrawSlot_Get(index);
    slot->draw = (DrawSlotFn) 1;
    return index;
}

/* As DrawSlot_Alloc for gDrawSlotsB, but the entry is not marked used. */
s16 DrawSlotB_Alloc(void)
{
    s32 index = gDrawSlotBNext;
    DrawSlot *slot;
    s32 j;

    if (index >= 0x100) {
        return -1;
    }
    slot = DrawSlotB_Get(index + 1);
    for (j = gDrawSlotBNext + 1; j < 0x100; j++, slot++) {
        if (slot->draw == 0) {
            break;
        }
    }
    gDrawSlotBNext = j;
    return index;
}

/* Clears entry index of gDrawSlotPool; the first-free hint moves back to it when it is lower. */
void DrawSlot_Free(s32 index)
{
    memset(DrawSlot_Get(index), 0, sizeof(DrawSlot));
    if (index < gDrawSlotNext) {
        gDrawSlotNext = index;
    }
}

/* As DrawSlot_Free for gDrawSlotsB, but the entry keeps its owner and part. */
void DrawSlotB_Free(s32 index)
{
    DrawSlot *slot = DrawSlotB_Get(index);
    void *owner = slot->owner;
    s32 part = slot->part;

    memset(slot, 0, sizeof(DrawSlot));
    slot->owner = owner;
    slot->part = part;
    if (index < gDrawSlotBNext) {
        gDrawSlotBNext = index;
    }
}

/*
 * Adds node to an ordering table of n buckets by its view depth (matrix 0x21):
 * nearer nodes go to higher buckets. A node behind the camera is put at depth
 * 0, or dropped (-1) when more than 1600 behind and clamp is 0. Returns the
 * bucket. 64-bucket tables are kept sorted within a bucket.
 */
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 bucketCount, s32 clamp)
{
    f32 worldPos[4];
    f32 viewPos[4];
    f32 viewMtx[4][4];
    u32 bucket;
    f32 *depth;

    worldPos[0] = node->pos[0];
    worldPos[1] = node->pos[1];
    worldPos[2] = node->pos[2];
    worldPos[3] = 1.0f;
    MtxSlot_Load((struct MtxSlot *) viewMtx, 0x21);
    Vec4_Transform((f32 *) viewPos, (f32 *) worldPos, (f32 *) viewMtx);
    depth = &viewPos[2];
    *depth *= -1.0f;
    if (*depth < 0.0f) {
        if (clamp != 0) {
            *depth = 0.0f;
        } else if (*depth < -1600.0f) {
            return -1;
        } else {
            *depth = 0.0f;
        }
    }
    node->key = *depth;
    bucket = *depth / 65000.0f * bucketCount;
    if (bucket >= bucketCount) {
        return -1;
    }
    if (bucketCount == 0x40) {
        OT_InsertSorted(node, &table[bucketCount - 1 - bucket]);
    } else {
        TagList_Push((u32 *) node, (u32 *) (&table[bucketCount - 1 - bucket]));
    }
    return bucket;
}

/* Pushes node onto bucket `bucket` of an ordering table of bucketCount buckets (table entry
 * bucketCount - 1 - bucket). Returns the bucket, or -1 when it is out of range. */
s32 OT_AddAt(u32 *table, void *node, s32 bucket, s32 bucketCount)
{
    if (bucket >= bucketCount || bucket < 0) {
        return -1;
    }
    TagList_Push(node, &table[bucketCount - 1 - bucket]);
    return bucket;
}

/* Runs the draw callback of every node of an ordering table or draw list
 * (its head word, then each node: TagList_Pop walks the tag words). The
 * cursor is the parameter itself: the game keeps it in its stack slot. */
void Task_RunList(u32 *list)
{
    for (;;) {
        list = TagList_Pop((u32 *)&list);
        if (list == NULL) {
            break;
        }
        ((DrawSlot *)list)->draw((DrawSlot *)list);
    }
}

/* 0xRRGGBB to three floats in 0..1. */
void Color_RgbToFloat(f32 *rgb, u32 color)
{
    rgb[0] = 0.003921569f * ((color >> 16) & 0xFF);
    rgb[1] = 0.003921569f * ((color >> 8) & 0xFF);
    rgb[2] = 0.003921569f * (color & 0xFF);
}

/* 0xAARRGGBB to r, g, b, a floats in 0..1. */
void Color_ArgbToFloat(f32 *rgba, u32 color)
{
    rgba[3] = 0.003921569f * ((color >> 24) & 0xFF);
    rgba[0] = 0.003921569f * ((color >> 16) & 0xFF);
    rgba[1] = 0.003921569f * ((color >> 8) & 0xFF);
    rgba[2] = 0.003921569f * (color & 0xFF);
}

/* Static, as the match needs: its callers then keep their first argument in its register
 * across the call, as the game's code does. 1 while the vibration option is off. */
static s32 Vib_IsOff(void)
{
    return gSysSave.options.vibration == 0;
}

/* Starts vibration pattern i on a pad port, unless vibration is off. */
void Vib_Start(s32 port, s32 pattern)
{
    if (!Vib_IsOff()) {
        Pad_SetVibration(port, gVibPatterns[pattern].a, gVibPatterns[pattern].b);
    }
}

/* The same on port 0, when Player_IsLocal allows it. */
void Vib_StartLocal(void *player, s32 pattern)
{
    if (!Vib_IsOff() && Player_IsLocal(player) != 0) {
        Pad_SetVibration(0, gVibPatterns[pattern].a, gVibPatterns[pattern].b);
    }
}

void Vib_Stop(s32 port)
{
    Pad_SetVibration(port, 0, 0);
}

/* Stops the vibration of both pad ports. */
void Vib_StopAll(void)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        Vib_Stop(i);
    }
}

/* Initialises the camera: a 512 x 448 viewport, then the defaults. */
void Camera_Init(void)
{
    gCamera = &gCameraData;
    gCamera->viewport[0] = 0;
    gCamera->viewport[1] = 0;
    gCamera->viewport[2] = 0x200;
    gCamera->viewport[3] = 0x1C0;
    gCamera->viewport[4] = 0;
    gCamera->viewportFarZ = 1.0f;
    Gs_SetState(RS_ORTHO, (u32)gCamera->viewport);
    Camera_Reset();
}

/* Default eye, target and direction, a FOV of 50 degrees, no roll. */
void Camera_Reset(void)
{
    Vec_Copy(gCamera->eye, gCamDefaultEye);
    Vec_Copy(gCamera->at, gCamDefaultAt);
    Vec_Copy(gCamera->viewDir, gCamDefaultDir);
    gCamera->fov = 0.87266463f;
    gCamera->fovPrev = 0.87266463f;
    Camera_ApplyProjection();
    gCamera->roll = 0.0f;
}

/*
 * The camera update: the view direction, the projection again when the FOV
 * changed, the up vector from the roll, the view matrix (render state 0x16),
 * and gCamYawMtx, a rotation by the camera's yaw (+ 90 degrees).
 */
void Camera_Update(void)
{
    f32 dir[4];
    f32 viewMtx[4][4];
    s32 changed = 0;
    f32 angle;
    f32 *fov;

    Vec_Diff(dir, gCamera->at, gCamera->eye);
    if (dir[0] != 0.0f || dir[2] != 0.0f) {
        Vec_Copy(gCamera->viewDir, dir);
    } else {
        Vec_Add(gCamera->at, gCamera->eye, gCamera->viewDir);
    }
    fov = &gCamera->fovPrev;
    if (*fov != gCamera->fov) {
        *fov = gCamera->fov;
        changed++;
    }
    if (changed) {
        Camera_ApplyProjection();
    }
    Camera_ComputeUp(gCamera->up, gCamera->eye, gCamera->at, gCamera->roll);
    Mtx_LookAt((f32 *) viewMtx, (f32 *) gCamera->eye, (f32 *) gCamera->at, (f32 *) gCamera->up);
    Gs_SetState(RS_VIEW_MTX, (u32)viewMtx);
    MtxSlot_Load((struct MtxSlot *) gViewMatrix, 0x21);
    Mtx_Inverse(gCamMatrix, (f32 *) gViewMatrix);
    angle = 6.2831855f * (s32)(u16)(Mtx_Heading(gCamMatrix) + 0x4000) / 65536.0f;
    Mtx_Identity((f32 *) gCamYawMtx);
    Mtx_SetRotXYZ((f32 *) gCamYawMtx, 0.0f, angle, 0.0f);
}

/* Selects camera preset id (0xFF: only reapplies the projection). */
void Camera_SetPreset(s32 id)
{
    CamPreset *preset;

    if (id != 0xFF) {
        if (gCameraPreset == id) {
            return;
        }
        gCameraPreset = id;
        preset = &gCameraPresets[id];
        gCamera->farZ = preset->farZ;
        gCamera->nearZ = preset->nearZ;
        gCamera->fov = gCamera->fovPrev = preset->fov;
        gCamera->roll = 0.0f;
    }
    Camera_ApplyProjection();
}

void Camera_BuildFrustum(f32 farZ)
{
    Frustum_BuildPlanes((f32 *) gFrustumPlanes, gCamera->fov, 1.4285715f, gCamera->nearZ, farZ);
}

/* Applies the projection (aspect 640/448), the far value 3000. */
void Camera_ApplyProjection(void)
{
    Camera_SetPerspective(0x20, gCamera->farZ, gCamera->nearZ, gCamera->fov, 1.4285715f);
    Frustum_BuildPlanes((f32 *) gFrustumPlanes, gCamera->fov, 1.4285715f, gCamera->nearZ, 3000.0f);
}

void Camera_SetFov(f32 fov)
{
    gCamera->fov = fov;
    Camera_ApplyProjection();
}

void Camera_RestoreFov(void)
{
    gCamera->fov = gCamera->fovPrev;
    Camera_ApplyProjection();
}

void Camera_SetRoll(f32 roll)
{
    gCamera->roll = roll;
}

void Camera_StoreFov(f32 fov)
{
    gCamera->fov = fov;
}

f32 Camera_Distance(void *pos)
{
    return Vec_Distance((f32 *) pos, (f32 *) gCamera);
}

/* The camera's yaw as a 16-bit angle. */
u16 Camera_GetYaw(void)
{
    return (s32)(0.5f + 65536.0f * Math_Atan2(-(gCamera->at[2] - gCamera->eye[2]),
                                                 gCamera->at[0] - gCamera->eye[0]) / 6.2831855f);
}

/* up = (0, 1, 0) rotated by roll around the view direction. */
void Camera_ComputeUp(f32 *up, f32 *eye, f32 *at, f32 roll)
{
    f32 vec[4];
    f32 rot[4][4];
    f32 quat[4];

    Vec_Diff(vec, at, eye);
    Quat_Set(vec, quat, roll);
    Quat_ToMatrix(quat, rot);
    vec[0] = 0.0f;
    vec[1] = 1.0f;
    vec[2] = 0.0f;
    Vec_Rotate((f32 *) up, (f32 *) vec, (f32 *) rot);
}

/* Clears the monster slots (only their first 0x970 bytes, a Player's size). */
void EmSlot_ClearAll(void)
{
    s16 i;
    Em *em = gEms;

    for (i = 0; i < EM_COUNT; i++, em++) {
        memset(em, 0, sizeof(Player));
    }
}

/* Frees a monster slot: its pool entry, its model slots, and its first 0x12
 * bytes. */
void EmSlot_Free(Em *em)
{
    s16 i;

    if (em->drawSlot != 0) {
        DrawSlot_Free(em->drawSlotId);
    }
    if (em->model != NULL) {
        for (i = em->modelFirst; i < em->modelFirst + em->modelCount; i++) {
            Model_FreeCopy(i);
            if (gModelUsed[i] != 0) {
                Model_Release(i, 1);
            }
            em->model = NULL;
            em->modelFirst = 0;
            em->modelCount = 0;
        }
    }
    memset(em, 0, 0x12);
}

/* Frees every monster slot in use (EmSlot_Free). */
void EmSlot_FreeAll(void)
{
    s16 i;
    Em *em = gEms;

    for (i = 0; i < EM_COUNT; i++, em++) {
        if (em->active != 0) {
            EmSlot_Free(em);
        }
    }
}

/* Gives a monster a model: a model slot, a copy of the scene's model header
 * e->modelId, and two sets of bone slots. */
void EmSlot_SetupModel(Em *em)
{
    s32 j;
    s32 bone;
    Model *emModel;
    s32 modelIndex;
    SceneState *scene = &gScene;
    s32 firstBone;

    modelIndex = Model_Alloc(1);
    if (modelIndex >= 0) {
        em->modelFirst = modelIndex;
        em->modelCount = 1;
        em->model = Model_Get(modelIndex);
        Model_Reserve(modelIndex, 1);
        emModel = Model_Get(modelIndex);
        memcpy(emModel, scene->models[em->modelSlot], sizeof(Model));
        emModel->active = 1;
        emModel->boneKind = 2;
        emModel->isCopy = 1;
        firstBone = Bone_Alloc(emModel->boneCount);
        emModel->boneFirst = firstBone;
        emModel->bones = Bone_Get(firstBone);
        emModel->bones2 = Bone_Get(firstBone + emModel->boneCount / 2);
        Bone_Reserve(firstBone, emModel->boneCount);
        for (bone = 0, j = 0; j < emModel->partCount; j++) {
            Bone_InitTreeRoot(&emModel->bones[bone], emModel->partSkels[j]);
            Bone_InitTreeRoot(&emModel->bones2[bone], emModel->partSkels[j]);
            emModel->partBones[j] = &emModel->bones[bone];
            emModel->partBones2[j] = &emModel->bones2[bone];
            bone += emModel->bones->count;
        }
    }
}

/*
 * Takes a free monster slot and initialises it. The original places the
 * last two stores after the loop and jumps back to the return; the gotos
 * reproduce that block order.
 */
Em *EmSlot_Alloc(void)
{
    u32 i;
    Em *em = gEms;

    for (i = 0; i < EM_COUNT; i++, em++) {
        if (em->active == 0) {
            memset(em, 0, sizeof(Em));
            em->slotNo = i;
            em->active = 1;
            em->isEm = 1;
            em->mineSlot = -1;
            em->index = i;
            em->model = NULL;
            em->remote = gScene.localPlayer;
            em->alpha = 1.0f;
            em->partMask = 0;
            goto init;
        done:
            return em;
        }
    }
    return NULL;
init:
    em->hitIds[i] = 0;
    em->hitIdNext = 0;
    goto done;
}

/*
 * Five lists of 32 EmObj pointers, each with a count of the entries used:
 * gAreaObjList (the monsters' goal search: baits and the like), gSightBlockObjs
 * (zones that block a monster's sight), gFlashObjs (flashes), gEmSenseObjs (the
 * objects monsters sense, Em_SenseObjects) and gUnusedObjs (only cleared). Add
 * takes the first empty entry and returns 1, or 0 when the list is full; Remove
 * empties every entry holding the object; the two Tick functions count each
 * object's timer down once a frame (Field_UpdateAll) and remove it at 0.
 */
/* Empties gAreaObjList (despite its name, it only clears). */
void ObjList_Update(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        gAreaObjList[i] = NULL;
    }
    gAreaObjCount = 0;
}

/* Adds obj to gAreaObjList; 0 when the list is full. */
s32 ObjList_Add(EmObj *obj)
{
    s32 i;

    if (gAreaObjCount >= 32) {
        return 0;
    }
    for (i = 0; i < 32; i++) {
        if (gAreaObjList[i] == NULL) {
            gAreaObjList[i] = obj;
            gAreaObjCount++;
            return 1;
        }
    }
    return 0;
}

/* Removes obj from gAreaObjList. */
void ObjList_Remove(EmObj *obj)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        if (gAreaObjList[i] == obj) {
            gAreaObjList[i] = NULL;
            gAreaObjCount--;
        }
    }
}

/* Empties gSightBlockObjs. */
void SightBlock_ClearAll(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        gSightBlockObjs[i] = NULL;
    }
    gSightBlockObjCount = 0;
}

/* Adds obj to gSightBlockObjs; 0 when the list is full. */
s32 SightBlock_Add(EmObj *obj)
{
    s32 i;

    if (gSightBlockObjCount >= 32) {
        return 0;
    }
    for (i = 0; i < 32; i++) {
        if (gSightBlockObjs[i] == NULL) {
            gSightBlockObjs[i] = obj;
            gSightBlockObjCount++;
            return 1;
        }
    }
    return 0;
}

/* Removes obj from gSightBlockObjs. */
void SightBlock_Remove(EmObj *obj)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        if (gSightBlockObjs[i] == obj) {
            gSightBlockObjs[i] = NULL;
            gSightBlockObjCount--;
        }
    }
}

/* Counts down the timer of each object of gSightBlockObjs and removes those that reach 0. */
void SightBlock_Tick(void)
{
    s32 i;
    EmObj *obj;

    for (i = 0; i < 32; i++) {
        obj = gSightBlockObjs[i];
        if (obj != NULL && --obj->timer <= 0) {
            SightBlock_Remove(obj);
        }
    }
}

/* Empties gFlashObjs. */
void FlashObj_ClearAll(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        gFlashObjs[i] = NULL;
    }
    gFlashObjCount = 0;
}

/* Adds obj to gFlashObjs, first alerting the monsters to it (EmObj_AlertMonsters); 0 when the list is full. */
s32 FlashObj_Add(EmObj *obj)
{
    s32 i;

    if (gFlashObjCount >= 32) {
        return 0;
    }
    for (i = 0; i < 32; i++) {
        if (gFlashObjs[i] == NULL) {
            EmObj_AlertMonsters((EmObj *)obj);
            gFlashObjs[i] = obj;
            gFlashObjCount++;
            return 1;
        }
    }
    return 0;
}

/* Removes obj from gFlashObjs. */
void FlashObj_Remove(EmObj *obj)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        if (gFlashObjs[i] == obj) {
            gFlashObjs[i] = NULL;
            gFlashObjCount--;
        }
    }
}

/* Counts down the timer of each object of gFlashObjs and removes those that reach 0. */
void FlashObj_Tick(void)
{
    s32 i;
    EmObj *obj;

    for (i = 0; i < 32; i++) {
        obj = gFlashObjs[i];
        if (obj != NULL && --obj->timer <= 0) {
            FlashObj_Remove(obj);
        }
    }
}

/* Empties gUnusedObjs (seen: no other code reads or writes that list). */
void ObjListAF0_Clear(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        gUnusedObjs[i] = NULL;
    }
    gUnusedObjCount = 0;
}

/* Empties gEmSenseObjs. */
void ObjListA70_Clear(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        gEmSenseObjs[i] = NULL;
    }
    gEmSenseObjCount = 0;
}

/* Adds obj to gEmSenseObjs; 0 when the list is full. */
s32 ObjListA70_Add(EmObj *obj)
{
    s32 i;

    if (gEmSenseObjCount >= 32) {
        return 0;
    }
    for (i = 0; i < 32; i++) {
        if (gEmSenseObjs[i] == NULL) {
            gEmSenseObjs[i] = obj;
            gEmSenseObjCount++;
            return 1;
        }
    }
    return 0;
}

/* Removes obj from gEmSenseObjs. */
void ObjListA70_Remove(EmObj *obj)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        if (gEmSenseObjs[i] == obj) {
            gEmSenseObjs[i] = NULL;
            gEmSenseObjCount--;
        }
    }
}

/*
 * Enters an area: resets the area state and fetches its stage record. Online,
 * look[0] and look[1] (areas 0x83, 0xAD-0xAF) come from the lobby
 * overlay; AREA_HUB_0x89 takes look[1..3] from Village_FacilityLevel.
 * The first stores address gAreaState directly, the later ones through q:
 * that is what the original does.
 */
void Area_Enter(u16 area)
{
    AreaState *state = &gAreaState;
    s16 i;

    gAreaState.unk007 = 0;
    gAreaState.unk006 = 0;
    gAreaState.unk005 = 0;
    gAreaState.timerState = 0;
    gAreaState.active = 1;
    gAreaState.pos[2] = 0;
    gAreaState.pos[1] = 0;
    gAreaState.pos[0] = 0;
    gAreaState.rot[2] = 0;
    gAreaState.rot[1] = 0;
    gAreaState.rot[0] = 0;
    gAreaState.unk01C[2] = 0;
    gAreaState.unk01C[1] = 0;
    gAreaState.unk01C[0] = 0;
    gAreaState.area = area;
    gAreaState.record = Stage_GetAreaRecord(area);
    gAreaState.monsterState = 0;
    gAreaState.flyer = 0;
    gAreaState.propTimer = 0;
    gAreaState.propHold = 0;
    gAreaState.propState = 0;
    gAreaState.look[0] = 0;
    gAreaState.look[1] = 0;
    gAreaState.look[2] = 0;
    gAreaState.look[3] = 0;
    if (gScene.inLobby != 0) {
        if (LobbyTime_IsAm() != 0) {
            state->look[0] = 0;
        } else {
            state->look[0] = 0xFF;
        }
    }
    switch (area) {
    case AREA_HUB_0x83:
        if (gScene.inLobby != 0) {
            switch (LobbyWorld_GetSeason()) {
            default:
            case 2:
                state->look[1] = 2;
                break;
            case 0:
                state->look[1] = 0;
                break;
            case 1:
                state->look[1] = 1;
                break;
            }
        } else {
            state->look[1] = 2;
        }
        break;
    case AREA_HUB_0x89:
        for (i = 1; i < 4; i++) {
            state->look[i] = Village_FacilityLevel(i);
        }
        break;
    case AREA_ONLINE_HOUSE_0:
    case AREA_ONLINE_HOUSE_1:
    case AREA_ONLINE_HOUSE_2:
        if (gScene.inLobby != 0) {
            state->look[1] = MyHouse_Level();
        } else {
            state->look[1] = Village_FacilityLevel(5);
        }
        break;
    }
}

/* Sets the fog render states: the area's own fog, or a default. */
void Area_ApplyFog(u16 area)
{
    AreaState *state = &gAreaState;
    FogParam *fogParams;

    switch (area) {
    case 0:
        fogParams = &gFogDefault;
        break;
    case AREA_HOUSE:
        fogParams = &gFogHouse;
        break;
    default:
        if (state->envBlocks > 0) {
            fogParams = &state->fog;
        } else {
            fogParams = &gFogDefault;
        }
        break;
    }
    Gs_SetState(RS_FOG_COLOR, ((fogParams->a << 24) | (fogParams->r << 16) | (fogParams->g << 8) | fogParams->b));
    Gs_SetState(RS_FOG_NEAR, *(u32 *)&fogParams->nearDist);
    Gs_SetState(RS_FOG_FAR, *(u32 *)&fogParams->farDist);
}

/* Online only: AreaProp_Spawn(0 .. n - 1). */
void AreaProp_SpawnInLobby(s16 count)
{
    s16 i;

    if (gScene.inLobby != 0) {
        for (i = 0; i < count; i++) {
            AreaProp_Spawn(i);
        }
    }
}

/*
 * Sets up an area: resets the fog/colour block of the area state, runs the
 * four per-area setups of gAreaInfo, then starts the area's objects (game
 * overlay functions).
 */
void Area_StartObjects(u16 area)
{
    f32 pos[3];
    AreaState *state = &gAreaState;
    s16 i;

    if (gAreaState.envBlocks < 2) {
        state->skyPos[0] = 5000.0f;
        state->skyPos[1] = 5000.0f;
        state->skyPos[2] = 5000.0f;
        state->band.y = 0;
        state->band.h = 0;
        state->band.c[0] = state->band.c[1] = state->band.c[2] = state->band.c[3] = state->band.c[4] = state->band.c[5] =
            state->band.c[6] = state->band.c[7] = 0xFFFFFF;
    }
    Area_ResetFade();
    if (gAreaInfo[area].atmosType != 0xFF) {
        AreaAtmos_Spawn(gAreaInfo[area].atmosType);
    }
    if (gAreaInfo[area].lifeSet != 0xFF) {
        AmbientLife_Create(gAreaInfo[area].lifeSet);
    }
    if (gAreaInfo[area].scenerySet != 0xFF) {
        AnimatedScenery_Create(gAreaInfo[area].scenerySet);
    }
    if (gAreaInfo[area].flyerPoint != 0xFF) {
        AreaFlyer_Create(gAreaInfo[area].flyerPoint);
    } else {
        state->flyer = 0;
    }
    switch (area) {
    case AREA_FORTRESS_0:
    case AREA_FORTRESS_NIGHT_0:
        FortressTremor_Create();
        break;
    case AREA_FORTRESS_4:
        AreaWeapon_Create(0);
        break;
    case AREA_FORTRESS_5:
        AreaWeapon_Create(1);
        break;
    case AREA_FORTRESS_NIGHT_5:
        AreaWeapon_Create(6);
        break;
    case AREA_FORTRESS_1:
    case AREA_FORTRESS_NIGHT_1:
        AreaFlag_Create();
        break;
    case 0x11:
        for (i = 0; i < 10; i++) {
            pos[0] = 1400.0f + 300.0f * i;
            pos[1] = 30.0f;
            pos[2] = 2000.0f + 300.0f * i;
            PlacedItem_Create(pos, PLACED_LARGE_BARREL_BOMB, area, 0);
        }
        break;
    case AREA_CASTLE_0:
        CastleGate_Create(0);
        break;
    case AREA_FOREST_HILLS_0:
    case AREA_FOREST_HILLS_NIGHT_0:
        pos[0] = 11250.0f;
        pos[1] = 40.0f;
        pos[2] = 9500.0f;
        FireBlast_CreateAt(pos, 2);
        FireBlast_CreateAt(pos, 3);
        break;
    case AREA_CASTLE_1:
        AreaWeapon_Create(2);
        CastleGate_Create(1);
        DistantLightning_Create();
        break;
    case AREA_FORTRESS_3:
    case AREA_FORTRESS_NIGHT_3:
        AreaWeapon_Create(3);
        AreaFlag_Create();
        BreakableStructure_Create();
        break;
    case AREA_FORTRESS_2:
        AreaWeapon_Create(4);
        break;
    case AREA_FORTRESS_NIGHT_2:
        AreaWeapon_Create(7);
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        LightShaft_Create();
        break;
    case AREA_FOREST_HILLS_11:
    case AREA_FOREST_HILLS_NIGHT_11:
        LightShaft_Create();
        AnimatedProps_Create();
        ScrollingOverlay_Create();
        break;
    case AREA_FOREST_HILLS_12:
    case AREA_FOREST_HILLS_NIGHT_12:
        LightShaft_Create();
        break;
    case AREA_DESERT_NIGHT_7:
        ScrollingOverlay_Create();
        break;
    case AREA_SWAMP_NIGHT_7:
    case AREA_SWAMP_NIGHT_9:
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        pos[2] = 0.0f;
        SoundObj_Start(SE_SLOT_SCENE, 0x51, 0x11, pos, 0x3C, 3);
        break;
    case AREA_FORTRESS_NIGHT_4:
        AreaWeapon_Create(5);
        break;
    case AREA_HOUSE:
        break;
    case AREA_ARENAS_NIGHT_0:
    case AREA_ARENAS_0:
        AreaFlag_Create();
        break;
    case AREA_GREAT_ARENA_NIGHT_1:
    case AREA_GREAT_ARENA_1:
        ScrollingOverlay_Create();
        AreaFlag_Create();
        break;
    case AREA_ARENA_NIGHT_1:
    case AREA_ARENA_1:
        AreaFlag_Create();
        break;
    case AREA_SNOWY_MOUNTAINS_1:
    case AREA_SNOWY_MOUNTAINS_NIGHT_1:
        LightShaft_Create();
        break;
    case AREA_SNOWY_MOUNTAINS_3:
    case AREA_SNOWY_MOUNTAINS_NIGHT_3:
        LightShaft_Create();
        break;
    case AREA_SNOWY_MOUNTAINS_6:
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
        AnimatedProps_Create();
        AreaFlag_Create();
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_5:
    case AREA_SNOWY_MOUNTAINS_NIGHT_8:
        AnimatedProps_Create();
        break;
    case AREA_JUNGLE_NIGHT_0:
    case AREA_JUNGLE_0:
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        pos[2] = 0.0f;
        SoundObj_Start(SE_SLOT_SCENE, 0x48, 0x11, pos, 0xB4, 1);
        break;
    case AREA_JUNGLE_NIGHT_1:
    case AREA_JUNGLE_1:
        GroundTileGrid_Create();
        break;
    case AREA_JUNGLE_NIGHT_3:
    case AREA_JUNGLE_3:
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        pos[2] = 0.0f;
        SoundObj_Start(SE_SLOT_SCENE, 0x48, 0x11, pos, 0xB4, 1);
        break;
    case AREA_JUNGLE_NIGHT_4:
    case AREA_JUNGLE_4:
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        pos[2] = 0.0f;
        SoundObj_Start(SE_SLOT_SCENE, 0x48, 0x11, pos, 0xB4, 1);
        break;
    case AREA_JUNGLE_NIGHT_6:
    case AREA_JUNGLE_6:
        GroundTileGrid_Create();
        break;
    case AREA_JUNGLE_10:
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        pos[2] = 0.0f;
        SoundObj_Start(SE_SLOT_SCENE, 0x48, 0x11, pos, 0xB4, 1);
        break;
    case AREA_TOWER_8:
        LightShaft_Create();
        AreaDrop_Create();
        break;
    case AREA_HUB_0x83:
        if (gScene.inLobby != 0) {
            AreaProp_SpawnInLobby(7);
            if (state->look[1] != 0) {
                LobbyFlock_Spawn();
            }
            BirdFlock_Spawn(0);
        }
        break;
    case AREA_HUB_0x84:
        AreaProp_SpawnInLobby(6);
        break;
    case AREA_HUB_0x85:
        AreaProp_SpawnInLobby(2);
        break;
    case AREA_TOWN_ARENA:
        break;
    case AREA_ELDER_HALL:
        AreaProp_SpawnInLobby(2);
        break;
    case AREA_HUB_0x89:
        AreaProp_SpawnInLobby(1);
        BirdFlock_Spawn(1);
        break;
    case AREA_DESERT_NIGHT_2:
    case AREA_DESERT_2:
        ScrollingOverlay_Create();
        break;
    case AREA_DESERT_NIGHT_3:
        SkyStreak_Create(0);
    case AREA_DESERT_3:
        ScrollingOverlay_Create();
        break;
    case AREA_DESERT_NIGHT_4:
    case AREA_DESERT_4:
        LightShaft_Create();
        AreaDrop_Create();
        break;
    case AREA_DESERT_NIGHT_9:
    case AREA_DESERT_9:
        ScrollingOverlay_Create();
        break;
    case AREA_DESERT_NIGHT_10:
    case AREA_DESERT_10:
        AreaDrop_Create();
        ScrollingOverlay_Create();
        break;
    case AREA_SWAMP_NIGHT_3:
    case AREA_SWAMP_NIGHT_4:
    case AREA_SWAMP_3:
    case AREA_SWAMP_4:
        LightShaft_Create();
        AreaDrop_Create();
        break;
    case AREA_SWAMP_NIGHT_5:
    case AREA_SWAMP_5:
        AnimatedGroundTiles_Create();
        break;
    case AREA_SWAMP_NIGHT_8:
    case AREA_SWAMP_8:
        AreaDrop_Create();
        break;
    case AREA_VOLCANO_NIGHT_0:
    case AREA_VOLCANO_0:
        pos[0] = 0.0f;
        pos[1] = 0.0f;
        pos[2] = 0.0f;
        SoundObj_Start(SE_SLOT_SCENE, 0x48, 0x11, pos, 0xB4, 1);
        break;
    case AREA_VOLCANO_1:
    case AREA_VOLCANO_2:
    case AREA_VOLCANO_5:
    case AREA_VOLCANO_6:
        AreaObj_K19_Create();
        break;
    case AREA_HUB_0xAA:
        AreaProp_SpawnInLobby(1);
        break;
    case AREA_HUB_0xAB:
        AreaProp_SpawnInLobby(1);
        break;
    case AREA_ONLINE_HOUSE_1:
        AreaProp_SpawnInLobby(1);
        break;
    case AREA_ONLINE_HOUSE_2:
        AreaProp_SpawnInLobby(1);
        break;
    case AREA_BATTLE_TOWN_1:
    case AREA_BATTLE_TOWN_NIGHT_1:
        AreaWeapon_Create(8);
        break;
    case AREA_BATTLE_TOWN_3:
    case AREA_BATTLE_TOWN_NIGHT_3:
        AreaWeapon_Create(9);
        break;
    case AREA_BATTLE_TOWN_0:
        TurningProp_Create();
        break;
    case AREA_BATTLE_TOWN_NIGHT_0:
        TurningProp_Create();
        AnimatedProps_Create();
        break;
    }
}

/* The number of sub-models Area_LoadSubModels loads for an area: 4 for AREA_HUB_0x89 (its own
 * and the three facilities' levels), 1 for any other. */
s16 Area_SubModelCount(u16 area)
{
    switch (area) {
    case AREA_HUB_0x89:
        return 4;
    default:
        return 1;
    }
}

/*
 * Checks whether the local player stands at one of the current area's action
 * spots (Stage_GetAreaData: 0x34-byte records ended by id 0xFFFF). The first one
 * found moves the player to its target position and angle.
 */
void Player_CheckActionSpot(void)
{
    f32 slabStart[3];
    f32 slabEnd[3];
    AreaSpot *spot;
    Player *pl = &gPlayers[gScene.localPlayer];
    s16 hit;
    f32 dx;
    f32 dz;

    if (gScene.inLobby == 0) {
        if (pl->hp <= 0 || gScene.questPhase == QUEST_PHASE_FAILED || gScene.questPhase == QUEST_PHASE_CLEARED) {
            if (pl->areaMoveState == PL_AREA_MOVE_ENTER) {
                pl->areaMoveState = PL_AREA_MOVE_NONE;
            }
            return;
        }
    } else if (Town_IsIdleOnline() == 0) {
        return;
    }
    if (pl->areaMoveState != PL_AREA_MOVE_NONE) {
        return;
    }
    if (pl->spotCheckDelay != 0) {
        return;
    }
    if (gScene.inLobby == 1) {
        spot = NULL;
    } else {
        spot = Stage_GetAreaData(pl->area);
    }
    if (spot == NULL) {
        return;
    }
    for (; spot->id != AREA_NONE; spot++) {
        if (pl->pos[1] >= spot->pos[1] && pl->pos[1] < spot->pos[1] + spot->height) {
            switch (spot->type) {
            case 0:
                dx = pl->pos[0] - spot->pos[0];
                dz = pl->pos[2] - spot->pos[2];
                if (Math_Sqrtf(dx * dx + dz * dz) <= spot->radius) {
                    hit = 1;
                } else {
                    hit = 0;
                }
                break;
            case 1:
                slabStart[0] = spot->pos[0];
                slabStart[1] = spot->pos[1];
                slabStart[2] = spot->pos[2];
                slabEnd[0] = spot->pos2[0];
                slabEnd[1] = spot->pos2[1];
                slabEnd[2] = spot->pos2[2];
                if (Slab_Contains(pl->pos, slabStart, slabEnd, spot->height, spot->radius) != 0) {
                    hit = 1;
                } else {
                    hit = 0;
                }
                break;
            }
            if (hit != 0) {
                pl->areaMoveState = PL_AREA_MOVE_ENTER;
                pl->nextArea = Area_IdForCurrentTime(spot->id);
                pl->startPos[0] = spot->target[0];
                pl->startPos[1] = spot->target[1];
                pl->startPos[2] = spot->target[2];
                pl->startAngle = spot->angle + 0x4000;
                Player_AddSpawnOffset(pl, pl->startPos, pl->startAngle);
                if (gScene.inLobby == 0) {
                    QSync_SendPlayer(pl, QSYNC_PL_RELOCATED, 0);
                }
                break;
            }
        }
    }
}

void Area_ClearState(void)
{
    memset(&gAreaState, 0, sizeof(AreaState));
}

void Area_ResetFade(void)
{
    gAreaFade = 0;
    gAreaFadeTimer = 0;
}

/*
 * A state (0-2) of areas of kind 8, 0xD or 0xE, cached in monsterState: 2 while a
 * monster of kind 0x36 or 0x3C is in the area; otherwise 0 when the two
 * counters of Quest_GetTime differ by less than 900, else 0 or 1 by the
 * parity of counter 0 / 7200. Kept as it is while Quest_GetOutcome(1).
 */
s16 Area_MonsterState(u16 area)
{
    Em *em = gEms;
    AreaState *state = &gAreaState;
    s16 i;

    switch (gAreaInfo[area].atmosType) {
    case 8:
    case 0xD:
    case 0xE:
        if (Quest_GetOutcome(1) != 0) {
            return state->monsterState;
        }
        for (i = 0; i < EM_COUNT; i++, em++) {
            if (em->active != 0 && em->visible != 0 && (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) && em->area == area) {
                state->monsterState = 2;
                return state->monsterState;
            }
        }
        if (Quest_GetTime(1) - Quest_GetTime(0) < 900) {
            state->monsterState = 0;
        } else if ((Quest_GetTime(0) / 7200) & 1) {
            state->monsterState = 0;
        } else {
            state->monsterState = 1;
        }
        return state->monsterState;
    }
    return 0;
}

s16 Area_Temperature(u16 area)
{
    return gAreaTemperatures[area];
}

/* The day map of a night map (gMapDayIds); any other map is its own. */
u16 Map_DayId(u16 map)
{
    u16 dayMap = gMapDayIds[map];

    if (dayMap != 0xFFFF) {
        return dayMap;
    }
    return map;
}

/*
 * The group a map belongs to, as the quest lists, the quest sheets and the
 * maps' names table sort them: its day map, with the two other tower maps
 * counted as MAP_TOWER.
 */
u16 Map_Group(u16 map)
{
    u16 group = Map_DayId(map);

    switch (group) {
    case MAP_TOWER_B:
    case MAP_TOWER_C:
        group = MAP_TOWER;
        break;
    }
    return group;
}

/*
 * A zone has one area id by day and another by night: gAreaTimeOfDay gives
 * an id's time (AREA_TIME_*), gAreaOtherTime the other one's id.
 * Area_IsDay: 0 for a night area, 1 for any other and for AREA_NONE.
 */
s32 Area_IsDay(u16 area)
{
    if (area == AREA_NONE) {
        return 1;
    } else if (gAreaTimeOfDay[area] == AREA_TIME_NIGHT) {
        return 0;
    } else {
        return 1;
    }
}

/* The same zone's id at the other time of day; an area without one is its own. */
u16 Area_OtherTimeId(u16 area)
{
    u16 otherId;

    if (area == AREA_NONE) {
        return AREA_NONE;
    }
    otherId = gAreaOtherTime[area];
    if (otherId != 0xFFFF) {
        return otherId;
    }
    return area;
}

/* The day id of an area: a night area's other id, else the area itself. */
u16 Area_DayId(u16 area)
{
    if (area == AREA_NONE) {
        return AREA_NONE;
    }
    if (Area_IsDay(area) == 0) {
        return Area_OtherTimeId(area);
    }
    return area;
}

/* The id of the area's zone at the current time of day (online always by day). */
u16 Area_IdForCurrentTime(u16 area)
{
    s16 daytime;

    if (area == AREA_NONE) {
        return AREA_NONE;
    }
    daytime = gScene.inLobby == 0 ? Daytime_Get() : 0;
    if (daytime != gAreaTimeOfDay[area]) {
        return Area_OtherTimeId(area);
    }
    return area;
}

/* The index of an area in its map's list (the n of AREA_<map>_<n>); AREA_INDEX_NONE when not there. */
u16 Map_AreaIndex(u16 map, u16 area)
{
    s16 i;

    for (i = 0; i < gMapAreaCount[map]; i++) {
        if (area == gMapAreas[map][i]) {
            return i;
        }
    }
    return AREA_INDEX_NONE;
}

/* The area id at index i of a map's list; AREA_NONE for no map or no index. */
u16 Map_AreaAt(u16 map, u16 index)
{
    if (map != 0xFFFF && index != AREA_INDEX_NONE) {
        return gMapAreas[map][index];
    }
    return AREA_NONE;
}

/* 1 in an area that bars barrel bombs (Item_CanPlaceLargeBarrelBomb, Item_CanUseSmallBarrelBomb):
 * the areas listed first always, the snowy mountains' areas 5, 6 and 8 (day and night) while
 * Area_MonsterState is 2; else 0. */
s32 Area_NoBarrelBombs(u16 area)
{
    switch (area) {
    case AREA_JUNGLE_NIGHT_0:
    case AREA_JUNGLE_NIGHT_1:
    case AREA_JUNGLE_NIGHT_2:
    case AREA_JUNGLE_NIGHT_3:
    case AREA_JUNGLE_NIGHT_4:
    case AREA_JUNGLE_NIGHT_5:
    case AREA_JUNGLE_NIGHT_6:
    case AREA_FORTRESS_NIGHT_4:
    case AREA_FORTRESS_NIGHT_5:
    case AREA_FORTRESS_NIGHT_1:
    case AREA_FORTRESS_NIGHT_3:
    case AREA_FORTRESS_NIGHT_2:
    case AREA_FORTRESS_NIGHT_6:
    case AREA_SWAMP_10:
    case AREA_SWAMP_0:
    case AREA_SWAMP_1:
    case AREA_SWAMP_2:
    case AREA_SWAMP_5:
    case AREA_SWAMP_6:
    case AREA_SWAMP_7:
    case AREA_SWAMP_9:
        return 1;
    case AREA_SNOWY_MOUNTAINS_5:
    case AREA_SNOWY_MOUNTAINS_6:
    case AREA_SNOWY_MOUNTAINS_8:
    case AREA_SNOWY_MOUNTAINS_NIGHT_5:
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
    case AREA_SNOWY_MOUNTAINS_NIGHT_8:
        return Area_MonsterState(area) == 2;
    default:
        return 0;
    }
}

/* The height added to the ground under a dust effect (DustCloud_FindGround, PartFx_PutOnGround):
 * 0.0 in every area; the two swamp night areas have a case of their own that also gives 0.0. */
f32 Area_GroundOffset(u16 area)
{
    switch (area) {
    case AREA_SWAMP_NIGHT_7:
    case AREA_SWAMP_NIGHT_9:
        return 0.0f;
    }
    return 0.0f;
}

s32 Area_HasFlyer(void)
{
    return gAreaState.flyer != 0;
}

/* Copies the flyer's position (gAreaState.flyerPos) to out when the area has a flyer point
 * in gAreaInfo; out is left as it is otherwise. */
void Area_GetFlyerPos(f32 *out, u16 area)
{
    if (gAreaInfo[area].flyerPoint != 0xFF) {
        Vec_Copy(out, gAreaState.flyerPos);
    }
}

void Area_StartTimers(AreaState *state)
{
    state->timerState++;
    state->frame = 0;
}

/* This file's initialised data. */
VibPattern gVibPatterns[6] = {
#include "sub_main/0043ECF0.inc"
};
f32 gCamDefaultEye[6] = {
#include "sub_main/0043ED20.inc"
};
f32 gCamDefaultDir[3] = {
#include "sub_main/0043ED38.inc"
};
CamPreset gCameraPresets[3] = {
#include "sub_main/0043ED50.inc"
};
u16 gMapDayIds[32] = {
#include "sub_main/0043ED80.inc"
};
u16 gMapAreaCount[40] = {
#include "sub_main/0043EDC0.inc"
};
u16 gMapAreas_2[16] = {
#include "sub_main/0043EE10.inc"
};
u16 gMapAreas_3[16] = {
#include "sub_main/0043EE30.inc"
};
u16 gMapAreas_4[16] = {
#include "sub_main/0043EE50.inc"
};
u16 gMapAreas_5[16] = {
#include "sub_main/0043EE70.inc"
};
u16 gMapAreas_6[12] = {
#include "sub_main/0043EE90.inc"
};
u16 gMapAreas_7[4] = {
#include "sub_main/0043EEA8.inc"
};
u32 gMapAreas_8[2] = {
#include "sub_main/0043EEB0.inc"
};
u32 gMapAreas_9[2] = {
#include "sub_main/0043EEB8.inc"
};
u32 gMapAreas_10[4] = {
#include "sub_main/0043EEC0.inc"
};
u16 gMapAreas_11[12] = {
#include "sub_main/0043EED0.inc"
};
u16 gMapAreas_12[4] = {
#include "sub_main/0043EEE8.inc"
};
u16 gMapAreas_13[16] = {
#include "sub_main/0043EEF0.inc"
};
u16 gMapAreas_14[16] = {
#include "sub_main/0043EF10.inc"
};
u16 gMapAreas_15[16] = {
#include "sub_main/0043EF30.inc"
};
u16 gMapAreas_16[16] = {
#include "sub_main/0043EF50.inc"
};
u16 gMapAreas_17[16] = {
#include "sub_main/0043EF70.inc"
};
u16 gMapAreas_18[16] = {
#include "sub_main/0043EF90.inc"
};
u16 gMapAreas_19[16] = {
#include "sub_main/0043EFB0.inc"
};
u16 gMapAreas_20[16] = {
#include "sub_main/0043EFD0.inc"
};
u16 gMapAreas_21[12] = {
#include "sub_main/0043EFF0.inc"
};
u16 gMapAreas_22[4] = {
#include "sub_main/0043F008.inc"
};
u16 gMapAreas_23[8] = {
#include "sub_main/0043F010.inc"
};
u16 gMapAreas_24[4] = {
#include "sub_main/0043F020.inc"
};
u16 gMapAreas_25[4] = {
#include "sub_main/0043F028.inc"
};
u16 *gMapAreas[28] = {
    &gMapAreaCount[28], &gMapAreaCount[28], gMapAreas_2, gMapAreas_3,
    gMapAreas_4, gMapAreas_5, gMapAreas_6, gMapAreas_7,
    (u16 *)gMapAreas_8, (u16 *)gMapAreas_9, (u16 *)gMapAreas_10, gMapAreas_11,
    gMapAreas_12, gMapAreas_13, gMapAreas_14, gMapAreas_15,
    gMapAreas_16, gMapAreas_17, gMapAreas_18, gMapAreas_19,
    gMapAreas_20, gMapAreas_21, gMapAreas_22, gMapAreas_23,
    gMapAreas_24, gMapAreas_25, NULL, NULL,
};
u32 gStartPos[592] = {
#include "sub_main/0043F0A0.inc"
};
u32 gStartAngle[100] = {
#include "sub_main/0043F9E0.inc"
};
u32 gAreaKindFlags[200] = {
#include "sub_main/0043FB70.inc"
};
u32 gAreaLightZones[96] = {
#include "sub_main/0043FE90.inc"
};
u32 gAreaLightZonesAlt[104] = {
#include "sub_main/00440010.inc"
};
u32 gAreaLightZones2[200] = {
#include "sub_main/004401B0.inc"
};
u32 gAreaLightColors[200] = {
#include "sub_main/004404D0.inc"
};
u32 gAreaTriKindFlags[200] = {
#include "sub_main/004407F0.inc"
};
u32 gAreaKinds[100] = {
#include "sub_main/00440B10.inc"
};
s16 gAreaTemperatures[200] = {
#include "sub_main/00440CA0.inc"
};
s16 gAreaTimeOfDay[200] = {
#include "sub_main/00440E30.inc"
};
u16 gAreaOtherTime[200] = {
#include "sub_main/00440FC0.inc"
};
