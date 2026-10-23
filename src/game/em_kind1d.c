/*
 * Monster kind 29 (the names EmKind1D_* give it in hex; no code shows its
 * species). Its functions are few: EmKind1D_Init sets the monster up from its
 * variant's row of EmKind1D_Variants (hit points, size and the one of its
 * five meshes shown; with no hit points in the row it gets 4000) and, at the
 * quest's stage 0, places it by its slot number (in a grid in
 * AREA_SNOWY_MOUNTAINS_NIGHT_9, in a row elsewhere). EmKind1D_Update and the
 * groups' dispatchers run its few states (em_kind1d.h): it never moves or
 * attacks, a flinch lasts 10 frames, and its death (EmKind1D_Die) plays its
 * variant's sound and effects. Its rows have the dromes' layout
 * (EmDromeKindEntry).
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_kind1d.h"
#include "em_drome.h"
#include "objattach.h"
#include "area.h"
#include "capsule.h"
#include "gm_em.h"
#include "gm_load.h"
#include "hit_effects.h"
#include "part_effects.h"
#include "hit_spark.h"
extern EmDromeKindEntry EmKind1D_Variants[];
/* end of prototypes */

/* Kind 29's own meaning of a work-area field (Em.work; EmWork has the shared ones). */
typedef struct EmKind1DWork {
    u8 fxStep;                  /* 0x000: as EmWork's */
    u8 unk001[4 - 1];
    s16 noHp;                   /* 0x004: Init: 1 when the variant's hp is 0 (the monster then gets 4000), else 0 */
    u8 unk006[256 - 6];
} EmKind1DWork;
typedef char EmKind1DWork_size_check[(sizeof(EmKind1DWork) == sizeof(EmWork)) ? 1 : -1];

/*
 * Sets the monster up: at the quest's stage 0 it is placed by its slot number
 * (a grid in AREA_SNOWY_MOUNTAINS_NIGHT_9, a row elsewhere); blind, no field
 * collision, local; KIND1D_MOT_IDLE in EM_STANCE_AIR; idle state 1, which the
 * idle dispatcher has no case for, so it stands still. Its variant's row of
 * EmKind1D_Variants gives the hit points (4000 when the row has none, and
 * noHp then 1), the scale and the one of its five meshes shown.
 */
void EmKind1D_Init(Em *em)
{
    EmKind1DWork *kindWork = (EmKind1DWork *)&em->work;
    EmDromeKindEntry *row = &EmKind1D_Variants[em->variant];
    f32 slotF;

    if (gQuest.stage == 0) {
        if (Area_DayId(gScene.area) == AREA_SNOWY_MOUNTAINS_NIGHT_9) {
            em->posX = 9500.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 9200.0f + (150.0f * (f32) (em->slotNo & 7));
        } else {
            slotF = (f32) (u32) em->slotNo;
            em->posX = 5000.0f + (200.0f * slotF);
            em->posY = 0.0f;
            em->posZ = 5000.0f;
        }
    }
    em->blind = 2;
    em->senseDelay = 5;
    em->fieldCollide = 0;
    em->remote = 0;
    Em_SetMotionRow(em, KIND1D_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_AIR;
    Em_SetStateRand(em, EM_GROUP_IDLE, 1);
    if (row->hp <= 0) {
        kindWork->noHp = 1;
        em->hpMax = em->hp = 4000;
    } else {
        kindWork->noHp = 0;
        em->hpMax = em->hp = row->hp;
    }
    em->bodyScale = em->scaleX = em->scaleY = em->scaleZ = row->scale;
    GmEm_ApplySize(em);
    em->meshShown[0] = 0;
    em->meshShown[1] = 0;
    em->meshShown[2] = 0;
    em->meshShown[3] = 0;
    em->meshShown[4] = 0;
    em->meshShown[row->part] = 1;
    Em_InitParts(em);
}

/*
 * Ends the state: the monster's script picks the next one.
 */
void EmKind1D_EndState(Em *em)
{
    em->scriptPending = 1;
}

void EmKind1D_IdleNop(void)
{
}

/*
 * Group 0's dispatcher (EM_GROUP_IDLE, and EM_GROUP_FLY's slot); it keeps
 * the state as the previous one first.
 */
void EmKind1D_RunIdleState(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case KIND1D_IDLE_NOP:
        EmKind1D_IdleNop();
        break;
    }
}

void EmKind1D_MoveEnd(Em *em)
{
    EmKind1D_EndState(em);
}

/*
 * Group 1's dispatcher (EM_GROUP_MOVE).
 */
void EmKind1D_RunMoveState(Em *em)
{
    switch (em->state) {
    case KIND1D_MOVE_END:
        EmKind1D_MoveEnd(em);
        break;
    }
}

void EmKind1D_AttackEnd(Em *em)
{
    EmKind1D_EndState(em);
}

/*
 * Group 3's dispatcher (EM_GROUP_ATTACK).
 */
void EmKind1D_RunAttackState(Em *em)
{
    switch (em->state) {
    case KIND1D_ATK_END:
        EmKind1D_AttackEnd(em);
        break;
    }
}

/*
 * KIND1D_REACT_FLINCH: waits 10 frames, then ends.
 */
void EmKind1D_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 10;
        break;
    case 1:
        if (--em->timer <= 0) {
            EmKind1D_EndState(em);
        }
        break;
    }
}

void EmKind1D_ReactEnd(Em *em)
{
    EmKind1D_EndState(em);
}

/*
 * Group 4's dispatcher (EM_GROUP_REACT; EM_GROUP_EVENT and EM_GROUP_EVENT2
 * run it too).
 */
void EmKind1D_RunReactState(Em *em)
{
    switch (em->state) {
    case KIND1D_REACT_FLINCH:
        EmKind1D_Flinch(em);
        break;
    case KIND1D_REACT_END:
        EmKind1D_ReactEnd(em);
        break;
    }
}

/*
 * The death (group 5, EM_GROUP_DEFEAT, whatever the state): unhittable and
 * unpushable, it plays its variant's death sound and effects (variant 4 adds
 * a fire blast and the dromes' defeat explosion), then hides and ends its
 * task the next frame.
 */
void EmKind1D_Die(Em *em)
{
    EmDromeKindEntry *row = &EmKind1D_Variants[em->variant];

    em->noHitTimer = 10;
    em->noPushTimer = 10;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 60;
        Em_PlaySe(em, row->se);
        HitSpark_SpawnForOwner(em, 0, 9, 0, &em->posX, em->scaleX);
        HitSpark_SpawnForOwner(em, 0, 9, 1, &em->posX, em->scaleX);
        PartFx_SpawnIfHere(em, &em->posX, 34, 5.0f * em->scaleX);
        if (em->variant == 4) {
            if (em->area == gScene.area) {
                FireBlast_CreateScaled(&em->posX, 0, 3.0f);
            }
            Explosion_CreateAt(&em->posX, EXPLOSION_DROME_DEFEAT, em->area, 0);
        }
        break;
    case 1:
        em->taskPhase++;
        em->visible = 0;
        break;
    }
}

/*
 * The per-frame update (the monster task's per-kind dispatch): blind, and
 * guarded while the halfword at Em.fullCloak is set; of Em_UpdateDamage's
 * results only a kill (KIND1D_DEFEAT_DIE), a part flinch or a hit of
 * hitReactMode 3 (KIND1D_REACT_FLINCH) and the stun's end count. Then the
 * pending script, and the state (twice on a state request).
 */
void EmKind1D_Update(Em *em)
{
    u8 dmgInfo[3];
    u32 damage;

    em->blind = 2;
    if (*(s16 *)&em->fullCloak != 0) {
        em->guardTimer = 10;
    }
    damage = Em_UpdateDamage(em, dmgInfo) & 0xFF;
    switch (damage) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_STATUS4:
    case EM_DMG_CAPTURED:
    case EM_DMG_STATUS2:
    case EM_DMG_FLASH:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_HIT:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
    case EM_DMG_ATTACK13:
    case EM_DMG_ATTACK6:
    case EM_DMG_STATUS_IN_AIR:
    case EM_DMG_HIT_LYING:
    case EM_DMG_DEAD:
        break;
    case EM_DMG_KILLED:
        Em_SetStateRand(em, EM_GROUP_DEFEAT, KIND1D_DEFEAT_DIE);
        break;
    case EM_DMG_PART_FLINCH:
    case EM_DMG_HIT_MODE3:
        Em_SetStateRand(em, EM_GROUP_REACT, KIND1D_REACT_FLINCH);
        Em_ResetPartHp(em, 0U);
        break;
    case EM_DMG_STATUS5:
        Em_EndStatus5(em);
        break;
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmKind1D_RunState(em);
    if (em->stateRequest != 0) {
        EmKind1D_RunState(em);
        em->stateRequest = 0;
    }
}

/*
 * Runs the state group's dispatcher: EM_GROUP_FLY's slot runs the idle one,
 * the two event groups the reactions'. The last case falls into the
 * default, as the game's code does.
 */
void EmKind1D_RunState(Em *em)
{
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmKind1D_RunIdleState(em);
        break;
    case EM_GROUP_MOVE:
        EmKind1D_RunMoveState(em);
        break;
    case EM_GROUP_FLY:
        EmKind1D_RunIdleState(em);
        break;
    case EM_GROUP_ATTACK:
        EmKind1D_RunAttackState(em);
        break;
    case EM_GROUP_REACT:
        EmKind1D_RunReactState(em);
        break;
    case EM_GROUP_DEFEAT:
        EmKind1D_Die(em);
        break;
    case EM_GROUP_EVENT:
        EmKind1D_RunReactState(em);
        break;
    case EM_GROUP_EVENT2:
        EmKind1D_RunReactState(em);
        
    default:
        break;
    }
}

/*
 * The motion-effects slot of the callbacks: it only steps fxStep from 0 to
 * 1; the kind has no motion effects.
 */
void EmKind1D_UpdateMotionEffects(Em *em)
{
    EmKind1DWork *kindWork = (EmKind1DWork *)&em->work;

    switch (kindWork->fxStep) {
    case 0:
        kindWork->fxStep++;
        
    case 1:
        break;
    }
}

void EmKind1D_TableNop(void)
{
}

/* The file's initialised data, in the game's order */
EmDromeKindEntry EmKind1D_Variants[8] = {
#include "game/008C4140.inc"
};
void *EmKind1D_Callbacks[8] = {
    EmKind1D_TableNop, EmKind1D_TableNop, EmKind1D_TableNop, EmKind1D_UpdateMotionEffects,
    GmEm_ReturnZero, EmKind1D_TableNop, NULL, NULL,
};
