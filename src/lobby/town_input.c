/*
 * The local town player's input.
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
 * Before it, four helpers of its cases: the townsperson's delayed talk
 * sound (Npc_PlaySeDelayed), the start of an offer the pouch asked for
 * (TownPc_StartRequestedOffer, also run by the idle and move states), the
 * bed menu's walk to the bed (TownPc_WalkToBed, as the bed spot's case
 * does it) and the bed's sleep-event test (Town_SleepEventPending).
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "pad.h"
#include "lobby_globals.h"
#include "area.h"
#include "quest.h"
#include "lobby_text.h"
#include "lobby_npc.h"
#include "box_menu.h"
#include "camera.h"
#include "hud.h"
#include "lobby_group_board.h"
#include "village_cond.h"
#include "town_input.h"
#include "area_board.h"
#include "area_menu.h"
#include "arena_cup.h"
#include "arm_wrestling.h"
#include "bait_menu.h"
#include "bed_menu.h"
#include "drink_contest.h"
#include "engine_math.h"
#include "game_util.h"
#include "hair_menu.h"
#include "house_access.h"
#include "hunter_card.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_queries.h"
#include "lobby_util.h"
#include "lobby_world.h"
#include "market.h"
#include "mem_util.h"
#include "menu_steps.h"
#include "move_menu.h"
#include "my_house.h"
#include "pets.h"
#include "quest_counter.h"
#include "quest_offers.h"
#include "records_lists.h"
#include "talk.h"
#include "town_gesture_drink.h"
#include "town_player_states.h"
#include "town_players.h"
#include "village_board.h"
#include "screen_fade.h"
#include "libc.h"
#include "vec_math.h"

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
        Actor_SetStateNow(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_OFFER_ITEM, 16);
        TownMsg_SendOfferAll(pl);
    }
}

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

u8 Village_FacilityLevel(s32 k);

/*
 * Returns 1 when the bed's sleep event of the current area is due: in
 * AREA_HUB_0xAC once the mine is at level 6 and Lao-Shan Lung has been
 * hunted, in AREA_HUB_0x57 once condition 81 holds; each only while its
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
        if (Save_TestEventFlag(EVENT_FLAG_HOUSE_SLEEP_EVENT) == 0 && Cond_Check(81U) == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Notice_OpenTextN(char *text, u32 width, s32 mode, s32 sel, s32 arg4);
void Daytime_Set(s32 a);
void Season_Set(u8 a); /* unit's own declaration: declared with a u8 season, so that TownPc_HandleInput passes it as a byte as the game's code does; the definition takes an s32 */
s32 LobbyWorld_GetSeason(void);
void NextQuest_SetFlyer(s32); /* unit's own declaration: declared with an s32, so that TownPc_HandleInput passes the roll unnarrowed as the game's code does; the definition takes an s8 */
s32 Lobby_EnterRollRare(void);
u16 Box_AddItem(SaveData *save, u8 type, u16 id, u16 level);

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
            turnDelta = (u16)(pl->targetYaw + ANGLE_TURN - pl->rot[1]);
            /* 135 to 225 degrees from the facing: turn round first */
            if (turnDelta >= 24576 && turnDelta <= 0xA000) {
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
            Mem_Fill(gTalkNpcName, 0, 52); /* the talk's state: gTalkNpcName up to gTalkMode */
            if (gNpcTalkSe[npcWork->id] != -1) {
                Npc_PlaySeDelayed(npc, (void *)gNpcTalkSe[npcWork->id]);
            }
            if (Npc_IsPet(npc) == 1) {
                if (npcWork->id == NPC_POOGIE_CARE) {
                    PoogieMenu_OpenCare(npc);
                    return;
                } else if (npcWork->id == 109) {
                    HousePet_Open(npc);
                    return;
                } else if (npcWork->id == NPC_POOGIE) {
                    /* not named yet: the naming menu */
                    if (*(s8 *)save->poogieName == 0 && npc->state != 88 && npcWork->petAct == 0) {
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
            } else if (npc->state != NPC_STATE_PAIR_STAND && npc->state != NPC_STATE_PAIR_STAND_1) {
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
                Actor_SetState(contestHost, 0, 110);
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
            /* the area's gather kind: 0 in AREA_HUB_0x89, 1 in AREA_ONLINE_VILLAGE */
            if (gScene.area == AREA_HUB_0x89) {
                gatherKind = 0;
            } else if (gScene.area == AREA_ONLINE_VILLAGE) {
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
                            NextQuest_SetHuntGoals(questLists + LobbyWorld_GetSeason() * 24 + 4); /* gTownQuestLists.a[season] */
                        }
                        NextQuest_SetFlyer(Lobby_EnterRollRare());
                        PspLink_ClearFlags();
                        FadeA_Start(1);
                        if (gLobbyQuestId == 6001 && Save_TestEventFlag(48) == 0) {
                            Save_SetEventFlag(48);
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
                        Box_AddItem(&gSave, EQUIP_BLADE, 190, 0);
                    } else {
                        CamReq_TownTour(10);
                        gFieldControl = 0;
                        Save_SetEventFlag(EVENT_FLAG_MINE_STRONG_PICK);
                        Box_AddItem(&gSave, EQUIP_BLADE, 163, 0);
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
                    CamReq_TownTour(11);
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
