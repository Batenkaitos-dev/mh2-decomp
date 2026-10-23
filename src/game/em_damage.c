/*
 * The monsters' status ailments and damage. Em_PickStatus picks the status
 * a monster begins, and Em_IsHeld tells whether it is held. Em_StartBlind,
 * Em_ApplyFlash and Em_ForceState19 are a flash's blindness, the reaction
 * to it and its end. Em_UpdateStatus runs the poison and the status
 * build-ups each frame; Em_AddPoison and Em_AddStatus1 .. Em_AddStatus5 add
 * to them, and Em_SetStatus1Timer .. Em_SetStatus5Timer set a status's
 * time. Em_ResetKindTimer and Em_HasKindTimer keep the hold reaction's
 * timer. Em_PickDamagedPart, Em_ApplyPartBreaks, Em_PartBroken and
 * Em_FlagPartBreak sum the damage per part and break the parts. Last,
 * Em_UpdateDamage settles the monster's hits and statuses into the
 * reaction its family plays, and Em_DamageResultIsReaction tells a
 * reaction from the other results.
 *
 * The .rodata is four jump tables: Em_StartBlind's, Em_ApplyFlash's two
 * and Em_UpdateStatus's.
 */

#include "common.h"
#include "quest.h"
#include "em.h"
#include "em_anteka.h"
#include "em_apceros.h"
#include "em_mosswine.h"
#include "player.h"
#include "area.h"
#include "em_blangonga.h"
#include "em_cephadrome.h"
#include "em_plesioth.h"
#include "em_rathalos.h"
#include "em_script.h"

void Effect06_SpawnAt(void *obj, s32 type, s32 bone, f32 *pos, f32 scale); /* unit's own declaration: type and bone declared s32 as the callers pass them; with the definition's s16 type and u8 bone the calls compile differently (Em_UpdateStatus most of all) */

s16 Player_IsState(void *obj, u8 state, u8 sub);
extern u8 *gEmDamagedPartTbls[];
void StunStars_CreateEm(Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale);
extern s8 gEmKindTimerIndex[];
extern s16 gEmBlindTimes[];
extern s16 gEmCombatInit[];
extern s16 gEmKindTimerValues[];
extern EmPoisonParam *gEmStatusParams[];
/*
 * Picks the status a monster begins, or 0: only when none runs (em->status 0)
 * and the last hit was not attack kind 6 or 9. In the air (EM_STANCE_AIR, or
 * EM_STANCE_FAMILY for the Kushala family) it ends statuses 1 and 2
 * (Em_EndStatus1And2) and returns 1; else 4 for a status 4 build-up, 2 for
 * status 1, 3 for status 2, 5 for status 5 (the stun). Em_UpdateDamage turns
 * these into EM_DMG_*.
 */
u8 Em_PickStatus(Em *em)
{
    u8 ret = 0;
    s32 pending;

    if (em->status == 0 && em->lastAttackKind != 6 && em->lastAttackKind != 9) {
        pending = (u8) (em->statusFlags & ~EM_STATUSF_POISONED);
        if (pending != 0) {
            if (em->stance == 2 || ((em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA || em->type == EM_KIND_LUNASTRA || em->type == EM_KIND_TEOSTRA) && em->stance == 1)) {
                Em_EndStatus1And2(em);
                ret = 1;
            } else if ((u8) pending & 8) {
                ret = 4;
            } else if ((u8) pending & 1) {
                ret = 2;
            } else if ((u8) pending & 2) {
                ret = 3;
            } else if ((u8) pending & 0x10) {
                ret = 5;
            }
        }
    }
    return ret;
}

/* Returns 1 while the monster is held (holdTimer or holdTimer2), else 0. */
s32 Em_IsHeld(Em *em)
{
    if (*(s8 *) &em->holdTimer) {
        return 1;
    }
    if (*(s8 *) &em->holdTimer2) {
        return 1;
    }
    return 0;
}

/*
 * Starts the monster's flash blindness: blindTimer from gEmBlindTimes[type],
 * with the stun stars over its head (StunStars_CreateEm, type 2) at a
 * height and size per kind; kinds with no case get none.
 */
void Em_StartBlind(Em *em)
{
    em->blindTimer = gEmBlindTimes[em->type];
    switch (em->type) {
    case EM_KIND_RATHIAN:
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_GYPCEROS:
    case EM_KIND_PLESIOTH:
    case EM_KIND_BASARIOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_YIAN_GARUGA:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_WHITE_MONOBLOS:
    case EM_KIND_GREEN_PLESIOTH:
    case EM_KIND_BLACK_GRAVIOS:
    case EM_KIND_AZURE_RATHALOS:
    case EM_KIND_PURPLE_GYPCEROS:
    case EM_KIND_RATHALOS:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_GRAVIOS:
    case EM_KIND_CEPHADROME:
    case EM_KIND_DIABLOS:
    case EM_KIND_CEPHALOS:
        StunStars_CreateEm(em, 2, 34, em->blindTimer, 150.0f, 3.0f);
        break;
    case EM_KIND_FATALIS_B:
    case EM_KIND_FATALIS_C:
    case EM_KIND_FATALIS:
        StunStars_CreateEm(em, 2, 51, em->blindTimer, 150.0f, 3.0f);
        break;
    case 9:
    case EM_KIND_MELYNX:
        StunStars_CreateEm(em, 2, 0, em->blindTimer, 90.0f, 1.0f);
        break;
    case EM_KIND_MOSSWINE:
        StunStars_CreateEm(em, 2, 0, em->blindTimer, 100.0f, 1.0f);
        break;
    case EM_KIND_GENPREY:
    case EM_KIND_VELOCIPREY:
    case EM_KIND_VELOCIDROME:
    case EM_KIND_GENDROME:
    case EM_KIND_IOPREY:
    case EM_KIND_IODROME:
    case EM_KIND_GIAPREY:
        StunStars_CreateEm(em, 2, 14, em->blindTimer, 50.0f, 1.0f);
        break;
    case 12:
    case EM_KIND_APCEROS:
        StunStars_CreateEm(em, 2, 15, em->blindTimer, 50.0f, 1.5f);
        break;
    case EM_KIND_KELBI:
    case EM_KIND_KIRIN:
    case EM_KIND_ANTEKA:
        StunStars_CreateEm(em, 2, 23, em->blindTimer, 50.0f, 1.5f);
        break;
    case EM_KIND_VESPOID:
    case EM_KIND_HORNETAUR:
        StunStars_CreateEm(em, 2, 8, em->blindTimer, 20.0f, 1.0f);
        break;
    case EM_KIND_CONGALALA:
    case EM_KIND_RAJANG:
    case EM_KIND_BLANGO:
    case EM_KIND_CONGA:
    case EM_KIND_BLANGONGA:
        StunStars_CreateEm(em, 2, 17, em->blindTimer, 100.0f, 2.0f);
        break;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
        StunStars_CreateEm(em, 2, 37, em->blindTimer, 150.0f, 3.0f);
        break;
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        StunStars_CreateEm(em, 2, 37, em->blindTimer, 270.0f, 3.0f);
        break;
    case EM_KIND_SHAKALAKA:
        StunStars_CreateEm(em, 2, 11, em->blindTimer, 30.0f, 1.0f);
        break;
    case EM_KIND_CHAMELEOS:
        StunStars_CreateEm(em, 2, 39, em->blindTimer, 200.0f, 3.5f);
        break;
    case EM_KIND_POPO:
        StunStars_CreateEm(em, 2, 14, em->blindTimer, 100.0f, 1.5f);
        
    case 0:
    default:
        break;
    }
}

void Em_ChangeState(Em *em, s32 arg1, u16 arg2, s32 arg3); /* unit's own declaration: the state declared
 * u16, so Em_ApplyFlash passes its u16 st as it is; with the definition's s32 the call would mask it to 16 bits */
/* The monster's reaction to a flash (Game's hit dispatcher calls it when em->flashed was set):
 * by type and stance, returns early (states that ignore it) or picks the state st (7-10, or 2
 * with Em_StartBlind and the timer), then Em_ChangeState(em, 4, st, 3). Case EM_KIND_SHAKALAKA's
 * `switch (em->stateGroup)` is written with its default first (falling into the return of
 * 4-6): with that form the compiler lays out every branch of the function as the game's code
 * has it. The other groups' tests are plain if/else and && chains. Em_ChangeState is declared
 * above with a u16 state (its definition takes an s32): with the s32 the call would mask st
 * to 16 bits, which the game's code doesn't.
 */
void Em_ApplyFlash(Em *em)
{
    u16 flashState;
    u8 group;

    switch (em->type) {
    case EM_KIND_DIABLOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_WHITE_MONOBLOS:
        if (em->stance == 4) {
            return;
        }
    case EM_KIND_RATHIAN:
    case EM_KIND_FATALIS:
    case EM_KIND_KELBI:
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_CEPHADROME:
    case EM_KIND_RATHALOS:
    case 12:
    case EM_KIND_GENPREY:
    case EM_KIND_VELOCIPREY:
    case EM_KIND_GRAVIOS:
    case EM_KIND_VESPOID:
    case EM_KIND_GYPCEROS:
    case EM_KIND_PLESIOTH:
    case EM_KIND_BASARIOS:
    case EM_KIND_HORNETAUR:
    case EM_KIND_APCEROS:
    case EM_KIND_VELOCIDROME:
    case EM_KIND_GENDROME:
    case EM_KIND_IOPREY:
    case EM_KIND_IODROME:
    case EM_KIND_KIRIN:
    case EM_KIND_CEPHALOS:
    case EM_KIND_GIAPREY:
    case EM_KIND_FATALIS_B:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_PURPLE_GYPCEROS:
    case EM_KIND_YIAN_GARUGA:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_GREEN_PLESIOTH:
    case EM_KIND_BLACK_GRAVIOS:
    case EM_KIND_AZURE_RATHALOS:
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_CHAMELEOS:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
    case EM_KIND_ANTEKA:
    case EM_KIND_POPO:
    case EM_KIND_FATALIS_C:
        em->blind = 10;
        Em_StartBlind(em);
        if (em->remote != 0) {
            return;
        }
        if (Em_IsHeld(em) == 1) {
            return;
        }
        if (!(em->statusFlags & 8)) {
            break;
        }
        return;
    case EM_KIND_MOSSWINE:
    case 9:
    case EM_KIND_MELYNX:
    case EM_KIND_SHAKALAKA:
        if (em->remote != 0) {
            return;
        }
        if (Em_IsHeld(em) == 1) {
            return;
        }
        if (!(em->statusFlags & 8)) {
            break;
        }
        return;
    case EM_KIND_BLANGONGA:
    case EM_KIND_CONGALALA:
    case EM_KIND_RAJANG:
    case EM_KIND_BLANGO:
    case EM_KIND_CONGA:
        if (EmBlangonga_InStateArmor(em) == 1) {
            return;
        }
        em->blind = 10;
        Em_StartBlind(em);
        if (em->remote != 0) {
            return;
        }
        if (Em_IsHeld(em) == 1) {
            return;
        }
        if (!(em->statusFlags & 8)) {
            break;
        }
        return;
    default:
        return;
    }
    switch (em->type) {
    case EM_KIND_RATHIAN:
    case EM_KIND_RATHALOS:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_AZURE_RATHALOS:
        if (em->stance == 2) {
            flashState = 9;
        } else {
            flashState = 10;
        }
        break;
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_GYPCEROS:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_PURPLE_GYPCEROS:
        if (em->stance == 2) {
            flashState = 9;
        } else {
            flashState = 10;
        }
        break;
    case EM_KIND_GRAVIOS:
    case EM_KIND_BASARIOS:
    case EM_KIND_BLACK_GRAVIOS:
        if (em->stance == 2) {
            flashState = 9;
        } else {
            flashState = 10;
        }
        break;
    case EM_KIND_YIAN_GARUGA:
        if (em->stance == 2) {
            flashState = 9;
        } else {
            flashState = 10;
        }
        break;
    case EM_KIND_DIABLOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_WHITE_MONOBLOS:
        if (em->stance == 2) {
            flashState = 9;
        } else {
            group = em->stateGroup;
            if (group == 4 || (group == 0 && em->state == 28) || (group == 0 && em->state == 29)
                || (group == 0 && em->state == 33) || (group == 4 && em->state == 6)
                || (group == 4 && em->state == 13) || (group == 4 && em->state == 14)
                || (group == 4 && em->state == 17) || (group == 4 && em->state == 18)) {
                return;
            }
            flashState = 10;
        }
        break;
    case EM_KIND_FATALIS:
    case EM_KIND_FATALIS_B:
    case EM_KIND_FATALIS_C:
        if (em->stance == 2) {
            flashState = 8;
        } else {
            flashState = 7;
        }
        break;
    case 9:
    case EM_KIND_MELYNX:
        if (em->stance == 0 && em->stateGroup != 4 && em->stateGroup != 5 && em->stateGroup != 6
            && Player_IsState((Player *) em, 4, 2) == 0) {
            flashState = 2;
            Em_StartBlind(em);
            em->timer = em->blindTimer;
            break;
        }
        return;
    case EM_KIND_SHAKALAKA:
        switch (em->stateGroup) {
        default:
            if (Player_IsState((Player *) em, 4, 2) == 0 && Player_IsState((Player *) em, 4, 4) == 0) {
                if (em->stance == 0) {
                    flashState = 2;
                }
                em->blind = 10;
                Em_StartBlind(em);
                em->timer = em->blindTimer;
                break;
            }
        case 6:
        case 5:
        case 4:
            return;
        }
        break;
    case EM_KIND_MOSSWINE:
        if (em->stance == 0 && em->stateGroup != 4 && em->stateGroup != 5 && em->stateGroup != 6
            && Player_IsState((Player *) em, 4, 4) == 0) {
            flashState = 4;
            Em_StartBlind(em);
            em->timer = em->blindTimer;
            break;
        }
        return;
    case EM_KIND_GENPREY:
    case EM_KIND_VELOCIPREY:
    case EM_KIND_IOPREY:
    case EM_KIND_GIAPREY:
        if (em->stance == 2) {
            flashState = 1;
        } else {
            flashState = 5;
        }
        break;
    case EM_KIND_VELOCIDROME:
    case EM_KIND_GENDROME:
    case EM_KIND_IODROME:
        if (em->stance == 2) {
            flashState = 1;
        } else {
            flashState = 5;
        }
        break;
    case 12:
    case EM_KIND_APCEROS:
    case EM_KIND_POPO:
        flashState = 5;
        break;
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
        switch (em->stance) {
        case 0:
            flashState = 10;
            break;
        case 1:
            flashState = 4;
            break;
        case 4:
            flashState = 17;
            break;
        case 2:
            flashState = 9;
            break;
        default:
            flashState = 10;
            break;
        }
        break;
    case EM_KIND_CEPHADROME:
    case EM_KIND_CEPHALOS:
        switch (em->stance) {
        case 0:
            flashState = 10;
            break;
        case 1:
            flashState = 4;
            break;
        case 4:
            flashState = 17;
            break;
        case 2:
            flashState = 9;
            break;
        default:
            flashState = 10;
            break;
        }
        break;
    case EM_KIND_KELBI:
    case EM_KIND_ANTEKA:
        flashState = 4;
        break;
    case EM_KIND_VESPOID:
    case EM_KIND_HORNETAUR:
        if (em->stance != 2) {
            flashState = 5;
        } else {
            flashState = 4;
        }
        break;
    case EM_KIND_KIRIN:
        flashState = 4;
        break;
    case EM_KIND_BLANGONGA:
    case EM_KIND_CONGALALA:
    case EM_KIND_RAJANG:
    case EM_KIND_BLANGO:
    case EM_KIND_CONGA:
        if (em->stance == 2) {
            flashState = 7;
        } else {
            if (EmBlangonga_IgnoresFlash(em) == 1) {
                return;
            }
            flashState = 1;
        }
        break;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        if (em->stance == 2 || em->stance == 1) {
            flashState = 9;
        } else {
            flashState = 10;
        }
        break;
    case EM_KIND_CHAMELEOS:
        if (em->stance == 2) {
            flashState = 9;
        } else {
            flashState = 10;
        }
        break;
    }
    em->scriptPending = 0;
    /* the game reads a register it never set: st is s0, still the caller's, after case EM_KIND_SHAKALAKA with em->stance != 0 */
    Em_ChangeState(em, 4, flashState, 3);
}

/*
 * Called by GmEm_Update when a grounded monster's flash blindness
 * (blindTimer) runs out outside EM_GROUP_REACT: puts it into an idle state
 * that plays one motion. The Rathalos family goes to RATH_IDLE_SHAKE_OFF,
 * the Cephadrome kinds to state 7, the Plesioth kinds to state 7 only
 * on the ground; other kinds are left as they are.
 */
void Em_ForceState19(Em *em)
{
    switch (em->type) {
    case EM_KIND_RATHIAN:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_RATHALOS:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_AZURE_RATHALOS:
        EmRathalos_ChangeState(em, 0, 25, 4);
        break;
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_DIABLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
    case EM_KIND_GRAVIOS:
    case EM_KIND_BLACK_GRAVIOS:
    case EM_KIND_GYPCEROS:
    case EM_KIND_PURPLE_GYPCEROS:
    case EM_KIND_BASARIOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_WHITE_MONOBLOS:
    case EM_KIND_YIAN_GARUGA:
        break;
    case EM_KIND_CEPHADROME:
    case EM_KIND_CEPHALOS:
        EmCephadrome_ChangeState(em, 0, 7, 4);
        break;
    case 9:
    case EM_KIND_MELYNX:
    case EM_KIND_MOSSWINE:
    case EM_KIND_BULLFANGO:
    case EM_KIND_BULLDROME:
    case 32:
    case EM_KIND_GENPREY:
    case EM_KIND_VELOCIPREY:
    case EM_KIND_GIAPREY:
    case EM_KIND_IOPREY:
    case EM_KIND_VELOCIDROME:
    case EM_KIND_GENDROME:
    case EM_KIND_IODROME:
    case 12:
    case EM_KIND_POPO:
    case EM_KIND_APCEROS:
        break;
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
        if (em->stance == 0) {
            EmPlesioth_ChangeState(em, 0, 7, 4);
        }
        break;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        break;
    }
}

/*
 * The monster's per-frame poison and status build-ups, from the kind's
 * parameter tables (GmEm_Update, outside EM_GROUP_DEFEAT). While poisoned
 * (EM_STATUSF_POISONED) it counts poisonDamage down, shows the poison effect
 * once a second, and at each poisonTimer tick takes the kind's poison damage
 * off hp (hp stays at 1 under guardTimer and for a large monster). When the
 * poison ends, up to four times (poisonCount) poisonThreshold rises by the
 * kind's step. The build-ups of poison and of statuses 4, 5, 1 and 2 not
 * running lose decayAmount every decayInterval frames.
 */
void Em_UpdateStatus(Em *em)
{
    u8 *poisonParam = (u8 *) gEmStatusParams[em->type];
    EmStatusParam *status4Param = gEmStatus4Params[em->type];
    EmStatusParam *status5Param = gEmStatus5Params[em->type];
    EmStatusParam *status2Param;
    EmStatusParam *status1Param = gEmStatus1Params[em->type];
    f32 pos[3];
    s32 bone;
    f32 scale;

    status2Param = gEmStatus2Params[em->type];

    if (em->statusFlags & EM_STATUSF_POISONED) {
        if (--em->poisonDamage > 0) {
            switch (em->type) {
            case EM_KIND_RATHIAN:
            case EM_KIND_YIAN_KUT_KU:
            case EM_KIND_CEPHADROME:
            case EM_KIND_RATHALOS:
            case EM_KIND_DIABLOS:
            case EM_KIND_KHEZU:
            case EM_KIND_GRAVIOS:
            case EM_KIND_GYPCEROS:
            case EM_KIND_PLESIOTH:
            case EM_KIND_BASARIOS:
            case EM_KIND_MONOBLOS:
            case EM_KIND_CEPHALOS:
            case EM_KIND_PINK_RATHIAN:
            case EM_KIND_BLUE_YIAN_KUT_KU:
            case EM_KIND_PURPLE_GYPCEROS:
            case EM_KIND_YIAN_GARUGA:
            case EM_KIND_SILVER_RATHALOS:
            case EM_KIND_GOLD_RATHIAN:
            case EM_KIND_BLACK_DIABLOS:
            case EM_KIND_WHITE_MONOBLOS:
            case EM_KIND_RED_KHEZU:
            case EM_KIND_GREEN_PLESIOTH:
            case EM_KIND_BLACK_GRAVIOS:
            case EM_KIND_AZURE_RATHALOS:
                pos[0] = 0.0f;
                pos[1] = 100.0f;
                pos[2] = 0.0f;
                bone = 34;
                scale = 2.0f;
                break;
            case EM_KIND_FATALIS:
            case EM_KIND_FATALIS_B:
            case EM_KIND_FATALIS_C:
                pos[0] = 0.0f;
                pos[1] = 200.0f;
                pos[2] = 0.0f;
                bone = 51;
                scale = 2.0f;
                break;
            case EM_KIND_KELBI:
            case EM_KIND_ANTEKA:
                pos[0] = 0.0f;
                pos[1] = 50.0f;
                pos[2] = 0.0f;
                bone = ANTEKA_BONE_HEAD;
                scale = 1.0f;
                break;
            case 12:
            case EM_KIND_APCEROS:
            case EM_KIND_POPO:
                pos[0] = 0.0f;
                pos[1] = 50.0f;
                pos[2] = 0.0f;
                bone = APC_BONE_BREATH;
                scale = 1.2f;
                break;
            case 9:
            case EM_KIND_MELYNX:
            case EM_KIND_SHAKALAKA:
                pos[0] = 0.0f;
                pos[1] = 20.0f;
                pos[2] = 0.0f;
                bone = 10;
                scale = 1.2f;
                break;
            case EM_KIND_GENPREY:
            case EM_KIND_VELOCIPREY:
            case EM_KIND_VELOCIDROME:
            case EM_KIND_GENDROME:
            case EM_KIND_IOPREY:
            case EM_KIND_IODROME:
            case EM_KIND_GIAPREY:
                pos[0] = 0.0f;
                pos[1] = 20.0f;
                pos[2] = 0.0f;
                bone = 14;
                scale = 1.2f;
                break;
            case EM_KIND_MOSSWINE:
            case EM_KIND_BULLFANGO:
            case 32:
            case EM_KIND_BULLDROME:
                pos[0] = 0.0f;
                pos[1] = 10.0f;
                pos[2] = 0.0f;
                bone = MOSS_BONE_BREATH;
                scale = 1.0f;
                break;
            case EM_KIND_VESPOID:
            case EM_KIND_HORNETAUR:
                pos[0] = 0.0f;
                pos[1] = 0.0f;
                pos[2] = 0.0f;
                bone = 8;
                scale = 0.5f;
                break;
            case EM_KIND_KIRIN:
                pos[0] = 0.0f;
                pos[1] = 50.0f;
                pos[2] = 0.0f;
                bone = 23;
                scale = 1.0f;
                break;
            case EM_KIND_BLANGONGA:
            case EM_KIND_CONGALALA:
            case EM_KIND_RAJANG:
            case EM_KIND_BLANGO:
            case EM_KIND_CONGA:
                pos[0] = 0.0f;
                pos[1] = 40.0f;
                pos[2] = 45.0f;
                bone = 17;
                scale = 1.0f;
                break;
            case EM_KIND_KUSHALA_DAORA:
            case EM_KIND_RUSTED_KUSHALA_DAORA:
            case EM_KIND_LUNASTRA:
            case EM_KIND_TEOSTRA:
                pos[0] = 0.0f;
                pos[1] = 100.0f;
                pos[2] = 0.0f;
                bone = 37;
                scale = 2.0f;
                break;
            case EM_KIND_DAIMYO_HERMITAUR:
            case EM_KIND_SHOGUN_CEANATAUR:
                pos[0] = 0.0f;
                pos[1] = 50.0f;
                pos[2] = 100.0f;
                bone = 31;
                scale = 2.0f;
                break;
            case EM_KIND_HERMITAUR:
            case EM_KIND_CEANATAUR:
                pos[0] = 0.0f;
                pos[1] = 40.0f;
                pos[2] = 90.0f;
                bone = 31;
                scale = 1.0f;
                break;
            case EM_KIND_CHAMELEOS:
                pos[0] = 0.0f;
                pos[1] = 100.0f;
                pos[2] = 0.0f;
                bone = 37;
                scale = 2.0f;
                break;
            case EM_KIND_REMOBRA:
                pos[0] = 0.0f;
                pos[1] = 25.0f;
                pos[2] = 45.0f;
                bone = 19;
                scale = 0.7f;
                break;
            default:
                pos[0] = 0.0f;
                pos[1] = 50.0f;
                pos[2] = 0.0f;
                bone = 0;
                scale = 1.0f;
                break;
            }
            if (gScene.frame % 60 == 0) {
                Effect06_SpawnAt(em, 3, bone, pos, scale);
            }
            if (--em->poisonTimer <= 0) {
                em->poisonTimer = *(s16 *)(poisonParam + 10);
                em->hp -= *(s16 *)(poisonParam + 6);
                if (em->hp <= 0 && ((s8) em->guardTimer > 0 || em->large != 0)) {
                    em->hp = 1;
                }
            }
        } else {
            em->poisonDamage = 0;
            em->poisonTimer = 0;
            em->statusFlags &= ~EM_STATUSF_POISONED;
            if (em->poisonCount < 4) {
                em->poisonCount++;
                em->poisonThreshold += *(s16 *)(poisonParam + 12);
            }
        }
    } else if (em->poisonBuildup != 0) {
        if (--em->poisonTimer <= 0) {
            em->poisonTimer = *(s16 *)(poisonParam + 2);
            em->poisonBuildup -= *(s16 *)(poisonParam + 4);
            if (em->poisonBuildup <= 0) {
                em->poisonBuildup = 0;
                em->poisonTimer = 0;
            }
        }
    }
    if (!(em->statusFlags & 8) && em->status4Buildup != 0) {
        if (--em->status4Decay <= 0) {
            em->status4Decay = status4Param->decayInterval;
            em->status4Buildup -= status4Param->decayAmount;
            if (em->status4Buildup <= 0) {
                em->status4Buildup = 0;
                em->status4Decay = 0;
            }
        }
    }
    if (!(em->statusFlags & EM_STATUSF_STUN) && em->status5Buildup != 0) {
        if (--em->status5Decay <= 0) {
            em->status5Decay = status5Param->decayInterval;
            em->status5Buildup -= status5Param->decayAmount;
            if (em->status5Buildup <= 0) {
                em->status5Buildup = 0;
                em->status5Decay = 0;
            }
        }
    }
    if (!(em->statusFlags & 1) && em->status1Buildup != 0) {
        if (--em->status1Decay <= 0) {
            em->status1Decay = status1Param->decayInterval;
            em->status1Buildup -= status1Param->decayAmount;
            if (em->status1Buildup <= 0) {
                em->status1Buildup = 0;
                em->status1Decay = 0;
            }
        }
    }
    if (!(em->statusFlags & 2) && em->status2Buildup != 0) {
        if (--em->status2Decay <= 0) {
            em->status2Decay = status2Param->decayInterval;
            em->status2Buildup -= status2Param->decayAmount;
            if (em->status2Buildup <= 0) {
                em->status2Buildup = 0;
                em->status2Decay = 0;
            }
        }
    }
}

/*
 * Adds amount to the monster's poison build-up; nothing for 0, an immune kind
 * (poisonThreshold 0) or a versus team with flag 1 (Versus_TeamHasFlag).
 * While poisoned the build-up stays just under the threshold; reaching it
 * poisons the monster (EM_STATUSF_POISONED, poisonDamage the poison's duration,
 * the first tick at once).
 */
void Em_AddPoison(Em *em, s32 amount)
{
    s16 max;
    u8 *param;

    param = (u8 *) gEmStatusParams[em->type];
    if (Versus_TeamHasFlag((void *) em, 1U) != 1 && amount != 0 && em->poisonThreshold > 0) {
        em->poisonBuildup = em->poisonBuildup + amount;
        if (em->statusFlags & EM_STATUSF_POISONED) {
            max = em->poisonThreshold;
            if (em->poisonBuildup >= max) {
                em->poisonBuildup = max - 1;
            }
        } else {
            em->poisonTimer = *(s16 *)(param + 2);
            if (em->poisonBuildup >= em->poisonThreshold) {
                em->poisonBuildup = 0;
                em->statusFlags |= EM_STATUSF_POISONED;
                em->poisonDamage = *(s16 *)(param + 8);
                em->poisonTimer = 0;
            }
        }
    }
}

/*
 * Adds amount to the status 4 build-up; nothing for 0, an immune kind
 * (status4Threshold 0) or a versus team with flag 0. Only while status 4 is
 * not pending (statusFlags bit 8); reaching status4Threshold sets that bit,
 * which Em_PickStatus turns into the status.
 */
void Em_AddStatus4(Em *em, s32 amount)
{
    EmStatusParam *param;

    param = gEmStatus4Params[em->type];
    if (Versus_TeamHasFlag((void *) em, 0U) != 1 && amount != 0 && em->status4Threshold > 0) {
        if (!(em->statusFlags & 8)) {
            em->status4Buildup = em->status4Buildup + amount;
            em->status4Decay = param->decayInterval;
        }
        if (em->status4Buildup >= em->status4Threshold) {
            em->statusFlags |= 8;
        }
    }
}

/*
 * As Em_AddStatus4 for status 5, the stun (EM_STATUSF_STUN,
 * status5Threshold), without the versus check.
 */
void Em_AddStatus5(Em *em, s32 amount)
{
    EmStatusParam *prm = gEmStatus5Params[em->type];

    if (amount != 0 && em->status5Threshold > 0) {
        if (!(em->statusFlags & EM_STATUSF_STUN)) {
            em->status5Buildup = em->status5Buildup + amount;
            em->status5Decay = prm->decayInterval;
        }
        if (em->status5Buildup >= em->status5Threshold) {
            em->statusFlags |= EM_STATUSF_STUN;
        }
    }
}

/*
 * As Em_AddStatus4 for status 2 (statusFlags bit 2, status2Threshold; versus
 * flag 2).
 */
void Em_AddStatus2(Em *em, s32 amount)
{
    EmStatusParam *param;

    param = gEmStatus2Params[em->type];
    if (Versus_TeamHasFlag((void *) em, 2U) != 1 && amount != 0 && em->status2Threshold > 0) {
        if (!(em->statusFlags & 2)) {
            em->status2Buildup = em->status2Buildup + amount;
            em->status2Decay = param->decayInterval;
        }
        if (em->status2Buildup >= em->status2Threshold) {
            em->statusFlags |= 2;
        }
    }
}

/*
 * As Em_AddStatus4 for status 1 (statusFlags bit 1, status1Threshold; versus
 * flag 2).
 */
void Em_AddStatus1(Em *em, s32 amount)
{
    EmStatusParam *param;

    param = gEmStatus1Params[em->type];
    if (Versus_TeamHasFlag((void *) em, 2U) != 1 && amount != 0 && em->status1Threshold > 0) {
        if (!(em->statusFlags & 1)) {
            em->status1Buildup = em->status1Buildup + amount;
            em->status1Decay = param->decayInterval;
        }
        if (em->status1Buildup >= em->status1Threshold) {
            em->statusFlags |= 1;
        }
    }
}

/* The monster's timer to the kind's status 4 duration (Em_BeginStatus4's counterpart in the
 * families' updates); unk78D cleared. */
void Em_SetStatus4Timer(Em *em)
{
    em->timer = gEmStatus4Params[em->type]->duration;
    em->scriptPending = 0;
}

/* The monster's timer to the kind's status 5 duration (Em_BeginStatus5's counterpart in the
 * families' updates); unk78D cleared. */
void Em_SetStatus5Timer(Em *em)
{
    em->timer = gEmStatus5Params[em->type]->duration;
    em->scriptPending = 0;
}

/* The monster's timer to the kind's status 2 duration (Em_BeginStatus2's counterpart in the
 * families' updates); unk78D cleared. */
void Em_SetStatus2Timer(Em *em)
{
    em->timer = gEmStatus2Params[em->type]->duration;
    em->scriptPending = 0;
}

/* The monster's timer to the kind's status 1 duration (Em_BeginStatus1's counterpart in the
 * families' updates); unk78D cleared. */
void Em_SetStatus1Timer(Em *em)
{
    em->timer = gEmStatus1Params[em->type]->duration;
    em->scriptPending = 0;
}

/*
 * Sets timer for the reaction to ATTACK_HOLD (EM_DMG_ATTACK13): the
 * kind's row of gEmKindTimerValues (by gEmKindTimerIndex[type]), or 10 when
 * it has none. Also clears scriptPending.
 */
void Em_ResetKindTimer(Em *em)
{
    s8 i = gEmKindTimerIndex[em->type];

    em->scriptPending = 0;
    if (i == -1) {
        em->timer = 10;
    } else {
        em->timer = gEmKindTimerValues[i];
    }
}

s32 Em_HasKindTimer(Em *em)
{
    return gEmKindTimerIndex[em->type] != -1;
}

/* sums the damage taken per part (scaled by unk728) into *sum and *neg, and passes it on */
void Em_PickDamagedPart(Em *em, s16 *sum, s16 *neg)
{
    u8 best;
    u32 i;
    s16 max;
    s16 damage;
    u8 *weakRow;

    max = 0;
    best = 0;
    weakRow = gEmDamagedPartTbls[em->type];
    for (i = 0; i < 8; i++) {
        damage = em->hitPartDamage[i] * em->damageFactor;
        if (em->hitPartDamage[i] > 0 && damage == 0) {
            damage = 1;
        }
        if (damage < 0) {
            *neg -= damage;
        } else {
            *sum += damage;
            if (max < em->hitPartDamage[i]) {
                max = em->hitPartDamage[i];
                best = i;
            }
        }
    }
    if (weakRow != NULL) {
        if ((em->partFlags[weakRow[0]] & weakRow[1]) == weakRow[1]) {
            em->hitPartDamage[8] = em->hitPartDamage[weakRow[0]];
            if (em->hitPartDamage[8] < 0) {
                em->hitPartDamage[8] = 0;
            }
        } else {
            em->hitPartDamage[8] = 0;
        }
    }
    if (em->rage == 0 && *sum > 0) {
        Em_CheckRageByHp(em, *sum);
    }
    em->flinchPart = best;
    if ((s8) em->guardTimer <= 0) {
        Em_AddStatus1(em, em->status1Hit);
        Em_AddStatus2(em, em->status2Hit);
        Em_AddPoison(em, em->poisonHit);
        Em_AddStatus4(em, em->status4Hit);
        Em_AddStatus5(em, em->status5Hit);
    }
}

/*
 * Breaks each part (0 to 15) set in partsToBreak and not yet in partsBroken
 * (Em_BreakPart), then adds partsToBreak to partsBroken.
 */
void Em_ApplyPartBreaks(Em *em)
{
    s32 i;

    for (i = 0; i < 16; i++) {
        if ((em->partsToBreak & (1 << i)) && !(em->partsBroken & (1 << i))) {
            Em_BreakPart(em, i);
        }
    }
    em->partsBroken = em->partsBroken | em->partsToBreak;
}

s32 Em_PartBroken(Em *em, s32 part)
{
    return (em->partsBroken & (1 << (part & 0xFFFF))) != 0;
}

/* Flags part (0 to 15) to break (partsToBreak); Em_ApplyPartBreaks breaks it. */
void Em_FlagPartBreak(Em *em, s32 part)
{
    em->partsToBreak |= (1 << (part & 0xFFFF)) & 0xFFFF;
}

/*
 * Returns 1 when result, an Em_UpdateDamage result, is a reaction (anything
 * but EM_DMG_NONE, EM_DMG_KILLED and EM_DMG_DEAD), else 0; em is unused. The
 * Kushala, Blangonga and Chameleos families' Update functions break parts
 * when it returns 1.
 */
s32 Em_DamageResultIsReaction(Em *em, s32 result)
{
    s32 code;

    code = result & 0xFF;
    if ((code != 0) && (code != 1) && (code != 2)) {
        return 1;
    }
    return 0;
}

/*
 * Settles the monster's last hit and statuses and returns an EmDamageResult
 * (EM_DMG_NONE when there is nothing to react to); *out gets combat as it was
 * on entry. In EM_GROUP_DEFEAT it returns EM_DMG_NONE, and EM_DMG_DEAD at 0
 * hp with no hit pending. On a hit it drops the pending triggers EM_TRIGGER_AREA_OBJ, EM_TRIGGER_PART_BUILDUP,
 * EM_TRIGGER_NOTICED and EM_TRIGGER_BLOCK_AREA, takes the damage of Em_PickDamagedPart off hp and the parts'
 * hp by partDamageMode, and breaks or flinches the parts that run out. Under
 * guardTimer hp stays at 1 (EM_DMG_NONE); else 0 hp is EM_DMG_KILLED (a large
 * monster dies only in the scene's area) and the hit starts combat. The
 * result then comes from the attack kind (ATTACK_HOLD, ATTACK_TRAP_B, ATTACK_TRAP_A), Em_PickStatus, a flash
 * (EM_DMG_FLASH) or hitReactMode and the broken parts. Every result but
 * EM_DMG_HIT also restarts the script (Em_RestartScript); Em_CheckKindRequest
 * runs last.
 */
u8 Em_UpdateDamage(Em *em, u8 *out)
{
    s16 sum;
    s16 neg;
    u8 broke;
    u8 first;
    s8 i;
    s16 damage;
    u8 bit;
    s32 (*fn)(Em *);
    u8 status;
    u8 result;

    sum = 0;
    neg = 0;
    *out = em->combat;
    broke = 0;
    if (em->stateGroup == 5) {
        return 0;
    }
    em->hitPartMask = 0;
    if (em->hp <= 0 && em->hitReceived == 0) {
        em->scriptPending = 0;
        em->combatPlayers = 0;
        return 2;
    }
    if (em->hitReceived != 0) {
        em->triggers &= 0x8D;
        Em_PickDamagedPart(em, &sum, &neg);
        if (em->hp > 0) {
            em->hp = em->hp - sum + neg;
            if (em->hpMax < em->hp) {
                em->hp = em->hpMax;
            }
        }
        first = 0;
        switch (em->partDamageMode) {
        case 1:
            for (i = 0; i < 8; i++) {
                damage = em->hitPartDamage[i] * em->damageFactor;
                if (em->hitPartDamage[i] > 0 && damage == 0) {
                    damage = 1;
                }
                em->parts[0].hp -= damage;
                if (em->parts[0].hp <= 0) {
                    broke = 1;
                    Em_ResetPartHp(em, 0);
                    if (!(em->hitPartMask & (1 << i))) {
                        em->hitPartMask |= 1 << i;
                        em->parts[0].state++;
                        if (em->parts[0].state >= 99) {
                            em->parts[0].state = 99;
                        }
                    }
                }
            }
            break;
        case 8:
            for (i = 0; i < 8; i++) {
                damage = em->hitPartDamage[i] * em->damageFactor;
                if (em->hitPartDamage[i] > 0 && damage == 0) {
                    damage = 1;
                }
                em->parts[i].hp -= damage;
                if (em->parts[i].hp <= 0) {
                    broke = 1;
                    Em_ResetPartHp(em, i);
                    if (!(em->hitPartMask & (1 << i))) {
                        em->hitPartMask |= 1 << i;
                        em->parts[i].state++;
                        if (em->parts[i].state >= 99) {
                            em->parts[i].state = 99;
                        }
                    }
                    if (first != 0) {
                        if (em->hitPartDamage[i] > em->hitPartDamage[em->flinchPart]) {
                            em->flinchPart = i;
                        }
                    } else {
                        em->flinchPart = i;
                        first = 1;
                    }
                }
            }
            break;
        case 9:
            em->part8Down = 0;
            for (i = 0; i < 9; i++) {
                damage = em->hitPartDamage[i] * em->damageFactor;
                if (em->hitPartDamage[i] > 0 && damage == 0) {
                    damage = 1;
                }
                em->parts[i].hp -= damage;
                if (em->parts[i].hp <= 0) {
                    if (i == 8) {
                        fn = em->handlers->part8DownQuery;
                        if (fn != NULL) {
                            if (fn(em) == 0 || (s8) em->guardTimer != 0) {
                                em->parts[i].hp = 1;
                                continue;
                            }
                            em->part8Down = 1;
                            goto reset;
                        } else {
                            em->parts[i].hp = 1;
                        }
                        continue;
                    }
                    if (!(em->hitPartMask & (1 << i))) {
                        em->hitPartMask |= 1 << i;
                        em->parts[i].state++;
                        if (em->parts[i].state >= 99) {
                            em->parts[i].state = 99;
                        }
                    }
                    broke = 1;
                reset:
                    Em_ResetPartHp(em, i);
                    if (first != 0) {
                        if (i < 8 && em->hitPartDamage[i] > em->hitPartDamage[em->flinchPart]) {
                            em->flinchPart = i;
                        }
                    } else {
                        em->flinchPart = i;
                        first = 1;
                    }
                }
            }
            break;
        }
        Em_SpawnPartEffects(em, em->hitPartMask);
        if ((s8) em->guardTimer != 0) {
            if (em->hp <= 0) {
                em->hp = 1;
            }
            if (em->hitReactMode != EM_HIT_REACT_WAKE) {
                Em_SetCombat(em, 1, gEmCombatInit[em->type]);
            }
            if (!((em->type == EM_KIND_FATALIS || em->type == EM_KIND_FATALIS_B || em->type == EM_KIND_FATALIS_C) && (em->lastAttackKind == 9 || em->lastAttackKind == 6))) {
                return 0;
            }
        } else {
            if ((em->type == EM_KIND_LAO_SHAN_LUNG || em->type == EM_KIND_ASHEN_LAO_SHAN_LUNG) && Area_DayId(em->area) != AREA_FORTRESS_5 && em->hp < 1000) {
                em->hp = 1000;
            }
            if (em->type == EM_KIND_SHEN_GAOREN && Map_DayId(gScene.map) == MAP_FORTRESS && Area_DayId(em->area) != AREA_FORTRESS_5 && em->hp < 1000) {
                em->hp = 1000;
            }
            if ((em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) && Map_DayId(gScene.map) == MAP_TOWER_C && Area_DayId(em->area) != AREA_TOWER_0x7C && em->hp < 1000) {
                em->hp = 1000;
            }
            if (em->hp <= 0) {
                em->scriptPending = 0;
                if (em->large != 0) {
                    if (gScene.area == em->area) {
                        em->hp = 0;
                        em->combatPlayers = 0;
                        return 1;
                    }
                    em->hp = 1;
                } else {
                    em->hp = 0;
                    em->combatPlayers = 0;
                    return 1;
                }
            }
            Em_SetCombat(em, 1, gEmCombatInit[em->type]);
        }
        if (Em_IsHeld(em) != 1 && em->lastAttackKind == 19 && Em_HasKindTimer(em) == 1) {
            result = 18;
            goto end;
        }
        if ((s8) em->holdTimer == 0) {
            if (em->lastAttackKind == 9) {
                result = 3;
                goto end;
            }
            if (em->lastAttackKind == 6) {
                result = 4;
                goto end;
            }
        }
    }
    if ((status = Em_PickStatus(em)) != 0 || em->flashed != 0) {
        Em_SetCombat(em, 1, gEmCombatInit[em->type]);
    }
    if (status == 1) {
        result = 5;
    } else if (status == 4) {
        result = 6;
    } else if (status == 2 && Em_IsHeld(em) == 1 && Em_NearLowHp(em) == 1) {
        result = 7;
    } else {
        if (Em_IsHeld(em) == 0 || Em_NearLowHp(em) == 0) {
            em->statusFlags &= 0xFE;
        }
        if (status == 3) {
            result = 8;
        } else if (status == 5) {
            result = 17;
        } else if (em->flashed != 0) {
            em->flashed = 0;
            Em_ApplyFlash(em);
            result = 9;
        } else if (em->hitReceived != 0) {
            if (em->hitReactMode == EM_HIT_REACT_WAKE) {
                result = 10;
            } else {
                if (em->hitReactMode != EM_HIT_REACT_STATUS && Em_IsHeld(em) == 0) {
                    em->hitReactMode = EM_HIT_REACT_FLINCH;
                }
                if (em->part8Down != 0) {
                    result = 11;
                } else if (em->stateGroup == 4) {
                    return 0;
                } else if (em->lastAttackKind == 5) {
                    if (broke) {
                        result = 16;
                    } else {
                        result = 15;
                    }
                } else if (broke) {
                    result = 12;
                } else {
                    if (em->hitReactMode == EM_HIT_REACT_NO_FLINCH) {
                        result = 13;
                    } else {
                        if (Em_IsHeld(em) == 1) {
                            return 0;
                        }
                        result = 14;
                        Em_SelectSightRow(em, 0);
                        Em_LoadReactionSet(em, 0);
                        if (*out == 0 || em->hitReactMode != EM_HIT_REACT_FLINCH) {
                            Em_RestartScript(em);
                            em->scriptPending = 1;
                        }
                    }
                }
            }
        } else {
            return 0;
        }
    }
end:
    if (result != 14) {
        em->scriptPending = 0;
        Em_RestartScript(em);
        Player_ClearFlag((Player *)em, PLF_NO_ROOT_MOVE);
    }
    Em_CheckKindRequest(em);
    return result;
}
