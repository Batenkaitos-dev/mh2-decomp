/*
 * The local town player's input and the town's gestures.
 *
 * TownPc_HandleInput runs once a frame for each town player: it starts the
 * walk, the dash or the dash's turn round from the stick (TownPc_MoveKind),
 * and for the local player the action button: on the townsperson it faces
 * a talk (the Poogie's menus, the hairdresser, another player's card), on
 * the action spot it stands at (Player.spot, an ActionSpot of kind enum
 * TownSpotKind) the spot's menu or act: the seats, the quest counter and
 * the gate, the box, the bed, the dig spots, the contests, the boards.
 * The town's menu mode (gTownMode, enum TownMode) says what is open;
 * Town_UpdateMode runs it. TownPc_SteerToDest moves the other members'
 * players toward the points their messages send.
 *
 * The second half is the town's gesture states (state group
 * TOWN_PC_GROUP_GESTURE): the town's copies of the field's gestures
 * (Player_StateGesture*, the same motions), started by TownPc_StartGesture
 * from Player.actRequest and run by TownPc_RunGestureState.
 *
 * The file's data holds the one-shot gestures' table, then the forge's
 * recipe and upgrade tables, which only the forge's menus read.
 */
#include "common.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "area.h"
#include "quest.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "lobby_npc.h"
#include "lobby_net.h"


s32 Player_IsLocal(void *obj);
void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);
void Actor_SetStateFlags(Player *obj, u8 state, u8 sub, u16 flags);
void Actor_SetPosture(void *a, u16 mode, s32 b, s32 c);
void TownPc_StartGesture(Player *arg0);
void Actor_SetStateNow(Player *obj, u8 state, u8 sub, u16 flags);
s32 Motion_ReachedFrame(void *, s32, f32);
void Player_SetPosture(Player *p, u16 mode, s16 flag1, s16 flag2);
void TownMsg_Send0AAll(Player *);
void Actor_ReturnToIdle(void *obj, s32 a1, s32 n, s32 m);
void Player_ClearFlag(void *obj, u32 flag);
void Player_SetFlag(void *obj, u32 flag);


extern s16 gTownGestureOneShots[];

extern Em *gDelayedSeNpc;
extern s32 gDelayedSeTimer;
extern s32 gDelayedSeId;

/*
 * Queues the sound seId of npc's sound bank to play 20 frames from now: one
 * sound at a time, a new call replaces the pending one. The town's mode
 * update counts the timer down and plays it.
 */
void Npc_PlaySeDelayed(Em *npc, void *seId)
{
    gDelayedSeNpc = npc;
    gDelayedSeId = (s32) seId;
    gDelayedSeTimer = 20;
}

/*
 * Marks the player free this frame (it may use items) and, when the pouch
 * has asked for an offer, starts it: the offer state and the offer message
 * to every member. The idle and move states call it each frame.
 */
void TownPc_StartRequestedOffer(Player *pl)
{
    pl->freeState = 1;
    if (pl->offerRequest != 0) {
        pl->offerStarted = 1;
        Actor_SetStateNow(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_OFFER_ITEM, 0x10);
        TownMsg_Send0AAll(pl);
    }
}

void Vec_Copy(f32 *dst, f32 *src);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void TownPc_StartMoveTo(Player *obj, s16 n);

/*
 * Sends the local player walking to its action spot's point, to face the
 * spot's angle, in the bed's lie-down state with the motion's root
 * movement off. The bed's menu calls it when the player goes to bed. The
 * spot's offset is a zero vector turned by the angle, which adds nothing;
 * the game's code computes it all the same.
 */
void TownPc_WalkToBed(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    f32 spotMtx[16];
    f32 spotPos[3];
    f32 offset[3];
    f32 rotatedOffset[3];
    s32 angles[3];
    u16 spotYaw;

    spotPos[0] = pl->spot->x;
    spotPos[1] = pl->spot->y;
    spotPos[2] = pl->spot->z;
    spotYaw = pl->spot->angle;
    Vec_Copy(pl->moveTo, spotPos);
    angles[1] = spotYaw;
    angles[0] = 0;
    angles[2] = 0;
    Mtx_FromAnglesXYZ(angles, spotMtx);
    Vec_Set(offset, 0.0f, 0.0f, 0.0f);
    Vec_Rotate(rotatedOffset, offset, spotMtx);
    pl->moveTo[0] += rotatedOffset[0];
    pl->moveTo[1] += rotatedOffset[1];
    pl->moveTo[2] += rotatedOffset[2];
    pl->targetYaw = spotYaw;
    TownPc_StartMoveTo(pl, 20 /* frames */);
    Actor_SetStateFlags(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_BED_LIE_DOWN, 0);
    Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
}

s32 Cond_Check(u16 id);
s32 Monster_IsHunted(u16 em);
s32 Save_TestEventFlag(s32 flag);
u8 Village_FacilityLevel(s32 k);

/*
 * Returns 1 when the bed's sleep event of the current area is due: in
 * AREA_HUB_0xAC once the mine is at level 6 and Lao-Shan Lung has been
 * hunted, in AREA_HUB_0x57 once condition 0x51 holds; each only while its
 * flag is clear. Returns 0 otherwise.
 */
s32 Town_SleepEventPending(void)
{
    switch (gScene.area) {
    case AREA_HUB_0xAC:
        if (Save_TestEventFlag(EVENT_FLAG_MINE_STRONG_PICK) == 0 && (Village_FacilityLevel(VILLAGE_FACILITY_MINE) & 0xFF) >= 6 && Monster_IsHunted(EM_KIND_LAO_SHAN_LUNG) == 1) {
            return 1;
        }
        break;
    case AREA_HUB_0x57:
        if (Save_TestEventFlag(EVENT_FLAG_HOUSE_SLEEP_EVENT) == 0 && Cond_Check(0x51U) == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

u16 LobbyPad_Get(s32);
u8 TownPc_MoveKind(Player *a);
u16 TownPc_InputYaw(Player *, s32);
s32 Hud_ItemGetBusy(void);
s32 TownPc_TakeOfferedItem(Player *p);
s32 Notice_OpenTextN(char *text, u32 width, s32 mode, s32 sel, s32 arg4);
void PcCard_Open(void);
void Mem_Fill(void *dst, u8 c, s32 n);
void Npc_PlaySeDelayed(Em *npc, void *seId);
s32 Npc_IsPet(Em *arg0);
void PoogieMenu_OpenCare(Em *task);
void HousePet_Open(Em *task);
void PoogieNaming_Open(Em *task);
s32 Area_IsOnlineHouse(s32);
s32 Menu_PushNoticeOnce(s32 arg0);
void PoogieMenu_Open(Em *task);
void PoogieMenu_OpenOutside(Em *task);
void Npc_StartTalkState(Em *p);
void TownMsg_SendSober(Player *pl);
void CamFocus_Start(Em *npc);
void Talk_Start(Em *npc, s8 quiet);
void HairMenu_Open(Em *npc, s32 b);
void MoveMenu_Open(void);
void AreaMenu_Open(u16 id);
s32 LobbyLevel0_HasGroups(void);
void AreaBoard_Open(void);
void GroupBoard_Open(void);
s32 Game_IsOnline(void);
Em *Npc_Find(u8 id);
void Actor_SetState(void *, s32, s32);
void Contest_Open(void *p);
s32 memcmp(void *a, void *b, s32 n);
void TownMsg_SendSelf(u8 len, u8 cmd, void *data);
void TownMsg_SendContestSeatAsk(Player *arg0);
void TownPc_TakeSeat(void);
s32 MyHouse_IsOwner(void);
void TownMsg_SendHouseSeatAsk(Player *a);
s32 LobbyTime_IsAm(void);
void DrinkContest_Open(Em *npc);
s32 ActSpot_IsFree(u32 n);
s32 Area_IsTownHall(s32 arg0);
void TownMsg_SendSeatAsk(Player *arg0);
void PriceInfo_Open(void);
void VillageBoard_Open(void);
s32 PoogieDig_OverLimit(u8);
void HouseAccess_Open(void);
s32 Quest_CheckOrder(u16 id);
s32 ArenaCup_QuestTaken(void);
void *LobbyLevel2_Entry(s32 arg0);
s32 LobbyTime_IsPm(void);
void TownMsg_SendArenaOpen(void);
void LobbyText_Open(s32 msg);
void Menu_SetYesNo(s8 arg0);
void Daytime_Set(s32 a);
void Season_Set(u8 a); /* unit's own declaration: declared with a u8 season, so that TownPc_HandleInput passes it as a byte as the game's code does; the definition takes an s32 */
s32 LobbyWorld_GetSeason(void);
void LobbyWorld_HoldClock(void);
void NetQuest_SetInfo(u8 *);
void NetQuest_SetRareRoll(s32); /* unit's own declaration: declared with an s32, so that TownPc_HandleInput passes the roll unnarrowed as the game's code does; the definition takes an s8 */
s32 Lobby_EnterRollRare(void);
void PspLink_ClearFlags(void);
void FadeA_Start(s32);
void Save_SetEventFlag(s32 n);
void QuestCounter_Open(void);
void LobbySe_Play(s32 se);
void Records_Open(void);
void TownSave_Open(void);
u16 Box_FindFreeSlot(void *unused0);
void CamReq_TownPc(void);
u16 Box_AddItem(SaveData *save, u8 type, u16 id, u16 level);
void CamReq_TownTour(s32 n);
void Npc_ResetTalked(void);
s32 BoxMenu_Open(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void BaitMenu_Open(void);
void *memset(void *, s32, u32);

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s32 gNpcTalkSe[];
extern u8 gTalkNpcName[];
extern u8 gTownActionDelay;
extern s8 gTakeSpotId;
extern s8 gContestSeatId;
extern s16 gSpotModeTimer;
extern s16 gHouseCloseTimer;
extern s8 gTownModeStep;
extern s8 gPoogieDigTaken;
extern u8 gDigSpotUses[];
extern s8 gDigOverLimit;
extern u16 gLobbyQuestId;
extern s8 gQuestMinMembers;
extern u8 gLobbyLevel2Sel;
extern LobbyState *gLobby;
extern LobbyMenu *gLobbyMenu;
extern u8 D_6ADB30[];

/*
 * A town player's input, once a frame. From the stick (TownPc_MoveKind; the
 * other members' players by their sent move) it starts the walk, the dash,
 * or the dash's turn round when the dash points behind it. Then, for the
 * local player only: a requested gesture; an item another player offers
 * (PAD_CIRCLE, no menu open); PAD_CIRCLE on the player or townsperson it
 * faces (Player.target): another player's card, or a talk, which for the
 * Poogies and the hairdresser opens their menus; PAD_CIRCLE at the action
 * spot it stands at (Player.spot): the spot's menu or act by its kind
 * (enum TownSpotKind). Once set out (TOWN_MODE_SET_OUT) only the gate
 * answers. Ends with TownPc_StartRequestedOffer.
 */
void TownPc_HandleInput(Player *pl)
{
    SaveData *save = &gSave;
    u16 pad;
    Em *npc;
    ActionSpot *spot;
    ActionSpot *usedSpot;
    u16 standYaw;
    s32 orderResult;
    u32 turnDelta;
    u8 gatherKind;
    f32 standPos[3];

    pad = LobbyPad_Get(0);
    switch (TownPc_MoveKind(pl)) {
    case TOWN_MOVE_NONE:
        break;
    case TOWN_MOVE_WALK:
    case 2:
    case TOWN_MOVE_WALK_FULL:
        /* not already walking or running (TOWN_PC_WALK to TOWN_PC_RUN_LOOP) */
        if ((u32)(pl->state - TOWN_PC_WALK) > 2) {
            if (pl->index == gScene.localPlayer) {
                pl->targetYaw = TownPc_InputYaw(pl, 0);
            }
            Actor_SetStateFlags(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_WALK, 0);
        }
        break;
    case TOWN_MOVE_DASH:
        if (pl->state != TOWN_PC_TURN_AROUND && pl->state != TOWN_PC_DRUNK_GET_UP && (u32)(pl->state - TOWN_PC_RUN) > 1) {
            if (pl->index == gScene.localPlayer) {
                pl->targetYaw = TownPc_InputYaw(pl, 0);
            }
            turnDelta = (u16)(pl->targetYaw + 0x10000 - pl->rot[1]);
            /* 135 to 225 degrees from the facing: turn round first */
            if (turnDelta >= 0x6000 && turnDelta <= 0xA000) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_TURN_AROUND, 0);
            } else {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_RUN, 0);
            }
        }
        break;
    }
    if (pl->index != gScene.localPlayer) {
        return;
    }
    if (pl->actRequest != 0) {
        TownPc_StartGesture(pl);
        return;
    }
    if (Hud_ItemGetBusy() == 1) {
        return;
    }
    if ((pl->input.pressed & PAD_CIRCLE) && gTownMode == TOWN_MODE_FREE && TownPc_TakeOfferedItem(pl) == 1) {
        return;
    }
    if (pl->target != (u8 *)pl && pl->target != NULL && (pad & PAD_CIRCLE) && gTownMode != TOWN_MODE_GAME_MENU) {
        Notice_OpenTextN(0, 0, 3, 0, 0);
        npc = pl->target;
        if (npc->ownTask == 0 && gTownMode == TOWN_MODE_FREE) {
            /* another player: its card */
            gTownMenuOpen = 0;
            gTownMode = TOWN_MODE_PC_CARD;
            PcCard_Open();
            gLobby->buttons = 0;
        } else if (gTownMode == TOWN_MODE_FREE) {
            NpcWork *npcWork;

            npcWork = (NpcWork *)&npc->work;
            /* no talk while the townsperson is busy */
            if ((s8)npcWork->noTalk == 1 || gTownActionDelay != 0) {
                return;
            }
            gLobby->buttons = 0;
            npc->contact = pl; /* the player talking to it */
            Mem_Fill(gTalkNpcName, 0, 0x34); /* the talk's state: gTalkNpcName up to gTalkMode */
            if (gNpcTalkSe[npcWork->id] != -1) {
                Npc_PlaySeDelayed(npc, (void *)gNpcTalkSe[npcWork->id]);
            }
            if (Npc_IsPet(npc) == 1) {
                if (npcWork->id == NPC_POOGIE_CARE) {
                    PoogieMenu_OpenCare(npc);
                    return;
                } else if (npcWork->id == 0x6D) {
                    HousePet_Open(npc);
                    return;
                } else if (npcWork->id == NPC_POOGIE) {
                    /* not named yet: the naming menu */
                    if (*(s8 *)save->poogieName == 0 && npc->state != 0x58 && npcWork->petAct == 0) {
                        PoogieNaming_Open(npc);
                        return;
                    }
                    if (Area_IsOnlineHouse(gScene.area) == 1 && *(s8 *)save->poogieName != 0) {
                        if (npc->state == NPC_STATE_HOUSE_OPEN) {
                            Menu_PushNoticeOnce(LOBBY_NOTICE_HOUSE_OPEN);
                        } else {
                            PoogieMenu_Open(npc);
                        }
                        return;
                    }
                    if (*(s8 *)save->poogieName != 0 && gScene.area == AREA_HUB_0x89) {
                        PoogieMenu_OpenOutside(npc);
                        return;
                    }
                    return;
                }
            } else if (npc->state != 0x43 && npc->state != 0x44) {
                Npc_StartTalkState(npc);
            }
            if (pl->drunk != 0) {
                pl->drunk = 0;
                pl->drinkCount = 0;
                Actor_ReturnToIdle(pl, 0, -4, 0);
                TownMsg_SendSober(pl);
            }
            gTownMenuOpen = 1;
            gTownMode = TOWN_MODE_TALK;
            Npc_StartTalkState(npc);
            switch (npcWork->id) {
            case NPC_HAIRDRESSER:
                CamFocus_Start(npc);
                Talk_Start(npc, 0);
                HairMenu_Open(npc, 1);
                break;
            }
        }
    }
    spot = pl->spot;
    if (spot != NULL && (pad & PAD_CIRCLE) && (gTownMode == TOWN_MODE_FREE || gTownMode == TOWN_MODE_SET_OUT)) {
        if (gTownActionDelay != 0) {
            return;
        }
        if (gTownMode == TOWN_MODE_SET_OUT && spot->kind != TOWN_SPOT_GATE) {
            return;
        }
        gLobby->buttons = 0;
        standPos[0] = pl->spot->x;
        standPos[1] = pl->spot->y;
        standPos[2] = pl->spot->z;
        usedSpot = pl->spot;
        standYaw = usedSpot->angle;
        switch (usedSpot->kind) {
        default:
            return;
        case TOWN_SPOT_MOVE:
            MoveMenu_Open();
            break;
        case TOWN_SPOT_AREA_DOOR:
            switch (usedSpot->type) {
            case AREA_ONLINE_HOUSE_0:
                /* the house's area by the house's level */
                AreaMenu_Open((u8)Village_FacilityLevel(VILLAGE_FACILITY_HOUSE) + AREA_ONLINE_HOUSE_0);
                break;
            default:
                AreaMenu_Open(usedSpot->type);
                break;
            }
            break;
        case TOWN_SPOT_BOARD:
            if (LobbyLevel0_HasGroups() == 0) {
                AreaBoard_Open();
            } else {
                GroupBoard_Open();
            }
            break;
        case TOWN_SPOT_CONTEST:
            pl->drunk = 0;
            if (Game_IsOnline() == 0) {
                Em *contestHost = Npc_Find(NPC_CONTEST_HOST);

                Actor_ReturnToIdle(pl, 0, -4, 0);
                Actor_SetState(contestHost, 0, 0x6E);
                Contest_Open(contestHost);
            } else {
                /* asks the host for the seat; Town_UpdateMode gives up after the timer */
                gContestSeatId = pl->spot->type;
                gSpotModeTimer = 150; /* frames: 5 s */
                gTownMode = TOWN_MODE_CONTEST_SEAT_WAIT;
                gTownMenuOpen = 1;
                Actor_ReturnToIdle(pl, 0, -4, 0);
                if (memcmp(gLobby->hostId, gLobby->myId, 8) == 0) { /* this player is the host */
                    TownMsg_SendSelf(2, TOWN_MSG_CONTEST_SEAT_ASK, pl->spot);
                } else {
                    TownMsg_SendContestSeatAsk(pl);
                }
            }
            break;
        case TOWN_SPOT_HOUSE_SEAT:
            if ((pl->state == TOWN_PC_IDLE || pl->state == TOWN_PC_DRUNK_IDLE) && gHouseCloseTimer <= 0) {
                /* asks the house's owner for the seat */
                gTakeSpotId = usedSpot->type;
                gSpotModeTimer = 150; /* frames: 5 s */
                gTownMode = TOWN_MODE_SEAT_WAIT;
                gTownMenuOpen = 1;
                if (Game_IsOnline() == 0) {
                    TownPc_TakeSeat();
                } else if (MyHouse_IsOwner() == 1) {
                    TownMsg_SendSelf(2, TOWN_MSG_HOUSE_SEAT_ASK, pl->spot);
                } else {
                    TownMsg_SendHouseSeatAsk(pl);
                }
            }
            break;
        case TOWN_SPOT_SEAT:
            if (Game_IsOnline() == 0 && pl->spot->kind == TOWN_SPOT_SEAT && LobbyTime_IsAm() == 0) {
                gTakeSpotId = pl->spot->type;
                DrinkContest_Open(Npc_Find(NPC_DRINK_CONTEST_HOST));
                break;
            }
            if ((pl->state == TOWN_PC_IDLE || pl->state == TOWN_PC_DRUNK_IDLE) && ActSpot_IsFree(pl->spot->type) == 1) {
                if (Area_IsTownHall(gScene.area) == 1) {
                    /* online: the seat is asked of the host */
                    gTakeSpotId = pl->spot->type;
                    if (memcmp(gLobby->hostId, gLobby->myId, 8) == 0) { /* this player is the host */
                        gSpotModeTimer = 150; /* frames: 5 s */
                        gTownMode = TOWN_MODE_SEAT_WAIT;
                        gTownMenuOpen = 1;
                        TownMsg_SendSelf(2, TOWN_MSG_SEAT_ASK, pl->spot);
                    } else {
                        gSpotModeTimer = 150; /* frames: 5 s */
                        gTownMode = TOWN_MODE_SEAT_WAIT;
                        gTownMenuOpen = 1;
                        TownMsg_SendSeatAsk(pl);
                    }
                } else {
                    gTakeSpotId = pl->spot->type;
                    TownPc_TakeSeat();
                }
            }
            return;
        case TOWN_SPOT_PRICE_BOARD:
            gTownMode = TOWN_MODE_PRICE_INFO;
            gTownMenuOpen = 1;
            PriceInfo_Open();
            return;
        case TOWN_SPOT_VILLAGE_BOARD:
            VillageBoard_Open();
            break;
        case TOWN_SPOT_POOGIE_DIG:
            gPoogieDigTaken = 1;
            Actor_SetStateNow(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_POOGIE_DIG, 0);
            gDigSpotUses[pl->spot->type]++;
            /* the area's gather kind: 0 in AREA_HUB_0x89, 1 in AREA_HUB_0x83 */
            if (gScene.area == AREA_HUB_0x89) {
                gatherKind = 0;
            } else if (gScene.area == AREA_HUB_0x83) {
                gatherKind = 1;
            } else {
                return;
            }
            if (PoogieDig_OverLimit(gatherKind) == 1) {
                gDigOverLimit = 1;
            }
            return;
        case TOWN_SPOT_MEAL:
            if (gLobby->mealEaten == 0) {
                gTakeSpotId = usedSpot->type;
                TownPc_TakeSeat();
            }
            return;
        case TOWN_SPOT_HOUSE_ACCESS:
            HouseAccess_Open();
            break;
        case TOWN_SPOT_GATE:
            switch (gScene.area) {
            case AREA_TAVERN:
            case AREA_TOWN_ARENA:
            case AREA_ELDER_HALL:
                if (gLobby->questTaken != 0) {
                    orderResult = Quest_CheckOrder(gLobbyQuestId);
                    if (gLobby->questPaid == 1) {
                        if (ArenaCup_QuestTaken() == 1) {
                            LobbyLevel2_Entry((s16)(gLobbyLevel2Sel - 1));
                            if (gTownMode != TOWN_MODE_QUEST_PARTY) {
                                if (LobbyTime_IsPm() == 0) {
                                    Menu_PushNoticeOnce(LOBBY_NOTICE_ARENA_CUP_HOURS);
                                } else if (gLobby->roomMembers == gQuestMinMembers) {
                                    TownMsg_SendArenaOpen();
                                } else if (gTownMode != TOWN_MODE_SET_OUT) {
                                    Menu_PushNoticeOnce(LOBBY_NOTICE_ARENA_CUP_MEMBERS);
                                }
                            }
                        } else if (orderResult == QUEST_ORDER_OK) {
                            if (gTownMode != TOWN_MODE_SET_OUT_PAID && gTownMode != TOWN_MODE_SET_OUT) {
                                LevelEntry *room = LobbyLevel2_Entry((s16)(gLobbyLevel2Sel - 1));

                                if (gLobby->roomMembers + 1 >= room->users) {
                                    LobbyText_Open(LOBBY_TEXT_SET_OUT_FULL_ASK);
                                    Menu_SetYesNo(0);
                                    gFieldControl = 0;
                                    gTownMode = TOWN_MODE_SET_OUT_ASK_FULL;
                                    gTownMenuOpen = 1;
                                } else {
                                    LobbyText_Open(LOBBY_TEXT_SET_OUT_SHORT_ASK);
                                    Menu_SetYesNo(1);
                                    gFieldControl = 0;
                                    gTownMode = TOWN_MODE_SET_OUT_ASK;
                                    gTownMenuOpen = 1;
                                }
                            }
                        } else {
                            if (orderResult == QUEST_ORDER_WRONG_TIME) {
                                LobbyText_Open(LOBBY_TEXT_GATE_CLOSED);
                            } else {
                                LobbyText_Open(LOBBY_TEXT_GATE_ORDER_TERMS);
                            }
                            gFieldControl = 0;
                            gTownMode = TOWN_MODE_GATE_MESSAGE_WAIT;
                            gTownMenuOpen = 1;
                        }
                    } else if (gTownMode == TOWN_MODE_SET_OUT) {
                        gTownMenuOpen = 1;
                        gTownMode = TOWN_MODE_SET_OUT_CANCEL_ASK;
                        gTownModeStep = 0;
                        gFieldControl = 0;
                        LobbyText_Open(LOBBY_TEXT_SET_OUT_CANCEL_ASK);
                        Menu_SetYesNo(1);
                    } else if (orderResult == QUEST_ORDER_TERMS) {
                        LobbyText_Open(LOBBY_TEXT_GATE_ORDER_TERMS);
                        gFieldControl = 0;
                        gTownMode = TOWN_MODE_GATE_MESSAGE_WAIT;
                        gTownMenuOpen = 1;
                        break;
                    } else {
                        gTownMode = TOWN_MODE_SET_OUT_JOIN;
                        LobbyText_Open(LOBBY_TEXT_SENDING);
                    }
                } else {
                    LobbyText_Open(LOBBY_TEXT_GATE_CLOSED);
                    gFieldControl = 0;
                    gTownMode = TOWN_MODE_SET_OUT_MESSAGE_WAIT;
                    gTownMenuOpen = 1;
                }
                return;
            case AREA_HUB_0x89:
            case AREA_HUB_0x57:
                if (gLobby->questTaken != 0) {
                    if (Quest_CheckOrder(gLobbyQuestId) == QUEST_ORDER_OK) {
                        /* the lists as bytes, taken before the time of day is
                         * set: the game's code loads the address there */
                        u8 *questLists = (u8 *)&gTownQuestLists;

                        /* the village's quest: sets out at once, with the town's
                         * time of day and season, and this season's wanted list
                         * as its hunt goals */
                        gTownMode = TOWN_MODE_DEPART;
                        if (LobbyTime_IsAm() == 0) {
                            Daytime_Set(DAYTIME_SET_NIGHT);
                        } else {
                            Daytime_Set(DAYTIME_SET_DAY);
                        }
                        Season_Set(LobbyWorld_GetSeason());
                        LobbyWorld_HoldClock();
                        if (gQuestOfferId != 0) {
                            memset(gNextQuest.huntGoals, 0, sizeof(gNextQuest.huntGoals));
                        } else {
                            NetQuest_SetInfo(questLists + LobbyWorld_GetSeason() * 0x18 + 4); /* gTownQuestLists.a[season] */
                        }
                        NetQuest_SetRareRoll(Lobby_EnterRollRare());
                        PspLink_ClearFlags();
                        FadeA_Start(1);
                        if (gLobbyQuestId == 0x1771 && Save_TestEventFlag(0x30) == 0) {
                            Save_SetEventFlag(0x30);
                        }
                    } else {
                        LobbyText_Open(LOBBY_TEXT_GATE_CLOSED);
                        gFieldControl = 0;
                        gTownMode = TOWN_MODE_GATE_MESSAGE_WAIT;
                        gTownMenuOpen = 1;
                    }
                }
                break;
            }
            return;
        case TOWN_SPOT_QUEST_COUNTER:
            if (gSave.hr != 0) {
                QuestCounter_Open();
            } else {
                Menu_PushNoticeOnce(LOBBY_NOTICE_NO_HUNTER_RANK);
            }
            return;
        case TOWN_SPOT_EXIT:
            gTownMode = TOWN_MODE_EXIT_ASK;
            LobbyText_Open(LOBBY_TEXT_EXIT_ASK);
            Menu_SetYesNo(1);
            LobbySe_Play(LOBBY_SE_CONFIRM);
            gLobbyMenu->redraw = 1;
            gTownMenuOpen = 1;
            gFieldControl = 0;
            break;
        case TOWN_SPOT_RECORDS:
            Records_Open();
            break;
        case TOWN_SPOT_SAVE:
            TownSave_Open();
            break;
        case TOWN_SPOT_BED:
            /* outside AREA_HUB_0x57 the village bed's own point and facing */
            if (gScene.area != AREA_HUB_0x57) {
                standPos[0] = 5075.0f;
                standPos[1] = 201.19f;
                standPos[2] = 6438.0f;
                standYaw = 0x72A2;
            }
            Vec_Copy(pl->moveTo, standPos);
            pl->targetYaw = standYaw;
            TownPc_StartMoveTo(pl, 20 /* frames */);
            if (Town_SleepEventPending() == 1) {
                /* the sleep event: its blade goes in the box */
                if ((u16)Box_FindFreeSlot(&gSave) == BOX_NONE) {
                    Menu_PushNoticeOnce(LOBBY_NOTICE_BOX_FULL);
                } else {
                    Actor_SetStateNow(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_REST_LONG, 0);
                    if (gScene.area == AREA_HUB_0x57) {
                        CamReq_TownPc();
                        gFieldControl = 0;
                        Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
                        Save_SetEventFlag(EVENT_FLAG_HOUSE_SLEEP_EVENT);
                        Box_AddItem(&gSave, EQUIP_BLADE, 0xBE, 0);
                    } else {
                        CamReq_TownTour(0xA);
                        gFieldControl = 0;
                        Save_SetEventFlag(EVENT_FLAG_MINE_STRONG_PICK);
                        Box_AddItem(&gSave, EQUIP_BLADE, 0xA3, 0);
                    }
                    Npc_ResetTalked();
                    gTownMode = TOWN_MODE_SLEEP;
                }
            } else {
                Actor_SetStateNow(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_REST, 0);
                gFieldControl = 0;
                if (gScene.area == AREA_HUB_0x57) {
                    Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
                } else {
                    CamReq_TownTour(0xB);
                }
                gTownMode = TOWN_MODE_SLEEP;
            }
            break;
        case TOWN_SPOT_BOX:
            if (BoxMenu_Open() == 1) {
                gTownMenuOpen = 1;
                gTownMode = TOWN_MODE_BOX_OPEN;
                LobbyMenu_SetDraw(0, 0, 0, 0);
            } else {
                LobbySe_Play(LOBBY_SE_REFUSE);
            }
            break;
        case TOWN_SPOT_FISHING:
            pl->drunk = 0;
            if (Game_IsOnline() == 0) {
                Actor_ReturnToIdle(pl, 0, -4, 0);
            }
            BaitMenu_Open();
            break;
        }
    }
    TownPc_StartRequestedOffer(pl);
}

f32 Math_Atan2(f32 y, f32 x);
f32 Vec_Distance(f32 *a, f32 *b);
void Actor_SetState(void *, s32, s32);
extern u16 gTownMoveFrames;

/*
 * Steers another member's player toward the destination its messages sent
 * (Player.moveTo), unless it is crouched, with the motion's root movement
 * off. From idle it runs (beyond 150 units) or walks (beyond 20); a run
 * slows to a walk when a new destination arrives within 150 units and
 * stops on reaching it; a walk goes idle on reaching it, or runs when the
 * destination is far. The offer, bed, contest and thrown states are left
 * alone.
 */
void TownPc_SteerToDest(Player *pl)
{
    f32 dist;
    f32 dx;
    f32 dz;
    u8 state;

    if (pl->action != PL_POSTURE_CROUCHED) {
        pl->moveTo[1] = pl->pos[1];
        dist = Vec_Distance(pl->pos, pl->moveTo);
        state = pl->state;
        dx = pl->moveTo[0] - pl->pos[0];
        dz = pl->moveTo[2] - pl->pos[2];
        switch (state) {
        case TOWN_PC_OFFER_ITEM:
        case TOWN_PC_TAKE_ITEM_WAIT:
        case TOWN_PC_OFFER_ITEM_END:
        case TOWN_PC_TAKE_ITEM:
        case TOWN_PC_CONTEST_SEAT_A:
        case TOWN_PC_CONTEST_SEAT_B:
        case TOWN_PC_CONTEST_LOSE:
        case TOWN_PC_BED_LIE_DOWN:
        case TOWN_PC_THROWN:
            break;
        default:
        case TOWN_PC_IDLE:
            if (!(dist <= 150.0f)) {
                Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_RUN);
                pl->targetYaw = (s16) (s32) (0.5f + ((65536.0f * Math_Atan2(dx, dz)) / 6.2831855f));
            } else if (!(dist <= 20.0f)) {
                pl->targetYaw = (s16) (s32) (0.5f + ((65536.0f * Math_Atan2(dx, dz)) / 6.2831855f));
                Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_WALK);
            }
            break;
        case TOWN_PC_RUN:
            if ((pl->moveToFrames == (gTownMoveFrames - 1)) && (dist < 150.0f)) {
                Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_WALK);
            } else if (dist < 1.0f) {
                pl->moveToFrames = 0;
                Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_RUN_STOP);
            } else {
                pl->targetYaw = (s16) (s32) (0.5f + ((65536.0f * Math_Atan2(dx, dz)) / 6.2831855f));
            }
            break;
        case TOWN_PC_WALK:
            if (dist < 1.0f) {
                pl->moveToFrames = 0;
                Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_IDLE);
                Actor_ReturnToIdle(pl, 0, 4, 0);
            } else if (!(dist <= 150.0f)) {
                pl->targetYaw = (s16) (s32) (0.5f + ((65536.0f * Math_Atan2(dx, dz)) / 6.2831855f));
                Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_RUN);
            } else {
                pl->targetYaw = (s16) (s32) (0.5f + ((65536.0f * Math_Atan2(dx, dz)) / 6.2831855f));
            }
            break;
        }
        Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
    }
}

/* Turns the player with the d-pad's left and right, 0x400 a frame, and
 * keeps its target yaw with it. */
static void TownPc_TurnByPad(Player *pl)
{
    if (gPadState[0].dir & PAD_LEFT) {
        pl->rot[1] = (s32) (pl->rot[1] - 0x400);
        pl->targetYaw = (s16) pl->rot[1];
    }
    if (gPadState[0].dir & PAD_RIGHT) {
        pl->rot[1] = (s32) (pl->rot[1] + 0x400);
        pl->targetYaw = (s16) pl->rot[1];
    }
}

extern s32 gTownMode;

/* The local player's input: while the box menu is open the d-pad turns
 * the player; otherwise TownPc_HandleInput. */
void TownPc_CheckAction(Player *pl)
{
    if (pl->index == gScene.localPlayer) {
        if (gTownMode == TOWN_MODE_BOX_MENU) {
            TownPc_TurnByPad(pl);
            return;
        }
        TownPc_HandleInput(pl);
    }
}

/*
 * The gesture PL_GESTURE_LIE_DOWN: lies down on its back. The local player
 * gets up with the stick (PL_GESTURE_GET_UP) or starts a newly requested
 * gesture. The town copy of Player_StateGestureLieDown, as are the others.
 */
void TownPc_GestureLieDown(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, PL_MOT_LIE_FACE_UP, 4, 0);
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (Player_IsLocal(pl) == 1) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateNow(pl, TOWN_PC_GROUP_GESTURE, PL_GESTURE_GET_UP, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_GET_UP: gets up from lying on its back, then idle.
 */
void TownPc_GestureGetUp(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, PL_MOT_GET_UP_FACE_UP, 0, 0);
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Actor_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_GESTURE_SCOUT: the scouting gesture, held at least 60 frames; then the
 * local player leaves it with the stick (TOWN_GESTURE_END) or starts a newly
 * requested gesture. The field's flyer check is not made in the town.
 */
void TownPc_GestureScout(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, PL_MOT_GESTURE_SCOUT, 6, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_03: the looped sitting gesture; from its frame 90 the local
 * player ends it with the stick (PL_GESTURE_03_END) or starts a newly
 * requested gesture.
 */
void TownPc_Gesture03(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, PL_MOT_GESTURE_SIT, 6, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if ((Player_IsLocal(pl) == 1) && (Motion_ReachedFrame(pl, 0, 90.0f) != 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateNow(pl, TOWN_PC_GROUP_GESTURE, PL_GESTURE_03_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_03_END: the sitting gesture's end, then the get-up motion with
 * the root movement off, then idle.
 */
void TownPc_Gesture03End(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, PL_MOT_GESTURE_SIT_END, 6, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step = (u8) (step + 1);
            Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
            Actor_SetMotion(pl, PL_MOT_GET_UP_FACE_UP, -4, 0x3C);
        }
        break;
    case 2:
        Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
        if (pl->motion[0].playing == 0) {
            Player_ClearFlag(pl, PLF_NO_ROOT_MOVE);
            Actor_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_GESTURE_05: held at least 60 frames, then left with the stick
 * (TOWN_GESTURE_END) or for a newly requested gesture. The field's version
 * also makes attack objects; this one doesn't.
 */
void TownPc_Gesture05(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x274, 6, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_06, and PL_GESTURE_06_ALT: held at least 60 frames, then left
 * with the stick (TOWN_GESTURE_END) or for a newly requested gesture.
 */
void TownPc_Gesture06(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x278, 4, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_07: as TownPc_Gesture06, with its own motion.
 */
void TownPc_Gesture07(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x26E, 4, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_08: as TownPc_Gesture06, with its own motion.
 */
void TownPc_Gesture08(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x273, 6, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * The one-shot gestures (PL_GESTURE_ONESHOT0, 1 and 3): row variant of
 * gTownGestureOneShots gives the motion, then the blend back to idle.
 */
void TownPc_GestureOneShot(Player *pl, s32 variant)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, gTownGestureOneShots[variant * 2], 4, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Actor_ReturnToIdle(pl, 0, gTownGestureOneShots[variant * 2 + 1], 0);
        }
        break;
    }
}

/*
 * PL_GESTURE_0A: as TownPc_Gesture06, with its own motion.
 */
void TownPc_Gesture0A(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x271, 4, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_0B: as TownPc_Gesture06, with its own motion and a longer
 * blend in.
 */
void TownPc_Gesture0B(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x276, 0x10, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_0C: as TownPc_Gesture06, with its own motion.
 */
void TownPc_Gesture0C(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x272, 6, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_10: as TownPc_Gesture06, with its own motion.
 */
void TownPc_Gesture10(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x279, 6, 0);
        pl->motionTime = 0;
        Actor_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60; /* frames: 1 s */
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= PL_STICK_HELD) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_GESTURE, TOWN_GESTURE_END, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * TOWN_GESTURE_END: a held gesture's end, back to idle.
 */
void TownPc_GestureEnd(Player *pl, s32 unused)
{
    Actor_ReturnToIdle(pl, 0, 0x4, 0);
}

void Debug_Message(char *msg, s32 a, s32 b, s32 c);

/*
 * Runs the player's gesture state (group TOWN_PC_GROUP_GESTURE): one
 * function per gesture, the argument picking a one-shot gesture's row; an
 * unknown state prints a debug message.
 */
void TownPc_RunGestureState(Player *pl)
{
    switch (pl->state) {
    case PL_GESTURE_LIE_DOWN:
        TownPc_GestureLieDown(pl, 0);
        break;
    case PL_GESTURE_GET_UP:
        TownPc_GestureGetUp(pl, 0);
        break;
    case PL_GESTURE_SCOUT:
        TownPc_GestureScout(pl, 0);
        break;
    case PL_GESTURE_03:
        TownPc_Gesture03(pl, 0);
        break;
    case PL_GESTURE_03_END:
        TownPc_Gesture03End(pl, 0);
        break;
    case PL_GESTURE_05:
        TownPc_Gesture05(pl, 0);
        break;
    case PL_GESTURE_06:
        TownPc_Gesture06(pl, 0);
        break;
    case PL_GESTURE_07:
        TownPc_Gesture07(pl, 0);
        break;
    case PL_GESTURE_08:
        TownPc_Gesture08(pl, 0);
        break;
    case PL_GESTURE_ONESHOT0:
        TownPc_GestureOneShot(pl, 0);
        break;
    case PL_GESTURE_0A:
        TownPc_Gesture0A(pl, 0);
        break;
    case PL_GESTURE_0B:
        TownPc_Gesture0B(pl, 0);
        break;
    case PL_GESTURE_0C:
        TownPc_Gesture0C(pl, 0);
        break;
    case PL_GESTURE_ONESHOT1:
        TownPc_GestureOneShot(pl, 1);
        break;
    case PL_GESTURE_06_ALT:
        TownPc_Gesture06(pl, 2);
        break;
    case PL_GESTURE_ONESHOT3:
        TownPc_GestureOneShot(pl, 3);
        break;
    case PL_GESTURE_10:
        TownPc_Gesture10(pl, 0);
        break;
    case TOWN_GESTURE_END:
        TownPc_GestureEnd(pl, 0);
        break;
    default:
        Debug_Message("NO PL ACTION", pl->stateGroup, pl->state, pl->prevState);
        break;
    }
}

/* The unit's data: the one-shot gestures' (motion, blend) rows, then the
 * forge's recipe and upgrade tables. */
s16 gTownGestureOneShots[8] = {
#include "lobby/00802C60.inc"
};
u32 gArmorRecipes[2276] = {
#include "lobby/00802C70.inc"
};
u32 D_00805000[832] = {
#include "lobby/00805000.inc"
};
u32 gWeaponRecipes[1216] = {
#include "lobby/00805D00.inc"
};
u32 D_00807000[208] = {
#include "lobby/00807000.inc"
};
u32 gBladeUpgrades[848] = {
#include "lobby/00807340.inc"
};
u32 D_00808080[2068] = {
#include "lobby/00808080.inc"
};
u32 gGunUpgrades[768] = {
#include "lobby/0080A0D0.inc"
};
