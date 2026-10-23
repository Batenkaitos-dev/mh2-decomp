/*
 * Drawing the hunters and what they hold.
 *
 * - The hunters: Player_Draw and Player_DrawLocal (the local one, faded near
 *   the camera) draw the skeleton's own model (the legs), the face and the
 *   four other armour pieces placed by their bones and lit per material
 *   (Player_SetLight, Player_PartFlags, Color_Cycle8), the weapon
 *   (Player_DrawWeapon) and the held props (Player_DrawHeldProp,
 *   Player_DrawHeldItem); CharMake_DrawHunter draws the hunter being made.
 * - The weapon: Player_MotionPhase says where it is in the current motion
 *   (in a hand or on the back, WP_PHASE_*), the WpPose tables place it on its
 *   bone, and keyframe tracks (Key1, Key2, Key3) move its moving bones in the
 *   weapon's own motions: Key1_Eval to Key3_Eval read one to three floats
 *   per time (a time of -1 ends a track), and Key3_EvalByTimer runs on the
 *   long sword's gauge timer.
 * - Bone_SetMatrices and Bone_SetMatricesRel (under a parent matrix) fill
 *   the matrix slots of a bone tree, for the armour pieces placed by their
 *   bones.
 * The data tables these read (the poses and keyframes, the bone markers'
 * directions, the cycling colours) are defined at the end of the file.
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "chredit.h"
#include "camera.h"
#include "objhead.h"
#include "sprite.h"
#include "area.h"
#include "pad.h"
#include "render_state.h"
#include "light.h"
#include "equip_preview.h"
#include "bgm.h"
#include "place_rules.h"
#include "save.h"
#include "text.h"
#include "window.h"
#include "model.h"
#include "cam_helpers.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "game_util.h"
#include "libc.h"
#include "engine_util.h"
#include "vec_math.h"

/* A keyframe track's clock: the callers pass a Player, whose motion layer
 * 0's frame (Player.motion[0].frame, at +0x15C) is the time. The callers cast
 * their Player to it. */
typedef struct KeyClock {
    u8 pad[348];
    f32 time;
} KeyClock;

/* The owner of the bones that a tree node can copy (by index & 0x3F): the
 * callers pass a Player, whose part records (Player.partRecs, at +0xD0) are
 * read here as bones (only the matrix at +0x40, EmBone.local). The callers
 * cast their Player to it. */
typedef struct BoneRoot {
    u8 pad[208];
    EmBone *bones[64];            /* 0xD0 */
} BoneRoot;

extern s16 *gBoneMtxSlotMaps[];       /* per table: a bone index per node, 0 none, bit 6 relative */
/* Sets the matrix slot of each bone of a tree (RS_MTX_SLOT(index)): skin
 * times a matrix that gBoneMtxSlotMaps[table] picks per bone: a bone of
 * root (below 64), the bone's world matrix with that bone's rotation
 * (64 and up), or only the bone's position (0). unused is passed down
 * unread. */
void Bone_SetMatrices(EmBone *bone, BoneRoot *root, s16 table, s32 unused)
{
    f32 out[16];
    f32 mtx[16];
    s16 slot = gBoneMtxSlotMaps[table][bone->index];

    if (slot > 0) {
        if (slot < 64) {
            Mtx_Copy(mtx, root->bones[slot & 0x3F]->local);
        } else {
            EmBone **srcBone;

            Mtx_Copy(mtx, bone->world);
            srcBone = (EmBone **)(((slot & 0x3F) << 2) + (u32)root + 208);
            Mtx_Copy3x3(mtx, (*srcBone)->local);
            Mtx_Mul3x3(mtx, bone->local, (*srcBone)->local);
        }
    } else {
        Mtx_Identity(mtx);
        mtx[12] = bone->world[12];
        mtx[13] = bone->world[13];
        mtx[14] = bone->world[14];
    }
    Mtx_Mul(out, bone->skin, mtx);
    Gs_SetState(RS_MTX_SLOT(bone->index), (u32)out);
    if (bone->child != NULL) {
        Bone_SetMatrices(bone->child, root, table, unused);
    }
    if (bone->sibling != NULL) {
        Bone_SetMatrices(bone->sibling, root, table, unused);
    }
}

/* As Bone_SetMatrices, but a bone mapped 64 and up takes its local
 * matrix times parent, the matrix its parent bone used. */
void Bone_SetMatricesRel(EmBone *bone, BoneRoot *root, s16 table, s32 unused, f32 *parent)
{
    f32 out[16];
    f32 mtx[16];
    s16 slot = gBoneMtxSlotMaps[table][bone->index];

    if (slot > 0) {
        if (slot < 64) {
            Mtx_Copy(mtx, root->bones[slot & 0x3F]->local);
        } else {
            Mtx_Mul(mtx, bone->local, parent);
        }
    } else {
        Mtx_Identity(mtx);
        mtx[12] = bone->world[12];
        mtx[13] = bone->world[13];
        mtx[14] = bone->world[14];
    }
    Mtx_Mul(out, bone->skin, mtx);
    Gs_SetState(RS_MTX_SLOT(bone->index), (u32)out);
    if (bone->child != NULL) {
        Bone_SetMatricesRel(bone->child, root, table, unused, mtx);
    }
    if (bone->sibling != NULL) {
        Bone_SetMatricesRel(bone->sibling, root, table, unused, parent);
    }
}

/* The value of a one-float track at the clock's time: interpolated
 * between the two keys around it, or the last key's from its time on. */
f32 Key1_Eval(KeyClock *clock, Key1 *key)
{
    for (;; key++) {
        if (key->t == -1.0f) {
            return key->v;
        }
        if (Motion_ReachedFrame(clock, 0, key->t) != 0 && Motion_ReachedFrame(clock, 0, key[1].t) == 0) {
            return key->v + (key[1].v - key->v) * ((clock->time - key->t) / (key[1].t - key->t));
        }
    }
}

/* As Key1_Eval for a three-float track, into out. */
void Key3_Eval(KeyClock *clock, Key3 *key, f32 *out)
{
    for (;; key++) {
        if (key->t == -1.0f) {
            out[0] = key->v[0];
            out[1] = key->v[1];
            out[2] = key->v[2];
            return;
        }
        if (Motion_ReachedFrame(clock, 0, key->t) != 0 && Motion_ReachedFrame(clock, 0, key[1].t) == 0) {
            out[0] = key->v[0] + (key[1].v[0] - key->v[0]) * ((clock->time - key->t) / (key[1].t - key->t));
            out[1] = key->v[1] + (key[1].v[1] - key->v[1]) * ((clock->time - key->t) / (key[1].t - key->t));
            out[2] = key->v[2] + (key[1].v[2] - key->v[2]) * ((clock->time - key->t) / (key[1].t - key->t));
            return;
        }
    }
}

/* As Key1_Eval for a two-float track, into firstOut and secondOut. */
void Key2_Eval(KeyClock *clock, Key2 *key, f32 *firstOut, f32 *secondOut)
{
    for (;; key++) {
        if (key->t == -1.0f) {
            *firstOut = key->v[0];
            *secondOut = key->v[1];
            return;
        }
        if (Motion_ReachedFrame(clock, 0, key->t) != 0 && Motion_ReachedFrame(clock, 0, key[1].t) == 0) {
            *firstOut = key->v[0] + (key[1].v[0] - key->v[0]) * ((clock->time - key->t) / (key[1].t - key->t));
            *secondOut = key->v[1] + (key[1].v[1] - key->v[1]) * ((clock->time - key->t) / (key[1].t - key->t));
            return;
        }
    }
}

/* As Key3_Eval, with the time 10 less the long sword's gauge timer. */
void Key3_EvalByTimer(Player *pl, Key3 *key, f32 *out)
{
    f32 elapsed = 10.0f - pl->swordGaugeTimer;
    f32 ratio;
    f32 delta;

    for (;; key++) {
        if (key->t == -1.0f) {
            out[0] = key->v[0];
            out[1] = key->v[1];
            out[2] = key->v[2];
            return;
        }
        if (!(elapsed < key->t) && elapsed < key[1].t) {
            /* Each channel repeats an empty check of its key interval after
             * the ratio. The compiler drops the checks but still orders the
             * statements around them as the game's code does; without them
             * out[1] and out[2] get other registers. */
            ratio = (elapsed - key->t) / (key[1].t - key->t);
            if (key[1].t == key->t || elapsed < key->t) { /* compiled-out check */
            }
            delta = key[1].v[0] - key->v[0];
            out[0] = key->v[0] + delta * ratio;
            ratio = (elapsed - key->t) / (key[1].t - key->t);
            if (key[1].t == key->t || elapsed < key->t) { /* compiled-out check */
            }
            delta = key[1].v[1] - key->v[1];
            out[1] = key->v[1] + delta * ratio;
            ratio = (elapsed - key->t) / (key[1].t - key->t);
            if (key[1].t == key->t || elapsed < key->t) { /* compiled-out check */
            }
            delta = key[1].v[2] - key->v[2];
            out[2] = key->v[2] + delta * ratio;
            return;
        }
    }
}

/* Where the weapon is drawn (Player_MotionPhase, Player_DrawWeapon). */
#define WP_PHASE_OTHER_HAND 0   /* in the other hand (PL_BONE_OTHER_HAND, gWpPoseDrawnAlt) */
#define WP_PHASE_HAND 1         /* in the hand (PL_BONE_HAND, gWpPoseDrawn) */
#define WP_PHASE_SHEATHED 2     /* on the back (gWpPoseSheathed) */

/* The hunter's hand bones, which are also the indices of their part records (Player.partRecs,
 * Player_ScaleBones). */

struct EmBone;

/* Where the player's weapon is drawn in its current motion (motionId[0]): WP_PHASE_HAND,
 * WP_PHASE_OTHER_HAND or WP_PHASE_SHEATHED, by weapon type and the frame the motion has
 * reached (the weapon's own motions, 1000 and up, draw and sheathe it), then by the shared
 * motions (the gestures, PL_MOT_SHARPEN ...), which override it. secondPart picks the
 * dual swords' second blade's frames; otherPlayer (set when Player_Draw draws another player's
 * hunter) keeps motion 841's weapon on the back. The arms
 * that set WP_PHASE_SHEATHED where it already is are the game's own: the compiler drops the
 * store but keeps the branch. */
s16 Player_MotionPhase(Player *pl, s32 secondPart, s32 otherPlayer)
{
    s16 phase = WP_PHASE_SHEATHED;

    switch (pl->weaponType) {
    case WEAPON_HAMMER:
        switch (pl->motionId[0]) {
        case 1002:
            if (Motion_ReachedFrame(pl, 0, 6.0f)) phase = WP_PHASE_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 1003:
            if (Motion_ReachedFrame(pl, 0, 48.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 1009:
            if (Motion_ReachedFrame(pl, 0, 50.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 1010:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    case WEAPON_HUNTING_HORN:
        switch (pl->motionId[0]) {
        case 1002:
            if (Motion_ReachedFrame(pl, 0, 68.0f)) phase = WP_PHASE_HAND;
            else if (Motion_ReachedFrame(pl, 0, 14.0f)) phase = WP_PHASE_OTHER_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 1003:
            if (Motion_ReachedFrame(pl, 0, 50.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 1005:
            if (Motion_ReachedFrame(pl, 0, 50.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 1006:
            if (Motion_ReachedFrame(pl, 0, 16.0f)) phase = WP_PHASE_HAND;
            else if (Motion_ReachedFrame(pl, 0, 6.0f)) phase = WP_PHASE_OTHER_HAND; else phase = WP_PHASE_SHEATHED;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 1404:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_HAND;
            else if (Motion_ReachedFrame(pl, 0, 54.0f)) phase = WP_PHASE_HAND;
            else phase = WP_PHASE_OTHER_HAND;
            break;
        case 1416:
            phase = WP_PHASE_OTHER_HAND;
            break;
        case 1431:
            if (Motion_ReachedFrame(pl, 0, 10.0f)) phase = WP_PHASE_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    case WEAPON_LANCE:
    case WEAPON_GUNLANCE:
        switch (pl->motionId[0]) {
        case 1002:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            else if (pl->weaponType == WEAPON_LANCE) phase = WP_PHASE_HAND;
            else if (Motion_ReachedFrame(pl, 0, 14.0f)) phase = WP_PHASE_HAND;
            break;
        case 1018:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 1003:
            if (Motion_ReachedFrame(pl, 0, 60.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 1009:
            if (Motion_ReachedFrame(pl, 0, 66.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 1010:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    case WEAPON_SWORD_SHIELD:
        switch (pl->motionId[0]) {
        case 1002:
        case 1018:
        case 1010:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 1003:
            if (Motion_ReachedFrame(pl, 0, 32.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 1009:
            if (Motion_ReachedFrame(pl, 0, 30.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 1016:
            if (Motion_ReachedFrame(pl, 0, 14.0f)) phase = WP_PHASE_OTHER_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 1017:
            if (Motion_ReachedFrame(pl, 0, 44.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_OTHER_HAND;
            break;
        case 1410:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_OTHER_HAND;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    case WEAPON_DUAL_SWORDS:
        switch (pl->motionId[0]) {
        case 1002:
            if (Motion_ReachedFrame(pl, 0, secondPart == 0 ? 8.0f : 10.0f)) phase = WP_PHASE_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 1003:
            if (Motion_ReachedFrame(pl, 0, secondPart == 0 ? 20.0f : 28.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 1009:
            if (Motion_ReachedFrame(pl, 0, secondPart == 0 ? 18.0f : 30.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 1017:
            if (Motion_ReachedFrame(pl, 0, 44.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_OTHER_HAND;
            break;
        case 1413:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            else if (Motion_ReachedFrame(pl, 0, 4.0f)) phase = WP_PHASE_HAND; else phase = WP_PHASE_SHEATHED;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    default:
        switch (pl->motionId[0]) {
        case 1002:
            if (Motion_ReachedFrame(pl, 0, 16.0f)) phase = WP_PHASE_OTHER_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 1003:
            if (Motion_ReachedFrame(pl, 0, 38.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_OTHER_HAND;
            break;
        case 1016:
            if (Motion_ReachedFrame(pl, 0, 14.0f)) phase = WP_PHASE_OTHER_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 1017:
            if (Motion_ReachedFrame(pl, 0, 44.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_OTHER_HAND;
            break;
        case 1412:
        case 1413:
        case 1414:
        case 1415:
            phase = WP_PHASE_OTHER_HAND;
            break;
        case 1410:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_OTHER_HAND;
            break;
        case 1431:
            if (Motion_ReachedFrame(pl, 0, 20.0f)) phase = WP_PHASE_HAND;
            else if (Motion_ReachedFrame(pl, 0, 6.0f)) phase = WP_PHASE_OTHER_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    case WEAPON_LONG_SWORD:
        switch (pl->motionId[0]) {
        case 1002:
            if (Motion_ReachedFrame(pl, 0, 56.0f)) phase = WP_PHASE_HAND;
            else if (Motion_ReachedFrame(pl, 0, 16.0f)) phase = WP_PHASE_OTHER_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 1003:
            if (Motion_ReachedFrame(pl, 0, 52.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_OTHER_HAND;
            break;
        case 1415:
            if (Motion_ReachedFrame(pl, 0, 28.0f)) phase = WP_PHASE_HAND;
            else if (Motion_ReachedFrame(pl, 0, 8.0f)) phase = WP_PHASE_OTHER_HAND; else phase = WP_PHASE_SHEATHED;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 1426:
            if (Motion_ReachedFrame(pl, 0, 4.0f)) phase = WP_PHASE_OTHER_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 1005:
            if (Motion_ReachedFrame(pl, 0, 44.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_OTHER_HAND;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    case WEAPON_BOW:
        switch (pl->motionId[0]) {
        case 1002:
            if (Motion_ReachedFrame(pl, 0, 18.0f)) phase = WP_PHASE_HAND;
            break;
        case 1010:
            if (Motion_ReachedFrame(pl, 0, 12.0f)) phase = WP_PHASE_HAND;
            break;
        case PL_MOT_BOW_CHARGE_FROM_DRAW:
            if (Motion_ReachedFrame(pl, 0, 6.0f)) phase = WP_PHASE_HAND;
            break;
        case 1014:
            if (Motion_ReachedFrame(pl, 0, 4.0f)) phase = WP_PHASE_HAND;
            break;
        case 1003:
            if (Motion_ReachedFrame(pl, 0, 64.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_HAND;
            break;
        case 1005:
            if (Motion_ReachedFrame(pl, 0, 78.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_HAND;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    case WEAPON_HEAVY_BOWGUN:
        switch (pl->motionId[0]) {
        case 1002:
            if (Motion_ReachedFrame(pl, 0, 2.0f)) phase = WP_PHASE_HAND;
            break;
        case 1003:
            if (Motion_ReachedFrame(pl, 0, 110.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_HAND;
            break;
        case 1009:
            if (Motion_ReachedFrame(pl, 0, 92.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_HAND;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    case WEAPON_LIGHT_BOWGUN:
        switch (pl->motionId[0]) {
        case 1002:
            if (Motion_ReachedFrame(pl, 0, 28.0f)) phase = WP_PHASE_OTHER_HAND; else phase = WP_PHASE_SHEATHED;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 1422:
            if (Motion_ReachedFrame(pl, 0, 26.0f)) phase = WP_PHASE_HAND; else phase = WP_PHASE_SHEATHED;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 1423:
            if (Motion_ReachedFrame(pl, 0, 24.0f)) phase = WP_PHASE_HAND; else phase = WP_PHASE_SHEATHED;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 1010:
            if (Motion_ReachedFrame(pl, 0, 22.0f)) phase = WP_PHASE_OTHER_HAND; else phase = WP_PHASE_SHEATHED;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 1003:
        case 1009:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_HAND; else phase = WP_PHASE_SHEATHED;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    }

    switch (pl->motionId[0]) {
    case 202:
    case 204:
        if (pl->weaponOut) phase = WP_PHASE_HAND;
        break;
    case 207:
    case 216:
        if (pl->weaponOut) {
            if (pl->stateGroup == PL_GROUP_GESTURE) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
        }
        break;
    case 60:
    case 61:
    case 640:
    case 620:
    case 621:
    case 622:
    case 623:
    case 624:
    case 625:
    case 626:
    case 627:
    case 628:
    case 629:
    case 630:
    case 631:
    case 632:
    case 633:
        phase = WP_PHASE_SHEATHED;
        break;
    case 205:
    case 206:
    case 208:
    case 209:
    case 211:
    case 212:
    case 213:
    case 214:
    case 215:
    case 219:
    case 220:
        phase = WP_PHASE_SHEATHED;
        break;
    case 210:
        if (pl->stateGroup == PL_GROUP_GESTURE) phase = WP_PHASE_SHEATHED;
        break;
    case 413:
        if (Motion_ReachedFrame(pl, 0, 328.0f)) phase = WP_PHASE_SHEATHED;
        else if (Motion_ReachedFrame(pl, 0, 22.0f)) phase = WP_PHASE_HAND;
        else phase = WP_PHASE_SHEATHED;
        if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
        break;
    case 841:
        if (otherPlayer == 1) phase = WP_PHASE_SHEATHED;
        break;
    }
    return phase;
}

extern f32 gBoneMarkerDirs[][3];
extern f32 gObjMarkerOffset[];

/* Draws the target marker (Hud_DrawTargetMarker) at bone record 14 of the player, moved
 * along direction dirIndex of gBoneMarkerDirs turned by that bone. */
void Player_DrawBoneMarker(Player *pl, u8 dirIndex)
{
    f32 pos[3];
    f32 dir[3];

    Vec_Rotate(dir, gBoneMarkerDirs[dirIndex], pl->partRecs[14]->m1);
    Mtx_GetTrans(pos, pl->partRecs[14]->m1);
    Vec_Add(pos, pos, dir);
    Hud_DrawTargetMarker(pos);
}

/* Draws the target marker (Hud_DrawTargetMarker) in front of the player: its position plus
 * gObjMarkerOffset turned by its aim (-aimPitch and rot[1], 16-bit turns), 100 higher. */
void Hud_DrawObjMarker(Player *pl)
{
    f32 rotMtx[16];
    f32 markerPos[3];

    Mtx_Identity(rotMtx);
    Mtx_RotateXYZ(rotMtx, 6.2831855f * (f32)(-pl->aimPitch) / 65536.0f,
                  6.2831855f * (f32)pl->rot[1] / 65536.0f, 0.0f);
    Vec_Rotate(markerPos, gObjMarkerOffset, rotMtx);
    Vec_Add(markerPos, markerPos, pl->pos);
    markerPos[1] += 100.0f;
    Hud_DrawTargetMarker(markerPos);
}

extern u8 gFishColors[][3];
extern u8 gFoodMat0Colors[][3];
extern u8 gFoodMat1Colors[][3];

/* Draws one part of a held prop with the matrix mtx and its materials (lights: the model's
 * ModelMat array, read as bytes), each with the player's alpha. mode colours them: 1 and 2 the
 * first two materials by row colorRow of gFoodMat0Colors and gFoodMat1Colors (1: the spit
 * roast's cookResult; 2: the food eaten, useItem, or nothing drawn for another item); 3, 4
 * and 5 the first material dark, white or light grey; 6 the second colour (RS_COLOR2) set to
 * fish colour 3 (gFishColors); 0 as the model has them. */
void Player_DrawPartLit(Player *pl, ModelMatSet *part, f32 *mtx, u8 *lights, u8 mode)
{
    s16 colorRow;
    s32 i;
    u8 red;
    u8 green;
    u8 blue;
    f32 *light;

    if (part != NULL && part->id != -1) {
        switch (mode) {
        case 1:
            colorRow = pl->cookResult;
            break;
        case 2:
            switch (pl->useItem) {
            case ITEM_ID_FOOD_STAMINA_75:
            case ITEM_ID_FOOD_RESTORE:
                colorRow = 1;
                break;
            case ITEM_ID_FOOD_STAMINA_150:
            case ITEM_ID_FOOD_REGEN_SHORT:
            case ITEM_ID_FOOD_REGEN:
            case ITEM_ID_FOOD_REGEN_B:
            case ITEM_ID_FOOD_REGEN_LONG:
                colorRow = 2;
                break;
            case ITEM_ID_FOOD_STAMINA_GAMBLE:
            case ITEM_ID_FOOD_RESTORE_B:
                colorRow = 3;
                break;
            case ITEM_ID_FOOD_STAMINA_FULL:
                colorRow = 4;
                break;
            case ITEM_ID_FOOD_HOT:
            case ITEM_ID_FOOD_HOT_STAMINA:
            case ITEM_ID_FOOD_COOL:
            case ITEM_ID_FOOD_COOL_STAMINA:
            case ITEM_ID_FOOD_CURE:
            case ITEM_ID_FOOD_CURE_STAMINA:
                colorRow = 4;
                break;
            default:
                return;
            }
            break;
        case 3:
            red = green = blue = 63;
            break;
        case 4:
            red = green = blue = 255;
            break;
        case 5:
            red = green = blue = 159;
            break;
        case 6:
            Gs_SetState(RS_COLOR2, 0xFF000000 | gFishColors[3][0] << 16 | gFishColors[3][1] << 8 | gFishColors[3][2]);
            break;
        }
        Gs_SetState(RS_MTX_SLOT(0), (u32)mtx);
        for (i = 0; i < part->count; i++) {
            light = (f32 *)(lights + part->mat[i] * 76);
            light[4] = pl->alpha;
            switch (mode) {
            case 1:
            case 2:
                switch (i) {
                case 0:
                    light[1] = gFoodMat0Colors[colorRow][0] / 255.0f;
                    light[2] = gFoodMat0Colors[colorRow][1] / 255.0f;
                    light[3] = gFoodMat0Colors[colorRow][2] / 255.0f;
                    break;
                case 1:
                    light[1] = gFoodMat1Colors[colorRow][0] / 255.0f;
                    light[2] = gFoodMat1Colors[colorRow][1] / 255.0f;
                    light[3] = gFoodMat1Colors[colorRow][2] / 255.0f;
                    break;
                }
                break;
            case 3:
            case 4:
            case 5:
                switch (i) {
                case 0:
                    light[1] = red / 255.0f;
                    light[2] = green / 255.0f;
                    light[3] = blue / 255.0f;
                    break;
                }
                break;
            }
            Gs_SetState((u8)RS_MATERIAL(i), (u32)light);
        }
        Draw_ApplyStates(part->flags);
        Model_Draw(part->id, 0);
    }
}


/* Picks the sound or effect (Effect_MapTexId) of motions PL_MOT_FISH_REEL to PL_MOT_FISH_PULLED_DOWN by the frames reached, and returns
 * its (s16) result. */
s16 Player_HeldPartForMotion(void *obj)
{
    ObjHead *head = obj;
    s16 texId;

    switch (head->motionId[0]) {
    case 807:
        if (Motion_ReachedFrame(head, 0, 20.0f) == 0) {
            texId = 68;
        } else if (Motion_ReachedFrame(head, 0, 30.0f) == 0) {
            texId = 131;
        } else {
            texId = 130;
        }
        break;
    case 808:
        if (Motion_ReachedFrame(head, 0, 20.0f) == 0) {
            texId = 131;
        } else if (Motion_ReachedFrame(head, 0, 26.0f) == 0) {
            texId = 130;
        } else if (Motion_ReachedFrame(head, 0, 28.0f) == 0) {
            texId = 131;
        } else {
            texId = 68;
        }
        break;
    case 809:
        if (Motion_ReachedFrame(head, 0, 36.0f) == 0) {
            texId = 131;
        } else if (Motion_ReachedFrame(head, 0, 40.0f) == 0) {
            texId = 130;
        } else if (Motion_ReachedFrame(head, 0, 42.0f) == 0) {
            texId = 131;
        } else {
            texId = 68;
        }
        break;
    case 810:
        if (Motion_ReachedFrame(head, 0, 20.0f) == 0) {
            texId = 131;
        } else if (Motion_ReachedFrame(head, 0, 22.0f) != 0) {
            texId = 130;
        } else if (Motion_ReachedFrame(head, 0, 24.0f) != 0) {
            texId = 131;
        } else {
            texId = 68;
        }
        break;
    default:
        texId = 68;
        break;
    }
    return Effect_MapTexId(texId);
}

s16 Player_IsState(void *obj, u8 state, u8 sub);

/* Draws the prop the player holds, a part of the stage's model set (gStageModelSet) placed at
 * a bone: the carried item (Player.carry: its kind picks the part, size and colour), the mash
 * status's prop (PL_STATUS_ICON_MASH), or the prop of the motion playing: the item motions
 * PL_MOT_PICKAXE_SWING to PL_MOT_TEAM_ITEM_END (PL_MOT_EATING: the food by useItem), the spit roast (PL_MOT_SPIT_ROAST_START to PL_MOT_SPIT_ROAST_TURN: the
 * meat, with the roaster standing on the ground in front of the player), motions PL_MOT_FISH_READY to
 * PL_MOT_FISH_PULLED_DOWN (the part Player_HeldPartForMotion picks), and the bow's arrow in its motions. */
/* The player comes in as `void *obj`, read through `Player *pl = obj;`, as the other player
 * helpers take it: with a Player * parameter the compiler swaps the two registers that hold
 * the player and the model set.
 * FAKEMATCH: the spit roast's last motion (PL_MOT_SPIT_ROAST_TURN) reaches off[1] and off[2] through the
 * pointers offY and offZ. Written as off[1] and off[2], the compiler gives that block its own
 * copies of their addresses, 4 instructions more than the game's
 * (docs/notes/hard-matches/Player_DrawHeldProp.md). */
void Player_DrawHeldProp(void *obj)
{
    Model *set;
    Player *pl = obj;
    u8 *lights;
    ModelMatSet *part;
    ModelMatSet *roasterPart;
    s16 drawn;
    s32 onBone;
    s32 idx;
    s16 bone;
    s16 mode;
    s16 stand;
    f32 *offY;
    f32 *offZ;

    f32 pos[3];
    f32 off[3];
    f32 propMtx[16];
    f32 boneMtx[16];
    f32 uvMtx[16];
    f32 scaleX;
    f32 scaleY;
    f32 scaleZ;
    f32 rotX;
    f32 rotY;
    f32 rotZ;
    f32 reachFrame;
    f32 x;
    f32 y;
    f32 z;

    drawn = 0;
    mode = 0;
    set = gStageModelSet;
    if (set != NULL && set->active != 0) {
        lights = (u8 *)set->mats;
        scaleX = 1.0f;
        scaleY = scaleX;
        scaleZ = scaleX;
        if (pl->carry & PL_CARRY_HOLD_MASK) {
            if (Player_IsState((u8 *)pl, PL_GROUP_CARRY, PL_CARRY_PICKED_UP) != 0 && pl->motionId[0] == 51 && pl->motion[0].blend != 0) {
                return;
            }
            switch ((pl->carry & PL_CARRY_KIND_MASK) >> PL_CARRY_KIND_SHIFT) {
            case 0:
            case 1:
            case 6:
                bone = PL_BONE_HAND;
                part = &set->matSets[133];
                scaleY *= 1.3f;
                rotX = -0.9424778f;
                rotY = -3.2812192f;
                rotZ = 2.0594885f;
                off[0] = 5.5f;
                off[1] = -28.0f;
                off[2] = 12.0f;
                Mtx_Translation(uvMtx, 0.0f, 0.0625f, 0.0f);
                Gs_SetState(RS_UV_MTX, (u32)uvMtx);
                break;
            case 2:
                rotX = -0.9424778f;
                bone = PL_BONE_HAND;
                part = &set->matSets[133];
                rotY = -3.2812192f;
                rotZ = 2.0594885f;
                off[0] = 8.9f;
                off[1] = -25.0f;
                off[2] = 4.6f;
                Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (u32)uvMtx);
                break;
            case 3:
            case 4:
            case 5:
                part = &set->matSets[41];
                scaleX *= 2.5f;
                scaleY *= 3.0f;
                scaleZ *= 2.0f;
                bone = PL_BONE_HAND;
                rotX = -0.24434611f;
                rotY = 2.86234f;
                rotZ = 1.2566371f;
                off[0] = 3.3f;
                off[1] = -24.0f;
                off[2] = 4.0f;
                switch ((pl->carry & PL_CARRY_KIND_MASK) >> PL_CARRY_KIND_SHIFT) {
                case 5:
                    mode = 3;
                    break;
                case 3:
                    mode = 4;
                    break;
                case 4:
                    mode = 5;
                    break;
                }
                break;
            default:
                return;
            }
        } else if (pl->statusIcons & PL_STATUS_ICON_MASH) {
            rotZ = 0.0f;
            bone = 2;
            rotY = rotZ;
            rotX = rotZ;
            part = &set->matSets[145];
            off[2] = 0.0f;
            off[1] = 0.0f;
            off[0] = 0.0f;
        } else {
            switch (pl->motionId[0]) {
            case 415:
                part = &set->matSets[Effect_MapTexId(70)];
                bone = PL_BONE_HAND;
                rotX = 0.36651915f;
                rotY = 0.17453294f;
                rotZ = 1.3613569f;
                off[0] = 17.0f;
                off[1] = 0.9f;
                off[2] = 22.0f;
                break;
            case 416:
                if ((pl->motion[0].blend == 0 || pl->stateVar1 == 0)
                    && (Motion_ReachedFrame(pl, 0, 16.0f) == 0 || Motion_ReachedFrame(pl, 0, 164.0f) != 0)) {
                    return;
                }
                switch (pl->useItem) {
                case ITEM_ID_FOOD_HEAL_20:
                case ITEM_ID_FOOD_PRIZE_A:
                case ITEM_ID_FOOD_PRIZE_B:
                    part = &set->matSets[114];
                    bone = PL_BONE_OTHER_HAND;
                    rotX = -1.2217306f;
                    rotY = 0.08726647f;
                    rotZ = -0.38397247f;
                    mode = 6;
                    off[0] = -8.0f;
                    off[1] = -3.0f;
                    off[2] = 0.0f;
                    break;
                case ITEM_ID_FOOD_RESTORE:
                case ITEM_ID_FOOD_REGEN_SHORT:
                case ITEM_ID_FOOD_REGEN:
                case ITEM_ID_FOOD_REGEN_B:
                case ITEM_ID_FOOD_REGEN_LONG:
                case ITEM_ID_FOOD_RESTORE_B:
                    scaleX = 0.75f;
                    scaleY = scaleX;
                    part = &set->matSets[128];
                    bone = PL_BONE_OTHER_HAND;
                    rotX = -0.38397247f;
                    rotY = 0.296706f;
                    mode = 2;
                    rotZ = -1.9722221f;
                    off[0] = -17.0f;
                    off[1] = -30.0f;
                    off[2] = 1.0f;
                    scaleZ = scaleX;
                    break;
                default:
                    part = &set->matSets[58];
                    bone = PL_BONE_OTHER_HAND;
                    rotX = -0.38397247f;
                    rotY = 0.296706f;
                    rotZ = -1.9722221f;
                    mode = 2;
                    off[0] = -17.0f;
                    off[1] = -30.0f;
                    off[2] = 1.0f;
                    break;
                }
                break;
            case 418:
                if (Motion_ReachedFrame(pl, 0, 118.0f) != 0) {
                    return;
                }
                rotX = 0.0f;
                bone = PL_BONE_OTHER_HAND;
                rotY = 3.1415927f;
                rotZ = rotY;
                part = &set->matSets[71];
                off[0] = -9.0f;
                off[1] = -1.0f;
                off[2] = 65.0f;
                break;
            case 419:
            case 422:
                switch (pl->motionId[0]) {
                case 419:
                    if (Motion_ReachedFrame(pl, 0, 26.0f) == 0 || Motion_ReachedFrame(pl, 0, 248.0f) != 0) {
                        return;
                    }
                    break;
                case 422:
                    if (Motion_ReachedFrame(pl, 0, 20.0f) == 0) {
                        return;
                    }
                    break;
                }
                rotX = 0.0f;
                bone = PL_BONE_HAND;
                rotY = rotX;
                rotZ = rotX;
                part = &set->matSets[120];
                off[0] = 8.0f;
                off[1] = 0.0f;
                off[2] = -6.0f;
                break;
            case 420:
            case 421:
                if (pl->motionId[0] == 420) {
                    if (Motion_ReachedFrame(pl, 0, 6.0f) == 0) {
                        return;
                    }
                } else if (pl->motionId[0] == 421) {
                    if (Motion_ReachedFrame(pl, 0, 34.0f) != 0) {
                        return;
                    }
                }
                bone = PL_BONE_OTHER_HAND;
                rotX = -1.0821042f;
                rotY = -2.9670596f;
                part = &set->matSets[19];
                rotZ = 0.69813174f;
                off[0] = -8.4f;
                off[1] = -3.8f;
                off[2] = -5.2f;
                break;
            case 424:
            case 425:
                if (pl->motionId[0] == 424) {
                    if (pl->motion[0].blend != 0) {
                        return;
                    }
                } else if (Motion_ReachedFrame(pl, 0, 58.0f) != 0) {
                    return;
                }
                bone = PL_BONE_OTHER_HAND;
                rotX = 2.86234f;
                rotY = -0.41887903f;
                part = &set->matSets[83];
                rotZ = -1.0646509f;
                off[0] = -12.0f;
                off[1] = -7.0f;
                off[2] = -3.0f;
                break;
            case 800:
            case 801:
            case 802:
            case 803:
                switch (pl->useItem) {
                case 163:
                case 164:
                case 165:
                case 166:
                case 167:
                default:
                    idx = 58;
                    break;
                case 168:
                case 169:
                    idx = 128;
                    break;
                }
                stand = 1;
                switch (pl->motionId[0]) {
                case 800:
                    if (Motion_ReachedFrame(pl, 0, 26.0f) == 0) {
                        return;
                    }
                    onBone = 0;
                    break;
                case 801:
                    if (Motion_ReachedFrame(pl, 0, 106.0f) != 0) {
                        return;
                    }
                    if (Motion_ReachedFrame(pl, 0, 96.0f) != 0) {
                        stand = 0;
                    }
                    onBone = 1;
                    break;
                case 802:
                    if (Motion_ReachedFrame(pl, 0, 100.0f) != 0) {
                        return;
                    }
                    if (Motion_ReachedFrame(pl, 0, 70.0f) != 0) {
                        stand = 2;
                    }
                    onBone = 1;
                    break;
                default:
                    onBone = 0;
                    break;
                }
                if (stand) {
                    roasterPart = &set->matSets[69];
                    Mtx_Translation(propMtx, pl->pos[0], pl->floorY, pl->pos[2]);
                    Mtx_RotateY(propMtx, (6.2831855f * pl->rot[1]) / 65536.0f);
                    Gs_SetState(RS_MTX_SLOT(0), (u32)propMtx);
                    Gs_SetState(RS_COLOR2, -1);
                    Player_DrawPartLit(pl, roasterPart, propMtx, lights, 0);
                    if (stand == 2) {
                        return;
                    }
                }
                if (onBone) {
                    part = &set->matSets[idx];
                    bone = PL_BONE_HAND;
                    rotX = 1.0471976f;
                    rotY = -0.7330383f;
                    rotZ = 0.41887903f;
                    off[0] = 37.0f;
                    off[1] = 14.0f;
                    off[2] = 37.0f;
                } else {
                    off[0] = 0.0f;
                    off[1] = 0.0f;
                    off[2] = 128.0f;
                    Vec_RotateY(off, (6.2831855f * pl->rot[1]) / 65536.0f);
                    pos[0] = pl->pos[0] + off[0];
                    pos[1] = 77.0f + pl->floorY;
                    pos[2] = pl->pos[2] + off[2];
                    off[0] = -52.0f;
                    off[1] = 0.0f;
                    off[2] = 0.0f;
                    rotY = (6.2831855f * (pl->rot[1] + 0x7FFF + 0x4001)) / 65536.0f;
                    Vec_RotateY(off, rotY);
                    x = pos[0] + off[0];
                    y = pos[1] + off[1];
                    z = pos[2] + off[2];
                    if (pl->motionId[0] == 803) {
                        offY = &off[1]; /* FAKEMATCH: see above */
                        offZ = &off[2];
                        off[0] = 12.0f;
                        *offY = 0.0f;
                        *offZ = 9.0f;
                        Mtx_Copy(propMtx, (void *)Em_GetBone(pl, PL_BONE_HAND));
                        Vec_RotateSelf(off, propMtx);
                        off[0] += propMtx[12];
                        *offY += propMtx[13];
                        *offZ += propMtx[14];
                        off[0] -= x;
                        *offY -= y;
                        *offZ -= z;
                        Vec_RotateY(off, -rotY);
                        rotX = Math_Atan2(*offZ, *offY);
                    } else {
                        rotX = 0.0f;
                    }
                    Mtx_Identity(propMtx);
                    Mtx_RotateX(propMtx, rotX);
                    Mtx_RotateY(propMtx, rotY);
                    roasterPart = &set->matSets[60];
                    Mtx_SetTrans(propMtx, x, y, z);
                    Gs_SetState(RS_COLOR2, 0xFF727162);
                    Player_DrawPartLit(pl, roasterPart, propMtx, lights, 0);
                    part = &set->matSets[idx];
                    rotZ = 0.0f;
                    drawn = 1;
                    Mtx_SetTrans(propMtx, pos[0], pos[1], pos[2]);
                }
                mode = 1;
                break;
            case 810:
                if (Motion_ReachedFrame(pl, 0, 22.0f) != 0) {
                    return;
                }
            case 804:
            case 805:
            case 806:
            case 807:
            case 808:
            case 809:
                part = &set->matSets[Player_HeldPartForMotion(pl)];
                bone = PL_BONE_OTHER_HAND;
                rotZ = -1.5707964f;
                off[0] = -9.2f;
                rotX = 0.0f;
                off[1] = -1.7f;
                rotY = rotX;
                off[2] = 12.0f;
                break;
            case 1009:
            case 1012:
            case 1013:
            case 1405:
            case 1418:
                if (pl->weaponType == WEAPON_BOW) {
                    switch (pl->motionId[0]) {
                    case PL_MOT_BOW_CHARGE_FROM_DRAW:
                        reachFrame = 6.0f;
                        if (Motion_ReachedFrame(pl, 0, reachFrame) == 0) {
                            return;
                        }
                        break;
                    case PL_MOT_BOW_CHARGE_FROM_STAB:
                        reachFrame = 4.0f;
                        if (Motion_ReachedFrame(pl, 0, reachFrame) == 0) {
                            return;
                        }
                        break;
                    default:
                        reachFrame = 0.0f;
                        break;
                    }
                    scaleX = scaleY = 1.0f;
                    if (((u32)(pl->motionId[0] - PL_MOT_BOW_CHARGE_FROM_DRAW) <= 1 || pl->motionId[0] == PL_MOT_BOW_CHARGE
                        || pl->motionId[0] == PL_MOT_BOW_CHARGE_FROM_SHOT)
                        && Motion_ReachedFrame(pl, 0, reachFrame) != 0 && Motion_ReachedFrame(pl, 0, 12.0f + reachFrame) == 0) {
                        scaleZ = (0.5f * (pl->motion[0].frame - reachFrame)) / 12.0f;
                    } else {
                        scaleZ = scaleX;
                    }
                    bone = PL_BONE_OTHER_HAND;
                    part = &set->matSets[149];
                    rotX = 1.5707964f;
                    rotY = 0.0f;
                    rotZ = -1.5707964f;
                    off[0] = -10.0f;
                    off[1] = 0.0f;
                    off[2] = 0.0f;
                } else {
                    return;
                }
                break;
            case 1014:
            case 1411:
            case 1413:
            case 1414:
                if (pl->weaponType == WEAPON_BOW) {
                    switch (pl->motionId[0]) {
                    case 1014:
                        if (Motion_ReachedFrame(pl, 0, 12.0f) == 0) {
                            return;
                        }
                        break;
                    case 1411:
                        if (Motion_ReachedFrame(pl, 0, 76.0f) != 0) {
                            return;
                        }
                        break;
                    case 1413:
                        if (Motion_ReachedFrame(pl, 0, 12.0f) == 0 || Motion_ReachedFrame(pl, 0, 98.0f) != 0) {
                            return;
                        }
                        break;
                    case 1414:
                        if (Motion_ReachedFrame(pl, 0, 100.0f) != 0) {
                            return;
                        }
                        break;
                    }
                    bone = PL_BONE_OTHER_HAND;
                    part = &set->matSets[149];
                    rotX = 3.1415927f;
                    rotY = 0.0f;
                    rotZ = -1.5707964f;
                    off[0] = -10.0f;
                    off[1] = 0.0f;
                    off[2] = 80.0f;
                    scaleZ = 0.9f;
                    scaleY = scaleZ;
                    scaleX = scaleZ;
                } else {
                    return;
                }
                break;
            default:
                return;
            }
        }
        if (drawn == 0) {
            Mtx_Copy(boneMtx, (void *)Em_GetBone(pl, bone));
            Mtx_GetTrans(pos, boneMtx);
            Vec_RotateSelf(off, boneMtx);
            pos[0] += off[0];
            pos[1] += off[1];
            pos[2] += off[2];
            Mtx_Scale(propMtx, scaleX, scaleY, scaleZ);
            Mtx_RotateXYZ(propMtx, rotX, rotY, rotZ);
            Mtx_Concat3x3(propMtx, boneMtx);
        }
        Mtx_SetTrans(propMtx, pos[0], pos[1], pos[2]);
        Gs_SetState(RS_COLOR2, -1);
        Player_DrawPartLit(pl, part, propMtx, lights, mode);
        Draw_SetDefaultState();
    }
}

/* Where a weapon model sits on its bone: an offset in the bone's frame and a rotation (radians). */
typedef struct WpPose {
    f32 off[3];
    f32 rot[3];
} WpPose;

extern WpPose gWpPoseDrawnAlt[];
extern WpPose gWpPoseDrawn[];
extern WpPose gWpPoseSheathed[];
extern WpPose gWpPoseSharpen[];
extern Key1 gLbgKey579ScaleZ[];
extern Key1 gHbgKey579ScaleZ[];
extern Key1 gLbgKey579ScaleY[];
extern Key1 gHbgKey579ScaleX[];
extern Key1 gLbgKey57CScaleZ[];
extern Key1 gHbgKey57CScaleZ[];
extern Key1 gLbgKey57CScaleY[];
extern Key1 gHbgKey57CScaleX[];
extern Key1 gLbgKey57DEmptyScaleZ[];
extern Key1 gHbgKey57DEmptyScaleZ[];
extern Key1 gLbgKey57DEmptyScaleY[];
extern Key1 gHbgKey57DEmptyScaleX[];
extern Key1 gLbgKey57DScaleZ[];
extern Key1 gHbgKey57DScaleZ[];
extern Key1 gLbgKey57DScaleY[];
extern Key1 gHbgKey57DScaleX[];
extern Key1 gBowgunKey580ScaleZ[];
extern Key1 gBowgunKey580Scale2[];
extern Key1 gHbgKey3EARotX[];
extern Key1 gHbgKey3EAScaleZ[];
extern Key1 gHbgKey3EAScaleX[];
extern Key1 gHbgKey3EBRotX[];
extern Key1 gHbgKey3F1ScaleZ[];
extern Key1 gHbgKey3EBScaleX[];
extern Key3 gLbgKey3EARot[];
extern Key3 gLbgKey3EAOff[];
extern Key3 gLbgKey3EBRot[];
extern Key3 gLbgKey3EBOff[];
extern Key3 gLbgKey3F1Rot[];
extern Key3 gLbgKey3F1Off[];
extern Key3 gLbgKey3F2Rot[];
extern Key3 gLbgKey3F2Off[];
extern Key1 gLanceKey19DScaleZ[];
extern Key1 gLanceKey3EAScaleZ[];
extern Key1 gLanceKey3EBScaleZ[];
extern Key1 gLanceKey3F1ScaleZ[];
extern Key1 gLanceKey3F2ScaleZ[];
extern Key1 gLanceKey3FAScaleZ[];
extern Key1 gHammerKey57EScale[];
extern Key1 gHammerKey57CScale[];
extern Key1 gGsKey19DScaleZ[];
extern Key1 gGsKey3EAScaleZ[];
extern Key1 gGsKey3EBScaleZ[];
extern Key1 gGsKey3F9ScaleZ[];
extern Key1 gGsKey582ScaleZ[];
extern Key2 gGsKey19DScaleXZ[];
extern Key2 gGsKey3EAScaleXZ[];
extern Key2 gGsKey3EBScaleXZ[];
extern Key2 gGsKey3F9ScaleXZ[];
extern Key2 gGsKey582ScaleXZ[];
extern Key2 gHornKey57DRot[];
extern Key3 gHornKey57DScale[];
extern Key1 gGlKey19DRotX[];
extern Key1 gGlKey3EARotX[];
extern Key1 gGlKey3EBRotX[];
extern Key1 gGlKey3F2RotX[];
extern Key1 gGlKey57CRotX[];
extern Key3 gGlKey580Rot[];
extern Key3 gGlKeyTimerRot[];
extern Key1 gBowKey3EARotX[];
extern Key1 gBowKey3EBRotX[];
extern Key1 gBowKey3EDRotX[];
extern Key1 gBowKey3F4RotX[];
extern Key1 gBowKey3F6RotX[];
extern Key1 gBowKey57DRotX[];
extern Key1 gBowKey580RotX[];
extern Key1 gBowKey587RotX[];
extern Key1 gBowKey58ARotX[];
extern Key1 gBowKey3EAScaleZ[];
extern Key1 gBowKey3EBScaleZ[];
extern Key1 gBowKey3EDScaleZ[];
extern Key1 gBowKey3F4ScaleZ[];
extern Key1 gBowKey3F6ScaleZ[];
extern Key1 gBowKey57DScaleZ[];
extern Key1 gBowKey580ScaleZ[];
extern Key1 gBowKey587ScaleZ[];
extern Key1 gBowKey58AScaleZ[];

#pragma optimization_level 4
/*
 * Draws the player's weapon (weaponModel) at the alpha given:
 * - places it by Player_MotionPhase: on the hand's bone (PL_BONE_HAND, PL_BONE_OTHER_HAND) with
 *   gWpPoseDrawn/DrawnAlt (gWpPoseSharpen while sharpening, PL_MOT_SHARPEN), or sheathed on part
 *   record 9 (the sword and shield, the dual swords) or 10 (the others) with gWpPoseSheathed,
 *   scaled per weapon;
 * - scales and turns its moving bones by weapon type and motion, from keyframe tracks of the
 *   weapon's own motions (Key1_Eval, Key2_Eval, Key3_Eval) or fixed values; a bowgun's
 *   materials show by its parts (Bowgun_HasBarrel, HeavyBowgun_HasShield,
 *   LightBowgun_HasSilencer);
 * - draws its material sets, the second one of a lance, a sword and shield or dual swords (the
 *   shield or the other blade) on bone 17, by the second part's phase;
 * - then, for the long sword, the bow and the gunlance, draws again the materials that stay on
 *   the back while the weapon is out, on part record 10 by gWpPoseSheathed.
 * otherPlayer is set when Player_Draw draws another player's hunter (Player_MotionPhase).
 */
void Player_DrawWeapon(Player *pl, s32 otherPlayer, f32 alpha)
{
    Model *wpModel;
    ModelMatSet *set;
    WpPose *pose;
    ModelMat *material;
    f32 *poseSrc;
    s32 i;
    s32 j;
    s16 phase;
    s16 secondPhase;
    f32 scale;
    EmBone *bones;
    ModelMat *materials;
    f32 identity[16];
    f32 mtx[16];
    f32 scaleMtx[16];
    f32 off[3];
    f32 pos[3];
    f32 offWorld[3];
    f32 boneScale[3];
    f32 boneScale2[3];
    f32 rot[3];

    if (pl->weaponModel == NULL) {
        return;
    }
    Gs_SetState(RS_COLOR2, -1);
    Tex_Prepare(4, pl->index + 33);
    phase = Player_MotionPhase(pl, 0, otherPlayer);
    scale = 1.0f;
    wpModel = pl->weaponModel;
    bones = wpModel->partBones[0];
    set = wpModel->matSets;
    if (phase != WP_PHASE_SHEATHED) {
        Mtx_Copy(mtx, pl->partRecs[phase != WP_PHASE_OTHER_HAND ? PL_BONE_HAND : PL_BONE_OTHER_HAND]->m1);
        if (phase == WP_PHASE_OTHER_HAND) {
            pose = &gWpPoseDrawnAlt[pl->weaponType];
        } else {
            pose = &gWpPoseDrawn[pl->weaponType];
        }
        if (pl->motionId[0] == 413) {
            pose = &gWpPoseSharpen[pl->weaponType];
        }
        poseSrc = pose->off;
        off[0] = *poseSrc++;
        off[1] = *poseSrc++;
        off[2] = *poseSrc++;
        rot[0] = *poseSrc++;
        rot[1] = *poseSrc++;
        rot[2] = *poseSrc++;
        Vec_Rotate(offWorld, off, pl->partRecs[phase != WP_PHASE_OTHER_HAND ? PL_BONE_HAND : PL_BONE_OTHER_HAND]->m1);
        Mtx_AddTrans(mtx, offWorld);
        Mtx_Identity(identity);
        if (pl->weaponType == WEAPON_LANCE || pl->weaponType == WEAPON_LIGHT_BOWGUN) {
            Mtx_GetTrans(pos, bones[2].local);
            Mtx_Scale(bones[2].local, 1.0f, 1.0f, 1.0f);
            Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
        }
    } else {
        switch (pl->weaponType) {
        case WEAPON_SWORD_SHIELD:
            Mtx_Copy(mtx, pl->partRecs[9]->m1);
            break;
        case WEAPON_DUAL_SWORDS:
            Mtx_Copy(mtx, pl->partRecs[9]->m1);
            break;
        default:
            Mtx_Copy(mtx, pl->partRecs[10]->m1);
            break;
        }
        if ((s32)pl->weaponType == WEAPON_LIGHT_BOWGUN
            && (pl->motionId[0] == 1422 || pl->motionId[0] == 1423 || pl->motionId[0] == 1002 || pl->motionId[0] == 1003
                || pl->motionId[0] == 1009 || pl->motionId[0] == 1010)
            && pl->motion[0].blend == 0) {
            switch (pl->motionId[0]) {
            case 1002:
            case 1422:
            case 1423:
                Key3_Eval((struct KeyClock *)pl, gLbgKey3EAOff, off);
                Key3_Eval((struct KeyClock *)pl, gLbgKey3EARot, rot);
                break;
            case 1003:
                Key3_Eval((struct KeyClock *)pl, gLbgKey3EBOff, off);
                Key3_Eval((struct KeyClock *)pl, gLbgKey3EBRot, rot);
                break;
            case 1009:
                Key3_Eval((struct KeyClock *)pl, gLbgKey3F1Off, off);
                Key3_Eval((struct KeyClock *)pl, gLbgKey3F1Rot, rot);
                break;
            case 1010:
                Key3_Eval((struct KeyClock *)pl, gLbgKey3F2Off, off);
                Key3_Eval((struct KeyClock *)pl, gLbgKey3F2Rot, rot);
                break;
            }
            rot[0] = 6.2831855f * rot[0] / 360.0f;
            rot[1] = 6.2831855f * rot[1] / 360.0f;
            rot[2] = 6.2831855f * rot[2] / 360.0f;
        } else {
            pose = &gWpPoseSheathed[(s32)pl->weaponType];
            poseSrc = pose->off;
            off[0] = *poseSrc++;
            off[1] = *poseSrc++;
            off[2] = *poseSrc++;
            rot[0] = *poseSrc++;
            rot[1] = *poseSrc++;
            rot[2] = *poseSrc++;
        }
        if (pl->weaponType == WEAPON_DUAL_SWORDS) {
            Vec_Rotate(offWorld, off, pl->partRecs[9]->m1);
        } else {
            Vec_Rotate(offWorld, off, pl->partRecs[10]->m1);
        }
        Mtx_AddTrans(mtx, offWorld);
        switch (pl->weaponType) {
        case WEAPON_HEAVY_BOWGUN:
        case WEAPON_BOW:
            Mtx_GetTrans(pos, bones[3].local);
            Mtx_Scale(bones[3].local, 1.0f, 0.0f, 3.0f);
            Mtx_SetTrans(bones[3].local, pos[0], pos[1], pos[2]);
            scale = 0.8f;
            break;
        case WEAPON_LIGHT_BOWGUN:
            Mtx_GetTrans(pos, bones[3].local);
            Mtx_Scale(bones[3].local, 1.0f, 0.0f, 3.0f);
            Mtx_SetTrans(bones[3].local, pos[0], pos[1], pos[2]);
            Mtx_GetTrans(pos, bones[2].local);
            Mtx_Scale(bones[2].local, 1.0f, 1.0f, 0.5f);
            Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
            scale = 1.0f;
            break;
        case WEAPON_LONG_SWORD:
        default:
            scale = 0.8f;
            break;
        case WEAPON_SWORD_SHIELD:
        case WEAPON_DUAL_SWORDS:
            scale = 1.0f;
            break;
        case WEAPON_LANCE:
            scale = 0.9f;
            Mtx_GetTrans(pos, bones[2].local);
            Mtx_Scale(bones[2].local, 1.0f, 1.0f, 0.45f);
            Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
            break;
        }
    }
    Mtx_Identity(bones[0].local);
    Mtx_SetRotXYZ(bones[0].local, rot[0], rot[1], rot[2]);
    Mtx_Scale(scaleMtx, scale, scale, scale);
    Mtx_Concat3x3(bones[0].local, scaleMtx);
    switch (pl->weaponType) {
    case WEAPON_HAMMER:
        Mtx_GetTrans(pos, bones[3].local);
        boneScale[0] = 1.0f;
        if (pl->weaponTimer > 0 && pl->weaponTimer / 30 >= 2) {
            boneScale[0] = 1.5f;
        }
        switch (pl->motionId[0]) {
        case 1404:
            if (pl->motion[0].blend == 0) {
                boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHammerKey57CScale);
            } else {
                boneScale[0] = 1.5f;
            }
            break;
        case 1406:
            if (pl->motion[0].blend == 0) {
                boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHammerKey57EScale);
            } else {
                boneScale[0] = 1.5f;
            }
            break;
        }
        Mtx_Scale(bones[3].local, boneScale[0], boneScale[0], boneScale[0]);
        Mtx_SetTrans(bones[3].local, pos[0], pos[1], pos[2]);
        break;
    case WEAPON_HUNTING_HORN:
        Mtx_GetTrans(off, bones[2].local);
        Mtx_GetTrans(pos, bones[3].local);
        Vec_Set(rot, 0.0f, 0.0f, 0.0f);
        Vec_Set(boneScale, 1.0f, 1.0f, 1.0f);
        switch (pl->motionId[0]) {
        case 1405:
        case 1406:
        case 1407:
            Key2_Eval((struct KeyClock *)pl, gHornKey57DRot, &rot[0], &rot[2]);
            Key3_Eval((struct KeyClock *)pl, gHornKey57DScale, boneScale);
            break;
        }
        rot[0] = 6.2831855f * rot[0] / 360.0f;
        rot[1] = 6.2831855f * rot[1] / 360.0f;
        rot[2] = 6.2831855f * rot[2] / 360.0f;
        Mtx_Scale(bones[2].local, 1.0f, 1.0f, 1.0f);
        Mtx_SetRotXYZ(bones[2].local, rot[0], rot[1], rot[2]);
        Mtx_SetTrans(bones[2].local, off[0], off[1], off[2]);
        Mtx_Scale(bones[3].local, boneScale[0], boneScale[1], boneScale[2]);
        Mtx_SetTrans(bones[3].local, pos[0], pos[1], pos[2]);
        break;
    case WEAPON_GREAT_SWORD:
    case WEAPON_LONG_SWORD:
        Mtx_GetTrans(pos, bones[2].local);
        Mtx_GetTrans(off, bones[3].local);
        if (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED) {
            Vec_Set(boneScale2, 1.0f, 1.0f, 1.0f);
        } else {
            Vec_Set(boneScale2, 2.6f, 1.0f, 2.5f);
        }
        switch (pl->motionId[0]) {
        case 413:
            if (pl->motion[0].blend == 0) {
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gGsKey19DScaleZ);
                Key2_Eval((struct KeyClock *)pl, gGsKey19DScaleXZ, &boneScale2[0], &boneScale2[2]);
            } else {
                boneScale[2] = 0.73f;
            }
            break;
        case 1002:
        case 1016:
        case 1410:
            if (pl->motion[0].blend == 0) {
                if (pl->motionId[0] != 1410) {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gGsKey3EAScaleZ);
                    Key2_Eval((struct KeyClock *)pl, gGsKey3EAScaleXZ, &boneScale2[0], &boneScale2[2]);
                } else {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gGsKey582ScaleZ);
                    Key2_Eval((struct KeyClock *)pl, gGsKey582ScaleXZ, &boneScale2[0], &boneScale2[2]);
                }
            } else {
                boneScale[2] = 0.73f;
                Vec_Set(boneScale2, 1.0f, 1.0f, 1.0f);
            }
            break;
        case 1003:
        case 1017:
            if (pl->motion[0].blend == 0) {
                if (pl->motionId[0] == 1003) {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gGsKey3EBScaleZ);
                    Key2_Eval((struct KeyClock *)pl, gGsKey3EBScaleXZ, &boneScale2[0], &boneScale2[2]);
                } else {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gGsKey3F9ScaleZ);
                    Key2_Eval((struct KeyClock *)pl, gGsKey3F9ScaleXZ, &boneScale2[0], &boneScale2[2]);
                }
            } else {
                boneScale[2] = 1.0f;
                Vec_Set(boneScale2, 2.6f, 1.0f, 2.5f);
            }
            break;
        default:
            if (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED) {
                boneScale[2] = 0.73f;
            } else {
                boneScale[2] = 1.0f;
            }
            break;
        }
        if (pl->weaponType == WEAPON_LONG_SWORD) {
            if (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED) {
                boneScale[2] = 0.73f;
            } else {
                boneScale[2] = 1.0f;
            }
        }
        Mtx_Scale(bones[2].local, 1.0f, 1.0f, boneScale[2]);
        Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
        Mtx_Scale(bones[3].local, boneScale2[0], boneScale2[1], boneScale2[2]);
        Mtx_SetTrans(bones[3].local, off[0], off[1], off[2]);
        break;
    case WEAPON_LANCE:
        Mtx_GetTrans(pos, bones[2].local);
        switch (pl->motionId[0]) {
        case 413:
            if (pl->motion[0].blend == 0) {
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLanceKey19DScaleZ);
            } else {
                boneScale[2] = 0.45f;
            }
            break;
        case 1002:
        case 1010:
            if (pl->motion[0].blend == 0) {
                if (pl->motionId[0] != 1010) {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLanceKey3EAScaleZ);
                } else {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLanceKey3F2ScaleZ);
                }
            } else {
                boneScale[2] = 0.45f;
            }
            break;
        case 1003:
        case 1009:
            if (pl->motion[0].blend == 0) {
                if (pl->motionId[0] == 1003) {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLanceKey3EBScaleZ);
                } else {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLanceKey3F1ScaleZ);
                }
            } else {
                boneScale[2] = 1.0f;
            }
            break;
        case 1018:
            if (pl->motion[0].blend == 0) {
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLanceKey3FAScaleZ);
            } else {
                boneScale[2] = 0.45f;
            }
            break;
        default:
            if (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED) {
                boneScale[2] = 0.45f;
            } else {
                boneScale[2] = 1.0f;
            }
            break;
        }
        Mtx_Scale(bones[2].local, 1.0f, 1.0f, boneScale[2]);
        Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
        break;
    case WEAPON_GUNLANCE:
        Mtx_GetTrans(pos, bones[2].local);
        Vec_Set(rot, 0.0f, 0.0f, 0.0f);
        Mtx_GetTrans(pos, bones[2].local);
        Mtx_GetTrans(off, bones[3].local);
        switch (pl->motionId[0]) {
        case 413:
            if (pl->motion[0].blend == 0) {
                rot[0] = Key1_Eval((struct KeyClock *)pl, gGlKey19DRotX);
            } else {
                rot[0] = 180.0f;
            }
            break;
        case 1002:
            if (pl->motion[0].blend == 0) {
                rot[0] = Key1_Eval((struct KeyClock *)pl, gGlKey3EARotX);
            } else {
                rot[0] = 180.0f;
            }
            break;
        case 1010:
            if (pl->motion[0].blend == 0) {
                rot[0] = Key1_Eval((struct KeyClock *)pl, gGlKey3F2RotX);
            } else {
                rot[0] = 180.0f;
            }
            break;
        case 1003:
        case 1009:
            if (pl->motion[0].blend == 0) {
                rot[0] = Key1_Eval((struct KeyClock *)pl, gGlKey3EBRotX);
            } else {
                rot[0] = 0.0f;
            }
            break;
        case 1404:
            if (pl->motion[0].blend == 0) {
                rot[0] = Key1_Eval((struct KeyClock *)pl, gGlKey57CRotX);
            } else {
                rot[0] = 0.0f;
            }
            break;
        default:
            if (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED) {
                rot[0] = 180.0f;
            } else {
                rot[0] = 0.0f;
            }
            break;
        }
        Mtx_SetRotXYZ(bones[2].local, 6.2831855f * rot[0] / 360.0f, 0.0f, 0.0f);
        Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
        if (gScene.inLobby != 0) {
            Vec_Set(rot, 0.0f, 0.0f, 0.0f);
        } else if (pl->weaponTimer > 0) {
            if (pl->motionId[0] == 1408) {
                Key3_Eval((struct KeyClock *)pl, gGlKey580Rot, rot);
            } else {
                Vec_Set(rot, 69.0f, -43.0f, -30.0f);
            }
        } else if (pl->swordGaugeTimer > 0) {
            Key3_EvalByTimer(pl, gGlKeyTimerRot, rot);
        } else {
            Vec_Set(rot, 0.0f, 0.0f, 0.0f);
        }
        Mtx_SetRotXYZ(bones[3].local, 6.2831855f * rot[0] / 360.0f, 6.2831855f * rot[1] / 360.0f,
                      6.2831855f * rot[2] / 360.0f);
        Mtx_SetTrans(bones[3].local, off[0], off[1], off[2]);
        break;
    case WEAPON_BOW:
        Mtx_GetTrans(pos, bones[2].local);
        Mtx_GetTrans(off, bones[3].local);
        Vec_Set(boneScale, 1.0f, 1.0f, 1.0f);
        Vec_Set(rot, 0.0f, 0.0f, 0.0f);
        if (pl->motion[0].blend == 0) {
            switch (pl->motionId[0]) {
            case 1002:
            case 1010:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey3EARotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey3EAScaleZ);
                break;
            case 1003:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey3EBRotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey3EBScaleZ);
                break;
            case 1005:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey3EDRotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey3EDScaleZ);
                break;
            case PL_MOT_BOW_CHARGE_WALK:
                rot[0] = 120.0f;
                boneScale[2] = 2.35f;
                break;
            case PL_MOT_BOW_CHARGE_FROM_DRAW:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey3F4RotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey3F4ScaleZ);
                break;
            case 1014:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey3F6RotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey3F6ScaleZ);
                break;
            case PL_MOT_BOW_CHARGE:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey57DRotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey57DScaleZ);
                break;
            case 1408:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey580RotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey580ScaleZ);
                break;
            case 1415:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey587RotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey587ScaleZ);
                break;
            case PL_MOT_BOW_CHARGE_FROM_SHOT:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey58ARotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey58AScaleZ);
                break;
            default:
                if (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED) {
                    rot[0] = 0.0f;
                } else {
                    rot[0] = 155.0f;
                }
                break;
            }
        } else {
            if ((pl->motionId[0] != 1003 && pl->motionId[0] != 1005 && (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED))
                || pl->motionId[0] == 1002) {
                rot[0] = 0.0f;
            } else {
                rot[0] = 155.0f;
            }
            if (pl->motionId[0] == 1408) {
                rot[0] = 110.0f;
                boneScale[2] = 2.7f;
            }
        }
        Mtx_SetRotXYZ(bones[2].local, 6.2831855f * rot[0] / 360.0f, 0.0f, 0.0f);
        Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
        Mtx_Scale(bones[3].local, boneScale[0], boneScale[1], boneScale[2]);
        Mtx_SetTrans(bones[3].local, off[0], off[1], off[2]);
        break;
    case WEAPON_HEAVY_BOWGUN:
    case WEAPON_LIGHT_BOWGUN:
        if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
            Mtx_GetTrans(off, bones[3].local);
        } else {
            Mtx_GetTrans(off, bones[3].local);
            Mtx_SetRotXYZ(bones[2].local, 3.1415927f, 0.0f, 0.0f);
        }
        boneScale[0] = 1.0f;
        boneScale[1] = 1.0f;
        boneScale[2] = 1.0f;
        if (pl->motion[0].blend == 0) {
            switch (pl->motionId[0]) {
            case 1401:
                if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLbgKey579ScaleZ);
                    boneScale[1] = Key1_Eval((struct KeyClock *)pl, gLbgKey579ScaleY);
                } else {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gHbgKey579ScaleZ);
                    boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHbgKey579ScaleX);
                }
                break;
            case 1405:
                if (pl->roundsLoaded == 0) {
                    if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                        boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLbgKey57DEmptyScaleZ);
                        boneScale[1] = Key1_Eval((struct KeyClock *)pl, gLbgKey57DEmptyScaleY);
                    } else {
                        boneScale[2] = Key1_Eval((struct KeyClock *)pl, gHbgKey57DEmptyScaleZ);
                        boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHbgKey57DEmptyScaleX);
                    }
                } else if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLbgKey57DScaleZ);
                    boneScale[1] = Key1_Eval((struct KeyClock *)pl, gLbgKey57DScaleY);
                } else {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gHbgKey57DScaleZ);
                    boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHbgKey57DScaleX);
                }
                break;
            case 1408:
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowgunKey580ScaleZ);
                if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                    boneScale[1] = Key1_Eval((struct KeyClock *)pl, gBowgunKey580Scale2);
                } else {
                    boneScale[0] = Key1_Eval((struct KeyClock *)pl, gBowgunKey580Scale2);
                }
                break;
            case 1404:
                if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLbgKey57CScaleZ);
                    boneScale[1] = Key1_Eval((struct KeyClock *)pl, gLbgKey57CScaleY);
                } else {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gHbgKey57CScaleZ);
                    boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHbgKey57CScaleX);
                }
                break;
            case 1002:
            case 1422:
            case 1423:
                if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
                    Mtx_SetRotXYZ(bones[2].local, 6.2831855f * Key1_Eval((struct KeyClock *)pl, gHbgKey3EARotX) / 360.0f, 0.0f, 0.0f);
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gHbgKey3EAScaleZ);
                    boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHbgKey3EAScaleX);
                }
                break;
            case 1003:
                if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
                    Mtx_SetRotXYZ(bones[2].local, 6.2831855f * Key1_Eval((struct KeyClock *)pl, gHbgKey3EBRotX) / 360.0f, 0.0f, 0.0f);
                    boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHbgKey3EBScaleX);
                }
                break;
            case 1009:
                if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
                    Mtx_SetRotXYZ(bones[2].local, 6.2831855f * Key1_Eval((struct KeyClock *)pl, gHbgKey3EBRotX) / 360.0f, 0.0f, 0.0f);
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gHbgKey3F1ScaleZ);
                    boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHbgKey3EBScaleX);
                }
                break;
            default:
                if (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED) {
                    Mtx_SetRotXYZ(bones[1].local, 0.0f, 0.0f, 0.0f);
                    if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
                        Mtx_SetRotXYZ(bones[2].local, 0.0f, 0.0f, 0.0f);
                    }
                    Mtx_SetRotXYZ(bones[3].local, 0.0f, 0.0f, 0.0f);
                    if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                        boneScale[1] = 0.4f;
                        boneScale[2] = 2.2f;
                    } else {
                        boneScale[0] = -0.5f;
                        boneScale[2] = 3.0f;
                    }
                } else {
                    if (pl->roundsLoaded != 0) {
                        if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                            boneScale[1] = 0.55f;
                            boneScale[2] = 2.0f;
                        } else {
                            boneScale[0] = 0.8f;
                            boneScale[2] = 3.0f;
                        }
                    }
                }
                break;
            }
        } else {
            if ((pl->roundsLoaded != 0 && pl->motionId[0] != 1404) || pl->motionId[0] == 1401) {
                if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                    boneScale[1] = 0.55f;
                    boneScale[2] = 2.0f;
                } else {
                    boneScale[0] = 0.8f;
                    boneScale[2] = 3.0f;
                }
            }
            if ((pl->motionId[0] != 1003 && pl->motionId[0] != 1009 && (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED))
                || pl->motionId[0] == 1002 || pl->motionId[0] == 1422 || pl->motionId[0] == 1423) {
                Mtx_SetRotXYZ(bones[1].local, 0.0f, 0.0f, 0.0f);
                if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
                    Mtx_SetRotXYZ(bones[2].local, 0.0f, 0.0f, 0.0f);
                }
                Mtx_SetRotXYZ(bones[3].local, 0.0f, 0.0f, 0.0f);
                if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                    boneScale[1] = 0.4f;
                    boneScale[2] = 2.2f;
                } else {
                    boneScale[0] = -0.5f;
                    boneScale[2] = 3.0f;
                }
            }
        }
        Mtx_Scale(bones[3].local, boneScale[0], boneScale[1], boneScale[2]);
        Mtx_SetTrans(bones[3].local, off[0], off[1], off[2]);
        break;
    }
    Bone_UpdateWorldScaledRoot(wpModel->partBones[0], mtx);
    Bone_StoreMatrices(wpModel->partBones[0]);
    materials = pl->weaponModel->mats;
    secondPhase = Player_MotionPhase(pl, 1, otherPlayer);
    for (i = 0; i < wpModel->matSetCount; i++, set++) {
        if (i != 0 && (pl->weaponType == WEAPON_LANCE || pl->weaponType == WEAPON_SWORD_SHIELD || pl->weaponType == WEAPON_DUAL_SWORDS)) {
            bones = wpModel->partBones[1];
            Mtx_Copy(mtx, (void *)Em_GetBone(pl, 17));
            Vec_Set(rot, 0.0f, 0.0f, 0.0f);
            switch (pl->weaponType) {
            case WEAPON_SWORD_SHIELD:
                Mtx_SetRotXYZ(bones[0].local, -0.45378563f, -0.05235988f, 0.13962634f);
                off[0] = -21.0f;
                off[1] = 2.2f;
                off[2] = 5.0f;
                break;
            case WEAPON_LANCE:
                Mtx_SetRotXYZ(bones[0].local, 0.36651915f, -3.0019662f, -0.043633234f);
                off[0] = -18.0f;
                off[1] = 2.0f;
                off[2] = -14.0f;
                break;
            case WEAPON_DUAL_SWORDS:
                if (secondPhase != WP_PHASE_SHEATHED) {
                    Mtx_Copy(mtx, (void *)Em_GetBone(pl, PL_BONE_OTHER_HAND));
                    Mtx_SetRotXYZ(bones[0].local, -0.45378563f, -0.05235988f, 0.13962634f);
                    off[0] = -7.4f;
                    off[1] = -0.4f;
                    off[2] = -6.5f;
                    rot[0] = 0.0f;
                    rot[1] = 0.0f;
                    rot[2] = 3.1415927f;
                } else {
                    Mtx_Copy(mtx, pl->partRecs[9]->m1);
                    off[0] = 17.0f;
                    off[1] = 50.0f;
                    off[2] = -18.0f;
                    rot[0] = 1.6755161f;
                    rot[1] = 0.148353f;
                    rot[2] = -0.48869222f;
                }
                break;
            }
            Vec_Rotate(offWorld, off, mtx);
            Mtx_AddTrans(mtx, offWorld);
            if (pl->weaponType == WEAPON_DUAL_SWORDS) {
                Mtx_Identity(wpModel->partBones[1][0].local);
                Mtx_SetRotXYZ(wpModel->partBones[1][0].local, rot[0], rot[1], rot[2]);
            }
            Bone_UpdateWorldScaledRoot(wpModel->partBones[1], mtx);
            Bone_StoreMatrices(wpModel->partBones[1]);
        }
        if (set->id != -1) {
            for (j = 0; j < set->count; j++) {
                material = &materials[set->mat[j]];
                material->alpha = alpha;
                if (pl->weaponType == WEAPON_HEAVY_BOWGUN || pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                    if (j == 1 && Bowgun_HasBarrel(pl) == 0) {
                        material->alpha = 0.0f;
                    }
                    if (j == 2) {
                        if (pl->weaponType == WEAPON_HEAVY_BOWGUN && HeavyBowgun_HasShield(pl) == 0) {
                            material->alpha = 0.0f;
                        } else if (pl->weaponType == WEAPON_LIGHT_BOWGUN && LightBowgun_HasSilencer(pl) == 0) {
                            material->alpha = 0.0f;
                        }
                    }
                } else {
                    switch (pl->weaponType) {
                    case WEAPON_LONG_SWORD:
                        if (secondPhase != WP_PHASE_SHEATHED) {
                            switch (j) {
                            case 0:
                            case 1:
                            case 2:
                                material->alpha = alpha;
                                break;
                            default:
                                material->alpha = 0.0f;
                                break;
                            }
                        } else {
                            switch (j) {
                            case 0:
                            case 3:
                            case 4:
                                material->alpha = alpha;
                                break;
                            default:
                                material->alpha = 0.0f;
                                break;
                            }
                        }
                        break;
                    case WEAPON_BOW:
                        if (j > 0) {
                            material->alpha = alpha;
                        } else {
                            material->alpha = 0.0f;
                        }
                        break;
                    case WEAPON_GUNLANCE:
                        if (j > 0) {
                            material->alpha = alpha;
                        } else {
                            material->alpha = 0.0f;
                        }
                        break;
                    }
                }
                Gs_SetState((u8)RS_MATERIAL(j), (u32)material);
            }
        }
        Model_Draw(set->id, 0);
        if (pl->weaponType == WEAPON_LONG_SWORD && secondPhase != WP_PHASE_SHEATHED) {
            Mtx_Copy(mtx, pl->partRecs[10]->m1);
            pose = &gWpPoseSheathed[pl->weaponType];
            poseSrc = pose->off;
            off[0] = *poseSrc++;
            off[1] = *poseSrc++;
            off[2] = *poseSrc++;
            rot[0] = *poseSrc++;
            rot[1] = *poseSrc++;
            rot[2] = *poseSrc++;
            Vec_Rotate(offWorld, off, pl->partRecs[10]->m1);
            Mtx_AddTrans(mtx, offWorld);
            Mtx_Identity(bones[0].local);
            Mtx_SetRotXYZ(bones[0].local, rot[0], rot[1], rot[2]);
            Mtx_Scale(scaleMtx, 0.8f, 0.8f, 0.8f);
            Mtx_Concat3x3(bones[0].local, scaleMtx);
            Mtx_GetTrans(pos, bones[2].local);
            Mtx_GetTrans(off, bones[3].local);
            Vec_Set(boneScale2, 1.0f, 1.0f, 1.0f);
            boneScale[2] = 0.73f;
            Mtx_Scale(bones[2].local, 1.0f, 1.0f, boneScale[2]);
            Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
            Mtx_Scale(bones[3].local, boneScale2[0], boneScale2[1], boneScale2[2]);
            Mtx_SetTrans(bones[3].local, off[0], off[1], off[2]);
            Bone_UpdateWorldScaledRoot(wpModel->partBones[0], mtx);
            Bone_StoreMatrices(wpModel->partBones[0]);
            materials = pl->weaponModel->mats;
            if (set->id != -1) {
                for (j = 0; j < set->count; j++) {
                    material = &materials[set->mat[j]];
                    switch (j) {
                    case 3:
                    case 4:
                        material->alpha = alpha;
                        break;
                    default:
                        material->alpha = 0.0f;
                        break;
                    }
                    Gs_SetState((u8)RS_MATERIAL(j), (u32)material);
                }
            }
            Model_Draw(set->id, 0);
        }
        if (pl->weaponType == WEAPON_BOW) {
            Mtx_Copy(mtx, pl->partRecs[2]->m1);
            off[0] = 8.5f;
            off[1] = 0.5f;
            off[2] = -20.0f;
            rot[0] = -0.17453294f;
            rot[1] = 0.0f;
            rot[2] = -0.27925268f;
            Vec_Rotate(offWorld, off, pl->partRecs[2]->m1);
            Mtx_AddTrans(mtx, offWorld);
            Mtx_Identity(bones[0].local);
            Mtx_SetRotXYZ(bones[0].local, rot[0], rot[1], rot[2]);
            Mtx_Scale(scaleMtx, 1.0f, 1.0f, 1.0f);
            Mtx_Concat3x3(bones[0].local, scaleMtx);
            Mtx_GetTrans(pos, bones[2].local);
            Mtx_GetTrans(off, bones[3].local);
            Vec_Set(boneScale2, 1.0f, 1.0f, 1.0f);
            boneScale[2] = 1.0f;
            Mtx_Scale(bones[2].local, 1.0f, 1.0f, boneScale[2]);
            Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
            Mtx_Scale(bones[3].local, boneScale2[0], boneScale2[1], boneScale2[2]);
            Mtx_SetTrans(bones[3].local, off[0], off[1], off[2]);
            Bone_UpdateWorldScaledRoot(wpModel->partBones[0], mtx);
            Bone_StoreMatrices(wpModel->partBones[0]);
            materials = pl->weaponModel->mats;
            if (set->id != -1) {
                for (j = 0; j < set->count; j++) {
                    material = &materials[set->mat[j]];
                    switch (j) {
                    case 0:
                        material->alpha = alpha;
                        break;
                    default:
                        material->alpha = 0.0f;
                        break;
                    }
                    Gs_SetState((u8)RS_MATERIAL(j), (u32)material);
                }
            }
            Model_Draw(set->id, 0);
        }
        if (pl->weaponType == WEAPON_GUNLANCE) {
            Mtx_Copy(mtx, pl->partRecs[17]->m1);
            off[0] = -18.0f;
            off[1] = 2.0f;
            off[2] = -14.0f;
            rot[0] = 0.36651915f;
            rot[1] = 3.0019662f;
            rot[2] = -0.043633234f;
            Vec_Rotate(offWorld, off, pl->partRecs[17]->m1);
            Mtx_AddTrans(mtx, offWorld);
            Mtx_Identity(bones[0].local);
            Mtx_SetRotXYZ(bones[0].local, rot[0], rot[1], rot[2]);
            Mtx_Scale(scaleMtx, 1.0f, 1.0f, 1.0f);
            Mtx_Concat3x3(bones[0].local, scaleMtx);
            Mtx_GetTrans(pos, bones[2].local);
            Mtx_GetTrans(off, bones[3].local);
            Vec_Set(boneScale2, 1.0f, 1.0f, 1.0f);
            boneScale[2] = 1.0f;
            Mtx_Scale(bones[2].local, 1.0f, 1.0f, boneScale[2]);
            Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
            Mtx_Scale(bones[3].local, boneScale2[0], boneScale2[1], boneScale2[2]);
            Mtx_SetTrans(bones[3].local, off[0], off[1], off[2]);
            Bone_UpdateWorldScaledRoot(wpModel->partBones[0], mtx);
            Bone_StoreMatrices(wpModel->partBones[0]);
            materials = pl->weaponModel->mats;
            if (set->id != -1) {
                for (j = 0; j < set->count; j++) {
                    material = &materials[set->mat[j]];
                    switch (j) {
                    case 0:
                        material->alpha = alpha;
                        break;
                    default:
                        material->alpha = 0.0f;
                        break;
                    }
                    Gs_SetState((u8)RS_MATERIAL(j), (u32)material);
                }
            }
            Model_Draw(set->id, 0);
        }
    }
}
#pragma optimization_level reset

extern u8 *gPlayerPartFlagTbls[];

/* An entry of the per-gender (6 slots each) table of slot's lists, by pl's armourKind for the slot. */
u8 Player_PartFlags(Player *pl, u8 slot)
{
    return gPlayerPartFlagTbls[pl->gender * 6 + slot][pl->armourKind[slot]];
}

extern u32 gCycleColors[];

/* A colour cycling through the 8 of gCycleColors, 16 frames each (blended by the scene frame counter). */
u32 Color_Cycle8(void)
{
    u8 fromPart;
    u8 channel;
    u16 tick;
    f32 frac;
    f32 invFrac;
    s32 k;
    u32 fromColor;
    u32 toColor;
    u8 toPart;
    u32 color;

    color = 0xFF000000;
    tick = gScene.frame;
    k = (tick >> 4) & 7;
    fromColor = gCycleColors[k];
    invFrac = 1.0f - (frac = (tick & 0xF) / 16.0f);
    k = (k + 1) & 7;
    toColor = gCycleColors[k];
    fromPart = fromColor >> 16;
    channel = toColor >> 16;
    fromPart = fromPart * invFrac;
    toPart = channel * frac;
    channel = fromPart + toPart;
    color |= channel << 16;
    fromPart = fromColor >> 8;
    channel = toColor >> 8;
    fromPart = fromPart * invFrac;
    toPart = channel * frac;
    channel = fromPart + toPart;
    color |= channel << 8;
    fromPart = fromColor;
    channel = toColor;
    fromPart = fromPart * invFrac;
    toPart = channel * frac;
    channel = fromPart + toPart;
    return color | channel;
}

/*
 * Sets material matIndex of material set setIndex of one of the player's parts and hands it to
 * Gs_SetState as RS_MATERIAL(matIndex), with the alpha matAlpha. The part slot is an EquipType
 * (EQUIP_LEGS is the skeleton's own model) or EQUIP_SLOT_FACE.
 * - The head's first material takes the hair colour.
 * - The second material of the first set of a part whose Player_PartFlags has bit 0 takes the
 *   style colour (the cycling Color_Cycle8 for style 11).
 * - The face is hidden (alpha 0) by the head armour's flags: bit 2 hides its first material,
 *   bit 4 all of it. With headAltModel set, neither the face nor the head's style colour is
 *   touched.
 */
void Player_SetLight(Player *pl, u8 slot, s32 setIndex, s32 matIndex, ModelMat *mat, f32 matAlpha)
{
    u32 color;
    u8 headFlags;

    if (pl->style == 11) {
        color = Color_Cycle8();
    } else {
        color = pl->styleColor;
    }
    mat->alpha = matAlpha;
    switch (slot) {
    case EQUIP_HEAD:
        if (setIndex == 0 && matIndex == 0) {
            mat->rgb[0] = (pl->hairColor >> 16 & 0xFF) / 255.0f;
            mat->rgb[1] = (pl->hairColor >> 8 & 0xFF) / 255.0f;
            mat->rgb[2] = (pl->hairColor & 0xFF) / 255.0f;
        }
        if (pl->headAltModel != 0) {
            break;
        }
    case EQUIP_LEGS:
    case EQUIP_BODY:
    case EQUIP_ARMS:
    case EQUIP_WAIST:
    default:
        if ((Player_PartFlags(pl, slot) & 1) && setIndex == 0 && matIndex == 1) {
            mat->rgb[0] = (color >> 16 & 0xFF) / 255.0f;
            mat->rgb[1] = (color >> 8 & 0xFF) / 255.0f;
            mat->rgb[2] = (color & 0xFF) / 255.0f;
        }
        break;
    case EQUIP_SLOT_FACE:
        if (pl->headAltModel == 0) {
            headFlags = Player_PartFlags(pl, EQUIP_HEAD);
            if (headFlags & 2) {
                if (setIndex == 0 && matIndex == 0) {
                    mat->alpha = 0.0f;
                }
            } else if (headFlags & 4) {
                mat->alpha = 0.0f;
            }
        }
        break;
    }
    Gs_SetState((u8)RS_MATERIAL(matIndex), (u32)mat);
}

extern u16 gPlPartRootBone[];

/*
 * Draws the local player's hunter (Player_Draw draws the others), unless it is aiming in the
 * first person (CamCtl_IsAiming):
 * - the skeleton's own model (the legs), at alpha 0.2 while nearCamera, else faded toward the
 *   alpha CamCtl_GetPlayerFade asks for in the field (a tenth of the way each frame, kept in
 *   fadeAlpha);
 * - the weapon (Player_DrawWeapon);
 * - the face and the four other armour pieces (partModels), each placed by its root bone
 *   (gPlPartRootBone) and lit per material (Player_SetLight);
 * - the held prop (Player_DrawHeldProp);
 * - the aiming markers: Hud_DrawObjMarker on the mounted gun with L1 held, Player_DrawBoneMarker
 *   for a loaded bowgun aimed with L1.
 * The body's textures take 2 slots per player from 13, the parts' 12 per player from 43.
 */
void Player_DrawLocal(Player *pl, s32 unused /* a1: DrawCb_LocalPlayer passes 0 */)
{
    s32 partSetIndex;
    ModelMatSet *partSet;
    ModelMat *partMats;
    s32 i;
    s32 *partMatWords;
    u16 *rootBone;
    s32 slot;
    s32 j;
    s32 partMatIndex;
    ModelMat *bodyMats;
    s32 setCount;
    f32 alpha;
    f32 world[16];
    s32 variant;
    EmBone *bones;
    Model *partModel;
    Model *body;
    ModelMatSet *bodySet;
    f32 targetAlpha;
    f32 identity[16];

    body = pl->model;
    if (Player_IsLocal(pl) == 1 && CamCtl_IsAiming(pl) != 0) {
        return;
    }
    LightSet_SetColors(pl, 1);
    LightSet_FadeColors(pl);
    Gs_SetState(RS_COLOR2, -1);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    if (pl->nearCamera != 0) {
        Gs_SetState(RS_ALPHA_REF, 0);
        alpha = 0.2f;
    } else {
        Gs_SetState(RS_ALPHA_REF, 192);
        alpha = pl->alpha;
    }
    if (Player_IsLocal(pl) == 1 && Scene_InField() == 1) {
        if (CamCtl_GetPlayerFade(&targetAlpha) == 1) {
            alpha = pl->fadeAlpha + (targetAlpha - pl->fadeAlpha) / 10.0f;
            if (Math_Absf(alpha - targetAlpha) <= 0.1f) {
                alpha = targetAlpha;
            }
            Gs_SetState(RS_ALPHA_REF, alpha < 1.0f ? 0 : 192);
            pl->fadeAlpha = alpha;
        } else {
            pl->fadeAlpha = 1.0f;
            Gs_SetState(RS_ALPHA_REF, 192);
            alpha = pl->alpha;
        }
    }
    Tex_Prepare(2, pl->index * 2 + 13);
    bodySet = body->matSets;
    Mtx_Copy(world, pl->worldMtx[0]);
    Bone_StoreMatrices(body->bones);
    bodyMats = pl->model->mats;
    for (i = 0; i < body->matSetCount; i++, bodySet++) {
        if (pl->meshShown[i] != 1) {
            continue;
        }
        if (bodySet->id != -1) {
            for (j = 0; j < bodySet->count; j++) {
                Player_SetLight(pl, EQUIP_LEGS, i, j, &bodyMats[bodySet->mat[j]], alpha);
            }
        }
        Draw_ApplyStates(bodySet->flags);
        Model_Draw(bodySet->id, 0);
    }
    Draw_SetDefaultState();
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    if (pl->weaponShown != 0) {
        Player_DrawWeapon(pl, 0, alpha);
    }
    Tex_Prepare(12, pl->index * 12 + 43);
    for (slot = EQUIP_SLOT_FACE, rootBone = &gPlPartRootBone[EQUIP_SLOT_FACE]; slot < EQUIP_SLOT_WEAPON; slot++, rootBone++) {
        variant = pl->armourKind[slot];
        partModel = pl->partModels[slot];
        if (partModel == NULL) {
            continue;
        }
        partSet = partModel->matSets;
        setCount = partModel->matSetCount;
        Bone_UpdateWorldScaledRoot(partModel->bones, pl->partRecs[*rootBone]->m1);
        if (slot == EQUIP_BODY || slot == EQUIP_WAIST) {
            Mtx_Identity(identity);
            Bone_SetMatricesRel(partModel->bones, (struct BoneRoot *) pl, slot, variant - 1, identity);
        } else if (slot == EQUIP_HEAD) {
            bones = partModel->bones;
            if (bones->count > 4) {
                Bone_UpdateWorldScaledRoot(&bones[4], pl->partRecs[20]->m1);
            }
            Mtx_Identity(identity);
            Bone_SetMatricesRel(partModel->bones, (struct BoneRoot *) pl, slot, variant - 1, identity);
        } else {
            Bone_SetMatrices(partModel->bones, (struct BoneRoot *) pl, slot, variant - 1);
        }
        partMats = pl->partModels[slot]->mats;
        for (partSetIndex = 0; partSetIndex < setCount; partSetIndex++, partSet++) {
            if (partSet->id != -1) {
                if (0 < partSet->count) { /* if + do-while: j2 = 0 in the test slot, the walker after it */
                    partMatIndex = 0;
                    partMatWords = (s32 *)partSet; /* the set as words: e2[2] is mat[j2] (a walker from parts->mat is computed first) */
                    do {
                        Player_SetLight(pl, slot, partSetIndex, partMatIndex, &partMats[partMatWords[2]], alpha);
                        partMatIndex++;
                        partMatWords++;
                    } while (partMatIndex < partSet->count);
                }
            }
            Draw_ApplyStates(partSet->flags);
            Model_Draw(partSet->id, 0);
        }
        Draw_SetDefaultState();
    }
    Player_DrawHeldProp(pl);
    if (pl->nearCamera != 0) {
        Gs_SetState(RS_ZWRITE, 1);
    }
    Gs_SetState(RS_ALPHA_REF, 0);
    LightSet_ResetColors(1);
    LightSet_Apply(1);
    if (Player_IsLocal(pl) != 0 && pl->aimMode == 0) {
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_AIM) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_REAIM) != 0) {
            if (pl->input.held & PAD_L1) {
                Hud_DrawObjMarker(pl);
            }
        } else if (pl->weaponOut != 0 && pl->stateGroup != PL_GROUP_REACT && pl->stateGroup != PL_GROUP_GESTURE) {
            switch (pl->weaponType) {
            case WEAPON_HEAVY_BOWGUN:
            case WEAPON_LIGHT_BOWGUN:
                if ((pl->input.held & PAD_L1) && pl->aimLockTimer == 0 && pl->roundsLoaded != 0) {
                    Player_DrawBoneMarker(pl, 0);
                }
                break;
            case WEAPON_BOW:
                break;
            }
        }
    }
}

/*
 * Draws the prop the player holds in some motions: a material set of gCommonModels[0] (lit by its
 * own materials) or of gAreaSubModels[0], placed at a bone with an offset and a turn.
 * - Crouched in AREA_TAVERN to AREA_ELDER_HALL or in AREA_HUB_0x89 (not in motions PL_MOT_TOWN_SIT and
 *   662, nor in the town's action state TOWN_PC_POOGIE_DIG): common set 40 at bone 18.
 * - Motion 250 in AREA_HUB_0xAC, frames 152 to 406: area set 5.
 * - PL_MOT_PICKAXE_SWING: the common set of Effect_MapTexId(70) at bone 14.
 * - Motion 647 in AREA_HUB_0x57, frames 232 to 578: area set 0.
 * - Motions PL_MOT_FISH_READY to PL_MOT_FISH_REEL_EMPTY: the common set Player_HeldPartForMotion picks.
 * - Motion 841: common set 34 at bone 14, posed as the drawn heavy bowgun (gWpPoseDrawn),
 *   10 lower.
 * The misses of motions 250 and 647 jump to the end instead of returning: only that way does
 * the compiler lay the function out as the game's.
 */
void Player_DrawHeldItem(void *obj)
{
    Player *pl = obj;
    f32 boneMtx[16];
    f32 mtx[16];
    f32 pos[3];
    f32 off[3];
    Model *model;
    ModelMatSet *part;
    u8 fromCommon;
    s16 setIndex;
    s16 bone;
    f32 rotX;
    f32 rotY;
    f32 rotZ;
    f32 *pose;

    fromCommon = 0;
    if (pl->action == PL_POSTURE_CROUCHED && pl->motionId[0] != 608 && pl->motionId[0] != 662
        && Player_IsState(pl, PL_GROUP_ACTION, 128) == 0
        && ((u32)(gScene.area - AREA_TAVERN) <= AREA_ELDER_HALL - AREA_TAVERN || gScene.area == AREA_HUB_0x89)) {
        fromCommon = 1;
        setIndex = 40;
        bone = PL_BONE_OTHER_HAND;
        rotX = 1.5707964f;
        off[0] = -23.5f;
        off[1] = 0.0f;
        off[2] = -13.5f;
        rotY = 0.0f;
        rotZ = 0.0f;
    } else {
        switch (pl->motionId[0]) {
        case 250:
            if (gScene.area == AREA_HUB_0xAC && Motion_InFrameRange(pl, 0, 152.0f, 406.0f) != 0) {
                setIndex = 5;
                bone = PL_BONE_OTHER_HAND;
                rotX = 0.0f;
                off[0] = -8.5f;
                rotY = 0.0f;
                rotZ = 0.0f;
                off[1] = -0.4f;
                off[2] = 1.2f;
            } else {
                goto end;
            }
            break;
        case 415:
            fromCommon = 1;
            setIndex = Effect_MapTexId(70);
            bone = PL_BONE_HAND;
            rotX = 0.36651915f;
            rotY = 0.17453294f;
            rotZ = 1.3613569f;
            off[0] = 17.0f;
            off[1] = 0.9f;
            off[2] = 22.0f;
            break;
        case 647:
            if (gScene.area == AREA_HUB_0x57 && Motion_InFrameRange(pl, 0, 232.0f, 578.0f) != 0) {
                setIndex = 0;
                bone = PL_BONE_OTHER_HAND;
                rotX = 0.0f;
                off[0] = 2.08f;
                off[1] = 3.02f;
                rotY = 0.0f;
                off[2] = -2.0f;
                rotZ = 0.0f;
            } else {
                goto end;
            }
            break;
        case 804:
        case 805:
        case 806:
        case 807:
        case 808:
        case 809:
            fromCommon = 1;
            setIndex = Player_HeldPartForMotion(pl);
            bone = PL_BONE_OTHER_HAND;
            rotX = 0.0f;
            rotY = 0.0f;
            rotZ = -1.5707964f;
            off[0] = -9.2f;
            off[1] = -1.7f;
            off[2] = 12.0f;
            break;
        case 841:
            bone = PL_BONE_HAND;
            fromCommon = 1;
            setIndex = 34;
            pose = (f32 *)&gWpPoseDrawn[WEAPON_HEAVY_BOWGUN];
            off[0] = *pose++;
            off[1] = *pose++;
            off[1] -= 10.0f;
            off[2] = *pose++;
            rotX = *pose++;
            rotY = *pose++;
            rotZ = *pose++;
            break;
        default:
            return;
        }
    }
    if (fromCommon) {
        model = gCommonModels[0];
    } else {
        model = gAreaSubModels[0];
    }
    if (model != NULL && model->active != 0) {
        part = &model->matSets[setIndex];
        Mtx_Copy(boneMtx, (void *)Em_GetBone(pl, bone));
        Mtx_GetTrans(pos, boneMtx);
        Vec_RotateSelf(off, boneMtx);
        pos[0] += off[0];
        pos[1] += off[1];
        pos[2] += off[2];
        Mtx_Identity(mtx);
        Mtx_RotateXYZ(mtx, rotX, rotY, rotZ);
        Mtx_Concat3x3(mtx, boneMtx);
        Mtx_SetTrans(mtx, pos[0], pos[1], pos[2]);
        if (part != NULL && part->id != -1) {
            Gs_SetState(RS_MTX_SLOT(0), (u32)mtx);
            if (fromCommon) {
                Gs_SetLightList(model->mats, part);
            }
            Draw_ApplyStates(part->flags);
            Model_Draw(part->id, 0);
        }
        Draw_SetDefaultState();
    }
end:
    ;
}

/* Draws another player's hunter as Player_DrawLocal draws the local one, without the camera's fade
 * and the markers: the skeleton's own model (alpha 0.2 while nearCamera, unless debug switch 123
 * is on), the weapon (Player_DrawWeapon, not in motion 841, where Player_DrawHeldItem draws it),
 * the face and the four other armour pieces, then the held item (Player_DrawHeldItem). */
void Player_Draw(Player *pl, s32 unused /* a1: DrawCb_Player passes 0 */)
{
    s32 partSetIndex;
    ModelMatSet *partSet;
    s32 i;
    Model *body;
    ModelMatSet *bodySet;
    s32 setCount;
    f32 world[16];
    s32 variant;
    f32 identity[16];
    s32 *partMatWords;
    s32 slot;
    s32 j;
    ModelMat *partMats;
    EmBone *bones;
    Model *partModel;
    ModelMat *material;
    ModelMat *bodyMats;
    f32 alpha;
    s32 partMatIndex;
    u16 *rootBone;

    body = pl->model;
    LightSet_SetColors(pl, 1);
    LightSet_FadeColors(pl);
    Gs_SetState(RS_COLOR2, -1);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    if (pl->nearCamera != 0 && Debug_IsOn(123) == 0) {
        Gs_SetState(RS_ALPHA_REF, 0);
        alpha = 0.2f;
    } else {
        Gs_SetState(RS_ALPHA_REF, 192);
        alpha = pl->alpha;
    }
    Tex_Prepare(2, pl->index * 2 + 13);
    bodySet = body->matSets;
    Mtx_Copy(world, pl->worldMtx[0]);
    Bone_StoreMatrices(body->bones);
    bodyMats = pl->model->mats;
    for (i = 0; i < body->matSetCount; i++, bodySet++) {
        if (pl->meshShown[i] != 1) {
            continue;
        }
        if (bodySet->id != -1) {
            for (j = 0; j < bodySet->count; j++) {
                material = &bodyMats[bodySet->mat[j]];
                material->alpha = alpha;
                Player_SetLight(pl, EQUIP_LEGS, i, j, material, alpha);
            }
        }
        Draw_ApplyStates(bodySet->flags);
        Model_Draw(bodySet->id, 0);
    }
    Draw_SetDefaultState();
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    if (pl->weaponShown != 0 && pl->motionId[0] != 841) {
        Player_DrawWeapon(pl, 1, alpha);
    }
    Tex_Prepare(12, pl->index * 12 + 43);
    for (slot = EQUIP_SLOT_FACE, rootBone = &gPlPartRootBone[EQUIP_SLOT_FACE]; slot < EQUIP_SLOT_WEAPON; slot++, rootBone++) {
        variant = pl->armourKind[slot];
        partModel = pl->partModels[slot];
        if (partModel == NULL) {
            continue;
        }
        partSet = partModel->matSets;
        setCount = partModel->matSetCount;
        Bone_UpdateWorldScaledRoot(partModel->bones, pl->partRecs[*rootBone]->m1);
        if (slot == EQUIP_BODY || slot == EQUIP_WAIST) {
            Mtx_Identity(identity);
            Bone_SetMatricesRel(partModel->bones, (struct BoneRoot *) pl, slot, variant - 1, identity);
        } else if (slot == EQUIP_HEAD) {
            bones = partModel->bones;
            if (bones->count > 4) {
                Bone_UpdateWorldScaledRoot(&bones[4], pl->partRecs[20]->m1);
            }
            Mtx_Identity(identity);
            Bone_SetMatricesRel(partModel->bones, (struct BoneRoot *) pl, slot, variant - 1, identity);
        } else {
            Bone_SetMatrices(partModel->bones, (struct BoneRoot *) pl, slot, variant - 1);
        }
        partMats = pl->partModels[slot]->mats;
        for (partSetIndex = 0; partSetIndex < setCount; partSetIndex++, partSet++) {
            if (partSet->id != -1) {
                if (0 < partSet->count) {
                    partMatIndex = 0;
                    partMatWords = (s32 *)partSet; /* the set as words: e[2] is mat[j2] (a walker from parts->mat is computed first) */
                    do {
                        Player_SetLight(pl, slot, partSetIndex, partMatIndex, &partMats[partMatWords[2]], alpha);
                        partMatIndex++;
                        partMatWords++;
                    } while (partMatIndex < partSet->count);
                }
            }
            Draw_ApplyStates(partSet->flags);
            Model_Draw(partSet->id, 0);
        }
        Draw_SetDefaultState();
    }
    Player_DrawHeldItem(pl);
    if (pl->nearCamera != 0) {
        Gs_SetState(RS_ZWRITE, 1);
    }
    Gs_SetState(RS_ALPHA_REF, 0);
    LightSet_ResetColors(1);
    LightSet_Apply(1);
}

extern Model *gAreaSubModel;
extern u8 gFaceBodyTypeMale[];

/* The character maker's parts model (gAreaSubModel) holds one material set per part: the bodies
 * (four per type, by gFaceBodyTypeMale's variant), then the faces, then the hair styles. */
#define CHARMAKE_SET_FACES 16   /* the first face's set */
#define CHARMAKE_SET_HAIRS 43   /* the first hair style's set */

/* Draws the hunter being made (gChrEdit) on the player's skeleton: the body of its type (the
 * variant gFaceBodyTypeMale gives for its face), its face and its hair, the hair's first material
 * in the chosen colour. */
void CharMake_DrawHunter(Player *pl, s32 unused /* a1: DrawCb_CharMake passes 0 */)
{
    u8 bodySet;
    f32 world[16];
    Model *skeleton;
    ChrEdit *edit;
    u8 typeBase;
    ModelMat *mat;
    s32 i;
    Model *parts;
    ModelMatSet *set;
    ModelMat *mats;

    skeleton = pl->model;
    edit = &gChrEdit;
    LightSet_SetColors(pl, 1);
    LightSet_FadeColors(pl);
    Gs_SetState(RS_COLOR2, -1);
    Gs_SetState(RS_ALPHA_REF, 192);
    Mtx_Copy(world, pl->worldMtx[0]);
    Bone_StoreMatrices(skeleton->bones);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    bodySet = 3 - gFaceBodyTypeMale[edit->face];
    typeBase = edit->type * 4;
    bodySet = bodySet + typeBase;
    parts = gAreaSubModel;
    set = parts->matSets + bodySet;
    mats = parts->mats;
    if (set->id != -1) {
        for (i = 0; i < set->count; i++) {
            Gs_SetState((u8)RS_MATERIAL(i), (u32)&mats[set->mat[i]]);
        }
    }
    Draw_ApplyStates(set->flags);
    Model_Draw(set->id, 0);
    set = parts->matSets + edit->face + CHARMAKE_SET_FACES;
    if (set->id != -1) {
        for (i = 0; i < set->count; i++) {
            Gs_SetState((u8)RS_MATERIAL(i), (u32)&mats[set->mat[i]]);
        }
    }
    Draw_ApplyStates(set->flags);
    Model_Draw(set->id, 0);
    set = parts->matSets + edit->hair + CHARMAKE_SET_HAIRS;
    if (set->id != -1) {
        for (i = 0; i < set->count; i++) {
            mat = &mats[set->mat[i]];
            if (i == 0) {
                mat->rgb[0] = ((edit->color >> 16) & 0xFF) / 255.0f;
                mat->rgb[1] = ((edit->color >> 8) & 0xFF) / 255.0f;
                mat->rgb[2] = (u8)edit->color / 255.0f;
                Gs_SetState((u8)RS_MATERIAL(i), (u32)mat);
            }
            Gs_SetState((u8)RS_MATERIAL(i), (u32)mat);
        }
    }
    Draw_ApplyStates(set->flags);
    Model_Draw(set->id, 0);
    Draw_SetDefaultState();
    LightSet_ResetColors(1);
    LightSet_Apply(1);
}


/* The file's data: the weapon poses and keyframe tracks, the bone matrix slot maps, the bone
 * markers' directions, the cycling colours and the part bones. */
WpPose gWpPoseDrawnAlt[11] = {
#include "sub_main/0043CA30.inc"
};
WpPose gWpPoseDrawn[11] = {
#include "sub_main/0043CB40.inc"
};
WpPose gWpPoseSheathed[11] = {
#include "sub_main/0043CC50.inc"
};
WpPose gWpPoseSharpen[11] = {
#include "sub_main/0043CD60.inc"
};
Key1 gLbgKey579ScaleZ[8] = {
#include "sub_main/0043CE70.inc"
};
Key1 gHbgKey579ScaleZ[8] = {
#include "sub_main/0043CEB0.inc"
};
Key1 gLbgKey579ScaleY[4] = {
#include "sub_main/0043CEF0.inc"
};
Key1 gHbgKey579ScaleX[4] = {
#include "sub_main/0043CF10.inc"
};
Key1 gLbgKey57CScaleZ[8] = {
#include "sub_main/0043CF30.inc"
};
Key1 gHbgKey57CScaleZ[8] = {
#include "sub_main/0043CF70.inc"
};
Key1 gLbgKey57CScaleY[4] = {
#include "sub_main/0043CFB0.inc"
};
Key1 gHbgKey57CScaleX[4] = {
#include "sub_main/0043CFD0.inc"
};
Key1 gLbgKey57DEmptyScaleZ[8] = {
#include "sub_main/0043CFF0.inc"
};
Key1 gHbgKey57DEmptyScaleZ[8] = {
#include "sub_main/0043D030.inc"
};
Key1 gLbgKey57DEmptyScaleY[4] = {
#include "sub_main/0043D070.inc"
};
Key1 gHbgKey57DEmptyScaleX[4] = {
#include "sub_main/0043D090.inc"
};
Key1 gLbgKey57DScaleZ[8] = {
#include "sub_main/0043D0B0.inc"
};
Key1 gHbgKey57DScaleZ[8] = {
#include "sub_main/0043D0F0.inc"
};
Key1 gLbgKey57DScaleY[6] = {
#include "sub_main/0043D130.inc"
};
Key1 gHbgKey57DScaleX[6] = {
#include "sub_main/0043D160.inc"
};
Key1 gBowgunKey580ScaleZ[8] = {
#include "sub_main/0043D190.inc"
};
Key1 gBowgunKey580Scale2[4] = {
#include "sub_main/0043D1D0.inc"
};
Key1 gHbgKey3EARotX[6] = {
#include "sub_main/0043D1F0.inc"
};
Key1 gHbgKey3EAScaleZ[8] = {
#include "sub_main/0043D220.inc"
};
Key1 gHbgKey3EAScaleX[4] = {
#include "sub_main/0043D260.inc"
};
Key1 gHbgKey3EBRotX[4] = {
#include "sub_main/0043D280.inc"
};
Key1 gHbgKey3F1ScaleZ[6] = {
#include "sub_main/0043D2A0.inc"
};
Key1 gHbgKey3EBScaleX[6] = {
#include "sub_main/0043D2D0.inc"
};
Key3 gLbgKey3EARot[9] = {
#include "sub_main/0043D300.inc"
};
Key3 gLbgKey3EAOff[9] = {
#include "sub_main/0043D390.inc"
};
Key3 gLbgKey3EBRot[12] = {
#include "sub_main/0043D420.inc"
};
Key3 gLbgKey3EBOff[12] = {
#include "sub_main/0043D4E0.inc"
};
Key3 gLbgKey3F1Rot[10] = {
#include "sub_main/0043D5A0.inc"
};
Key3 gLbgKey3F1Off[10] = {
#include "sub_main/0043D640.inc"
};
Key3 gLbgKey3F2Rot[10] = {
#include "sub_main/0043D6E0.inc"
};
Key3 gLbgKey3F2Off[10] = {
#include "sub_main/0043D780.inc"
};
Key1 gLanceKey19DScaleZ[6] = {
#include "sub_main/0043D820.inc"
};
Key1 gLanceKey3EAScaleZ[6] = {
#include "sub_main/0043D850.inc"
};
Key1 gLanceKey3EBScaleZ[6] = {
#include "sub_main/0043D880.inc"
};
Key1 gLanceKey3F1ScaleZ[6] = {
#include "sub_main/0043D8B0.inc"
};
Key1 gLanceKey3F2ScaleZ[6] = {
#include "sub_main/0043D8E0.inc"
};
Key1 gLanceKey3FAScaleZ[6] = {
#include "sub_main/0043D910.inc"
};
Key1 gHammerKey57EScale[6] = {
#include "sub_main/0043D940.inc"
};
Key1 gHammerKey57CScale[6] = {
#include "sub_main/0043D970.inc"
};
Key1 gGsKey19DScaleZ[6] = {
#include "sub_main/0043D9A0.inc"
};
Key1 gGsKey3EAScaleZ[8] = {
#include "sub_main/0043D9D0.inc"
};
Key1 gGsKey3EBScaleZ[8] = {
#include "sub_main/0043DA10.inc"
};
Key1 gGsKey3F9ScaleZ[8] = {
#include "sub_main/0043DA50.inc"
};
Key1 gGsKey582ScaleZ[6] = {
#include "sub_main/0043DA90.inc"
};
Key2 gGsKey19DScaleXZ[10] = {
#include "sub_main/0043DAC0.inc"
};
Key2 gGsKey3EAScaleXZ[6] = {
#include "sub_main/0043DB40.inc"
};
Key2 gGsKey3EBScaleXZ[6] = {
#include "sub_main/0043DB90.inc"
};
Key2 gGsKey3F9ScaleXZ[6] = {
#include "sub_main/0043DBE0.inc"
};
Key2 gGsKey582ScaleXZ[6] = {
#include "sub_main/0043DC30.inc"
};
Key2 gHornKey57DRot[18] = {
#include "sub_main/0043DC80.inc"
};
Key3 gHornKey57DScale[24] = {
#include "sub_main/0043DD60.inc"
};
Key1 gGlKey19DRotX[10] = {
#include "sub_main/0043DEE0.inc"
};
Key1 gGlKey3EARotX[6] = {
#include "sub_main/0043DF30.inc"
};
Key1 gGlKey3EBRotX[8] = {
#include "sub_main/0043DF60.inc"
};
Key1 gGlKey3F2RotX[4] = {
#include "sub_main/0043DFA0.inc"
};
Key1 gGlKey57CRotX[16] = {
#include "sub_main/0043DFC0.inc"
};
Key3 gGlKey580Rot[6] = {
#include "sub_main/0043E040.inc"
};
Key3 gGlKeyTimerRot[7] = {
#include "sub_main/0043E0A0.inc"
};
Key1 gBowKey3EARotX[8] = {
#include "sub_main/0043E110.inc"
};
Key1 gBowKey3EBRotX[12] = {
#include "sub_main/0043E150.inc"
};
Key1 gBowKey3EDRotX[12] = {
#include "sub_main/0043E1B0.inc"
};
Key1 gBowKey3F4RotX[8] = {
#include "sub_main/0043E210.inc"
};
Key1 gBowKey3F6RotX[8] = {
#include "sub_main/0043E250.inc"
};
Key1 gBowKey57DRotX[6] = {
#include "sub_main/0043E290.inc"
};
Key1 gBowKey580RotX[8] = {
#include "sub_main/0043E2C0.inc"
};
Key1 gBowKey587RotX[8] = {
#include "sub_main/0043E300.inc"
};
Key1 gBowKey58ARotX[4] = {
#include "sub_main/0043E340.inc"
};
Key1 gBowKey3EAScaleZ[6] = {
#include "sub_main/0043E360.inc"
};
Key1 gBowKey3EBScaleZ[6] = {
#include "sub_main/0043E390.inc"
};
Key1 gBowKey3EDScaleZ[6] = {
#include "sub_main/0043E3C0.inc"
};
Key1 gBowKey3F4ScaleZ[6] = {
#include "sub_main/0043E3F0.inc"
};
Key1 gBowKey3F6ScaleZ[6] = {
#include "sub_main/0043E420.inc"
};
Key1 gBowKey57DScaleZ[8] = {
#include "sub_main/0043E450.inc"
};
Key1 gBowKey580ScaleZ[6] = {
#include "sub_main/0043E490.inc"
};
Key1 gBowKey587ScaleZ[6] = {
#include "sub_main/0043E4C0.inc"
};
Key1 gBowKey58AScaleZ[5] = {
#include "sub_main/0043E4F0.inc"
};
s16 gBoneMtxSlotMaps_0[4] = {
#include "sub_main/0043E518.inc"
};
s16 gBoneMtxSlotMaps_1[8] = {
#include "sub_main/0043E520.inc"
};
s16 gBoneMtxSlotMaps_2[16] = {
#include "sub_main/0043E530.inc"
};
s16 gBoneMtxSlotMaps_3[16] = {
#include "sub_main/0043E550.inc"
};
s16 gBoneMtxSlotMaps_4[16] = {
#include "sub_main/0043E570.inc"
};
s16 gBoneMtxSlotMaps_5[16] = {
#include "sub_main/0043E590.inc"
};
s16 *gBoneMtxSlotMaps[8] = {
    gBoneMtxSlotMaps_0, gBoneMtxSlotMaps_1, gBoneMtxSlotMaps_2, gBoneMtxSlotMaps_3,
    gBoneMtxSlotMaps_4, gBoneMtxSlotMaps_5, NULL, NULL,
};
f32 gBoneMarkerDirs[6][3] = {
#include "sub_main/0043E5D0.inc"
};
u32 gCycleColors[8] = {
#include "sub_main/0043E620.inc"
};
/* The bone each part slot (EQUIP_SLOT_FACE to EQUIP_WAIST) hangs from (Player_DrawLocal). */
u16 gPlPartRootBone[8] = {
#include "sub_main/0043E640.inc"
};
