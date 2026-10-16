#include "common.h"
#include "player.h"
#include "scene.h"
#include "item.h"
#include "objhead.h"
#include "sprite.h"

/* An entry of a start item list: n of item; n == -1 ends the list. */
typedef struct StartItem {
    s16 n;
    u16 item;
} StartItem;

/* The player system's state (0x10 bytes). */
typedef struct PlayerSys {
    s8 state;                   /* 0x0 */
    s8 unk1;                    /* 0x1 */
    u8 unk2[0x10 - 0x2];
} PlayerSys;

typedef struct Offset2 {
    f32 x;
    f32 z;
} Offset2;

typedef struct Vec3f {
    f32 x;
    f32 y;
    f32 z;
} Vec3f;

/* This file's view of Player's rotation: the angles as integers at 0xA0
 * (Mtx_FromAnglesXYZ), and 0xE a 16-bit copy of the Y angle. */
typedef struct PlayerRot {
    u8 pad00[0xE];
    s16 angle;                  /* 0x0E */
    u8 pad10[0xA0 - 0x10];
    s32 rx;                     /* 0xA0 */
    s32 ry;                     /* 0xA4 */
    s32 rz;                     /* 0xA8 */
} PlayerRot;

extern Vec3f gStartPos[];
extern u16 gStartAngle[];
extern Vec3f gVersusStartPos[];
extern u16 gVersusStartAngle[];
extern Vec3f gPlayerResetPos[];
extern s32 gPlayerResetRotY[];

extern StartItem *gStartItemLists[];
extern StartItem *D_896C20[];
extern StartItem *gStartItemsByClass[];
extern u16 gPartIdMax[];
extern u32 gPresetHairColors[];
extern s8 gPresetEquipIds[];
extern PlayerVt *gPlayerHandlers;
extern Offset2 gPlayerSpawnOffset[];
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
void Effect01_Spawn(void *obj, s16 type);
void Effect06_SpawnAtBone(Player *p, s16 type, u8 sub, s16 bone, f32 scale);
void Hit_ResetAll(void);
void QSync_SendPlayer(Player *p, s32 kind, u16 mode);
struct StageObj *StageObj_K05_Create(void *owner, s32 a, s32 b);
struct Em;
void EmFxSeg_Spawn(struct Em *em, u8 k); /* game em_bulldrome.c; called here with a Player */
void StageObj_BowArc_Create(void *owner);
void StageObj_K29_CreateOnPlayerDefault(Player *p, u8 k, f32 scale);
void Game_UpdatePlayers(void);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
s32 Debug_IsOn(s32 unused0);
/* unit's own declaration: with the definition's void (Player *p, s32 frame, s32 n, s32 part) (sub_main/unk_2E3940.c), Player_Reset: 2 of 678 words differ, Player_UpdateMotionStep: 2 of 110 words differ */
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
void Player_ClearMealEffects(Player *p);
void Player_StartIdleMotion(Player *p, s32 a, s32 b);
void Player_ClearActionState(Player *p);
void Player_ResetAim(Player *p);
void Vec_Copy(f32 *dst, f32 *src);
void Player_CalcResists(Player *p);
void Player_ClearUnk540(Player *p);
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
s32 Player_NearEm40(Player *p);
/* unit's own declaration: with the definition's u8 (Player *p, u8 group) (sub_main/unk_3AC9C0.c), Player_UpdateStatus: 10 of 745 words differ */
s32 Player_GetSkill(Player *p, u8 group);
void Player_AddHp(Player *p, s16 n);
s16 Player_IsState(void *obj, u8 state, u8 sub);
u16 Pouch_FindCarryItem(Player *p);
void Jingle_CarryLost(Player *p);
u8 Pouch_Add(Player *p, u16 item, s16 n);
extern s16 D_005B396C;

void Pouch_SetStartItems(Player *p)
{
    StartItem *e;
    s16 i;
    u16 item;
    s16 n;

    if (p->startItemList != 0) {
        p->startItemList--;
        for (i = 0; i < 20; i++) {
            p->pouch[i].item = 0;
            p->pouch[i].count = 0;
        }
        if (Quest_IsDrill() == 1) {
            e = gStartItemLists[p->startItemList];
        } else if (Quest_IsTraining() == 1) {
            e = D_896C20[p->startItemList];
        } else {
            e = gStartItemsByClass[p->weaponType];
        }
        while ((n = e->n) != -1) {
            item = e->item;
            e++;
            Pouch_SetAtLeast(p, 0, item, n);
        }
    }
}

void Player_InitForQuest(Player *p)
{
    s32 max;
    u32 i;
    s8 *tbl;

    if (gScene.inQuest[p->index] == 0xFF && Player_IsLocal(p) == 0) {
        p->active = 0;
        p->visible = 0;
        return;
    }
    p->isEm = 0;
    p->unk01E = 0;
    p->motionLayerCount = 2;
    p->unk310 = 0;
    p->unk311 = -1;
    if (p->presetEquip != 0) {
        if (Quest_IsDrill() == 0 && Quest_IsTraining() == 0) {
            tbl = &gPresetEquipIds[p->presetEquip * 6 + p->gender * 36];
            for (i = 0; i < 6; i++, tbl++) {
                if (*tbl < 0) {
                    max = gPartIdMax[i + p->gender * 7];
                    p->equip[i].id = Rand_Next(1) % max + 1;
                } else {
                    p->equip[i].id = *tbl;
                }
            }
            p->hairColor = gPresetHairColors[p->presetEquip + p->gender * 6] | 0xFF000000;
            Player_SetColorWord(p, 2);
        }
        Player_CalcSkills(p);
    }
    Player_LoadWeaponModel(p->weaponKind, p->index);
    Player_LoadPartModels(p);
    Player_ClearFxBlock(p);
}

void Player_ClearAll(void)
{
    s32 i;

    for (i = 0; i < 10; i++) {
        memset(&gPlayers[i], 0, sizeof(Player));
    }
    memset(&gPlayerSys, 0, sizeof(gPlayerSys));
}

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

void Player_InitAll(s16 mode)
{
    s32 i;
    Player *p;

    gPlayerSys.unk1 = 0;
    gPlayerSys.state = 1;
    for (i = 0, p = gPlayers; i < 4; i++, p++) {
        if (p->active) {
            p->drawSlotId = DrawSlot_Alloc();
            if (p->drawSlotId != -1) {
                p->drawSlot = DrawSlot_Get(p->drawSlotId);
                p->drawSlot->player = p->index;
                p->drawSlot->draw = DrawCb_LocalPlayer;
            }
            if (mode == 0 || (mode == 1 && p->areaMoveState != 0)) {
                Player_Init(p, i, gPlayerHandlers);
            }
            if (mode == 0) {
                if (Player_IsLocal(p) == 1) {
                    p->nextArea = p->area;
                    p->startPos[0] = p->pos[0];
                    p->startPos[1] = p->pos[1];
                    p->startPos[2] = p->pos[2];
                    p->startAngle = p->rot[1];
                    QSync_SendPlayer(p, 3, 0);
                }
                Effect01_Spawn(p, 1);
                Effect06_SpawnAtBone(p, 7, 0, 1, 1.0f);
                switch (p->weaponType) {
                case 6:
                    StageObj_K05_Create(p, -1, 3);
                    EmFxSeg_Spawn((struct Em *)p, 0);
                    break;
                case 10:
                    StageObj_BowArc_Create(p);
                    break;
                case 8:
                    StageObj_K29_CreateOnPlayerDefault(p, 7, 1.0f);
                    break;
                }
            }
        }
    }
    Hit_ResetAll();
}

void Player_Init(Player *p, u8 idx, PlayerVt *vt)
{
    p->index = idx;
    if (gScene.inQuest[p->index] == 1 || Player_IsLocal(p) == 1) {
        p->visible = 1;
    } else {
        p->visible = 0;
    }
    p->handlers = vt;
    Player_Reset(p);
    p->motion[0].unk04 = idx + 1;
    p->handlers->unk0(p);
    p->handlers->unkC(p);
}

void Player_AddSpawnOffset(Player *p, f32 *pos, u16 angle)
{
    s32 rot[3];
    f32 v[3];
    f32 out[3];
    f32 m[4][4];
    u8 idx;

    if (gScene.online == 0) {
        idx = p->index;
        rot[1] = angle;
        rot[0] = 0;
        rot[2] = 0;
        Mtx_FromAnglesXYZ(rot, (f32 *) m);
        v[0] = gPlayerSpawnOffset[idx].x;
        v[1] = 0.0f;
        v[2] = gPlayerSpawnOffset[idx].z;
        Vec_Rotate((f32 *) out, (f32 *) v, (f32 *) m);
        pos[0] += out[0];
        pos[2] += out[2];
    }
}

s32 Player_GetHpSkillBonus(Player *p)
{
    if (Player_HasSkill(p, 0x15) == 1) {
        return 10;
    }
    if (Player_HasSkill(p, 0x16) == 1) {
        return 20;
    }
    if (Player_HasSkill(p, 0x17) == 1) {
        return 30;
    }
    if (Player_HasSkill(p, 0x18) == 1) {
        return -10;
    }
    if (Player_HasSkill(p, 0x19) == 1) {
        return -20;
    }
    if (Player_HasSkill(p, 0x1A) == 1) {
        return -30;
    }
    return 0;
}

void Player_CalcSharpnessMax(Player *p)
{
    p->sharpnessMax = gBladeData[p->equip[6].id].sharpMax * 50 + 150;
    if (Player_HasSkill(p, 0x21) == 1) {
        p->sharpnessMax += 50;
        if (p->sharpnessMax > 400) {
            p->sharpnessMax = 400;
        }
    } else if (Player_HasSkill(p, 0x22) == 1) {
        p->sharpnessMax -= 50;
        if (p->sharpnessMax < 150) {
            p->sharpnessMax = 150;
        }
    }
}

void Player_Reset(Player *p)
{
    f32 v[3];
    f32 out[3];
    s32 rot[3];
    f32 m[4][4];
    s32 side;
    u8 flags;
    s32 i;
    PlayerRot *r = (PlayerRot *)p;

    *(u32 *)&p->unk004 = 0;
    p->unk2B4 = 0;
    *(u16 *)p->motionStarted = 0;
    p->scale[0] = 1.0f;
    p->scale[1] = 1.0f;
    p->scale[2] = 1.0f;
    switch (p->areaMoveState) {
    case 0:
        if (Quest_IsVersus() == 1) {
            side = (u8)((p->side > 0) ? (u8)(p->side - 1) : 0) & 1;
            p->pos[0] = gVersusStartPos[side].x;
            p->pos[1] = gVersusStartPos[side].y;
            p->pos[2] = gVersusStartPos[side].z;
            r->ry = gVersusStartAngle[side];
            r->rx = 0;
            r->rz = 0;
            Mtx_FromAnglesXYZ(p->rot, p->rotMtx);
            if (p->side >= 3) {
                rot[0] = 0;
                rot[1] = r->ry;
                rot[2] = 0;
                Mtx_FromAnglesXYZ(rot, (f32 *) m);
                v[0] = 80.0f;
                v[1] = 0.0f;
                v[2] = 80.0f;
                Vec_Rotate((f32 *) out, (f32 *) v, (f32 *) m);
                p->pos[0] += out[0];
                p->pos[2] += out[2];
            }
        } else {
            p->pos[0] = gStartPos[gScene.area].x;
            p->pos[1] = gStartPos[gScene.area].y;
            p->pos[2] = gStartPos[gScene.area].z;
            r->ry = gStartAngle[gScene.area];
            r->rx = 0;
            r->rz = 0;
            Mtx_FromAnglesXYZ(p->rot, p->rotMtx);
            Player_AddSpawnOffset(p, p->pos, r->ry);
        }
        if (Player_IsLocal(p) == 1) {
            Player_CheckTeamCombo(p);
        } else {
            p->mealSkill = 0;
            p->mealSkillGroup = 0;
        }
        p->hpMax = 100;
        p->staminaMax = 300;
        p->attackDrug = 0;
        p->defenseDrug = 0;
        p->attackBuff = 0;
        p->defenseBuff = 0;
        p->attackBuffTimer = 0;
        p->defenseBuffTimer = 0;
        if (Player_IsLocal(p) == 1) {
            p->hpMax += gScene.mealHealth;
            p->hpMax = p->hpMax + Player_GetHpSkillBonus(p);
            if (p->hpMax > 150) {
                p->hpMax = 150;
            }
            Player_AddMaxStamina(p, gScene.mealStamina);
            p->attackDrug = gScene.mealAttack;
            p->defenseDrug = gScene.mealDefense;
        }
        p->hp = p->hpMax;
        p->hpRecoverable = p->hpMax;
        p->hpMaxBase = p->hpMax;
        p->staminaTimer = 10800;
        p->stamina = p->staminaMax;
        Player_CalcSharpnessMax(p);
        p->sharpness = p->sharpnessMax;
        p->sharpnessLevel = Player_GetSharpnessLevel(p);
        p->staminaLockTimer = 0;
        p->coolDrinkTimer = 0;
        p->hotDrinkTimer = 0;
        p->detectTimer = 0;
        p->actDetectTimer = 0;
        p->regenTimer = 0;
        p->status5Delay = 0;
        p->status5Buildup = 0;
        p->poison = 0;
        p->mashLeft = 0;
        p->mashBuildup = 0;
        p->status6Timer = 0;
        p->status7Timer = 0;
        p->status2Buildup = 0;
        p->status4Buildup = 0;
        p->recoverTimer = 0;
        p->unk7E5 = 0;
        p->swordGaugeTimer = 0;
        p->swordGauge = 0;
        p->attackFactor = 1.0f;
        p->defenseFactor = 1.0f;
        p->unk8FE = 0;
        Pouch_SelectFirst(p);
        p->faints = 0;
        p->weaponSeMute = 0;
        p->tradeCount = 0;
        p->tradeCooldown = 0;
        p->weaponOut = 0;
        p->roundsLoaded = 0;
        p->clipSize = 0;
        p->spotCheckDelay = 0;
        p->unk8B3 = 0;
        Player_InitRecords(p);
        if (gScene.inQuest[p->index] == 1 || Player_IsLocal(p) == 1) {
            p->stateGroup = 0;
            p->state = 0;
        } else {
            Player_SetState(p, 4, 5, 2);
        }
        p->syncTimer = 0;
        p->useItem = 0;
        p->fieldCollide = 1;
        p->supplyTaken = 0;
        Pouch_SetStartItems(p);
        Player_ResetAttackPercent(p);
        switch (p->weaponType) {
        case 9:
            switch (p->bladeSpecial % 3) {
            case 0:
                p->ammoCount = 5;
                break;
            case 1:
                p->ammoCount = 2;
                break;
            case 2:
                p->ammoCount = 3;
                break;
            }
            p->clipSize = p->ammoCount;
            break;
        case 10:
            p->unk7CC = 0x87;
            p->unk7CE = 0xFF;
            p->sharpnessLevel = 3;
            break;
        }
        p->ammoSlot = Bowgun_NextAmmoSlot(p, 0, 2);
        p->ammoCount = p->pouch[p->ammoSlot].count;
        Bowgun_LoadAmmo(p, 0);
        p->coatingSlot = 0xFF;
        HuntingHorn_ClearNotes(p);
        Player_ClearMealEffects(p);
        Player_StartIdleMotion(p, 0, 0);
        break;
    case 1:
        p->pos[0] = p->startPos[0];
        p->pos[1] = p->startPos[1];
        p->pos[2] = p->startPos[2];
        r->ry = p->startAngle;
        Mtx_FromAnglesXYZ(p->rot, p->rotMtx);
        p->areaMoveState = 0;
        p->spotCheckDelay = 0;
        if (gScene.inQuest[p->index] == 1 || Player_IsLocal(p) == 1) {
            p->stateGroup = 0;
            p->state = 0;
        } else {
            Player_SetState(p, 4, 5, 2);
        }
        p->syncTimer = 0;
        flags = p->carry;
        if (flags & 0xF) {
            Player_ClearActionState(p);
            p->carry = (u8)(flags & 0xF0) | 5;
            Player_SetState(p, 5, 0, 0);
        } else if (p->mashLeft > 0) {
            Player_SetState(p, 2, 0x1E, 0xC);
        } else {
            Player_ClearActionState(p);
            Player_StartIdleMotion(p, 0, 0);
        }
        break;
    case 2:
    case 3:
        if (p->areaMoveState == 2) {
            p->pos[0] = gPlayerResetPos[gScene.map].x;
            p->pos[1] = gPlayerResetPos[gScene.map].y;
            p->pos[2] = gPlayerResetPos[gScene.map].z;
            r->ry = gPlayerResetRotY[gScene.map];
        } else {
            p->pos[0] = p->startPos[0];
            p->pos[1] = p->startPos[1];
            p->pos[2] = p->startPos[2];
            r->ry = p->startAngle;
        }
        Player_AddSpawnOffset(p, p->pos, r->ry);
        Mtx_FromAnglesXYZ(p->rot, p->rotMtx);
        p->floorY = p->pos[1];
        r->angle = r->ry;
        Player_CalcSharpnessMax(p);
        p->hpMax = 100;
        if (Player_IsLocal(p) == 1) {
            p->hpMax = p->hpMax + Player_GetHpSkillBonus(p);
            if (p->hpMax > 150) {
                p->hpMax = 150;
            }
        }
        p->hp = p->hpMax;
        p->hpRecoverable = p->hpMax;
        p->hpMaxBase = p->hpMax;
        p->staminaMax = 300;
        p->staminaTimer = 10800;
        p->stamina = p->staminaMax;
        p->staminaLockTimer = 0;
        p->coolDrinkTimer = 0;
        p->hotDrinkTimer = 0;
        p->regenTimer = 0;
        p->detectTimer = 0;
        p->actDetectTimer = 0;
        p->status5Delay = 0;
        p->status5Buildup = 0;
        p->poison = 0;
        p->status6Timer = 0;
        p->status7Timer = 0;
        p->mashLeft = 0;
        p->mashBuildup = 0;
        p->status2Buildup = 0;
        p->status4Buildup = 0;
        p->recoverTimer = 0;
        p->spotCheckDelay = 20;
        p->visible = 0;
        p->attackDrug = 0;
        p->defenseDrug = 0;
        p->attackBuff = 0;
        p->defenseBuff = 0;
        p->attackBuffTimer = 0;
        p->defenseBuffTimer = 0;
        p->weaponSeMute = 0;
        p->weaponOut = 0;
        HuntingHorn_ClearNotes(p);
        Player_ClearMealEffects(p);
        if (p->areaMoveState == 2) {
            Player_SetState(p, 4, 0, 0xC);
        } else {
            p->visible = 1;
            p->stateGroup = 0;
            p->state = 0;
            Player_SetState(p, 0, 0, 0xC);
            Player_ClearActionState(p);
            Player_StartIdleMotion(p, 0, 0);
        }
        p->areaMoveState = 0;
        break;
    }
    p->lookFound = 0;
    r->angle = r->ry;
    p->lean = 0;
    p->lookYaw = 0;
    p->hitTimers = &p->hitDelay;
    p->action = 0;
    p->ammoMenuOpen = 0;
    p->unk4C4 = 0;
    p->rideState = 0;
    p->unk58C = -1;
    p->unk58E = -1;
    p->unk3F8 = 0;
    p->painted = 0xFF;
    p->alpha = 1.0f;
    p->fadeAlpha = 1.0f;
    p->weaponShown = 1;
    p->supplyBoxOpen = 0;
    p->deliveryMenu = 0;
    p->menuPage = 0;
    p->deliveryOpen = 0;
    p->menuDelay = 0;
    p->menuCursor = 0;
    p->actRequest = 0;
    p->offerRequest = 0;
    p->teamItemMask = 0;
    p->tradeMsgTimer = 0;
    p->offerNoticeTimer = 0;
    p->demonMode = 0;
    p->noFlinchTimer = 0;
    p->noFlinchItemTimer = 0;
    p->motionTime = 0;
    p->noHitTimer = 30;
    p->motion[0].step = 2.0f;
    p->motion[1].step = 2.0f;
    p->bowgunReady = 0;
    p->freeState = 0;
    p->prevFreeState = 0;
    p->pouchHudFlags = 0;
    p->itemSelect = 0;
    p->pushPriority = 1;
    p->unk417 = 0;
    p->dashDelay = 0;
    p->itemUseDelay = 0;
    p->unk816 = 0;
    p->hideShadow = 0;
    p->envTimer = 90;
    p->motionSpeed = 1.0f;
    p->inPoisonArea = 0;
    Player_ResetAim(p);
    p->moveTo[0] = 0.0f;
    p->moveTo[1] = 0.0f;
    p->moveTo[2] = 0.0f;
    Vec_Copy(p->moveToStep, p->moveTo);
    p->moveToFrames = 0;
    for (i = 0; i < 4; i++) {
        p->partAnimOn[i] = 0;
        p->partAnimMotion[i] = 0;
        p->partAnimWeight[i] = 0;
    }
    p->aimWeight = 0;
    Player_BeginLayerMotion(p, p->motionFrame[0], p->motionBlend[0], 0);
    Player_BeginLayerMotion(p, p->motionFrame[1], p->motionBlend[1], 1);
    Player_UpdateMotion(p);
    Player_CalcResists(p);
    Player_ClearUnk540(p);
    Player_UpdateAreaPos(p);
    Player_CheckAreaMsgEvent(p);
}

/* Player fields that Player_UpdateStatus reads as u16 (the spec has them as s16). */
typedef struct PlayerTimers {
    u8 pad000[0x62A];
    u16 unk62A;                 /* 0x62A */
    u8 pad62C[0x88C - 0x62C];
    u16 unk88C;                 /* 0x88C */
    u8 pad88E[0x8BA - 0x88E];
    u16 unk8BA;                 /* 0x8BA */
} PlayerTimers;

/* Counts the player's timers down each frame, regenerates stamina and health,
 * and ends timed effects. Case 2's test of the state is a switch with its
 * default first: that leaves every conditional branch delay slot of the
 * function empty, as in the original. */
void Player_UpdateStatus(Player *p)
{
    PlayerTimers *t = (PlayerTimers *)p;
    u8 rate;
    s16 interval;
    s16 amount;
    s16 diff;
    u16 item;
    u8 flags;
    u8 lo;
    s16 i;

    if (p->noFlinchItemTimer != 0) {
        p->noFlinchItemTimer--;
    }
    if (p->offerNoticeTimer != 0) {
        p->offerNoticeTimer--;
    }
    if (p->weaponSeMute != 0) {
        p->weaponSeMute--;
    }
    if (p->unk4C1 != 0) {
        p->unk4C1--;
    }
    if (p->tradeMsgTimer != 0) {
        p->tradeMsgTimer--;
    }
    if (p->tradeCooldown > 0) {
        p->tradeCooldown--;
        if (p->tradeCooldown <= 0) {
            p->tradeCooldown = 0;
            p->tradeCount = 0;
        }
    }
    if (p->dashDelay != 0) {
        p->dashDelay--;
    }
    if (p->staminaLockTimer != 0) {
        p->staminaLockTimer--;
        if (p->staminaLockTimer == 0 && Player_IsLocal(p) == 1 && Quest_PhaseIsOver(1) == 0) {
            Notice_Push(0, 0x15, 0);
        }
    }
    if (p->coolDrinkTimer != 0) {
        if (Area_Temperature(p->area) == 3) {
            if (p->coolDrinkTimer < 2) {
                p->coolDrinkTimer = 0;
            } else {
                p->coolDrinkTimer -= 2;
            }
        } else {
            p->coolDrinkTimer--;
        }
        if (p->coolDrinkTimer == 0 && Player_IsLocal(p) == 1 && Quest_PhaseIsOver(1) == 0) {
            Notice_Push(0, 0x12, 0);
        }
    }
    if (p->hotDrinkTimer != 0) {
        if (Area_Temperature(p->area) == 4) {
            if (p->hotDrinkTimer < 2) {
                p->hotDrinkTimer = 0;
            } else {
                p->hotDrinkTimer -= 2;
            }
        } else {
            p->hotDrinkTimer--;
        }
        if (p->hotDrinkTimer == 0 && Player_IsLocal(p) == 1 && Quest_PhaseIsOver(1) == 0) {
            Notice_Push(0, 0x13, 0);
        }
    }
    if (p->regenTimer != 0) {
        p->regenTimer--;
        if (p->regenTimer == 0 && Player_IsLocal(p) == 1 && Quest_PhaseIsOver(1) == 0) {
            Notice_Push(0, 0x16, 0);
        }
    }
    if (t->unk88C != 0) {
        t->unk88C--;
    }
    if (t->unk8BA != 0) {
        t->unk8BA--;
    }
    if (t->unk62A != 0) {
        t->unk62A--;
        if (t->unk62A == 0) {
            p->attackBuff = 0;
        }
    }
    if (p->defenseBuffTimer != 0) {
        p->defenseBuffTimer--;
        if (p->defenseBuffTimer == 0) {
            p->defenseBuff = 0;
        }
    }
    if (p->mashLeft != 0) {
        p->mashLeft--;
    }
    if (p->swordGaugeTimer != 0) {
        p->swordGaugeTimer--;
    }
    if (p->status6Timer > 0) {
        p->status6Timer--;
        if (p->status6Timer < 0) {
            p->status6Timer = 0;
        }
    }
    if (p->status7Timer > 0) {
        p->status7Timer--;
        if (p->status7Timer < 0) {
            p->status7Timer = 0;
        }
    }
    if (p->weaponType == 7 && p->swordGaugeTimer == 0 && p->swordGauge != 0 && !(gScene.frame & 7)) {
        p->swordGauge--;
    }
    if (p->weaponType == 9 && p->weaponTimer > 0) {
        p->weaponTimer--;
        if (p->weaponTimer == 0) {
            p->unk8B3 = 0;
            Player_PlaySe(p, 0x15, 0, p->pos, 1, 0);
            if (Player_IsLocal(p) == 1) {
                Sound_PlaySeFlat(0, 0x21, 0);
            }
            p->swordGaugeTimer = 10;
        } else {
            p->unk8B3 = 1;
        }
    }
    if (Player_TestFlag(p, 0x200) == 0 && p->demonMode == 0) {
        switch (p->stateGroup) {
        case 0:
            if (p->state != 0x6E && p->state != 0x6D && p->state != 0x6C && p->state != 0x6B &&
                p->state != 0x44 && p->state != 0x32 && p->state != 0x31 && p->state != 0x2D &&
                p->state != 0x1C && p->state != 0x78 && p->state != 0x77) {
                Player_AddStamina(p, 2);
            }
            break;
        case 1:
            switch (p->weaponType) {
            case 2:
                switch (p->state) {
                default:
                    Player_AddStamina(p, 2);
                    break;
                case 3:
                case 0xC:
                case 0xD:
                case 0xE:
                case 4:
                    break;
                }
                break;
            case 3:
                if (p->state != 0x10 && p->state != 5 && p->state != 0xE && p->state != 3) {
                    Player_AddStamina(p, 2);
                }
                break;
            case 10:
                if (p->state != 0xA && p->state != 8 && p->weaponTimer == 0) {
                    Player_AddStamina(p, 2);
                }
                break;
            default:
                Player_AddStamina(p, 2);
                break;
            }
            break;
        case 2:
        case 3:
            break;
        default:
            Player_AddStamina(p, 2);
            break;
        }
    }
    if (p->demonMode != 0) {
        Player_UseStamina(p, -1);
        if (p->stamina <= 0 && p->stateGroup != 1) {
            p->demonMode = 0;
        }
    }
    Player_UpdateHunger(p);
    if (p->stateGroup != 2 && p->stateGroup != 3) {
        if (p->coolDrinkTimer == 0) {
            switch (Area_Temperature(p->area)) {
            case 1:
                if (Player_HasSkill(p, 0x85) == 0 && Player_HasSkill(p, 0x86) == 0) {
                    goto no_regen;
                }
                break;
            case 3:
                if (Player_HasSkill(p, 0x86) == 0) {
                    goto no_regen;
                }
                break;
            }
        }
        rate = p->unk669;
        if (rate == 0 && Player_NearEm40(p) == 1) {
            rate = 2;
        }
        if (rate == 1 && Player_HasSkill(p, 0xB) == 1) {
            rate = 0;
        }
        if (p->poison == 0 && rate == 0) {
            interval = 60;
            if ((u8)Player_GetSkill(p, 7)) {
                if (Player_HasSkill(p, 0x1B) == 1) {
                    interval = 20;
                }
                if (Player_HasSkill(p, 0x1C) == 1) {
                    interval = 15;
                }
                if (Player_HasSkill(p, 0x1D) == 1) {
                    interval = 180;
                }
                if (Player_HasSkill(p, 0x1E) == 1) {
                    interval = 240;
                }
            }
            amount = (p->regenTimer != 0) ? 2 : 1;
            if (gScene.frame % interval == 0 && p->hp < p->hpRecoverable) {
                diff = p->hpRecoverable - p->hp;
                if (diff >= amount) {
                    Player_AddHp(p, amount);
                } else {
                    Player_AddHp(p, diff);
                }
            }
        }
no_regen:
        if (p->status5Delay > 0) {
            p->status5Delay--;
        } else {
            p->status5Delay = 0;
            if (p->status5Buildup != 0) {
                p->status5Buildup--;
            }
        }
    }
    if (p->aimLockTimer != 0) {
        p->aimLockTimer--;
    }
    if (p->unk816 != 0) {
        p->unk816--;
    }
    if (p->itemUseDelay != 0) {
        p->itemUseDelay--;
    }
    flags = p->carry;
    lo = flags & 0xF;
    if (lo != 0 && p->stateGroup == 5) {
        p->carry = lo;
        p->carry--;
        p->carry |= (u8)(flags & 0xF0);
    } else if (Player_IsState(p, 0, 0x47) == 0 && Player_IsState(p, 0, 0x4A) == 0 &&
               Player_IsState(p, 0, 0x51) == 0 && Player_IsState(p, 0, 0x5D) == 0 &&
               Player_IsState(p, 0, 0x60) == 0) {
        item = Pouch_FindCarryItem(p);
        if (item != 0xFFFF) {
            if (Player_IsLocal(p) == 1 && Quest_PhaseIsOver(1) == 0) {
                Jingle_CarryLost(p);
                Notice_Push(1, 5, item);
            }
            Pouch_Add(p, item, -10);
        }
    }
    for (i = 0; i < 12; i++) {
        if (p->mealEffectTimer[i] > 0) {
            p->mealEffectTimer[i]--;
            if (p->mealEffectTimer[i] <= 0) {
                p->mealEffectTimer[i] = 0;
                if (i == 3) {
                    p->hpMax = p->hpMaxBase;
                    if (p->hp >= p->hpMax) {
                        p->hp = p->hpMax;
                        D_005B396C = p->hp;
                    }
                    if (p->hpRecoverable >= p->hpMax) {
                        p->hpRecoverable = p->hpMax;
                    }
                }
            }
        }
    }
    if (p->spotCheckDelay != 0) {
        p->spotCheckDelay--;
    }
    if (p->fishTimer != 0) {
        p->fishTimer--;
    }
}

void Player_TickPaint(Player *p)
{
    if (p->startItemList == 0 && p->painted != 0xFF && p->painted != 0) {
        if (p->paintTimer != 0) {
            p->paintTimer--;
        } else {
            p->painted = 0;
        }
    }
}

void Player_TickTimers(Player *p)
{
    if (p->hitStop == 0) {
        if (Player_TestFlag(p, 0x400001)) {
            p->unk3A0 += 2;
        } else {
            p->unk3A0 = 0;
        }
        p->motionTime += 2;
        if ((s32)p->motionTime >= 100000) {
            p->motionTime = 10000;
        }
        if (p->noHitTimer != 0) {
            p->noHitTimer--;
        }
        if (p->unk434 != 0) {
            p->unk434--;
        }
        if (p->noFlinchTimer != 0) {
            p->noFlinchTimer--;
        }
        if (p->noPushTimer != 0) {
            p->noPushTimer--;
        }
        if (p->hideShadow != 0) {
            p->hideShadow--;
        }
        if (p->isEm == 0) {
            Player_UpdateStatus(p);
        } else {
            Player_TickPaint(p);
        }
    }
    Player_ClearFlag(p, 0x800);
    if (Player_TestFlag(p, 1)) {
        Player_SetFlag(p, 0x2000);
    } else {
        Player_ClearFlag(p, 0x2000);
    }
}

void Player_UpdateHitStop(void *obj)
{
    ObjHead *o = obj;
    if (o->hitStopAdd != 0) {
        o->hitStop += o->hitStopAdd;
        o->hitStopAdd = 0;
    } else if (o->hitStop != 0) {
        o->hitStop--;
    }
}

void Player_UpdateMotionStep(Player *p)
{
    if (Debug_IsOn(0x18) == 0) {
        Mtx_FromAnglesXYZ(p->rot, p->rotMtx);
        if (p->hitStop != 0) {
            p->motion[0].step = 0.0f;
            p->motion[1].step = 0.0f;
        } else if (p->hitSlowTimer > 0) {
            p->hitSlowTimer--;
            p->motion[0].step = 0.2f;
            p->motion[1].step = 0.2f;
        } else {
            p->hitSlowTimer = 0;
            switch (p->motionId[0]) {
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
                if (p->motion[0].step <= 2.0f) {
                    p->motion[0].step = 2.0f;
                    p->motion[1].step = 2.0f;
                }
                break;
            case 3:
            case 0x22:
            case 0x23:
                break;
            default:
                p->motion[0].step = 2.0f * p->motionSpeed;
                p->motion[1].step = 2.0f * p->motionSpeed;
                break;
            }
        }
        if (p->motionStarted[0] == 0) {
            p->motionStarted[0]++;
            Player_BeginLayerMotion(p, p->motionFrame[0], p->motionBlend[0], 0);
        }
        if (p->motionStarted[1] == 0) {
            p->motionStarted[1]++;
            Player_BeginLayerMotion(p, p->motionFrame[1], p->motionBlend[1], 1);
        }
        Player_UpdateMotion(p);
    }
}

/* unk_399F70's .data (0x44A190-0x44AF80) */
void func_00395CF0(Player *p);
void Player_MotionFxStep(Player *p);
PlayerVt D_0044A190[2] = {
    {func_00395CF0, func_00395CF0, func_00395CF0, Player_MotionFxStep},
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
u32 gEquipFxListsAltTorso[2] = {
    (u32)D_0044A3A0, (u32)D_0044A580,
};
u32 gEquipFxListsTorso[2] = {
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
u32 D_0044A9A0[292] = {
#include "sub_main/0044A9A0.inc"
};
u32 gBladeSpecialMarks[12] = {
#include "sub_main/0044AE30.inc"
};
u32 gPlayerColorWords[12] = {
#include "sub_main/0044AE60.inc"
};
PlayerVt *gPlayerHandlers = D_0044A190;
u32 gPresetHairColors[12] = {
#include "sub_main/0044AEA0.inc"
};
s8 gPresetEquipIds[80] = {
#include "sub_main/0044AED0.inc"
};
Offset2 gPlayerSpawnOffset[8] = {
#include "sub_main/0044AF20.inc"
};
Vec3f gVersusStartPos[2] = {
#include "sub_main/0044AF60.inc"
};
u16 gVersusStartAngle[4] = {
#include "sub_main/0044AF78.inc"
};
