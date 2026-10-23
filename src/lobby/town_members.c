/*
 * The town's players as the lobby's members come and go. Each member of
 * the town (gNetUsers, LOBBY_MEMBER_COUNT of them) has a player in
 * gPlayers at the same index, and the town keeps its own copy of each
 * member's NetUser in LobbyState.members.
 *
 * - TownPc_SyncMembers, each frame on the lobby server: drops the players
 *   whose members left (TownPc_RemoveLeft), applies the hunter cards that
 *   changed (TownPc_RefreshChanged), spawns the players of the members who
 *   joined (TownPc_AddJoined; a seated local player tells them its seat
 *   again), and starts loading the new players' models.
 * - The players' models: TownPc_LoadAllModels on entering an area (the
 *   common motions, the monster slots' models, the players'),
 *   TownPc_LoadModel and TownPc_StartLoad, TownPc_Free and its two short
 *   forms; TownPc_IsLoaded tells when a player is ready. The per-player
 *   flags are LobbyState's modelLoading, modelLoaded, pcLoaded and
 *   lookPending.
 * - TownPc_Spawn and TownPc_InitAll set players up with their draw slots.
 */
#include "common.h"
#include "item.h"
#include "scene.h"
#include "lobby_menu.h"
#include "em.h"
#include "sprite.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "objhead.h"
#include "capsule.h"
#include "lobby_net.h"
#include "stream.h"
#include "town_members.h"
#include "draw_slots.h"
#include "frame.h"
#include "game_util.h"
#include "hunter_card.h"
#include "lobby_queries.h"
#include "lock_on_mark.h"
#include "mem_util.h"
#include "my_house.h"
#include "town_player_activities.h"
#include "town_player_states.h"
#include "town_players.h"
#include "libc.h"

extern NetUser gNetUsers[];
void Player_FreeWeaponModel(Player *p);
void Player_FreeModels(Player *p);
extern u8 gTownSyncWait;
extern u8 gTownAreaMsgPending;
extern u8 gPlayerHandlerTable[];
extern s32 gTownMode;
void Player_LoadPartModels(Player *p);
void Player_FreeWeaponModelIfChanged(Player *p);
void Player_FreeChangedPartModels(Player *p);

/* Whether player pl is ready to be shown: its models loaded and not
 * loading and, while it is active, its look applied (lookPending clear)
 * and its model in place (pcLoaded). 1 if so. */
s32 TownPc_IsLoaded(s8 pl)
{
    u8 active;

    if ((s8)gLobby->modelLoading[pl] == 1) {
        return 0;
    }
    if (gLobby->modelLoaded[pl] == 0) {
        return 0;
    }
    active = gPlayers[pl].active;
    if (active != 0 && (s8)gLobby->lookPending[pl] != 0) {
        return 0;
    }
    if (active != 0 && (s8)gLobby->pcLoaded[pl] == 0) {
        return 0;
    }
    return 1;
}

/* Drops the players whose members left: those whose copy in
 * LobbyState.members holds an id that gNetUsers no longer has at that
 * index, and whose look isn't waiting to be applied. Each is freed (its
 * models and draw slot), cleared with its copy, and its loading flags
 * cleared. Returns 1 if any was dropped. off is the copy's byte offset
 * (LOBBY_MEMBER_AT). */
s32 TownPc_RemoveLeft(void)
{
    s8 i;
    s32 found = 0;
    s32 off;
    NetUser *member;
    Player *pl;
    s32 j;

    for (i = 0, off = 0, member = gNetUsers, pl = gPlayers; i < LOBBY_MEMBER_COUNT; i++, off += sizeof(NetUser), member++, pl++) {
        if (memcmp(member->id, LOBBY_MEMBER_AT(off).id, LOBBY_USER_ID_LEN) != 0
            && *(s8 *)LOBBY_MEMBER_AT(off).id != 0
            && Lobby_IsLookPending(pl->index) == 0) {
            j = i;
            found = 1;
            gLobby->modelLoaded[j] = 0;
            gLobby->modelLoading[j] = 0;
            pl->visible = 0;
            pl->active = 0;
            Player_FreeWeaponModel(pl);
            Player_FreeModels(pl);
            if (pl->drawSlot != NULL) {
                DrawSlot_Free(pl->drawSlotId);
                pl->drawSlot = NULL;
            }
            memset(pl, 0, sizeof(Player));
            memset(&LOBBY_MEMBER_AT(off), 0, sizeof(NetUser));
            gLobby->pcLoaded[j] = 0;
            gLobby->lookPending[j] = 0;
        }
    }
    return found;
}

/* Applies the hunter cards that changed: for each active player whose look
 * isn't pending and whose member's text in gNetUsers differs from the
 * town's copy (LobbyState.members), the new card is recorded
 * (LobbyMember_SetRecord); then the skills are worked out again, the sync
 * question goes to it (TownMsg_SendSyncAskTo, TOWN_MSG_SYNC_ASK) and, if its
 * look is now pending, the look is marked changed (lookPending 2).
 * Returns 1 if any changed. */
s32 TownPc_RefreshChanged(void)
{
    s32 found = 0;
    s8 i;

    for (i = 0; i < LOBBY_MEMBER_COUNT; i++) {
        if (Lobby_IsLookPending(gPlayers[i].index) == 0 && gPlayers[i].active != 0
            && memcmp(gNetUsers[i].text, gLobby->members[i].text, sizeof(gNetUsers[i].text)) != 0
            && LobbyMember_SetRecord(i, &gNetUsers[i], gNetUsers[i].text) == 1) {
            Player_CalcSkills(&gPlayers[i]);
            TownMsg_SendSyncAskTo(i);
            gPlayers[i].netActive = 1;
            if (Lobby_IsLookPending(gPlayers[i].index) != 0) {
                gLobby->lookPending[gPlayers[i].index] = 2;
            }
            found = 1;
        }
    }
    return found;
}

/* Online, spawns the players of the members who joined: a member of
 * gNetUsers whose copy in LobbyState.members is empty (and whose look
 * isn't pending) is copied there (id, name and text), its hunter card
 * applied and its player spawned. A seated local player then tells the
 * members its seat again: its table seat in AREA_TAVERN, AREA_TOWN_ARENA
 * or AREA_ELDER_HALL (TOWN_MSG_SEAT_TAKEN), its seat in an online house
 * (TOWN_MSG_HOUSE_SEAT_TAKEN), or its contest seat
 * (TOWN_MSG_CONTEST_SEAT_TAKEN). off is the copy's byte offset
 * (LOBBY_MEMBER_AT); member and idSource walk gNetUsers together. */
void TownPc_AddJoined(void)
{
    s8 i;
    Player *localPl = &gPlayers[gScene.localPlayer];
    NetUser *member;
    s32 off;
    NetUser *idSource;

    if (Game_IsOnline() != 0) {
        for (i = 0, member = gNetUsers, off = 0, idSource = gNetUsers; i < LOBBY_MEMBER_COUNT; i++, member++, off += sizeof(NetUser), idSource++) {
            if ((s8)gLobby->lookPending[i] == 0 && member->id[0] != 0) {
                LobbyState *base = (LobbyState *)((u8 *)gLobby + off);
                if (*(s8 *)base->members[0].id == 0) {
                    strcpy(base->members[0].id, idSource->id);
                    strcpy(LOBBY_MEMBER_AT(off).name, member->name);
                    memcpy(LOBBY_MEMBER_AT(off).text, member->text, sizeof(member->text));
                    HunterCard_Apply(i, member->text);
                    TownPc_Spawn(i, member, member->name);
                    if (Lobby_IsLookPending(localPl->index) != 0) {
                        gLobby->lookPending[localPl->index] = 2;
                    }
                    if (TownPc_IsSeatedState(localPl->stateGroup, localPl->state) == 1) {
                        if (Area_IsTownHall(gScene.area) == 1) {
                            TownMsg_SendSeatTaken();
                        } else if (Area_IsOnlineHouse(gScene.area) == 1) {
                            TownMsg_SendHouseSeatTaken();
                        }
                    } else if (TownPc_IsInContest((Player *)localPl) == 1) {
                        TownMsg_SendContestSeatTaken();
                    }
                }
            }
        }
    }
}

/* Each frame while on the lobby server: drops the players whose members
 * left, or else applies the cards that changed; either, when it changed
 * anything in an area with town players, compacts the memory and sets
 * gTownSyncWait to 3, and the frame ends there, as it does while
 * gTownSyncWait is set. Then spawns the members who joined, starts
 * loading the models of the active players not loaded yet (an online
 * house's at once), and sends the sync question when one is pending
 * (gTownAreaMsgPending). */
void TownPc_SyncMembers(void)
{
    s8 i;
    Player *pl;

    if (gLobby->connected == 0) {
        return;
    }
    if (TownPc_RemoveLeft() == 1) {
        if (Area_HasTownPcs(gScene.area) == 1) {
            Mem_Compact();
            gTownSyncWait = 3;
        }
        return;
    }
    if (gTownSyncWait != 0) {
        return;
    }
    if (TownPc_RefreshChanged() == 1) {
        if (Area_HasTownPcs(gScene.area) == 1) {
            Mem_Compact();
            gTownSyncWait = 3;
        }
        return;
    }
    if (gTownSyncWait != 0) {
        return;
    }
    TownPc_AddJoined();
    if (Area_HasTownPcs(gScene.area) == 1) {
        for (i = 0, pl = gPlayers; i < LOBBY_MEMBER_COUNT; i++, pl++) {
            if (pl->active != 0 && (s8)gLobby->pcLoaded[i] == 0 && Lobby_IsLookPending(pl->index) == 0) {
                if (Area_IsOnlineHouse(pl->area) == 1) {
                    TownPc_LoadModelNow(pl);
                } else {
                    TownPc_StartLoad(pl);
                }
                Lobby_ReceiveWhileWaiting();
            }
        }
    }
    if (gTownAreaMsgPending == 1) {
        TownMsg_SendSyncAsk();
        gTownAreaMsgPending = 0;
    }
}

/* Loads what the town's players need: the common motions once per area
 * (set 2 online, 3 away from the village, else 1), the monster slots'
 * models of the online town or of the village, and the models of the
 * active players not loaded yet. */
void TownPc_LoadAllModels(void)
{
    u32 i;
    Player *pl;

    if (gLobby->areaLoaded == 0) {
        gLobby->areaLoaded = 1;
        if (Game_IsOnline() == 1) {
            Anim_LoadCommon(2);
        } else if (gSave.awayFromVillage != 0) {
            Anim_LoadCommon(3);
        } else {
            Anim_LoadCommon(1);
        }
    }
    if (Game_IsOnline() == 0) {
        EmSlot_LoadModel(11, 0, 2);
        EmSlot_LoadModel(12, 0, 3);
    } else {
        EmSlot_LoadModel(4, 0, 0);
        EmSlot_LoadModel(5, 2, 1);
        EmSlot_LoadModel(11, 0, 2);
    }
    for (i = 0, pl = gPlayers; i < LOBBY_MEMBER_COUNT; i++, pl++) {
        if (pl->active != 0 && (s8)gLobby->pcLoaded[i] == 0) {
            TownPc_StartLoad(pl);
            Lobby_ReceiveWhileWaiting();
        }
    }
}

/* Sets player pl up, named name (the member's NetUser is passed but not
 * used): active, at index pl, unscaled, opaque, with two motion layers, a draw slot drawn by
 * DrawCb_Player, idle with the hunters' handlers, and its motion effects
 * step run once. */
void TownPc_Spawn(u8 pl, NetUser *member, char *name)
{
    Player *hunter;
    s16 slot;
    u16 i = pl;

    hunter = &gPlayers[i];
    hunter->active = 1;
    hunter->index = i;
    memcpy(hunter->name, name, LOBBY_USER_NAME_LEN + 1);
    hunter->scale[2] = 1.0f;
    hunter->scale[1] = 1.0f;
    hunter->scale[0] = 1.0f;
    hunter->motionLayerCount = 2;
    hunter->alpha = 1.0f;
    hunter->motion[0].step = 1.0f;
    hunter->motion[1].step = 1.0f;
    hunter->drawSlotId = DrawSlot_Alloc();
    hunter->fieldCollide = 1;
    slot = hunter->drawSlotId;
    if (slot != -1) {
        hunter->drawSlot = DrawSlot_Get(slot);
        hunter->drawSlot->player = hunter->index;
        hunter->drawSlot->draw = DrawCb_Player;
    }
    Actor_ReturnToIdle(hunter, 0, 0, 0);
    hunter->state = TOWN_PC_IDLE;
    hunter->handlers = (struct PlayerVt *)gPlayerHandlerTable;
    hunter->handlers->motionFxStep(hunter);
}

/* Sets the active players up again: the local one's lock-on mark (unless
 * the town is being left, TOWN_MODE_EXIT), no action pending, and, when
 * withDrawSlots' low byte is set, a new draw slot each; then clears the
 * hits. */
void TownPc_InitAll(s32 withDrawSlots)
{
    s32 j;
    Player *pl;
    s16 slot;

    j = 0;
    pl = gPlayers;
    do {
        if (pl->active != 0) {
            if ((gTownMode != TOWN_MODE_EXIT) && (pl->index == gScene.localPlayer)) {
                LockOnMark_Spawn((void *) pl);
            }
            pl->actRequest = 0;
            if (withDrawSlots & 0xFF) {
                pl->drawSlotId = DrawSlot_Alloc();
                slot = pl->drawSlotId;
                if (slot != -1) {
                    pl->drawSlot = DrawSlot_Get((s32) slot);
                    pl->drawSlot->player = pl->index;
                    pl->drawSlot->draw = DrawCb_Player;
                }
            }
        }
        j += 1;
        pl++;
    } while (j < LOBBY_MEMBER_COUNT);
    Hit_ResetAll();
}

/* Loads player pl's models now: mode 0 sets its part table up first; mode 1
 * marks it loading meanwhile and, when done, loaded and shown. The weapon
 * and part models, motion 1 on layer 0 and 101 on layer 1, and its
 * records are set up. */
void TownPc_LoadModel(Player *pl, s32 modeArg)
{
    u8 mode = modeArg;

    if (mode == 1) {
        gLobby->modelLoading[pl->index] = 1;
    }
    if (mode == 0) {
        Player_InitPartTable(pl->index);
    }
    Player_LoadWeaponModel(pl->weaponKind, pl->index);
    Player_LoadPartModels(pl);
    Player_ClearFxBlock(pl);
    Player_StartMotionLayer(pl, 1, 0, 0, 0);
    Player_StartMotionLayer(pl, 101, 0, 0, 1);
    Player_InitRecords(pl);
    if (mode == 1) {
        gLobby->modelLoading[pl->index] = 0;
        gLobby->modelLoaded[pl->index] = 1;
        gLobby->pcLoaded[pl->index] = 1;
        pl->visible = 1;
    }
}

/* Queues the files of player pl's look (Player_QueueLookFiles); once
 * they are queued, marks its models loading (LobbyState.modelLoading). */
void TownPc_StartLoad(Player *pl)
{
    LobbyState *lobby;

    if (Player_QueueLookFiles(pl) == 1) {
        lobby = gLobby;
        lobby->modelLoading[pl->index] = 1;
    }
}

void TownPc_LoadModelNow(Player *pl)
{
    TownPc_LoadModel(pl, 1);
}

/* Frees player pl's models: mode 0 all of them, the player made inactive
 * and hidden; mode 1 only those that changed. Its draw slot is freed and
 * the memory compacted in an area with town players; for mode 0 its
 * loading flags are cleared too. */
void TownPc_Free(Player *pl, s32 modeArg)
{
    u8 mode = modeArg;
    Player *other;
    s32 i;

    if (mode == 0) {
        pl->visible = 0;
        pl->active = 0;
        Player_FreeWeaponModel(pl);
        Player_FreeModels(pl);
    } else {
        Player_FreeWeaponModelIfChanged(pl);
        Player_FreeChangedPartModels(pl);
    }
    if (pl->drawSlot != NULL) {
        DrawSlot_Free(pl->drawSlotId);
        pl->drawSlot = NULL;
    }
    if (Area_HasTownPcs(gScene.area) == 1) {
        Mem_Compact();
    }
    if (mode != 1) {
        for (i = 0, other = gPlayers; i < LOBBY_MEMBER_COUNT; i++, other++) {
            if (other == pl) {
                gLobby->modelLoaded[i] = 0;
                gLobbyState.modelLoading[i] = 0;
                gLobbyState.lookPending[i] = 0;
                return;
            }
        }
    }
}

void TownPc_Remove(Player *pl)
{
    TownPc_Free(pl, 0);
}

void TownPc_FreeChangedModels(Player *pl)
{
    TownPc_Free(pl, 1);
}
