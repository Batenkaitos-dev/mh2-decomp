/*
 * The monsters' common code, which every family uses. It covers motion layers
 * and root movement, turning the look and bending the body toward a target
 * (Em_TurnToward, Em_BendStep), moving by a turned velocity, and the state
 * change: dispatch to the families (Em_ChangeState), the forms that recompute
 * the goal and the online forms (Em_SetStateSync, Em_SyncState).
 *
 * It also holds rage, the goal a monster heads for (Em_ComputeGoal on the
 * host, Em_ComputeSyncedGoal from the host's values), and how a monster
 * senses and notices players: noticeCount, alertBuildup and the reaction
 * tables. Then come the four need gauges and the requests they raise
 * (Em_PickRequest), the pending placements, and handing a monster to another
 * console online (Em_MigrateHost).
 *
 * GmEm_Update calls the per-frame parts. The family files call the movement,
 * state and goal helpers. The tables at the end are indexed by Em.type:
 * reaction distances and angles, goal points, sense limits, alert buildup
 * rows, reactions, notice decay rates, the need rates, and the need-3 rates
 * followed by the poison parameters that gEmStatusParams points to.
 */
#include "common.h"
#include "quest.h"
#include "em.h"
#include "obj.h"
#include "player.h"
#include "scene.h"
#include "sprite.h"
#include "area.h"
#include "camera.h"
#include "capsule.h"
#include "render_state.h"
#include "sound.h"
#include "text.h"
#include "ambient_life.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "game_util.h"
#include "hit_effects.h"
#include "part_effects.h"
#include "rand.h"
#include "stage_data.h"
#include "volcano_hazard.h"
#include "vec_math.h"
#include "engine_util.h"
#include "em_script.h"
#include "prim.h"
#include "save.h"
#include "sprite_queue.h"


/* The objects of other files and modules this file's tables point to */
extern void * gEmKindTables_29[7];
extern void * gEmKindTables_Apceros[7];
extern void * gEmKindTables_Blangonga[7];
extern void * gEmKindTables_Cephadrome[7];
extern void * gEmKindTables_Cephalos[7];
extern void * gEmKindTables_Chameleos[6];
extern void * gEmKindTables_DaimyoHermitaur[7];
extern void * gEmKindTables_Diablos[7];
extern void * gEmKindTables_Fatalis[6];
extern void * gEmKindTables_Genprey[7];
extern void * gEmKindTables_Gravios[7];
extern void * gEmKindTables_Kelbi[7];
extern void * gEmKindTables_Khezu[7];
extern void * gEmKindTables_Kirin[7];
extern void * gEmKindTables_KushalaDaora[6];
extern void * gEmKindTables_LaoShanLung[7];
extern void * gEmKindTables_Lunastra[6];
extern void * gEmKindTables_Melynx[7];
extern void * gEmKindTables_Mosswine[7];
extern void * gEmKindTables_Plesioth[7];
extern void * gEmKindTables_Rathian[6];
extern void * gEmKindTables_ShenGaoren[7];
extern void * gEmKindTables_YamaTsukami[7];
extern void * gEmKindTables_YamaTsukamiB[7];
extern void * gEmKindTables_YianGaruga[7];
extern void * gEmKindTables_YianKutKu[7];
extern EmObj *gSightBlockObjs[32];


extern EmRate *gEmRateTbl[];

/* Em +0x6C4: four s16 values, one per player, and +0x6CC their last seen copies (Em_UpdateReactions runs
 * the reaction of a player whose value changed). Em has no fields there yet: a view of it. */
typedef struct EmReactView {
    u8 pad[1732];
    s16 cur[4];
    s16 last[4];
} EmReactView;

/* Em +0x818: what Em_SensePlayers measures of each player each frame (em.h has bytes there: other
 * code uses +0x846 as an s16). */
typedef struct EmSenseView {
    u8 pad[2020];
    s16 noticeTimer[4];              /* 0x7E4: Em unk7E4 */
    u8 pad7EC[2072 - 2028];
    f32 dist[4];                /* 0x818: from the monster to the player */
    f32 distXZ[4];              /* 0x828: the same, in the ground plane */
    f32 range[4];               /* 0x838: the four reaction ranges (Em_GetReaction kinds 3 to 6) */
    s32 alertBuildup[4];              /* 0x848: Em unk848, cleared when the player is out of reach */
    u16 dir[4];                 /* 0x858: the direction to the player (Math_AngleXZ) */
} EmSenseView;

extern s32 (**gEmPlayerActionKinds[])[4];
s32 EmBlangonga_CarriedShown(Em *em, u16 a);
s32 Em_PartBroken(Em *em, s32 arg1);
extern f32 D_008B0B8C[];
extern f32 BreakableStructure_EffectPos[][3];
extern EmObj *gFlashObjs[32];
typedef struct EmSight {
    f32 *pos;
    s32 target;
    s32 dir;
    u16 half;
} EmSight;
u8 AreaHit_Raycast(f32 *from, f32 *to, f32 *out, u16 k);
extern f32 D_008B0A94[];
extern f32 D_008B0A98[];
extern f32 D_008B0AC4[];
extern f32 D_008B0AC8[];
extern f32 CastleGate_PosOpen[][3];
extern f32 CastleGate_PosShut[][3];
typedef struct BreakableStructurePiece {
    s16 slot;
    s16 idx;
    f32 pos[3];
    u16 ang;
    u8 pad12[2];
    DrawSlot *ds;
} BreakableStructurePiece;
extern f32 BreakableStructure_Pos[][3];
extern f32 D_008B0A14[];
extern f32 D_008B0A18[];
extern f32 D_008B0AAC;
extern f32 TurningProp_Pos[][3];
void Player_BeginLayerMotion(Player *p, s32 frame, s32 n, s32 part);
/* unit's own declaration: bone declared s32 as Em_NearestActionToBone passes it; with the definition's s16 the call compiles differently */
void Em_BonePos(void *obj, s32 bone, f32 *pos);
extern f32 gCamMatrix[];
struct Segment;
EmReaction *Em_GetReaction(Em *em, u8 kind, u8 player);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmAnteka_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmApceros_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmBlangonga_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmBulldrome_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmCephadrome_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmChameleos_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmDiablos_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmDrome_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmFatalis_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmGaruga_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmGravios_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmHermitaur_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmKhezu_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmKirin_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmKushala_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmKutku_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmLaoshan_ChangeState(Em *em, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmMelynx_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmMosswine_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmPlesioth_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmRathalos_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmRemobra_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmShakalaka_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmShenGaoren_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmVelociprey_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmVespoid_ChangeState(Em *, s32, s32, s32);
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmYamaTsukami_ChangeState(Em *, s32, s32, s32);
struct TexRect;
/* unit's own declaration: group, state and mode declared s32 as the callers pass them; with the definition's u16, u16 and u8 the calls compile differently */
void EmKind38_ChangeState(Em *, s32, s32, s32);
extern u8 AreaObj_K19_PointsA2;
extern u8 AreaObj_K19_PointsA3;
extern u8 AreaObj_K19_PointsA6;
extern u8 AreaObj_K19_PointsA7;
extern EmObj *gAreaObjList[32];
extern f32 gViewMatrix[];
extern f32 gFrustumPlanes[];
typedef struct EmTurnView {
    u8 pad[1424];
    u16 target;
    u16 heading;
    u8 pad594[1596 - 1428];
    u32 bend[4];
    u16 speed;
    u16 hold;
    u8 phase;
} EmTurnView;
typedef struct EmBendView {
    u8 pad[1596];
    u32 bend[4];
} EmBendView;
/* end of prototypes */

/*
 * Starts motion `motion` on motion layer `layer` (below motionLayerCount)
 * from frame `frame`, blending over blend / 2 frames. There is no blend when
 * the layer already plays that motion or plays nothing. It also rebuilds
 * rotMtx from the rotation angles. Each byte offset is added before the
 * monster's address, and the (u32) on the second address stops the compiler
 * from sharing it with the third: the game's code computes each address
 * separately.
 */
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer)
{
    s32 off;
    u16 *idSlot;
    s16 *blendSlot;

    if (layer < em->motionLayerCount) {
        Mtx_FromAnglesXYZ(&em->rotX, em->rotMtx[0]);
        off = layer * 2;
        idSlot = (u16 *) (off + (s32) em + 668);
        if (*idSlot == motion || *idSlot == 0) {
            blend = 0;
        }
        *idSlot = motion;
        blendSlot = (s16 *) (off + (u32) em + 684);
        *blendSlot = blend / 2;
        *(u16 *) (off + (s32) em + 676) = frame;
        Player_BeginLayerMotion((Player *)em, *(u16 *) (off + (s32) em + 676), *blendSlot, layer);
    }
}

/*
 * Picks what the monster's look and body bend follow this frame and passes it
 * to Em_TurnToward. With lookAtTarget clear, or with no target player, the
 * turn is cleared (mode 0). With lookAtPoint set it follows lookPoint;
 * otherwise it follows the target player's position (Em_GetTargetPos).
 */
void Em_UpdateBendTarget(Em *em)
{
    f32 pos[3];
    s8 target;

    if (em->lookAtTarget == 0) {
        Em_TurnToward(em, gPlayers[0].pos, 0);
        return;
    }
    if (*(s8 *)&em->lookAtPoint != 0) { /* read as s8 here */
        Em_TurnToward(em, em->lookPoint, 1);
        return;
    }
    target = em->targetNo;
    if (target == -1) {
        Em_TurnToward(em, gPlayers[0].pos, 0);
        return;
    }
    Em_GetTargetPos(em, target & 0xFF, pos);
    Em_TurnToward(em, pos, 1);
}

/*
 * Turns the monster's look heading (turnHeading) toward the point `target`,
 * seen from the kind's look origin (gEmBendOrigins[type]->ahead units in
 * front of the monster), and then bends the body after it (Em_BendStep). Mode
 * 1 sets the wanted angle turnTarget (relative to rotY) and starts the turn.
 * Mode 0 clears it, so the look eases back to straight. turnPhase 1 starts
 * the turn (Chameleos ignores angles under 20 degrees), phase 2 speeds
 * turnSpeed up or down by turnAccel as the angle left shrinks, and phase 3
 * ends the turn. While the wanted angle crosses the half-turn line, turnHold
 * keeps the old wanted angle (an angle, not a frame count) until the step
 * allows the cross. At the end the four bend angles are masked to 16 bits.
 *
 * The (u16) casts on the target angle in case 2's side test and on cur after
 * the switch, the `(u16) cur >= 0x8000` test, the order of the declarations,
 * and holding Math_Heading's result in cur and case 2's first side flag in
 * `left` all stay: each makes the compiler mask, order and assign registers
 * as the game's code does.
 */
void Em_TurnToward(Em *em, f32 *target, s32 mode)
{
    u16 diff;
    u16 spd;
    EmTurnView *view = (EmTurnView *) em;
    u16 base = gEmBendOrigins[em->type]->turnSpeed;
    u16 max = gEmBendOrigins[em->type]->turnSpeedMax;
    u16 acc = gEmBendOrigins[em->type]->turnAccel;
    EmBendRow *tbl = gEmBendOrigins[em->type];
    s32 cur;
    s32 tgt;
    u16 holdDiff;
    u16 useHold;
    s32 wanted;
    u16 left;
    u16 side1;
    u16 held;
    u16 slowLim;
    f32 origin[3];
    f32 ofs[3];
    if (mode == 0) {
        view->target = 0;
        view->hold = 0;
        view->phase = 0;
    } else {
        origin[0] = 0.0f;
        origin[1] = 0.0f;
        origin[2] = tbl->ahead;
        Vec_Rotate(ofs, origin, (f32 *) em->rotMtx);
        origin[0] = em->posX + ofs[0];
        origin[2] = em->posZ + ofs[2];
        cur = (u16) Math_Heading(origin[0], origin[2], target[0], target[2]);
        view->target = cur - (ANGLE_TURN / 4) - em->rotY;
        if (view->phase == 0) {
            view->phase = 1;
        }
    }
    cur = view->heading;
    tgt = view->target;
    useHold = 0;
    diff = tgt - cur;
    switch (view->phase) {
    case 0:
        view->hold = tgt;
        view->speed = base;
        spd = base;
        break;
    case 1:
        left = diff;
        view->hold = tgt;
        view->speed = base;
        if (left > 0x8000) {
            left = 0x10000 - left;
        }
        if (em->type == EM_KIND_CHAMELEOS && left < 3641) {
            spd = 0;
            break;
        }
        if (left >= tbl->slowAngle / 4) {
            spd = base;
            view->phase = 2;
        } else if (left >= tbl->slowAngle / 8) {
            spd = base;
            view->phase = 3;
        } else {
            spd = base;
        }
        break;
    case 2:
        held = view->hold;
        holdDiff = held - cur;
        left = 0;
        if (held <= 0x8000 && cur >= 0x8000) {
            left = 1;
        }
        if ((u16) cur <= 0x8000 && held >= 0x8000) {
            left = 2;
        }
        side1 = 0;
        if ((u16) tgt <= 0x8000 && cur >= 0x8000) {
            side1 = 1;
        }
        if (cur <= 0x8000 && (u16) tgt >= 0x8000) {
            side1 = 2;
        }
        if (side1 == left) {
            view->hold = tgt;
        } else {
            switch (side1) {
            case 0:
                if ((u16) (diff + view->speed) < (u32) (view->speed * 2)) {
                    if (side1 == left) {
                        view->hold = tgt;
                    } else {
                        tgt = held;
                        diff = holdDiff;
                        useHold = 1;
                    }
                } else {
                    if (side1 == left) {
                        view->hold = tgt;
                    } else {
                        tgt = held;
                        diff = holdDiff;
                        useHold = 1;
                    }
                }
                break;
            case 1:
            case 2:
                slowLim = tbl->slowAngle;
                if ((((u16) tgt < slowLim && cur > 0x10000 - slowLim) || (cur < slowLim && 0x10000 - slowLim < (u16) tgt))
                    && (u16) (diff + view->speed) < (u32) (view->speed * 2)) {
                    if (side1 == left) {
                        view->hold = tgt;
                    } else {
                        tgt = held;
                        diff = holdDiff;
                        useHold = 1;
                    }
                } else {
                    if (side1 == left) {
                        view->hold = tgt;
                    } else {
                        tgt = held;
                        diff = holdDiff;
                        useHold = 1;
                    }
                }
                break;
            }
        }
        left = diff;
        if (left > 0x8000) {
            left = 0x10000 - left;
        }
        if (!useHold) {
            slowLim = tbl->slowAngle;
            if (left >= slowLim) {
                if (max > view->speed) {
                    view->speed += acc;
                }
            } else if (left >= slowLim / 2) {
                /* FAKEMATCH: the (s32) cast changes nothing in C (max is promoted
                 * anyway) but stays: without it the compiler computes max / 2 once
                 * for both tests and gives the speed and max / 2 each other's
                 * registers, unlike the game's code. */
                if (view->speed < (s32) max / 2) {
                    view->speed += acc;
                } else if (view->speed > max / 2) {
                    view->speed -= acc;
                }
            } else if (left >= slowLim / 4) {
                if (view->speed < (max / 8) * 3) {
                    view->speed += acc;
                } else if (view->speed > (max / 8) * 3) {
                    view->speed -= acc;
                }
            } else if (max / 4 < view->speed) {
                view->speed -= acc;
            } else {
                view->speed = base;
                view->phase = 3;
                view->hold = view->target;
                tgt = view->target;
            }
        } else if (max / 4 < view->speed) {
            view->speed -= acc;
        } else {
            view->speed = base;
            view->phase = 3;
            view->hold = view->target;
            tgt = view->target;
        }
        spd = view->speed;
        break;
    case 3:
        view->hold = tgt;
        left = diff;
        view->speed = base;
        if (left == 0) {
            spd = 0;
            view->phase = 1;
        } else {
            if (left > 0x8000) {
                left = 0x10000 - left;
            }
            if (left >= tbl->slowAngle / 4) {
                spd = base;
                view->phase = 2;
            } else {
                spd = base;
            }
        }
        break;
    }
    wanted = (useHold) ? view->hold : view->target;
    cur = view->heading;
    side1 = 0;
    if ((u16) wanted <= 0x8000 && (u16) cur >= 0x8000) {
        side1 = 1;
    }
    if ((u16) cur <= 0x8000 && (u16) wanted >= 0x8000) {
        side1 = 2;
    }
    switch (side1) {
    case 0:
        if ((u16) (diff + spd) < spd * 2) {
            view->heading = view->target;
            Em_BendStep(em, spd, tgt);
        } else if (diff < 0x8000) {
            view->heading = (u16) cur + spd;
            Em_BendStep(em, spd, tgt);
        } else {
            view->heading = (u16) cur - spd;
            Em_BendStep(em, spd, tgt);
        }
        break;
    case 1:
    case 2:
        slowLim = tbl->slowAngle;
        if (((wanted < slowLim && (u16) cur > 0x10000 - slowLim) || ((u16) cur < slowLim && 0x10000 - slowLim < wanted))
            && (u16) (diff + spd) < spd * 2) {
            view->heading = view->target;
            Em_BendStep(em, spd, tgt);
        } else if (side1 == 1) {
            view->heading = (u16) cur + spd;
            Em_BendStep(em, spd, tgt);
        } else {
            view->heading = (u16) cur - spd;
            Em_BendStep(em, spd, tgt);
        }
        break;
    }
    view->bend[0] = *(u16 *) &view->bend[0];
    view->bend[1] = *(u16 *) &view->bend[1];
    view->bend[2] = *(u16 *) &view->bend[2];
    view->bend[3] = *(u16 *) &view->bend[3];
}

/*
 * Moves the four bend angles (bendAngle) toward their shares of `angle`,
 * folded to the shorter side. The shares are |angle| / 2, / 4, / 8 and / 16,
 * each plus |angle| / 64 and capped by the kind's bendMax, with any excess
 * carried to the next. Each step is `speed` split the same way, and an angle
 * snaps to its share once within one step. One variable, cur, holds |angle|
 * and then each current bend angle. With a separate variable, the compiler
 * swaps two registers in the second half, unlike the game's code.
 */
void Em_BendStep(Em *em, u32 speed, s32 angle)
{
    EmBendView *view = (EmBendView *) em;
    EmBendRow *tbl = gEmBendOrigins[em->type];
    u32 step0;
    u32 step1;
    u32 step2;
    u32 step3;
    u32 want0;
    u32 want1;
    u32 want2;
    u32 want3;
    u32 gap;
    u32 cur;
    u32 mirror0;
    u32 mirror1;
    u32 mirror2;
    u32 mirror3;

    step0 = (u16) ((speed >> 1) + (speed >> 6));
    step1 = (u16) ((speed >> 2) + (speed >> 6));
    step2 = (u16) ((speed >> 3) + (speed >> 6));
    step3 = (u16) ((speed >> 4) + (speed >> 6));
    cur = (u16) angle;
    if (cur > 0x8000) {
        cur = 0x10000 - cur;
    }
    want0 = (u16) ((cur >> 1) + (cur >> 6));
    want1 = (u16) ((cur >> 2) + (cur >> 6));
    want2 = (u16) ((cur >> 3) + (cur >> 6));
    want3 = (u16) ((cur >> 4) + (cur >> 6));
    if (tbl->bendMax[0] < want0) {
        want1 += want0 - tbl->bendMax[0];
        want0 = tbl->bendMax[0];
    }
    if (tbl->bendMax[1] < want1) {
        want2 += want1 - tbl->bendMax[1];
        want1 = tbl->bendMax[1];
    }
    if (tbl->bendMax[2] < want2) {
        want3 += want2 - tbl->bendMax[2];
        want2 = tbl->bendMax[2];
    }
    if (tbl->bendMax[3] < want3) {
        want3 = tbl->bendMax[3];
    }
    if ((u16) angle < (ANGLE_TURN / 2)) {
        cur = view->bend[0];
        if (cur > 0x8000) {
            gap = want0 + (0x10000 - cur);
            if (gap >= 0x10000) {
                gap = (u16) (0x10000 - gap);
            }
        } else {
            gap = want0 - cur;
            if (gap >= 0x10000) {
                gap = (u16) (0x10000 - gap);
            }
        }
        if (gap <= step0) {
            view->bend[0] = want0;
        } else if (want0 < cur && cur < 0x8000) {
            view->bend[0] -= step0;
        } else {
            view->bend[0] += step0;
        }
        cur = view->bend[1];
        if (cur > 0x8000) {
            gap = want1 + (0x10000 - cur);
            if (gap >= 0x10000) {
                gap = (u16) (0x10000 - gap);
            }
        } else {
            gap = want1 - cur;
            if (gap >= 0x10000) {
                gap = (u16) (0x10000 - gap);
            }
        }
        if (gap <= step1) {
            view->bend[1] = want1;
        } else if (want1 < cur && cur < 0x8000) {
            view->bend[1] -= step1;
        } else {
            view->bend[1] += step1;
        }
        cur = view->bend[2];
        if (cur > 0x8000) {
            gap = want2 + (0x10000 - cur);
            if (gap >= 0x10000) {
                gap = (u16) (0x10000 - gap);
            }
        } else {
            gap = want2 - cur;
            if (gap >= 0x10000) {
                gap = (u16) (0x10000 - gap);
            }
        }
        if (gap <= step2) {
            view->bend[2] = want2;
        } else if (want2 < cur && cur < 0x8000) {
            view->bend[2] -= step2;
        } else {
            view->bend[2] += step2;
        }
        cur = view->bend[3];
        if (cur > 0x8000) {
            gap = want3 + (0x10000 - cur);
            if (gap >= 0x10000) {
                gap = (u16) (0x10000 - gap);
            }
        } else {
            gap = want3 - cur;
            if (gap >= 0x10000) {
                gap = (u16) (0x10000 - gap);
            }
        }
        if (gap <= step3) {
            view->bend[3] = want3;
        } else if (want3 < cur && cur < 0x8000) {
            view->bend[3] -= step3;
        } else {
            view->bend[3] += step3;
        }
    } else {
        cur = view->bend[0];
        mirror0 = 0x10000 - want0;
        want0 = mirror0 - cur;
        if (want0 >= 0x8000) {
            want0 = (u16) (0x10000 - want0);
        }
        if (want0 <= step0) {
            view->bend[0] = (u16) mirror0;
        } else if (cur < mirror0 && cur > 0x8000) {
            view->bend[0] += step0;
        } else {
            view->bend[0] -= step0;
        }
        cur = view->bend[1];
        mirror1 = 0x10000 - want1;
        want1 = mirror1 - cur;
        if (want1 >= 0x8000) {
            want1 = (u16) (0x10000 - want1);
        }
        if (want1 <= step1) {
            view->bend[1] = (u16) mirror1;
        } else if (cur < mirror1 && cur > 0x8000) {
            view->bend[1] += step1;
        } else {
            view->bend[1] -= step1;
        }
        cur = view->bend[2];
        mirror2 = 0x10000 - want2;
        want2 = mirror2 - cur;
        if (want2 >= 0x8000) {
            want2 = (u16) (0x10000 - want2);
        }
        if (want2 <= step2) {
            view->bend[2] = (u16) mirror2;
        } else if (cur < mirror2 && cur > 0x8000) {
            view->bend[2] += step2;
        } else {
            view->bend[2] -= step2;
        }
        cur = view->bend[3];
        mirror3 = 0x10000 - want3;
        want3 = mirror3 - cur;
        if (want3 >= 0x8000) {
            want3 = (u16) (0x10000 - want3);
        }
        if (want3 <= step3) {
            view->bend[3] = (u16) mirror3;
        } else if (cur < mirror3 && cur > 0x8000) {
            view->bend[3] += step3;
        } else {
            view->bend[3] -= step3;
        }
    }
}

extern s8 gAreaObjCount;
/*
 * Finds the record of gAreaObjList nearest to bone `bone` that lies within
 * the kind's action reach (sightTable value 6), measuring to the position
 * stored 0x30 bytes into each record. Writes that position to `out` and
 * returns 1, or returns 0 when there is none.
 */
s8 Em_NearestActionToBone(Em *em, s32 bone, f32 *out)
{
    f32 pos[4];
    f32 best = -1.0f;
    s32 i;
    u8 *rec;
    EmObj **slot;
    f32 dist;

    if (gAreaObjCount == 0) {
        return 0;
    }
    for (i = 0, slot = gAreaObjList; i < 32; i++, slot++) {
        rec = (u8 *) *slot;
        if (rec != NULL) {
            Em_BonePos(em, bone, pos);
            dist = Vec_Distance(pos, (f32 *) (rec + 48));
            if (dist <= ((f32 *) em->sightTable)[6]) {
                if (best == -1.0f) {
                    best = dist;
                    out[0] = *(f32 *) (rec + 48);
                    out[1] = *(f32 *) (rec + 52);
                    out[2] = *(f32 *) (rec + 56);
                } else if (dist < best) {
                    best = dist;
                    out[0] = *(f32 *) (rec + 48);
                    out[1] = *(f32 *) (rec + 52);
                    out[2] = *(f32 *) (rec + 56);
                }
            }
        }
    }
    return best == -1.0f ? 0 : 1;
}

/*
 * When need1Gauge is at or below the kind's seekLevel, the monster is on the
 * ground (stance 0) and some player is active in its area, it finds the
 * nearest area object of its area whose radius reaches the monster. With a
 * clear line from 100 above the monster to the object (AreaHit_Raycast), it
 * copies the object's kind, goalArg and goalNo to actionGoal and returns 1.
 * Otherwise actionGoal is all 0xFF and it returns 0. n first counts the
 * players away, then flags a find: one variable for both, as the game's code
 * uses one register.
 */
s32 Em_FindNearAreaObj(Em *em)
{
    EmObj *obj;
    u8 count = 0;
    f32 eye[4];
    f32 hitPos[4];
    f32 best = -1.0f;
    f32 dist;
    u8 nearest;
    u8 i;
    EmObj **w;
    u8 j;

    if (em->need1Gauge <= gEmRateTbl[em->type]->seekLevel && em->stance == 0) {
        for (i = 0; i < gScene.playerCount; i++) {
            if (gPlayers[i].active == 0 || Obj_SameArea(em, &gPlayers[i]) == 0) {
                count++;
            }
        }
        if (count == gScene.playerCount) {
            count = 0;
        } else {
            for (j = 0, count = 0, w = gAreaObjList; j < 32; j++, w++) {
                obj = (EmObj *) *w;
                if (obj != NULL && obj->area == em->area) {
                    dist = Vec_Distance(&em->posX, obj->pos);
                    if (dist <= obj->radius) {
                        count = 1;
                        if (best == -1.0f) {
                            best = dist;
                            nearest = j;
                        } else if (!(best <= dist)) {
                            best = dist;
                            nearest = j;
                        }
                    }
                }
            }
        }
    }
    if (count == 0) {
        em->actionGoal[0] = em->actionGoal[2] = em->actionGoal[1] = 255;
        return 0;
    }
    obj = (EmObj *) gAreaObjList[nearest];
    Vec_Copy(eye, &em->posX);
    eye[1] += 100.0f;
    if (AreaHit_Raycast(eye, (f32 *)obj, hitPos, em->collMask) == 0) {
        em->actionGoal[1] = obj->goalArg;
        em->actionGoal[2] = obj->goalNo;
        em->actionGoal[0] = obj->kind;
        return 1;
    }
    em->actionGoal[0] = em->actionGoal[2] = em->actionGoal[1] = 255;
    return 0;
}

/*
 * Returns the record of gAreaObjList whose goalArg and goalNo bytes match
 * actionGoal[1] and [2], or NULL.
 */
EmObj *Em_FindGoalAction(Em *em)
{
    s8 i;
    EmObj *act;

    if (gAreaObjCount == 0) {
        return NULL;
    }
    for (i = 0; i < 32; i++) {
        act = gAreaObjList[i];
        if (act != NULL && em->actionGoal[1] == act->goalArg && em->actionGoal[2] == act->goalNo) {
            return act;
        }
    }
    return NULL;
}

/*
 * On the host, makes the action spot found by Em_FindNearAreaObj the goal
 * (goalKind 7, goalArg and goalNo from actionGoal) and computes it. In all
 * cases it then runs Em_UseGoalAction and adds `need1Gain` to need1Gauge
 * (Em_AddNeed1).
 */
void Em_StartGoalAction(Em *em, s32 need1Gain)
{
    if (em->remote == 0) {
        em->goalKind = 7;
        em->goalArg = (s16) em->actionGoal[1];
        em->goalNo = (s16) em->actionGoal[2];
        Em_ComputeGoal(em, em->goal);
    }
    Em_UseGoalAction(em);
    Em_AddNeed1(em, need1Gain);
}

/*
 * Adds the monster's velocity (velX..velZ), turned by the angles `ang` (three
 * s32, ANGLE_TURN to a turn, Y-X-Z order), to its position.
 */
void Em_MoveRotated(Em *em, void *ang)
{
    f32 vel[3];
    f32 out[3];
    f32 mtx[16];

    vel[0] = em->velX;
    vel[1] = em->velY;
    vel[2] = em->velZ;
    Mtx_FromAnglesYXZ(ang, mtx);
    Vec_Rotate(out, vel, mtx);
    em->posX += out[0];
    em->posY += out[1];
    em->posZ += out[2];
}

/*
 * Same as Em_MoveRotated, then adds the acceleration (accelX..accelZ) to the
 * velocity.
 */
void Em_MoveRotatedAccel(Em *em, void *ang)
{
    f32 vel[4];
    f32 move[4];
    f32 mtx[16];

    Mtx_FromAnglesYXZ(ang, mtx);
    vel[0] = *(f32 *) &em->velX;
    vel[1] = em->velY;
    vel[2] = em->velZ;
    Vec_Rotate(move, vel, mtx);
    em->posX = em->posX + move[0];
    em->posY = em->posY + move[1];
    em->posZ = em->posZ + move[2];
    *(f32 *) &em->velX += *(f32 *) &em->accelX;
    em->velY += *(f32 *) &em->accelY;
    em->velZ += em->accelZ;
}

/*
 * Changes the monster to state `state` of group `group` through its family's
 * ChangeState, with `mode` one of EM_SYNC_*. Kinds without a family (kind 0,
 * the trader, the Felyne cart, kind 29) only draw stateRand and set the
 * state (Em_SetStateRand).
 */
void Em_ChangeState(Em *em, s32 group, s32 state, s32 mode)
{
    switch (em->type) {
    case EM_KIND_RATHIAN:
    case EM_KIND_RATHALOS:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_AZURE_RATHALOS:
        EmRathalos_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_FATALIS:
    case EM_KIND_FATALIS_B:
    case EM_KIND_FATALIS_C:
        EmFatalis_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_KELBI:
    case EM_KIND_ANTEKA:
        EmAnteka_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_MOSSWINE:
    case EM_KIND_BULLFANGO:
    case EM_KIND_PIGLET:
        EmMosswine_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_GYPCEROS:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_PURPLE_GYPCEROS:
        EmKutku_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_LAO_SHAN_LUNG:
    case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        EmLaoshan_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_CEPHADROME:
    case EM_KIND_CEPHALOS:
        EmCephadrome_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_FELYNE:
    case EM_KIND_MELYNX:
        EmMelynx_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_SHAKALAKA:
        EmShakalaka_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_HERBIVORE_0:
    case EM_KIND_APCEROS:
    case EM_KIND_POPO:
        EmApceros_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_GENPREY:
    case EM_KIND_VELOCIPREY:
    case EM_KIND_IOPREY:
    case EM_KIND_GIAPREY:
        EmVelociprey_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_DIABLOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_WHITE_MONOBLOS:
        EmDiablos_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
        EmKhezu_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_GRAVIOS:
    case EM_KIND_BASARIOS:
    case EM_KIND_BLACK_GRAVIOS:
        EmGravios_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
        EmPlesioth_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_VELOCIDROME:
    case EM_KIND_GENDROME:
    case EM_KIND_IODROME:
        EmDrome_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_VESPOID:
    case EM_KIND_HORNETAUR:
        EmVespoid_ChangeState(em, group, state, mode);
        return;
    case 0:
    default:
        Em_SetStateRand(em, group, state);
        return;
    case EM_KIND_KIRIN:
        EmKirin_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_YIAN_GARUGA:
        EmGaruga_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_DAIMYO_HERMITAUR:
    case EM_KIND_HERMITAUR:
    case EM_KIND_SHOGUN_CEANATAUR:
    case EM_KIND_CEANATAUR:
        EmHermitaur_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_BLANGONGA:
    case EM_KIND_CONGALALA:
    case EM_KIND_RAJANG:
    case EM_KIND_BLANGO:
    case EM_KIND_CONGA:
        EmBlangonga_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        EmKushala_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_SHEN_GAOREN:
        EmShenGaoren_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_THUNDERBUG:
        EmKind38_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_YAMA_TSUKAMI:
    case EM_KIND_YAMA_TSUKAMI_B:
        EmYamaTsukami_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_CHAMELEOS:
        EmChameleos_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_REMOBRA:
        EmRemobra_ChangeState(em, group, state, mode);
        return;
    case EM_KIND_BULLDROME:
        EmBulldrome_ChangeState(em, group, state, mode);
        return;
    }
}

/*
 * Recomputes the goal before the state change: from goalKind, goalArg and
 * goalNo on the host (Em_ComputeGoal), or from the values the host sent
 * (Em_ComputeSyncedGoal) when the monster is remote. It then calls
 * Em_ChangeState. The script's state command, the versus quest end and the
 * received host states use it.
 */
void Em_ChangeStateRouted(Em *em, s32 group, s32 state, s32 mode)
{
    if (em->remote == 0) {
        Em_ComputeGoal(em, em->goal);
    } else {
        Em_ComputeGoal2(em, em->goal);
    }
    Em_ChangeState(em, group, state, mode);
}

/*
 * Sets state `state` of group `group` (Player_ForceState), clears speedBonus
 * and sets stateRequest so the family runs the new state again in the same
 * frame.
 */
void Em_SetState(Em *em, s32 group, s32 state)
{
    em->speedBonus = 0;
    Player_ForceState(em, group, state);
    em->stateRequest = 1;
}

/* Draws a new stateRand, then Em_SetState. */
void Em_SetStateRand(Em *em, s32 group, s32 state)
{
    em->stateRand = Rand_Next(0);
    Em_SetState(em, group, state);
}

/*
 * Online, sets and/or sends the state by `mode` (EM_SYNC_*): LOCAL sets it
 * here, SEND only sends it (Em_SyncState), SEND_AND_SET does both, and OWNER
 * and OWNER_FLAGGED send it (the latter with flag 1) and set it unless the
 * monster is remote. Offline it draws stateRand and sets the state.
 */
void Em_SetStateSync(Em *em, s32 group, s32 state, u8 mode)
{
    if (Game_IsOnline() == 1) {
        switch (mode) {
        case 0:
            Em_SetState(em, group, state);
            break;
        case 1:
            Em_SyncState(em, group, state, 0);
            break;
        case 2:
            Em_SyncState(em, group, state, 0);
            Em_SetState(em, group, state);
            break;
        case 3:
            Em_SyncState(em, group, state, 0);
            if (em->remote == 0) {
                Em_SetState(em, group, state);
            }
            break;
        case 4:
            Em_SyncState(em, group, state, 1);
            if (em->remote == 0) {
                Em_SetState(em, group, state);
            }
            break;
        }
    } else {
        em->stateRand = Rand_Next(0);
        Em_SetState(em, group, state);
    }
}

/*
 * Writes to `out` the root movement of layer 0's motion over this frame's
 * step, scaled by scaleX..scaleZ. While blending it mixes the current
 * motion's movement with the blend source's by a Hermite weight; with a
 * negative blend the movement is zero.
 */
void Em_MotionDelta(Em *em, f32 *out)
{
    f32 moveA[4];
    f32 moveB[4];
    Model *model;
    f32 weight;
    f32 step;

    step = em->motion[0].step;
    model = em->model;
    if (em->motion[0].blend != 0) {
        if (em->motion[0].blend < 0) {
            out[0] = 0.0f;
            out[1] = 0.0f;
            out[2] = 0.0f;
        } else {
            Bone_GetAnimMoveFrom(em->motion[0].frame, em->motion[0].frame + step, moveA, model->partBones[0]->child);
            weight = Math_Hermite(em->motion[0].blendT, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
            Bone_GetAnimMoveFrom(em->motion[0].blendFrame, em->motion[0].blendFrame + step, moveB, model->partBones2[0]->child);
            Vec_Blend(out, moveA, moveB, 1.0f - weight, weight);
        }
    } else {
        Bone_GetAnimMoveFrom(em->motion[0].frame - step, em->motion[0].frame, out, model->partBones[0]->child);
    }
    out[0] *= em->scaleX;
    out[1] *= em->scaleY;
    out[2] *= em->scaleZ;
}

//---
/*
 * Adds `amount` times the kind's buildupByHp factor for the monster's hp
 * percentage (11 buckets, 100% then by tens) to rageBuildup. When it reaches
 * rageThreshold, it is capped there and ragePending is set. Congalala goes by
 * its need1Gauge percentage instead. Rajang counts only while
 * EmBlangonga_CarriedShown(em, 6) is nonzero. The Kushala, Lunastra and
 * Teostra kinds count double with part 0 broken. Nothing for kinds without a
 * rage row.
 */
void Em_CheckRageByHp(Em *em, s16 amount)
{
    s32 bucket;
    f32 pct;
    EmRageRow *row = gEmRageTbl[em->type];

    if (row == NULL) {
        return;
    }
    if (em->type == EM_KIND_CONGALALA) {
        pct = em->need1Gauge * 100 / em->need1Max;
    } else if (em->type == EM_KIND_RAJANG) {
        if (EmBlangonga_CarriedShown(em, 6) == 0) {
            return;
        }
        pct = em->hp * 100 / em->hpMax;
    } else {
        pct = em->hp * 100 / em->hpMax;
    }
    if (pct == 100.0f) {
        bucket = 0;
    } else if (!(pct < 90.0f)) {
        bucket = 1;
    } else if (!(pct < 80.0f)) {
        bucket = 2;
    } else if (!(pct < 70.0f)) {
        bucket = 3;
    } else if (!(pct < 60.0f)) {
        bucket = 4;
    } else if (!(pct < 50.0f)) {
        bucket = 5;
    } else if (!(pct < 40.0f)) {
        bucket = 6;
    } else if (!(pct < 30.0f)) {
        bucket = 7;
    } else if (!(pct < 20.0f)) {
        bucket = 8;
    } else if (!(pct < 10.0f)) {
        bucket = 9;
    } else {
        bucket = 10;
    }
    if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
        if (Em_PartBroken(em, 0) == 1) {
            amount += amount;
        }
    }
    if (em->type == EM_KIND_LUNASTRA || em->type == EM_KIND_TEOSTRA) {
        if (Em_PartBroken(em, 0) == 1) {
            amount += amount;
        }
    }
    em->rageBuildup += (s16) ((f32) amount * row->buildupByHp[bucket]);
    if (em->rageBuildup >= em->rageThreshold && em->ragePending == 0) {
        em->rageBuildup = em->rageThreshold;
        em->ragePending = 1;
    }
}

/*
 * Starts the rage that the buildup made pending. It clears ragePending, sets
 * rage, rageTimer and the level-scaled attack and damage factors from the
 * kind's rage row, sets scriptPending, raises EM_TRIGGER_RAGE and clears
 * EM_TRIGGER_AREA_OBJ. Nothing for kinds without a rage row.
 */
void Em_StartRage2(Em *em)
{
    EmRageRow *rage;

    rage = gEmRageTbl[em->type];
    if (rage != NULL) {
        em->ragePending = 0;
        em->rage = 1;
        em->rageTimer = rage->time;
        em->attackFactor = rage->attackFactor * Em_LevelAttackFactor((Em *) em);
        em->damageFactor = rage->damageFactor * Em_LevelDamageFactor((Em *) em);
        em->scriptPending = 1;
        em->triggers |= 8;
        em->triggers &= 0xFD;
    }
}

/*
 * Sets rage, rageTimer and the level-scaled attack and damage factors from
 * the kind's rage row, without touching the triggers or the script.
 */
void Em_StartRage(Em *em)
{
    EmRageRow *rage;

    rage = gEmRageTbl[em->type];
    if (rage != NULL) {
        em->rage = 1;
        em->rageTimer = rage->time;
        em->attackFactor = rage->attackFactor * Em_LevelAttackFactor((Em *) em);
        em->damageFactor = rage->damageFactor * Em_LevelDamageFactor((Em *) em);
    }
}

/*
 * Turns the versus gauge into a rage. On the host it lasts versusGauge * 30 +
 * 450 frames (also stored in timer); a remote monster takes the length from
 * timer. It sets rage, rageStarted and the rage factors, and clears
 * versusGauge. Nothing without a rage row or with a zero gauge.
 */
void Em_StartPendingRage(Em *em)
{
    s16 frames;
    EmRageRow *rage;

    rage = gEmRageTbl[em->type];
    if ((rage != NULL) && (em->versusGauge != 0)) {
        em->rage = 1;
        em->rageStarted = 1;
        if (em->remote == 0) {
            frames = (em->versusGauge * 30) + 450;
            em->rageTimer = frames;
            em->timer = (s32) frames;
        } else {
            em->rageTimer = (s16) em->timer;
        }
        em->attackFactor = rage->attackFactor * Em_LevelAttackFactor((Em *) em);
        em->damageFactor = rage->damageFactor * Em_LevelDamageFactor((Em *) em);
        em->versusGauge = 0;
    }
}

/*
 * For each player whose reactValue (the damage it has dealt to the monster)
 * changed, copies it to reactValueLast and applies that player's hit reaction
 * (Em_GetReaction kind 1) to its noticeCount.
 */
void Em_UpdateReactions(Em *em)
{
    s32 i;
    EmReactView *view = (EmReactView *) em;
    EmReaction *react;

    for (i = 0; i < 4; i++) {
        if (view->last[i] != view->cur[i]) {
            view->last[i] = view->cur[i];
            react = Em_GetReaction(em, 1, i);
            Em_AddNotice(em, react->noticeAdd, react->noticeMax, i);
        }
    }
}

/*
 * Returns how many frames a player stays noticed by a monster of kind `type`:
 * 900 for the Prey and Drome kinds, 2700 for Yian Garuga, 1800 for every
 * other kind.
 */
static s16 Em_NoticeTime(u8 type)
{
    s32 kind;

    kind = type & 0xFF;
    switch (kind) {
    case 16:
    case 35:
    case 13:
    case 30:
    case 27:
    case 28:
    case 31:
        return 900;
    case 40:
        return 2700;
    default:
        return 1800;
    }
}

/*
 * Adds `add` to the noticeCount of player `player` when the sum stays under
 * `max`; otherwise the count is left alone. Only active players that have not
 * fainted count. From 9000 on, with the player in the monster's area, it
 * marks the player noticed and restarts its noticeTimer (Em_NoticeTime).
 */
void Em_AddNotice(Em *em, s32 add, s32 max, u8 player)
{
    s32 i = player;
    Player *pl = &gPlayers[i];
    s32 *count;
    s32 sum;
    s32 off;

    if (pl->active != 0 && pl->stateGroup != PL_GROUP_FAINT) {
        /* a byte offset of its own, added before the monster's address (`&em->noticeCount[i]` adds em
         * first) */
        off = i * 4;
        count = (s32 *) (off + (s32) em + 2156);
        sum = *count + add;
        if (sum < max) {
            *count = sum;
        }
        if (*count >= 9000 && Obj_SameArea(em, pl) != 0) {
            em->noticed |= 1 << player;
            em->noticeTimer[i] = Em_NoticeTime(em->type);
        }
    }
}

/*
 * Each frame, lowers the noticeCount values by the kind's gEmNoticeDecayRates
 * (never below 0). The combat target's count drops by rate 0 in the monster's
 * area or rate 2 out of it. A player not sensed, or past 30000 and not the
 * target, drops by rate 1 in the area or rate 3 out of it. A player carrying
 * something then gets reaction 2. A player with flag 0x10000 in the area gets
 * reaction 8, which also marks the player noticed when it adds anything.
 */
void Em_DecayNotice(Em *em)
{
    s32 i;
    s32 *rate = gEmNoticeDecayRates[em->type];
    Player *target;

    if (em->combat == 1 && em->targetNo != -1) {
        target = &gPlayers[em->targetNo & 0xF];
        if (Obj_SameArea(em, target) != 0 && target->active != 0) {
            if (rate[0] < em->noticeCount[em->targetNo]) {
                em->noticeCount[em->targetNo] -= rate[0];
            } else {
                em->noticeCount[em->targetNo] = 0;
            }
        } else {
            if (rate[2] < em->noticeCount[em->targetNo]) {
                em->noticeCount[em->targetNo] -= rate[2];
            } else {
                em->noticeCount[em->targetNo] = 0;
            }
        }
    }
    for (i = 0; i < 4; i++) {
        EmReaction *react;

        if (gPlayers[i].active == 0) {
            continue;
        }
        if (!(em->sensedPlayers & (1 << i)) || (em->noticeCount[i] >= 30000 && em->targetNo != i)) {
            if (Obj_SameArea(em, &gPlayers[i]) != 0 && gPlayers[i].active != 0) {
                if (rate[1] < em->noticeCount[i]) {
                    em->noticeCount[i] -= rate[1];
                } else {
                    em->noticeCount[i] = 0;
                }
            } else {
                if (rate[3] < em->noticeCount[i]) {
                    em->noticeCount[i] -= rate[3];
                } else {
                    em->noticeCount[i] = 0;
                }
            }
        }
        if (gPlayers[i].carry & PL_CARRY_HOLD_MASK) {
            react = Em_GetReaction(em, 2, i);
            Em_AddNotice(em, react->noticeAdd, react->noticeMax, i);
        }
        if (Player_TestFlag(&gPlayers[i], 0x10000) != 0 && Obj_SameArea(em, &gPlayers[i]) != 0) {
            react = Em_GetReaction(em, 8, i);
            Em_AddNotice(em, react->noticeAdd, react->noticeMax, i);
            if (react->noticeAdd != 0) {
                em->noticed |= (u8) (1 << i);
                em->noticeTimer[i] = Em_NoticeTime(em->type);
            }
        }
    }
}

/*
 * Rates how noticeable player `player`'s action is, from 0 to 4
 * (Player_TestFlag). Crouched it is 1 with flags 0x0201020B and 0 otherwise.
 * Standing, airborne or down it is 4 with flags 0x02000200, 3 with flags
 * 0x1000B and 2 otherwise. Other postures give 0. Rank 0 also selects the
 * skill-19 reactions in Em_GetReaction.
 */
u8 Em_PlayerActionKind(u8 player)
{
    u8 rank = 0;
    Player *pl = &gPlayers[player];

    switch (pl->action) {
    case PL_POSTURE_CROUCHED:
        if (Player_TestFlag(pl, 0x0201020B) != 0) {
            rank = 1;
        }
        break;
    case PL_POSTURE_STANDING:
    case PL_POSTURE_AIRBORNE:
    case PL_POSTURE_DOWN:
        rank = 2;
        if (Player_TestFlag(pl, 0x02000200) != 0) {
            rank = 4;
        } else if (Player_TestFlag(pl, 0x1000B) != 0) {
            rank = 3;
        }
        break;
    }
    return rank;
}

/* One sector's case of Em_AddAlertBuildup: when cond holds, adds the base
 * amount of the reaction range and the sector row's amount for the action
 * rank. The rank is copied to a block-local k first: the game's code keeps
 * the masked rank in a register of its own for each case. */
#define EM_AGGRO_ADD(cond, rank, row) {                   \
        s32 k = (rank);                                   \
        if (cond) {                                       \
            em->alertBuildup[playerIdx] += base[range];   \
            em->alertBuildup[playerIdx] += rows[row][k][range]; \
        }                                                 \
    }

/*
 * Out of combat, with no trigger script of rank EM_TRIGGER_PART_BUILDUP or higher running and
 * the player's alertBuildup under the kind's limit, it adds to alertBuildup.
 * The amount is the base for reaction range `range` (0 nearest to 3) plus the
 * gEmPlayerActionKinds row of sense sector `sector`, picked by the player's
 * action rank (Em_PlayerActionKind). Sectors 0 and 4 (outside all four
 * sectors) always add. Sector 1 needs rank 1 or more, sector 2 rank 2 or
 * more, and sector 3 rank 3 or more. A player with skill 19 adds nothing
 * unless at rank 4. Each case copies the rank to its own block-local k, and
 * case 1 tests r != 0: this keeps the masked rank in the register the game's
 * code uses.
 */
void Em_AddAlertBuildup(Em *em, u8 player, u8 range, s8 sector)
{
    u8 rank;
    Player *pl;
    s32 *base;
    s32 (**rows)[4];
    s32 playerIdx;

    playerIdx = player;
    pl = &gPlayers[playerIdx];
    rows = gEmPlayerActionKinds[em->type];
    base = (s32 *) rows[0];

    if (em->alertBuildup[playerIdx] < gEmPartBuildupLimits[em->type][em->alertLimitRow] && Em_ScriptFlagsFrom(em, EM_TRIGGER_PART_BUILDUP) == 0 && em->combat == 0) {
        rank = Em_PlayerActionKind(player);
        if (Player_HasSkill(pl, 19) != 1 || rank == 4) {
            switch (sector) {
            case 0:
                EM_AGGRO_ADD(1, rank, 1);
                break;
            case 1:
                EM_AGGRO_ADD(rank != 0, rank, 2);
                break;
            case 2:
                EM_AGGRO_ADD(rank > 1, rank, 3);
                break;
            case 3:
                EM_AGGRO_ADD(rank > 2, rank, 4);
                break;
            case 4:
                EM_AGGRO_ADD(1, rank, 1);
                break;
            }
        }
    }
}

#undef EM_AGGRO_ADD

extern EmReactionDists *gEmReactionDists[];
extern EmReactionAngles *gEmReactionAngles[];
/*
 * Loads reaction set `k` of the monster's kind: the four distance limits
 * (reactDist[0..1], nearRadius and a fourth at the bytes shared with
 * inBattle), the four reaction ranges (reactRange and a fourth at the bytes
 * shared with aimPitch) and the four sector half-angles (sectorHalfAngle). It
 * also sets reactionSet to k. The row pointers are loaded first and offset by
 * k in separate statements: as one expression, the compiler would assign the
 * address registers differently from the game's code.
 */
void Em_LoadReactionSet(Em *em, u8 k)
{
    f32 *dists;
    u16 *angles;

    dists = (f32 *)gEmReactionDists[em->type];
    angles = (u16 *)gEmReactionAngles[em->type];
    dists = (f32 *)((u32)dists + k * 32);
    angles = (u16 *)((u32)angles + k * 8);
    em->reactionSet = k;
    em->reactDist[0] = *dists++;
    em->reactDist[1] = *dists++;
    em->nearRadius = *dists++;
    *(f32 *) ((u8 *) em + 1904) = *dists++; /* limit 3: Em has the Player fields inBattle, prevInBattle here */
    em->reactRange[0] = *dists++;
    em->sectorHalfAngle[0] = *angles++;
    em->reactRange[1] = *dists++;
    em->sectorHalfAngle[1] = *angles++;
    em->reactRange[2] = *dists++;
    em->sectorHalfAngle[2] = *angles++;
    *(f32 *) ((u8 *) em + 2116) = *dists++; /* range 3: Em has the Player field aimPitch at +0x846 */
    em->sectorHalfAngle[3] = *angles++;
}

/* the monster's position tables (Em_AreaEntryValue, Em_FindAreaEntry) */
struct EmIdEntry;
f32 *Em_AreaEntryValue(Em *em, struct EmIdEntry *entry);
struct EmIdEntry *Em_FindAreaEntry(Em *em, struct EmIdEntry *entry, s16 id);
extern u8 gEmGoalPosSets[];
extern f32 *gEmGoalPoints[];

/*
 * On the host, computes the goal point `target` (usually em->goal) from
 * goalKind, goalArg and goalNo, and copies them to curGoalKind, curGoalArg
 * and curGoalNo for the online copy. The kinds are: 1 a player (in combat,
 * the script's player); 2 a point of an area table, or the area's first
 * point, or a fixed point of the area record; 3 another area (nextArea,
 * Em_PickRoutePoint); 5 a point of the kind's list; 6 a point 500 or 1000
 * away at a quarter-turn step from the heading; 7 the matching area object; 8
 * objectPos; 9 a route point; 11 a point near a player; 13 another monster.
 * When the table, route or object is missing, it falls back to the first
 * point of the area's table 0.
 */
void Em_ComputeGoal(Em *em, f32 *target)
{
    s8 i;
    f32 *point;
    EmRouteSet *set;
    EmRoutePt *routePts;
    EmObj *obj;
    u8 found;
    EmObj **w;
    f32 *pts;
    u16 k;
    u16 turn;
    f32 scale;
    f32 aheadVec[3];
    f32 aheadOfs[3];
    s32 aheadAng[3];
    s32 sideAng[3];
    f32 sideOfs[3];
    f32 sideVec[3];
    f32 aheadMtx[16];
    f32 sideMtx[16];

    em->curGoalKind = em->goalKind;
    em->curGoalArg = *(u16 *) &em->goalArg;
    em->curGoalNo = *(u16 *) &em->goalNo;
    switch (em->goalKind) {
    case 1:
        switch (em->curGoalArg) {
        case 0:
        if (em->curGoalNo == -1) {
            em->targetNo = -1;
            em->target = NULL;
        } else {
        if (em->combat == 1 && (s8) em->scriptPlayer != -1) {
            em->goalNo = (s8) em->scriptPlayer & 0xF;
            em->curGoalNo = *(u16 *) &em->goalNo;
        }
        em->targetNo = *(u16 *) &em->goalNo;
        em->target = &gPlayers[em->targetNo];
            Em_GetTargetPos(em, em->targetNo, target);
        }
        break;
        }
        break;
    case 2:
        if (*(u16 *) &em->goalArg == 7) {
            pts = Em_AreaEntryValue(em, (struct EmIdEntry *)gEmGoalPosSets);
        } else {
            pts = Em_AreaEntryValue(em, em->tables[*(u16 *) &em->goalArg]);
        }
        switch (*(u16 *) &em->goalArg) {
        case 0:
        first:
            if (pts == NULL) {
                target[0] = 5000.0f;
                target[1] = em->posY;
                target[2] = 5000.0f;
            } else {
                target[0] = pts[0];
                target[1] = pts[1];
                target[2] = pts[2];
            }
            break;
        case 1:
        case 7:
            if (pts == NULL) {
                target[0] = 5000.0f;
                target[1] = 0.0f;
                target[2] = 5000.0f;
            } else {
                point = pts + *(u16 *) &em->goalNo * 3;
                target[0] = point[0];
                target[1] = point[1];
                target[2] = point[2];
            }
            break;
        case 2:
            em->curGoalNo = *(u16 *) &em->goalNo;
            if (pts == NULL) {
                point = (f32 *)Stage_GetAreaRecord(em->area);
                switch (*(u16 *) &em->goalNo) {
                case 0:
                    target[0] = 0.2f * point[4];
                    target[1] = 1000.0f + point[6];
                    target[2] = 0.2f * point[5];
                    break;
                case 1:
                    target[0] = 0.2f * point[4];
                    target[1] = 1000.0f + point[6];
                    target[2] = 0.8f * point[5];
                    break;
                case 2:
                    target[0] = 0.8f * point[4];
                    target[1] = 1000.0f + point[6];
                    target[2] = 0.8f * point[5];
                    break;
                case 3:
                default:
                    target[0] = 0.8f * point[4];
                    target[1] = 1000.0f + point[6];
                    target[2] = 0.2f * point[5];
                    break;
                }
            } else {
                point = pts + *(u16 *) &em->goalNo * 3;
                target[0] = point[0];
                target[1] = point[1];
                target[2] = point[2];
            }
            break;
        case 3:
            if (pts == NULL) {
                em->goalArg = 0;
                em->curGoalArg = *(u16 *) &em->goalArg;
                pts = Em_AreaEntryValue(em, em->tables[0]);
                goto first;
            }
            point = pts + (em->curGoalNo = *(u16 *) &em->goalNo) * 3;
            target[0] = point[0];
            target[1] = point[1];
            target[2] = point[2];
            break;
        case 4:
            if (pts == NULL) {
                em->goalArg = 0;
                em->curGoalArg = *(u16 *) &em->goalArg;
                pts = Em_AreaEntryValue(em, em->tables[0]);
                goto first;
            }
            point = pts + *(u16 *) &em->goalNo * 3;
            target[0] = point[0];
            target[1] = point[1];
            target[2] = point[2];
            break;
        case 5:
            if (pts == NULL) {
                em->goalArg = 0;
                em->curGoalArg = *(u16 *) &em->goalArg;
                pts = Em_AreaEntryValue(em, em->tables[0]);
                goto first;
            }
            point = pts + *(u16 *) &em->goalNo * 3;
            target[0] = point[0];
            target[1] = point[1];
            target[2] = point[2];
            break;
        }
        break;
    case 3:
        em->nextArea = *(u16 *) &em->goalNo;
        Em_PickRoutePoint(em);
        em->goalArg = *(u16 *) &em->destArea;
        break;
    case 5:
        point = gEmGoalPoints[em->type];
        target[0] = point[em->curGoalNo * 3 + 0];
        target[1] = point[em->curGoalNo * 3 + 1];
        target[2] = point[em->curGoalNo * 3 + 2];
        break;
    case 6:
        switch (em->curGoalArg) {
        case 0:
            scale = 1.0f;
            turn = 0;
            break;
        case 1:
            scale = 1.0f;
            turn = 49152;
            break;
        case 2:
            scale = 1.0f;
            turn = 16384;
            break;
        case 3:
            scale = 1.0f;
            turn = 32768;
            break;
        case 8:
            scale = 2.0f;
            turn = 0;
            break;
        case 9:
            scale = 2.0f;
            turn = 49152;
            break;
        case 10:
            scale = 2.0f;
            turn = 16384;
            break;
        case 11:
            scale = 2.0f;
            turn = 32768;
            break;
        }
        aheadOfs[0] = 0.0f;
        aheadOfs[1] = 0.0f;
        aheadOfs[2] = 500.0f * scale;
        aheadAng[0] = 0;
        aheadAng[1] = (u16) (em->rotY + turn);
        aheadAng[2] = 0;
        Mtx_FromAnglesXYZ(aheadAng, aheadMtx);
        Vec_Rotate(aheadVec, aheadOfs, aheadMtx);
        target[0] = em->posX + aheadVec[0];
        target[1] = em->posY + aheadVec[1];
        target[2] = em->posZ + aheadVec[2];
        break;
    case 7:
        found = 0;
        if (gAreaObjCount != 0) {
            for (i = 0, w = gAreaObjList; i < 32; i++, w++) {
                obj = *w;
                if (obj != NULL && obj->goalArg == em->curGoalArg && obj->goalNo == em->curGoalNo) {
                    found = 1;
                    target[0] = obj->pos[0];
                    target[1] = obj->pos[1];
                    target[2] = obj->pos[2];
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        pts = Em_AreaEntryValue(em, em->tables[0]);
        goto first;
    case 8:
        target[0] = em->objectPos[0];
        target[1] = em->objectPos[1];
        target[2] = em->objectPos[2];
        break;
    case 9:
        set = (EmRouteSet *) Em_FindAreaEntry(em, em->tables[6], em->area);
        if (set == NULL) {
            pts = Em_AreaEntryValue(em, em->tables[0]);
            goto first;
        }
        if (set->routes == NULL) {
            pts = Em_AreaEntryValue(em, em->tables[0]);
            goto first;
        }
        routePts = set->routes[em->curGoalArg].pts;
        if (routePts == NULL) {
            pts = Em_AreaEntryValue(em, em->tables[0]);
            goto first;
        }
        target[0] = routePts[em->curGoalNo].pos[0];
        target[1] = routePts[em->curGoalNo].pos[1];
        target[2] = routePts[em->curGoalNo].pos[2];
        em->routeNo = em->curGoalArg;
        em->routePoint = em->curGoalNo;
        break;
    case 11:
        em->targetNo = em->curGoalArg;
        em->target = &gPlayers[em->targetNo];
        k = *(u16 *) ((u8 *) em->target + 1634);
        point = Em_AreaEntryValue(em, em->tables[0]) + k * 3;
        k = Math_AngleXZ(point, &em->posX);
        sideAng[0] = 0;
        sideAng[1] = k;
        sideAng[2] = 0;
        switch (em->curGoalNo) {
        case 0:
            sideOfs[0] = 0.0f;
            sideOfs[1] = 500.0f;
            sideOfs[2] = 1000.0f;
            break;
        case 1:
            sideOfs[0] = 0.0f;
            sideOfs[1] = 400.0f;
            sideOfs[2] = 800.0f;
            break;
        case 2:
            sideOfs[0] = 0.0f;
            sideOfs[1] = 400.0f;
            sideOfs[2] = 1500.0f;
            break;
        case 3:
            sideOfs[0] = 0.0f;
            sideOfs[1] = 600.0f;
            sideOfs[2] = 600.0f;
            break;
        }
        Mtx_FromAnglesYXZ(sideAng, sideMtx);
        Vec_Rotate(sideVec, sideOfs, sideMtx);
        target[0] = point[0] + sideVec[0];
        target[1] = point[1] + sideVec[1];
        target[2] = point[2] + sideVec[2];
        break;
    case 13:
        switch (em->curGoalArg) {
        case 0:
        if (em->curGoalNo == -1) {
            em->targetNo = -1;
            em->target = NULL;
        } else {
        if (em->combat == 1 && (s8) em->scriptPlayer != -1) {
            em->goalNo = (s8) em->scriptPlayer % 20;
            em->curGoalNo = *(u16 *) &em->goalNo;
        }
        em->targetNo = *(u16 *) &em->goalNo;
        em->target = &gEms[em->targetNo];
            Em_GetTargetPos(em, em->targetNo, target);
        }
        break;
        }
        break;
    }
    em->curGoalKind = em->goalKind;
    em->curGoalArg = *(u16 *) &em->goalArg;
    em->curGoalNo = *(u16 *) &em->goalNo;
}

/*
 * Computes the goal point `target` from curGoalKind, curGoalArg and curGoalNo
 * (the values the host sent, or a pending placement's) and copies them back
 * to goalKind, goalArg and goalNo. It works as Em_ComputeGoal does, with
 * these differences: kinds 1 and 13 also set scriptPlayer; kind 3 only sets
 * destArea and nextArea; kind 7 also fills actionGoal; kind 9 keeps the route
 * and point indices in range; kind 12 sets trapTimer to curGoalNo * 4
 * (goalArg 0).
 */
void Em_ComputeGoal2(Em *em, f32 *target)
{
    s8 i;
    f32 *point;
    EmRouteSet *set;
    EmRoute *routeList;
    EmRoutePt *routePts;
    EmObj *obj;
    u8 found;
    EmObj **w;
    f32 *pts;
    u16 k;
    u16 turn;
    f32 scale;
    f32 aheadVec[3];
    f32 aheadOfs[3];
    s32 aheadAng[3];
    s32 sideAng[3];
    f32 sideOfs[3];
    f32 sideVec[3];
    f32 aheadMtx[16];
    f32 sideMtx[16];

    em->goalKind = em->curGoalKind;
    em->goalArg = em->curGoalArg;
    em->goalNo = em->curGoalNo;
    switch (em->curGoalKind) {
    case 1:
        switch (em->curGoalArg) {
        case 0:
            if (em->curGoalNo == -1) {
                em->targetNo = -1;
                em->target = NULL;
            } else {
                em->targetNo = em->curGoalNo;
                em->target = &gPlayers[em->curGoalNo];
                Em_GetTargetPos(em, em->targetNo, target);
                em->scriptPlayer = em->curGoalNo;
            }
            break;
        }
        break;
    case 2:
        if (em->curGoalArg == 7) {
            pts = Em_AreaEntryValue(em, (struct EmIdEntry *)gEmGoalPosSets);
        } else {
            pts = Em_AreaEntryValue(em, em->tables[em->curGoalArg]);
        }
        switch (em->curGoalArg) {
        case 0:
        first:
            if (pts == NULL) {
                target[0] = 5000.0f;
                target[1] = em->posY;
                target[2] = 5000.0f;
            } else {
                target[0] = pts[0];
                target[1] = pts[1];
                target[2] = pts[2];
            }
            break;
        case 1:
        case 7:
            if (pts == NULL) {
                target[0] = 5000.0f;
                target[1] = 0.0f;
                target[2] = 5000.0f;
            } else {
                point = pts + em->curGoalNo * 3;
                target[0] = point[0];
                target[1] = point[1];
                target[2] = point[2];
            }
            break;
        case 2:
            if (pts == NULL) {
                point = (f32 *)Stage_GetAreaRecord(em->area);
                switch (em->curGoalNo) {
                case 0:
                    target[0] = 0.2f * point[4];
                    target[1] = 1000.0f + point[6];
                    target[2] = 0.2f * point[5];
                    break;
                case 1:
                    target[0] = 0.2f * point[4];
                    target[1] = 1000.0f + point[6];
                    target[2] = 0.8f * point[5];
                    break;
                case 2:
                    target[0] = 0.8f * point[4];
                    target[1] = 1000.0f + point[6];
                    target[2] = 0.8f * point[5];
                    break;
                case 3:
                default:
                    target[0] = 0.8f * point[4];
                    target[1] = 1000.0f + point[6];
                    target[2] = 0.2f * point[5];
                    break;
                }
            } else {
                point = pts + em->curGoalNo * 3;
                target[0] = point[0];
                target[1] = point[1];
                target[2] = point[2];
            }
            break;
        case 3:
        case 4:
        case 5:
            if (pts != NULL) {
                point = pts + em->curGoalNo * 3;
                target[0] = point[0];
                target[1] = point[1];
                target[2] = point[2];
            } else {
                goto fallback;
            }
            break;
        }
        break;
    case 3:
        em->destArea = em->curGoalArg;
        em->nextArea = em->curGoalNo;
        break;
    case 5:
        point = gEmGoalPoints[em->type];
        target[0] = point[em->curGoalNo * 3 + 0];
        target[1] = point[em->curGoalNo * 3 + 1];
        target[2] = point[em->curGoalNo * 3 + 2];
        break;
    case 6:
        switch (em->curGoalArg) {
        case 0:
            scale = 1.0f;
            turn = 0;
            break;
        case 1:
            scale = 1.0f;
            turn = 49152;
            break;
        case 2:
            scale = 1.0f;
            turn = 16384;
            break;
        case 3:
            scale = 1.0f;
            turn = 32768;
            break;
        case 8:
            scale = 2.0f;
            turn = 0;
            break;
        case 9:
            scale = 2.0f;
            turn = 49152;
            break;
        case 10:
            scale = 2.0f;
            turn = 16384;
            break;
        case 11:
            scale = 2.0f;
            turn = 32768;
            break;
        }
        aheadOfs[0] = 0.0f;
        aheadOfs[1] = 0.0f;
        aheadOfs[2] = 500.0f * scale;
        aheadAng[0] = 0;
        aheadAng[1] = (u16) (em->rotY + turn);
        aheadAng[2] = 0;
        Mtx_FromAnglesXYZ(aheadAng, aheadMtx);
        Vec_Rotate(aheadVec, aheadOfs, aheadMtx);
        target[0] = em->posX + aheadVec[0];
        target[1] = em->posY + aheadVec[1];
        target[2] = em->posZ + aheadVec[2];
        break;
    case 7:
        found = 0;
        if (gAreaObjCount == 0) {
            goto fallback;
        }
        em->actionGoal[1] = em->curGoalArg;
        em->actionGoal[2] = em->curGoalNo;
        for (i = 0, w = gAreaObjList; i < 32; i++, w++) {
            obj = *w;
            if (obj != NULL && obj->goalArg == em->curGoalArg && obj->goalNo == em->curGoalNo) {
                found = 1;
                target[0] = obj->pos[0];
                target[1] = obj->pos[1];
                target[2] = obj->pos[2];
                break;
            }
        }
        if (found) {
            break;
        }
    fallback:
        pts = Em_AreaEntryValue(em, em->tables[0]);
        goto first;
    case 8:
        target[0] = em->objectPos[0];
        target[1] = em->objectPos[1];
        target[2] = em->objectPos[2];
        break;
    case 9:
        set = (EmRouteSet *) Em_FindAreaEntry(em, em->tables[6], em->area);
        if (set == NULL) {
            pts = Em_AreaEntryValue(em, em->tables[0]);
            goto first;
        }
        routeList = set->routes;
        if (routeList == NULL) {
            pts = Em_AreaEntryValue(em, em->tables[0]);
            goto first;
        }
        if (em->curGoalArg >= set->n) {
            pts = Em_AreaEntryValue(em, em->tables[0]);
            goto first;
        }
        routePts = routeList[em->curGoalArg].pts;
        if (routePts == NULL) {
            pts = Em_AreaEntryValue(em, em->tables[0]);
            goto first;
        }
        if (em->curGoalNo >= routeList->n) {
            em->curGoalNo = routeList->n - 1;
        }
        target[0] = routePts[em->curGoalNo].pos[0];
        target[1] = routePts[em->curGoalNo].pos[1];
        target[2] = routePts[em->curGoalNo].pos[2];
        em->routeNo = em->curGoalArg;
        em->routePoint = em->curGoalNo;
        break;
    case 11:
        em->targetNo = em->curGoalArg;
        em->target = &gPlayers[em->targetNo];
        k = *(u16 *) ((u8 *) em->target + 1634);
        point = Em_AreaEntryValue(em, em->tables[0]) + k * 3;
        k = Math_AngleXZ(point, &em->posX);
        sideAng[0] = 0;
        sideAng[1] = k;
        sideAng[2] = 0;
        switch (em->curGoalNo) {
        case 0:
            sideOfs[0] = 0.0f;
            sideOfs[1] = 500.0f;
            sideOfs[2] = 1000.0f;
            break;
        case 1:
            sideOfs[0] = 0.0f;
            sideOfs[1] = 400.0f;
            sideOfs[2] = 800.0f;
            break;
        case 2:
            sideOfs[0] = 0.0f;
            sideOfs[1] = 400.0f;
            sideOfs[2] = 1500.0f;
            break;
        case 3:
            sideOfs[0] = 0.0f;
            sideOfs[1] = 600.0f;
            sideOfs[2] = 600.0f;
            break;
        }
        Mtx_FromAnglesYXZ(sideAng, sideMtx);
        Vec_Rotate(sideVec, sideOfs, sideMtx);
        target[0] = point[0] + sideVec[0];
        target[1] = point[1] + sideVec[1];
        target[2] = point[2] + sideVec[2];
        break;
    case 12:
        if (em->curGoalArg == 0) {
            em->trapTimer = em->curGoalNo * 4;
        }
        break;
    case 13:
        switch (em->curGoalArg) {
        case 0:
            if (em->curGoalNo == -1) {
                em->targetNo = -1;
                em->target = NULL;
            } else {
                em->targetNo = em->curGoalNo;
                em->target = &gEms[em->curGoalNo];
                Em_GetTargetPos(em, em->targetNo, target);
                em->scriptPlayer = em->curGoalNo;
            }
            break;
        }
        break;
    }
    em->goalKind = em->curGoalKind;
    em->goalArg = em->curGoalArg;
    em->goalNo = em->curGoalNo;
}

/*
 * Marks the players in sight (seen) noticed with a fresh noticeTimer. The
 * other noticed players' timers count down, and each player is forgotten at
 * 0. Returns the noticed bits.
 */
u8 Em_UpdateNoticed(Em *em)
{
    s32 i;

    for (i = 0; i < gScene.playerCount; i++) {
        if (em->seen & (1 << i)) {
            em->noticed |= (u8) (1 << i);
            em->noticeTimer[i] = Em_NoticeTime(em->type);
        } else if (em->noticed & (1 << i)) {
            if (--em->noticeTimer[i] <= 0) {
                em->noticed &= (u8) ~(1 << i);
            }
        }
    }
    return em->noticed;
}

/*
 * Each frame, for each active player in the monster's area that has not
 * fainted, it measures playerDist, playerDistXZ and playerAngle (flat holds
 * both positions at height 0). Within the kind's sense reach (sightTable
 * value 7), it sets the player's sensedPlayers bit, and within gEmSenseLimits
 * for the player's sense sector (Em_AngleSector) it marks the player noticed.
 * The first reaction range (0 to 3, negative ranges skipped) that holds the
 * player gives reaction 3 to 6 and adds alert buildup. Otherwise it clears
 * the player's sensedPlayers bit and alertBuildup.
 */
void Em_SensePlayers(Em *em)
{
    EmSenseView *sense = (EmSenseView *) em;
    f32 playerPos[3];
    s32 i;
    f32 *lim;
    s32 bit;
    s8 sector;
    f32 flat[2][3];
    f32 *to = flat[1];
    EmReaction *react;

    for (i = 0; i < 4; i++) {
        if (gPlayers[i].active != 0) {
            if (i < gScene.playerCount) {
                if (Obj_SameArea(em, &gPlayers[i]) != 0 && gPlayers[i].stateGroup != PL_GROUP_FAINT) {
                    lim = gEmSenseLimits[em->type];
                    Em_GetTargetPos(em, i, playerPos);
                    sense->dist[i] = Vec_Distance(&em->posX, playerPos);
                    Vec_Set(flat[0], em->posX, 0.0f, em->posZ);
                    Vec_Set(to, playerPos[0], 0.0f, playerPos[2]);
                    sense->distXZ[i] = Vec_Distance(flat[0], to);
                    sense->dir[i] = Math_AngleXZ(flat[0], to);
                    if (sense->dist[i] <= ((f32 *) em->sightTable)[7]) {
                        bit = 1 << i;
                        em->sensedPlayers |= bit;
                        sector = Em_AngleSector(em, i);
                        if (sense->dist[i] <= lim[sector]) {
                            em->noticed |= bit;
                            sense->noticeTimer[i] = Em_NoticeTime(em->type);
                        }
                        if (!(sense->range[0] < sense->dist[i]) && !(sense->range[0] < 0.0f)) {
                            react = Em_GetReaction(em, 3, i);
                            Em_AddNotice(em, react->noticeAdd, react->noticeMax, i);
                            Em_AddAlertBuildup(em, i, 0, sector);
                        } else if (!(sense->range[1] < sense->dist[i]) && !(sense->range[1] < 0.0f)) {
                            react = Em_GetReaction(em, 4, i);
                            Em_AddNotice(em, react->noticeAdd, react->noticeMax, i);
                            Em_AddAlertBuildup(em, i, 1, sector);
                        } else if (!(sense->range[2] < sense->dist[i]) && !(sense->range[2] < 0.0f)) {
                            react = Em_GetReaction(em, 5, i);
                            Em_AddNotice(em, react->noticeAdd, react->noticeMax, i);
                            Em_AddAlertBuildup(em, i, 2, sector);
                        } else if (!(sense->range[3] < sense->dist[i]) && !(sense->range[3] < 0.0f)) {
                            react = Em_GetReaction(em, 6, i);
                            Em_AddNotice(em, react->noticeAdd, react->noticeMax, i);
                            Em_AddAlertBuildup(em, i, 3, sector);
                        }
                    } else {
                        em->sensedPlayers &= ~(1 << i);
                        sense->alertBuildup[i] = 0;
                    }
                } else {
                    em->sensedPlayers &= ~(1 << i);
                    sense->alertBuildup[i] = 0;
                }
            } else {
                em->sensedPlayers &= ~(1 << i);
                sense->alertBuildup[i] = 0;
            }
        }
    }
}

/*
 * For each sensed player whose alertBuildup reached the kind's limit
 * (gEmAlertBuildupLimits row alertLimitRow), out of combat, with noReact
 * clear and no trigger script of rank EM_TRIGGER_PART_BUILDUP or higher running: sets
 * scriptPending, raises EM_TRIGGER_PART_BUILDUP, sets the player's
 * alertedPlayers bit and clears its alertBuildup.
 */
void Em_CheckPartBuildup(Em *em)
{
    s32 i;
    s32 lim = gEmPartBuildupLimits[em->type][em->alertLimitRow];

    for (i = 0; i < 4; i++) {
        if (Em_ScriptFlagsFrom(em, EM_TRIGGER_PART_BUILDUP) == 0 && em->combat != 1 && em->noReact == 0 && em->alertBuildup[i] >= lim
            && (em->sensedPlayers & (1 << i))) {
            em->scriptPending = 1;
            em->triggers |= 0x10;
            em->alertedPlayers |= 1 << i;
            em->alertBuildup[i] = 0;
        }
    }
}

/*
 * Monster helpers: the per-kind setup dispatch, angles and target
 * positions, the +0x967 priority requests, counters decreased by per-kind
 * rates, checks against sub_main's object tables, and pending placements.
 * See include/em.h.
 */



extern EmReaction *gEmReactions[];
extern EmRate *gEmDecay7ECRates[];
extern u8 gEmCarryReact[];
extern EmObj *gEmSenseObjs[32];
extern s8 gEmSenseObjCount;

void QSync_SendEm(Em *em, s32 a, s32 b); /* unit's own declaration: the definition (gm_sub qnet_sync.c) takes u8 kind, u8 arg; u8 here changes Em_SyncState */
void Em_ChangeStateRouted(Em *em, s32 a, s32 b, s32 c);
void EmDrome_Setup(Em *em);
void EmGaruga_Setup(Em *em);
void EmHermitaur_Setup(Em *em);
void EmGravios_Setup(Em *em);
void EmKutku_Setup(Em *em);
void EmPlesioth_Setup(Em *em);
void EmDiablos_Setup(Em *em);
void EmKhezu_Setup(Em *em);
void EmLaoshan_Setup(Em *em);
void EmCephadrome_Setup(Em *em);
void EmRathalos_Setup(Em *em);
void EmBlangonga_Setup(Em *em);
void EmShenGaoren_Setup(Em *em);
void EmKushala_Setup(Em *em);
void EmYamaTsukami_Setup(Em *em);
void EmChameleos_Setup(Em *em);
void EmBulldrome_Setup(Em *em);

/*
 * Makes request `request` pending with priority `prio` unless a higher-
 * priority request is already pending (requestPrio). Sets requestPrio,
 * requested, requestId and scriptRestart.
 */
void Em_SetRequest(Em *em, u8 request, u8 prio)
{
    if (em->requestPrio <= prio) {
        em->requestPrio = prio;
        em->requested = 1;
        em->requestId = request;
        em->scriptRestart = 1;
    }
}

/*
 * Returns 1 when scriptFlags has a bit at or above the lowest set bit of
 * `mask` (a trigger script of that rank or higher is running), else 0 (also
 * for mask 0).
 */
s32 Em_ScriptFlagsFrom(Em *em, u8 mask)
{
    s32 i;
    u8 bits;

    for (i = 0; i < 8; i++) {
        if (mask & (1 << i)) {
            break;
        }
    }
    if (i > 7) {
        return 0;
    }
    bits = 0;
    for (; i < 8; i++) {
        bits |= 1 << i;
    }
    return (em->scriptFlags & bits) != 0;
}

/*
 * Sets the monster up for its kind: Em_ApplyAreaValues, no script loop
 * (loopPoint and loopCount -1), then the family's Setup. The Rathalos, Khezu,
 * Kut-Ku, Garuga, Kushala and Chameleos families also take the kind's setup
 * cut off need1Gauge and need2Gauge. Daimyo Hermitaur takes it off need1Gauge
 * only. The host's restart message runs it again.
 */
void Em_TypeSetup(Em *em)
{
    Em_ApplyAreaValues(em);
    em->loopPoint = -1;
    em->loopCount = -1;
    switch (em->type) {
    case EM_KIND_RATHIAN:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_RATHALOS:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_AZURE_RATHALOS:
        EmRathalos_Setup(em);
        Em_Reduce7F0(em);
        Em_Reduce7EC(em);
        break;
    case EM_KIND_LAO_SHAN_LUNG:
    case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        EmLaoshan_Setup(em);
        break;
    case EM_KIND_CEPHADROME:
    case EM_KIND_CEPHALOS:
        EmCephadrome_Setup(em);
        break;
    case EM_KIND_DIABLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_WHITE_MONOBLOS:
        EmDiablos_Setup(em);
        break;
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
        EmKhezu_Setup(em);
        Em_Reduce7F0(em);
        Em_Reduce7EC(em);
        break;
    case EM_KIND_GRAVIOS:
    case EM_KIND_BLACK_GRAVIOS:
    case EM_KIND_BASARIOS:
        EmGravios_Setup(em);
        break;
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_GYPCEROS:
    case EM_KIND_PURPLE_GYPCEROS:
        EmKutku_Setup(em);
        Em_Reduce7F0(em);
        Em_Reduce7EC(em);
        break;
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
        EmPlesioth_Setup(em);
        break;
    case EM_KIND_VELOCIDROME:
    case EM_KIND_GENDROME:
    case EM_KIND_IODROME:
        EmDrome_Setup(em);
        break;
    case EM_KIND_YIAN_GARUGA:
        EmGaruga_Setup(em);
        Em_Reduce7F0(em);
        Em_Reduce7EC(em);
        break;
    case EM_KIND_DAIMYO_HERMITAUR:
        Em_Reduce7F0(em);
    case EM_KIND_SHOGUN_CEANATAUR:
        EmHermitaur_Setup(em);
        break;
    case EM_KIND_BLANGONGA:
    case EM_KIND_CONGALALA:
    case EM_KIND_RAJANG:
        EmBlangonga_Setup(em);
        break;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        EmKushala_Setup(em);
        Em_Reduce7F0(em);
        Em_Reduce7EC(em);
        break;
    case EM_KIND_SHEN_GAOREN:
        EmShenGaoren_Setup(em);
        break;
    case EM_KIND_YAMA_TSUKAMI:
    case EM_KIND_YAMA_TSUKAMI_B:
        EmYamaTsukami_Setup(em);
        break;
    case EM_KIND_CHAMELEOS:
        EmChameleos_Setup(em);
        Em_Reduce7F0(em);
        Em_Reduce7EC(em);
        break;
    case EM_KIND_BULLDROME:
        EmBulldrome_Setup(em);
        break;
    }
}

/*
 * Returns the heading from `from` to `to` in the XZ plane (ANGLE_TURN to a turn).
 */
u16 Math_AngleXZ(f32 *from, f32 *to)
{
    return Math_Heading(from[0], from[2], to[0], to[2]) - 0x4000;
}

/*
 * Writes to *out the pitch angle (ANGLE_TURN to a turn) from the monster to the
 * target player's position. `rise` is added to the target's height and
 * `reach` to the ground distance. Writes nothing when there is no target.
 */
void Em_GetPitchToTarget(Em *em, s32 *out, f32 rise, f32 reach)
{
    f32 target[4];
    f32 dist;

    if (em->targetNo != -1) {
        Em_GetTargetPos(em, em->targetNo, target);
        dist = Vec_DistanceXZ(&em->posX, target);
        *out = (u16)(ANGLE_TURN - Math_Heading(Math_Absf(dist + reach), em->posY, 0.0f, target[1] + rise));
    }
}

/* Same as Em_GetPitchToTarget, toward the goal point (em->goal). */
void Em_GetPitchToGoal(Em *em, s32 *out, f32 rise, f32 reach)
{
    f32 dist = Vec_DistanceXZ(&em->posX, em->goal);

    *out = (u16)(ANGLE_TURN - Math_Heading(Math_Absf(dist + reach), em->posY,
                                       0.0f, em->goal[1] + rise));
}

/*
 * Writes to `out` the position of target `idx`: monster idx when curGoalKind
 * is 13; otherwise player idx, using its targetPos online for a large monster
 * and its pos in every other case. Returns 1. The tests of idx against -1
 * cannot hold on a u8.
 */
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out)
{
    Em *target;
    Player *player;

    if (em->curGoalKind == 13) {
        if (idx == -1) {
            return 0;
        }
        target = &gEms[idx];
        out[0] = target->posX;
        out[1] = target->posY;
        out[2] = target->posZ;
    } else if (Game_IsOnline() == 1) {
        if (idx == -1) {
            return 0;
        }
        player = &gPlayers[idx];
        if (em->large != 0) {
            out[0] = player->targetPos[0];
            out[1] = player->targetPos[1];
            out[2] = player->targetPos[2];
        } else {
            out[0] = player->pos[0];
            out[1] = player->pos[1];
            out[2] = player->pos[2];
        }
    } else {
        if (idx == -1) {
            return 0;
        }
        player = &gPlayers[idx];
        out[0] = player->pos[0];
        out[1] = player->pos[1];
        out[2] = player->pos[2];
    }
    return 1;
}

/*
 * Picks the next request by priority. The low-hp request comes first (128),
 * then need4 (112, cleared when used), need1 (96), need2 (80) and, out
 * of combat, need3 (64). Each is made only when requestPrio is lower.
 * Nothing happens while a 128 request is pending, while an EM_TRIGGER_PLAYER_AREA
 * script runs, or in combat in an area of the kind's gEmNoRequestAreas list.
 */
void Em_PickRequest(Em *em)
{
    s16 *ids = gEmRequestIds[em->type];

    if (em->requestPrio >= 128) {
        return;
    }
    if (Em_ScriptFlagsFrom(em, EM_TRIGGER_PLAYER_AREA)) {
        return;
    }
    if (ids != NULL && em->combat == 1) {
        while (*ids != -1) {
            if (*ids >= 10000) {
                if (*ids - 10000 == em->area) {
                    return;
                }
            } else if (Area_DayId(*ids) == Area_DayId(em->area)) {
                return;
            }
            ids++;
        }
    }
    if (em->lowHpNeed != 0) {
        em->requestPrio = 128;
        Em_SetRequest(em, gEmKindInfo[em->type].lowHpRequest, 128);
        return;
    }
    if (em->requestPrio < 112) {
        if (em->need4 != 0) {
            em->need4 = 0;
            em->requestPrio = 112;
            Em_SetRequest(em, gEmKindInfo[em->type].need4Request, 112);
            return;
        }
        if (em->requestPrio < 96) {
            if (em->need1 != 0) {
                em->requestPrio = 96;
                Em_SetRequest(em, gEmKindInfo[em->type].need1Request, 96);
                return;
            }
            if (em->requestPrio < 80) {
                if (em->need2 != 0) {
                    em->requestPrio = 80;
                    Em_SetRequest(em, gEmKindInfo[em->type].need2Request, 80);
                    return;
                }
                if (em->requestPrio < 64 && em->need3 != 0 && em->combat == 0) {
                    em->requestPrio = 64;
                    Em_SetRequest(em, gEmKindInfo[em->type].need3Request, 64);
                }
            }
        }
    }
}

/*
 * Sets lowHpNeed when hp falls under hpMax * `ratio`. It clears lowHpNeed for
 * the quest's special monster, for kinds without a low-hp request and from
 * EM_GROUP_DEFEAT on. It leaves lowHpNeed alone while that request is
 * pending, while an EM_TRIGGER_PLAYER_AREA script runs, or in combat in a listed area.
 */
void Em_CheckLowHp(Em *em, f32 ratio)
{
    s16 *ids = gEmRequestIds[em->type];

    if ((u8)Quest_FindSpecialMonster(em->type) != 255) {
        em->lowHpNeed = 0;
        return;
    }
    if (gEmKindInfo[em->type].lowHpRequest == 0) {
        em->lowHpNeed = 0;
        return;
    }
    if (em->stateGroup >= 5) {
        em->lowHpNeed = 0;
        return;
    }
    if (em->hp >= (s16)(em->hpMax * ratio)) {
        return;
    }
    if (em->requested == 1 && em->requestId == gEmKindInfo[em->type].lowHpRequest) {
        return;
    }
    if (Em_ScriptFlagsFrom(em, EM_TRIGGER_PLAYER_AREA)) {
        return;
    }
    if (ids != NULL && em->combat == 1) {
        while (*ids != -1) {
            if (*ids >= 10000) {
                if (*ids - 10000 == em->area) {
                    return;
                }
            } else if (Area_DayId(*ids) == Area_DayId(em->area)) {
                return;
            }
            ids++;
        }
    }
    em->lowHpNeed = 1;
}

/*
 * For the kinds of gEmCarryReact that have a need-4 request: when that
 * request is pending and no player carries anything any more, it cancels it
 * (requested, requestId and requestPrio cleared). Otherwise, while some
 * player carries something, it sets need4.
 */
void Em_CheckCarryRequest(Em *em)
{
    u8 request;
    s8 i;
    s32 ok;

    if (gEmCarryReact[em->type] == 0) {
        return;
    }
    request = gEmKindInfo[em->type].need4Request;
    if (request == 0) {
        return;
    }
    if (em->requested == 1 && em->requestId == request) {
        for (i = 0; i < gScene.playerCount; i++) {
            ok = !(gPlayers[i].carry & PL_CARRY_HOLD_MASK);
            if (!ok) {
                return;
            }
        }
        em->requested = 0;
        em->requestId = 0;
        em->requestPrio = 0;
    } else {
        for (i = 0; i < gScene.playerCount; i++) {
            ok = !(gPlayers[i].carry & PL_CARRY_HOLD_MASK);
            if (!ok) {
                em->need4 = 1;
                return;
            }
        }
    }
}

/*
 * Lowers need1Gauge by the kind's calm or combat drain (never below 0). For
 * kinds with a need-1 request, it sets need1State and need1 when the gauge
 * first reaches 0.
 */
void Em_DecayNeed1(Em *em)
{
    EmRate *rate = gEmRateTbl[em->type];
    s32 drain;

    if (em->combat == 0) {
        drain = rate->calmDrain;
    } else {
        drain = rate->combatDrain;
    }
    em->need1Gauge -= drain;
    if (em->need1Gauge < 0) {
        em->need1Gauge = 0;
    }
    if (gEmKindInfo[em->type].need1Request != 0) {
        switch (em->need1State) {
        case 0:
            if (em->need1Gauge == 0) {
                em->need1State = 1;
                em->need1 = 1;
            }
            break;
        }
    }
}

/*
 * Lowers need1Gauge by the kind's calm or combat drain (never below 0),
 * without raising need1.
 */
void Em_DecayNeed1Only(Em *em)
{
    EmRate *rate = gEmRateTbl[em->type];
    s32 drain;

    if (em->combat == 0) {
        drain = rate->calmDrain;
    } else {
        drain = rate->combatDrain;
    }
    em->need1Gauge -= drain;
    if (em->need1Gauge < 0) {
        em->need1Gauge = 0;
    }
}

/*
 * Lowers need2Gauge by the kind's calm or combat drain (never below 0). For
 * kinds with a need-2 request, it sets need2 and need2State when the gauge
 * first reaches 0.
 */
void Em_DecayNeed2(Em *em)
{
    EmRate *rate = gEmDecay7ECRates[em->type];
    s32 drain;

    if (em->combat == 0) {
        drain = rate->calmDrain;
    } else {
        drain = rate->combatDrain;
    }
    em->need2Gauge -= drain;
    if (em->need2Gauge < 0) {
        em->need2Gauge = 0;
    }
    if (gEmKindInfo[em->type].need2Request != 0) {
        switch (em->need2State) {
        case 0:
            if (em->need2Gauge == 0) {
                em->need2 = 1;
                em->need2State = 1;
            }
            break;
        }
    }
}

/*
 * Lowers need3Gauge by the kind's calm or combat drain (never below 0). For
 * kinds with a need-3 request, it sets need3State and need3 when the gauge
 * first reaches 0.
 */
void Em_DecayNeed3(Em *em)
{
    EmRate *rate = gEmRateStatus.rate[em->type];
    s32 drain;

    if (em->combat == 0) {
        drain = rate->seekLevel;
    } else {
        drain = rate->calmDrain;
    }
    em->need3Gauge -= drain;
    if (em->need3Gauge < 0) {
        em->need3Gauge = 0;
    }
    if (gEmKindInfo[em->type].need3Request != 0) {
        switch (em->need3State) {
        case 0:
            if (em->need3Gauge == 0) {
                em->need3State = 1;
                em->need3 = 1;
            }
            break;
        }
    }
}

/*
 * Takes the kind's setup cut off need1Gauge, leaving 0 when the gauge does
 * not exceed the cut.
 */
void Em_Reduce7F0(Em *em)
{
    if (em->need1Gauge > gEmRateTbl[em->type]->setupCut) {
        em->need1Gauge -= gEmRateTbl[em->type]->setupCut;
    } else {
        em->need1Gauge = 0;
    }
}

/*
 * Takes the kind's setup cut off need2Gauge, leaving 0 when the gauge does
 * not exceed the cut.
 */
void Em_Reduce7EC(Em *em)
{
    if (em->need2Gauge > gEmDecay7ECRates[em->type]->setupCut) {
        em->need2Gauge -= gEmDecay7ECRates[em->type]->setupCut;
    } else {
        em->need2Gauge = 0;
    }
}

/*
 * Returns the first of the four sense sectors (0 to 3, each sectorHalfAngle
 * wide on each side of the heading) that holds player `player`'s direction
 * (playerAngle), or 4 when none does.
 */
s8 Em_AngleSector(Em *em, s32 player)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        if ((u16)(em->playerAngle[player] + em->sectorHalfAngle[i] - em->rotY) <= em->sectorHalfAngle[i] * 2) {
            return i;
        }
    }
    return i;
}

/*
 * On the host only. A large monster records the state for the other consoles
 * (syncKind = group, syncArg = state, the cur goal values, a new syncRand)
 * and sends it. For EM_GROUP_REACT and EM_GROUP_DEFEAT it sends
 * QSYNC_EMMSG_SYNC, otherwise QSYNC_EMMSG_STATE, with flag `flag`. A small
 * monster draws stateRand and sets the state at once.
 */
void Em_SyncState(Em *em, s32 group, s32 state, s32 flag)
{
    if (em->remote != 0) {
        return;
    }
    if (em->large != 0) {
        em->syncKind = group;
        em->syncArg = state;
        em->syncGoalKind = em->curGoalKind;
        em->syncGoalArg = em->curGoalArg;
        em->syncGoalNo = em->curGoalNo;
        em->syncRand = Rand_Next(0);
        if ((u16) group == 4 || (u16) group == 5) {
            QSync_SendEm(em, 3, flag);
        } else {
            QSync_SendEm(em, 1, flag);
        }
    } else {
        em->stateRand = Rand_Next(0);
        Em_SetState(em, group, state);
    }
}

/*
 * Returns 1 when the monster stands inside a sight-block zone of its area
 * whose kind has bit 1 (likely the small zones), else 0. The Plesioth and
 * Anteka families make a request then.
 */
s32 Em_InBlockArea1(Em *em)
{
    s32 i;
    EmObj *obj;

    for (i = 0; i < 32; i++) {
        obj = gSightBlockObjs[i];
        if (obj != NULL && obj->area == em->area && (obj->kind & 1)
            && Vec_Distance(obj->pos, &em->posX) <= obj->radius) {
            return 1;
        }
    }
    return 0;
}

/*
 * Returns 1 when the monster, on the ground or in stance 1 and with no low-hp
 * request pending, stands inside a sight-block zone of its area whose kind
 * has bit 2 (likely the large zones), else 0. GmEm_Update raises
 * EM_TRIGGER_BLOCK_AREA on it.
 */
s32 Em_InBlockArea2(Em *em)
{
    s32 i;
    EmObj *obj;

    for (i = 0; i < 32; i++) {
        obj = gSightBlockObjs[i];
        if (obj != NULL && obj->area == em->area && (obj->kind & 2)
            && (em->stance == 0 || em->stance == 1) && em->requestPrio < 128
            && Vec_Distance(obj->pos, &em->posX) <= obj->radius) {
            return 1;
        }
    }
    return 0;
}

/*
 * When object `obj` (a flash; FlashObj_Add) appears, every active monster in
 * the area of the object's player (obj->goalArg) that has not fainted marks
 * that player noticed and applies reaction 7. Returns 1 when the player
 * stands within the object's radius (checked for each such monster).
 */
u8 EmObj_AlertMonsters(EmObj *obj)
{
    f32 pos[4];
    s32 i;
    s32 hit = 0;
    Em *em = gEms;
    Player *player;
    EmReaction *react;

    for (i = 0; i < EM_COUNT; i++, em++) {
        if (em->active == 0) {
            continue;
        }
        player = &gPlayers[obj->goalArg];
        if (Obj_SameArea(em, player) && player->stateGroup != PL_GROUP_FAINT) {
            em->noticed |= 1 << obj->goalArg;
            em->noticeTimer[obj->goalArg] = Em_NoticeTime(em->type);
            react = Em_GetReaction(em, 7, obj->goalArg);
            Em_AddNotice(em, react->noticeAdd, react->noticeMax, obj->goalArg);
            Em_GetTargetPos(em, obj->goalArg, pos);
            if (Vec_Distance(obj->pos, pos) <= obj->radius) {
                hit = 1;
            }
        }
    }
    return hit;
}

/*
 * Returns the kind's reaction row for reaction kind `kind` and player
 * `player`: 0 seen, 1 hit, 2 carrying, 3 to 6 the sense ranges, 7 a flash, 8
 * player flag 0x10000. Skill 20 shifts kind 1 to 9 and kinds 3-6 to 10-13.
 * Otherwise a silencer moves kind 1 to 18, and skill 19 with action rank
 * 0 moves kinds 3-6 to 14-17.
 */
EmReaction *Em_GetReaction(Em *em, u8 kind, u8 player)
{
    Player *pl = &gPlayers[player];
    EmReaction *table = gEmReactions[em->type];

    switch (kind) {
    case 1:
        if (Player_HasSkill(pl, 20) == 1) {
            kind += 8;
        } else if (LightBowgun_HasSilencer(pl) == 1) {
            kind = 18;
        }
        break;
    case 3:
    case 4:
    case 5:
    case 6:
        if (Player_HasSkill(pl, 20) == 1) {
            kind += 7;
        } else if (Player_HasSkill(pl, 19) == 1 && (u8)Em_PlayerActionKind(player) == 0) {
            kind += 11;
        }
        break;
    }
    return &table[kind];
}

/*
 * Applies each pending placement (spawns). Once the monster runs, outside
 * EM_GROUP_REACT and EM_GROUP_DEFEAT, and when the record's arg is 0, it
 * takes the record's position, rotation, cur goal values, area, timer,
 * stateRand and state, then changes to that state (Em_ChangeStateRouted).
 * Every pending flag is cleared either way.
 */
void Em_ApplyPendingSpawn(Em *em)
{
    s32 i;
    u8 group;
    u8 newState;

    for (i = 0; i < 2; i++) {
        if (em->spawns[i].pending != 0) {
            em->spawns[i].pending = 0;
            if (em->taskPhase > 0 && !((u32)(em->stateGroup - 4) <= 1) && em->spawns[i].arg == 0) {
                em->posX = em->spawns[i].pos[0];
                em->posY = em->spawns[i].pos[1];
                em->posZ = em->spawns[i].pos[2];
                em->rotX = em->spawns[i].rot[0];
                em->rotY = em->spawns[i].rot[1];
                em->rotZ = em->spawns[i].rot[2];
                group = em->spawns[i].stateGroup;
                newState = em->spawns[i].state;
                em->curGoalKind = em->spawns[i].goalKind;
                em->curGoalArg = em->spawns[i].goalArg;
                em->curGoalNo = em->spawns[i].goalNo;
                em->area = em->spawns[i].area;
                em->timer = em->spawns[i].timer;
                em->stateRand = em->spawns[i].rand;
                em->stateGroup = group;
                em->state = newState;
                Em_ChangeStateRouted(em, group, newState, 0);
            }
        }
    }
}


/*
 * Online, for a large monster with more than one player: counts hostTimer
 * down and hands the monster over when its console's player is gone. On a
 * remote console whose host slot is empty (IN_QUEST_NO_PLAYER), the first
 * player still in the quest takes the monster over when it is the local one
 * (QSYNC_EMMSG_HANDOVER). With fewer than two players left, this console
 * takes it. On the host, when the local player is neither in the monster's
 * area nor arriving there and another quest player is in it, it hands the
 * monster to the next such player after the local one: hostNo, hostTimer 60,
 * hostChanged, remote, then Em_ApplyPendingSpawn.
 *
 * j holds the first loop's found flag, the counts are copied between locals,
 * the second part's bound is read as an s32 without a mask, and the area test
 * goes through pl: each keeps the compiler's register choices and load order
 * the same as the game's code.
 */
void Em_MigrateHost(Em *em)
{
    s8 areaMask = 0;
    s8 i;
    s8 k;
    s8 j;
    s8 inQuestMask;
    s8 inQuestCount;
    s8 a;
    s32 n;
    s32 m;
    Player *pl;

    if (em->large == 0 || gScene.playerCount == 1) {
        return;
    }
    if (em->hostTimer != 0) {
        em->hostTimer--;
    }
    if (em->remote > 0) {
        if (gScene.inQuest[em->hostNo] == 255) {
            n = gScene.playerCount;
            for (i = 0, a = 0, inQuestMask = 0; i < n; i++) {
                if (gScene.inQuest[i] == 1) {
                    a++;
                    inQuestMask |= 1 << i;
                }
            }
            m = a;
            if (m < 2) {
                em->remote = 0;
                return;
            }
            j = 0;
            for (k = 0; k < n; k++) {
                j %= n;
                if (inQuestMask & (1 << j)) {
                    break;
                }
                j++;
            }
            if (j == gScene.localPlayer) {
                em->remote = 0;
                em->hostTimer = 60;
                em->hostNo = j;
                QSync_SendEm(em, 4, 0);
            }
        }
        return;
    }
    j = 0;
    for (i = 0; i < gScene.playerCount; i++) {
        if (gPlayers[i].areaMoveState != PL_AREA_MOVE_NONE && i == gScene.localPlayer) {
            pl = &gPlayers[i];
            if (pl->nextArea == em->area && gScene.inQuest[i] == 1) {
                j = 1;
            }
        } else if (Obj_SameArea(em, &gPlayers[i]) && gPlayers[i].active) {
            if (i == gScene.localPlayer && gScene.inQuest[i] == 1) {
                return;
            }
            areaMask |= 1 << i;
        }
    }
    if (j != 0 && areaMask == 0) {
        return;
    }
    if (!areaMask) {
        return;
    }
    m = (s32) gScene.playerCount;
    for (k = 0, inQuestCount = 0, inQuestMask = 0; k < m; k++) {
        if (gScene.inQuest[k] == 1) {
            inQuestCount++;
            inQuestMask |= 1 << k;
        }
    }
    n = inQuestCount;
    if (n < 2) {
        em->remote = 0;
        return;
    }
    j = gScene.localPlayer + 1;
    for (k = 0; k < m; k++) {
        j %= m;
        if (inQuestMask & (1 << j)) {
            break;
        }
        j++;
    }
    for (a = 0; a < n; a++) {
        if ((areaMask & (1 << j)) && gScene.inQuest[j] == 1) {
            em->hostTimer = 60;
            em->hostNo = j;
            QSync_SendEm(em, 4, 0);
            em->hostChanged = 1;
            em->remote = 1;
            Em_ApplyPendingSpawn(em);
            return;
        }
        j++;
        for (k = 0; k < m; k++) {
            j %= m;
            if (inQuestMask & (1 << j)) {
                break;
            }
            j++;
        }
    }
}

/*
 * Unless senseDelay runs, finds the first object of gEmSenseObjs (other than
 * `self`) in the monster's area whose radius holds the monster and whose kind
 * this monster reacts to: kind 0 the Velociprey, Giaprey and Velocidrome, 1
 * kind 9, 2 the Apceros-family kinds, 3 and 4 Blangonga and Blango, and other
 * object kinds every monster. Copies its position to objectPos and returns 1,
 * else 0.
 */
u8 Em_SenseObjects(Em *em, EmObj *self)
{
    s8 i;
    EmObj *obj;

    if (gEmSenseObjCount == 0) {
        return 0;
    }
    if (em->senseDelay != 0) {
        return 0;
    }
    for (i = 0; i < 32; i++) {
        obj = gEmSenseObjs[i];
        if (obj == NULL || obj == self || obj->area != em->area) {
            continue;
        }
        switch (obj->kind) {
        case 0:
            if (em->type != EM_KIND_VELOCIPREY && em->type != EM_KIND_GIAPREY && em->type != EM_KIND_VELOCIDROME) {
                continue;
            }
            break;
        case 1:
            if (em->type != EM_KIND_FELYNE) {
                continue;
            }
            break;
        case 2:
            if (em->type != EM_KIND_HERBIVORE_0 && em->type != EM_KIND_POPO && em->type != EM_KIND_APCEROS) {
                continue;
            }
            break;
        case 3:
            if (em->type != EM_KIND_BLANGONGA && em->type != EM_KIND_BLANGO) {
                continue;
            }
            break;
        case 4:
            if (em->type != EM_KIND_BLANGONGA && em->type != EM_KIND_BLANGO) {
                continue;
            }
            break;
        }
        if (!(obj->radius < Vec_Distance(obj->pos, &em->posX))) {
            Vec_Set(em->objectPos, obj->pos[0], obj->pos[1], obj->pos[2]);
            return 1;
        }
    }
    return 0;
}

/* The file's data. */
extern void *gEmKindTables_68[];
extern u32 D_008F5920[];
extern s32 D_008F5930[4];
extern s32 D_008F5940[5][4];
extern s32 D_008F5990[5][4];
extern s32 D_008F59E0[5][4];
extern s32 D_008F5A30[5][4];
EmReactionDists gEmReactionDists_0[2] = {
#include "game/008BAC10.inc"
};
EmReactionDists gEmReactionDists_2[2] = {
#include "game/008BAC50.inc"
};
EmReactionDists gEmReactionDists_3[1] = {
#include "game/008BAC90.inc"
};
EmReactionDists gEmReactionDists_4[1] = {
#include "game/008BACB0.inc"
};
EmReactionDists gEmReactionDists_8[1] = {
#include "game/008BACD0.inc"
};
EmReactionDists gEmReactionDists_9[1] = {
#include "game/008BACF0.inc"
};
EmReactionDists gEmReactionDists_12[1] = {
#include "game/008BAD10.inc"
};
EmReactionDists gEmReactionDists_14[2] = {
#include "game/008BAD30.inc"
};
EmReactionDists gEmReactionDists_15[2] = {
#include "game/008BAD70.inc"
};
EmReactionDists gEmReactionDists_13[1] = {
#include "game/008BADB0.inc"
};
EmReactionDists gEmReactionDists_17[2] = {
#include "game/008BADD0.inc"
};
EmReactionDists gEmReactionDists_21[3] = {
#include "game/008BAE10.inc"
};
EmReactionDists gEmReactionDists_40[2] = {
#include "game/008BAE70.inc"
};
EmReactionDists gEmReactionDists_52[2] = {
#include "game/008BAEB0.inc"
};
EmReactionDists gEmReactionDists_53[2] = {
#include "game/008BAEF0.inc"
};
EmReactionDists gEmReactionDists_54[2] = {
#include "game/008BAF30.inc"
};
EmReactionDists gEmReactionDists_57[1] = {
#include "game/008BAF70.inc"
};
EmReactionDists gEmReactionDists_66[1] = {
#include "game/008BAF90.inc"
};
EmReactionDists gEmReactionDists_68[1] = {
#include "game/008BAFB0.inc"
};
EmReactionDists gEmReactionDists_71[2] = {
#include "game/008BAFD0.inc"
};
EmReactionDists *gEmReactionDists[76] = {
    gEmReactionDists_0, gEmReactionDists_0, gEmReactionDists_2, gEmReactionDists_3,
    gEmReactionDists_4, gEmReactionDists_4, gEmReactionDists_0, gEmReactionDists_0,
    gEmReactionDists_8, gEmReactionDists_9, gEmReactionDists_0, gEmReactionDists_0,
    gEmReactionDists_12, gEmReactionDists_13, gEmReactionDists_14, gEmReactionDists_15,
    gEmReactionDists_13, gEmReactionDists_17, gEmReactionDists_0, gEmReactionDists_0,
    gEmReactionDists_0, gEmReactionDists_21, gEmReactionDists_0, gEmReactionDists_9,
    gEmReactionDists_0, gEmReactionDists_12, gEmReactionDists_14, gEmReactionDists_13,
    gEmReactionDists_13, gEmReactionDists_0, gEmReactionDists_13, gEmReactionDists_13,
    gEmReactionDists_0, gEmReactionDists_3, gEmReactionDists_8, gEmReactionDists_13,
    gEmReactionDists_2, gEmReactionDists_0, gEmReactionDists_0, gEmReactionDists_0,
    gEmReactionDists_40, gEmReactionDists_0, gEmReactionDists_0, gEmReactionDists_14,
    gEmReactionDists_14, gEmReactionDists_15, gEmReactionDists_21, gEmReactionDists_17,
    gEmReactionDists_0, gEmReactionDists_0, gEmReactionDists_0, gEmReactionDists_0,
    gEmReactionDists_52, gEmReactionDists_53, gEmReactionDists_54, gEmReactionDists_0,
    gEmReactionDists_0, gEmReactionDists_57, gEmReactionDists_0, gEmReactionDists_0,
    gEmReactionDists_0, gEmReactionDists_0, gEmReactionDists_52, gEmReactionDists_0,
    gEmReactionDists_0, gEmReactionDists_0, gEmReactionDists_66, gEmReactionDists_0,
    gEmReactionDists_68, gEmReactionDists_3, gEmReactionDists_12, gEmReactionDists_71,
    gEmReactionDists_0, gEmReactionDists_66, NULL, NULL,
};
EmReactionAngles gEmReactionAngles_0[2] = {
#include "game/008BB140.inc"
};
EmReactionAngles *gEmReactionAngles[76] = {
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0, gEmReactionAngles_0,
    gEmReactionAngles_0, gEmReactionAngles_0, NULL, NULL,
};
void **gEmKindTables[76] = {
#include "game/008BB280.inc"
};
extern f32 D_00911410[];
extern f32 D_00913B90[];
extern f32 D_00914140[];
extern f32 D_00919780[];
extern f32 D_0091C440[];
f32 *gEmGoalPoints[76] = {
    NULL, NULL, NULL, NULL,
    D_00911410, D_00911410, NULL, NULL,
    NULL, NULL, D_00913B90, NULL,
    D_00914140, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, D_00919780, NULL, NULL,
    NULL, D_00914140, NULL, NULL,
    NULL, D_0091C440, NULL, NULL,
    D_00911410, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, D_00919780, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    D_00911410, NULL, D_00914140, NULL,
    NULL, NULL, NULL, NULL,
};
f32 gEmSenseLimits_1[4] = {
#include "game/008BB4E0.inc"
};
f32 gEmSenseLimits_4[4] = {
#include "game/008BB4F0.inc"
};
f32 gEmSenseLimits_13[4] = {
#include "game/008BB500.inc"
};
f32 gEmSenseLimits_40[4] = {
#include "game/008BB510.inc"
};
f32 *gEmSenseLimits[76] = {
    NULL, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_4, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, (f32 *)D_008F5920, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_13, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_13, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_13,
    gEmSenseLimits_13, (f32 *)D_008F5920, gEmSenseLimits_13, gEmSenseLimits_13,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_13,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_40, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, (f32 *)D_008F5920, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1, gEmSenseLimits_1,
    gEmSenseLimits_1, gEmSenseLimits_1, NULL, NULL,
};
s32 gEmPartBuildupLimits_1[4] = {
#include "game/008BB650.inc"
};
s32 *gEmPartBuildupLimits[76] = {
    NULL, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, gEmPartBuildupLimits_1,
    gEmPartBuildupLimits_1, gEmPartBuildupLimits_1, NULL, NULL,
};
/* The alert build-up tables (Em_AddAlertBuildup), per kind through
 * gEmPlayerActionKinds: [0] the base, per reaction range; [1] to [4] per
 * sense sector 0 to 3, a row per player action rank (Em_PlayerActionKind),
 * per reaction range. gEmPlayerActionKinds_4's lists are in .bss. */
s32 gEmPlayerActionKinds_1_0[4] = {
#include "game/008BB790.inc"
};
s32 gEmPlayerActionKinds_1_1[5][4] = {
#include "game/008BB7A0.inc"
};
s32 gEmPlayerActionKinds_1_2[5][4] = {
#include "game/008BB7F0.inc"
};
s32 gEmPlayerActionKinds_1_3[5][4] = {
#include "game/008BB840.inc"
};
s32 gEmPlayerActionKinds_1_4[5][4] = {
#include "game/008BB890.inc"
};
s32 (*gEmPlayerActionKinds_1[8])[4] = {
    &gEmPlayerActionKinds_1_0, gEmPlayerActionKinds_1_1, gEmPlayerActionKinds_1_2, gEmPlayerActionKinds_1_3,
    gEmPlayerActionKinds_1_4, NULL, NULL, NULL,
};
s32 (*gEmPlayerActionKinds_4[8])[4] = {
    &D_008F5930, D_008F5940, D_008F5990, D_008F59E0,
    D_008F5A30, NULL, NULL, NULL,
};
s32 gEmPlayerActionKinds_22_0[4] = {
#include "game/008BB920.inc"
};
s32 gEmPlayerActionKinds_22_1[5][4] = {
#include "game/008BB930.inc"
};
s32 gEmPlayerActionKinds_22_2[5][4] = {
#include "game/008BB980.inc"
};
s32 gEmPlayerActionKinds_22_3[5][4] = {
#include "game/008BB9D0.inc"
};
s32 gEmPlayerActionKinds_22_4[5][4] = {
#include "game/008BBA20.inc"
};
s32 (*gEmPlayerActionKinds_22[8])[4] = {
    &gEmPlayerActionKinds_22_0, gEmPlayerActionKinds_22_1, gEmPlayerActionKinds_22_2, gEmPlayerActionKinds_22_3,
    gEmPlayerActionKinds_22_4, NULL, NULL, NULL,
};
s32 (**gEmPlayerActionKinds[76])[4] = {
    NULL, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1,
    gEmPlayerActionKinds_4, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1,
    gEmPlayerActionKinds_1, gEmPlayerActionKinds_4, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1,
    gEmPlayerActionKinds_4, gEmPlayerActionKinds_4, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1,
    gEmPlayerActionKinds_4, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1,
    gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_22, gEmPlayerActionKinds_1,
    gEmPlayerActionKinds_1, gEmPlayerActionKinds_4, gEmPlayerActionKinds_1, gEmPlayerActionKinds_4,
    gEmPlayerActionKinds_4, gEmPlayerActionKinds_4, gEmPlayerActionKinds_4, gEmPlayerActionKinds_4,
    gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_4,
    gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1,
    gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1,
    gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1,
    gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1,
    gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1,
    gEmPlayerActionKinds_1, gEmPlayerActionKinds_4, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1,
    gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1,
    gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_1,
    gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, gEmPlayerActionKinds_4, gEmPlayerActionKinds_1,
    gEmPlayerActionKinds_1, gEmPlayerActionKinds_1, NULL, NULL,
};
EmReaction gEmReactions_1[20] = {
#include "game/008BBBC0.inc"
};
EmReaction gEmReactions_2[20] = {
#include "game/008BBC60.inc"
};
EmReaction gEmReactions_3[20] = {
#include "game/008BBD00.inc"
};
EmReaction gEmReactions_9[20] = {
#include "game/008BBDA0.inc"
};
EmReaction gEmReactions_12[20] = {
#include "game/008BBE40.inc"
};
EmReaction gEmReactions_13[20] = {
#include "game/008BBEE0.inc"
};
EmReaction gEmReactions_22[20] = {
#include "game/008BBF80.inc"
};
EmReaction gEmReactions_27[20] = {
#include "game/008BC020.inc"
};
EmReaction gEmReactions_29[20] = {
#include "game/008BC0C0.inc"
};
EmReaction gEmReactions_66[20] = {
#include "game/008BC160.inc"
};
EmReaction *gEmReactions[76] = {
    NULL, gEmReactions_1, gEmReactions_2, gEmReactions_3,
    gEmReactions_3, gEmReactions_3, gEmReactions_1, gEmReactions_2,
    gEmReactions_1, gEmReactions_9, gEmReactions_1, gEmReactions_1,
    gEmReactions_12, gEmReactions_13, gEmReactions_1, gEmReactions_1,
    gEmReactions_13, gEmReactions_1, gEmReactions_1, gEmReactions_3,
    gEmReactions_1, gEmReactions_1, gEmReactions_22, gEmReactions_9,
    gEmReactions_3, gEmReactions_12, gEmReactions_1, gEmReactions_27,
    gEmReactions_27, gEmReactions_29, gEmReactions_13, gEmReactions_27,
    gEmReactions_3, gEmReactions_1, gEmReactions_1, gEmReactions_13,
    gEmReactions_2, gEmReactions_1, gEmReactions_1, gEmReactions_1,
    gEmReactions_1, gEmReactions_1, gEmReactions_1, gEmReactions_1,
    gEmReactions_1, gEmReactions_1, gEmReactions_1, gEmReactions_1,
    gEmReactions_1, gEmReactions_1, gEmReactions_1, gEmReactions_1,
    gEmReactions_1, gEmReactions_1, gEmReactions_1, gEmReactions_22,
    gEmReactions_1, gEmReactions_9, gEmReactions_1, gEmReactions_1,
    gEmReactions_1, gEmReactions_1, gEmReactions_1, gEmReactions_1,
    gEmReactions_1, gEmReactions_1, gEmReactions_66, gEmReactions_1,
    gEmReactions_3, gEmReactions_3, gEmReactions_12, gEmReactions_2,
    gEmReactions_1, gEmReactions_66, NULL, NULL,
};
s32 gEmNoticeDecayRates_1[4] = {
#include "game/008BC330.inc"
};
s32 gEmNoticeDecayRates_27[4] = {
#include "game/008BC340.inc"
};
s32 *gEmNoticeDecayRates[76] = {
    NULL, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_27,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, gEmNoticeDecayRates_1,
    gEmNoticeDecayRates_1, gEmNoticeDecayRates_1, NULL, NULL,
};
EmRate gEmRateTbl_1 = {
#include "game/008BC480.inc"
};
EmRate gEmRateTbl_8 = {
#include "game/008BC4A0.inc"
};
EmRate gEmRateTbl_6 = {
#include "game/008BC4C0.inc"
};
EmRate gEmRateTbl_48 = {
#include "game/008BC4E0.inc"
};
EmRate gEmRateTbl_52 = {
#include "game/008BC500.inc"
};
EmRate *gEmRateTbl[76] = {
    NULL, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_6, &gEmRateTbl_1,
    &gEmRateTbl_8, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_6,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_6, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_6, &gEmRateTbl_6,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_6, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_48, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_52, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_48,
    &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1, &gEmRateTbl_1,
    &gEmRateTbl_1, &gEmRateTbl_1, NULL, NULL,
};
EmRate gEmDecay7ECRates_1 = {
#include "game/008BC650.inc"
};
EmRate gEmDecay7ECRates_15 = {
#include "game/008BC670.inc"
};
EmRate *gEmDecay7ECRates[76] = {
    NULL, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_15,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_15, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, &gEmDecay7ECRates_1,
    &gEmDecay7ECRates_1, &gEmDecay7ECRates_1, NULL, NULL,
};
u32 gEmRateStatus_0[4] = {
#include "game/008BC7C0.inc"
};
EmRateStatusTbl gEmRateStatus = {
#include "game/008BC7D0.inc"
};
