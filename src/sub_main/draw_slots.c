/*
 * The draw slot pools (DrawSlot) and the ordering tables the slots go into:
 * gDrawSlotPool, gDrawSlotsB and gHudDrawHooks, each with a first-free hint;
 * OT_AddByDepth sorts a slot into a table by view depth, OT_AddAt puts it
 * at a bucket, and Task_RunList runs a table's or a list's draw callbacks.
 * Before them, the players' world matrices (Player_UpdateWorldMatrix, with
 * Player_ScaleBones for the bones, and Players_UpdateMatrices, which runs
 * it on the active players), then two small helpers: counting loaded
 * textures and handing a material set to the render states. After them,
 * colours as floats.
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
#include "area_draw.h"
#include "pad.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "main.h"
#include "libc.h"
#include "engine_util.h"
#include "vec_math.h"

extern DrawSlot gHudDrawHooks[4];
extern DrawSlot gDrawSlotsB[256];
extern DrawSlot gDrawSlotPool[512];
extern s32 gDrawSlotBNext;          /* first free entry of gDrawSlotsB */
extern s32 gDrawSlotNext;          /* first free entry of gDrawSlotPool */
extern s32 gTexHandles[];
extern LobbyState gLobbyState;

struct MtxSlot;
struct EmBone;

/* Scales the player's first 21 bones each by its part's scale (partRecs[i]->scale), after
 * Player_TwistBoneToLook turns it, updates them under the matrix parent, and copies each bone's
 * world matrix back into its part's record (m1). */
void Player_ScaleBones(Player *pl, EmBone *bones, f32 *parent)
{
    f32 scaleMtx[16];
    f32 scaled[16];
    s32 i;
    EmBone *bone;

    for (i = 0, bone = bones; i < 21; i++, bone++) {
        Player_TwistBoneToLook(pl, bone->index);
        Mtx_Scale(scaleMtx, pl->partRecs[i]->scale[0], pl->partRecs[i]->scale[1], pl->partRecs[i]->scale[2]);
        Mtx_Mul(scaled, bone->local, scaleMtx);
        bone->local[12] = scaled[12];
        bone->local[13] = scaled[13];
        bone->local[14] = scaled[14];
    }
    Bone_UpdateWorldScaledRoot(bones, parent);
    Bone_StoreMatrices(bones);
    for (i = 0; i < 21; i++, bones++) {
        Mtx_Copy(pl->partRecs[bones->index]->m1, bones->world);
    }
}

/* For a player being drawn (visible): its world matrix from its scale, angles and position, kept
 * in worldMtx[0], then its bones placed under it (Player_ScaleBones). */
void Player_UpdateWorldMatrix(Player *pl)
{
    f32 world[16];
    f32 angles[3];
    Model *model;

    if (pl->visible == 0) {
        return;
    }
    model = pl->model;
    Vec_AnglesToRad(pl->rot, angles);
    Mtx_Scale(world, pl->scale[0], pl->scale[1], pl->scale[2]);
    Mtx_RotateYXZ(world, angles[0], angles[1], angles[2]);
    Mtx_SetTrans(world, pl->pos[0], pl->pos[1], pl->pos[2]);
    Mtx_Copy(pl->worldMtx[0], world);
    Bone_UpdateWorld(model->partBones[0], world);
    Player_ScaleBones(pl, model->partBones[0], world);
}

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
    DrawPool_Clear(gDrawSlotPool, 512);
    gDrawSlotNext = 0;
}

void DrawSlotB_ResetAll(void)
{
    DrawPool_Clear(gDrawSlotsB, 256);
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

    if (index >= 512) {
        return -1;
    }
    slot = DrawSlot_Get(index + 1);
    for (j = gDrawSlotNext + 1; j < 512; j++, slot++) {
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

    if (index >= 256) {
        return -1;
    }
    slot = DrawSlotB_Get(index + 1);
    for (j = gDrawSlotBNext + 1; j < 256; j++, slot++) {
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
 * Adds node to an ordering table of n buckets by its view depth (MTX_SLOT_VIEW):
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
    MtxSlot_Load((struct MtxSlot *) viewMtx, 33);
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
    if (bucketCount == 64) {
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
