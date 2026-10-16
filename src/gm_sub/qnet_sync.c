/* NETWORK: the game state messages of the in-quest protocol: what the game
 * sends through QNetIf_Post and what QNetIf_OnMessage hands on (players'
 * and monsters' positions, parts, states). */
#include "common.h"
#include "quest.h"
#include "scene.h"
#include "m2c_macros.h"
#include "em.h"
#include "player.h"
#include "qsync.h"

/* The fort monsters' first two work bytes, as QSync_ApplyEmState reads them:
 * Lao-Shan Lung (types 7, 0x32), Shen Gaoren (0x37), Yama Tsukami (0x3A) and
 * Kushala Daora (0x36, 0x3C, 0x40, 0x41); their views in em.h
 * (EmLaoshanWork and the others) have them at the same place. */
typedef struct EmFortWork {
    u8 stage;                   /* 0x000: the fight's stage (the families' ScriptQuery returns it); the messages only raise it */
    s8 fortHits;                /* 0x001: the hits on the fort (Em.fortStage), copied to gScene.fortStage */
    u8 unk002[0x100 - 0x002];
} EmFortWork;
typedef char EmFortWork_size_check[(sizeof(EmFortWork) == sizeof(EmWork)) ? 1 : -1];

s32 strlen(char *);
void Chat_LogAddLocal(u8, u8, char *);
void func_D643F0(u8, u8, char *);

extern s8 D_6ADF26[4][8];         /* 8 bytes per player (message 10 kind 2) */
void QNetIf_Post(void *msg, s32 unused);
u8 Pouch_Add(Player *p, u16 item, s16 n);
void SupplyBox_StartMove(u16, u8);

s32 Game_IsOnline(void);
void Em_ChangeStateRouted(Em *em, u8 a, u8 b, s32 c); /* unit's own declaration: the definition (game unk_701810.c) takes s32 a, s32 b; s32 here changes QSync_OnEm */
void Em_SpawnPartEffects(Em *em, u8 mask);
s32 Quest_IsVersus(void);
void Loading_Stop(void);
void Quest_Finish(void);
void Game_ResetSubsystems(void);
void Scene_LoadOverlay(s32);
s32 QuestNet_GetPing(u8);
u16 Map_AreaAt(u16 map, u16 i);
void Quest_HandleNetMsg(void);
void Hud_StartMapPing(s32 i);
u8 Notice_Push(u8 kind, s16 a, s16 b);
void Em_RestartScript(Em *em);
s32 Scene_InField(void);
s32 Player_IsLocal(void *obj);
s16 Player_IsState(void *obj, u8 state, u8 sub);
u16 Camera_GetYaw(void);
void QSync_QueuePlayerPos(Player *p, f32 *pos);
extern s32 gQSyncResendDelay;
void Player_StartMoveTo(Player *p, s16 frames);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
void Player_Reset(Player *p);
void Item_Use(Player *p);
void Item_UseTool(Player *p);
void Item_UseMisc(Player *p);
void Item_UseTeamPowder(Player *p);
void Item_UsePlaced(Player *p);
s16 Pouch_Count(Player *p, u16 item);
u8 PouchBackup_Use(Player *p, u16 item, s16 n);
s32 Side_Match(u8 a, u8 b, u8 none);
void HuntingHorn_PlaySong(Player *p, u8 song, u8 quiet); /* the definition (game unk_6BB9D0.c) has (Em *em, ...): the player is passed */
void HuntingHorn_SongFxAllies(Player *p);
void HuntingHorn_SongFxSelf(Player *p);

/* Sends the local player's state (messages 1-4, QSyncPlayerMsg) of the given
 * kind; mode goes into the message (k1.mode, or the item hand-over's other
 * player and result). kind is an s32 switched on as (u8), and the case 7/8
 * test reuses that (u8) value (a0). p and kind get the original's s2/s1
 * only with msg.kind stored after msg.cmd (a u8 kind, or msg.kind stored
 * first, gives s1/s2). */
void QSync_SendPlayer(Player *p, s32 kind, u16 mode) {
    QSyncPlayerMsg msg;

    if (Game_IsOnline() != 0 && Scene_InField() != 0 && p->isEm == 0 && Player_IsLocal(p) != 0) {
        msg.flags = 0;
        msg.cmd = p->index + 1;
        msg.kind = kind;
        switch ((u8)kind) {
        case 1:
            msg.len = 0x2F;
            msg.u.k1.icons = p->statusIcons;
            msg.u.k1.weaponOut = p->weaponOut;
            msg.u.k1.mode = mode;
            msg.u.k1.pos[0] = p->pos[0];
            msg.u.k1.pos[1] = p->pos[1];
            msg.u.k1.pos[2] = p->pos[2];
            if (Player_IsState(p, 0, 3) != 0) {
                msg.u.k1.angle = p->input.stickAngle + Camera_GetYaw();
            } else {
                msg.u.k1.angle = p->targetYaw;
            }
            msg.u.k1.state = p->stateGroup;
            msg.u.k1.sub = p->state;
            msg.u.k1.useItem = p->useItem;
            msg.u.k1.ammoType = p->ammoType;
            msg.u.k1.stateRand = p->stateRand;
            msg.u.k1.chargeLevel = p->chargeLevel;
            msg.u.k1.carry = p->carry;
            msg.u.k1.hp = p->hp;
            msg.u.k1.stamina = p->stamina;
            msg.u.k1.staminaMax = p->staminaMax;
            msg.u.k1.hpMax = p->hpMax;
            msg.u.k1.sharpOrPitch = p->sharpnessLevel;
            msg.u.k1.aimWeight = p->aimWeight;
            msg.u.k1.visible = p->visible;
            msg.u.k1.area = p->area;
            msg.u.k1.attack = p->attack;
            QSync_QueuePlayerPos(p, p->pos);
            p->syncTimer = gQSyncResendDelay;
            break;
        case 9:
            msg.len = 0x2F;
            msg.u.k1.icons = p->statusIcons;
            msg.u.k1.weaponOut = p->weaponOut;
            msg.u.k1.mode = mode;
            msg.u.k1.pos[0] = p->pos[0];
            msg.u.k1.pos[1] = p->pos[1];
            msg.u.k1.pos[2] = p->pos[2];
            msg.u.k1.angle = p->targetYaw;
            msg.u.k1.state = p->stateGroup;
            msg.u.k1.sub = p->state;
            msg.u.k1.useItem = p->useItem;
            msg.u.k1.ammoType = p->ammoType;
            msg.u.k1.stateRand = p->stateRand;
            msg.u.k1.chargeLevel = p->chargeLevel;
            msg.u.k1.carry = p->carry;
            msg.u.k1.hp = p->hp;
            msg.u.k1.stamina = p->stamina;
            msg.u.k1.staminaMax = p->staminaMax;
            msg.u.k1.hpMax = p->hpMax;
            msg.u.k1.sharpOrPitch = p->aimPitchPct;
            msg.u.k1.aimWeight = p->aimWeight;
            msg.u.k1.visible = p->visible;
            msg.u.k1.area = p->area;
            msg.u.k1.attack = p->attack;
            QSync_QueuePlayerPos(p, p->pos);
            p->syncTimer = gQSyncResendDelay;
            break;
        case 2:
            msg.len = 0x1E;
            msg.u.k2.pos[0] = p->pos[0];
            msg.u.k2.pos[1] = p->pos[1];
            msg.u.k2.pos[2] = p->pos[2];
            msg.u.k2.angle = p->targetYaw;
            msg.u.k2.hp = p->hp;
            msg.u.k2.stamina = p->stamina;
            msg.u.k2.area = p->area;
            msg.u.k2.icons = p->statusIcons;
            msg.u.k2.hpMax = p->hpMax;
            msg.u.k2.staminaMax = p->staminaMax;
            QSync_QueuePlayerPos(p, p->pos);
            break;
        case 3:
        case 4:
        case 5:
            msg.len = 0x17;
            msg.u.k3.pos[0] = p->startPos[0];
            msg.u.k3.pos[1] = p->startPos[1];
            msg.u.k3.pos[2] = p->startPos[2];
            msg.u.k3.angle = p->startAngle;
            msg.u.k3.area = p->nextArea;
            msg.u.k3.carry = p->carry;
            msg.u.k3.mashLeft = p->mashLeft;
            QSync_QueuePlayerPos(p, p->startPos);
            p->syncTimer = gQSyncResendDelay;
            break;
        case 6:
            msg.len = 0x28;
            msg.u.k6.icons = p->statusIcons;
            msg.u.k6.weaponOut = p->weaponOut;
            msg.u.k6.mode = mode;
            msg.u.k6.pos[0] = p->pos[0];
            msg.u.k6.pos[1] = p->pos[1];
            msg.u.k6.pos[2] = p->pos[2];
            msg.u.k6.angle = p->targetYaw;
            msg.u.k6.state = p->stateGroup;
            msg.u.k6.sub = p->state;
            msg.u.k6.useItem = p->useItem;
            msg.u.k6.ammoType = p->ammoType;
            msg.u.k6.stateRand = p->stateRand;
            msg.u.k6.hpMax = p->hpMax;
            msg.u.k6.tradeWith = p->offerTo;
            msg.u.k6.tradeItem = p->offerItem;
            msg.u.k6.tradeCount = p->offerCount;
            QSync_QueuePlayerPos(p, p->pos);
            p->syncTimer = 0x1E;
            break;
        case 7:
        case 8:
            msg.len = 0xC;
            if ((u8)kind == 7) {
                msg.u.k7.asker = p->index;
                msg.u.k7.giver = mode;
            } else {
                msg.u.k7.result = mode;
                msg.u.k7.asker = p->offerState;
                msg.u.k7.giver = p->index;
            }
            msg.u.k7.item = p->offerItem;
            msg.u.k7.count = p->offerCount;
            p->syncTimer = 0x1E;
            break;
        case 10:
            msg.len = 0x1F;
            msg.u.k2.pos[0] = p->pos[0];
            msg.u.k2.pos[1] = p->pos[1];
            msg.u.k2.pos[2] = p->pos[2];
            msg.u.k2.angle = p->targetYaw;
            msg.u.k2.hp = p->hp;
            msg.u.k2.stamina = p->stamina;
            msg.u.k2.icons = p->statusIcons;
            msg.u.k2.hpMax = p->hpMax;
            msg.u.k2.staminaMax = p->staminaMax;
            msg.u.k2.hornSong = p->hornSong;
            msg.u.k2.area = p->area;
            break;
        }
        QNetIf_Post(&msg, 0);
    }
}

/* Applies game messages 1-4 (QSync_SendPlayer's) to the sender's slot; kinds
 * 7 and 8 hand an item from one player to the other (the asked player
 * checks its own offer, removes the items and answers with kind 8).
 * The game's registers (from and then d in s0, msg and then p in s1) come
 * from pl, a Player * copy of p for PouchBackup_Use (a round 11 permuter
 * find), and d set before kind (round 13's type sweep). The `switch` on
 * the map with `default:` first (case 4) is what leaves every conditional
 * branch's delay slot empty, as in the game; lp declared before p gives
 * the two-register allocation. */
void QSync_OnPlayer(u8 from, QSyncPlayerMsg *msg, s32 arg) {
    Player *pl;
    QSyncPlayerData *d;
    Player *p;
    Player *lp;
    Player *q;
    s32 kind;
    u16 item;

    if (Game_IsOnline() != 0 && Scene_InField() != 0) {
        p = (Player *)&gPlayers[from];
        d = &msg->u;
        kind = msg->kind;
        switch (kind) {
        case 1:
            p->statusIcons = d->k1.icons;
            p->weaponOut = d->k1.weaponOut;
            if (d->k1.mode & 4) {
                p->pos[0] = d->k1.pos[0];
                p->pos[1] = d->k1.pos[1];
                p->pos[2] = d->k1.pos[2];
                p->moveToFrames = 0;
            } else {
                p->moveTo[0] = d->k1.pos[0];
                p->moveTo[1] = d->k1.pos[1];
                p->moveTo[2] = d->k1.pos[2];
                Player_StartMoveTo(p, 10);
            }
            if (d->k1.mode & 8) {
                p->rot[1] = d->k1.angle;
                p->targetYaw = p->rot[1];
            } else {
                p->targetYaw = d->k1.angle;
            }
            Player_SetState(p, d->k1.state, d->k1.sub, 0);
            p->useItem = d->k1.useItem;
            p->ammoType = d->k1.ammoType;
            p->stateRand = d->k1.stateRand;
            p->chargeLevel = d->k1.chargeLevel;
            p->carry = d->k1.carry;
            p->hp = d->k1.hp;
            p->stamina = d->k1.stamina;
            p->staminaMax = d->k1.staminaMax;
            p->hpMax = d->k1.hpMax;
            p->sharpnessLevel = d->k1.sharpOrPitch;
            p->aimWeight = d->k1.aimWeight;
            p->visible = d->k1.visible;
            p->area = d->k1.area;
            p->attack = d->k1.attack;
            p->targetPos[0] = p->pos[0];
            p->targetPos[1] = p->pos[1];
            p->targetPos[2] = p->pos[2];
            if (p->stateGroup == 0 && Player_IsLocal(p) == 0) {
                switch (p->state) {
                case 0x14:
                case 0x18:
                    Item_UseTool(p);
                    break;
                case 0x2F:
                    Item_UseMisc(p);
                    break;
                case 0x63:
                    Item_UseTeamPowder(p);
                    break;
                case 0x70:
                    Item_UsePlaced(p);
                    break;
                case 0x71:
                    Item_Use(p);
                    break;
                }
            }
            break;
        case 9:
            p->statusIcons = d->k1.icons;
            p->weaponOut = d->k1.weaponOut;
            if (d->k1.mode & 4) {
                p->pos[0] = d->k1.pos[0];
                p->pos[1] = d->k1.pos[1];
                p->pos[2] = d->k1.pos[2];
                p->moveToFrames = 0;
            } else {
                p->moveTo[0] = d->k1.pos[0];
                p->moveTo[1] = d->k1.pos[1];
                p->moveTo[2] = d->k1.pos[2];
                Player_StartMoveTo(p, 10);
            }
            if (d->k1.mode & 8) {
                p->rot[1] = d->k1.angle;
                p->targetYaw = p->rot[1];
            } else {
                p->targetYaw = d->k1.angle;
            }
            Player_SetState(p, d->k1.state, d->k1.sub, 0);
            p->useItem = d->k1.useItem;
            p->ammoType = d->k1.ammoType;
            p->stateRand = d->k1.stateRand;
            p->chargeLevel = d->k1.chargeLevel;
            p->carry = d->k1.carry;
            p->hp = d->k1.hp;
            p->stamina = d->k1.stamina;
            p->staminaMax = d->k1.staminaMax;
            p->hpMax = d->k1.hpMax;
            p->aimPitchPct = d->k1.sharpOrPitch;
            p->aimWeight = d->k1.aimWeight;
            p->visible = d->k1.visible;
            p->area = d->k1.area;
            p->attack = d->k1.attack;
            p->targetPos[0] = p->pos[0];
            p->targetPos[1] = p->pos[1];
            p->targetPos[2] = p->pos[2];
            break;
        case 2:
            p->targetYaw = d->k2.angle;
            p->moveTo[0] = d->k2.pos[0];
            p->moveTo[1] = d->k2.pos[1];
            p->moveTo[2] = d->k2.pos[2];
            Player_StartMoveTo(p, 0x14);
            p->hp = d->k2.hp;
            p->stamina = d->k2.stamina;
            p->area = d->k2.area;
            p->statusIcons = d->k2.icons;
            p->hpMax = d->k2.hpMax;
            p->staminaMax = d->k2.staminaMax;
            p->targetPos[0] = d->k2.pos[0];
            p->targetPos[1] = d->k2.pos[1];
            p->targetPos[2] = d->k2.pos[2];
            break;
        case 3:
        case 4:
        case 5:
            p->startPos[0] = d->k3.pos[0];
            p->startPos[1] = d->k3.pos[1];
            p->startPos[2] = d->k3.pos[2];
            p->startAngle = d->k3.angle;
            p->targetYaw = d->k3.angle;
            p->area = d->k3.area;
            p->carry = d->k3.carry;
            p->mashLeft = (u8)d->k3.mashLeft;
            if (kind == 4) {
                switch (p->area) {
                default:
                    p->visible = 0;
                    break;
                case 0x54:
                case 0x7D:
                case 0x81:
                    p->visible = 1;
                    p->areaMoveState = 3;
                    Player_Reset(p);
                    break;
                }
            } else {
                if (kind == 3) {
                    p->areaMoveState = 0;
                } else {
                    p->areaMoveState = 1;
                }
                Player_Reset(p);
            }
            p->pos[0] = p->startPos[0];
            p->pos[1] = p->startPos[1];
            p->pos[2] = p->startPos[2];
            p->targetPos[0] = p->startPos[0];
            p->targetPos[1] = p->startPos[1];
            p->targetPos[2] = p->startPos[2];
            break;
        case 6:
            p->statusIcons = d->k6.icons;
            p->weaponOut = d->k6.weaponOut;
            if (d->k6.mode & 4) {
                p->pos[0] = d->k6.pos[0];
                p->pos[1] = d->k6.pos[1];
                p->pos[2] = d->k6.pos[2];
                p->moveToFrames = 0;
            } else {
                p->moveTo[0] = d->k6.pos[0];
                p->moveTo[1] = d->k6.pos[1];
                p->moveTo[2] = d->k6.pos[2];
                Player_StartMoveTo(p, 10);
            }
            if (d->k6.mode & 8) {
                p->rot[1] = d->k6.angle;
                p->targetYaw = p->rot[1];
            } else {
                p->targetYaw = d->k6.angle;
            }
            Player_SetState(p, d->k6.state, d->k6.sub, 0);
            p->useItem = d->k6.useItem;
            p->ammoType = d->k6.ammoType;
            p->stateRand = d->k6.stateRand;
            p->hpMax = d->k6.hpMax;
            p->offerTo = d->k6.tradeWith;
            p->offerItem = d->k6.tradeItem;
            p->offerCount = d->k6.tradeCount;
            p->targetPos[0] = p->pos[0];
            p->targetPos[1] = p->pos[1];
            p->targetPos[2] = p->pos[2];
            break;
        case 7:
            if (d->k7.giver == gScene.localPlayer) {
                p = (Player *)&gPlayers[gScene.localPlayer];
                pl = (Player *)p;
                p->offerState = d->k7.asker;
                if (Player_IsState(p, 0, 0x67) != 0 && p->offerTo == d->k7.asker && p->offerItem == d->k7.item
                    && p->offerCount == d->k7.count && Pouch_Count((Player *)p, p->offerItem) >= p->offerCount) {
                    if (Quest_IsVersus() == 0) {
                        PouchBackup_Use(pl, p->offerItem, -p->offerCount);
                    }
                    Pouch_Add((Player *)p, p->offerItem, -p->offerCount);
                    QSync_SendPlayer(p, 8, 1);
                    p->offerState = 0xFF;
                } else {
                    QSync_SendPlayer(p, 8, 2);
                }
            }
            break;
        case 8:
            if (d->k7.asker == gScene.localPlayer) {
                q = (Player *)&gPlayers[gScene.localPlayer];
                if (d->k7.result & 1) {
                    q->offerState = 1;
                    item = d->k7.item;
                    Pouch_Add((Player *)q, item, d->k7.count);
                    Notice_Push(1, 0xD, item);
                } else {
                    q->offerState = 2;
                }
            }
            break;
        case 10:
            p->hp = d->k2.hp;
            p->stamina = d->k2.stamina;
            p->statusIcons = d->k2.icons;
            p->hpMax = d->k2.hpMax;
            p->staminaMax = d->k2.staminaMax;
            p->hornSong = d->k2.hornSong;
            p->area = d->k2.area;
            lp = (Player *)&gPlayers[gScene.localPlayer];
            if (lp->area == p->area && lp->stateGroup != 3 && lp->stateGroup != 4 && lp->areaMoveState == 0) {
                switch (p->hornSong) {
                case 0:
                case 1:
                    HuntingHorn_SongFxSelf(p);
                    break;
                default:
                    if (Side_Match(lp->side, p->side, 0) == 1) {
                        lp->hornSong = p->hornSong;
                        HuntingHorn_PlaySong(lp, p->hornSong, 1);
                    } else {
                        HuntingHorn_SongFxAllies(p);
                    }
                    break;
                }
            }
            break;
        }
    }
}

/* QSync_SendEm and QSync_OnEm (game message 8, a monster's state). */
u16 Map_AreaIndex(u16 map, u16 area);
void QSync_PackEmParts(Em *, QSyncEmParts *);
void QSync_PackEmState(Em *, u8, QSyncEmMsg *);
void QSync_QueueEmState(Em *, u8);
void QSync_UnpackEmParts(Em *, QSyncEmParts *);
void QSync_ApplyEmState(Em *, u8, QSyncEmData *);
void Em_BreakParts(Em *, u16);
void Em_AddPoison(Em *em, s32 arg);
void Em_ApplyFlash(Em *em);
void Em_CheckRageByHp(Em *em, s16 n);
void Em_StartRage2(Em *em);
void Em_EndRage(Em *em);
void Em_TypeSetup(Em *em);
void Em_ResetToBasePose(Em *em);
extern s16 gEmCombatInit[];

/* Sends game message 8, a monster's state; the layout after the header
 * depends on the kind (1-4). */
void QSync_SendEm(Em *em, u8 kind, u8 arg) {
    QSyncEmMsg msg;
    u16 area;
    u16 area2;
    u8 fl;
    u8 f2;

    area = Map_AreaIndex(gScene.map, em->area);
    if (em->prevArea != 0xFFFF) {
        area2 = Map_AreaIndex(gScene.map, em->prevArea);
    } else {
        area2 = 0xFF;
    }
    if (Game_IsOnline() != 0 && Scene_InField() != 0) {
        msg.cmd = 8;
        msg.flags = 0;
        msg.kind = kind;
        switch (kind) {
        case 0:
            break;
        case 1:
            msg.len = 0x48;
            msg.u.k1.pos[0] = em->posX;
            msg.u.k1.pos[1] = em->posY;
            msg.u.k1.pos[2] = em->posZ;
            msg.u.k1.rot[0] = em->rotX;
            msg.u.k1.rot[1] = em->rotY;
            msg.u.k1.rot[2] = em->rotZ;
            msg.u.k1.stateGroup = em->syncKind;
            msg.u.k1.state = em->syncArg;
            msg.u.k1.goalKind = em->syncGoalKind;
            msg.u.k1.goalArg = em->syncGoalArg;
            msg.u.k1.goalNo = em->syncGoalNo;
            msg.u.k1.area = area;
            msg.u.k1.prevArea = area2;
            msg.u.k1.index = em->index;
            msg.u.k1.timer = em->timer;
            msg.u.k1.combat = em->combat;
            msg.u.k1.rand = em->syncRand;
            msg.u.k1.hp = em->hp;
            msg.u.k1.partsBroken = em->partsBroken;
            QSync_PackEmParts(em, &msg.u.k1.parts);
            f2 = 0;
            if (em->blind != 0) {
                f2 |= 2;
            }
            msg.u.k1.flags = f2;
            fl = 0;
            if (em->statusFlags & 4) {
                fl |= 1;
            }
            if (em->blindTimer != 0) {
                fl |= 2;
            }
            if (*(s8 *)&em->syncPulse[0] != 0) {
                fl |= 4;
            }
            if (*(s8 *)&em->syncPulse[1] != 0) {
                fl |= 8;
            }
            if (em->painted != 0) {
                fl |= 0x10;
            }
            if (em->rage != 0) {
                fl |= 0x20;
            }
            if (em->syncRequest == 1) {
                em->syncRequest = 0;
                fl |= 0x40;
            }
            if (em->syncRequest == 2) {
                em->syncRequest = 0;
                fl |= 0x80;
            }
            msg.u.k1.status = fl;
            QSync_PackEmState(em, kind, &msg);
            msg.u.k1.from = gScene.localPlayer;
            QSync_QueueEmState(em, arg);
            break;
        case 2:
            msg.len = 0x46;
            msg.u.k1.pos[0] = em->posX;
            msg.u.k1.pos[1] = em->posY;
            msg.u.k1.pos[2] = em->posZ;
            msg.u.k1.rot[0] = em->rotX;
            msg.u.k1.rot[1] = em->rotY;
            msg.u.k1.rot[2] = em->rotZ;
            msg.u.k1.stateGroup = em->stateGroup;
            msg.u.k1.state = em->state;
            msg.u.k1.goalKind = em->syncGoalKind;
            msg.u.k1.goalArg = em->syncGoalArg;
            msg.u.k1.goalNo = em->syncGoalNo;
            msg.u.k1.area = area;
            msg.u.k1.prevArea = area2;
            msg.u.k1.index = em->index;
            msg.u.k1.timer = em->timer;
            msg.u.k1.combat = em->combat;
            msg.u.k1.rand = em->syncRand;
            msg.u.k1.hp = em->hp;
            fl = 0;
            if (em->statusFlags & 4) {
                fl |= 1;
            }
            if (em->blindTimer != 0) {
                fl |= 2;
            }
            if (*(s8 *)&em->syncPulse[0] != 0) {
                fl |= 4;
            }
            if (*(s8 *)&em->syncPulse[1] != 0) {
                fl |= 8;
            }
            if (em->painted != 0) {
                fl |= 0x10;
            }
            if (em->rage != 0) {
                fl |= 0x20;
            }
            if (em->syncRequest == 1) {
                em->syncRequest = 0;
                fl |= 0x40;
            }
            if (em->syncRequest == 2) {
                em->syncRequest = 0;
                fl |= 0x80;
            }
            msg.u.k1.status = fl;
            QSync_PackEmState(em, kind, &msg);
            msg.u.k1.from = gScene.localPlayer;
            break;
        case 3:
            msg.len = 0x46;
            msg.u.k3.pos[0] = em->posX;
            msg.u.k3.pos[1] = em->posY;
            msg.u.k3.pos[2] = em->posZ;
            msg.u.k3.rot[0] = em->rotX;
            msg.u.k3.rot[1] = em->rotY;
            msg.u.k3.rot[2] = em->rotZ;
            msg.u.k3.stateGroup = em->syncKind;
            msg.u.k3.state = em->syncArg;
            msg.u.k3.index = em->index;
            msg.u.k3.hp = em->hp;
            msg.u.k3.rand = em->syncRand;
            msg.u.k3.timer = em->timer;
            msg.u.k3.goalKind = em->syncGoalKind;
            msg.u.k3.goalArg = em->syncGoalArg;
            msg.u.k3.goalNo = em->syncGoalNo;
            msg.u.k3.combat = em->combat;
            msg.u.k3.partsBroken = em->partsBroken;
            QSync_PackEmParts(em, &msg.u.k3.parts);
            f2 = 0;
            if (em->blind != 0) {
                f2 |= 2;
            }
            msg.u.k3.flags = f2;
            fl = 0;
            if (em->statusFlags & 4) {
                fl |= 1;
            }
            if (em->blindTimer != 0) {
                fl |= 2;
            }
            if (*(s8 *)&em->syncPulse[0] != 0) {
                fl |= 4;
            }
            if (*(s8 *)&em->syncPulse[1] != 0) {
                fl |= 8;
            }
            if (em->painted != 0) {
                fl |= 0x10;
            }
            if (em->rage != 0) {
                fl |= 0x20;
            }
            if (em->syncRequest == 1) {
                em->syncRequest = 0;
                fl |= 0x40;
            }
            if (em->syncRequest == 2) {
                em->syncRequest = 0;
                fl |= 0x80;
            }
            msg.u.k3.status = fl;
            msg.u.k3.from = gScene.localPlayer;
            QSync_PackEmState(em, kind, &msg);
            break;
        case 4:
            msg.len = 0x46;
            msg.u.k4.index = em->index;
            msg.u.k4.pos[0] = em->posX;
            msg.u.k4.pos[1] = em->posY;
            msg.u.k4.pos[2] = em->posZ;
            msg.u.k4.rot[0] = em->rotX;
            msg.u.k4.rot[1] = em->rotY;
            msg.u.k4.rot[2] = em->rotZ;
            msg.u.k4.hp = em->hp;
            msg.u.k4.hostNo = em->hostNo;
            msg.u.k4.area = area;
            msg.u.k4.prevArea = area2;
            msg.u.k4.painted = em->painted;
            msg.u.k4.paintTimer = em->paintTimer;
            msg.u.k4.goalKind = em->goalKind;
            msg.u.k4.goalArg = *(u16 *)&em->goalArg;
            msg.u.k4.goalNo = *(u16 *)&em->goalNo;
            msg.u.k4.poisonDamage = em->poisonDamage;
            msg.u.k4.syncKind = em->syncKind;
            msg.u.k4.partsBroken = em->partsBroken;
            QSync_PackEmParts(em, &msg.u.k4.parts);
            f2 = 0;
            if (em->blind != 0) {
                f2 |= 2;
            }
            msg.u.k4.flags = f2;
            QSync_PackEmState(em, kind, &msg);
            break;
        }
        QNetIf_Post(&msg, 0);
    }
}

/* Applies game message 8 (QSync_SendEm's) to the monster it names. */
void QSync_OnEm(u8 from, QSyncEmMsg *msg, s32 arg) {
    u8 kind;
    QSyncEmData *d;
    Em *em;
    Em *e;
    u8 y;
    u8 x;

    if (Game_IsOnline() != 0 && Scene_InField() != 0) {
        kind = msg->kind;
        d = &msg->u;
        switch (kind) {
        case 0:
            break;
        case 1:
            if (gScene.inQuest[d->k1.from] != 1) {
                break;
            }
            em = &gEms[d->k1.index];
            em->posX = d->k1.pos[0];
            em->posY = d->k1.pos[1];
            em->posZ = d->k1.pos[2];
            em->rotX = d->k1.rot[0];
            em->rotY = d->k1.rot[1];
            em->rotZ = d->k1.rot[2];
            x = d->k1.stateGroup;
            y = d->k1.state;
            em->curGoalKind = d->k1.goalKind;
            em->curGoalArg = d->k1.goalArg;
            em->curGoalNo = d->k1.goalNo;
            if (gQuest.stage != 0) {
                em->area = Map_AreaAt(gScene.map, d->k1.area);
                if (d->k1.prevArea != 0xFF) {
                    em->prevArea = Map_AreaIndex(gScene.map, d->k1.prevArea);
                } else {
                    em->prevArea = 0xFFFF;
                }
            }
            em->timer = d->k1.timer;
            em->stateRand = d->k1.rand;
            em->hp = d->k1.hp;
            if (d->k1.flags & 2) {
                em->blind = 10;
            } else {
                em->blind = 0;
            }
            if (d->k1.combat == 0) {
                Em_SetCombat(em, 0, 0);
            } else {
                Em_SetCombat(em, 1, gEmCombatInit[em->type]);
            }
            Em_BreakParts(em, d->k1.partsBroken);
            QSync_UnpackEmParts(em, &d->k1.parts);
            if (em->taskPhase > 0) {
                switch (em->type) {
                case 0x1B:
                case 0x1C:
                case 0x1F:
                    if (em->stateGroup != 5 || em->state != 4) {
                        em->stateGroup = x;
                        em->state = y;
                        Em_ChangeStateRouted(em, (s32)x, (s32)y, 0);
                        QSync_QueueEmState(em, 0);
                    }
                    break;
                default:
                    em->stateGroup = x;
                    em->state = y;
                    Em_ChangeStateRouted(em, (s32)x, (s32)y, 0);
                    QSync_QueueEmState(em, 0);
                    break;
                }
            }
            x = d->k1.status;
            if ((x & 1) && !(em->statusFlags & 4)) {
                Em_AddPoison(em, em->poisonThreshold);
            }
            if ((x & 2) && em->blindTimer == 0) {
                Em_ApplyFlash(em);
            } else if (!(x & 2) && em->blindTimer != 0) {
                em->blindTimer = 1;
            }
            if ((x & 4) && *(s8 *)&em->syncPulse[0] == 0) {
                em->syncPulse[0] = 2;
            }
            if ((x & 8) && *(s8 *)&em->syncPulse[1] == 0) {
                em->syncPulse[1] = 2;
            }
            if ((x & 0x10) && em->painted == 0) {
                em->painted = 1;
                em->paintTimer = 0x4650;
            }
            if (!(x & 0x10) && em->painted != 0) {
                em->painted = 0;
                em->paintTimer = 0;
            }
            if (x & 0x20) {
                if (em->rage == 0) {
                    Em_CheckRageByHp(em, em->rageThreshold);
                    Em_StartRage2(em);
                }
            } else if (em->rage != 0) {
                Em_EndRage(em);
            }
            if (x & 0x40) {
                Em_TypeSetup(em);
                Em_RestartScript(em);
                em->syncRequest = 0;
            }
            if (x & 0x80) {
                Em_RestartScript(em);
                em->syncRequest = 0;
            }
            QSync_ApplyEmState(em, kind, d);
            break;
        case 2:
            if (gScene.inQuest[d->k1.from] != 1) {
                break;
            }
            em = &gEms[d->k1.index];
            em->hp = d->k1.hp;
            x = d->k1.stateGroup;
            y = d->k1.state;
            if (d->k1.combat == 0) {
                Em_SetCombat(em, 0, 0);
            } else {
                Em_SetCombat(em, 1, gEmCombatInit[em->type]);
            }
            if (em->taskPhase > 0 && em->stateGroup != x && em->state != y) {
                em->posX = d->k1.pos[0];
                em->posY = d->k1.pos[1];
                em->posZ = d->k1.pos[2];
                em->rotX = d->k1.rot[0];
                em->rotY = d->k1.rot[1];
                em->rotZ = d->k1.rot[2];
            }
            x = d->k1.status;
            if ((x & 1) && !(em->statusFlags & 4)) {
                Em_AddPoison(em, em->poisonThreshold);
            }
            if ((x & 2) && em->blindTimer == 0) {
                Em_ApplyFlash(em);
            } else if (!(x & 2) && em->blindTimer != 0) {
                em->blindTimer = 1;
            }
            if ((x & 4) && *(s8 *)&em->syncPulse[0] == 0) {
                em->syncPulse[0] = 2;
            }
            if ((x & 8) && *(s8 *)&em->syncPulse[1] == 0) {
                em->syncPulse[1] = 2;
            }
            if ((x & 0x10) && em->painted == 0) {
                em->painted = 1;
                em->paintTimer = 0x4650;
            }
            if (!(x & 0x10) && em->painted != 0) {
                em->painted = 0;
                em->paintTimer = 0;
            }
            if (x & 0x20) {
                if (em->rage == 0) {
                    Em_CheckRageByHp(em, em->rageThreshold);
                    Em_StartRage2(em);
                }
            } else if (em->rage != 0) {
                Em_EndRage(em);
            }
            if (x & 0x40) {
                Em_TypeSetup(em);
                Em_RestartScript(em);
                em->syncRequest = 0;
            }
            if (x & 0x80) {
                Em_RestartScript(em);
                em->syncRequest = 0;
            }
            QSync_ApplyEmState(em, kind, d);
            break;
        case 3:
            if (gScene.inQuest[d->k3.from] != 1) {
                break;
            }
            em = &gEms[d->k3.index];
            em->posX = d->k3.pos[0];
            em->posY = d->k3.pos[1];
            em->posZ = d->k3.pos[2];
            em->rotX = d->k3.rot[0];
            em->rotY = d->k3.rot[1];
            em->rotZ = d->k3.rot[2];
            em->curGoalKind = d->k3.goalKind;
            em->curGoalArg = d->k3.goalArg;
            em->curGoalNo = d->k3.goalNo;
            em->stateRand = d->k3.rand;
            if (d->k3.flags & 2) {
                em->blind = 10;
            } else {
                em->blind = 0;
            }
            if (d->k3.combat == 0) {
                Em_SetCombat(em, 0, 0);
            } else {
                Em_SetCombat(em, 1, gEmCombatInit[em->type]);
            }
            Em_BreakParts(em, d->k3.partsBroken);
            QSync_UnpackEmParts(em, &d->k3.parts);
            x = d->k3.stateGroup;
            y = d->k3.state;
            if (em->taskPhase > 0) {
                em->stateGroup = x;
                em->state = y;
                Em_ChangeStateRouted(em, (s32)x, (s32)y, 0);
                QSync_QueueEmState(em, 0);
                switch (em->type) {
                case 1:
                case 0x2A:
                case 0x25:
                case 0xB:
                case 0x29:
                case 0x31:
                case 0xE:
                case 0x2B:
                case 0x1A:
                case 0x2C:
                    if (x == 4 && y == 0xF) {
                        Em_ResetToBasePose(em);
                    }
                    break;
                case 0x11:
                case 0x2F:
                case 0x16:
                    if (x == 4 && y == 0x11) {
                        Em_ResetToBasePose(em);
                    }
                    break;
                }
            }
            em->hp = d->k3.hp;
            em->syncRand = d->k3.rand;
            em->timer = d->k3.timer;
            x = d->k3.status;
            if ((x & 1) && !(em->statusFlags & 4)) {
                Em_AddPoison(em, em->poisonThreshold);
            }
            if ((x & 2) && em->blindTimer == 0) {
                Em_ApplyFlash(em);
            } else if (!(x & 2) && em->blindTimer != 0) {
                em->blindTimer = 1;
            }
            if ((x & 4) && *(s8 *)&em->syncPulse[0] == 0) {
                em->syncPulse[0] = 2;
            }
            if ((x & 8) && *(s8 *)&em->syncPulse[1] == 0) {
                em->syncPulse[1] = 2;
            }
            if ((x & 0x10) && em->painted == 0) {
                em->painted = 1;
                em->paintTimer = 0x4650;
            }
            if (!(x & 0x10) && em->painted != 0) {
                em->painted = 0;
                em->paintTimer = 0;
            }
            if (x & 0x20) {
                if (em->rage == 0) {
                    Em_CheckRageByHp(em, em->rageThreshold);
                    Em_StartRage2(em);
                }
            } else if (em->rage != 0) {
                Em_EndRage(em);
            }
            if (x & 0x40) {
                em->syncRequest = 0;
                Em_TypeSetup(em);
                Em_RestartScript(em);
            }
            if (x & 0x80) {
                em->syncRequest = 0;
                Em_RestartScript(em);
            }
            QSync_ApplyEmState(em, kind, d);
            break;
        case 4:
            e = &gEms[d->k4.index];
            if (e->type != 2 && e->type != 0x24 && e->type != 0x47) {
                e->posX = d->k4.pos[0];
                e->posY = d->k4.pos[1];
                e->posZ = d->k4.pos[2];
                e->rotX = d->k4.rot[0];
                e->rotY = d->k4.rot[1];
                e->rotZ = d->k4.rot[2];
            }
            if (d->k4.syncKind != 5) {
                e->hp = d->k4.hp;
            }
            e->painted = d->k4.painted;
            e->paintTimer = d->k4.paintTimer;
            e->hostNo = d->k4.hostNo;
            e->goalKind = d->k4.goalKind;
            *(u16 *)&e->goalArg = d->k4.goalArg;
            e->goalNo = d->k4.goalNo;
            if (d->k4.flags & 2) {
                e->blind = 10;
            } else {
                e->blind = 0;
            }
            e->poisonDamage = d->k4.poisonDamage;
            e->curGoalKind = e->goalKind;
            e->curGoalArg = *(u16 *)&e->goalArg;
            e->curGoalNo = *(u16 *)&e->goalNo;
            Em_BreakParts(e, d->k4.partsBroken);
            QSync_UnpackEmParts(e, &d->k4.parts);
            QSync_ApplyEmState(e, kind, d);
            if (gQuest.stage != 0) {
                e->area = Map_AreaAt(gScene.map, d->k4.area);
                if (d->k4.prevArea != 0xFF) {
                    e->prevArea = Map_AreaIndex(gScene.map, d->k4.prevArea);
                } else {
                    e->prevArea = 0xFFFF;
                }
            }
            if (gScene.localPlayer == e->hostNo) {
                e->remote = 0;
            }
            break;
        }
    }
}

void QSync_PackEmParts(Em *em, QSyncEmParts *msg) {
    msg->state[0] = em->parts[0].state;
    msg->hp[0] = em->parts[0].hp;
    msg->state[1] = em->parts[1].state;
    msg->hp[1] = em->parts[1].hp;
    msg->state[2] = em->parts[2].state;
    msg->hp[2] = em->parts[2].hp;
    msg->state[3] = em->parts[3].state;
    msg->hp[3] = em->parts[3].hp;
    msg->state[4] = em->parts[4].state;
    msg->hp[4] = em->parts[4].hp;
    msg->state[5] = em->parts[5].state;
    msg->hp[5] = em->parts[5].hp;
    msg->state[6] = em->parts[6].state;
    msg->hp[6] = em->parts[6].hp;
    msg->state[7] = em->parts[7].state;
    msg->hp[7] = em->parts[7].hp;
    msg->hp[8] = em->parts[8].hp;
}

void QSync_UnpackEmParts(Em *em, QSyncEmParts *msg) {
    s32 i;
    u8 mask = 0;

    for (i = 0; i < 8; i++) {
        if (em->parts[i].state < msg->state[i]) {
            mask |= (u8)(1 << i);
        }
        em->parts[i].state = msg->state[i];
        em->parts[i].hp = msg->hp[i];
    }
    em->parts[8].hp = msg->hp[8];
    if (mask & 0xFF) {
        Em_SpawnPartEffects(em, mask);
    }
}

void QSync_PackEmState(Em *em, u8 kind, QSyncEmMsg *msg) {
    switch (kind) {
    case 1:
        if (Quest_IsVersus() == 1) {
            msg->u.k1.state0 = em->versusGauge;
            break;
        }
        switch (em->type) {
        case 7:
        case 0x32:
        case 0x37:
        case 0x3A:
        case 0x36:
        case 0x3C:
        case 0x40:
        case 0x41:
            msg->u.k1.state0 = em->work;
            msg->u.k1.state1 = em->fortStage;
            em->syncTimer = 0x96;
            break;
        case 0xF:
        case 0x2D:
            msg->u.k1.state0 = ((EmKhezuWork *)&em->work)->unk044;
            msg->u.k1.state1 = ((EmKhezuWork *)&em->work)->unk045;
            break;
        }
        break;
    case 2:
        switch (em->type) {
        case 7:
        case 0x32:
        case 0x37:
        case 0x3A:
            msg->u.k1.state0 = em->work;
            msg->u.k1.state1 = em->fortStage;
            em->syncTimer = 0x96;
            break;
        }
        break;
    case 3:
        if (Quest_IsVersus() == 1) {
            msg->u.k3.state0 = em->versusGauge;
            break;
        }
        switch (em->type) {
        case 7:
        case 0x32:
        case 0x37:
        case 0x3A:
        case 0x36:
        case 0x3C:
        case 0x40:
        case 0x41:
            msg->u.k3.state0 = em->work;
            msg->u.k3.state1 = em->fortStage;
            em->syncTimer = 0x96;
            break;
        case 0xF:
        case 0x2D:
            msg->u.k3.state0 = ((EmKhezuWork *)&em->work)->unk044;
            msg->u.k3.state1 = ((EmKhezuWork *)&em->work)->unk045;
            break;
        }
        break;
    case 4:
        switch (em->type) {
        case 7:
        case 0x32:
        case 0x37:
        case 0x3A:
        case 0x36:
        case 0x3C:
        case 0x40:
        case 0x41:
            msg->u.k4.state0 = em->work;
            msg->u.k4.state1 = em->fortStage;
            em->syncTimer = 0x96;
            break;
        case 0xF:
        case 0x2D:
            msg->u.k4.state0 = ((EmKhezuWork *)&em->work)->unk044;
            msg->u.k4.state1 = ((EmKhezuWork *)&em->work)->unk045;
            break;
        }
        break;
    }
}

void QSync_ApplyEmState(Em *em, u8 kind, QSyncEmData *d) {
    EmFortWork *st;
    EmKhezuWork *kw;

    switch (kind) {
    case 0:
        break;
    case 1:
        if (Quest_IsVersus() == 1) {
            em->versusGauge = d->k1.state0;
            break;
        }
        switch (em->type) {
        case 7:
        case 0x32:
        case 0x37:
        case 0x3A:
        case 0x36:
        case 0x3C:
        case 0x40:
        case 0x41:
            em->work = d->k1.state0;
            em->fortStage = d->k1.state1;
            gScene.fortStage = em->fortStage;
            break;
        case 0xF:
        case 0x2D:
            ((EmKhezuWork *)&em->work)->unk044 = d->k1.state0;
            ((EmKhezuWork *)&em->work)->unk045 = d->k1.state1;
            break;
        }
        break;
    case 2:
        switch (em->type) {
        case 7:
        case 0x32:
        case 0x37:
        case 0x3A:
            em->work = d->k1.state0;
            em->fortStage = d->k1.state1;
            gScene.fortStage = em->fortStage;
            break;
        }
        break;
    case 3:
        if (Quest_IsVersus() == 1) {
            em->versusGauge = d->k3.state0;
            break;
        }
        switch (em->type) {
        case 7:
        case 0x32:
        case 0x37:
        case 0x3A:
        case 0x36:
        case 0x3C:
        case 0x40:
        case 0x41:
            em->work = d->k3.state0;
            em->fortStage = d->k3.state1;
            gScene.fortStage = em->fortStage;
            break;
        case 0xF:
        case 0x2D:
            ((EmKhezuWork *)&em->work)->unk044 = d->k3.state0;
            ((EmKhezuWork *)&em->work)->unk045 = d->k3.state1;
            break;
        }
        break;
    case 4:
        switch (em->type) {
        case 7:
        case 0x32:
        case 0x37:
        case 0x3A:
            st = (EmFortWork *)&em->work;
            if (st->stage < d->k4.state0) {
                st->stage = d->k4.state0;
            }
            if (st->fortHits < d->k4.state1) {
                st->fortHits = d->k4.state1;
            }
            gScene.fortStage = st->fortHits;
            if (em->area != Map_AreaAt(gScene.map, d->k4.area)) {
                st->stage = 0;
                Em_RestartScript(em);
            }
            if (gScene.localPlayer == em->hostNo) {
                Em_RestartScript(em);
            }
            break;
        case 0x36:
        case 0x3C:
        case 0x40:
        case 0x41:
            st = (EmFortWork *)&em->work;
            if (st->stage < d->k4.state0) {
                st->stage = d->k4.state0;
            }
            if (st->fortHits < d->k4.state1) {
                st->fortHits = d->k4.state1;
            }
            gScene.fortStage = st->fortHits;
            break;
        case 0xF:
        case 0x2D:
            ((EmKhezuWork *)&em->work)->unk044 = d->k4.state0;
            ((EmKhezuWork *)&em->work)->unk045 = d->k4.state1;
            kw = (EmKhezuWork *)&em->work;
            if (gScene.localPlayer == em->hostNo && kw->unk045 != 0) {
                Em_RestartScript(em);
            }
            break;
        case 0x33:
            if (gScene.localPlayer == em->hostNo && em->combat == 1 && em->stateArmor != 0) {
                Em_RestartScript(em);
            }
            break;
        }
        break;
    }
}

/* Builds game message 10 (QSyncStateMsg) for the local player. The sub_main
 * data without a symbol yet (0x6ADB48, 0x5CAA38, 0x5D87B1-0x5D87B9,
 * 0x6ADC98) is addressed from gScene: MWCC schedules loads through a
 * symbol as the original does, but keeps loads from a plain absolute address
 * in place. (A macro computing the same offsets does not match.) */
void QSync_SendState(u8 kind, u8 who) {
    QuestSync *g = &gQuestSync;
    Player *p = (Player *)&gPlayers[gScene.localPlayer];
    QSyncStateMsg msg;
    s32 k;

    if (Game_IsOnline() != 0 && who == gScene.localPlayer) {
        msg.flags = 0;
        msg.cmd = 10;
        msg.kind = kind;
        switch (kind) {
        case 0:
            break;
        case 1:
            msg.len = 8;
            msg.u.k1.count = gScene.playerCount;
            break;
        case 2:
            msg.len = 0x22;
            msg.u.k2.who = who;
            msg.u.k2.presetEquip = g->presetEquip[who];
            msg.u.k2.weaponType = g->weapon[who].type;
            msg.u.k2.weaponId = g->weapon[who].id;
            msg.u.k2.weaponLevel = g->weapon[who].level;
            msg.u.k2.area = g->area;
            msg.u.k2.ready = g->ready[who];
            msg.u.k2.gender = g->gender[who];
            msg.u.k2.startItemList = g->startItemList[who];
            msg.u.k2.pick = g->picks[who];
            msg.u.k2.stageId = *(u16 *)((u8 *)&gScene - 0x168);
            msg.u.k2.unk11 = g->unkDE;
            msg.u.k2.unk12 = g->unkDA;
            msg.u.k2.unk14 = g->unkDC;
            msg.u.k2.unk16 = g->unkE0;
            msg.u.k2.unk17 = g->unkDF;
            for (k = 0; k < 8; k++) {
                msg.u.k2.preset[k] = ((s8 *)&gScene - 0xE3278)[k];
            }
            break;
        case 6:
            msg.len = 0xE;
            msg.u.k6.from = who;
            msg.u.k6.arg0 = *(u16 *)((u8 *)&gScene - 0xD54FE);
            msg.u.k6.type = *((u8 *)&gScene - 0xD54FF);
            msg.u.k6.arg1 = *(s16 *)((u8 *)&gScene - 0xD54FC);
            msg.u.k6.arg2 = *(s16 *)((u8 *)&gScene - 0xD54FA);
            msg.u.k6.area = *(u16 *)((u8 *)&gScene - 0xD54F8);
            break;
        case 7:
        case 10:
            msg.u.supply.who = who;
            msg.len = 0x10;
            if (kind == 7) {
                msg.u.supply.answer = 0;
                msg.u.supply.slot = p->menuCursor;
            } else {
                msg.u.supply.answer = 1;
                msg.u.supply.taker = gScene.supplyWho;
                msg.u.supply.slot = gScene.supplySlot;
                msg.u.supply.taken[0] = gScene.supplyTaken[0];
                msg.u.supply.taken[1] = gScene.supplyTaken[1];
            }
            break;
        case 8:
            msg.u.k12.who = who;
            msg.len = 8;
            break;
        case 9:
            msg.u.k12.who = who;
            msg.len = 8;
            break;
        case 12:
            msg.u.k12.who = who;
            msg.len = 8;
            msg.u.k12.pick = *((u8 *)&gScene - 0x18) | 0x80;
            break;
        case 13:
            msg.u.k12.who = who;
            msg.len = 8;
            msg.u.k12.pick = *((u8 *)&gScene - 0x18);
            break;
        }
        QNetIf_Post(&msg, 0);
    }
}

extern u16 gNextQuestId;
/* Game message 10 (QSync_SendState builds it). Kind 1: the host's 0x6ADCEE;
 * 2: a player's QSyncState fields, and from player 0 the shared ones; 6:
 * the block at 0x5D87B0 (Quest_HandleNetMsg applies it); 7: a request to the host
 * for a supply box slot, answered with kind 10 (who took which slot, 0xFF
 * when it was taken already, and supplyTaken), where the taker gets the
 * item; 9: Hud_StartMapPing; 12, 13: g->picks[who] (13 only while bit 7 is
 * clear). Kind 7 marks the slot as QSync_OnSupply does. */
void QSync_OnState(u8 from, QSyncStateMsg *msg, s32 arg) {
    QuestSync *g = &gQuestSync;
    QSyncStateData *d;
    u16 who;
    u8 id;
    u16 item;
    s32 k;
    s8 *dst;

    if (Game_IsOnline() != 0) {
        d = &msg->u;
        switch (msg->kind) {
        case 0:
            break;
        case 1:
            if (gScene.localPlayer != gScene.host) {
                gScene.playerCount = d->k1.count;
            }
            break;
        case 2:
            who = d->k2.who;
            g->gender[who] = d->k2.gender;
            g->startItemList[who] = d->k2.startItemList;
            g->presetEquip[who] = d->k2.presetEquip;
            g->weapon[who].type = d->k2.weaponType;
            g->weapon[who].id = d->k2.weaponId;
            g->weapon[who].level = d->k2.weaponLevel;
            g->ready[who] = d->k2.ready;
            g->picks[who] = d->k2.pick;
            dst = D_6ADF26[who];
            for (k = 0; k < 8; k++) {
                dst[k] = d->k2.preset[k];
            }
            if (gScene.localPlayer != gScene.host && who == 0) {
                g->area = d->k2.area;
                gNextQuestId = d->k2.stageId;
                g->unkDE = d->k2.unk11;
                g->unkDA = d->k2.unk12;
                g->unkDC = d->k2.unk14;
                g->unkE0 = d->k2.unk16;
                g->unkDF = d->k2.unk17;
            }
            break;
        case 6:
            if (gScene.localPlayer != d->k6.from) {
                gQuest.msgFrom = d->k6.from;
                gQuest.msgArg0 = d->k6.arg0;
                gQuest.msgType = d->k6.type;
                gQuest.msgArg1 = d->k6.arg1;
                gQuest.msgArg2 = d->k6.arg2;
                gQuest.msgArea = d->k6.area;
                Quest_HandleNetMsg();
            }
            break;
        case 7:
        case 10:
            who = d->supply.who;
            id = d->supply.slot;
            if (msg->kind == 7) {
                if (gScene.localPlayer != gScene.host) {
                    break;
                }
                gScene.supplyWho = who;
                if (gScene.supplyTaken[(u16)id / 32] & (1 << (id % 32))) {
                    gScene.supplySlot = 0xFF;
                } else {
                    gScene.supplyTaken[(u16)id / 32] |= 1 << (id % 32);
                    gScene.supplySlot = id;
                }
                QSync_SendState(0xA, 0);
            } else {
                if (d->supply.taker == gScene.localPlayer && id != 0xFF) {
                    item = gScene.supply[id].item;
                    Pouch_Add(&gPlayers[d->supply.taker], item, gScene.supply[id].count);
                    switch (item) {
                    case 0x2A4:
                        Notice_Push(1, 0x13, item);
                        break;
                    case 0x2A6:
                        Notice_Push(1, 0x14, item);
                        break;
                    }
                }
                gScene.supplyTaken[0] |= d->supply.taken[0];
                gScene.supplyTaken[1] |= d->supply.taken[1];
            }
            break;
        case 8:
            break;
        case 9:
            Hud_StartMapPing(d->k12.who);
            break;
        case 12:
            gQuestSync.picks[d->k12.who] = d->k12.pick;
            break;
        case 13:
            if (!(gQuestSync.picks[d->k12.who] & 0x80)) {
                gQuestSync.picks[d->k12.who] = d->k12.pick;
            }
            break;
        }
    }
}

void QSync_SendChat(u8 who, u8 kind, char *text, u8 to) {
    QSyncChatMsg msg;
    s32 n;
    s8 i;
    u8 *dst;

    if (Game_IsOnline() != 0 && who == gScene.localPlayer) {
        n = strlen(text);
        if (n < 0x40) {
            msg.cmd = 6;
            msg.flags = 0;
            msg.kind = kind;
            switch (kind) {
            case 0:
                break;
            case 1:
                msg.c.who = who;
                msg.c.to = to;
                msg.len = 0x48;
                dst = msg.c.text;
                msg.c.n = n;
                for (i = 0; i < n; i++) {
                    *dst++ = *text++;
                }
                *dst = 0;
                break;
            }
            QNetIf_Post(&msg, 0);
        }
    }
}

/* Game message 6 (QSyncChatMsg): shows a chat text sent to the local player.
 * d is the body after the 4-byte header: the text, then n, who, to. */
void QSync_OnChat(u8 from, QSyncChatMsg *msg, s32 arg) {
    char buf[0x40];
    QSyncChat *d;
    s8 i;
    s32 n;
    char *p;
    u8 who;

    if (Game_IsOnline() != 0) {
        d = &msg->c;
        switch (msg->kind) {
        case 0:
            break;
        case 1:
            who = d->who;
            if (who != gScene.localPlayer && (d->to & (1 << gScene.localPlayer))) {
                n = d->n;
                p = buf;
                for (i = 0; i < n; i++) {
                    *p++ = d->text[i];
                }
                *p = 0;
                if (gScene.plselOpen == 0) {
                    Chat_LogAddLocal(who, d->to, buf);
                } else {
                    func_D643F0(who, d->to, buf);
                }
            }
            break;
        }
    }
}

void QSync_SendSupply(u8 kind, u8 who) {
    Player *p = (Player *)&gPlayers[gScene.localPlayer];
    QSyncSupplyMsg msg;

    if (Game_IsOnline() != 0 && who == gScene.localPlayer) {
        msg.cmd = 7;
        msg.flags = 0;
        msg.kind = kind;
        switch (kind) {
        case 0:
            break;
        case 1:
        case 2:
            msg.s.who = who;
            msg.len = 0x10;
            if (kind == 1) {
                msg.s.answer = 0;
                msg.s.slot = p->menuCursor;
            } else {
                msg.s.answer = 1;
                msg.s.taker = gScene.supplyWho;
                msg.s.slot = gScene.supplySlot;
                msg.s.taken[0] = gScene.supplyTaken[0];
                msg.s.taken[1] = gScene.supplyTaken[1];
            }
            break;
        }
        QNetIf_Post(&msg, 0);
    }
}

/* Game message 7: a supply box slot taken. Kind 1 asks the host (0x6ADF1E),
 * which marks the slot in supplyTaken and answers with kind 2 (the slot, or
 * 0xFF when it was already taken, and the whole supplyTaken). The word's
 * index goes through u16 ((u16)id / 32): with id / 32 the id's
 * zero-extension and the word's address swap registers (a1/a0 for a0/a1). */
void QSync_OnSupply(u8 from, QSyncSupplyMsg *msg, s32 arg) {
    u8 id;
    QSyncSupply *d;
    Player *p;
    u16 who;

    if (Game_IsOnline() != 0) {
        d = &msg->s;
        switch (msg->kind) {
        case 0:
            break;
        case 1:
        case 2:
            who = d->who;
            id = d->slot;
            if (msg->kind == 1) {

                if (gScene.localPlayer != gScene.host) {
                    break;
                }
                gScene.supplyWho = who;
                if (gScene.supplyTaken[(u16)id / 32] & (1 << (id % 32))) {
                    gScene.supplySlot = 0xFF;
                } else {
                    gScene.supplyTaken[(u16)id / 32] |= 1 << (id % 32);
                    gScene.supplySlot = id;
                }
                QSync_SendSupply(2, gScene.host);
            } else {
                if (d->taker == gScene.localPlayer) {
                    p = &gPlayers[d->taker];
                    p->supplyTaken = 0;
                    if (id != 0xFF) {
                        Pouch_Add(p, gScene.supply[id].item, gScene.supply[id].count);
                        SupplyBox_StartMove(gScene.supply[id].item, id);
                    }
                }
                gScene.supplyTaken[0] |= d->taken[0];
                gScene.supplyTaken[1] |= d->taken[1];
            }
            break;
        }
    }
}

extern u16 gSrvSyncDelay;
s32 QSync_Begin(void) {
    s32 i;
    s32 n;
    s32 sum;
    PlayerPend *pd;
    u16 t;

    if (Game_IsOnline() == 0) {
        return 0;
    }
    if (gScene.inQuest[gScene.localPlayer] == 0xFF) {
        Loading_Stop();
        gScene.step = 0;
        gScene.scene = 5;
        Quest_Finish();
        Game_ResetSubsystems();
        Scene_LoadOverlay(0xB);
        return 0;
    }
    pd = ((Player *)&gPlayers[gScene.localPlayer])->pend;
    pd[0].timer = 0;
    pd[0].pos[0] = 0.0f;
    pd[0].pos[1] = 0.0f;
    pd[0].pos[2] = 0.0f;
    pd = ((Player *)&gPlayers[gScene.localPlayer])->pend;
    pd[1].timer = 0;
    pd[1].pos[0] = 0.0f;
    pd[1].pos[1] = 0.0f;
    pd[1].pos[2] = 0.0f;
    gScene.syncDelay = 1;
    if (gScene.playerCount == 1) {
        return 1;
    }
    t = gSrvSyncDelay;
    if (t != 0) {
        gScene.syncDelay = t;
    } else {
        n = 0;
        sum = 0;
        for (i = 0; i < 4; i++) {
            if (gScene.localPlayer != i && gScene.inQuest[i] == 1) {
                n++;
                sum += QuestNet_GetPing(i);
            }
        }
        if (n == 0) {
            gScene.syncDelay = 8;
        } else if (sum == 0) {
            gScene.syncDelay = 8;
        } else {
            gScene.syncDelay = sum / n;
        }
        if (gScene.syncDelay <= 0) {
            gScene.syncDelay = 8;
        }
        if (gScene.syncDelay > 30) {
            gScene.syncDelay = 30;
        }
    }
    return 1;
}

void QSync_QueuePlayerPos(Player *p, f32 *pos) {
    s8 i;

    if (Game_IsOnline() != 0) {
        for (i = 0; i < 2; i++) {
            if (p->pend[i].timer == 0) {
                p->pend[i].timer = gScene.syncDelay;
                p->pend[i].pos[0] = pos[0];
                p->pend[i].pos[1] = pos[1];
                p->pend[i].pos[2] = pos[2];
                break;
            }
        }
    }
}

void QSync_UpdatePlayerPos(void) {
    Player *p = (Player *)&gPlayers[gScene.localPlayer];
    s8 i;

    if (Game_IsOnline() != 0) {
        for (i = 0; i < 2; i++) {
            if (p->pend[i].timer != 0) {
                if (--p->pend[i].timer <= 0) {
                    p->pend[i].timer = 0;
                    p->targetPos[0] = p->pend[i].pos[0];
                    p->targetPos[1] = p->pend[i].pos[1];
                    p->targetPos[2] = p->pend[i].pos[2];
                }
            }
        }
    }
}

/* Queues a monster's state to apply after the network delay (Em.spawns,
 * two slots), as QSync_QueuePlayerPos does for players. The last stores
 * go through the byte offset k: the original keeps j * sizeof(EmSpawn)
 * through the if/else (with em->spawns[j] MWCC keeps j and multiplies
 * again, through s it addresses from s). */
void QSync_QueueEmState(Em *em, u8 arg) {
    s8 i;
    s32 j;
    EmSpawn *s;
    s32 k;

    if (Game_IsOnline() != 0) {
        for (i = 0; i < 2; i++) {
            if (em->spawns[i].pending == 0) {
                j = i;
                k = j * sizeof(EmSpawn);
                em->spawns[j].pending = gScene.syncDelay;
                em->spawns[j].arg = arg;
                em->spawns[j].pos[0] = em->posX;
                em->spawns[j].pos[1] = em->posY;
                em->spawns[j].pos[2] = em->posZ;
                em->spawns[j].rot[0] = em->rotX;
                em->spawns[j].rot[1] = em->rotY;
                em->spawns[j].rot[2] = em->rotZ;
                s = &em->spawns[j];
                if (em->remote == 0) {
                    em->spawns[j].stateGroup = em->syncKind;
                    em->spawns[j].state = em->syncArg;
                    em->spawnQueued = 1;
                } else {
                    em->spawns[j].stateGroup = em->stateGroup;
                    em->spawns[j].state = em->state;
                    if (em->stateGroup != 5) {
                        s->pending = 1;
                    } else {
                        s->pending = 0;
                    }
                }
                ((EmSpawn *)((u8 *)em->spawns + k))->rand = em->syncRand;
                ((EmSpawn *)((u8 *)em->spawns + k))->goalKind = em->syncGoalKind;
                ((EmSpawn *)((u8 *)em->spawns + k))->goalArg = em->syncGoalArg;
                ((EmSpawn *)((u8 *)em->spawns + k))->goalNo = em->syncGoalNo;
                ((EmSpawn *)((u8 *)em->spawns + k))->area = em->area;
                ((EmSpawn *)((u8 *)em->spawns + k))->timer = em->timer;
                break;
            }
        }
    }
}

void QSync_UpdateEmState(Em *em) {
    s8 i;
    u8 a;
    u8 b;

    if (Game_IsOnline() != 0) {
        for (i = 0; i < 2; i++) {
            if (em->spawns[i].pending != 0) {
                if (--em->spawns[i].pending <= 0) {
                    em->spawns[i].pending = 0;
                    if (em->remote != 0) {
                        return;
                    }
                    if (em->taskPhase > 0 && (u32)(em->stateGroup - 4) > 1 && em->spawns[i].arg == 0) {
                        em->posX = em->spawns[i].pos[0];
                        em->posY = em->spawns[i].pos[1];
                        em->posZ = em->spawns[i].pos[2];
                        em->rotX = em->spawns[i].rot[0];
                        em->rotY = em->spawns[i].rot[1];
                        em->rotZ = em->spawns[i].rot[2];
                        a = em->spawns[i].stateGroup;
                        b = em->spawns[i].state;
                        em->curGoalKind = em->spawns[i].goalKind;
                        em->curGoalArg = em->spawns[i].goalArg;
                        em->curGoalNo = em->spawns[i].goalNo;
                        em->area = em->spawns[i].area;
                        em->timer = em->spawns[i].timer;
                        em->stateRand = em->spawns[i].rand;
                        em->stateGroup = a;
                        em->state = b;
                        Em_ChangeStateRouted(em, a, b, 0);
                    }
                }
            }
        }
    }
}
