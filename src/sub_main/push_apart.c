/*
 * The push that keeps bodies from overlapping, once a frame
 * (Field_PushApart): a player against another player (their body capsules,
 * Player_BodyCapsule) or a monster (its kind's hit shapes, placed by its
 * bones), and two monsters (their kinds' push spheres). The body with the
 * lower pushPriority moves (both halfway when they are equal) and is marked
 * pushed. The monsters are read through the Player struct: the fields used
 * sit at the same offsets in Em.
 */
#include "common.h"
#include "bgm.h"
#include "place_rules.h"
#include "player.h"
#include "em.h"
#include "objhead.h"
#include "capsule.h"
#include "push_apart.h"
#include "engine_math.h"
#include "equip_shop.h"
#include "hit_setup.h"
#include "area.h"
extern Em gEms[EM_COUNT];
void Player_BodyCapsule(Player *p, HitCapsule *cap, f32 r);
extern HitShape *gEmBoneHits[];
/* end of prototypes */

/*
 * Pushes a player and a monster apart. The player is its body capsule
 * (Player_BodyCapsule, radius 40); the monster is its kind's hit shapes
 * (gEmBoneHits, or gEmBoneHitsLobby in the lobby), placed by its bones, less the
 * shapes flagged as not pushing. Up to two overlaps are summed, at the
 * longest one's length; the height counts only while the player is
 * airborne (PL_POSTURE_AIRBORNE), and never upward while it rises in state groups 2
 * and 3. The body with the lower pushPriority moves (both halfway when
 * they are equal) and is marked pushed. The player then stays between its
 * floor and 480 above it (unless it started higher).
 * The monster is read through the Player struct: the fields used (its kind
 * as weaponType, pos, pushPriority, pushed) sit at the same offsets in Em.
 */
void PlayerEm_PushApart(Player *pl, Player *em)
{
    HitShape *shape;
    f32 pushLen[2];
    Capsule bodyCap;
    Capsule shapeCap;
    HitCapsule bodyHit;
    HitCapsule shapeHit;
    HitSphere shapeSphere;
    f32 push[2][3];
    f32 startY;
    s16 count;
    s16 i;
    s32 boneNo;
    s16 shapeKind;
    s32 hit;

    startY = pl->pos[1];
    count = 0;
    Player_BodyCapsule(pl, &bodyHit, 40.0f);
    Capsule_Copy(&bodyHit, &bodyCap);
    if (gScene.inLobby == 0) {
        shape = gEmBoneHits[em->weaponType];
    } else {
        shape = gEmBoneHitsLobby[em->weaponType];
    }
    while ((boneNo = shape->bone) != -1) {
        if ((shape->atkFlags & HITSHAPE_TRAP_ONLY) || (shape->atkFlags2 & HITSHAPE2_HOLD_ONLY) || (shape->openFlags & HITSHAPE_HERMIT_ONLY) || (shape->openFlags & HITSHAPE_WIND_ONLY)
            || (shape->offFlags & HITSHAPE_NO_PUSH) || (shape->offFlags & HITSHAPE_NO_HIT)) {
            shape++;
            continue;
        }
        switch (boneNo) {
        case 127: /* a shape at the monster's position */
        case 126:
            shapeKind = HitShape_Place(em->pos, shape, &shapeHit, &shapeSphere, 1.0f);
            break;
        case 125: /* a group's header: on to the group's shapes */
            HitList_Next((Em *)em, &shape);
            continue;
        default:
            shapeKind = BoneHit_Place((ObjHead *)em, shape, &shapeHit, &shapeSphere);
            break;
        }
        if (shapeKind == 0) {
            hit = (u8)Capsule_PushSphere(&bodyCap, shapeSphere.pos, push[count], shapeSphere.radius);
        } else {
            Capsule_Copy(&shapeHit, &shapeCap);
            hit = (u8)Capsule_PushCapsule(&shapeCap, &bodyCap, push[count]);
        }
        if ((u8)hit) {
            pushLen[count] = Vec_Length(push[count]);
            if (pl->action != PL_POSTURE_AIRBORNE) {
                push[count][1] = 0.0f;
            }
            count++;
        }
        if (count >= 2) {
            break;
        }
        shape++;
    }
    if (count != 0) {
        for (i = 1; i < count; i++) {
            push[0][0] += push[i][0];
            push[0][1] += push[i][1];
            push[0][2] += push[i][2];
            if (pushLen[0] < pushLen[i]) {
                pushLen[0] = pushLen[i];
            }
        }
        Vec_Normalize(push[0]);
        push[0][0] *= pushLen[0];
        push[0][1] *= pushLen[0];
        push[0][2] *= pushLen[0];
        if ((pl->stateGroup == 2 || pl->stateGroup == 3) && pl->vel[1] > 0.0f && push[0][1] > 0.0f) {
            push[0][1] = 0.0f;
        }
        if (em->pushPriority < pl->pushPriority) {
            em->pos[0] -= push[0][0];
            em->pos[1] -= push[0][1];
            em->pos[2] -= push[0][2];
            em->pushed = 1;
        } else if (em->pushPriority == pl->pushPriority) {
            pl->pos[0] += 0.5f * push[0][0];
            pl->pos[1] += 0.5f * push[0][1];
            pl->pos[2] += 0.5f * push[0][2];
            /* A height check with nothing in it: the game's code still
             * evaluates it, and the compiler orders the stores around it. */
            if (pl->pos[1] < pl->floorY || pl->pos[1] > pl->floorY + 480.0f) {
            }
            em->pos[0] += -0.5f * push[0][0];
            em->pos[1] += -0.5f * push[0][1];
            em->pos[2] += -0.5f * push[0][2];
            pl->pushed = 1;
            em->pushed = 1;
        } else {
            pl->pos[0] += push[0][0];
            pl->pos[1] += push[0][1];
            pl->pos[2] += push[0][2];
            pl->pushed = 1;
        }
        if (pl->pos[1] < pl->floorY) {
            pl->pos[1] = pl->floorY;
        } else if (startY < pl->floorY + 480.0f && pl->pos[1] > pl->floorY + 480.0f) {
            pl->pos[1] = pl->floorY + 480.0f;
        }
    }
}

/*
 * Pushes two monsters apart when their push spheres touch: each kind's list
 * in gEmPushSpheres (or gEmPushSpheresLobby in the lobby) of {x, y, z, radius}
 * records, ended by radius -1, offset from the monster's position. Both
 * lists are scaled by the first monster's scale, the radii each by its
 * own. Up to two overlaps are summed, at the longest one's length; the
 * height counts only for a monster in the air (stance EM_STANCE_AIR). Both move halfway
 * when their pushPriority is equal, else the one with the lower; the moved
 * ones are marked pushed. Both are read through the Player struct, whose
 * fields used here sit at the same offsets in Em.
 */
void EmEm_PushApart(Player *emA, Player *emB)
{
    f32 centerA[3];
    f32 centerB[3];
    f32 pushLen[2];
    f32 push[2][3];
    f32 *spheresA;
    f32 *spheresB;
    f32 radiusA;
    s16 count;
    s16 i;

    count = 0;
    if (gScene.inLobby == 0) {
        spheresA = gEmPushSpheres[emA->weaponType];
        spheresB = gEmPushSpheres[emB->weaponType];
    } else {
        spheresA = gEmPushSpheresLobby[emA->weaponType];
        spheresB = gEmPushSpheresLobby[emB->weaponType];
    }
    for (; spheresA[3] != -1.0f; spheresA += 4) {
        centerA[0] = emA->pos[0] + spheresA[0] * emA->scale[0];
        centerA[1] = emA->pos[1] + spheresA[1] * emA->scale[1];
        centerA[2] = emA->pos[2] + spheresA[2] * emA->scale[2];
        radiusA = spheresA[3] * emA->scale[0];
        for (; spheresB[3] != -1.0f; spheresB += 4) {
            centerB[0] = emB->pos[0] + spheresB[0] * emA->scale[0];
            centerB[1] = emB->pos[1] + spheresB[1] * emA->scale[1];
            centerB[2] = emB->pos[2] + spheresB[2] * emA->scale[2];
            if (Sphere_PushOut(radiusA, spheresB[3] * emB->scale[0], centerA, centerB, push[count]) != 0) {
                pushLen[count] = Vec_Length(push[count]);
                count++;
            }
            if (count >= 2) {
                goto done; /* leaves both loops */
            }
        }
    }
done:
    if (count != 0) {
        for (i = 1; i < count; i++) {
            push[0][0] += push[i][0];
            push[0][1] += push[i][1];
            push[0][2] += push[i][2];
            if (pushLen[0] < pushLen[i]) {
                pushLen[0] = pushLen[i];
            }
        }
        if (emA->action != EM_STANCE_AIR) {
            push[0][1] = 0.0f;
        }
        Vec_Normalize(push[0]);
        push[0][0] *= pushLen[0];
        push[0][1] *= pushLen[0];
        push[0][2] *= pushLen[0];
        if (emA->pushPriority == emB->pushPriority) {
            emA->pos[0] += 0.5f * push[0][0];
            if (emA->action == EM_STANCE_AIR) {
                emA->pos[1] += 0.5f * push[0][1];
            }
            emA->pos[2] += 0.5f * push[0][2];
            emB->pos[0] += -0.5f * push[0][0];
            if (emB->action == EM_STANCE_AIR) {
                emB->pos[1] += -0.5f * push[0][1];
            }
            emB->pos[2] += -0.5f * push[0][2];
            emA->pushed = 1;
            emB->pushed = 1;
        } else if (emA->pushPriority < emB->pushPriority) {
            emA->pos[0] += push[0][0];
            if (emA->action == EM_STANCE_AIR) {
                emA->pos[1] += push[0][1];
            }
            emA->pos[2] += push[0][2];
            emA->pushed = 1;
        } else {
            emB->pos[0] -= push[0][0];
            if (emB->action == EM_STANCE_AIR) {
                emB->pos[1] -= push[0][1];
            }
            emB->pos[2] -= push[0][2];
            emB->pushed = 1;
        }
    }
}

/* Pushes two players apart when their body capsules (Player_BodyCapsule,
 * radius 40) meet: half the push each across the ground; the height half
 * each when both are airborne (PL_POSTURE_AIRBORNE), else all of it to the one that
 * is; both marked pushed. */
void Players_PushApart(Player *plA, Player *plB)
{
    Capsule capA;
    Capsule capB;
    HitCapsule hitA;
    HitCapsule hitB;
    f32 push[3];

    Player_BodyCapsule(plA, &hitA, 40.0f);
    Capsule_Copy(&hitA, &capA);
    Player_BodyCapsule(plB, &hitB, 40.0f);
    Capsule_Copy(&hitB, &capB);
    if (Capsule_PushCapsule(&capB, &capA, push) != 0) {
        plA->pos[0] += 0.5f * push[0];
        plA->pos[2] += 0.5f * push[2];
        plB->pos[0] += -0.5f * push[0];
        plB->pos[2] += -0.5f * push[2];
        if (plA->action == PL_POSTURE_AIRBORNE && plB->action == PL_POSTURE_AIRBORNE) {
            plA->pos[1] += 0.5f * push[1];
            plB->pos[1] += -0.5f * push[1];
        } else if (plA->action == PL_POSTURE_AIRBORNE) {
            plA->pos[1] += push[1];
        } else if (plB->action == PL_POSTURE_AIRBORNE) {
            plB->pos[1] -= push[1];
        }
        plA->pushed = 1;
        plB->pushed = 1;
    }
}


/*
 * Keeps bodies from overlapping, once a frame. Clears every player's and
 * monster's pushed mark. Each local player in the current area is pushed
 * against the other players (only with debug switch 169 on) and against
 * the monsters in its area, its emDist set to its distance to each in turn.
 * Then each pair of monsters in the current area is pushed apart: never two
 * of kind 29; otherwise only when one is of kind 29, or when this
 * console runs both (Em.remote clear) and neither has Em.wallTouch set.
 * The monsters are walked through Player pointers, as the push functions
 * read them, so Em.remote is read as Player.startItemList, at the same
 * offset: read through an Em cast, the compiler assigns its registers
 * otherwise. Em.wallTouch is read unsigned, as the game's code loads it.
 * diffY and diffZ point into diff, set up with the first loop: the game's
 * code keeps the two pointers in registers.
 */
void Field_PushApart(void)
{
    Em *em;
    Player *other;
    Player *emA;
    s32 i;
    s32 j;
    f32 *diffY;
    f32 *diffZ;
    f32 diff[3];
    s16 wallFlag;
    f32 dist;
    Player *pl;

    for (i = 0, other = gPlayers; i < SCENE_PLAYERS; i++, other++) {
        other->pushed = 0;
    }
    for (em = gEms, j = 0; j < EM_COUNT; j++, em++) {
        em->pushed = 0;
    }
    for (i = 0, pl = gPlayers, diffY = &diff[1], diffZ = &diff[2]; i < SCENE_PLAYERS; i++, pl++) {
        if (pl->active != 0 && pl->visible != 0 && Player_IsLocal(pl) != 0 && pl->noPushTimer == 0
            && (u8)Obj_InCurrentArea(pl)) {
            if (Debug_IsOn(169) != 0) {
                for (j = 0, other = gPlayers; j < SCENE_PLAYERS; j++, other++) {
                    if (other->active != 0 && other->visible != 0 && other->noPushTimer == 0 && (u8)Obj_SameArea(pl, other)) {
                        Players_PushApart(pl, other);
                    }
                }
            }
            for (other = (Player *)gEms, j = 0; j < EM_COUNT; j++, other = (Player *)((u8 *)other + sizeof(Em))) {
                if (other->active != 0 && other->visible != 0 && other->noPushTimer == 0 && (u8)Obj_SameArea(pl, other)) {
                    PlayerEm_PushApart(pl, other);
                    diff[0] = pl->pos[0] - other->pos[0];
                    *diffY = pl->pos[1] - other->pos[1];
                    *diffZ = pl->pos[2] - other->pos[2];
                    dist = Vec_Length(diff);
                    pl->emDist = dist;
                }
            }
        }
    }
    for (i = 0, emA = (Player *)gEms; i < EM_COUNT - 1; i++, emA = (Player *)((u8 *)emA + sizeof(Em))) {
        if (emA->active != 0 && emA->visible != 0 && emA->noPushTimer == 0 && (u8)Obj_InCurrentArea(emA)) {
            wallFlag = (u16)((Em *)emA)->wallTouch != 0;
            for (j = i + 1, other = (Player *)&gEms[i + 1]; j < EM_COUNT; j++, other = (Player *)((u8 *)other + sizeof(Em))) {
                if (other->active != 0 && other->visible != 0) {
                    if (emA->weaponType == 29 && other->weaponType == 29) {
                        continue;
                    }
                    if ((emA->weaponType == 29 || other->weaponType == 29 || (emA->startItemList == 0 && other->startItemList == 0))
                        && other->noPushTimer == 0 && (u8)Obj_SameArea(emA, other)) {
                        if ((wallFlag == 0 && (u16)((Em *)other)->wallTouch == 0) || emA->weaponType == 29 || other->weaponType == 29) {
                            EmEm_PushApart(emA, other);
                        }
                    }
                }
            }
        }
    }
}

