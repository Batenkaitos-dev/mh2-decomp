/*
 * The fishing, and the countdown effect that sits beside it in sub_main's code. Both are
 * effect objects of the stage object list (FxObj: include/fxobj.h).
 * - The fish (kind 23, FishWork): Effect17_SpawnArea starts them at the area's fishing
 *   spots (FishSpot). Each swims around its home, goes for a local player's lure, nibbles,
 *   bites (Player.fishTimer) and, once struck, follows the lure out. Its record (FishKind)
 *   comes from gFishKindsQuest, or gFishKindsLobby in the town.
 * - The lure (STAGEOBJ_KIND_LURE, LureWork): Lure_Spawn starts it from the cast; it follows the
 *   player's fishing motions (PL_MOT_FISH_*) and draws the line from the rod tip.
 * - The countdown (kind 44, Effect2C_Spawn): black bars and the 3, 2, 1 of its last
 *   90 frames (versus, the town's games).
 * The .data holds the fishing tables, the town's stage data and the player select and
 * item box text tables, whose texts make up most of the .rodata.
 *
 * One file: no code here reads the town's stage data or the text tables, but the stage
 * data lies among the fishing tables, and the texts lie before the countdown's own
 * .rodata (a short literal, then Countdown_Draw's jump table). That
 * .rodata doesn't start on the 16-byte boundary a file of its own would start at, so
 * neither the countdown nor the texts can be cut out.
 */
#include "common.h"
#include "stageobj.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "sprite.h"
#include "prim.h"
#include "fxobj.h"
#include "area.h"
#include "render_state.h"
#include "sound.h"
#include "fishing.h"
#include "card.h"
#include "quest.h"
#include "text.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "font.h"
#include "rand.h"
#include "stage_data.h"
#include "vibration.h"
#include "vec_math.h"
#include "spark_fx.h"

struct FxObj;
struct DrawSlot;

/* A fish's update (kind 23): runs its state, Fish_Start, Fish_Update, Fish_End or Fish_Free. */
void Fish_Run(FxObj *fish)
{
    u8 state;

    state = fish->state;
    switch (state) {
    case 0:
        Fish_Start(fish);
        break;
    case 1:
        Fish_Update(fish);
        break;
    case 2:
        Fish_End(fish);
        break;
    case 3:
        Fish_Free(fish);
        break;
    }
}

/* Next state: takes a draw slot (slotId, slot) that draws with Fish_Draw, then Fish_Update. */
void Fish_Start(FxObj *fish)
{
    fish->state++;
    fish->step = FISH_STEP_SPAWN;
    fish->visible = 1;
    fish->draw = NULL;
    fish->slotId = DrawSlot_Alloc();
    if (fish->slotId != -1) {
        fish->slot = DrawSlot_Get(fish->slotId);
        fish->slot->owner = (void *)fish;
        fish->slot->draw = Fish_Draw;
    } else {
        fish->slot = NULL;
    }
    Fish_Update(fish);
}

/* The heading from a to b, a quarter turn back (the effects' angle convention). */
u16 Fx_HeadingTo(f32 *from, f32 *to)
{
    return Math_Heading(from[0], from[2], to[0], to[2]) - 0x4000;
}

/* Whether the bait the player is using (Player.useItem) draws fish kind fishKind: 1 for
 * ITEM_ID_FISH_BAIT_A, _B, _C and _G and for any other item, 0 for ITEM_ID_FISH_BAIT_LURE_ALT,
 * and for _D, _E and _F 1 only for the kinds listed under each. */
s32 Fish_BaitMatches_Unk(Player *pl, s8 fishKind)
{
    switch (pl->useItem) {
    case ITEM_ID_FISH_BAIT_A:
    case ITEM_ID_FISH_BAIT_B:
    case ITEM_ID_FISH_BAIT_C:
    case ITEM_ID_FISH_BAIT_G:
        break;
    case ITEM_ID_FISH_BAIT_LURE_ALT:
        return 0;
    case ITEM_ID_FISH_BAIT_D:
        if (fishKind == 3 || fishKind == 10 || fishKind == 11 || fishKind == 20 || fishKind == 23) {
            break;
        }
        return 0;
    case ITEM_ID_FISH_BAIT_E:
        if (fishKind == 7 || fishKind == 8 || fishKind == 22) {
            break;
        }
        return 0;
    case ITEM_ID_FISH_BAIT_F:
        if (fishKind == 5) {
            break;
        }
        return 0;
    default:
        return 1;
    }
    return 1;
}

extern FxObj gStageObjs[];

/* A local player's lure (STAGEOBJ_KIND_LURE, in LURE_STEP_BITE, not claimed) that the fish may bite: within its
 * radius of home, allowed for its kind (Fish_BaitMatches_Unk) and in front (within ANGLE_TURN * 3 / 16 of its heading).
 * Mode 0 aims the fish at it; otherwise the fish claims it (its claimed) once within 30. NULL when none. */
FxObj *Fish_FindLure(FxObj *fish, FishWork *w, s32 mode)
{
    s32 i;
    FxObj *lure;
    u16 lureHeading;

    for (i = 0, lure = gStageObjs; i < 128; i++, lure++) { /* the stage object pool */
        if (lure->kind == STAGEOBJ_KIND_LURE && lure->type != LURE_TYPE_ALT && lure->step == LURE_STEP_BITE && lure->claimed == 0
            && Player_IsLocal(lure->owner) == 1 && Fish_BaitMatches_Unk(lure->owner, w->kind)
            && Vec_DistanceXZ(w->home, lure->pos) < w->radius) {
            lureHeading = Fx_HeadingTo(fish->pos, lure->pos);
            if ((u16)(lureHeading - w->angles[1] + ANGLE_TURN * 3 / 16) <= 0x5FFF) {
                if (mode == 0) {
                    w->heading = lureHeading;
                    return lure;
                }
                if ((w->dist = Vec_DistanceXZ(fish->pos, lure->pos)) < 30.0f) {
                    lure->claimed = 1;
                    return lure;
                }
            }
        }
    }
    return NULL;
}

extern FishKind gFishKindsQuest[];
extern FishKind gFishKindsLobby[];

/* The record of fish kind i: gFishKindsQuest's, or gFishKindsLobby's when gScene.inLobby is 1. */
static FishKind *Fish_GetKind(s8 i)
{
    if (gScene.inLobby != 1) {
        return &gFishKindsQuest[i];
    }
    return &gFishKindsLobby[i];
}

/* FISH_STEP_SPAWN: picks the fish's kind (Fish_PickKind; none: the object ends), places it at a
 * random point within half its radius of home, then grows from 0.1 to its kind's size
 * (FishKind.scale) over 10 frames and goes to FISH_STEP_SWIM. */
void Fish_StepSpawn(FxObj *fish, FishWork *w)
{
    FishKind *rec = Fish_GetKind(w->kind);
    f32 forward[3];
    f32 delta[3];
    s32 rotAngles[3];
    f32 rotMtx[16];
    s32 halfRadius;

    switch (fish->phase) {
    case 0:
        fish->phase++;
        w->bright = 0;
        w->kind = Fish_PickKind(fish);
        if (w->kind == 0) {
            fish->state++;
            fish->step = FISH_STEP_SPAWN;
            fish->visible = 0;
        }
        Vec_Copy(fish->pos, w->home);
        rotAngles[0] = 0;
        rotAngles[1] = Rand_Next(1);
        rotAngles[2] = 0;
        Mtx_FromAnglesXYZ(rotAngles, rotMtx);
        halfRadius = (s32)w->radius / 2;
        forward[2] = Rand_Next(1) % halfRadius;
        forward[0] = 0.0f;
        forward[1] = 0.0f;
        Vec_Rotate(delta, forward, rotMtx);
        fish->pos[0] += delta[0];
        fish->pos[1] += delta[1];
        fish->pos[2] += delta[2];
        fish->timer = 10;
        w->angles[2] = 0;
        w->angles[0] = 0;
        w->heading = w->angles[1] = (Rand_Next(1) & 0xF) << 12;
        w->turn = 32;
        w->wiggle = 0;
        w->scale[0] = w->scale[1] = w->scale[2] = 0.1f;
        w->angles[0] = 0;
        break;
    case 1:
        w->scale[0] += rec->scale[0] / 10.0f;
        w->scale[1] += rec->scale[1] / 10.0f;
        w->scale[2] += rec->scale[2] / 10.0f;
        if (--fish->timer <= 0.0f) {
            fish->step = FISH_STEP_SWIM;
            fish->phase = 0;
            fish->timer = (Rand_Next(1) & 0x1F) + 10;
            w->scale[0] = rec->scale[0];
            w->scale[1] = rec->scale[1];
            w->scale[2] = rec->scale[2];
        }
        break;
    }
}


/* FISH_STEP_SWIM: turns toward a heading (a lure's, or a random one back toward home), then
 * swims forward at a slowing speed; on a lure whose player's baited is set, FISH_STEP_BITE. */
void Fish_StepSwim(FxObj *fish, FishWork *w)
{
    f32 forward[3];
    f32 delta[3];
    f32 rotMtx[16];
    s32 homeHeading;

    switch (fish->phase) {
    case 0:
        if (--fish->timer <= 0 || w->angles[1] - w->heading == 0) {
            fish->phase++;
            fish->scale = 3.0f;
            w->wiggle = 256;
            if (Fish_FindLure(fish, w, 0) == NULL) {
                homeHeading = Fx_HeadingTo(fish->pos, w->home);
                w->heading = homeHeading - (ANGLE_TURN / 4) + ((Rand_Next(1) & 0xF) << 11);
            }
        }
        w->turn += (32 - w->turn) / 8;
        Angle_TurnToward(&w->angles[1], &w->heading, w->turn);
        break;
    case 1:
        forward[0] = 0.0f;
        forward[1] = 0.0f;
        forward[2] = fish->scale;
        Mtx_FromAnglesXYZ(w->angles, rotMtx);
        Vec_Rotate(delta, forward, rotMtx);
        fish->pos[0] += delta[0];
        fish->pos[1] += delta[1];
        fish->pos[2] += delta[2];
        w->wiggle = -w->wiggle;
        w->wiggle -= w->wiggle / 2;
        if ((w->lure = Fish_FindLure(fish, w, 1)) != NULL && w->lure->owner->baited != 0) {
            fish->step = FISH_STEP_BITE;
            fish->phase = 0;
            fish->timer = 3;
            w->wiggle = 0;
        } else if ((fish->scale += -0.1f) <= 0.0f) {
            fish->phase = 0;
            fish->timer = (Rand_Next(1) & 0x1F) + 10;
            w->wiggle = 0;
        }
        w->turn += (256 - w->turn) / 8;
        Angle_TurnToward(&w->angles[1], &w->heading, w->turn);
        break;
    }
}

s16 Player_IsState(void *obj, u8 a, u8 b);
extern u16 gFishBiteJitter[];

/* FISH_STEP_BITE: at the lure. It nibbles (up to its kind's count, none with ITEM_ID_FISH_BAIT_G), darting
 * forward and back, then bites: Player.fishTimer gets the kind's value (doubled with SKILL_FISHING_BITE_X2) and
 * a vibration; a nibble splashes (effect 7) and vibrates. FISH_STEP_SWIM when the lure is gone or the player
 * reels in without a bite (PL_ACT_FISH_REEL_BAITED or PL_ACT_FISH_REEL_EMPTY; in the town,
 * PL_TOWN_FISH_REEL_EMPTY or PL_TOWN_FISH_REEL_BAITED); FISH_STEP_HOOKED when he strikes (PL_ACT_FISH_REEL;
 * in the town PL_TOWN_FISH_REEL) with no catch pending (gotItemId, set to the kind's item). */
void Fish_StepBite(FxObj *fish, FishWork *w)
{
    FxObj *lure = w->lure;
    FishKind *rec = Fish_GetKind(w->kind);
    f32 vec[3];
    f32 delta[3];
    f32 rotMtx[16];
    f32 *vecY;
    f32 *vecZ;

    if (gScene.inLobby != 1) {
        if (lure->kind != STAGEOBJ_KIND_LURE || lure->active == 0 || lure->state >= 2
            || Player_IsState(lure->owner, PL_GROUP_ACTION, PL_ACT_FISH_REEL_BAITED) || Player_IsState(lure->owner, PL_GROUP_ACTION, PL_ACT_FISH_REEL_EMPTY)) {
            fish->step = FISH_STEP_SWIM;
            fish->phase = 0;
            fish->timer = (Rand_Next(1) & 0x1F) + 10;
            return;
        }
        if (lure->owner->gotItemId == 0 && Player_IsState(lure->owner, PL_GROUP_ACTION, PL_ACT_FISH_REEL)) {
            fish->step = FISH_STEP_HOOKED;
            fish->phase = 0;
            lure->owner->gotItemId = rec->item;
            return;
        }
    } else {
        if (lure->kind != STAGEOBJ_KIND_LURE || lure->active == 0 || lure->state >= 2
            || Player_IsState(lure->owner, PL_GROUP_ACTION, PL_TOWN_FISH_REEL_EMPTY) || Player_IsState(lure->owner, PL_GROUP_ACTION, PL_TOWN_FISH_REEL_BAITED)) {
            fish->step = FISH_STEP_SWIM;
            fish->phase = 0;
            fish->timer = (Rand_Next(1) & 0x1F) + 10;
            return;
        }
        if (lure->owner->gotItemId == 0 && Player_IsState(lure->owner, PL_GROUP_ACTION, PL_TOWN_FISH_REEL)) {
            fish->step = FISH_STEP_HOOKED;
            fish->phase = 0;
            lure->owner->gotItemId = rec->item;
            return;
        }
    }
    switch (fish->phase) {
    case 0:
        fish->phase++;
        if (lure->owner->useItem == ITEM_ID_FISH_BAIT_G) {
            w->nibbles = 0;
        } else {
            w->nibbles = Rand_Next(1) % rec->nibbles + 1;
        }
    case 1:
        if (--w->nibbles <= 0) {
            fish->phase = 5;
        } else {
            fish->phase++;
        }
        fish->timer = 5;
        fish->scale = Vec_DistanceXZ(fish->pos, lure->pos) / fish->timer;
        w->heading = w->angles[1] = Fx_HeadingTo(fish->pos, w->lure->pos);
        break;
    case 2:
    case 5:
        vec[0] = 0.0f;
        vecY = &vec[1];
        *vecY = 0.0f;
        vecZ = &vec[2];
        *vecZ = fish->scale;
        Mtx_FromAnglesXYZ(w->angles, rotMtx);
        Vec_Rotate(delta, vec, rotMtx);
        fish->pos[0] += delta[0];
        fish->pos[1] += delta[1];
        fish->pos[2] += delta[2];
        if (--fish->timer <= 0) {
            if (fish->phase == 5) {
                lure->owner->fishTimer = rec->biteTime + gFishBiteJitter[Rand_Next(1) & 3];
                if (Player_HasSkill(lure->owner, SKILL_FISHING_BITE_X2) == 1) {
                    lure->owner->fishTimer *= 2;
                }
                if (!(Rand_Next(1) & 3)) {
                    lure->owner->baited = 0;
                }
                Vib_StartLocal(lure->owner, 1);
            } else {
                vec[0] = lure->pos[0];
                *vecY = 10.0f + Lure_GetWaterHeight(lure);
                *vecZ = lure->pos[2];
                SparkFx_Spawn(vec, 7, 0, 0.5f);
                Vib_StartLocal(lure->owner, 4);
            }
            fish->phase++;
            fish->scale = -5.0f;
            w->accel = 0.5f;
        }
        break;
    case 3:
    case 6:
        vec[0] = 0.0f;
        vec[1] = 0.0f;
        vec[2] = fish->scale;
        Mtx_FromAnglesXYZ(w->angles, rotMtx);
        Vec_Rotate(delta, vec, rotMtx);
        fish->pos[0] += delta[0];
        fish->pos[1] += delta[1];
        fish->pos[2] += delta[2];
        if ((fish->scale += w->accel) >= 0.0f) {
            fish->phase++;
            fish->timer = 20;
        }
        break;
    case 4:
        if (--fish->timer <= 0) {
            fish->phase = 1;
        }
        break;
    case 7:
        if (--fish->timer <= 0) {
            fish->phase++;
            fish->timer = (Rand_Next(1) & 0x1F) + 60;
            w->heading = w->angles[1] + ((Rand_Next(1) & 7) << 12);
            lure->claimed = 0;
            fish->scale = 8.0f;
            w->accel = -0.3f;
        }
        break;
    case 8:
        vec[0] = 0.0f;
        vec[1] = 0.0f;
        vec[2] = fish->scale;
        Mtx_FromAnglesXYZ(w->angles, rotMtx);
        Vec_Rotate(delta, vec, rotMtx);
        fish->pos[0] += delta[0];
        fish->pos[1] += delta[1];
        fish->pos[2] += delta[2];
        if ((fish->scale += w->accel) <= 0.0f) {
            fish->step = FISH_STEP_SWIM;
            fish->phase = 0;
            fish->timer = (Rand_Next(1) & 0x1F) + 10;
        }
        break;
    }
    Angle_TurnToward(&w->angles[1], &w->heading, 0x100);
}


/* FISH_STEP_HOOKED: it follows the lure (turning toward it, a vibration every 10 frames)
 * until the lure's LURE_STEP_PULL_OUT (then bright); FISH_STEP_WAIT when the lure is gone. */
void Fish_StepHooked(FxObj *fish, FishWork *w)
{
    FxObj *lure = w->lure;

    if (lure->kind != STAGEOBJ_KIND_LURE || lure->active == 0 || lure->state >= 2) {
        fish->step = FISH_STEP_WAIT;
        fish->phase = 0;
        return;
    }
    switch (fish->phase) {
    case 0:
        fish->phase++;
        fish->timer = 10;
    case 1:
        w->heading = Fx_HeadingTo(fish->pos, lure->pos);
        Angle_TurnToward(&w->angles[1], &w->heading, ANGLE_TURN / 32);
        fish->pos[0] = lure->pos[0];
        fish->pos[2] = lure->pos[2];
        if (--fish->timer <= 0) {
            fish->timer = 10;
            Vib_StartLocal(lure->owner, 1);
        }
        if (lure->step > LURE_STEP_REEL) {
            fish->phase++;
            w->bright = 1;
            Vib_StartLocal(lure->owner, 3);
        }
        break;
    case 2:
        w->angles[0] = -(ANGLE_TURN / 4);
        Vec_Copy(fish->pos, lure->pos);
        break;
    }
}

extern s8 gFishWaitSecs[];


/* FISH_STEP_WAIT: waits 30 to 30 x gFishWaitSecs[] frames at home (velocity
 * zero), then starts again at FISH_STEP_SPAWN. */
void Fish_StepWait(FxObj *fish, FishWork *w)
{
    switch (fish->phase) {
    case 0:
        fish->phase++;
        w->bright = 0;
        fish->timer = gFishWaitSecs[Rand_Next(1) & 3] * 30;
        w->scale[0] = 0.0f;
        w->scale[1] = 0.0f;
        w->scale[2] = 0.0f;
        Vec_Copy(fish->pos, w->home);
        break;
    case 1:
        if (--fish->timer <= 0.0f) {
            fish->step = FISH_STEP_SPAWN;
            fish->phase = 0;
        }
        break;
    }
}

/* A fish's update: its step, kept within radius of home (horizontally), then drawn. */
void Fish_Update(FxObj *fish)
{
    u8 step = fish->step;
    FishWork *w = fish->work;
    f32 fromHome[3];
    f32 *fromHomeY;

    switch (step) {
    case FISH_STEP_SPAWN:
        Fish_StepSpawn(fish, w);
        break;
    case FISH_STEP_SWIM:
        Fish_StepSwim(fish, w);
        break;
    case FISH_STEP_BITE:
        Fish_StepBite(fish, w);
        break;
    case FISH_STEP_HOOKED:
        Fish_StepHooked(fish, w);
        break;
    case FISH_STEP_WAIT:
        Fish_StepWait(fish, w);
        break;
    }
    if (fish->visible) {
        if (fish->step <= FISH_STEP_BITE) {
            Vec_Sub(fromHome, fish->pos, w->home);
            fromHomeY = &fromHome[1];
            *fromHomeY = 0.0f;
            if (Vec_Length(fromHome) > w->radius) {
                Vec_Normalize(fromHome);
                fish->pos[0] = w->home[0] + fromHome[0] * w->radius;
                fish->pos[1] = w->home[1] + *fromHomeY * w->radius;
                fish->pos[2] = w->home[2] + fromHome[2] * w->radius;
            }
        }
        fish->slot->pos[0] = fish->pos[0];
        fish->slot->pos[1] = fish->pos[1];
        fish->slot->pos[2] = fish->pos[2];
        OT_AddByDepth(gDrawOt32, fish->slot, 32, 0);
    }
}

void Fish_End(FxObj *fish)
{
    fish->state++;
    DrawSlot_Free(fish->slotId);
}

void Fish_Free(FxObj *fish)
{
    StageObj_Free((struct StageObj *)fish);
}

extern u8 gFishColors[][3];

/* Draws a fish (the draw slot's update): its kind's model scaled, turned and placed, in its
 * kind's color (gFishColors), at half brightness unless its bright is set. */
void Fish_Draw(DrawSlot *slot)
{
    FxObj *fish = (FxObj *)slot->owner;
    FishWork *w = fish->work;
    FishKind *rec = Fish_GetKind(w->kind);
    Model *set = gCommonModels[0];
    f32 mtx[16];
    ModelMat *base;
    ModelMatSet *matSet;
    f32 brightness;
    u32 color;

    if (set == NULL || set->active == 0) {
        return;
    }
    Gs_SetState(RS_ALPHA_REF, 128);
    matSet = &set->matSets[rec->model];
    base = set->mats;
    Mtx_Scale(mtx, w->scale[0], w->scale[1], w->scale[2]);
    Mtx_RotateZXY(mtx, 6.2831855f * w->angles[0] / 65536.0f, 6.2831855f * (w->angles[1] + w->wiggle) / 65536.0f,
                  6.2831855f * w->angles[2] / 65536.0f);
    Mtx_SetTrans(mtx, fish->pos[0], fish->pos[1], fish->pos[2]);
    Gs_SetState(RS_COLOR2, -1);
    if (matSet != NULL && matSet->id != -1) {
        Gs_SetState(RS_ALPHA_REF, 128);
        Gs_SetState(RS_MTX_SLOT(0), (u32)mtx);
        if (w->bright == 0) {
            brightness = 0.5f;
        } else {
            brightness = 1.0f;
        }
        color = 0xFF000000 | ((u8)(brightness * gFishColors[rec->color][0]) << 16) | ((u8)(brightness * gFishColors[rec->color][1]) << 8) | (u8)(brightness * gFishColors[rec->color][2]);
        Gs_SetState(RS_COLOR2, color);
        Gs_SetLightList(base, matSet);
        Draw_ApplyStates(matSet->flags);
        Model_Draw(matSet->id, 0);
        Draw_SetDefaultState();
    }
}

/* A random fish kind from the fish's table, each entry's kind with a chance of its weight
 * in the table's total; 0 past the end. */
s8 Fish_PickKind(FxObj *fish)
{
    FishWork *w = fish->work;
    FishTableEntry *entry;
    u8 total;
    u8 roll;
    u8 sum;

    total = 0;
    for (entry = w->table; entry->weight != 255; entry++) {
        total += entry->weight;
    }
    sum = Rand_Next(0) % total;
    roll = sum;
    for (sum = 0, entry = w->table; entry->weight != 255; entry++) {
        sum += entry->weight;
        if (roll < sum) {
            return entry->kind;
        }
    }
    return 0;
}

/* Starts an effect object of kind 23 (update Fish_Run) from record src: x, y, z to it and to its work's home, the fourth value to its radius. */
void Effect17_Spawn(s32 spotKind, f32 *src, FishTableEntry *table)
{
    FxObj *fish = (FxObj *)StageObj_Alloc(1);

    if (fish != NULL) {
        FishWork *w = fish->work;

        fish->kind = 23;
        fish->update = Fish_Run;
        fish->draw = NULL;
        fish->type = spotKind;
        w->home[0] = fish->pos[0] = *src++;
        w->home[1] = fish->pos[1] = *src++;
        w->home[2] = fish->pos[2] = *src++;
        w->radius = *src;
        w->table = table;
    }
}


/* Starts the fish of an area: for each of its fishing spots (Stage_FindData(area), up to a
 * negative kind), Stage_GetSeasonalData(kind) gives the fish table and the count, and
 * Effect17_Spawn starts that many fish there. Nothing when the area has no spots. */
void Effect17_SpawnArea(s32 area)
{
    FishSpot *spot = Stage_FindData(area);
    FishTableEntry *table;
    s32 fishCount;

    if (spot != NULL) {
        for (;;) {
            if (spot->kind < 0) {
                break;
            }
            table = Stage_GetSeasonalData(spot->kind, &fishCount);
            while (fishCount > 0) {
                Effect17_Spawn(spot->kind, spot->circle, table);
                fishCount--;
            }
            spot++;
        }
    }
}

/* The first fishing spot of an area (the list Effect17_SpawnArea reads): its home x, y, z to
 * pos; returns its radius, or -1.0 when the area has none. The area's update plays a sound
 * there while it is positive. */
f32 Stage_GetDataPoint(s32 area, f32 *pos)
{
    f32 *data;

    data = Stage_FindData(area & 0xFFFF);
    if (data == NULL) {
        return -1.0f;
    }
    pos[0] = data[0];
    pos[1] = data[1];
    pos[2] = data[2];
    return data[3];
}

/* Starts player pl's lure (STAGEOBJ_KIND_LURE, update Lure_Run) of type lureType (1 for
 * ITEM_ID_FISH_BAIT_LURE_ALT, else 0); its colour is the player's (index & 3, gLureColors).
 * Nothing when no object is free. The cast states call it. */
void Lure_Spawn(Player *pl, u8 lureType)
{
    FxObj *lure = (FxObj *)StageObj_Alloc(1);

    if (lure != NULL) {
        lure->kind = STAGEOBJ_KIND_LURE;
        lure->update = Lure_Run;
        lure->draw = NULL;
        lure->owner = pl;
        lure->ownerIndex = (u8)pl->index & 3;
        lure->type = lureType;
    }
}

/* A lure's update (STAGEOBJ_KIND_LURE): runs its state, Lure_Start, Lure_Update, Lure_End or Lure_Free. */
void Lure_Run(FxObj *lure)
{
    u8 state;

    state = lure->state;
    switch (state) {
    case 0:
        Lure_Start(lure);
        break;
    case 1:
        Lure_Update(lure);
        break;
    case 2:
        Lure_End(lure);
        break;
    case 3:
        Lure_Free(lure);
        break;
    }
}

/* The lure's first state: at the rod (bone 14 of the player), a random throw velocity turned to
 * the player's heading, its scale (0.3 for variant 1), the water height of the area (waterY), and a
 * draw slot drawn by Lure_Draw (none: the lure ends). */
void Lure_Start(FxObj *lure)
{
    Player *pl = lure->owner;
    LureWork *w = lure->work;

    lure->state++;
    lure->step = LURE_STEP_FLY;
    lure->phase = 0;
    lure->visible = 1;
    lure->draw = NULL;
    Em_GetBonePos(pl, PL_BONE_HAND, lure->pos);
    Vec_Copy(w->target, lure->pos);
    lure->angle = pl->rot[1];
    w->counter = 0;
    w->vel[0] = Rand_Signed();
    w->vel[1] = 2.0f + 2.0f * Rand_Unit();
    w->vel[2] = 30.0f + 2.0f * Rand_Unit();
    w->gravity = -2.0f;
    switch (lure->type) {
    default:
        w->scale = 1.0f;
        break;
    case LURE_TYPE_ALT:
        w->scale = 0.3f;
        break;
    }
    switch (Area_DayId(gScene.area)) {
    case AREA_JUNGLE_NIGHT_0:
        w->waterY = -94.0f;
        break;
    case AREA_JUNGLE_NIGHT_2:
        w->waterY = -131.0f;
        break;
    case AREA_FOREST_HILLS_0:
        w->waterY = -10.0f;
        break;
    case AREA_FOREST_HILLS_11:
        w->waterY = -45.0f;
        break;
    case AREA_DESERT_NIGHT_2:
        w->waterY = -20.0f;
        break;
    case AREA_VOLCANO_NIGHT_1:
        w->waterY = -222.0f;
        break;
    case AREA_VOLCANO_NIGHT_3:
        w->waterY = -41.0f;
        break;
    case AREA_SWAMP_NIGHT_2:
        w->waterY = -20.0f;
        break;
    case AREA_VOLCANO_9:
        w->waterY = -10.0f;
        break;
    case AREA_SWAMP_NIGHT_9:
        w->waterY = -20.0f;
        break;
    case AREA_SNOWY_MOUNTAINS_7:
        w->waterY = -50.0f;
        break;
    case AREA_JUNGLE_0:
        w->waterY = -57.0f;
        break;
    case AREA_JUNGLE_3:
        w->waterY = -12.0f;
        break;
    case AREA_JUNGLE_4:
        w->waterY = -64.0f;
        break;
    case AREA_JUNGLE_6:
        w->waterY = -37.0f;
        break;
    case AREA_TOWER_0:
        w->waterY = -447.0f;
        break;
    case AREA_TOWER_8:
        w->waterY = -270.0f;
        break;
    case AREA_HUB_0x84:
        w->waterY = 810.0f;
        break;
    case AREA_HUB_0x89:
        w->waterY = -595.0f;
        break;
    case AREA_DESERT_2:
        w->waterY = 30.0f;
        break;
    case AREA_DESERT_4:
        w->waterY = 0.0f;
        break;
    case AREA_DESERT_6:
        w->waterY = 5.0f;
        break;
    case AREA_SWAMP_0:
        w->waterY = -47.5f;
        break;
    case AREA_SWAMP_3:
        w->waterY = -10.0f;
        break;
    case AREA_BATTLE_TOWN_0:
        w->waterY = 810.0f;
        break;
    default:
        w->waterY = 0.0f;
        break;
    }
    w->line = 0;
    Vec_RotateY(w->vel, 6.2831855f * lure->angle / 65536.0f);
    lure->slotId = DrawSlot_Alloc();
    if (lure->slotId != -1) {
        lure->slot = DrawSlot_Get(lure->slotId);
        lure->slot->owner = (void *)lure;
        lure->slot->draw = Lure_Draw;
    } else {
        StageObj_Free((struct StageObj *) lure);
    }
}

/* Moves the lure by its velocity, then adds its gravity to the vertical velocity. */
void Lure_ApplyVelocity(FxObj *lure, LureWork *w)
{
    lure->pos[0] += w->vel[0];
    lure->pos[1] += w->vel[1];
    lure->pos[2] += w->vel[2];
    w->vel[1] += w->gravity;
}

/* Effects of kind 8 above pos (at height y + 15): variant 7, or 3 and 0 (k 1), or 4 and 1 (k 2) 5 higher. */
void Fx_SpawnSplash(f32 *pos, f32 y, f32 scale, u8 pattern)
{
    f32 at[3];
    f32 *atY = &at[1];

    at[0] = pos[0];
    *atY = 15.0f + y;
    at[2] = pos[2];
    switch (pattern) {
    default:
        SparkFx_Spawn(at, 7, 0, scale);
        break;
    case 1:
        SparkFx_Spawn(at, 3, 0, scale);
        *atY += 5.0f;
        SparkFx_Spawn(at, 0, 0, scale);
        break;
    case 2:
        SparkFx_Spawn(at, 4, 0, scale);
        *atY += 5.0f;
        SparkFx_Spawn(at, 1, 0, scale);
        break;
    }
}

/* The lure while it is out, by its step (LURE_STEP_FLY to LURE_STEP_IN_HAND); then kept
 * within 100 of its target, drawn, and ended when the player leaves the area or drops the rod. */
void Lure_Update(FxObj *lure)
{
    LureWork *w;
    Player *pl;
    f32 vec[3];
    f32 size;
    f32 targetDist;
    s16 reelFrames;

    w = lure->work;
    pl = lure->owner;
    lure->timer++;
    switch (lure->step) {
    case LURE_STEP_FLY:
        Lure_ApplyVelocity(lure, w);
        if (lure->pos[1] < w->waterY - 5.0f) {
            lure->pos[1] = w->waterY - 5.0f;
            Vec_Copy(w->target, lure->pos);
            Fx_SpawnSplash(w->target, w->waterY, 1.0f, 0);
            w->vel[0] = 0.0f;
            w->vel[1] = 0.0f;
            w->vel[2] = 0.0f;
            w->gravity = 0.0f;
            lure->step++;
            w->counter = 0;
        }
        break;
    case LURE_STEP_LANDED:
        if (Player_TestFlag(pl, PLF_FISHING) == 0) {
            lure->state++;
            return;
        }
        if (pl->motionId[0] == PL_MOT_FISH_WAIT) {
            lure->step++;
        }
        break;
    case LURE_STEP_BITE:
        if (Player_TestFlag(pl, PLF_FISHING) == 0) {
            lure->state++;
            return;
        }
        if (w->counter < pl->fishTimer) {
            Fx_SpawnSplash(lure->pos, w->waterY, 1.0f, 0);
            Fishing_PlaySe(lure, lure->pos, 0);
            w->vel[1] -= 20.0f + pl->fishTimer;
            lure->ownerIndex |= LURE_PULLED_UNDER;
        }
        w->counter = pl->fishTimer;
        if (w->counter <= 0) {
            lure->ownerIndex &= ~LURE_PULLED_UNDER;
        }
        if (lure->pos[1] > w->target[1]) {
            lure->pos[1] = w->target[1];
        }
        if (lure->pos[1] < w->target[1]) {
            w->gravity = 1.0f;
        } else {
            w->vel[1] *= 0.2f;
            w->gravity = 2.0f;
        }
        if ((lure->ownerIndex & LURE_PULLED_UNDER) && w->vel[1] > 0.0f) {
            w->vel[1] = 0.0f;
        }
        switch (pl->motionId[0]) {
        case PL_MOT_FISH_REEL:
            lure->ownerIndex &= ~LURE_PULLED_UNDER;
            lure->step++;
            w->vel[0] = lure->pos[0] - pl->pos[0];
            w->vel[2] = lure->pos[2] - pl->pos[2];
            Vec_Normalize(w->vel);
            switch (pl->gotItemId) {
            case 238:
            case 239:
            case 236:
                Vec_Scale(w->vel, w->vel, 1.0f);
                w->strongPull = 0;
                break;
            default:
                Vec_Scale(w->vel, w->vel, 3.0f);
                w->strongPull = 1;
                break;
            }
            Vec_RotateY(w->vel, (6.2831855f * (f32)((Rand_Next(1) & 0x3F) - 32)) / 65536.0f);
            break;
        case PL_MOT_FISH_CATCH:
            lure->ownerIndex &= ~LURE_PULLED_UNDER;
            w->mode = LURE_MODE_CATCH;
            Lure_Cast(lure, w);
            break;
        case PL_MOT_FISH_REEL_EMPTY:
            if (Motion_ReachedFrame(pl, 0, 26.0f)) {
                lure->ownerIndex &= ~LURE_PULLED_UNDER;
                w->mode = LURE_MODE_EMPTY;
                Lure_Cast(lure, w);
            }
            break;
        case PL_MOT_FISH_PULLED_DOWN:
            lure->ownerIndex &= ~LURE_PULLED_UNDER;
            w->mode = LURE_MODE_PULLED;
            Lure_Cast(lure, w);
            break;
        }
        Lure_ApplyVelocity(lure, w);
        break;
    case LURE_STEP_REEL:
        Vec_Copy(vec, w->vel);
        if (w->strongPull == 0) {
            Vec_Scale(vec, vec, 0.2f);
            Vec_RotateY(vec, (6.2831855f * (f32)((Rand_Next(1) & 0x7FFF) - 16384)) / 65536.0f);
            size = 1.5f;
        } else {
            Vec_Scale(vec, vec, 0.5f);
            Vec_RotateY(vec, (6.2831855f * (f32)((Rand_Next(1) & 0x3FFF) - 8192)) / 65536.0f);
            size = 3.0f;
        }
        Vec_Add(w->vel, w->vel, vec);
        if (lure->pos[1] > w->target[1]) {
            lure->pos[1] = w->target[1];
        }
        if ((lure->timer & 7) == 0) {
            Fx_SpawnSplash(lure->pos, w->waterY, size, 1);
            Fishing_PlaySe(lure, lure->pos, 0);
        }
        if (lure->pos[1] < w->target[1]) {
            w->gravity = 1.0f;
        } else {
            w->vel[1] *= 0.2f;
            w->gravity = 2.0f;
        }
        switch (pl->motionId[0]) {
        case PL_MOT_FISH_REEL:
            break;
        case PL_MOT_FISH_CATCH:
            if (Motion_PassedFrame(pl, 0, 16.0f)) {
                w->mode = LURE_MODE_CATCH;
                Lure_Cast(lure, w);
                Fx_SpawnSplash(lure->pos, w->waterY, size, 2);
                Fishing_PlaySe(lure, lure->pos, 1);
            }
            break;
        default:
            lure->state++;
            return;
        }
        Lure_ApplyVelocity(lure, w);
        break;
    case LURE_STEP_PULL_OUT:
        if ((w->mode == LURE_MODE_CATCH && pl->motionId[0] == PL_MOT_FISH_CATCH) || (w->mode == LURE_MODE_EMPTY && pl->motionId[0] == PL_MOT_FISH_REEL_EMPTY) ||
            (w->mode == LURE_MODE_PULLED && pl->motionId[0] == PL_MOT_FISH_PULLED_DOWN)) {
            w->counter++;
            switch (w->mode) {
            case LURE_MODE_CATCH:
                size = 0.3f;
                reelFrames = 35;
                break;
            case LURE_MODE_EMPTY:
                size = 0.5f;
                reelFrames = 27;
                break;
            default:
                size = 0.5f;
                reelFrames = 40;
                break;
            }
            switch (lure->type) {
            default:
                if (w->scale > size) {
                    w->scale -= (1.0f - size) / reelFrames;
                    if (w->scale < size) {
                        w->scale = size;
                    }
                }
                break;
            case LURE_TYPE_ALT:
                size = 0.3f;
                break;
            }
            switch (w->mode) {
            case LURE_MODE_CATCH:
            case LURE_MODE_EMPTY:
                if (w->counter >= reelFrames) {
                    Em_GetBonePos(pl, PL_BONE_HAND, lure->pos);
                    lure->pos[1] -= 10.0f;
                    lure->step++;
                    w->counter = 0;
                    w->vel[0] = 0.0f;
                    w->vel[1] = 0.0f;
                    w->vel[2] = 0.0f;
                    w->gravity = 0.0f;
                    w->scale = size;
                }
                break;
            default:
                if (w->counter < reelFrames) {
                    vec[0] = w->target[0] - lure->pos[0];
                    vec[1] = w->target[1] - lure->pos[1];
                    vec[2] = w->target[2] - lure->pos[2];
                    w->vel[0] = 0.09f * vec[0];
                    w->vel[1] = 0.09f * vec[1];
                    w->vel[2] = 0.09f * vec[2];
                } else {
                    lure->state++;
                    return;
                }
                break;
            }
        } else {
            lure->state++;
            return;
        }
        Lure_ApplyVelocity(lure, w);
        break;
    case LURE_STEP_IN_HAND:
        if ((w->mode == LURE_MODE_CATCH && pl->motionId[0] == PL_MOT_FISH_CATCH) || (w->mode == LURE_MODE_EMPTY && pl->motionId[0] == PL_MOT_FISH_REEL_EMPTY)) {
            reelFrames = w->mode == LURE_MODE_CATCH ? 50 : 20;
            if (++w->counter < reelFrames) {
                Em_GetBonePos(pl, PL_BONE_HAND, lure->pos);
                lure->pos[1] -= 10.0f;
            } else {
                lure->state++;
                return;
            }
        } else {
            lure->state++;
            return;
        }
        break;
    }
    Fish_StepState_Unk(lure, w);
    if (lure->step == LURE_STEP_REEL) {
        targetDist = Vec_Distance(lure->pos, w->target);
        if (targetDist > 100.0f) {
            Vec_Sub(vec, lure->pos, w->target);
            Vec_Scale(vec, vec, 100.0f / targetDist);
            Vec_Add(lure->pos, w->target, vec);
        }
    }
    lure->slot->pos[0] = lure->pos[0];
    lure->slot->pos[1] = lure->pos[1];
    lure->slot->pos[2] = lure->pos[2];
    if ((u8)Obj_InCurrentArea(pl) == 0) {
        lure->state++;
        return;
    }
    OT_AddByDepth(gDrawOt32, lure->slot, 32, 0);
}

void Lure_End(FxObj *lure)
{
    lure->state++;
    DrawSlot_Free(lure->slotId);
}

void Lure_Free(FxObj *lure)
{
    StageObj_Free((struct StageObj *)lure);
}

extern u32 gLureColors[];
extern u32 gLureColorPulled;

/* Draws a lure (the draw slot's update): its model (entry 24 of model set 0 for variant 1, else
 * the one Effect_MapTexId(113) picks), turned and placed at the slot; while the line is out
 * (line), the line model from the rod tip (Player_GetRodTip) to the lure. */
void Lure_Draw(DrawSlot *slot)
{
    FxObj *lure = (FxObj *)slot->owner;
    LureWork *w = lure->work;
    Model *set = gCommonModels[0];
    f32 rodTip[3];
    f32 tipToLure[3];
    f32 mtx[16];
    f32 uvMtx[16];
    f32 pitch;
    f32 roll;
    ModelMat *base;
    ModelMatSet *matSet;
    ModelMatSet *line;
    f32 *mtxPtr;
    f32 *tipToLureY;

    if (set != NULL && set->active) {
        base = set->mats;
        Mtx_Scale(mtx, w->scale, w->scale, w->scale);
        switch (lure->type) {
        case LURE_TYPE_ALT:
            matSet = &set->matSets[24];
            break;
        default:
            matSet = &set->matSets[Effect_MapTexId(113)];
            break;
        }
        Mtx_RotateY(mtx, 6.2831855f * lure->angle / 65536.0f);
        Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
        if (lure->ownerIndex & LURE_PULLED_UNDER) {
            Gs_SetState(RS_COLOR2, gLureColorPulled);
        } else {
            Gs_SetState(RS_COLOR2, gLureColors[lure->ownerIndex]);
        }
        Gs_SetState(RS_MTX_SLOT(0), (u32)(mtxPtr = mtx));
        Gs_SetLightList(base, matSet);
        Draw_ApplyStates(matSet->flags);
        Model_Draw(matSet->id, 0);
        if (w->line) {
            line = &set->matSets[Effect_MapTexId(127)];
            Player_GetRodTip(rodTip, lure->owner);
            Vec_Sub(tipToLure, slot->pos, rodTip);
            tipToLureY = &tipToLure[1];
            pitch = Math_Atan2(-tipToLure[2], Math_Sqrtf(tipToLure[0] * tipToLure[0] + *tipToLureY * *tipToLureY));
            roll = Math_Atan2(tipToLure[0], -*tipToLureY);
            Mtx_Scale(mtxPtr, 1.0f, Vec_Length(tipToLure), 1.0f);
            Mtx_RotateX(mtxPtr, pitch);
            Mtx_RotateZ(mtxPtr, roll);
            Mtx_SetTrans(mtxPtr, rodTip[0], rodTip[1], rodTip[2]);
            Gs_SetState(RS_COLOR2, -1);
            Gs_SetState(RS_MTX_SLOT(0), (u32)mtxPtr);
            Mtx_Translation(uvMtx, 0.0f, 0.03125f * w->frame, 0.0f);
            Gs_SetState(RS_UV_MTX, (u32)uvMtx);
            Gs_SetLightList(base, line);
            Draw_ApplyStates(line->flags);
            Model_Draw(line->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/* The water height under a lure (its waterY, set by area in Lure_Start). */
f32 Lure_GetWaterHeight(FxObj *lure)
{
    return ((LureWork *)lure->work)->waterY;
}

static void Lure_SetLaunchVel(f32 *vel, f32 startY, f32 targetY, f32 gravity, f32 frames)
{
    vel[1] = (f32) (((targetY - startY) / frames) - (0.5f * gravity * frames));
}


/* Casts the lure: its target in front of the player (20, 150, 100 turned by his angle), and a
 * velocity reaching it in 35 frames (mode 0) or 27 (mode 1) under the gravity. */
void Lure_Cast(FxObj *lure, LureWork *w)
{
    f32 offset[3];
    f32 dir[3];
    f32 flatTarget[3];
    Player *pl = lure->owner;
    f32 *offsetY = &offset[1];
    f32 *offsetZ = &offset[2];
    f32 len;

    offset[0] = 20.0f;
    *offsetY = 150.0f;
    *offsetZ = 100.0f;
    Vec_RotateY(offset, 6.2831855f * pl->rot[1] / 65536.0f);
    w->target[0] = pl->pos[0] + offset[0];
    w->target[1] = pl->pos[1] + *offsetY;
    w->target[2] = pl->pos[2] + *offsetZ;
    lure->step = LURE_STEP_PULL_OUT;
    switch (w->mode) {
    case LURE_MODE_CATCH:
        w->gravity = -2.0f;
        dir[0] = lure->pos[0];
        dir[1] = 0.0f;
        dir[2] = lure->pos[2];
        flatTarget[0] = w->target[0];
        flatTarget[1] = 0.0f;
        flatTarget[2] = w->target[2];
        Vec_Sub(dir, flatTarget, dir);
        len = Vec_Length(dir);
        Vec_Normalize(dir);
        len /= 35.0f;
        Vec_Scale(w->vel, dir, len);
        Lure_SetLaunchVel(w->vel, lure->pos[1], w->target[1], w->gravity, 35.0f);
        break;
    case LURE_MODE_EMPTY:
        w->gravity = -1.0f;
        dir[0] = lure->pos[0];
        dir[1] = 0.0f;
        dir[2] = lure->pos[2];
        flatTarget[0] = w->target[0];
        flatTarget[1] = 0.0f;
        flatTarget[2] = w->target[2];
        Vec_Sub(dir, flatTarget, dir);
        len = Vec_Length(dir);
        Vec_Normalize(dir);
        len /= 27.0f;
        Vec_Scale(w->vel, dir, len);
        Lure_SetLaunchVel(w->vel, lure->pos[1], w->target[1], w->gravity, 27.0f);
        break;
    default:
        w->gravity = 0.0f;
        w->vel[0] = 0.0f;
        w->vel[1] = 0.0f;
        w->vel[2] = 0.0f;
        break;
    }
    w->counter = 0;
}

/* The lure's line, every frame: while a hooked fish pulls (LURE_STEP_REEL) a 1 in 8 roll each frame
 * sets line (Lure_Draw draws the line only while it is set) for four frames, which frame
 * counts and scrolls the line's texture by; in every other step line is cleared. */
void Fish_StepState_Unk(FxObj *lure, LureWork *w)
{
    switch (lure->step) {
    case LURE_STEP_REEL:
        if (w->line == 0) {
            if ((Rand_Next(1) & 7) == 0) {
                w->line = 1;
                w->frame = 0;
            }
        } else if (++w->frame >= 4) {
            w->line = 0;
        }
        break;
    default:
        w->line = 0;
        break;
    }
}

extern f32 gRodTipOffsets[][3];

/* The tip of the player's rod: bone 18 plus (-9.2, -1.7, 12) in its frame, plus the offset
 * gRodTipOffsets[k] of the cast's bend k, which the casting motions (PL_MOT_FISH_REEL .. PL_MOT_FISH_PULLED_DOWN) set by frame. */
void Player_GetRodTip(f32 *out, Player *pl)
{
    f32 gripOffset[3];
    f32 bendOffset[3];
    f32 handMtx[16];
    f32 bendMtx[16];
    f32 *gripOffsetY;
    f32 *bendOffsetY;
    f32 *gripOffsetZ;
    f32 *bendOffsetZ;
    s16 bend;

    switch (pl->motionId[0]) {
    case PL_MOT_FISH_REEL:
        if (!Motion_ReachedFrame(pl, 0, 20.0f)) {
            bend = 0;
        } else if (!Motion_ReachedFrame(pl, 0, 30.0f)) {
            bend = 1;
        } else {
            bend = 2;
        }
        break;
    case PL_MOT_FISH_CATCH:
        if (!Motion_ReachedFrame(pl, 0, 20.0f)) {
            bend = 1;
        } else if (!Motion_ReachedFrame(pl, 0, 26.0f)) {
            bend = 2;
        } else if (!Motion_ReachedFrame(pl, 0, 28.0f)) {
            bend = 1;
        } else {
            bend = 0;
        }
        break;
    case PL_MOT_FISH_REEL_EMPTY:
        if (!Motion_ReachedFrame(pl, 0, 36.0f)) {
            bend = 1;
        } else if (!Motion_ReachedFrame(pl, 0, 40.0f)) {
            bend = 2;
        } else if (!Motion_ReachedFrame(pl, 0, 42.0f)) {
            bend = 1;
        } else {
            bend = 0;
        }
        break;
    case PL_MOT_FISH_PULLED_DOWN:
        if (!Motion_ReachedFrame(pl, 0, 20.0f)) {
            bend = 1;
        } else if (Motion_ReachedFrame(pl, 0, 22.0f)) {
            bend = 2;
        } else if (Motion_ReachedFrame(pl, 0, 24.0f)) {
            bend = 1;
        } else {
            bend = 0;
        }
        break;
    default:
        bend = 0;
        break;
    }
    Mtx_Copy(handMtx, (void *)Em_GetBone(pl, PL_BONE_OTHER_HAND));
    gripOffset[0] = -9.2f;
    gripOffsetY = &gripOffset[1];
    *gripOffsetY = -1.7f;
    gripOffsetZ = &gripOffset[2];
    *gripOffsetZ = 12.0f;
    Vec_RotateSelf(gripOffset, handMtx);
    Mtx_Identity(bendMtx);
    Mtx_RotateZ(bendMtx, -1.5707964f);
    Mtx_Concat3x3(bendMtx, handMtx);
    bendOffset[0] = gRodTipOffsets[bend][0];
    bendOffsetY = &bendOffset[1];
    *bendOffsetY = gRodTipOffsets[bend][1];
    bendOffsetZ = &bendOffset[2];
    *bendOffsetZ = gRodTipOffsets[bend][2];
    Vec_RotateSelf(bendOffset, bendMtx);
    out[0] = bendOffset[0] + (handMtx[12] + gripOffset[0]);
    out[1] = *bendOffsetY + (handMtx[13] + *gripOffsetY);
    out[2] = *bendOffsetZ + (handMtx[14] + *gripOffsetZ);
}

/* Plays a fishing sound at pos from the common bank: 46 for sound 0 (the lure's splashes),
 * 112 for sound 1 (the catch lands). The lure is not used. */
void Fishing_PlaySe(FxObj *lure, f32 *pos, s16 sound)
{
    switch (sound) {
    case 0:
        Sound_PlaySeAtPosChecked(SE_SLOT_COMMON, 46, 0, pos, 1, 0);
        break;
    case 1:
        Sound_PlaySeAtPosChecked(SE_SLOT_COMMON, 112, 0, pos, 1, 0);
        break;
    }
}

/* The countdown's update (kind 44): runs its state, CountdownFx_Start, CountdownFx_Run,
 * CountdownFx_End or CountdownFx_Free. */
void CountdownFx_Update(FxObj *countdown)
{
    u8 state;

    state = countdown->state;
    switch (state) {
    case 0:
        CountdownFx_Start(countdown);
        break;
    case 1:
        CountdownFx_Run(countdown);
        break;
    case 2:
        CountdownFx_End(countdown);
        break;
    case 3:
        CountdownFx_Free(countdown);
        break;
    }
}

/* Starts the effect: state 1 with a draw slot drawn by CountdownFx_Draw (then CountdownFx_Run), or StageObj_Free when no slot is free. */
void CountdownFx_Start(FxObj *countdown)
{
    countdown->step = 0;
    countdown->phase = 0;
    countdown->claimed = 0;
    countdown->timer = 0;
    countdown->state = 1;
    countdown->visible = 1;
    countdown->draw = NULL;
    countdown->slotId = DrawSlot_Alloc();
    if (countdown->slotId != -1) {
        countdown->slot = DrawSlot_Get(countdown->slotId);
        countdown->slot->owner = (void *)countdown;
        countdown->slot->draw = CountdownFx_Draw;
        CountdownFx_Run(countdown);
        return;
    }
    StageObj_Free((struct StageObj *) countdown);
}

/* The countdown's run state, every frame: its time (angle) counts down; the bars (timer) grow
 * over 5 frames, stay until the time is below -8, then shrink; at 0 it plays the end sound
 * (not in the town) and ends. Drawn every frame (gDrawOtSorted). Ends at once without a slot. */
void CountdownFx_Run(FxObj *countdown)
{
    if (countdown->slot == NULL) {
        countdown->state = 2;
        return;
    }
    countdown->angle--;
    switch (countdown->step) {
    case 0:
        if (++countdown->timer >= 5) {
            countdown->step++;
        }
        break;
    case 1:
        if (countdown->angle < -8) {
            countdown->step++;
        }
        break;
    case 2:
        if (--countdown->timer <= 0) {
            if (gScene.inLobby != 1) {
                Sound_PlaySeFlat(SE_SLOT_SCENE, 31, 0);
            }
            countdown->state = 2;
        }
        break;
    }
    OT_AddAt(gDrawOtSorted, countdown->slot, 0, 1);
}

/* The countdown's end state: frees its draw slot, if it has one, and goes to the free state. */
void CountdownFx_End(FxObj *countdown)
{
    countdown->state = 3;
    if (countdown->slot != NULL) {
        DrawSlot_Free(countdown->slotId);
    }
}

void CountdownFx_Free(FxObj *countdown)
{
    StageObj_Free((struct StageObj *)countdown);
}

/* Draws the countdown (the draw slot's update): while it is visible, the number of its last
 * three seconds (Countdown_Draw with its time) and the black bars (Letterbox_Draw with timer). */
void CountdownFx_Draw(DrawSlot *slot)
{
    FxObj *countdown;

    countdown = slot->owner;
    if (countdown->visible != 0) {
        Text_SetSize(0);
        Countdown_Draw(countdown->angle);
        Letterbox_Draw((s8)countdown->timer);
    }
}

/* Starts an effect object of kind 44 (update CountdownFx_Update) with n frames on its clock (angle). */
void Effect2C_Spawn(s16 frames)
{
    FxObj *countdown = (FxObj *)StageObj_Alloc(0);

    if (countdown != NULL) {
        countdown->kind = 44;
        countdown->type = 0;
        countdown->update = CountdownFx_Update;
        countdown->angle = frames;
    }
}

/* The texts gPlselTexts and gHelpTexts_6 (below) point to; no code loads them. Not const:
 * those tables are char *. */
#pragma rodata on
#pragma explicit_zero_data on
char D_004B0798[1] = {
#include "sub_main/004B0798.inc"
};
#pragma explicit_zero_data reset
char D_004B07A0[18] = {
#include "sub_main/004B07A0.inc"
};
char D_004B07B8[11] = {
#include "sub_main/004B07B8.inc"
};
char D_004B07D0[27] = {
#include "sub_main/004B07D0.inc"
};
char D_004B07F0[33] = {
#include "sub_main/004B07F0.inc"
};
char D_004B0820[19] = {
#include "sub_main/004B0820.inc"
};
char D_004B0840[25] = {
#include "sub_main/004B0840.inc"
};
char D_004B0860[6] = {
#include "sub_main/004B0860.inc"
};
char D_004B0868[8] = {
#include "sub_main/004B0868.inc"
};
char D_004B0870[33] = {
#include "sub_main/004B0870.inc"
};
char D_004B0898[14] = {
#include "sub_main/004B0898.inc"
};
char D_004B08A8[12] = {
#include "sub_main/004B08A8.inc"
};
char D_004B08C0[31] = {
#include "sub_main/004B08C0.inc"
};
char D_004B08E0[31] = {
#include "sub_main/004B08E0.inc"
};
char D_004B0900[31] = {
#include "sub_main/004B0900.inc"
};
char D_004B0920[31] = {
#include "sub_main/004B0920.inc"
};
char D_004B0940[31] = {
#include "sub_main/004B0940.inc"
};
char D_004B0960[37] = {
#include "sub_main/004B0960.inc"
};
char D_004B0988[12] = {
#include "sub_main/004B0988.inc"
};
char D_004B09A0[25] = {
#include "sub_main/004B09A0.inc"
};
char D_004B09C0[57] = {
#include "sub_main/004B09C0.inc"
};
char D_004B0A00[31] = {
#include "sub_main/004B0A00.inc"
};
char D_004B0A20[25] = {
#include "sub_main/004B0A20.inc"
};
char D_004B0A40[57] = {
#include "sub_main/004B0A40.inc"
};
char D_004B0A80[31] = {
#include "sub_main/004B0A80.inc"
};
char D_004B0AA0[31] = {
#include "sub_main/004B0AA0.inc"
};
char D_004B0AC0[31] = {
#include "sub_main/004B0AC0.inc"
};
char D_004B0AE0[31] = {
#include "sub_main/004B0AE0.inc"
};
char D_004B0B00[31] = {
#include "sub_main/004B0B00.inc"
};
char D_004B0B20[31] = {
#include "sub_main/004B0B20.inc"
};
char D_004B0B40[29] = {
#include "sub_main/004B0B40.inc"
};
char D_004B0B60[31] = {
#include "sub_main/004B0B60.inc"
};
char D_004B0B80[10] = {
#include "sub_main/004B0B80.inc"
};
char D_004B0B90[11] = {
#include "sub_main/004B0B90.inc"
};
char D_004B0BA0[15] = {
#include "sub_main/004B0BA0.inc"
};
char D_004B0BB0[17] = {
#include "sub_main/004B0BB0.inc"
};
char D_004B0BD0[18] = {
#include "sub_main/004B0BD0.inc"
};
char D_004B0BF0[18] = {
#include "sub_main/004B0BF0.inc"
};
char D_004B0C10[19] = {
#include "sub_main/004B0C10.inc"
};
char D_004B0C30[19] = {
#include "sub_main/004B0C30.inc"
};
char D_004B0C50[18] = {
#include "sub_main/004B0C50.inc"
};
char D_004B0C70[19] = {
#include "sub_main/004B0C70.inc"
};
char D_004B0C88[9] = {
#include "sub_main/004B0C88.inc"
};
char D_004B0CA0[21] = {
#include "sub_main/004B0CA0.inc"
};
char D_004B0CB8[11] = {
#include "sub_main/004B0CB8.inc"
};
char D_004B0CC8[11] = {
#include "sub_main/004B0CC8.inc"
};
char D_004B0CD8[11] = {
#include "sub_main/004B0CD8.inc"
};
char D_004B0CE8[11] = {
#include "sub_main/004B0CE8.inc"
};
char D_004B0D00[18] = {
#include "sub_main/004B0D00.inc"
};
char D_004B0D20[29] = {
#include "sub_main/004B0D20.inc"
};
char D_004B0D40[27] = {
#include "sub_main/004B0D40.inc"
};
char D_004B0D60[29] = {
#include "sub_main/004B0D60.inc"
};
char D_004B0D80[29] = {
#include "sub_main/004B0D80.inc"
};
char D_004B0DA0[29] = {
#include "sub_main/004B0DA0.inc"
};
char D_004B0DC0[29] = {
#include "sub_main/004B0DC0.inc"
};
char D_004B0DE0[29] = {
#include "sub_main/004B0DE0.inc"
};
char D_004B0E00[29] = {
#include "sub_main/004B0E00.inc"
};
char D_004B0E20[29] = {
#include "sub_main/004B0E20.inc"
};
char D_004B0E40[29] = {
#include "sub_main/004B0E40.inc"
};
char D_004B0E60[31] = {
#include "sub_main/004B0E60.inc"
};
char D_004B0E80[29] = {
#include "sub_main/004B0E80.inc"
};
char D_004B0EA0[18] = {
#include "sub_main/004B0EA0.inc"
};
char D_004B0EC0[18] = {
#include "sub_main/004B0EC0.inc"
};
char D_004B0EE0[19] = {
#include "sub_main/004B0EE0.inc"
};
char D_004B0F00[19] = {
#include "sub_main/004B0F00.inc"
};
char gResultRuleText_0[53] = {
#include "sub_main/004B0F20.inc"
};
char D_004B0F58[5] = {
#include "sub_main/004B0F58.inc"
};
char gWtdClassAText_0[24] = {
#include "sub_main/004B0F60.inc"
};
char gWtdClassBText_0[24] = {
#include "sub_main/004B0F80.inc"
};
char gWtdClassCText_0[24] = {
#include "sub_main/004B0FA0.inc"
};
char gHighClassBonusText_0[27] = {
#include "sub_main/004B0FC0.inc"
};
char D_004B0FE0[6] = {
#include "sub_main/004B0FE0.inc"
};
char D_004B0FE8[5] = {
#include "sub_main/004B0FE8.inc"
};
char D_004B0FF0[14] = {
#include "sub_main/004B0FF0.inc"
};
char gHrpNotMetText_0[47] = {
#include "sub_main/004B1000.inc"
};
char gHrpNoLossText_0[49] = {
#include "sub_main/004B1030.inc"
};
char gHrMaxText_0[35] = {
#include "sub_main/004B1070.inc"
};
char gHrUpText_0[39] = {
#include "sub_main/004B10A0.inc"
};
char gDeathPenaltyText_0[27] = {
#include "sub_main/004B10D0.inc"
};
char D_004B10F0[21] = {
#include "sub_main/004B10F0.inc"
};
char D_004B1110[26] = {
#include "sub_main/004B1110.inc"
};
char D_004B1130[35] = {
#include "sub_main/004B1130.inc"
};
char D_004B1160[35] = {
#include "sub_main/004B1160.inc"
};
char D_004B1190[35] = {
#include "sub_main/004B1190.inc"
};
char D_004B11C0[39] = {
#include "sub_main/004B11C0.inc"
};
char D_004B11F0[35] = {
#include "sub_main/004B11F0.inc"
};
char D_004B1220[35] = {
#include "sub_main/004B1220.inc"
};
char D_004B1250[35] = {
#include "sub_main/004B1250.inc"
};
char D_004B1280[24] = {
#include "sub_main/004B1280.inc"
};
char D_004B12A0[26] = {
#include "sub_main/004B12A0.inc"
};
char D_004B12C0[35] = {
#include "sub_main/004B12C0.inc"
};
char D_004B12F0[35] = {
#include "sub_main/004B12F0.inc"
};
char D_004B1320[35] = {
#include "sub_main/004B1320.inc"
};
char D_004B1350[33] = {
#include "sub_main/004B1350.inc"
};
char D_004B1380[19] = {
#include "sub_main/004B1380.inc"
};
char D_004B13A0[17] = {
#include "sub_main/004B13A0.inc"
};
char D_004B13C0[33] = {
#include "sub_main/004B13C0.inc"
};
char D_004B13F0[31] = {
#include "sub_main/004B13F0.inc"
};
char D_004B1410[35] = {
#include "sub_main/004B1410.inc"
};
char D_004B1440[35] = {
#include "sub_main/004B1440.inc"
};
char D_004B1470[31] = {
#include "sub_main/004B1470.inc"
};
char D_004B1490[31] = {
#include "sub_main/004B1490.inc"
};
char D_004B14B0[23] = {
#include "sub_main/004B14B0.inc"
};
char D_004B14D0[19] = {
#include "sub_main/004B14D0.inc"
};
char D_004B14F0[31] = {
#include "sub_main/004B14F0.inc"
};
char D_004B1510[31] = {
#include "sub_main/004B1510.inc"
};
char D_004B1530[31] = {
#include "sub_main/004B1530.inc"
};
char D_004B1550[31] = {
#include "sub_main/004B1550.inc"
};
char D_004B1570[31] = {
#include "sub_main/004B1570.inc"
};
char D_004B1590[31] = {
#include "sub_main/004B1590.inc"
};
char D_004B15B0[31] = {
#include "sub_main/004B15B0.inc"
};
char D_004B15D0[31] = {
#include "sub_main/004B15D0.inc"
};
char D_004B15F0[47] = {
#include "sub_main/004B15F0.inc"
};
char D_004B1620[48] = {
#include "sub_main/004B1620.inc"
};
char D_004B1650[21] = {
#include "sub_main/004B1650.inc"
};
char D_004B1670[21] = {
#include "sub_main/004B1670.inc"
};
char D_004B1690[23] = {
#include "sub_main/004B1690.inc"
};
char D_004B16A8[11] = {
#include "sub_main/004B16A8.inc"
};
char D_004B16B8[11] = {
#include "sub_main/004B16B8.inc"
};
char D_004B16C8[5] = {
#include "sub_main/004B16C8.inc"
};
char D_004B16D0[5] = {
#include "sub_main/004B16D0.inc"
};
char D_004B16D8[5] = {
#include "sub_main/004B16D8.inc"
};
char gHelpTexts_6_0[69] = {
#include "sub_main/004B16E0.inc"
};
char gHelpTexts_6_1[71] = {
#include "sub_main/004B1730.inc"
};
char gHelpTexts_6_2[27] = {
#include "sub_main/004B1780.inc"
};
char gHelpTexts_6_3[36] = {
#include "sub_main/004B17A0.inc"
};
char gHelpTexts_6_4[38] = {
#include "sub_main/004B17D0.inc"
};
char gHelpTexts_6_5[40] = {
#include "sub_main/004B1800.inc"
};
char D_004B1830[19] = {
#include "sub_main/004B1830.inc"
};
char D_004B1848[13] = {
#include "sub_main/004B1848.inc"
};
char D_004B1860[17] = {
#include "sub_main/004B1860.inc"
};
char D_004B1878[9] = {
#include "sub_main/004B1878.inc"
};
char D_004B1888[4] = {
#include "sub_main/004B1888.inc"
};
char D_004B1890[5] = {
#include "sub_main/004B1890.inc"
};
#pragma rodata off


/* The countdown's last seconds: the number (3, 2, 1) with its sound at the start of each second,
 * drawn larger and fading while the second passes. */
void Countdown_Draw(s16 framesLeft)
{
    u8 sec;
    u8 sinceShown;
    u8 stage;
    s32 alpha;
    u32 color;
    f32 scale;

    if (framesLeft <= 0) {
        return;
    }
    sec = (framesLeft - 1) / 30 + 1;
    switch (sec) {
    case 3:
        sinceShown = Math_Absf(framesLeft - 90);
        if (sinceShown == 6) {
            if (gScene.inLobby != 1) {
                Sound_PlaySeFlat(SE_SLOT_SCENE, 28, 0);
            } else {
                Sound_PlaySeFlat(6, 85, 0);
            }
        }
        break;
    case 2:
        sinceShown = Math_Absf(framesLeft - 60);
        if (sinceShown == 6) {
            if (gScene.inLobby != 1) {
                Sound_PlaySeFlat(SE_SLOT_SCENE, 29, 0);
            } else {
                Sound_PlaySeFlat(6, 86, 0);
            }
        }
        break;
    case 1:
        sinceShown = Math_Absf(framesLeft - 30);
        if (sinceShown == 6) {
            if (gScene.inLobby != 1) {
                Sound_PlaySeFlat(SE_SLOT_SCENE, 30, 0);
            } else {
                Sound_PlaySeFlat(6, 87, 0);
            }
        }
        break;
    default:
        sinceShown = 100;
        break;
    }
    if (sinceShown < 2) {
        stage = 0;
    } else if (sinceShown < 4 && sinceShown >= 2) {
        stage = 1;
    } else if (sinceShown < 6 && sinceShown >= 4) {
        stage = 2;
    } else if (sinceShown > 27) {
        stage = 5;
    } else if (sinceShown < 28 && sinceShown >= 25) {
        stage = 4;
    } else {
        stage = 3;
    }
    switch (stage) {
    case 0:
        scale = 8.0f;
        alpha = 75;
        break;
    case 1:
        scale = 3.0f;
        alpha = 128;
        break;
    case 2:
        scale = 1.0f;
        alpha = 153;
        break;
    case 3:
        scale = 1.1f;
        alpha = 175;
        break;
    case 4:
        scale = 0.8f;
        alpha = 50;
        break;
    case 5:
    default:
        alpha = 0;
        scale = 0.0f;
        break;
    }
    color = (alpha << 24) | 0xA01B00;
    if (scale > 0.0f) {
        Countdown_DrawDigit(sec, color, scale);
    }
    if (stage < 5) {
        Font_SetSpacing(32, 48);
        Text_PrintfAt(312, 390, 0, "%i", sec);
    }
}

/* A countdown digit (1, 2 or 3) at the screen's center, as one to three triangles of the given
 * color and size (124 x scale high). */
void Countdown_DrawDigit(u8 digit, u32 color, f32 scale)
{
    PrimTri tri0, tri1, tri2;
    f32 height = 124.0f * scale;
    f32 width = 0.865f * height;

    switch (digit) {
    case 1:
        tri0.x0 = 320.0f - 0.25f * height;
        tri0.y0 = 224.0f + 0.5f * width;
        tri0.x1 = 320.0f + 0.25f * height;
        tri0.y1 = tri0.y0;
        tri0.x2 = 320;
        tri0.y2 = 224.0f - 0.5f * width;
        tri0.color = color;
        Prim_FillTri(&tri0);
        break;
    case 2:
        tri0.x0 = 320.0f - 0.5f * height;
        tri0.y0 = 224.0f + 0.5f * width;
        tri0.x1 = 320;
        tri0.y1 = tri0.y0;
        tri0.x2 = 320.0f - 0.25f * height;
        tri0.y2 = 224.0f - 0.5f * width;
        tri1.x0 = tri0.x1;
        tri1.y0 = tri0.y1;
        tri1.x1 = 320.0f + 0.5f * height;
        tri1.y1 = tri1.y0;
        tri1.x2 = 320.0f + 0.25f * height;
        tri1.y2 = 224.0f - 0.5f * width;
        tri0.color = tri1.color = color;
        Prim_FillTri(&tri0);
        Prim_FillTri(&tri1);
        break;
    case 3:
        tri0.x0 = 320.0f - 0.5f * height;
        tri0.y0 = 224.0f + 0.5f * width;
        tri0.x1 = 320;
        tri0.y1 = tri0.y0;
        tri0.x2 = 320.0f - 0.25f * height;
        tri0.y2 = 224;
        tri1.x0 = tri0.x1;
        tri1.y0 = tri0.y1;
        tri1.x1 = 320.0f + 0.5f * height;
        tri1.y1 = tri1.y0;
        tri1.x2 = 320.0f + 0.25f * height;
        tri1.y2 = tri0.y2;
        tri2.x0 = tri0.x2;
        tri2.y0 = tri0.y2;
        tri2.x1 = tri1.x2;
        tri2.y1 = tri1.y2;
        tri2.x2 = 320;
        tri2.y2 = 224.0f - 0.5f * width;
        tri0.color = tri1.color = tri2.color = color;
        Prim_FillTri(&tri0);
        Prim_FillTri(&tri1);
        Prim_FillTri(&tri2);
        break;
    }
}

/* Black bars at the top and bottom of the screen (16 high per unit of size, at most 72), two
 * triangles each. The last function of sub_main's code: the 32 zero words after it, up to the
 * VU1 programs, are built as a piece of their own. */
void Letterbox_Draw(s8 size)
{
    PrimTri tri0, tri1, tri2, tri3;
    f32 barHeight = 16.0f * size;

    if (!(barHeight < 72.0f)) {
        barHeight = 72.0f;
    }
    tri0.x0 = 0;
    tri0.y0 = 0;
    tri0.x1 = 640;
    tri0.y1 = barHeight;
    tri0.x2 = 640;
    tri0.y2 = 0;
    tri0.color = 0xFF000000;
    tri1.x0 = 0;
    tri1.y0 = 0;
    tri1.x1 = 640;
    tri1.y1 = barHeight;
    tri1.x2 = 0;
    tri1.y2 = barHeight;
    tri1.color = 0xFF000000;
    tri2.x0 = 0;
    tri2.y0 = 448.0f - barHeight;
    tri2.x1 = 640;
    tri2.y1 = 448;
    tri2.x2 = 640;
    tri2.y2 = 448.0f - barHeight;
    tri2.color = 0xFF000000;
    tri3.x0 = 0;
    tri3.y0 = 448.0f - barHeight;
    tri3.x1 = 640;
    tri3.y1 = 448;
    tri3.x2 = 0;
    tri3.y2 = 448;
    tri3.color = 0xFF000000;
    Prim_FillTri(&tri0);
    Prim_FillTri(&tri1);
    Prim_FillTri(&tri2);
    Prim_FillTri(&tri3);
}

/* Zeros to the end of sub_main's .rodata, after Countdown_Draw's jump table and this array's
 * alignment. */
const u32 D_004B18C0[16] = { 0 };

/* The file's .data: the fish kinds, the town's stage data, the fishing tables and two text
 * tables. The externs are this file's .rodata texts. */
FishKind gFishKindsQuest[24] = {
#include "sub_main/004698E0.inc"
};
FishKind gFishKindsLobby[24] = {
#include "sub_main/00469AC0.inc"
};
FishSpot gLobbyStageData_1[2] = {
#include "sub_main/00469CA0.inc"
};
FishSpot gLobbyStageData_3[2] = {
#include "sub_main/00469CD0.inc"
};
LobbyStageData gLobbyStageData = {
#include "sub_main/00469D00.inc"
};
LobbyFishTable gLobbySeasonalData_0[6] = {
#include "sub_main/00469D30.inc"
};
LobbyFishTable gLobbySeasonalData_2[6] = {
#include "sub_main/00469D60.inc"
};
LobbyFishTable *gLobbySeasonalData[4] = {
    gLobbySeasonalData_0, gLobbySeasonalData_0, gLobbySeasonalData_2, gLobbySeasonalData_2,
};
u16 gFishBiteJitter[4] = {
#include "sub_main/00469DA0.inc"
};
s8 gFishWaitSecs[8] = {
#include "sub_main/00469DA8.inc"
};
u32 gLureColors[4] = {
#include "sub_main/00469DB0.inc"
};
u32 gLureColorPulled = {
#include "sub_main/00469DC0.inc"
};
f32 gRodTipOffsets[4][3] = {
#include "sub_main/00469DD0.inc"
};
/* 168 text pointers (into this file's .rodata) that no code names. */
char *gPlselTexts[168] = {
    D_004B0798, D_004B0798, D_004B0798, D_004B0798,
    D_004B0798, D_004B0798, D_004B0798, D_004B07A0,
    D_004B0798, D_004B0798, D_004B0798, D_004B0798,
    D_004B0798, D_004B0798, D_004B0798, D_004B0798,
    D_004B0798, D_004B0798, D_004B0798, D_004B07A0,
    D_004B0798, D_004B0798, D_004B0798, D_004B0798,
    D_004B0798, D_004B0798, D_004B07B8, D_004B07D0,
    D_004B07F0, D_004B0820, D_004B0840, D_004B07A0,
    D_004B0860, D_004B0868, D_004B0870, D_004B0798,
    D_004B0798, D_004B0898, D_004B0798, D_004B0798,
    D_004B0798, D_004B0798, D_004B0798, NULL,
    D_004B08A8, D_004B08C0, D_004B08E0, D_004B0900,
    D_004B0920, D_004B0940, D_004B08A8, D_004B0960,
    D_004B0988, D_004B09A0, D_004B09C0, D_004B0A00,
    D_004B08A8, D_004B0A20, D_004B0A40, D_004B0A80,
    D_004B0AA0, D_004B0AC0, D_004B0AE0, D_004B0B00,
    D_004B0B20, D_004B08A8, D_004B0B40, D_004B0988,
    D_004B0B60, D_004B0B80, D_004B0B90, D_004B0BA0,
    D_004B0BB0, D_004B0BD0, D_004B0BF0, D_004B0C10,
    D_004B0C30, D_004B0C50, D_004B0C70, D_004B0C88,
    D_004B0CA0, D_004B0CB8, D_004B0CC8, D_004B0CD8,
    D_004B0CE8, D_004B0D00, D_004B0D20, D_004B0D40,
    D_004B0D60, D_004B0D80, D_004B0DA0, D_004B0DC0,
    D_004B0DE0, D_004B0E00, D_004B0E20, D_004B0E40,
    D_004B0E60, D_004B0E80, D_004B0EA0, D_004B0EC0,
    D_004B0EE0, D_004B0F00, NULL, NULL,
    gResultRuleText_0, D_004B0F58, gWtdClassAText_0, gWtdClassBText_0,
    gWtdClassCText_0, gHighClassBonusText_0, D_004B0FE0, (char *)D_004B0FE8,
    D_004B0FF0, gHrpNotMetText_0, gHrpNoLossText_0, gHrMaxText_0,
    gHrUpText_0, gDeathPenaltyText_0, NULL, NULL,
    D_004B10F0, D_004B1110, D_004B1130, D_004B1160,
    D_004B1190, D_004B11C0, D_004B11F0, D_004B1220,
    D_004B1250, D_004B1250, D_004B1220, D_004B1280,
    D_004B12A0, D_004B12C0, D_004B12F0, D_004B1320,
    D_004B1350, D_004B1380, D_004B13A0, D_004B13C0,
    D_004B13F0, D_004B1410, D_004B1440, D_004B1220,
    D_004B1470, D_004B1490, D_004B14B0, D_004B14D0,
    D_004B14F0, D_004B1510, D_004B1530, D_004B1550,
    D_004B1570, D_004B1590, D_004B15B0, D_004B15D0,
    D_004B15F0, D_004B1620, D_004B1650, D_004B1670,
    D_004B1690, D_004B16A8, D_004B16B8, D_004B16C8,
    D_004B16D0, D_004B16D8, NULL, NULL,
};
char *gHelpTexts_6[24] = {
    gHelpTexts_6_0, gHelpTexts_6_1, gHelpTexts_6_2, gHelpTexts_6_3,
    gHelpTexts_6_4, gHelpTexts_6_5, NULL, NULL,
    D_004B1830, D_004B1848, D_004B1830, D_004B0FE8,
    NULL, NULL, NULL, NULL,
    D_004B1860, D_004B1878, D_004B1888, D_004B1890,
    NULL, NULL, NULL, NULL,
};
