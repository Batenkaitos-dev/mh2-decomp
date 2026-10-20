/*
 * The player system: setting the hunters up and their per-frame upkeep.
 *
 * gPlayers holds the hunters (Player, player.h); gPlayerSys is the system's
 * own state, which PlayerSys_Update steps: the first frame sets every
 * active hunter up (Player_InitAll), then each frame updates them
 * (Game_UpdatePlayers). A hunter's set-up (Player_Init) gives it its
 * handlers (gPlayerHandlers: Player_MotionFxStep, which runs the motions'
 * sounds and effects) and resets
 * it (Player_Reset): at its start point or where it entered the area, with
 * its health, stamina, sharpness and ammo from its equipment and skills.
 * Each frame Player_TickTimers counts its timers down and Player_UpdateStatus
 * runs its status: stamina and health regeneration, the drinks and foods'
 * timed effects, the heat and the cold, the statuses' ends.
 * Player_UpdateMotionStep sets the speed of its motion layers.
 *
 * The data at the end: the player handler table, the equipment id bounds,
 * the preset equipment of the training quests, the start points and the
 * start item lists.
 */
#include "common.h"
#include "stageobj.h"
#include "player.h"
#include "scene.h"
#include "item.h"
#include "objhead.h"
#include "sprite.h"
#include "qsync.h"
#include "shadow.h"

/* An entry of a start item list (gStartItemLists, gStartItemsByClass): n of item; n == -1 ends
 * the list. */
typedef struct StartItem {
    s16 n;
    u16 item;
} StartItem;

/* The player system's state (gPlayerSys, 0x10 bytes). */
typedef struct PlayerSys {
    s8 state;                   /* 0x0: 0 set the hunters up, 1 run them; 2 and 3 idle */
    s8 unk1;                    /* 0x1: cleared by Player_InitAll; nothing reads it */
    u8 unk2[0x10 - 0x2];
} PlayerSys;

/* A hunter's offset from the start point (gPlayerSpawnOffset, by player index). */
typedef struct SpawnOffset {
    f32 x;
    f32 z;
} SpawnOffset;

extern Vec3f gStartPos[];
extern u16 gStartAngle[];
extern Vec3f gVersusStartPos[];
extern u16 gVersusStartAngle[];
extern Vec3f gPlayerResetPos[];
extern s32 gPlayerResetRotY[];

extern StartItem *gStartItemLists[];
extern StartItem *gTrainingStartItems[];
extern StartItem *gStartItemsByClass[];
extern u16 gPartIdMax[];
extern u32 gPresetHairColors[];
extern s8 gPresetEquipIds[];
extern PlayerVt *gPlayerHandlers;
extern SpawnOffset gPlayerSpawnOffset[];
extern PlayerSys gPlayerSys;
extern BladeData gBladeData[];

void *memset(void *p, s32 c, u32 n);
u16 Rand_Next(s32 stream);
s32 Pouch_SetAtLeast(Player *p, u16 mode, u16 item, s16 n);
s32 Player_IsLocal(void *obj);
s32 Player_HasSkill(Player *p, u8 skill);
void Player_CalcSkills(Player *pl);
u32 Player_TestFlag(void *obj, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
void Player_ClearFlag(void *obj, u32 flag);
s32 Quest_IsDrill(void);
s32 Quest_IsTraining(void);
void Player_SetColorWord(Player *p, u8 i);
void Player_LoadWeaponModel(s32 kind, s32 pl);
void Player_LoadPartModels(Player *p);
void Player_ClearFxBlock(Player *p);
DrawSlot *DrawSlot_Get(s32 i);
s16 DrawSlot_Alloc(void);
void DrawCb_LocalPlayer(DrawSlot *slot);
void Shadow_Spawn(void *obj, s16 type);
void Effect06_SpawnAtBone(Player *p, s16 type, u8 sub, s16 bone, f32 scale);
void Hit_ResetAll(void);
void QSync_SendPlayer(Player *p, s32 kind, u16 mode);
struct StageObj *WeaponTrail_Create(void *owner, s32 a, s32 b);
struct Em;
void DemonGlow_Create(struct Em *em, u8 k); /* game em_bulldrome.c; called here with a Player */
void BowArc_Create(void *owner);
void PlayerFx_CreateOnPlayerDefault(Player *p, u8 k, f32 scale);
void Game_UpdatePlayers(void);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
s32 Debug_IsOn(s32 unused0);
/* The definition takes (Player *, s32 frame, s32 n, s32 part); Player_Reset's and
 * Player_UpdateMotionStep's calls compile as the game's only with these u16 and s16
 * parameters. */
/* unit's own declaration: kept for the match */
void Player_BeginLayerMotion(Player *p, u16 motion, s16 a, s32 layer);
s32 Player_UpdateMotion(Player *p);
void Player_InitAll(s16 mode);
void Player_Init(Player *p, u8 idx, PlayerVt *vt);
void Player_Reset(Player *p);
void Player_UpdateStatus(Player *p);
void Player_TickPaint(Player *p);
s32 Quest_IsVersus(void);
void Player_CheckTeamCombo(Player *self);
void Player_AddMaxStamina(Player *p, s16 n);
u8 Player_GetSharpnessLevel(Player *p);
void Pouch_SelectFirst(Player *p);
void Player_InitRecords(Player *p);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
void Player_ResetAttackPercent(Player *p);
u16 Bowgun_NextAmmoSlot(Player *p, u16 slot, u8 dir);
void Bowgun_LoadAmmo(Player *p, s32 a);
void HuntingHorn_ClearNotes(Player *p);
void Player_ClearSongEffects(Player *p);
void Player_StartIdleMotion(Player *p, s32 a, s32 b);
void Player_ClearActionState(Player *p);
void Player_ResetAim(Player *p);
void Vec_Copy(f32 *dst, f32 *src);
void Player_CalcResists(Player *p);
void Player_ClearLightColors(Player *p);
void Player_UpdateAreaPos(void *obj);
void Player_CheckAreaMsgEvent(Player *p);
s32 Quest_PhaseIsOver(s32 a);
u8 Notice_Push(u8 kind, s16 a, s16 b);
s16 Area_Temperature(u16 area);
void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void Player_AddStamina(Player *p, s16 n);
void Player_UseStamina(Player *p, s32 n);
void Player_UpdateHunger(Player *p);
s32 Player_InEmAura(Player *p);
/* The definition returns u8; Player_UpdateStatus compiles as the game's only with the s32
 * return. */
/* unit's own declaration: kept for the match */
s32 Player_GetSkill(Player *p, u8 group);
void Player_AddHp(Player *p, s16 n);
s16 Player_IsState(void *obj, u8 state, u8 sub);
u16 Pouch_FindCarryItem(Player *p);
void Jingle_CarryLost(Player *p);
u8 Pouch_Add(Player *p, u16 item, s16 n);
extern s16 D_005B396C;

/* Fills the pouch from a start item list when Player.startItemList is set (list + 1; cleared
 * here): the drill quests' list, the training quests' list, or the weapon type's. */
void Pouch_SetStartItems(Player *pl)
{
    StartItem *entry;
    s16 i;
    u16 item;
    s16 n;

    if (pl->startItemList != 0) {
        pl->startItemList--;
        for (i = 0; i < 20; i++) {
            pl->pouch[i].item = 0;
            pl->pouch[i].count = 0;
        }
        if (Quest_IsDrill() == 1) {
            entry = gStartItemLists[pl->startItemList];
        } else if (Quest_IsTraining() == 1) {
            entry = gTrainingStartItems[pl->startItemList];
        } else {
            entry = gStartItemsByClass[pl->weaponType];
        }
        while ((n = entry->n) != -1) {
            item = entry->item;
            entry++;
            Pouch_SetAtLeast(pl, 0, item, n);
        }
    }
}

/* Gets a hunter ready for the quest: a slot with no player that isn't the local one is turned
 * off; otherwise the motion layers, the preset equipment of the training quests (an id below 0
 * picks a random part, up to gPartIdMax), the hair colour and the skills, then the models. */
void Player_InitForQuest(Player *pl)
{
    s32 max;
    u32 i;
    s8 *presetIds;

    if (gScene.inQuest[pl->index] == IN_QUEST_NO_PLAYER && Player_IsLocal(pl) == 0) {
        pl->active = 0;
        pl->visible = 0;
        return;
    }
    pl->isEm = 0;
    pl->ownTask = 0;
    pl->motionLayerCount = 2;
    pl->unk310 = 0;
    pl->unk311 = -1;
    if (pl->presetEquip != 0) {
        if (Quest_IsDrill() == 0 && Quest_IsTraining() == 0) {
            presetIds = &gPresetEquipIds[pl->presetEquip * 6 + pl->gender * 36];
            for (i = 0; i < 6; i++, presetIds++) {
                if (*presetIds < 0) {
                    max = gPartIdMax[i + pl->gender * 7];
                    pl->equip[i].id = Rand_Next(1) % max + 1;
                } else {
                    pl->equip[i].id = *presetIds;
                }
            }
            pl->hairColor = gPresetHairColors[pl->presetEquip + pl->gender * 6] | 0xFF000000;
            Player_SetColorWord(pl, 2);
        }
        Player_CalcSkills(pl);
    }
    Player_LoadWeaponModel(pl->weaponKind, pl->index);
    Player_LoadPartModels(pl);
    Player_ClearFxBlock(pl);
}

/* Clears the ten Player records and the player system. */
void Player_ClearAll(void)
{
    s32 i;

    for (i = 0; i < 10; i++) {
        memset(&gPlayers[i], 0, sizeof(Player));
    }
    memset(&gPlayerSys, 0, sizeof(gPlayerSys));
}

/* The player system's frame: sets every hunter up the first time (Player_InitAll), then runs
 * them (Game_UpdatePlayers). */
void PlayerSys_Update(void)
{
    switch (gPlayerSys.state) {
    case 0:
        Player_InitAll(0);
    case 1:
        Game_UpdatePlayers();
        break;
    case 2:
    case 3:
        break;
    }
}

/* Sets up the four hunters: a draw slot each, and with mode 0 (the quest's start) every one,
 * with mode 1 those moving between areas (Player.areaMoveState). At the start the local one
 * keeps its start point and sends it (QSync_SendPlayer), and each gets its spawn effects and
 * its weapon's objects (the dual swords' trail, the bow's arc, the horn's). */
void Player_InitAll(s16 mode)
{
    s32 i;
    Player *pl;

    gPlayerSys.unk1 = 0;
    gPlayerSys.state = 1;
    for (i = 0, pl = gPlayers; i < 4; i++, pl++) {
        if (pl->active) {
            pl->drawSlotId = DrawSlot_Alloc();
            if (pl->drawSlotId != -1) {
                pl->drawSlot = DrawSlot_Get(pl->drawSlotId);
                pl->drawSlot->player = pl->index;
                pl->drawSlot->draw = DrawCb_LocalPlayer;
            }
            if (mode == 0 || (mode == 1 && pl->areaMoveState != PL_AREA_MOVE_NONE)) {
                Player_Init(pl, i, gPlayerHandlers);
            }
            if (mode == 0) {
                if (Player_IsLocal(pl) == 1) {
                    pl->nextArea = pl->area;
                    pl->startPos[0] = pl->pos[0];
                    pl->startPos[1] = pl->pos[1];
                    pl->startPos[2] = pl->pos[2];
                    pl->startAngle = pl->rot[1];
                    QSync_SendPlayer(pl, QSYNC_PL_ENTER_AREA, 0);
                }
                Shadow_Spawn(pl, SHADOW_PLAYER);
                Effect06_SpawnAtBone(pl, 7, 0, 1, 1.0f);
                switch (pl->weaponType) {
                case WEAPON_DUAL_SWORDS:
                    WeaponTrail_Create(pl, WTRAIL_LIFE_FOREVER, WTRAIL_DEMON);
                    DemonGlow_Create((struct Em *)pl, 0);
                    break;
                case WEAPON_BOW:
                    BowArc_Create(pl);
                    break;
                case WEAPON_HUNTING_HORN:
                    PlayerFx_CreateOnPlayerDefault(pl, 7, 1.0f);
                    break;
                }
            }
        }
    }
    Hit_ResetAll();
}

/* Sets hunter idx up: visible if it is in the quest or the local one, its handler table, reset
 * (Player_Reset); then runs its first and last handlers. */
void Player_Init(Player *pl, u8 idx, PlayerVt *table)
{
    pl->index = idx;
    if (gScene.inQuest[pl->index] == IN_QUEST_YES || Player_IsLocal(pl) == 1) {
        pl->visible = 1;
    } else {
        pl->visible = 0;
    }
    pl->handlers = table;
    Player_Reset(pl);
    pl->motion[0].unk04 = idx + 1;
    pl->handlers->init(pl);
    pl->handlers->motionFxStep(pl);
}

/* Offline, moves pos by the hunter's spawn offset (gPlayerSpawnOffset by player index), turned by
 * angle, so that the hunters don't start on one spot. */
void Player_AddSpawnOffset(Player *pl, f32 *pos, u16 angle)
{
    s32 rot[3];
    f32 offset[3];
    f32 out[3];
    f32 mtx[4][4];
    u8 idx;

    if (gScene.inLobby == 0) {
        idx = pl->index;
        rot[1] = angle;
        rot[0] = 0;
        rot[2] = 0;
        Mtx_FromAnglesXYZ(rot, (f32 *) mtx);
        offset[0] = gPlayerSpawnOffset[idx].x;
        offset[1] = 0.0f;
        offset[2] = gPlayerSpawnOffset[idx].z;
        Vec_Rotate((f32 *) out, (f32 *) offset, (f32 *) mtx);
        pos[0] += out[0];
        pos[2] += out[2];
    }
}

/* The maximum health the health skills add: +10, +20, +30, -10, -20 or -30 (the first skill held,
 * in that order), else 0. */
s32 Player_GetHpSkillBonus(Player *pl)
{
    if (Player_HasSkill(pl, SKILL_HEALTH_PLUS10) == 1) {
        return 10;
    }
    if (Player_HasSkill(pl, SKILL_HEALTH_PLUS20) == 1) {
        return 20;
    }
    if (Player_HasSkill(pl, SKILL_HEALTH_PLUS30) == 1) {
        return 30;
    }
    if (Player_HasSkill(pl, SKILL_HEALTH_MINUS10) == 1) {
        return -10;
    }
    if (Player_HasSkill(pl, SKILL_HEALTH_MINUS20) == 1) {
        return -20;
    }
    if (Player_HasSkill(pl, SKILL_HEALTH_MINUS30) == 1) {
        return -30;
    }
    return 0;
}

/* Player.sharpnessMax from the blade's sharpMax (150 + 50 a step), with the sharpness skills'
 * +50 (to at most 400) or -50 (to at least 150). */
void Player_CalcSharpnessMax(Player *pl)
{
    pl->sharpnessMax = gBladeData[pl->equip[EQUIP_SLOT_WEAPON].id].sharpMax * 50 + 150;
    if (Player_HasSkill(pl, SKILL_SHARPNESS_PLUS50) == 1) {
        pl->sharpnessMax += 50;
        if (pl->sharpnessMax > 400) {
            pl->sharpnessMax = 400;
        }
    } else if (Player_HasSkill(pl, SKILL_SHARPNESS_MINUS50) == 1) {
        pl->sharpnessMax -= 50;
        if (pl->sharpnessMax < 150) {
            pl->sharpnessMax = 150;
        }
    }
}

/*
 * Resets a hunter by Player.areaMoveState. PL_AREA_MOVE_NONE, the quest's start: at its
 * start point (the versus side's, or the area's with Player_AddSpawnOffset), its health and
 * stamina maxima from the meal and the skills, sharpness, the pouch and the ammo, then idle.
 * PL_AREA_MOVE_ENTER, entering another area: at the point it entered, keeping its carried
 * item or its status. PL_AREA_MOVE_CART and PL_AREA_MOVE_RESPAWN: back in the area after a
 * faint (the cart at the map's reset point into an event state, or on its feet at its start
 * point). Then clears the per-state fields.
 */
void Player_Reset(Player *pl)
{
    f32 offset[3];
    f32 out[3];
    s32 rot[3];
    f32 mtx[4][4];
    s32 side;
    u8 flags;
    s32 i;

    *(u32 *)&pl->unk004 = 0;
    pl->unk2B4 = 0;
    *(u16 *)pl->motionStarted = 0;
    pl->scale[0] = 1.0f;
    pl->scale[1] = 1.0f;
    pl->scale[2] = 1.0f;
    switch (pl->areaMoveState) {
    case PL_AREA_MOVE_NONE:
        if (Quest_IsVersus() == 1) {
            side = (u8)((pl->side > 0) ? (u8)(pl->side - 1) : 0) & 1;
            pl->pos[0] = gVersusStartPos[side].x;
            pl->pos[1] = gVersusStartPos[side].y;
            pl->pos[2] = gVersusStartPos[side].z;
            pl->rot[1] = gVersusStartAngle[side];
            pl->rot[0] = 0;
            pl->rot[2] = 0;
            Mtx_FromAnglesXYZ(pl->rot, pl->rotMtx);
            if (pl->side >= 3) {
                rot[0] = 0;
                rot[1] = pl->rot[1];
                rot[2] = 0;
                Mtx_FromAnglesXYZ(rot, (f32 *) mtx);
                offset[0] = 80.0f;
                offset[1] = 0.0f;
                offset[2] = 80.0f;
                Vec_Rotate((f32 *) out, (f32 *) offset, (f32 *) mtx);
                pl->pos[0] += out[0];
                pl->pos[2] += out[2];
            }
        } else {
            pl->pos[0] = gStartPos[gScene.area].x;
            pl->pos[1] = gStartPos[gScene.area].y;
            pl->pos[2] = gStartPos[gScene.area].z;
            pl->rot[1] = gStartAngle[gScene.area];
            pl->rot[0] = 0;
            pl->rot[2] = 0;
            Mtx_FromAnglesXYZ(pl->rot, pl->rotMtx);
            Player_AddSpawnOffset(pl, pl->pos, pl->rot[1]);
        }
        if (Player_IsLocal(pl) == 1) {
            Player_CheckTeamCombo(pl);
        } else {
            pl->teamSkill = 0;
            pl->teamSkillGroup = 0;
        }
        pl->hpMax = 100;
        pl->staminaMax = 300;
        pl->attackDrug = 0;
        pl->defenseDrug = 0;
        pl->attackBuff = 0;
        pl->defenseBuff = 0;
        pl->attackBuffTimer = 0;
        pl->defenseBuffTimer = 0;
        if (Player_IsLocal(pl) == 1) {
            pl->hpMax += gScene.mealHealth;
            pl->hpMax = pl->hpMax + Player_GetHpSkillBonus(pl);
            if (pl->hpMax > 150) {
                pl->hpMax = 150;
            }
            Player_AddMaxStamina(pl, gScene.mealStamina);
            pl->attackDrug = gScene.mealAttack;
            pl->defenseDrug = gScene.mealDefense;
        }
        pl->hp = pl->hpMax;
        pl->hpRecoverable = pl->hpMax;
        pl->hpMaxBase = pl->hpMax;
        pl->staminaTimer = 10800;
        pl->stamina = pl->staminaMax;
        Player_CalcSharpnessMax(pl);
        pl->sharpness = pl->sharpnessMax;
        pl->sharpnessLevel = Player_GetSharpnessLevel(pl);
        pl->staminaLockTimer = 0;
        pl->coolDrinkTimer = 0;
        pl->hotDrinkTimer = 0;
        pl->detectTimer = 0;
        pl->actDetectTimer = 0;
        pl->regenTimer = 0;
        pl->status5Delay = 0;
        pl->status5Buildup = 0;
        pl->poison = 0;
        pl->mashLeft = 0;
        pl->mashBuildup = 0;
        pl->status6Timer = 0;
        pl->status7Timer = 0;
        pl->status2Buildup = 0;
        pl->status4Buildup = 0;
        pl->recoverTimer = 0;
        pl->questStartDone = 0;
        pl->swordGaugeTimer = 0;
        pl->swordGauge = 0;
        pl->attackFactor = 1.0f;
        pl->defenseFactor = 1.0f;
        pl->scoutSpotted = 0;
        Pouch_SelectFirst(pl);
        pl->faints = 0;
        pl->weaponSeMute = 0;
        pl->tradeCount = 0;
        pl->tradeCooldown = 0;
        pl->weaponOut = 0;
        pl->roundsLoaded = 0;
        pl->clipSize = 0;
        pl->spotCheckDelay = 0;
        pl->unk8B3 = 0;
        Player_InitRecords(pl);
        if (gScene.inQuest[pl->index] == IN_QUEST_YES || Player_IsLocal(pl) == 1) {
            pl->stateGroup = 0;
            pl->state = 0;
        } else {
            Player_SetState(pl, PL_GROUP_EVENT, PL_EVENT_LEAVE, 2);
        }
        pl->syncTimer = 0;
        pl->useItem = 0;
        pl->fieldCollide = 1;
        pl->supplyTaken = 0;
        Pouch_SetStartItems(pl);
        Player_ResetAttackPercent(pl);
        switch (pl->weaponType) {
        case WEAPON_GUNLANCE:
            switch (pl->bladeSpecial % 3) {
            case 0:
                pl->ammoCount = 5;
                break;
            case 1:
                pl->ammoCount = 2;
                break;
            case 2:
                pl->ammoCount = 3;
                break;
            }
            pl->clipSize = pl->ammoCount;
            break;
        case WEAPON_BOW:
            pl->unk7CC = 0x87;
            pl->unk7CE = 0xFF;
            pl->sharpnessLevel = 3;
            break;
        }
        pl->ammoSlot = Bowgun_NextAmmoSlot(pl, 0, 2);
        pl->ammoCount = pl->pouch[pl->ammoSlot].count;
        Bowgun_LoadAmmo(pl, 0);
        pl->coatingSlot = POUCH_SLOT_NONE;
        HuntingHorn_ClearNotes(pl);
        Player_ClearSongEffects(pl);
        Player_StartIdleMotion(pl, 0, 0);
        break;
    case PL_AREA_MOVE_ENTER:
        pl->pos[0] = pl->startPos[0];
        pl->pos[1] = pl->startPos[1];
        pl->pos[2] = pl->startPos[2];
        pl->rot[1] = pl->startAngle;
        Mtx_FromAnglesXYZ(pl->rot, pl->rotMtx);
        pl->areaMoveState = PL_AREA_MOVE_NONE;
        pl->spotCheckDelay = 0;
        if (gScene.inQuest[pl->index] == IN_QUEST_YES || Player_IsLocal(pl) == 1) {
            pl->stateGroup = 0;
            pl->state = 0;
        } else {
            Player_SetState(pl, PL_GROUP_EVENT, PL_EVENT_LEAVE, 2);
        }
        pl->syncTimer = 0;
        flags = pl->carry;
        if (flags & PL_CARRY_HOLD_MASK) {
            Player_ClearActionState(pl);
            pl->carry = (u8)(flags & PL_CARRY_KIND_MASK) | PL_CARRY_HOLD_ENTER;
            Player_SetState(pl, PL_GROUP_CARRY, PL_CARRY_IDLE, 0);
        } else if (pl->mashLeft > 0) {
            Player_SetState(pl, PL_GROUP_REACT, PL_REACT_MASH_IDLE, 0xC);
        } else {
            Player_ClearActionState(pl);
            Player_StartIdleMotion(pl, 0, 0);
        }
        break;
    case PL_AREA_MOVE_CART:
    case PL_AREA_MOVE_RESPAWN:
        if (pl->areaMoveState == PL_AREA_MOVE_CART) {
            pl->pos[0] = gPlayerResetPos[gScene.map].x;
            pl->pos[1] = gPlayerResetPos[gScene.map].y;
            pl->pos[2] = gPlayerResetPos[gScene.map].z;
            pl->rot[1] = gPlayerResetRotY[gScene.map];
        } else {
            pl->pos[0] = pl->startPos[0];
            pl->pos[1] = pl->startPos[1];
            pl->pos[2] = pl->startPos[2];
            pl->rot[1] = pl->startAngle;
        }
        Player_AddSpawnOffset(pl, pl->pos, pl->rot[1]);
        Mtx_FromAnglesXYZ(pl->rot, pl->rotMtx);
        pl->floorY = pl->pos[1];
        pl->targetYaw = pl->rot[1];
        Player_CalcSharpnessMax(pl);
        pl->hpMax = 100;
        if (Player_IsLocal(pl) == 1) {
            pl->hpMax = pl->hpMax + Player_GetHpSkillBonus(pl);
            if (pl->hpMax > 150) {
                pl->hpMax = 150;
            }
        }
        pl->hp = pl->hpMax;
        pl->hpRecoverable = pl->hpMax;
        pl->hpMaxBase = pl->hpMax;
        pl->staminaMax = 300;
        pl->staminaTimer = 10800;
        pl->stamina = pl->staminaMax;
        pl->staminaLockTimer = 0;
        pl->coolDrinkTimer = 0;
        pl->hotDrinkTimer = 0;
        pl->regenTimer = 0;
        pl->detectTimer = 0;
        pl->actDetectTimer = 0;
        pl->status5Delay = 0;
        pl->status5Buildup = 0;
        pl->poison = 0;
        pl->status6Timer = 0;
        pl->status7Timer = 0;
        pl->mashLeft = 0;
        pl->mashBuildup = 0;
        pl->status2Buildup = 0;
        pl->status4Buildup = 0;
        pl->recoverTimer = 0;
        pl->spotCheckDelay = 20;
        pl->visible = 0;
        pl->attackDrug = 0;
        pl->defenseDrug = 0;
        pl->attackBuff = 0;
        pl->defenseBuff = 0;
        pl->attackBuffTimer = 0;
        pl->defenseBuffTimer = 0;
        pl->weaponSeMute = 0;
        pl->weaponOut = 0;
        HuntingHorn_ClearNotes(pl);
        Player_ClearSongEffects(pl);
        if (pl->areaMoveState == PL_AREA_MOVE_CART) {
            Player_SetState(pl, PL_GROUP_EVENT, PL_EVENT_CART, 0xC);
        } else {
            pl->visible = 1;
            pl->stateGroup = 0;
            pl->state = 0;
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_IDLE, 0xC);
            Player_ClearActionState(pl);
            Player_StartIdleMotion(pl, 0, 0);
        }
        pl->areaMoveState = PL_AREA_MOVE_NONE;
        break;
    }
    pl->lookFound = 0;
    pl->targetYaw = pl->rot[1];
    pl->lean = 0;
    pl->lookYaw = 0;
    pl->hitTimers = &pl->hitDelay;
    pl->action = PL_POSTURE_STANDING;
    pl->ammoMenuOpen = 0;
    pl->unk4C4 = 0;
    pl->rideState = 0;
    pl->unk58C = -1;
    pl->unk58E = -1;
    pl->unk3F8 = 0;
    pl->painted = 0xFF;
    pl->alpha = 1.0f;
    pl->fadeAlpha = 1.0f;
    pl->weaponShown = 1;
    pl->supplyBoxOpen = 0;
    pl->deliveryMenu = 0;
    pl->menuPage = 0;
    pl->deliveryOpen = 0;
    pl->menuDelay = 0;
    pl->menuCursor = 0;
    pl->actRequest = 0;
    pl->offerRequest = 0;
    pl->teamItemMask = 0;
    pl->tradeMsgTimer = 0;
    pl->offerNoticeTimer = 0;
    pl->demonMode = 0;
    pl->noFlinchTimer = 0;
    pl->noFlinchItemTimer = 0;
    pl->motionTime = 0;
    pl->noHitTimer = 30;
    pl->motion[0].step = 2.0f;
    pl->motion[1].step = 2.0f;
    pl->bowgunReady = 0;
    pl->freeState = 0;
    pl->prevFreeState = 0;
    pl->pouchHudFlags = 0;
    pl->itemSelect = 0;
    pl->pushPriority = 1;
    pl->unk417 = 0;
    pl->dashDelay = 0;
    pl->itemUseDelay = 0;
    pl->ammoMenuLock = 0;
    pl->hideShadow = 0;
    pl->envTimer = 90;
    pl->motionSpeed = 1.0f;
    pl->inPoisonArea = 0;
    Player_ResetAim(pl);
    pl->moveTo[0] = 0.0f;
    pl->moveTo[1] = 0.0f;
    pl->moveTo[2] = 0.0f;
    Vec_Copy(pl->moveToStep, pl->moveTo);
    pl->moveToFrames = 0;
    for (i = 0; i < 4; i++) {
        pl->partAnimOn[i] = 0;
        pl->partAnimMotion[i] = 0;
        pl->partAnimWeight[i] = 0;
    }
    pl->aimWeight = 0;
    Player_BeginLayerMotion(pl, pl->motionFrame[0], pl->motionBlend[0], 0);
    Player_BeginLayerMotion(pl, pl->motionFrame[1], pl->motionBlend[1], 1);
    Player_UpdateMotion(pl);
    Player_CalcResists(pl);
    Player_ClearLightColors(pl);
    Player_UpdateAreaPos(pl);
    Player_CheckAreaMsgEvent(pl);
}

/* Three Player timers as Player_UpdateStatus reads them, as u16 (Player has them as s16):
 * kept for the match, which needs the unsigned loads. */
typedef struct PlayerTimers {
    u8 pad000[0x62A];
    u16 attackBuffTimer;        /* 0x62A: Player.attackBuffTimer */
    u8 pad62C[0x88C - 0x62C];
    u16 detectTimer;            /* 0x88C: Player.detectTimer */
    u8 pad88E[0x8BA - 0x88E];
    u16 actDetectTimer;         /* 0x8BA: Player.actDetectTimer */
} PlayerTimers;

/* Counts the player's timers down each frame, regenerates stamina and health,
 * and ends timed effects. Case 2's test of the state is a switch with its
 * default first: that leaves every conditional branch delay slot of the
 * function empty, as in the original. */
void Player_UpdateStatus(Player *pl)
{
    PlayerTimers *timers = (PlayerTimers *)pl;
    u8 rate;
    s16 interval;
    s16 amount;
    s16 diff;
    u16 item;
    u8 flags;
    u8 holdLeft;
    s16 i;

    if (pl->noFlinchItemTimer != 0) {
        pl->noFlinchItemTimer--;
    }
    if (pl->offerNoticeTimer != 0) {
        pl->offerNoticeTimer--;
    }
    if (pl->weaponSeMute != 0) {
        pl->weaponSeMute--;
    }
    if (pl->unk4C1 != 0) {
        pl->unk4C1--;
    }
    if (pl->tradeMsgTimer != 0) {
        pl->tradeMsgTimer--;
    }
    if (pl->tradeCooldown > 0) {
        pl->tradeCooldown--;
        if (pl->tradeCooldown <= 0) {
            pl->tradeCooldown = 0;
            pl->tradeCount = 0;
        }
    }
    if (pl->dashDelay != 0) {
        pl->dashDelay--;
    }
    if (pl->staminaLockTimer != 0) {
        pl->staminaLockTimer--;
        if (pl->staminaLockTimer == 0 && Player_IsLocal(pl) == 1 && Quest_PhaseIsOver(1) == 0) {
            Notice_Push(0, 0x15, 0);
        }
    }
    if (pl->coolDrinkTimer != 0) {
        if (Area_Temperature(pl->area) == 3) {
            if (pl->coolDrinkTimer < 2) {
                pl->coolDrinkTimer = 0;
            } else {
                pl->coolDrinkTimer -= 2;
            }
        } else {
            pl->coolDrinkTimer--;
        }
        if (pl->coolDrinkTimer == 0 && Player_IsLocal(pl) == 1 && Quest_PhaseIsOver(1) == 0) {
            Notice_Push(0, 0x12, 0);
        }
    }
    if (pl->hotDrinkTimer != 0) {
        if (Area_Temperature(pl->area) == 4) {
            if (pl->hotDrinkTimer < 2) {
                pl->hotDrinkTimer = 0;
            } else {
                pl->hotDrinkTimer -= 2;
            }
        } else {
            pl->hotDrinkTimer--;
        }
        if (pl->hotDrinkTimer == 0 && Player_IsLocal(pl) == 1 && Quest_PhaseIsOver(1) == 0) {
            Notice_Push(0, 0x13, 0);
        }
    }
    if (pl->regenTimer != 0) {
        pl->regenTimer--;
        if (pl->regenTimer == 0 && Player_IsLocal(pl) == 1 && Quest_PhaseIsOver(1) == 0) {
            Notice_Push(0, 0x16, 0);
        }
    }
    if (timers->detectTimer != 0) {
        timers->detectTimer--;
    }
    if (timers->actDetectTimer != 0) {
        timers->actDetectTimer--;
    }
    if (timers->attackBuffTimer != 0) {
        timers->attackBuffTimer--;
        if (timers->attackBuffTimer == 0) {
            pl->attackBuff = 0;
        }
    }
    if (pl->defenseBuffTimer != 0) {
        pl->defenseBuffTimer--;
        if (pl->defenseBuffTimer == 0) {
            pl->defenseBuff = 0;
        }
    }
    if (pl->mashLeft != 0) {
        pl->mashLeft--;
    }
    if (pl->swordGaugeTimer != 0) {
        pl->swordGaugeTimer--;
    }
    if (pl->status6Timer > 0) {
        pl->status6Timer--;
        if (pl->status6Timer < 0) {
            pl->status6Timer = 0;
        }
    }
    if (pl->status7Timer > 0) {
        pl->status7Timer--;
        if (pl->status7Timer < 0) {
            pl->status7Timer = 0;
        }
    }
    if (pl->weaponType == WEAPON_LONG_SWORD && pl->swordGaugeTimer == 0 && pl->swordGauge != 0 && !(gScene.frame & 7)) {
        pl->swordGauge--;
    }
    if (pl->weaponType == WEAPON_GUNLANCE && pl->weaponTimer > 0) {
        pl->weaponTimer--;
        if (pl->weaponTimer == 0) {
            pl->unk8B3 = 0;
            Player_PlaySe(pl, 0x15, 0, pl->pos, 1, 0);
            if (Player_IsLocal(pl) == 1) {
                Sound_PlaySeFlat(0, 0x21, 0);
            }
            pl->swordGaugeTimer = 10;
        } else {
            pl->unk8B3 = 1;
        }
    }
    if (Player_TestFlag(pl, 0x200) == 0 && pl->demonMode == 0) {
        switch (pl->stateGroup) {
        case 0:
            if (pl->state != 0x6E && pl->state != 0x6D && pl->state != 0x6C && pl->state != 0x6B &&
                pl->state != 0x44 && pl->state != 0x32 && pl->state != 0x31 && pl->state != 0x2D &&
                pl->state != 0x1C && pl->state != 0x78 && pl->state != 0x77) {
                Player_AddStamina(pl, 2);
            }
            break;
        case 1:
            switch (pl->weaponType) {
            case WEAPON_HAMMER:
                switch (pl->state) {
                default:
                    Player_AddStamina(pl, 2);
                    break;
                case 3:
                case 0xC:
                case 0xD:
                case 0xE:
                case 4:
                    break;
                }
                break;
            case WEAPON_LANCE:
                if (pl->state != 0x10 && pl->state != 5 && pl->state != 0xE && pl->state != 3) {
                    Player_AddStamina(pl, 2);
                }
                break;
            case WEAPON_BOW:
                if (pl->state != 0xA && pl->state != 8 && pl->weaponTimer == 0) {
                    Player_AddStamina(pl, 2);
                }
                break;
            default:
                Player_AddStamina(pl, 2);
                break;
            }
            break;
        case 2:
        case 3:
            break;
        default:
            Player_AddStamina(pl, 2);
            break;
        }
    }
    if (pl->demonMode != 0) {
        Player_UseStamina(pl, -1);
        if (pl->stamina <= 0 && pl->stateGroup != PL_GROUP_WEAPON) {
            pl->demonMode = 0;
        }
    }
    Player_UpdateHunger(pl);
    if (pl->stateGroup != PL_GROUP_REACT && pl->stateGroup != PL_GROUP_FAINT) {
        if (pl->coolDrinkTimer == 0) {
            switch (Area_Temperature(pl->area)) {
            case 1:
                if (Player_HasSkill(pl, SKILL_HEAT_RES) == 0 && Player_HasSkill(pl, SKILL_HEAT_RES2) == 0) {
                    goto no_regen; /* the heat stops the regeneration without the skill */
                }
                break;
            case 3:
                if (Player_HasSkill(pl, SKILL_HEAT_RES2) == 0) {
                    goto no_regen;
                }
                break;
            }
        }
        rate = pl->floorHazard;
        if (rate == 0 && Player_InEmAura(pl) == 1) {
            rate = 2;
        }
        if (rate == 1 && Player_HasSkill(pl, SKILL_POISON_IMMUNE) == 1) {
            rate = 0;
        }
        if (pl->poison == 0 && rate == 0) {
            interval = 60;
            if ((u8)Player_GetSkill(pl, 7)) {
                if (Player_HasSkill(pl, 0x1B) == 1) {
                    interval = 20;
                }
                if (Player_HasSkill(pl, 0x1C) == 1) {
                    interval = 15;
                }
                if (Player_HasSkill(pl, 0x1D) == 1) {
                    interval = 180;
                }
                if (Player_HasSkill(pl, 0x1E) == 1) {
                    interval = 240;
                }
            }
            amount = (pl->regenTimer != 0) ? 2 : 1;
            if (gScene.frame % interval == 0 && pl->hp < pl->hpRecoverable) {
                diff = pl->hpRecoverable - pl->hp;
                if (diff >= amount) {
                    Player_AddHp(pl, amount);
                } else {
                    Player_AddHp(pl, diff);
                }
            }
        }
no_regen:
        if (pl->status5Delay > 0) {
            pl->status5Delay--;
        } else {
            pl->status5Delay = 0;
            if (pl->status5Buildup != 0) {
                pl->status5Buildup--;
            }
        }
    }
    if (pl->aimLockTimer != 0) {
        pl->aimLockTimer--;
    }
    if (pl->ammoMenuLock != 0) {
        pl->ammoMenuLock--;
    }
    if (pl->itemUseDelay != 0) {
        pl->itemUseDelay--;
    }
    flags = pl->carry;
    holdLeft = flags & PL_CARRY_HOLD_MASK;
    if (holdLeft != 0 && pl->stateGroup == PL_GROUP_CARRY) {
        pl->carry = holdLeft;
        pl->carry--;
        pl->carry |= (u8)(flags & PL_CARRY_KIND_MASK);
    } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_GATHER) == 0 && Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_CARVE) == 0 &&
               Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MINE) == 0 && Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_GATHER_CROUCHED) == 0 &&
               Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_CARVE_CROUCHED) == 0) {
        item = Pouch_FindCarryItem(pl);
        if (item != 0xFFFF) {
            if (Player_IsLocal(pl) == 1 && Quest_PhaseIsOver(1) == 0) {
                Jingle_CarryLost(pl);
                Notice_Push(1, 5, item);
            }
            Pouch_Add(pl, item, -10);
        }
    }
    for (i = 0; i < 12; i++) {
        if (pl->songEffectTimer[i] > 0) {
            pl->songEffectTimer[i]--;
            if (pl->songEffectTimer[i] <= 0) {
                pl->songEffectTimer[i] = 0;
                if (i == 3) {
                    pl->hpMax = pl->hpMaxBase;
                    if (pl->hp >= pl->hpMax) {
                        pl->hp = pl->hpMax;
                        D_005B396C = pl->hp;
                    }
                    if (pl->hpRecoverable >= pl->hpMax) {
                        pl->hpRecoverable = pl->hpMax;
                    }
                }
            }
        }
    }
    if (pl->spotCheckDelay != 0) {
        pl->spotCheckDelay--;
    }
    if (pl->fishTimer != 0) {
        pl->fishTimer--;
    }
}

/* For a record with isEm set, instead of Player_UpdateStatus: counts the paintball's timer
 * down, then clears the paint (Player.painted 0xFF stays). */
void Player_TickPaint(Player *pl)
{
    if (pl->startItemList == 0 && pl->painted != 0xFF && pl->painted != 0) {
        if (pl->paintTimer != 0) {
            pl->paintTimer--;
        } else {
            pl->painted = 0;
        }
    }
}

/* Each frame, unless in hit stop: counts the hunter's timers down and runs its status
 * (Player_UpdateStatus); then updates the flag bits 0x800 and 0x2000 (0x2000 follows bit 1). */
void Player_TickTimers(Player *pl)
{
    if (pl->hitStop == 0) {
        if (Player_TestFlag(pl, 0x400001)) {
            pl->unk3A0 += 2;
        } else {
            pl->unk3A0 = 0;
        }
        pl->motionTime += 2;
        if ((s32)pl->motionTime >= 100000) {
            pl->motionTime = 10000;
        }
        if (pl->noHitTimer != 0) {
            pl->noHitTimer--;
        }
        if (pl->unk434 != 0) {
            pl->unk434--;
        }
        if (pl->noFlinchTimer != 0) {
            pl->noFlinchTimer--;
        }
        if (pl->noPushTimer != 0) {
            pl->noPushTimer--;
        }
        if (pl->hideShadow != 0) {
            pl->hideShadow--;
        }
        if (pl->isEm == 0) {
            Player_UpdateStatus(pl);
        } else {
            Player_TickPaint(pl);
        }
    }
    Player_ClearFlag(pl, 0x800);
    if (Player_TestFlag(pl, 1)) {
        Player_SetFlag(pl, 0x2000);
    } else {
        Player_ClearFlag(pl, 0x2000);
    }
}

/* Adds the pending hit stop of a hunter or monster (ObjHead.hitStopAdd) to its hit stop, else
 * counts the hit stop down. */
void Player_UpdateHitStop(void *obj)
{
    ObjHead *head = obj;
    if (head->hitStopAdd != 0) {
        head->hitStop += head->hitStopAdd;
        head->hitStopAdd = 0;
    } else if (head->hitStop != 0) {
        head->hitStop--;
    }
}

/* Sets the step of the hunter's two motion layers: 0 in hit stop, 0.2 while slowed by a hit, else
 * by the motion (some at least 2, three kept, the rest 2 times Player.motionSpeed); starts the
 * layers' new motions and steps them (unless the debug switch 0x18 is on). */
void Player_UpdateMotionStep(Player *pl)
{
    if (Debug_IsOn(0x18) == 0) {
        Mtx_FromAnglesXYZ(pl->rot, pl->rotMtx);
        if (pl->hitStop != 0) {
            pl->motion[0].step = 0.0f;
            pl->motion[1].step = 0.0f;
        } else if (pl->hitSlowTimer > 0) {
            pl->hitSlowTimer--;
            pl->motion[0].step = 0.2f;
            pl->motion[1].step = 0.2f;
        } else {
            pl->hitSlowTimer = 0;
            switch (pl->motionId[0]) {
            case 0x195:
            case 0x19B:
            case 0x19C:
            case 0x19D:
            case 0x19F:
            case 0x1A0:
            case 0x1A2:
            case 0x1A3:
            case 0x1A6:
            case 0x1A7:
                if (pl->motion[0].step <= 2.0f) {
                    pl->motion[0].step = 2.0f;
                    pl->motion[1].step = 2.0f;
                }
                break;
            case 3:
            case 0x22:
            case 0x23:
                break;
            default:
                pl->motion[0].step = 2.0f * pl->motionSpeed;
                pl->motion[1].step = 2.0f * pl->motionSpeed;
                break;
            }
        }
        if (pl->motionStarted[0] == 0) {
            pl->motionStarted[0]++;
            Player_BeginLayerMotion(pl, pl->motionFrame[0], pl->motionBlend[0], 0);
        }
        if (pl->motionStarted[1] == 0) {
            pl->motionStarted[1]++;
            Player_BeginLayerMotion(pl, pl->motionFrame[1], pl->motionBlend[1], 1);
        }
        Player_UpdateMotion(pl);
    }
}

/* The file's tables. */
void Player_NoHandler(Player *p);
void Player_MotionFxStep(Player *p);
PlayerVt gPlayerHandlerTable[2] = {
    {Player_NoHandler, Player_NoHandler, Player_NoHandler, Player_MotionFxStep},
    {NULL, NULL, NULL, NULL},
};
u16 gPartIdMax[16] = {
#include "sub_main/0044A1B0.inc"
};
u32 gArmourIdMax[4] = {
#include "sub_main/0044A1D0.inc"
};
u32 gWeaponClassInfo[8] = {
#include "sub_main/0044A1E0.inc"
};
u32 gFaceBodyTypeMale[8] = {
#include "sub_main/0044A200.inc"
};
u32 gFaceBodyTypeFemale[8] = {
#include "sub_main/0044A220.inc"
};
u32 D_0044A240[2] = {
#include "sub_main/0044A240.inc"
};
u32 D_0044A248[2] = {
#include "sub_main/0044A248.inc"
};
u32 D_0044A250[2] = {
#include "sub_main/0044A250.inc"
};
u32 D_0044A258[2] = {
#include "sub_main/0044A258.inc"
};
u32 D_0044A260[2] = {
#include "sub_main/0044A260.inc"
};
u32 D_0044A268[2] = {
#include "sub_main/0044A268.inc"
};
u32 D_0044A270[2] = {
#include "sub_main/0044A270.inc"
};
u32 D_0044A278[2] = {
#include "sub_main/0044A278.inc"
};
u32 D_0044A280[2] = {
#include "sub_main/0044A280.inc"
};
u32 D_0044A288[2] = {
#include "sub_main/0044A288.inc"
};
u32 D_0044A290[2] = {
#include "sub_main/0044A290.inc"
};
u32 D_0044A298[2] = {
#include "sub_main/0044A298.inc"
};
u32 D_0044A2A0[2] = {
#include "sub_main/0044A2A0.inc"
};
u32 D_0044A2A8[2] = {
#include "sub_main/0044A2A8.inc"
};
u32 D_0044A2B0[2] = {
#include "sub_main/0044A2B0.inc"
};
u32 D_0044A2B8[2] = {
#include "sub_main/0044A2B8.inc"
};
u32 D_0044A2C0[2] = {
#include "sub_main/0044A2C0.inc"
};
u32 D_0044A2C8[2] = {
#include "sub_main/0044A2C8.inc"
};
u32 D_0044A2D0[2] = {
#include "sub_main/0044A2D0.inc"
};
u32 D_0044A2D8[2] = {
#include "sub_main/0044A2D8.inc"
};
u32 D_0044A2E0[2] = {
#include "sub_main/0044A2E0.inc"
};
u32 D_0044A2E8[2] = {
#include "sub_main/0044A2E8.inc"
};
u32 D_0044A2F0[2] = {
#include "sub_main/0044A2F0.inc"
};
u32 D_0044A2F8[2] = {
#include "sub_main/0044A2F8.inc"
};
u32 D_0044A300[2] = {
#include "sub_main/0044A300.inc"
};
u32 D_0044A308[2] = {
#include "sub_main/0044A308.inc"
};
u32 D_0044A310[2] = {
#include "sub_main/0044A310.inc"
};
u32 D_0044A318[2] = {
#include "sub_main/0044A318.inc"
};
u32 D_0044A320[2] = {
#include "sub_main/0044A320.inc"
};
u32 D_0044A328[2] = {
#include "sub_main/0044A328.inc"
};
u32 D_0044A330[2] = {
#include "sub_main/0044A330.inc"
};
u32 D_0044A338[2] = {
#include "sub_main/0044A338.inc"
};
u32 D_0044A340[2] = {
#include "sub_main/0044A340.inc"
};
u32 D_0044A348[2] = {
#include "sub_main/0044A348.inc"
};
u32 D_0044A350[2] = {
#include "sub_main/0044A350.inc"
};
u32 D_0044A358[2] = {
#include "sub_main/0044A358.inc"
};
u32 D_0044A360[2] = {
#include "sub_main/0044A360.inc"
};
u32 D_0044A368[2] = {
#include "sub_main/0044A368.inc"
};
u32 D_0044A370[2] = {
#include "sub_main/0044A370.inc"
};
u32 D_0044A378[2] = {
#include "sub_main/0044A378.inc"
};
u32 D_0044A380[2] = {
#include "sub_main/0044A380.inc"
};
u32 D_0044A388[2] = {
#include "sub_main/0044A388.inc"
};
u32 D_0044A390[4] = {
#include "sub_main/0044A390.inc"
};
u32 D_0044A3A0[12] = {
#include "sub_main/0044A3A0.inc"
};
u32 D_0044A3D0[108] = {
#include "sub_main/0044A3D0.inc"
};
u32 D_0044A580[24] = {
#include "sub_main/0044A580.inc"
};
u32 D_0044A5E0[158] = {
#include "sub_main/0044A5E0.inc"
};
u32 gEquipFxListsAltHead[2] = {
    (u32)D_0044A3A0, (u32)D_0044A580,
};
u32 gEquipFxListsHead[2] = {
    (u32)D_0044A3D0, (u32)D_0044A5E0,
};
u32 gLobbyActorFxIds[2] = {
#include "sub_main/0044A868.inc"
};
StartItem gStartItemsByClass_0[8] = {
#include "sub_main/0044A870.inc"
};
StartItem gStartItemsByClass_1[12] = {
#include "sub_main/0044A890.inc"
};
StartItem gStartItemsByClass_2[8] = {
#include "sub_main/0044A8C0.inc"
};
StartItem gStartItemsByClass_3[8] = {
#include "sub_main/0044A8E0.inc"
};
StartItem gStartItemsByClass_4[8] = {
#include "sub_main/0044A900.inc"
};
StartItem gStartItemsByClass_5[12] = {
#include "sub_main/0044A920.inc"
};
StartItem gStartItemsByClass_10[8] = {
#include "sub_main/0044A950.inc"
};
StartItem *gStartItemsByClass[12] = {
    gStartItemsByClass_0, gStartItemsByClass_1, gStartItemsByClass_2, gStartItemsByClass_3,
    gStartItemsByClass_4, gStartItemsByClass_5, gStartItemsByClass_4, gStartItemsByClass_0,
    gStartItemsByClass_0, gStartItemsByClass_0, gStartItemsByClass_10, NULL,
};
/* The skill groups and their thresholds: SkillDef records (item.h), ended
 * by SKILL_GROUP_END. The game's bytes are included here as words. */
u32 gSkillTable[292] = {
#include "sub_main/0044A9A0.inc"
};
u32 gBladeSpecialMarks[12] = {
#include "sub_main/0044AE30.inc"
};
u32 gPlayerColorWords[12] = {
#include "sub_main/0044AE60.inc"
};
PlayerVt *gPlayerHandlers = gPlayerHandlerTable;
u32 gPresetHairColors[12] = {
#include "sub_main/0044AEA0.inc"
};
s8 gPresetEquipIds[80] = {
#include "sub_main/0044AED0.inc"
};
SpawnOffset gPlayerSpawnOffset[8] = {
#include "sub_main/0044AF20.inc"
};
Vec3f gVersusStartPos[2] = {
#include "sub_main/0044AF60.inc"
};
u16 gVersusStartAngle[4] = {
#include "sub_main/0044AF78.inc"
};
