/*
 * sub_main 0x376C00-0x379EC0: small systems of the field.
 * - Three pools of draw slots (DrawSlot; 0x6A3A50: 4, 0x6A3AD0: 256,
 *   0x6A5AD0: 512) with a "first free" hint each.
 * - Depth-sorted insertion into ordering tables (OT_AddByDepth).
 * - Pad vibration shortcuts (Vib_Start), gated by the vibration option.
 * - The camera (gCamera -> gCameraData): eye, target, up, FOV, roll,
 *   viewport, and the projection (Camera_ApplyProjection).
 * - The monster slots (gEms, 20 x 0x9D0): allocation, model setup, free.
 * - Five lists of 32 object pointers (0x5D8A70.., the objects monsters check,
 *   EmObj in include/em.h), with their counts.
 * - The area state at 0x5EC460 (entering an area, its fog, starting its
 *   objects, the action spots) and the per-area tables (area lists per map,
 *   day/night variants of an area id).
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



/* A vibration pattern: the two values given to Pad_SetVibration (probably the
 * pad actuators). */
typedef struct VibPattern {
    s32 a;
    s32 b;
} VibPattern;

/* A camera preset (gCameraPresets, Camera_SetPreset). */
typedef struct CamPreset {
    f32 unk34;
    f32 unk30;
    f32 fov;
    f32 unkC;
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

/* Per area id (4 bytes): four setup ids for Area_StartObjects (0xFF: none). unk0
 * is also a kind: 8, 0xD and 0xE get Area_MonsterState's state. */
typedef struct AreaInfo {
    u8 unk0;            /* AmbientSound_Spawn */
    u8 unk1;            /* AreaObj_K09_Create */
    u8 unk2;            /* AreaObj_K13_Create */
    u8 unk3;            /* StageObj_AreaFlyer_Create; 0xFF clears AreaState.flyer */
} AreaInfo;

extern DrawSlot gHudDrawHooks[4];
extern DrawSlot gDrawSlotsB[0x100];
extern DrawSlot gDrawSlotPool[0x200];
extern s32 gDrawSlotBNext;          /* first free entry of gDrawSlotsB */
extern s32 gDrawSlotNext;          /* first free entry of gDrawSlotPool */
extern s32 gTexHandles[];
extern LobbyState gLobbyState;
extern VibPattern gVibPatterns[];
extern f32 D_0043ED20[];       /* default eye */
extern f32 D_0043ED2C[3];       /* default target */
extern f32 D_0043ED38[3];
extern CamPreset gCameraPresets[];
extern u8 gCameraPreset;           /* the current camera preset */
extern f32 gFrustumPlanes[4][4];    /* the projection matrix */
extern f32 gCamYawMtx[4][4];    /* rotation by the camera's yaw */
extern f32 gCamMatrix[];
extern f32 gViewMatrix[4][4];
extern u8 gModelUsed[];
extern EmObj *gEmSenseObjs[32];
extern EmObj *D_005D8AF0[32];
extern EmObj *gFlashObjs[32];
extern EmObj *gSightBlockObjs[32];
extern EmObj *gAreaObjList[32];
extern s8 gEmSenseObjCount;           /* entries used in gEmSenseObjs */
extern s8 D_005D8A48;           /* ... in D_005D8AF0 */
extern s8 gFlashObjCount;           /* ... in gFlashObjs */
extern s8 gSightBlockObjCount;           /* ... in gSightBlockObjs */
extern s8 gAreaObjCount;           /* ... in gAreaObjList */
extern FogParam gFogDefault;     /* the default fog */
extern FogParam gFogArea56;     /* the fog of area 0x56 */
extern AreaInfo gAreaInfo[];
extern s16 gAreaFade;
extern s16 gAreaFadeTimer;
extern s16 gAreaTemperatures[];
extern u16 gAreaBaseIds[];
extern s16 gAreaTimeOfDay[];        /* per area id: its time of day (0, 1) */
extern u16 gAreaOtherTime[];        /* per area id: the other time's id, 0xFFFF: none */
extern u16 gMapAreaCount[];        /* per map: number of areas */
extern u16 *gMapAreas[];       /* per map: its area ids */

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
struct Mtx44Copy;
void MtxSlot_Load(struct Mtx44Copy *m, s32 i);
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
void Effect06_Start(s16 i);
void SoundObj_Start(s32 bank, s32 se, s32 k, f32 *pos, s16 n, u8 quiet);
void AmbientSound_Spawn(s32 a);
void Player_AddSpawnOffset(Player *p, f32 *pos, u16 angle);
void *Stage_GetAreaData(s32 area);
/* unit's own declaration: with the definition's u8 *(u16 x) (sub_main/stage_data.c), Area_Enter: 33 of 147 words differ */
AreaRecord *Stage_GetAreaRecord(u32 id);
s32 Quest_IsRunning(s32 ending);
s32 Quest_GetTime(s32 a);
s32 Player_IsLocal(void *obj);
u8 Village_FacilityLevel(s32 i);
/* unit's own declaration: with the definition's u8 (f32 *pos, f32 *a, f32 *b, f32 height, f32 radius) (sub_main/unk_3BE060.c), Player_CheckActionSpot: 72 of 178 words differ */
s32 Slab_Contains(f32 *pos, f32 *a, f32 *b, f32 height, f32 radius);
void Obj_UpdateMatrix(u8 *o);
void VFlock_Spawn(s8 arg0);
void Eff10_Create(f32 *pos, u8 kind, u16 area, Player *pl);
void AreaObj_K00_Create(void);
void AreaObj_K02_Create(u8 a);
void AreaObj_K03_Create(void);
void AreaObj_K04_Create(void);
void AreaObj_K05_Create(u8 a);
void AreaObj_K07_Create(void);
void AreaObj_K08_Create(void);
void AreaObj_K09_Create(u8 a);
void AreaObj_K0A_Create(void);
void AreaObj_K0B_Create(void);
void AreaObj_K0E_Create(void);
void AreaObj_K10_Create(void);
void AreaObj_K11_Create(void);
void AreaObj_K13_Create(u8 a);
void AreaObj_K14_Create(u8 kind);
void AreaObj_K16_Create(void);
void AreaObj_K19_Create(void);
u8 EmObj_AlertMonsters(EmObj *obj);
s32 Town_IsIdleOnline(void);
u8 MyHouse_Level(void);
s32 LobbyTime_IsAm(void);
s32 LobbyWorld_GetSeason(void);
void LobbyFlock_Spawn(void);
void StageObj_K0E_CreateAt(f32 *pos, s16 k);
void StageObj_AreaFlyer_Create(u8 a);
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

/* Runs Obj_UpdateMatrix on the active players (10 slots from gPlayers). */
void Players_UpdateMatrices(void)
{
    Player *p = gPlayers;
    s32 i;

    for (i = 0; i < 10; i++, p++) {
        /* the lobby is loaded: its town players are drawn once their model is (pcLoaded, read as s8 here: lb) */
        if ((Overlay_IsLoaded(4) != 1 || (s8)gLobbyState.pcLoaded[p->index] == 1) && p->active != 0) {
            Obj_UpdateMatrix((u8 *) p);
        }
    }
}

/* Counts the nonzero entries of gTexHandles[start .. start + 29]. */
s32 Tex_CountLoaded(s32 start)
{
    s32 i;
    s32 n;
    s32 *p;

    for (i = 0, n = 0, p = &gTexHandles[start]; i < 30; i++, p++) {
        if (*p != 0) {
            n++;
        }
    }
    return n;
}

void Gs_SetLightList(ModelMat *mats, ModelMatSet *set)
{
    s32 i;

    for (i = 0; i < set->count; i++) {
        Gs_SetState(i + 0x3A, (u32)&mats[set->mat[i]]);
    }
}

/* Inserts node into the list at link, before the first node whose key is
 * not greater (the list is kept in decreasing depth). */
void OT_InsertSorted(DrawSlot *node, u32 *link)
{
    u32 next;
    DrawSlot *n;

    for (;;) {
        next = *link;
        if (next == 0 || !(next & 1)) {
            *link = (u32)node | 1;
            node->next = next;
            return;
        }
        n = (DrawSlot *)(next & ~1);
        if (n->key <= node->key) {
            *link = (u32)node | 1;
            node->next = next;
            return;
        }
        link = &n->next;
    }
}

void DrawPool_Clear(DrawSlot *p, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++, p++) {
        memset(p, 0, sizeof(DrawSlot));
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

DrawSlot *DrawSlot_Get(s32 i)
{
    DrawSlot *p = NULL;

    if (i >= 0) {
        p = &gDrawSlotPool[i];
    }
    return p;
}

DrawSlot *DrawSlotB_Get(s32 i)
{
    DrawSlot *p = NULL;

    if (i >= 0) {
        p = &gDrawSlotsB[i];
    }
    return p;
}

/* Takes the first free entry of gDrawSlotPool and moves the hint to the next
 * free one; -1 when full. */
s16 DrawSlot_Alloc(void)
{
    s32 i = gDrawSlotNext;
    DrawSlot *p;
    s32 j;

    if (i >= 0x200) {
        return -1;
    }
    p = DrawSlot_Get(i + 1);
    for (j = gDrawSlotNext + 1; j < 0x200; j++, p++) {
        if (p->draw == 0) {
            break;
        }
    }
    gDrawSlotNext = j;
    p = DrawSlot_Get(i);
    p->draw = (DrawSlotFn) 1;
    return i;
}

/* As DrawSlot_Alloc for gDrawSlotsB, but the entry is not marked used. */
s16 DrawSlotB_Alloc(void)
{
    s32 i = gDrawSlotBNext;
    DrawSlot *p;
    s32 j;

    if (i >= 0x100) {
        return -1;
    }
    p = DrawSlotB_Get(i + 1);
    for (j = gDrawSlotBNext + 1; j < 0x100; j++, p++) {
        if (p->draw == 0) {
            break;
        }
    }
    gDrawSlotBNext = j;
    return i;
}

void DrawSlot_Free(s32 i)
{
    memset(DrawSlot_Get(i), 0, sizeof(DrawSlot));
    if (i < gDrawSlotNext) {
        gDrawSlotNext = i;
    }
}

void DrawSlotB_Free(s32 i)
{
    DrawSlot *p = DrawSlotB_Get(i);
    void *owner = p->owner;
    s32 part = p->part;

    memset(p, 0, sizeof(DrawSlot));
    p->owner = owner;
    p->part = part;
    if (i < gDrawSlotBNext) {
        gDrawSlotBNext = i;
    }
}

/*
 * Adds node to an ordering table of n buckets by its view depth (matrix 0x21):
 * nearer nodes go to higher buckets. A node behind the camera is put at depth
 * 0, or dropped (-1) when more than 1600 behind and clamp is 0. Returns the
 * bucket. 64-bucket tables are kept sorted within a bucket.
 */
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp)
{
    f32 in[4];
    f32 out[4];
    f32 m[4][4];
    u32 idx;
    f32 *z;

    in[0] = node->pos[0];
    in[1] = node->pos[1];
    in[2] = node->pos[2];
    in[3] = 1.0f;
    MtxSlot_Load((struct Mtx44Copy *) m, 0x21);
    Vec4_Transform((f32 *) out, (f32 *) in, (f32 *) m);
    z = &out[2];
    *z *= -1.0f;
    if (*z < 0.0f) {
        if (clamp != 0) {
            *z = 0.0f;
        } else if (*z < -1600.0f) {
            return -1;
        } else {
            *z = 0.0f;
        }
    }
    node->key = *z;
    idx = *z / 65000.0f * n;
    if (idx >= n) {
        return -1;
    }
    if (n == 0x40) {
        OT_InsertSorted(node, &table[n - 1 - idx]);
    } else {
        TagList_Push((u32 *) node, (u32 *) (&table[n - 1 - idx]));
    }
    return idx;
}

s32 OT_AddAt(u32 *table, void *node, s32 idx, s32 n)
{
    if (idx >= n || idx < 0) {
        return -1;
    }
    TagList_Push(node, &table[n - 1 - idx]);
    return idx;
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
void Color_RgbToFloat(f32 *rgb, u32 c)
{
    rgb[0] = 0.003921569f * ((c >> 16) & 0xFF);
    rgb[1] = 0.003921569f * ((c >> 8) & 0xFF);
    rgb[2] = 0.003921569f * (c & 0xFF);
}

/* 0xAARRGGBB to r, g, b, a floats in 0..1. */
void Color_ArgbToFloat(f32 *rgba, u32 c)
{
    rgba[3] = 0.003921569f * ((c >> 24) & 0xFF);
    rgba[0] = 0.003921569f * ((c >> 16) & 0xFF);
    rgba[1] = 0.003921569f * ((c >> 8) & 0xFF);
    rgba[2] = 0.003921569f * (c & 0xFF);
}

/* Static: its callers keep a0 across the call (MWCC knows it is unused). */
static s32 Vib_IsOff(void)
{
    return gSysSave.options.vibration == 0;
}

/* Starts vibration pattern i on a pad port, unless vibration is off. */
void Vib_Start(s32 port, s32 i)
{
    if (!Vib_IsOff()) {
        Pad_SetVibration(port, gVibPatterns[i].a, gVibPatterns[i].b);
    }
}

/* The same on port 0, when Player_IsLocal allows it. */
void Vib_StartLocal(void *port, s32 i)
{
    if (!Vib_IsOff() && Player_IsLocal(port) != 0) {
        Pad_SetVibration(0, gVibPatterns[i].a, gVibPatterns[i].b);
    }
}

void Vib_Stop(s32 port)
{
    Pad_SetVibration(port, 0, 0);
}

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
    gCamera->unk58 = 1.0f;
    Gs_SetState(0x18, (u32)gCamera->viewport);
    Camera_Reset();
}

/* Default eye, target and direction, a FOV of 50 degrees, no roll. */
void Camera_Reset(void)
{
    Vec_Copy(gCamera->eye, D_0043ED20);
    Vec_Copy(gCamera->at, D_0043ED2C);
    Vec_Copy(gCamera->unk24, D_0043ED38);
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
    f32 m[4][4];
    s32 changed = 0;
    f32 angle;
    f32 *fov;

    Vec_Diff(dir, gCamera->at, gCamera->eye);
    if (dir[0] != 0.0f || dir[2] != 0.0f) {
        Vec_Copy(gCamera->unk24, dir);
    } else {
        Vec_Add(gCamera->at, gCamera->eye, gCamera->unk24);
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
    Mtx_LookAt((f32 *) m, (f32 *) gCamera->eye, (f32 *) gCamera->at, (f32 *) gCamera->up);
    Gs_SetState(0x16, (u32)m);
    MtxSlot_Load((struct Mtx44Copy *) gViewMatrix, 0x21);
    Mtx_Inverse(gCamMatrix, (f32 *) gViewMatrix);
    angle = 6.2831855f * (s32)(u16)(Mtx_Heading(gCamMatrix) + 0x4000) / 65536.0f;
    Mtx_Identity((f32 *) gCamYawMtx);
    Mtx_SetRotXYZ((f32 *) gCamYawMtx, 0.0f, angle, 0.0f);
}

/* Selects camera preset id (0xFF: only reapplies the projection). */
void Camera_SetPreset(s32 id)
{
    CamPreset *p;

    if (id != 0xFF) {
        if (gCameraPreset == id) {
            return;
        }
        gCameraPreset = id;
        p = &gCameraPresets[id];
        gCamera->unk34 = p->unk34;
        gCamera->unk30 = p->unk30;
        gCamera->fov = gCamera->fovPrev = p->fov;
        gCamera->roll = 0.0f;
    }
    Camera_ApplyProjection();
}

void Camera_BuildFrustum(f32 b)
{
    Frustum_BuildPlanes((f32 *) gFrustumPlanes, gCamera->fov, 1.4285715f, gCamera->unk30, b);
}

/* Applies the projection (aspect 640/448), the far value 3000. */
void Camera_ApplyProjection(void)
{
    Camera_SetPerspective(0x20, gCamera->unk34, gCamera->unk30, gCamera->fov, 1.4285715f);
    Frustum_BuildPlanes((f32 *) gFrustumPlanes, gCamera->fov, 1.4285715f, gCamera->unk30, 3000.0f);
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

f32 Camera_Distance(void *out)
{
    return Vec_Distance((f32 *) out, (f32 *) gCamera);
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
    f32 v[4];
    f32 m[4][4];
    f32 q[4];

    Vec_Diff(v, at, eye);
    Quat_Set(v, q, roll);
    Quat_ToMatrix(q, m);
    v[0] = 0.0f;
    v[1] = 1.0f;
    v[2] = 0.0f;
    Vec_Rotate((f32 *) up, (f32 *) v, (f32 *) m);
}

/* Clears the monster slots (only their first 0x970 bytes, a Player's size). */
void EmSlot_ClearAll(void)
{
    s16 i;
    Em *e = gEms;

    for (i = 0; i < 20; i++, e++) {
        memset(e, 0, sizeof(Player));
    }
}

/* Frees a monster slot: its pool entry, its model slots, and its first 0x12
 * bytes. */
void EmSlot_Free(Em *e)
{
    s16 i;

    if (e->drawSlot != 0) {
        DrawSlot_Free(e->drawSlotId);
    }
    if (e->model != NULL) {
        for (i = e->modelFirst; i < e->modelFirst + e->modelCount; i++) {
            Model_FreeCopy(i);
            if (gModelUsed[i] != 0) {
                Model_Release(i, 1);
            }
            e->model = NULL;
            e->modelFirst = 0;
            e->modelCount = 0;
        }
    }
    memset(e, 0, 0x12);
}

void EmSlot_FreeAll(void)
{
    s16 i;
    Em *e = gEms;

    for (i = 0; i < 20; i++, e++) {
        if (e->active != 0) {
            EmSlot_Free(e);
        }
    }
}

/* Gives a monster a model: a model slot, a copy of the scene's model header
 * e->modelId, and two sets of bone slots. */
void EmSlot_SetupModel(Em *e)
{
    s32 j;
    s32 i;
    Model *m;
    s32 id;
    SceneState *scene = &gScene;
    s32 slot;

    id = Model_Alloc(1);
    if (id >= 0) {
        e->modelFirst = id;
        e->modelCount = 1;
        e->model = Model_Get(id);
        Model_Reserve(id, 1);
        m = Model_Get(id);
        memcpy(m, scene->models[e->modelSlot], 0x80);
        m->active = 1;
        m->boneKind = 2;
        m->unk75 = 1;
        slot = Bone_Alloc(m->boneCount);
        m->boneFirst = slot;
        m->bones = Bone_Get(slot);
        m->bones2 = Bone_Get(slot + m->boneCount / 2);
        Bone_Reserve(slot, m->boneCount);
        for (i = 0, j = 0; j < m->partCount; j++) {
            Bone_InitTreeRoot(&m->bones[i], m->partSkels[j]);
            Bone_InitTreeRoot(&m->bones2[i], m->partSkels[j]);
            m->partBones[j] = &m->bones[i];
            m->partBones2[j] = &m->bones2[i];
            i += m->bones->count;
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
    Em *e = gEms;

    for (i = 0; i < 20; i++, e++) {
        if (e->active == 0) {
            memset(e, 0, sizeof(Em));
            e->slotNo = i;
            e->active = 1;
            e->isEm = 1;
            e->mineSlot = -1;
            e->index = i;
            e->model = NULL;
            e->remote = gScene.localPlayer;
            e->alpha = 1.0f;
            e->partMask = 0;
            goto init;
        done:
            return e;
        }
    }
    return NULL;
init:
    e->hitIds[i] = 0;
    e->hitIdNext = 0;
    goto done;
}

/*
 * Five lists of 32 object pointers with a count each: clear, add (0 when
 * full), remove, and for two of them a tick that removes an object when its
 * timer runs out. The game overlay's monsters check the lists at 0x5D8A70
 * and 0x5D8BF0 (src/game/unk_705AC0.c).
 */
void ObjList_Update(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        gAreaObjList[i] = NULL;
    }
    gAreaObjCount = 0;
}

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

void SightBlock_ClearAll(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        gSightBlockObjs[i] = NULL;
    }
    gSightBlockObjCount = 0;
}

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

void FlashObj_ClearAll(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        gFlashObjs[i] = NULL;
    }
    gFlashObjCount = 0;
}

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

void ObjListAF0_Clear(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        D_005D8AF0[i] = NULL;
    }
    D_005D8A48 = 0;
}

void ObjListA70_Clear(void)
{
    s32 i;

    for (i = 0; i < 32; i++) {
        gEmSenseObjs[i] = NULL;
    }
    gEmSenseObjCount = 0;
}

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
 * overlay; area 0x89 takes look[1..3] from Village_FacilityLevel.
 * The first stores address gAreaState directly, the later ones through q:
 * that is what the original does.
 */
void Area_Enter(u16 area)
{
    AreaState *q = &gAreaState;
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
    if (gScene.online != 0) {
        if (LobbyTime_IsAm() != 0) {
            q->look[0] = 0;
        } else {
            q->look[0] = 0xFF;
        }
    }
    switch (area) {
    case 0x83:
        if (gScene.online != 0) {
            switch (LobbyWorld_GetSeason()) {
            default:
            case 2:
                q->look[1] = 2;
                break;
            case 0:
                q->look[1] = 0;
                break;
            case 1:
                q->look[1] = 1;
                break;
            }
        } else {
            q->look[1] = 2;
        }
        break;
    case 0x89:
        for (i = 1; i < 4; i++) {
            q->look[i] = Village_FacilityLevel(i);
        }
        break;
    case 0xAD:
    case 0xAE:
    case 0xAF:
        if (gScene.online != 0) {
            q->look[1] = MyHouse_Level();
        } else {
            q->look[1] = Village_FacilityLevel(5);
        }
        break;
    }
}

/* Sets the fog render states: the area's own fog, or a default. */
void Area_ApplyFog(u16 area)
{
    AreaState *q = &gAreaState;
    FogParam *p;

    switch (area) {
    case 0:
        p = &gFogDefault;
        break;
    case 0x56:
        p = &gFogArea56;
        break;
    default:
        if (q->envBlocks > 0) {
            p = &q->fog;
        } else {
            p = &gFogDefault;
        }
        break;
    }
    Gs_SetState(0xF, ((p->a << 24) | (p->r << 16) | (p->g << 8) | p->b));
    Gs_SetState(0x10, *(u32 *)&p->nearDist);
    Gs_SetState(0x11, *(u32 *)&p->farDist);
}

/* Online only: Effect06_Start(0 .. n - 1). */
void Effect06_StartOnline(s16 n)
{
    s16 i;

    if (gScene.online != 0) {
        for (i = 0; i < n; i++) {
            Effect06_Start(i);
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
    f32 v[3];
    AreaState *q = &gAreaState;
    s16 i;

    if (gAreaState.envBlocks < 2) {
        q->skyPos[0] = 5000.0f;
        q->skyPos[1] = 5000.0f;
        q->skyPos[2] = 5000.0f;
        q->band.y = 0;
        q->band.h = 0;
        q->band.c[0] = q->band.c[1] = q->band.c[2] = q->band.c[3] = q->band.c[4] = q->band.c[5] =
            q->band.c[6] = q->band.c[7] = 0xFFFFFF;
    }
    Area_ResetFade();
    if (gAreaInfo[area].unk0 != 0xFF) {
        AmbientSound_Spawn(gAreaInfo[area].unk0);
    }
    if (gAreaInfo[area].unk1 != 0xFF) {
        AreaObj_K09_Create(gAreaInfo[area].unk1);
    }
    if (gAreaInfo[area].unk2 != 0xFF) {
        AreaObj_K13_Create(gAreaInfo[area].unk2);
    }
    if (gAreaInfo[area].unk3 != 0xFF) {
        StageObj_AreaFlyer_Create(gAreaInfo[area].unk3);
    } else {
        q->flyer = 0;
    }
    switch (area) {
    case 0xA:
    case 0x4C:
        AreaObj_K0A_Create();
        break;
    case 0xB:
        AreaObj_K05_Create(0);
        break;
    case 0xC:
        AreaObj_K05_Create(1);
        break;
    case 0x4E:
        AreaObj_K05_Create(6);
        break;
    case 0xE:
    case 0x4F:
        AreaObj_K04_Create();
        break;
    case 0x11:
        for (i = 0; i < 10; i++) {
            v[0] = 1400.0f + 300.0f * i;
            v[1] = 30.0f;
            v[2] = 2000.0f + 300.0f * i;
            Eff10_Create(v, 0, area, 0);
        }
        break;
    case 0x14:
        AreaObj_K14_Create(0);
        break;
    case 0x15:
    case 0xB8:
        v[0] = 11250.0f;
        v[1] = 40.0f;
        v[2] = 9500.0f;
        StageObj_K0E_CreateAt(v, 2);
        StageObj_K0E_CreateAt(v, 3);
        break;
    case 0x19:
        AreaObj_K05_Create(2);
        AreaObj_K14_Create(1);
        AreaObj_K16_Create();
        break;
    case 0x1C:
    case 0x50:
        AreaObj_K05_Create(3);
        AreaObj_K04_Create();
        AreaObj_K0B_Create();
        break;
    case 0x1E:
        AreaObj_K05_Create(4);
        break;
    case 0x51:
        AreaObj_K05_Create(7);
        break;
    case 0x29:
    case 0xC2:
        AreaObj_K00_Create();
        break;
    case 0x2A:
    case 0xC3:
        AreaObj_K00_Create();
        AreaObj_K07_Create();
        AreaObj_K0E_Create();
        break;
    case 0x2B:
    case 0xC4:
        AreaObj_K00_Create();
        break;
    case 0x35:
        AreaObj_K0E_Create();
        break;
    case 0x48:
    case 0x4B:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 0.0f;
        SoundObj_Start(7, 0x51, 0x11, v, 0x3C, 3);
        break;
    case 0x4D:
        AreaObj_K05_Create(5);
        break;
    case 0x56:
        break;
    case 0x2E:
    case 0x59:
        AreaObj_K04_Create();
        break;
    case 0x39:
    case 0x5A:
        AreaObj_K0E_Create();
        AreaObj_K04_Create();
        break;
    case 0x42:
    case 0x5B:
        AreaObj_K04_Create();
        break;
    case 0x5C:
    case 0x65:
        AreaObj_K00_Create();
        break;
    case 0x5E:
    case 0x67:
        AreaObj_K00_Create();
        break;
    case 0x61:
    case 0x6A:
        AreaObj_K07_Create();
        AreaObj_K04_Create();
        break;
    case 0x69:
    case 0x6D:
        AreaObj_K07_Create();
        break;
    case 1:
    case 0x6E:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 0.0f;
        SoundObj_Start(7, 0x48, 0x11, v, 0xB4, 1);
        break;
    case 2:
    case 0x6F:
        AreaObj_K11_Create();
        break;
    case 4:
    case 0x71:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 0.0f;
        SoundObj_Start(7, 0x48, 0x11, v, 0xB4, 1);
        break;
    case 5:
    case 0x72:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 0.0f;
        SoundObj_Start(7, 0x48, 0x11, v, 0xB4, 1);
        break;
    case 0x13:
    case 0x74:
        AreaObj_K11_Create();
        break;
    case 0x78:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 0.0f;
        SoundObj_Start(7, 0x48, 0x11, v, 0xB4, 1);
        break;
    case 0x82:
        AreaObj_K00_Create();
        AreaObj_K03_Create();
        break;
    case 0x83:
        if (gScene.online != 0) {
            Effect06_StartOnline(7);
            if (q->look[1] != 0) {
                LobbyFlock_Spawn();
            }
            VFlock_Spawn(0);
        }
        break;
    case 0x84:
        Effect06_StartOnline(6);
        break;
    case 0x85:
        Effect06_StartOnline(2);
        break;
    case 0x87:
        break;
    case 0x88:
        Effect06_StartOnline(2);
        break;
    case 0x89:
        Effect06_StartOnline(1);
        VFlock_Spawn(1);
        break;
    case 0x30:
    case 0x8E:
        AreaObj_K0E_Create();
        break;
    case 0x31:
        AreaObj_K02_Create(0);
    case 0x8F:
        AreaObj_K0E_Create();
        break;
    case 0x32:
    case 0x90:
        AreaObj_K00_Create();
        AreaObj_K03_Create();
        break;
    case 0x37:
    case 0x95:
        AreaObj_K0E_Create();
        break;
    case 0x38:
    case 0x96:
        AreaObj_K03_Create();
        AreaObj_K0E_Create();
        break;
    case 0x44:
    case 0x45:
    case 0x9A:
    case 0x9B:
        AreaObj_K00_Create();
        AreaObj_K03_Create();
        break;
    case 0x46:
    case 0x9C:
        AreaObj_K08_Create();
        break;
    case 0x49:
    case 0x9F:
        AreaObj_K03_Create();
        break;
    case 0x3A:
    case 0xA1:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 0.0f;
        SoundObj_Start(7, 0x48, 0x11, v, 0xB4, 1);
        break;
    case 0xA2:
    case 0xA3:
    case 0xA6:
    case 0xA7:
        AreaObj_K19_Create();
        break;
    case 0xAA:
        Effect06_StartOnline(1);
        break;
    case 0xAB:
        Effect06_StartOnline(1);
        break;
    case 0xAE:
        Effect06_StartOnline(1);
        break;
    case 0xAF:
        Effect06_StartOnline(1);
        break;
    case 0xB0:
    case 0xB4:
        AreaObj_K05_Create(8);
        break;
    case 0xB2:
    case 0xB6:
        AreaObj_K05_Create(9);
        break;
    case 0xB3:
        AreaObj_K10_Create();
        break;
    case 0xB7:
        AreaObj_K10_Create();
        AreaObj_K07_Create();
        break;
    }
}

s16 Area_SubModelCount(u16 area)
{
    switch (area) {
    case 0x89:
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
    f32 a[3];
    f32 b[3];
    AreaSpot *spot;
    Player *p = &gPlayers[gScene.localPlayer];
    s16 hit;
    f32 dx;
    f32 dz;

    if (gScene.online == 0) {
        if (p->hp <= 0 || gScene.questPhase == 6 || gScene.questPhase == 4) {
            if (p->areaMoveState == 1) {
                p->areaMoveState = 0;
            }
            return;
        }
    } else if (Town_IsIdleOnline() == 0) {
        return;
    }
    if (p->areaMoveState != 0) {
        return;
    }
    if (p->spotCheckDelay != 0) {
        return;
    }
    if (gScene.online == 1) {
        spot = NULL;
    } else {
        spot = Stage_GetAreaData(p->area);
    }
    if (spot == NULL) {
        return;
    }
    for (; spot->id != 0xFFFF; spot++) {
        if (p->pos[1] >= spot->pos[1] && p->pos[1] < spot->pos[1] + spot->height) {
            switch (spot->type) {
            case 0:
                dx = p->pos[0] - spot->pos[0];
                dz = p->pos[2] - spot->pos[2];
                if (Math_Sqrtf(dx * dx + dz * dz) <= spot->radius) {
                    hit = 1;
                } else {
                    hit = 0;
                }
                break;
            case 1:
                a[0] = spot->pos[0];
                a[1] = spot->pos[1];
                a[2] = spot->pos[2];
                b[0] = spot->pos2[0];
                b[1] = spot->pos2[1];
                b[2] = spot->pos2[2];
                if (Slab_Contains(p->pos, a, b, spot->height, spot->radius) != 0) {
                    hit = 1;
                } else {
                    hit = 0;
                }
                break;
            }
            if (hit != 0) {
                p->areaMoveState = 1;
                p->nextArea = Area_IdForCurrentTime(spot->id);
                p->startPos[0] = spot->target[0];
                p->startPos[1] = spot->target[1];
                p->startPos[2] = spot->target[2];
                p->startAngle = spot->angle + 0x4000;
                Player_AddSpawnOffset(p, p->startPos, p->startAngle);
                if (gScene.online == 0) {
                    QSync_SendPlayer(p, 5, 0);
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
 * parity of counter 0 / 7200. Kept as it is while Quest_IsRunning(1).
 */
s16 Area_MonsterState(u16 area)
{
    Em *e = gEms;
    AreaState *q = &gAreaState;
    s16 i;

    switch (gAreaInfo[area].unk0) {
    case 8:
    case 0xD:
    case 0xE:
        if (Quest_IsRunning(1) != 0) {
            return q->monsterState;
        }
        for (i = 0; i < 20; i++, e++) {
            if (e->active != 0 && e->visible != 0 && (e->type == 0x36 || e->type == 0x3C) && e->area == area) {
                q->monsterState = 2;
                return q->monsterState;
            }
        }
        if (Quest_GetTime(1) - Quest_GetTime(0) < 900) {
            q->monsterState = 0;
        } else if ((Quest_GetTime(0) / 7200) & 1) {
            q->monsterState = 0;
        } else {
            q->monsterState = 1;
        }
        return q->monsterState;
    }
    return 0;
}

s16 Area_Temperature(u16 area)
{
    return gAreaTemperatures[area];
}

u16 Area_BaseId(u16 area)
{
    u16 v = gAreaBaseIds[area];

    if (v != 0xFFFF) {
        return v;
    }
    return area;
}

u16 Area_MapGroup(u16 area)
{
    u16 v = Area_BaseId(area);

    switch (v) {
    case 0xE:
    case 0xF:
        v = 0xD;
        break;
    }
    return v;
}

/* Area ids come in day/night pairs: gAreaTimeOfDay gives an id's time of day,
 * gAreaOtherTime the other one's id. */
s32 Area_IsDay(u16 area)
{
    if (area == 0xFFFF) {
        return 1;
    } else if (gAreaTimeOfDay[area] == 1) {
        return 0;
    } else {
        return 1;
    }
}

u16 Area_OtherTimeId(u16 area)
{
    u16 v;

    if (area == 0xFFFF) {
        return 0xFFFF;
    }
    v = gAreaOtherTime[area];
    if (v != 0xFFFF) {
        return v;
    }
    return area;
}

u16 Area_DayId(u16 area)
{
    if (area == 0xFFFF) {
        return 0xFFFF;
    }
    if (Area_IsDay(area) == 0) {
        return Area_OtherTimeId(area);
    }
    return area;
}

/* The id of the area for the current time of day (0 online). */
u16 Area_IdForCurrentTime(u16 area)
{
    s16 v;

    if (area == 0xFFFF) {
        return 0xFFFF;
    }
    v = gScene.online == 0 ? Daytime_Get() : 0;
    if (v != gAreaTimeOfDay[area]) {
        return Area_OtherTimeId(area);
    }
    return area;
}

/* The index of an area in its map's list; 0xFFFF when not there. */
u16 Map_AreaIndex(u16 map, u16 area)
{
    s16 i;

    for (i = 0; i < gMapAreaCount[map]; i++) {
        if (area == gMapAreas[map][i]) {
            return i;
        }
    }
    return 0xFFFF;
}

/* The area id at index i of a map's list. */
u16 Map_AreaAt(u16 map, u16 i)
{
    if (map != 0xFFFF && i != 0xFFFF) {
        return gMapAreas[map][i];
    }
    return 0xFFFF;
}

s32 Area_NoBarrelBombs(u16 area)
{
    switch (area) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 0x12:
    case 0x13:
    case 0x4D:
    case 0x4E:
    case 0x4F:
    case 0x50:
    case 0x51:
    case 0x52:
    case 9:
    case 0x97:
    case 0x98:
    case 0x99:
    case 0x9C:
    case 0x9D:
    case 0x9E:
    case 0xA0:
        return 1;
    case 0x60:
    case 0x61:
    case 0x64:
    case 0x69:
    case 0x6A:
    case 0x6D:
        return Area_MonsterState(area) == 2;
    default:
        return 0;
    }
}

f32 Area_GroundOffset(u16 area)
{
    switch (area) {
    case 0x48:
    case 0x4B:
        return 0.0f;
    }
    return 0.0f;
}

s32 func_00379E40(void)
{
    return gAreaState.flyer != 0;
}

void func_00379E50(f32 *out, u16 area)
{
    if (gAreaInfo[area].unk3 != 0xFF) {
        Vec_Copy(out, gAreaState.flyerPos);
    }
}

void Area_StartTimers(AreaState *s)
{
    s->timerState++;
    s->frame = 0;
}

/* unk_376C00's .data (0x43ECF0-0x441150) */
extern u32 gAreaKindFlags_170[];
extern u32 D_0045F080[];
extern u32 gPartLodModels_170[];
extern u32 D_006AEDF8[];
extern u32 D_006AEE18[];
VibPattern gVibPatterns[6] = {
#include "sub_main/0043ECF0.inc"
};
f32 D_0043ED20[6] = {
#include "sub_main/0043ED20.inc"
};
f32 D_0043ED38[3] = {
#include "sub_main/0043ED38.inc"
};
CamPreset gCameraPresets[3] = {
#include "sub_main/0043ED50.inc"
};
u16 gAreaBaseIds[32] = {
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
u32 gPartLodRecs[96] = {
#include "sub_main/0043FE90.inc"
};
u32 D_00440010[104] = {
#include "sub_main/00440010.inc"
};
u32 gPartLodRecs2[200] = {
#include "sub_main/004401B0.inc"
};
u32 gPartLodModels[200] = {
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
