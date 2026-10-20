/*
 * NETWORK: the game messages of the in-quest protocol (qsync.h), preserved
 * byte for byte for online play. Each QSync_Send* function builds one
 * message in a local buffer and posts it (QNetIf_Post, qnet_if.c);
 * QNetIf_OnMessage hands each one received to its QSync_On* handler by cmd:
 *   messages 1-4, a player's state: QSync_SendPlayer, QSync_OnPlayer;
 *   message 6, a chat text: QSync_SendChat, QSync_OnChat;
 *   message 7, the supply box: QSync_SendSupply, QSync_OnSupply;
 *   message 8, a monster's state, with its parts and its family's own
 *     state: QSync_SendEm, QSync_OnEm and the Pack and Apply functions;
 *   message 10, the quest's setup and requests: QSync_SendState,
 *     QSync_OnState.
 * The positions and monster states received wait gScene.syncDelay frames
 * before they apply (QSync_Begin sets it at the quest's start; the Queue
 * and Update functions at the end).
 */
#include "common.h"
#include "quest.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "qsync.h"
#include "area.h"
#include "overlay.h"
#include "item.h"
#include "chat_log.h"

/* The fort monsters' first two work bytes, as QSync_ApplyEmState reads them:
 * EM_KIND_LAO_SHAN_LUNG and EM_KIND_ASHEN_LAO_SHAN_LUNG, EM_KIND_SHEN_GAOREN,
 * EM_KIND_YAMA_TSUKAMI, and the Kushala Daora family (EM_KIND_KUSHALA_DAORA,
 * EM_KIND_RUSTED_KUSHALA_DAORA, EM_KIND_LUNASTRA, EM_KIND_TEOSTRA); their
 * views in em.h (EmLaoshanWork and the others) have them at the same place. */
typedef struct EmFortWork {
    u8 stage;                   /* 0x000: the fight's stage (the families' ScriptQuery returns it); the messages only raise it */
    s8 fortHits;                /* 0x001: the hits on the fort (Em.fortStage), copied to gScene.fortStage */
    u8 unk002[0x100 - 0x002];
} EmFortWork;
typedef char EmFortWork_size_check[(sizeof(EmFortWork) == sizeof(EmWork)) ? 1 : -1];

s32 strlen(char *);
void Chat_LogAddLocal(u8, u8, char *);
void Plsel_LogChatFrom(u8 slot, u8 flag, char *text);
/* gScene.userIds, the players' user ids (message 10 kind 2), under its own
 * name: QSync_OnState matches only with the address loaded whole (the
 * field form folds the offset into the store). */
extern s8 gSceneUserIds[SCENE_PLAYERS][LOBBY_USER_ID_LEN];

void QNetIf_Post(void *msg, s32 unused);
u8 Pouch_Add(Player *p, u16 item, s16 n);
void SupplyBox_StartMove(u16, u8);

s32 Game_IsOnline(void);
/* QSync_OnEm's calls narrow the group and the state to bytes, as the game's
 * code does; with the definition's s32 a and b they would pass them whole.
 * unit's own declaration: declared as the caller uses it. */
void Em_ChangeStateRouted(Em *em, u8 a, u8 b, s32 c);
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
void HuntingHorn_PlaySong(Player *p, u8 song, u8 quiet);
void HuntingHorn_SongFxAllies(Player *p);
void HuntingHorn_SongFxSelf(Player *p);

/* Sends the local player's state (messages 1-4, QSyncPlayerMsg) of the given
 * kind (QSyncPlayerKind); mode goes into the message: Player_SetState's
 * flags (k1.mode, k6.mode), the player asked for an item (kind 7) or the
 * answer (kind 8, QSYNC_ITEM_*). For the match, kind is an s32 switched on
 * as (u8), and msg.kind is stored after msg.cmd (a u8 kind, or msg.kind
 * stored first, swaps pl's and kind's registers). */
void QSync_SendPlayer(Player *pl, s32 kind, u16 mode)
{
    QSyncPlayerMsg msg;

    if (Game_IsOnline() != 0 && Scene_InField() != 0 && pl->isEm == 0 && Player_IsLocal(pl) != 0) {
        msg.flags = 0;
        msg.cmd = pl->index + QSYNC_CMD_PLAYER;
        msg.kind = kind;
        switch ((u8)kind) {
        case QSYNC_PL_STATE:
            msg.len = QSYNC_LEN_PL_STATE;
            msg.u.k1.icons = pl->statusIcons;
            msg.u.k1.weaponOut = pl->weaponOut;
            msg.u.k1.mode = mode;
            msg.u.k1.pos[0] = pl->pos[0];
            msg.u.k1.pos[1] = pl->pos[1];
            msg.u.k1.pos[2] = pl->pos[2];
            if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALK_DRAWN) != 0) {
                msg.u.k1.angle = pl->input.stickAngle + Camera_GetYaw();
            } else {
                msg.u.k1.angle = pl->targetYaw;
            }
            msg.u.k1.state = pl->stateGroup;
            msg.u.k1.sub = pl->state;
            msg.u.k1.useItem = pl->useItem;
            msg.u.k1.ammoType = pl->ammoType;
            msg.u.k1.stateRand = pl->stateRand;
            msg.u.k1.chargeLevel = pl->chargeLevel;
            msg.u.k1.carry = pl->carry;
            msg.u.k1.hp = pl->hp;
            msg.u.k1.stamina = pl->stamina;
            msg.u.k1.staminaMax = pl->staminaMax;
            msg.u.k1.hpMax = pl->hpMax;
            msg.u.k1.sharpOrPitch = pl->sharpnessLevel;
            msg.u.k1.aimWeight = pl->aimWeight;
            msg.u.k1.visible = pl->visible;
            msg.u.k1.area = pl->area;
            msg.u.k1.attack = pl->attack;
            QSync_QueuePlayerPos(pl, pl->pos);
            pl->syncTimer = gQSyncResendDelay;
            break;
        case QSYNC_PL_STATE_AIM:
            msg.len = QSYNC_LEN_PL_STATE;
            msg.u.k1.icons = pl->statusIcons;
            msg.u.k1.weaponOut = pl->weaponOut;
            msg.u.k1.mode = mode;
            msg.u.k1.pos[0] = pl->pos[0];
            msg.u.k1.pos[1] = pl->pos[1];
            msg.u.k1.pos[2] = pl->pos[2];
            msg.u.k1.angle = pl->targetYaw;
            msg.u.k1.state = pl->stateGroup;
            msg.u.k1.sub = pl->state;
            msg.u.k1.useItem = pl->useItem;
            msg.u.k1.ammoType = pl->ammoType;
            msg.u.k1.stateRand = pl->stateRand;
            msg.u.k1.chargeLevel = pl->chargeLevel;
            msg.u.k1.carry = pl->carry;
            msg.u.k1.hp = pl->hp;
            msg.u.k1.stamina = pl->stamina;
            msg.u.k1.staminaMax = pl->staminaMax;
            msg.u.k1.hpMax = pl->hpMax;
            msg.u.k1.sharpOrPitch = pl->aimPitchPct;
            msg.u.k1.aimWeight = pl->aimWeight;
            msg.u.k1.visible = pl->visible;
            msg.u.k1.area = pl->area;
            msg.u.k1.attack = pl->attack;
            QSync_QueuePlayerPos(pl, pl->pos);
            pl->syncTimer = gQSyncResendDelay;
            break;
        case QSYNC_PL_POS:
            msg.len = QSYNC_LEN_PL_POS;
            msg.u.k2.pos[0] = pl->pos[0];
            msg.u.k2.pos[1] = pl->pos[1];
            msg.u.k2.pos[2] = pl->pos[2];
            msg.u.k2.angle = pl->targetYaw;
            msg.u.k2.hp = pl->hp;
            msg.u.k2.stamina = pl->stamina;
            msg.u.k2.area = pl->area;
            msg.u.k2.icons = pl->statusIcons;
            msg.u.k2.hpMax = pl->hpMax;
            msg.u.k2.staminaMax = pl->staminaMax;
            QSync_QueuePlayerPos(pl, pl->pos);
            break;
        case QSYNC_PL_ENTER_AREA:
        case QSYNC_PL_FAINT_TO_CAMP:
        case QSYNC_PL_RELOCATED:
            msg.len = QSYNC_LEN_PL_PLACE;
            msg.u.k3.pos[0] = pl->startPos[0];
            msg.u.k3.pos[1] = pl->startPos[1];
            msg.u.k3.pos[2] = pl->startPos[2];
            msg.u.k3.angle = pl->startAngle;
            msg.u.k3.area = pl->nextArea;
            msg.u.k3.carry = pl->carry;
            msg.u.k3.mashLeft = pl->mashLeft;
            QSync_QueuePlayerPos(pl, pl->startPos);
            pl->syncTimer = gQSyncResendDelay;
            break;
        case QSYNC_PL_HANDOVER:
            msg.len = QSYNC_LEN_PL_HANDOVER;
            msg.u.k6.icons = pl->statusIcons;
            msg.u.k6.weaponOut = pl->weaponOut;
            msg.u.k6.mode = mode;
            msg.u.k6.pos[0] = pl->pos[0];
            msg.u.k6.pos[1] = pl->pos[1];
            msg.u.k6.pos[2] = pl->pos[2];
            msg.u.k6.angle = pl->targetYaw;
            msg.u.k6.state = pl->stateGroup;
            msg.u.k6.sub = pl->state;
            msg.u.k6.useItem = pl->useItem;
            msg.u.k6.ammoType = pl->ammoType;
            msg.u.k6.stateRand = pl->stateRand;
            msg.u.k6.hpMax = pl->hpMax;
            msg.u.k6.tradeWith = pl->offerTo;
            msg.u.k6.tradeItem = pl->offerItem;
            msg.u.k6.tradeCount = pl->offerCount;
            QSync_QueuePlayerPos(pl, pl->pos);
            pl->syncTimer = 30; /* frames: 1 s */
            break;
        case QSYNC_PL_ITEM_ASK:
        case QSYNC_PL_ITEM_ANSWER:
            msg.len = QSYNC_LEN_PL_ITEM;
            if ((u8)kind == QSYNC_PL_ITEM_ASK) {
                msg.u.k7.asker = pl->index;
                msg.u.k7.giver = mode;
            } else {
                msg.u.k7.result = mode;
                msg.u.k7.asker = pl->offerState;
                msg.u.k7.giver = pl->index;
            }
            msg.u.k7.item = pl->offerItem;
            msg.u.k7.count = pl->offerCount;
            pl->syncTimer = 30; /* frames: 1 s */
            break;
        case QSYNC_PL_POS_SONG:
            msg.len = QSYNC_LEN_PL_POS_SONG;
            msg.u.k2.pos[0] = pl->pos[0];
            msg.u.k2.pos[1] = pl->pos[1];
            msg.u.k2.pos[2] = pl->pos[2];
            msg.u.k2.angle = pl->targetYaw;
            msg.u.k2.hp = pl->hp;
            msg.u.k2.stamina = pl->stamina;
            msg.u.k2.icons = pl->statusIcons;
            msg.u.k2.hpMax = pl->hpMax;
            msg.u.k2.staminaMax = pl->staminaMax;
            msg.u.k2.hornSong = pl->hornSong;
            msg.u.k2.area = pl->area;
            break;
        }
        QNetIf_Post(&msg, 0);
    }
}

/* Applies game messages 1-4 (QSync_SendPlayer's) to the sender's Player;
 * kinds 7 and 8 hand an item from one player to the other (the asked
 * player checks its own offer, removes the items and answers with kind 8).
 * For the match: pl is a second Player * to the local player, passed to
 * PouchBackup_Use; data is set before kind; and the area test is a
 * `switch` with `default:` first, which leaves every conditional branch's
 * delay slot empty as in the game. */
void QSync_OnPlayer(u8 from, QSyncPlayerMsg *msg, s32 arg)
{
    Player *pl;
    QSyncPlayerData *data;
    Player *player;
    Player *local;
    Player *localPl;
    s32 kind;
    u16 item;

    if (Game_IsOnline() != 0 && Scene_InField() != 0) {
        player = (Player *)&gPlayers[from];
        data = &msg->u;
        kind = msg->kind;
        switch (kind) {
        case QSYNC_PL_STATE:
            player->statusIcons = data->k1.icons;
            player->weaponOut = data->k1.weaponOut;
            if (data->k1.mode & PL_SET_SNAP_POS) {
                player->pos[0] = data->k1.pos[0];
                player->pos[1] = data->k1.pos[1];
                player->pos[2] = data->k1.pos[2];
                player->moveToFrames = 0;
            } else {
                player->moveTo[0] = data->k1.pos[0];
                player->moveTo[1] = data->k1.pos[1];
                player->moveTo[2] = data->k1.pos[2];
                Player_StartMoveTo(player, 10); /* frames */
            }
            if (data->k1.mode & PL_SET_SNAP_YAW) {
                player->rot[1] = data->k1.angle;
                player->targetYaw = player->rot[1];
            } else {
                player->targetYaw = data->k1.angle;
            }
            Player_SetState(player, data->k1.state, data->k1.sub, 0);
            player->useItem = data->k1.useItem;
            player->ammoType = data->k1.ammoType;
            player->stateRand = data->k1.stateRand;
            player->chargeLevel = data->k1.chargeLevel;
            player->carry = data->k1.carry;
            player->hp = data->k1.hp;
            player->stamina = data->k1.stamina;
            player->staminaMax = data->k1.staminaMax;
            player->hpMax = data->k1.hpMax;
            player->sharpnessLevel = data->k1.sharpOrPitch;
            player->aimWeight = data->k1.aimWeight;
            player->visible = data->k1.visible;
            player->area = data->k1.area;
            player->attack = data->k1.attack;
            player->targetPos[0] = player->pos[0];
            player->targetPos[1] = player->pos[1];
            player->targetPos[2] = player->pos[2];
            if (player->stateGroup == PL_GROUP_ACTION && Player_IsLocal(player) == 0) {
                switch (player->state) {
                case PL_ACT_PLACE_ITEM_CROUCHED:
                case PL_ACT_PLACE_ITEM:
                    Item_UseTool(player);
                    break;
                case PL_ACT_SET_TRAP_DONE:
                    Item_UseMisc(player);
                    break;
                case PL_ACT_TEAM_ITEM_END:
                    Item_UseTeamPowder(player);
                    break;
                case PL_ACT_PLACE_LARGE_ITEM_SET:
                    Item_UsePlaced(player);
                    break;
                case PL_ACT_APPLY_ITEM:
                    Item_Use(player);
                    break;
                }
            }
            break;
        case QSYNC_PL_STATE_AIM:
            player->statusIcons = data->k1.icons;
            player->weaponOut = data->k1.weaponOut;
            if (data->k1.mode & PL_SET_SNAP_POS) {
                player->pos[0] = data->k1.pos[0];
                player->pos[1] = data->k1.pos[1];
                player->pos[2] = data->k1.pos[2];
                player->moveToFrames = 0;
            } else {
                player->moveTo[0] = data->k1.pos[0];
                player->moveTo[1] = data->k1.pos[1];
                player->moveTo[2] = data->k1.pos[2];
                Player_StartMoveTo(player, 10); /* frames */
            }
            if (data->k1.mode & PL_SET_SNAP_YAW) {
                player->rot[1] = data->k1.angle;
                player->targetYaw = player->rot[1];
            } else {
                player->targetYaw = data->k1.angle;
            }
            Player_SetState(player, data->k1.state, data->k1.sub, 0);
            player->useItem = data->k1.useItem;
            player->ammoType = data->k1.ammoType;
            player->stateRand = data->k1.stateRand;
            player->chargeLevel = data->k1.chargeLevel;
            player->carry = data->k1.carry;
            player->hp = data->k1.hp;
            player->stamina = data->k1.stamina;
            player->staminaMax = data->k1.staminaMax;
            player->hpMax = data->k1.hpMax;
            player->aimPitchPct = data->k1.sharpOrPitch;
            player->aimWeight = data->k1.aimWeight;
            player->visible = data->k1.visible;
            player->area = data->k1.area;
            player->attack = data->k1.attack;
            player->targetPos[0] = player->pos[0];
            player->targetPos[1] = player->pos[1];
            player->targetPos[2] = player->pos[2];
            break;
        case QSYNC_PL_POS:
            player->targetYaw = data->k2.angle;
            player->moveTo[0] = data->k2.pos[0];
            player->moveTo[1] = data->k2.pos[1];
            player->moveTo[2] = data->k2.pos[2];
            Player_StartMoveTo(player, 20); /* frames */
            player->hp = data->k2.hp;
            player->stamina = data->k2.stamina;
            player->area = data->k2.area;
            player->statusIcons = data->k2.icons;
            player->hpMax = data->k2.hpMax;
            player->staminaMax = data->k2.staminaMax;
            player->targetPos[0] = data->k2.pos[0];
            player->targetPos[1] = data->k2.pos[1];
            player->targetPos[2] = data->k2.pos[2];
            break;
        case QSYNC_PL_ENTER_AREA:
        case QSYNC_PL_FAINT_TO_CAMP:
        case QSYNC_PL_RELOCATED:
            player->startPos[0] = data->k3.pos[0];
            player->startPos[1] = data->k3.pos[1];
            player->startPos[2] = data->k3.pos[2];
            player->startAngle = data->k3.angle;
            player->targetYaw = data->k3.angle;
            player->area = data->k3.area;
            player->carry = data->k3.carry;
            player->mashLeft = (u8)data->k3.mashLeft;
            if (kind == QSYNC_PL_FAINT_TO_CAMP) {
                switch (player->area) {
                default:
                    player->visible = 0;
                    break;
                case AREA_TOWER_0x54:
                case AREA_TOWER_3:
                case AREA_TOWER_7:
                    player->visible = 1;
                    player->areaMoveState = PL_AREA_MOVE_RESPAWN;
                    Player_Reset(player);
                    break;
                }
            } else {
                if (kind == QSYNC_PL_ENTER_AREA) {
                    player->areaMoveState = PL_AREA_MOVE_NONE;
                } else {
                    player->areaMoveState = PL_AREA_MOVE_ENTER;
                }
                Player_Reset(player);
            }
            player->pos[0] = player->startPos[0];
            player->pos[1] = player->startPos[1];
            player->pos[2] = player->startPos[2];
            player->targetPos[0] = player->startPos[0];
            player->targetPos[1] = player->startPos[1];
            player->targetPos[2] = player->startPos[2];
            break;
        case QSYNC_PL_HANDOVER:
            player->statusIcons = data->k6.icons;
            player->weaponOut = data->k6.weaponOut;
            if (data->k6.mode & PL_SET_SNAP_POS) {
                player->pos[0] = data->k6.pos[0];
                player->pos[1] = data->k6.pos[1];
                player->pos[2] = data->k6.pos[2];
                player->moveToFrames = 0;
            } else {
                player->moveTo[0] = data->k6.pos[0];
                player->moveTo[1] = data->k6.pos[1];
                player->moveTo[2] = data->k6.pos[2];
                Player_StartMoveTo(player, 10); /* frames */
            }
            if (data->k6.mode & PL_SET_SNAP_YAW) {
                player->rot[1] = data->k6.angle;
                player->targetYaw = player->rot[1];
            } else {
                player->targetYaw = data->k6.angle;
            }
            Player_SetState(player, data->k6.state, data->k6.sub, 0);
            player->useItem = data->k6.useItem;
            player->ammoType = data->k6.ammoType;
            player->stateRand = data->k6.stateRand;
            player->hpMax = data->k6.hpMax;
            player->offerTo = data->k6.tradeWith;
            player->offerItem = data->k6.tradeItem;
            player->offerCount = data->k6.tradeCount;
            player->targetPos[0] = player->pos[0];
            player->targetPos[1] = player->pos[1];
            player->targetPos[2] = player->pos[2];
            break;
        case QSYNC_PL_ITEM_ASK:
            if (data->k7.giver == gScene.localPlayer) {
                player = (Player *)&gPlayers[gScene.localPlayer];
                pl = (Player *)player;
                player->offerState = data->k7.asker;
                if (Player_IsState(player, PL_GROUP_ACTION, PL_ACT_OFFER_ITEM) != 0 && player->offerTo == data->k7.asker && player->offerItem == data->k7.item
                    && player->offerCount == data->k7.count && Pouch_Count((Player *)player, player->offerItem) >= player->offerCount) {
                    if (Quest_IsVersus() == 0) {
                        PouchBackup_Use(pl, player->offerItem, -player->offerCount);
                    }
                    Pouch_Add((Player *)player, player->offerItem, -player->offerCount);
                    QSync_SendPlayer(player, QSYNC_PL_ITEM_ANSWER, QSYNC_ITEM_GIVEN);
                    player->offerState = PL_OFFER_GIVEN;
                } else {
                    QSync_SendPlayer(player, QSYNC_PL_ITEM_ANSWER, QSYNC_ITEM_REFUSED);
                }
            }
            break;
        case QSYNC_PL_ITEM_ANSWER:
            if (data->k7.asker == gScene.localPlayer) {
                localPl = (Player *)&gPlayers[gScene.localPlayer];
                if (data->k7.result & QSYNC_ITEM_GIVEN) {
                    localPl->offerState = PL_OFFER_RECEIVED;
                    item = data->k7.item;
                    Pouch_Add((Player *)localPl, item, data->k7.count);
                    Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_RECEIVED, item);
                } else {
                    localPl->offerState = PL_OFFER_REFUSED;
                }
            }
            break;
        case QSYNC_PL_POS_SONG:
            player->hp = data->k2.hp;
            player->stamina = data->k2.stamina;
            player->statusIcons = data->k2.icons;
            player->hpMax = data->k2.hpMax;
            player->staminaMax = data->k2.staminaMax;
            player->hornSong = data->k2.hornSong;
            player->area = data->k2.area;
            local = (Player *)&gPlayers[gScene.localPlayer];
            if (local->area == player->area && local->stateGroup != PL_GROUP_FAINT && local->stateGroup != PL_GROUP_EVENT && local->areaMoveState == PL_AREA_MOVE_NONE) {
                switch (player->hornSong) {
                case 0:
                case 1:
                    HuntingHorn_SongFxSelf(player);
                    break;
                default:
                    if (Side_Match(local->side, player->side, 0) == 1) {
                        local->hornSong = player->hornSong;
                        HuntingHorn_PlaySong(local, player->hornSong, 1);
                    } else {
                        HuntingHorn_SongFxAllies(player);
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
void QSync_SendEm(Em *em, u8 kind, u8 arg)
{
    QSyncEmMsg msg;
    u16 areaIdx;
    u16 prevAreaIdx;
    u8 statusBits;
    u8 flagBits;

    areaIdx = Map_AreaIndex(gScene.map, em->area);
    if (em->prevArea != AREA_NONE) {
        prevAreaIdx = Map_AreaIndex(gScene.map, em->prevArea);
    } else {
        prevAreaIdx = QSYNC_AREA_NONE;
    }
    if (Game_IsOnline() != 0 && Scene_InField() != 0) {
        msg.cmd = QSYNC_CMD_EM;
        msg.flags = 0;
        msg.kind = kind;
        switch (kind) {
        case QSYNC_EMMSG_NONE:
            break;
        case QSYNC_EMMSG_STATE:
            msg.len = QSYNC_LEN_EM_STATE;
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
            msg.u.k1.area = areaIdx;
            msg.u.k1.prevArea = prevAreaIdx;
            msg.u.k1.index = em->index;
            msg.u.k1.timer = em->timer;
            msg.u.k1.combat = em->combat;
            msg.u.k1.rand = em->syncRand;
            msg.u.k1.hp = em->hp;
            msg.u.k1.partsBroken = em->partsBroken;
            QSync_PackEmParts(em, &msg.u.k1.parts);
            flagBits = 0;
            if (em->blind != 0) {
                flagBits |= QSYNC_EM_BLIND;
            }
            msg.u.k1.flags = flagBits;
            statusBits = 0;
            if (em->statusFlags & EM_STATUSF_POISONED) {
                statusBits |= QSYNC_EM_POISONED;
            }
            if (em->blindTimer != 0) {
                statusBits |= QSYNC_EM_FLASHED;
            }
            if (em->syncPulse[0] != 0) {
                statusBits |= QSYNC_EM_PULSE0;
            }
            if (em->syncPulse[1] != 0) {
                statusBits |= QSYNC_EM_PULSE1;
            }
            if (em->painted != 0) {
                statusBits |= QSYNC_EM_PAINTED;
            }
            if (em->rage != 0) {
                statusBits |= QSYNC_EM_ENRAGED;
            }
            if (em->syncRequest == 1) {
                em->syncRequest = 0;
                statusBits |= QSYNC_EM_RESTART;
            }
            if (em->syncRequest == 2) {
                em->syncRequest = 0;
                statusBits |= QSYNC_EM_RESTART2;
            }
            msg.u.k1.status = statusBits;
            QSync_PackEmState(em, kind, &msg);
            msg.u.k1.from = gScene.localPlayer;
            QSync_QueueEmState(em, arg);
            break;
        case QSYNC_EMMSG_STATE_NO_PARTS:
            msg.len = QSYNC_LEN_EM;
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
            msg.u.k1.area = areaIdx;
            msg.u.k1.prevArea = prevAreaIdx;
            msg.u.k1.index = em->index;
            msg.u.k1.timer = em->timer;
            msg.u.k1.combat = em->combat;
            msg.u.k1.rand = em->syncRand;
            msg.u.k1.hp = em->hp;
            statusBits = 0;
            if (em->statusFlags & EM_STATUSF_POISONED) {
                statusBits |= QSYNC_EM_POISONED;
            }
            if (em->blindTimer != 0) {
                statusBits |= QSYNC_EM_FLASHED;
            }
            if (em->syncPulse[0] != 0) {
                statusBits |= QSYNC_EM_PULSE0;
            }
            if (em->syncPulse[1] != 0) {
                statusBits |= QSYNC_EM_PULSE1;
            }
            if (em->painted != 0) {
                statusBits |= QSYNC_EM_PAINTED;
            }
            if (em->rage != 0) {
                statusBits |= QSYNC_EM_ENRAGED;
            }
            if (em->syncRequest == 1) {
                em->syncRequest = 0;
                statusBits |= QSYNC_EM_RESTART;
            }
            if (em->syncRequest == 2) {
                em->syncRequest = 0;
                statusBits |= QSYNC_EM_RESTART2;
            }
            msg.u.k1.status = statusBits;
            QSync_PackEmState(em, kind, &msg);
            msg.u.k1.from = gScene.localPlayer;
            break;
        case QSYNC_EMMSG_SYNC:
            msg.len = QSYNC_LEN_EM;
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
            flagBits = 0;
            if (em->blind != 0) {
                flagBits |= QSYNC_EM_BLIND;
            }
            msg.u.k3.flags = flagBits;
            statusBits = 0;
            if (em->statusFlags & EM_STATUSF_POISONED) {
                statusBits |= QSYNC_EM_POISONED;
            }
            if (em->blindTimer != 0) {
                statusBits |= QSYNC_EM_FLASHED;
            }
            if (em->syncPulse[0] != 0) {
                statusBits |= QSYNC_EM_PULSE0;
            }
            if (em->syncPulse[1] != 0) {
                statusBits |= QSYNC_EM_PULSE1;
            }
            if (em->painted != 0) {
                statusBits |= QSYNC_EM_PAINTED;
            }
            if (em->rage != 0) {
                statusBits |= QSYNC_EM_ENRAGED;
            }
            if (em->syncRequest == 1) {
                em->syncRequest = 0;
                statusBits |= QSYNC_EM_RESTART;
            }
            if (em->syncRequest == 2) {
                em->syncRequest = 0;
                statusBits |= QSYNC_EM_RESTART2;
            }
            msg.u.k3.status = statusBits;
            msg.u.k3.from = gScene.localPlayer;
            QSync_PackEmState(em, kind, &msg);
            break;
        case QSYNC_EMMSG_HANDOVER:
            msg.len = QSYNC_LEN_EM;
            msg.u.k4.index = em->index;
            msg.u.k4.pos[0] = em->posX;
            msg.u.k4.pos[1] = em->posY;
            msg.u.k4.pos[2] = em->posZ;
            msg.u.k4.rot[0] = em->rotX;
            msg.u.k4.rot[1] = em->rotY;
            msg.u.k4.rot[2] = em->rotZ;
            msg.u.k4.hp = em->hp;
            msg.u.k4.hostNo = em->hostNo;
            msg.u.k4.area = areaIdx;
            msg.u.k4.prevArea = prevAreaIdx;
            msg.u.k4.painted = em->painted;
            msg.u.k4.paintTimer = em->paintTimer;
            msg.u.k4.goalKind = em->goalKind;
            /* the s16 fields read as u16: the game loads them unsigned */
            msg.u.k4.goalArg = *(u16 *)&em->goalArg;
            msg.u.k4.goalNo = *(u16 *)&em->goalNo;
            msg.u.k4.poisonDamage = em->poisonDamage;
            msg.u.k4.syncKind = em->syncKind;
            msg.u.k4.partsBroken = em->partsBroken;
            QSync_PackEmParts(em, &msg.u.k4.parts);
            flagBits = 0;
            if (em->blind != 0) {
                flagBits |= QSYNC_EM_BLIND;
            }
            msg.u.k4.flags = flagBits;
            QSync_PackEmState(em, kind, &msg);
            break;
        }
        QNetIf_Post(&msg, 0);
    }
}

/* Applies game message 8 (QSync_SendEm's) to the monster it names: its
 * position, goal, area, hit points, parts and statuses, and its state
 * (queued for the network delay, QSync_QueueEmState), only while the sender
 * is in the quest. Kind 4 also makes the local console the monster's host
 * when the message names it. */
void QSync_OnEm(u8 from, QSyncEmMsg *msg, s32 arg)
{
    u8 kind;
    QSyncEmData *data;
    Em *em;
    Em *target;
    u8 state;
    u8 group;
    u8 status;

    if (Game_IsOnline() != 0 && Scene_InField() != 0) {
        kind = msg->kind;
        data = &msg->u;
        switch (kind) {
        case QSYNC_EMMSG_NONE:
            break;
        case QSYNC_EMMSG_STATE:
            if (gScene.inQuest[data->k1.from] != IN_QUEST_YES) {
                break;
            }
            em = &gEms[data->k1.index];
            em->posX = data->k1.pos[0];
            em->posY = data->k1.pos[1];
            em->posZ = data->k1.pos[2];
            em->rotX = data->k1.rot[0];
            em->rotY = data->k1.rot[1];
            em->rotZ = data->k1.rot[2];
            group = data->k1.stateGroup;
            state = data->k1.state;
            em->curGoalKind = data->k1.goalKind;
            em->curGoalArg = data->k1.goalArg;
            em->curGoalNo = data->k1.goalNo;
            if (gQuest.stage != 0) {
                em->area = Map_AreaAt(gScene.map, data->k1.area);
                if (data->k1.prevArea != QSYNC_AREA_NONE) {
                    em->prevArea = Map_AreaIndex(gScene.map, data->k1.prevArea);
                } else {
                    em->prevArea = AREA_NONE;
                }
            }
            em->timer = data->k1.timer;
            em->stateRand = data->k1.rand;
            em->hp = data->k1.hp;
            if (data->k1.flags & QSYNC_EM_BLIND) {
                em->blind = 10; /* frames */
            } else {
                em->blind = 0;
            }
            if (data->k1.combat == 0) {
                Em_SetCombat(em, 0, 0);
            } else {
                Em_SetCombat(em, 1, gEmCombatInit[em->type]);
            }
            Em_BreakParts(em, data->k1.partsBroken);
            QSync_UnpackEmParts(em, &data->k1.parts);
            if (em->taskPhase > 0) {
                switch (em->type) {
                case EM_KIND_VELOCIDROME:
                case EM_KIND_GENDROME:
                case EM_KIND_IODROME:
                    /* not while in their defeat group's state 4 */
                    if (em->stateGroup != EM_GROUP_DEFEAT || em->state != 4) {
                        em->stateGroup = group;
                        em->state = state;
                        Em_ChangeStateRouted(em, (s32)group, (s32)state, 0);
                        QSync_QueueEmState(em, 0);
                    }
                    break;
                default:
                    em->stateGroup = group;
                    em->state = state;
                    Em_ChangeStateRouted(em, (s32)group, (s32)state, 0);
                    QSync_QueueEmState(em, 0);
                    break;
                }
            }
            status = data->k1.status;
            if ((status & QSYNC_EM_POISONED) && !(em->statusFlags & EM_STATUSF_POISONED)) {
                Em_AddPoison(em, em->poisonThreshold);
            }
            if ((status & QSYNC_EM_FLASHED) && em->blindTimer == 0) {
                Em_ApplyFlash(em);
            } else if (!(status & QSYNC_EM_FLASHED) && em->blindTimer != 0) {
                em->blindTimer = 1;
            }
            if ((status & QSYNC_EM_PULSE0) && em->syncPulse[0] == 0) {
                em->syncPulse[0] = 2;
            }
            if ((status & QSYNC_EM_PULSE1) && em->syncPulse[1] == 0) {
                em->syncPulse[1] = 2;
            }
            if ((status & QSYNC_EM_PAINTED) && em->painted == 0) {
                em->painted = 1;
                em->paintTimer = EM_PAINT_FRAMES; /* 10 min */
            }
            if (!(status & QSYNC_EM_PAINTED) && em->painted != 0) {
                em->painted = 0;
                em->paintTimer = 0;
            }
            if (status & QSYNC_EM_ENRAGED) {
                if (em->rage == 0) {
                    Em_CheckRageByHp(em, em->rageThreshold);
                    Em_StartRage2(em);
                }
            } else if (em->rage != 0) {
                Em_EndRage(em);
            }
            if (status & QSYNC_EM_RESTART) {
                Em_TypeSetup(em);
                Em_RestartScript(em);
                em->syncRequest = 0;
            }
            if (status & QSYNC_EM_RESTART2) {
                Em_RestartScript(em);
                em->syncRequest = 0;
            }
            QSync_ApplyEmState(em, kind, data);
            break;
        case QSYNC_EMMSG_STATE_NO_PARTS:
            if (gScene.inQuest[data->k1.from] != IN_QUEST_YES) {
                break;
            }
            em = &gEms[data->k1.index];
            em->hp = data->k1.hp;
            group = data->k1.stateGroup;
            state = data->k1.state;
            if (data->k1.combat == 0) {
                Em_SetCombat(em, 0, 0);
            } else {
                Em_SetCombat(em, 1, gEmCombatInit[em->type]);
            }
            if (em->taskPhase > 0 && em->stateGroup != group && em->state != state) {
                em->posX = data->k1.pos[0];
                em->posY = data->k1.pos[1];
                em->posZ = data->k1.pos[2];
                em->rotX = data->k1.rot[0];
                em->rotY = data->k1.rot[1];
                em->rotZ = data->k1.rot[2];
            }
            status = data->k1.status;
            if ((status & QSYNC_EM_POISONED) && !(em->statusFlags & EM_STATUSF_POISONED)) {
                Em_AddPoison(em, em->poisonThreshold);
            }
            if ((status & QSYNC_EM_FLASHED) && em->blindTimer == 0) {
                Em_ApplyFlash(em);
            } else if (!(status & QSYNC_EM_FLASHED) && em->blindTimer != 0) {
                em->blindTimer = 1;
            }
            if ((status & QSYNC_EM_PULSE0) && em->syncPulse[0] == 0) {
                em->syncPulse[0] = 2;
            }
            if ((status & QSYNC_EM_PULSE1) && em->syncPulse[1] == 0) {
                em->syncPulse[1] = 2;
            }
            if ((status & QSYNC_EM_PAINTED) && em->painted == 0) {
                em->painted = 1;
                em->paintTimer = EM_PAINT_FRAMES; /* 10 min */
            }
            if (!(status & QSYNC_EM_PAINTED) && em->painted != 0) {
                em->painted = 0;
                em->paintTimer = 0;
            }
            if (status & QSYNC_EM_ENRAGED) {
                if (em->rage == 0) {
                    Em_CheckRageByHp(em, em->rageThreshold);
                    Em_StartRage2(em);
                }
            } else if (em->rage != 0) {
                Em_EndRage(em);
            }
            if (status & QSYNC_EM_RESTART) {
                Em_TypeSetup(em);
                Em_RestartScript(em);
                em->syncRequest = 0;
            }
            if (status & QSYNC_EM_RESTART2) {
                Em_RestartScript(em);
                em->syncRequest = 0;
            }
            QSync_ApplyEmState(em, kind, data);
            break;
        case QSYNC_EMMSG_SYNC:
            if (gScene.inQuest[data->k3.from] != IN_QUEST_YES) {
                break;
            }
            em = &gEms[data->k3.index];
            em->posX = data->k3.pos[0];
            em->posY = data->k3.pos[1];
            em->posZ = data->k3.pos[2];
            em->rotX = data->k3.rot[0];
            em->rotY = data->k3.rot[1];
            em->rotZ = data->k3.rot[2];
            em->curGoalKind = data->k3.goalKind;
            em->curGoalArg = data->k3.goalArg;
            em->curGoalNo = data->k3.goalNo;
            em->stateRand = data->k3.rand;
            if (data->k3.flags & QSYNC_EM_BLIND) {
                em->blind = 10; /* frames */
            } else {
                em->blind = 0;
            }
            if (data->k3.combat == 0) {
                Em_SetCombat(em, 0, 0);
            } else {
                Em_SetCombat(em, 1, gEmCombatInit[em->type]);
            }
            Em_BreakParts(em, data->k3.partsBroken);
            QSync_UnpackEmParts(em, &data->k3.parts);
            group = data->k3.stateGroup;
            state = data->k3.state;
            if (em->taskPhase > 0) {
                em->stateGroup = group;
                em->state = state;
                Em_ChangeStateRouted(em, (s32)group, (s32)state, 0);
                QSync_QueueEmState(em, 0);
                switch (em->type) {
                case EM_KIND_RATHIAN:
                case EM_KIND_GOLD_RATHIAN:
                case EM_KIND_PINK_RATHIAN:
                case EM_KIND_RATHALOS:
                case EM_KIND_SILVER_RATHALOS:
                case EM_KIND_AZURE_RATHALOS:
                case EM_KIND_DIABLOS:
                case EM_KIND_BLACK_DIABLOS:
                case EM_KIND_MONOBLOS:
                case EM_KIND_WHITE_MONOBLOS:
                    /* reaction state 0xF: the Rathalos family's
                     * RATH_REACT_PART_BREAK_END (0x11 for the Gravios kinds) */
                    if (group == EM_GROUP_REACT && state == 0xF) {
                        Em_ResetToBasePose(em);
                    }
                    break;
                case EM_KIND_GRAVIOS:
                case EM_KIND_BLACK_GRAVIOS:
                case EM_KIND_BASARIOS:
                    if (group == EM_GROUP_REACT && state == 0x11) {
                        Em_ResetToBasePose(em);
                    }
                    break;
                }
            }
            em->hp = data->k3.hp;
            em->syncRand = data->k3.rand;
            em->timer = data->k3.timer;
            status = data->k3.status;
            if ((status & QSYNC_EM_POISONED) && !(em->statusFlags & EM_STATUSF_POISONED)) {
                Em_AddPoison(em, em->poisonThreshold);
            }
            if ((status & QSYNC_EM_FLASHED) && em->blindTimer == 0) {
                Em_ApplyFlash(em);
            } else if (!(status & QSYNC_EM_FLASHED) && em->blindTimer != 0) {
                em->blindTimer = 1;
            }
            if ((status & QSYNC_EM_PULSE0) && em->syncPulse[0] == 0) {
                em->syncPulse[0] = 2;
            }
            if ((status & QSYNC_EM_PULSE1) && em->syncPulse[1] == 0) {
                em->syncPulse[1] = 2;
            }
            if ((status & QSYNC_EM_PAINTED) && em->painted == 0) {
                em->painted = 1;
                em->paintTimer = EM_PAINT_FRAMES; /* 10 min */
            }
            if (!(status & QSYNC_EM_PAINTED) && em->painted != 0) {
                em->painted = 0;
                em->paintTimer = 0;
            }
            if (status & QSYNC_EM_ENRAGED) {
                if (em->rage == 0) {
                    Em_CheckRageByHp(em, em->rageThreshold);
                    Em_StartRage2(em);
                }
            } else if (em->rage != 0) {
                Em_EndRage(em);
            }
            if (status & QSYNC_EM_RESTART) {
                em->syncRequest = 0;
                Em_TypeSetup(em);
                Em_RestartScript(em);
            }
            if (status & QSYNC_EM_RESTART2) {
                em->syncRequest = 0;
                Em_RestartScript(em);
            }
            QSync_ApplyEmState(em, kind, data);
            break;
        case QSYNC_EMMSG_HANDOVER:
            target = &gEms[data->k4.index];
            if (target->type != EM_KIND_FATALIS && target->type != EM_KIND_FATALIS_B && target->type != EM_KIND_FATALIS_C) {
                target->posX = data->k4.pos[0];
                target->posY = data->k4.pos[1];
                target->posZ = data->k4.pos[2];
                target->rotX = data->k4.rot[0];
                target->rotY = data->k4.rot[1];
                target->rotZ = data->k4.rot[2];
            }
            if (data->k4.syncKind != EM_GROUP_DEFEAT) {
                target->hp = data->k4.hp;
            }
            target->painted = data->k4.painted;
            target->paintTimer = data->k4.paintTimer;
            target->hostNo = data->k4.hostNo;
            target->goalKind = data->k4.goalKind;
            target->goalArg = data->k4.goalArg;
            target->goalNo = data->k4.goalNo;
            if (data->k4.flags & QSYNC_EM_BLIND) {
                target->blind = 10; /* frames */
            } else {
                target->blind = 0;
            }
            target->poisonDamage = data->k4.poisonDamage;
            target->curGoalKind = target->goalKind;
            /* read as u16, as in QSync_SendEm */
            target->curGoalArg = *(u16 *)&target->goalArg;
            target->curGoalNo = *(u16 *)&target->goalNo;
            Em_BreakParts(target, data->k4.partsBroken);
            QSync_UnpackEmParts(target, &data->k4.parts);
            QSync_ApplyEmState(target, kind, data);
            if (gQuest.stage != 0) {
                target->area = Map_AreaAt(gScene.map, data->k4.area);
                if (data->k4.prevArea != QSYNC_AREA_NONE) {
                    target->prevArea = Map_AreaIndex(gScene.map, data->k4.prevArea);
                } else {
                    target->prevArea = AREA_NONE;
                }
            }
            if (gScene.localPlayer == target->hostNo) {
                target->remote = 0;
            }
            break;
        }
    }
}

/*
 * Copies a monster's eight breakable parts (state and hit points) and the
 * ninth part's hit points into a state message.
 */
void QSync_PackEmParts(Em *em, QSyncEmParts *msg)
{
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

/*
 * Applies a state message's parts to a monster, and spawns the break
 * effects of each part whose state went up (Em_SpawnPartEffects, a bit per
 * part).
 */
void QSync_UnpackEmParts(Em *em, QSyncEmParts *msg)
{
    s32 i;
    u8 mask = 0;

    for (i = 0; i < QSYNC_EM_BREAK_PARTS; i++) {
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

/*
 * Puts a monster's own state into a state message (kinds 1 to 3): the
 * versus gauge in a versus quest, else the fort monsters' work state and
 * fort stage (restarting their sync timer, 0x96 frames), or the Khezu
 * family's two work bytes.
 */
void QSync_PackEmState(Em *em, u8 kind, QSyncEmMsg *msg)
{
    switch (kind) {
    case QSYNC_EMMSG_STATE:
        if (Quest_IsVersus() == 1) {
            msg->u.k1.state0 = em->versusGauge;
            break;
        }
        switch (em->type) {
        case EM_KIND_LAO_SHAN_LUNG:
        case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        case EM_KIND_SHEN_GAOREN:
        case EM_KIND_YAMA_TSUKAMI:
        case EM_KIND_KUSHALA_DAORA:
        case EM_KIND_RUSTED_KUSHALA_DAORA:
        case EM_KIND_LUNASTRA:
        case EM_KIND_TEOSTRA:
            msg->u.k1.state0 = em->work;
            msg->u.k1.state1 = em->fortStage;
            em->syncTimer = 150; /* frames: 5 s */
            break;
        case EM_KIND_KHEZU:
        case EM_KIND_RED_KHEZU:
            msg->u.k1.state0 = ((EmKhezuWork *)&em->work)->unk044;
            msg->u.k1.state1 = ((EmKhezuWork *)&em->work)->unk045;
            break;
        }
        break;
    case QSYNC_EMMSG_STATE_NO_PARTS:
        switch (em->type) {
        case EM_KIND_LAO_SHAN_LUNG:
        case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        case EM_KIND_SHEN_GAOREN:
        case EM_KIND_YAMA_TSUKAMI:
            msg->u.k1.state0 = em->work;
            msg->u.k1.state1 = em->fortStage;
            em->syncTimer = 150; /* frames: 5 s */
            break;
        }
        break;
    case QSYNC_EMMSG_SYNC:
        if (Quest_IsVersus() == 1) {
            msg->u.k3.state0 = em->versusGauge;
            break;
        }
        switch (em->type) {
        case EM_KIND_LAO_SHAN_LUNG:
        case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        case EM_KIND_SHEN_GAOREN:
        case EM_KIND_YAMA_TSUKAMI:
        case EM_KIND_KUSHALA_DAORA:
        case EM_KIND_RUSTED_KUSHALA_DAORA:
        case EM_KIND_LUNASTRA:
        case EM_KIND_TEOSTRA:
            msg->u.k3.state0 = em->work;
            msg->u.k3.state1 = em->fortStage;
            em->syncTimer = 150; /* frames: 5 s */
            break;
        case EM_KIND_KHEZU:
        case EM_KIND_RED_KHEZU:
            msg->u.k3.state0 = ((EmKhezuWork *)&em->work)->unk044;
            msg->u.k3.state1 = ((EmKhezuWork *)&em->work)->unk045;
            break;
        }
        break;
    case QSYNC_EMMSG_HANDOVER:
        switch (em->type) {
        case EM_KIND_LAO_SHAN_LUNG:
        case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        case EM_KIND_SHEN_GAOREN:
        case EM_KIND_YAMA_TSUKAMI:
        case EM_KIND_KUSHALA_DAORA:
        case EM_KIND_RUSTED_KUSHALA_DAORA:
        case EM_KIND_LUNASTRA:
        case EM_KIND_TEOSTRA:
            msg->u.k4.state0 = em->work;
            msg->u.k4.state1 = em->fortStage;
            em->syncTimer = 150; /* frames: 5 s */
            break;
        case EM_KIND_KHEZU:
        case EM_KIND_RED_KHEZU:
            msg->u.k4.state0 = ((EmKhezuWork *)&em->work)->unk044;
            msg->u.k4.state1 = ((EmKhezuWork *)&em->work)->unk045;
            break;
        }
        break;
    }
}

/*
 * Applies a state message's own state to a monster, as QSync_PackEmState
 * packs it; a fort monster's stage also goes into gScene.fortStage.
 */
void QSync_ApplyEmState(Em *em, u8 kind, QSyncEmData *data)
{
    EmFortWork *fort;
    EmKhezuWork *khezu;

    switch (kind) {
    case QSYNC_EMMSG_NONE:
        break;
    case QSYNC_EMMSG_STATE:
        if (Quest_IsVersus() == 1) {
            em->versusGauge = data->k1.state0;
            break;
        }
        switch (em->type) {
        case EM_KIND_LAO_SHAN_LUNG:
        case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        case EM_KIND_SHEN_GAOREN:
        case EM_KIND_YAMA_TSUKAMI:
        case EM_KIND_KUSHALA_DAORA:
        case EM_KIND_RUSTED_KUSHALA_DAORA:
        case EM_KIND_LUNASTRA:
        case EM_KIND_TEOSTRA:
            em->work = data->k1.state0;
            em->fortStage = data->k1.state1;
            gScene.fortStage = em->fortStage;
            break;
        case EM_KIND_KHEZU:
        case EM_KIND_RED_KHEZU:
            ((EmKhezuWork *)&em->work)->unk044 = data->k1.state0;
            ((EmKhezuWork *)&em->work)->unk045 = data->k1.state1;
            break;
        }
        break;
    case QSYNC_EMMSG_STATE_NO_PARTS:
        switch (em->type) {
        case EM_KIND_LAO_SHAN_LUNG:
        case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        case EM_KIND_SHEN_GAOREN:
        case EM_KIND_YAMA_TSUKAMI:
            em->work = data->k1.state0;
            em->fortStage = data->k1.state1;
            gScene.fortStage = em->fortStage;
            break;
        }
        break;
    case QSYNC_EMMSG_SYNC:
        if (Quest_IsVersus() == 1) {
            em->versusGauge = data->k3.state0;
            break;
        }
        switch (em->type) {
        case EM_KIND_LAO_SHAN_LUNG:
        case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        case EM_KIND_SHEN_GAOREN:
        case EM_KIND_YAMA_TSUKAMI:
        case EM_KIND_KUSHALA_DAORA:
        case EM_KIND_RUSTED_KUSHALA_DAORA:
        case EM_KIND_LUNASTRA:
        case EM_KIND_TEOSTRA:
            em->work = data->k3.state0;
            em->fortStage = data->k3.state1;
            gScene.fortStage = em->fortStage;
            break;
        case EM_KIND_KHEZU:
        case EM_KIND_RED_KHEZU:
            ((EmKhezuWork *)&em->work)->unk044 = data->k3.state0;
            ((EmKhezuWork *)&em->work)->unk045 = data->k3.state1;
            break;
        }
        break;
    case QSYNC_EMMSG_HANDOVER:
        switch (em->type) {
        case EM_KIND_LAO_SHAN_LUNG:
        case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        case EM_KIND_SHEN_GAOREN:
        case EM_KIND_YAMA_TSUKAMI:
            fort = (EmFortWork *)&em->work;
            if (fort->stage < data->k4.state0) {
                fort->stage = data->k4.state0;
            }
            if (fort->fortHits < data->k4.state1) {
                fort->fortHits = data->k4.state1;
            }
            gScene.fortStage = fort->fortHits;
            if (em->area != Map_AreaAt(gScene.map, data->k4.area)) {
                fort->stage = 0;
                Em_RestartScript(em);
            }
            if (gScene.localPlayer == em->hostNo) {
                Em_RestartScript(em);
            }
            break;
        case EM_KIND_KUSHALA_DAORA:
        case EM_KIND_RUSTED_KUSHALA_DAORA:
        case EM_KIND_LUNASTRA:
        case EM_KIND_TEOSTRA:
            fort = (EmFortWork *)&em->work;
            if (fort->stage < data->k4.state0) {
                fort->stage = data->k4.state0;
            }
            if (fort->fortHits < data->k4.state1) {
                fort->fortHits = data->k4.state1;
            }
            gScene.fortStage = fort->fortHits;
            break;
        case EM_KIND_KHEZU:
        case EM_KIND_RED_KHEZU:
            ((EmKhezuWork *)&em->work)->unk044 = data->k4.state0;
            ((EmKhezuWork *)&em->work)->unk045 = data->k4.state1;
            khezu = (EmKhezuWork *)&em->work;
            if (gScene.localPlayer == em->hostNo && khezu->unk045 != 0) {
                Em_RestartScript(em);
            }
            break;
        case EM_KIND_BLANGONGA:
            if (gScene.localPlayer == em->hostNo && em->combat == 1 && em->stateArmor != 0) {
                Em_RestartScript(em);
            }
            break;
        }
        break;
    }
}

/* Builds game message 10 (QSyncStateMsg) of the given kind
 * (QSyncStateKind) for the local player and posts it; who must be the local
 * player. Some of sub_main's data is read as byte offsets from gScene:
 * gNextQuestId, gMyUserId (the local player's user id), gQuest's net
 * message (Quest.msgType to msgArea) and gQuestSync.pick. The match needs
 * this form: MWCC schedules loads through a symbol as the game does, while
 * the field forms, or a macro computing the same offsets, schedule them
 * differently. */
void QSync_SendState(u8 kind, u8 who)
{
    QuestSync *sync = &gQuestSync;
    Player *pl = (Player *)&gPlayers[gScene.localPlayer];
    QSyncStateMsg msg;
    s32 k;

    if (Game_IsOnline() != 0 && who == gScene.localPlayer) {
        msg.flags = 0;
        msg.cmd = QSYNC_CMD_QUEST;
        msg.kind = kind;
        switch (kind) {
        case QSYNC_STATE_NONE:
            break;
        case QSYNC_STATE_PLAYER_COUNT:
            msg.len = QSYNC_LEN_ST_SHORT;
            msg.u.k1.count = gScene.playerCount;
            break;
        case QSYNC_STATE_SETUP:
            msg.len = QSYNC_LEN_ST_SETUP;
            msg.u.k2.who = who;
            msg.u.k2.presetEquip = sync->presetEquip[who];
            msg.u.k2.weaponType = sync->weapon[who].type;
            msg.u.k2.weaponId = sync->weapon[who].id;
            msg.u.k2.weaponLevel = sync->weapon[who].level;
            msg.u.k2.area = sync->area;
            msg.u.k2.ready = sync->ready[who];
            msg.u.k2.gender = sync->gender[who];
            msg.u.k2.startItemList = sync->startItemList[who];
            msg.u.k2.pick = sync->picks[who];
            msg.u.k2.stageId = *(u16 *)((u8 *)&gScene - 0x168);
            msg.u.k2.unk11 = sync->unkDE;
            msg.u.k2.unk12 = sync->unkDA;
            msg.u.k2.unk14 = sync->unkDC;
            msg.u.k2.unk16 = sync->unkE0;
            msg.u.k2.unk17 = sync->unkDF;
            for (k = 0; k < LOBBY_USER_ID_LEN; k++) {
                msg.u.k2.userId[k] = ((s8 *)&gScene - 0xE3278)[k];
            }
            break;
        case QSYNC_STATE_QUEST_EVENT:
            msg.len = QSYNC_LEN_ST_QUEST_EVENT;
            msg.u.k6.from = who;
            msg.u.k6.arg0 = *(u16 *)((u8 *)&gScene - 0xD54FE);
            msg.u.k6.type = *((u8 *)&gScene - 0xD54FF);
            msg.u.k6.arg1 = *(s16 *)((u8 *)&gScene - 0xD54FC);
            msg.u.k6.arg2 = *(s16 *)((u8 *)&gScene - 0xD54FA);
            msg.u.k6.area = *(u16 *)((u8 *)&gScene - 0xD54F8);
            break;
        case QSYNC_STATE_SUPPLY_ASK:
        case QSYNC_STATE_SUPPLY_ANSWER:
            msg.u.supply.who = who;
            msg.len = QSYNC_LEN_SUPPLY;
            if (kind == QSYNC_STATE_SUPPLY_ASK) {
                msg.u.supply.answer = 0;
                msg.u.supply.slot = pl->menuCursor;
            } else {
                msg.u.supply.answer = 1;
                msg.u.supply.taker = gScene.supplyWho;
                msg.u.supply.slot = gScene.supplySlot;
                msg.u.supply.taken[0] = gScene.supplyTaken[0];
                msg.u.supply.taken[1] = gScene.supplyTaken[1];
            }
            break;
        case QSYNC_STATE_IDLE:
            msg.u.k12.who = who;
            msg.len = QSYNC_LEN_ST_SHORT;
            break;
        case QSYNC_STATE_MAP_PING:
            msg.u.k12.who = who;
            msg.len = QSYNC_LEN_ST_SHORT;
            break;
        case QSYNC_STATE_PICK_TAKEN:
            msg.u.k12.who = who;
            msg.len = QSYNC_LEN_ST_SHORT;
            msg.u.k12.pick = *((u8 *)&gScene - 0x18) | QSYNC_PICK_TAKEN;
            break;
        case QSYNC_STATE_PICK_CURSOR:
            msg.u.k12.who = who;
            msg.len = QSYNC_LEN_ST_SHORT;
            msg.u.k12.pick = *((u8 *)&gScene - 0x18);
            break;
        }
        QNetIf_Post(&msg, 0);
    }
}

extern u16 gNextQuestId;
/* Applies game message 10 (QSync_SendState's), by kind (QSyncStateKind):
 * the host's player count; a player's part of gQuestSync and its user id
 * (into gScene.userIds), and from player 0 the shared values; gQuest's net
 * message (Quest_HandleNetMsg applies it); a request to the host for a
 * supply box slot, which marks the slot as QSync_OnSupply does and answers
 * with QSYNC_STATE_SUPPLY_ANSWER (the slot, or QSYNC_SLOT_TAKEN, and the
 * taken slots), where the taker gets the item; a map ping; the preset
 * hunter a player took or points at (the cursor's only while it isn't
 * taken). */
void QSync_OnState(u8 from, QSyncStateMsg *msg, s32 arg)
{
    QuestSync *sync = &gQuestSync;
    QSyncStateData *data;
    u16 who;
    u8 id;
    u16 item;
    s32 k;
    s8 *dst;

    if (Game_IsOnline() != 0) {
        data = &msg->u;
        switch (msg->kind) {
        case QSYNC_STATE_NONE:
            break;
        case QSYNC_STATE_PLAYER_COUNT:
            if (gScene.localPlayer != gScene.host) {
                gScene.playerCount = data->k1.count;
            }
            break;
        case QSYNC_STATE_SETUP:
            who = data->k2.who;
            sync->gender[who] = data->k2.gender;
            sync->startItemList[who] = data->k2.startItemList;
            sync->presetEquip[who] = data->k2.presetEquip;
            sync->weapon[who].type = data->k2.weaponType;
            sync->weapon[who].id = data->k2.weaponId;
            sync->weapon[who].level = data->k2.weaponLevel;
            sync->ready[who] = data->k2.ready;
            sync->picks[who] = data->k2.pick;
            dst = gSceneUserIds[who];
            for (k = 0; k < LOBBY_USER_ID_LEN; k++) {
                dst[k] = data->k2.userId[k];
            }
            if (gScene.localPlayer != gScene.host && who == 0) {
                sync->area = data->k2.area;
                gNextQuestId = data->k2.stageId;
                sync->unkDE = data->k2.unk11;
                sync->unkDA = data->k2.unk12;
                sync->unkDC = data->k2.unk14;
                sync->unkE0 = data->k2.unk16;
                sync->unkDF = data->k2.unk17;
            }
            break;
        case QSYNC_STATE_QUEST_EVENT:
            if (gScene.localPlayer != data->k6.from) {
                gQuest.msgFrom = data->k6.from;
                gQuest.msgArg0 = data->k6.arg0;
                gQuest.msgType = data->k6.type;
                gQuest.msgArg1 = data->k6.arg1;
                gQuest.msgArg2 = data->k6.arg2;
                gQuest.msgArea = data->k6.area;
                Quest_HandleNetMsg();
            }
            break;
        case QSYNC_STATE_SUPPLY_ASK:
        case QSYNC_STATE_SUPPLY_ANSWER:
            who = data->supply.who;
            id = data->supply.slot;
            if (msg->kind == QSYNC_STATE_SUPPLY_ASK) {
                if (gScene.localPlayer != gScene.host) {
                    break;
                }
                gScene.supplyWho = who;
                if (gScene.supplyTaken[(u16)id / 32] & (1 << (id % 32))) {
                    gScene.supplySlot = QSYNC_SLOT_TAKEN;
                } else {
                    gScene.supplyTaken[(u16)id / 32] |= 1 << (id % 32);
                    gScene.supplySlot = id;
                }
                QSync_SendState(QSYNC_STATE_SUPPLY_ANSWER, 0);
            } else {
                if (data->supply.taker == gScene.localPlayer && id != QSYNC_SLOT_TAKEN) {
                    item = gScene.supply[id].item;
                    Pouch_Add(&gPlayers[data->supply.taker], item, gScene.supply[id].count);
                    switch (item) {
                    case 0x2A4:
                        Notice_Push(1, 0x13, item);
                        break;
                    case ITEM_ID_DELIVERY:
                        Notice_Push(1, 0x14, item);
                        break;
                    }
                }
                gScene.supplyTaken[0] |= data->supply.taken[0];
                gScene.supplyTaken[1] |= data->supply.taken[1];
            }
            break;
        case QSYNC_STATE_IDLE:
            break;
        case QSYNC_STATE_MAP_PING:
            Hud_StartMapPing(data->k12.who);
            break;
        case QSYNC_STATE_PICK_TAKEN:
            gQuestSync.picks[data->k12.who] = data->k12.pick;
            break;
        case QSYNC_STATE_PICK_CURSOR:
            if (!(gQuestSync.picks[data->k12.who] & QSYNC_PICK_TAKEN)) {
                gQuestSync.picks[data->k12.who] = data->k12.pick;
            }
            break;
        }
    }
}

/*
 * Sends the local player's chat text (kind 1, under 0x40 characters) to
 * the player toPlayer, while online.
 */
void QSync_SendChat(u8 who, u8 kind, char *text, u8 toPlayer)
{
    QSyncChatMsg msg;
    s32 n;
    s8 i;
    u8 *dst;

    if (Game_IsOnline() != 0 && who == gScene.localPlayer) {
        n = strlen(text);
        if (n < QSYNC_CHAT_TEXT_MAX) {
            msg.cmd = QSYNC_CMD_CHAT;
            msg.flags = 0;
            msg.kind = kind;
            switch (kind) {
            case QSYNC_CHAT_NONE:
                break;
            case QSYNC_CHAT_TEXT:
                msg.c.who = who;
                msg.c.to = toPlayer;
                msg.len = QSYNC_LEN_CHAT;
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
 * chat is the body after the 4-byte header: the text, then n, who, to. */
void QSync_OnChat(u8 from, QSyncChatMsg *msg, s32 arg)
{
    char buf[QSYNC_CHAT_TEXT_MAX];
    QSyncChat *chat;
    s8 i;
    s32 n;
    char *cursor;
    u8 who;

    if (Game_IsOnline() != 0) {
        chat = &msg->c;
        switch (msg->kind) {
        case QSYNC_CHAT_NONE:
            break;
        case QSYNC_CHAT_TEXT:
            who = chat->who;
            if (who != gScene.localPlayer && (chat->to & (1 << gScene.localPlayer))) {
                n = chat->n;
                cursor = buf;
                for (i = 0; i < n; i++) {
                    *cursor++ = chat->text[i];
                }
                *cursor = 0;
                if (gScene.plselOpen == 0) {
                    Chat_LogAddLocal(who, chat->to, buf);
                } else {
                    Plsel_LogChatFrom(who, chat->to, buf);
                }
            }
            break;
        }
    }
}

/*
 * Sends the local player's supply box message while online: kind 1 asks
 * the host for the slot under the cursor, kind 2 is the host's answer
 * (the player answered, the slot and the taken slots' bits).
 */
void QSync_SendSupply(u8 kind, u8 who)
{
    Player *pl = (Player *)&gPlayers[gScene.localPlayer];
    QSyncSupplyMsg msg;

    if (Game_IsOnline() != 0 && who == gScene.localPlayer) {
        msg.cmd = QSYNC_CMD_SUPPLY;
        msg.flags = 0;
        msg.kind = kind;
        switch (kind) {
        case QSYNC_SUPPLY_NONE:
            break;
        case QSYNC_SUPPLY_ASK:
        case QSYNC_SUPPLY_ANSWER:
            msg.s.who = who;
            msg.len = QSYNC_LEN_SUPPLY;
            if (kind == QSYNC_SUPPLY_ASK) {
                msg.s.answer = 0;
                msg.s.slot = pl->menuCursor;
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

/* Applies game message 7, a supply box slot taken. QSYNC_SUPPLY_ASK asks the
 * host (gScene.host), which marks the slot in supplyTaken and answers with
 * QSYNC_SUPPLY_ANSWER (the slot, or QSYNC_SLOT_TAKEN, and the whole
 * supplyTaken); the asker gets the item and the box's move. The word's
 * index is (u16)id / 32 for the match: id / 32 swaps two registers. */
void QSync_OnSupply(u8 from, QSyncSupplyMsg *msg, s32 arg)
{
    u8 id;
    QSyncSupply *box;
    Player *pl;
    u16 who;

    if (Game_IsOnline() != 0) {
        box = &msg->s;
        switch (msg->kind) {
        case QSYNC_SUPPLY_NONE:
            break;
        case QSYNC_SUPPLY_ASK:
        case QSYNC_SUPPLY_ANSWER:
            who = box->who;
            id = box->slot;
            if (msg->kind == QSYNC_SUPPLY_ASK) {
                if (gScene.localPlayer != gScene.host) {
                    break;
                }
                gScene.supplyWho = who;
                if (gScene.supplyTaken[(u16)id / 32] & (1 << (id % 32))) {
                    gScene.supplySlot = QSYNC_SLOT_TAKEN;
                } else {
                    gScene.supplyTaken[(u16)id / 32] |= 1 << (id % 32);
                    gScene.supplySlot = id;
                }
                QSync_SendSupply(QSYNC_SUPPLY_ANSWER, gScene.host);
            } else {
                if (box->taker == gScene.localPlayer) {
                    pl = &gPlayers[box->taker];
                    pl->supplyTaken = 0;
                    if (id != QSYNC_SLOT_TAKEN) {
                        Pouch_Add(pl, gScene.supply[id].item, gScene.supply[id].count);
                        SupplyBox_StartMove(gScene.supply[id].item, id);
                    }
                }
                gScene.supplyTaken[0] |= box->taken[0];
                gScene.supplyTaken[1] |= box->taken[1];
            }
            break;
        }
    }
}

extern u16 gSrvSyncDelay;
/*
 * Prepares the local player's sync at the quest's start: a player no
 * longer in the quest goes to the results (SCENE_RESULTS, plsel's overlay)
 * and gets 0; else the queued positions are cleared and the sync delay set
 * (frames a position or a state waits before it applies): the server's
 * value, else the mean ping of the peers in the quest, 8 without one,
 * within 1 to 30. Returns 1 when online and in the quest.
 */
s32 QSync_Begin(void)
{
    s32 i;
    s32 n;
    s32 sum;
    PlayerPend *pending;
    u16 delay;

    if (Game_IsOnline() == 0) {
        return 0;
    }
    if (gScene.inQuest[gScene.localPlayer] == IN_QUEST_NO_PLAYER) {
        Loading_Stop();
        gScene.step = 0;
        gScene.scene = SCENE_RESULTS;
        Quest_Finish();
        Game_ResetSubsystems();
        Scene_LoadOverlay(OVERLAY_PLSEL);
        return 0;
    }
    pending = ((Player *)&gPlayers[gScene.localPlayer])->pend;
    pending[0].timer = 0;
    pending[0].pos[0] = 0.0f;
    pending[0].pos[1] = 0.0f;
    pending[0].pos[2] = 0.0f;
    pending = ((Player *)&gPlayers[gScene.localPlayer])->pend;
    pending[1].timer = 0;
    pending[1].pos[0] = 0.0f;
    pending[1].pos[1] = 0.0f;
    pending[1].pos[2] = 0.0f;
    gScene.syncDelay = 1;
    if (gScene.playerCount == 1) {
        return 1;
    }
    delay = gSrvSyncDelay;
    if (delay != 0) {
        gScene.syncDelay = delay;
    } else {
        n = 0;
        sum = 0;
        for (i = 0; i < SCENE_PLAYERS; i++) {
            if (gScene.localPlayer != i && gScene.inQuest[i] == IN_QUEST_YES) {
                n++;
                sum += QuestNet_GetPing(i);
            }
        }
        if (n == 0) {
            gScene.syncDelay = QSYNC_DELAY_DEFAULT;
        } else if (sum == 0) {
            gScene.syncDelay = QSYNC_DELAY_DEFAULT;
        } else {
            gScene.syncDelay = sum / n;
        }
        if (gScene.syncDelay <= 0) {
            gScene.syncDelay = QSYNC_DELAY_DEFAULT;
        }
        if (gScene.syncDelay > QSYNC_DELAY_MAX) {
            gScene.syncDelay = QSYNC_DELAY_MAX;
        }
    }
    return 1;
}

/*
 * Queues a player's target position to apply after the sync delay (two
 * slots, Player.pend), while online.
 */
void QSync_QueuePlayerPos(Player *pl, f32 *pos)
{
    s8 i;

    if (Game_IsOnline() != 0) {
        for (i = 0; i < 2; i++) {
            if (pl->pend[i].timer == 0) {
                pl->pend[i].timer = gScene.syncDelay;
                pl->pend[i].pos[0] = pos[0];
                pl->pend[i].pos[1] = pos[1];
                pl->pend[i].pos[2] = pos[2];
                break;
            }
        }
    }
}

/*
 * Counts down the local player's queued positions and moves its target
 * position to each one whose delay is over.
 */
void QSync_UpdatePlayerPos(void)
{
    Player *pl = (Player *)&gPlayers[gScene.localPlayer];
    s8 i;

    if (Game_IsOnline() != 0) {
        for (i = 0; i < 2; i++) {
            if (pl->pend[i].timer != 0) {
                if (--pl->pend[i].timer <= 0) {
                    pl->pend[i].timer = 0;
                    pl->targetPos[0] = pl->pend[i].pos[0];
                    pl->targetPos[1] = pl->pend[i].pos[1];
                    pl->targetPos[2] = pl->pend[i].pos[2];
                }
            }
        }
    }
}

/* Queues a monster's state to apply after the network delay (Em.spawns,
 * two slots), as QSync_QueuePlayerPos does for players. The last stores
 * go through the byte offset k for the match: the game keeps the slot's
 * offset across the if/else (em->spawns[j] multiplies again, spawn
 * addresses from spawn). */
void QSync_QueueEmState(Em *em, u8 arg)
{
    s8 i;
    s32 j;
    EmSpawn *spawn;
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
                spawn = &em->spawns[j];
                if (em->remote == 0) {
                    em->spawns[j].stateGroup = em->syncKind;
                    em->spawns[j].state = em->syncArg;
                    em->spawnQueued = 1;
                } else {
                    em->spawns[j].stateGroup = em->stateGroup;
                    em->spawns[j].state = em->state;
                    if (em->stateGroup != EM_GROUP_DEFEAT) {
                        spawn->pending = 1;
                    } else {
                        spawn->pending = 0;
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

/*
 * Counts down a monster's queued states (QSync_QueueEmState) and, while
 * Em.remote is clear, applies each one whose delay is over: its position,
 * rotation, state, goal, area, timer and random value (only once its task
 * runs, outside EM_GROUP_REACT and EM_GROUP_DEFEAT, and for a queued arg
 * of 0).
 */
void QSync_UpdateEmState(Em *em)
{
    s8 i;
    u8 groupId;
    u8 stateId;

    if (Game_IsOnline() != 0) {
        for (i = 0; i < 2; i++) {
            if (em->spawns[i].pending != 0) {
                if (--em->spawns[i].pending <= 0) {
                    em->spawns[i].pending = 0;
                    if (em->remote != 0) {
                        return;
                    }
                    if (em->taskPhase > 0 && (u32)(em->stateGroup - EM_GROUP_REACT) > 1 && em->spawns[i].arg == 0) {
                        em->posX = em->spawns[i].pos[0];
                        em->posY = em->spawns[i].pos[1];
                        em->posZ = em->spawns[i].pos[2];
                        em->rotX = em->spawns[i].rot[0];
                        em->rotY = em->spawns[i].rot[1];
                        em->rotZ = em->spawns[i].rot[2];
                        groupId = em->spawns[i].stateGroup;
                        stateId = em->spawns[i].state;
                        em->curGoalKind = em->spawns[i].goalKind;
                        em->curGoalArg = em->spawns[i].goalArg;
                        em->curGoalNo = em->spawns[i].goalNo;
                        em->area = em->spawns[i].area;
                        em->timer = em->spawns[i].timer;
                        em->stateRand = em->spawns[i].rand;
                        em->stateGroup = groupId;
                        em->state = stateId;
                        Em_ChangeStateRouted(em, groupId, stateId, 0);
                    }
                }
            }
        }
    }
}
