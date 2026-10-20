/*
 * The receiver of the town messages (enum TownMsgCmd) and most of its
 * handlers. TownMsg_Recv takes a message from another member of the town,
 * or one the player sends itself (TownMsg_SendSelf), finds the sender's
 * member index (Lobby_FindMember) and passes the data to the command's
 * handler, named TownMsg_On and the command:
 * - the members' moves, states and areas (TownMsg_OnMove, TownMsg_OnActor,
 *   TownMsg_OnArea), which keep each member's Player in step;
 * - the requests the town host answers (LobbyState.hostId): a song for the
 *   town (TownMsg_OnSongAsk), the contest's seats and the table seats; the
 *   answers reach every member;
 * - an item handed from one member to another (TownMsg_OnOffer, then
 *   TownMsg_OnOfferTake at the giver and TownMsg_OnOfferResult at the
 *   taker);
 * - the arena quest's second side (TownMsg_OnArenaTeam), the Poogie's place
 *   in a member's house, and the house kitchen's messages
 *   (TownMsg_OnKitchen, enum KitchenMsg).
 * - the seats: the town halls' table seats (ActSpot_Take, asked of the town
 *   host) and the seats of a member's house (asked of the house's owner,
 *   kept in LobbyState.houseSeatBits and houseSeatIds), and the house each member
 *   is in (TownMsg_OnHouseVisit, LobbyState.memberSlot).
 * LobbyMember_SetRecord (a member's hunter card received) and Chat_Receive
 * (a chat line) are among the handlers.
 * The .rodata is TownMsg_Recv's switch table.
 */
#include "common.h"
#include "lobby_text.h"
#include "pad.h"
#include "quest.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "item.h"
#include "lobby_globals.h"
#include "lobby_net.h"
#include "hud.h"
#include "scene.h"
#include "save.h"
#include "em.h"
#include "player.h"
#include "lobby_menu.h"
#include "combine.h"
#include "area.h"
#include "lobby_npc.h"
#include "chat_log.h"

/* unit's own declaration: declared with a u16 area (the definition takes s32): with the s32, TownMsg_OnArea passes its u16 area without the zero-extension the game's code has */
s32 Area_IsOnlineHouse(u16 area);
void KitchenGuest_Open(void);
void KitchenGuest_SendCook(s32 arg0);
void KitchenGuest_OnMealEnd(void);
void KitchenGuest_OnMealChosen(struct KitchenMeal *meal);
s32 Lobby_GetPlace(void);
u8 Lobby_FindMember(u8 *id);
void TownMsg_OnMove(u8 pl, void *data, u8 cmd);
void TownMsg_OnActor(u8 pl, void *data);
void TownMsg_OnArea(u8 pl, u8 *msg);
void TownMsg_OnPoogiePos(u8, void *);
void TownMsg_OnKitchen(s8 idx, u8 *data);
void TownMsg_SendRecords(void *to, u8 kind);
void Records_RecvEquipBits(void *id, s32 idx, void *data);
void Records_RecvEquipTally(void *id, void *data);
void Records_RecvItemBits(void *id, void *data);
void Records_RecvTitleBits(void *id, void *data);
void Records_RecvMonsterRecords(void *id, u8 *data, s8 kind);
void TownMsg_OnSongAsk(void *arg0, void *arg1);
void TownMsg_OnSongAnswer(s8 *arg0);
s32 LobbyBgm_IsActive(void);
void LobbyBgm_Play(s32 id, s32 pos, s32 arg2);
void ArenaMenu_Open(void);
void LobbyText_Open(s32);
void TownMsg_OnArenaTeam(u8 *data);
void TownMsg_OnContestSeatAsk(void *id, u8 *msg);
void TownMsg_OnContestSeatAnswer(s8 idx, u8 *msg);
void TownMsg_OnContestSeatTaken(u8 *arg0, u8 *arg1);
void Contest_Open(void *p);
void Contest_RecvStart(void *arg0);
void Contest_RecvPower(void *src);
void Contest_RecvResult(s8 idx, void *data);
void TownMsg_OnSeatAsk(void *arg0, u8 *arg1);
void TownMsg_OnHouseSeatAsk(char *id, u8 *msg);
void TownMsg_OnSeatAnswer(s8, u8 *);
void TownMsg_OnHouseSeatAnswer(s8, u8 *);
void ActSpot_Release(s32 slot);
void TownMsg_OnHouseSeatFree(s8 *msg);
s32 LobbyMember_InMyParty(s32 idx);
void NetQuest_SetInfo(u8 *);
void NetQuest_SetRareRoll(s8);
s32 Season_Get(void);
s32 LobbyTime_IsPm(void);
s32 MyHouse_IsOwner(void);
void Actor_ReturnToIdle(void *obj, s32 a1, s32 n, s32 m);
void TownMsg_OnSeatTaken(u8 *a0);
void TownMsg_OnHouseSeatTaken(u8 *msg);
void TownMsg_OnHouseVisit(s8 idx, u8 *msg);
void TownMsg_OnSober(s8 idx);
void TownMsg_OnOffer(s8 idx, void *src);
void TownMsg_OnOfferTake(void *to, void *src);
void TownMsg_OnOfferResult(void *src);
void TownMsg_OnSyncAsk(u8 *arg0);
void Lobby_ShowMemberName(s8 idx, void *data);
void *memcpy(void *dst, const void *src, u32 n);
extern s32 gTownMode;
extern u8 gTownSeats[];


/* Handles a town message from member `from` (pkt: the command byte, then
 * its data). In a town place a message from someone not among the members
 * is dropped; outside one only command 9 passes, and it does nothing. Each
 * command goes to its handler: a member's moves, facing, area and actions;
 * the records the members exchange (equipment, items, titles, monsters);
 * the stream played in the town; the arena cup's start and the versus
 * quest's sides (into gNextQuest); the town's seats, contests, deliveries
 * and ready marks; the party's quest information; the kitchen's messages
 * (TownMsg_OnKitchen). The first switch on cmd and the two on the arena role
 * put `default` first: written so, the compiler leaves the empty delay
 * slots the game's code has. */
void TownMsg_Recv(void *from, u8 *pkt)
{
    u8 *data;
    u8 cmd;
    u8 idx;
    u16 yaw;
    s32 bgm[2];
    s32 pos;
    u8 buf[0x1A];
    u16 unused;
    Player *pl;
    u8 self;

    cmd = pkt[0];
    data = pkt + 1;
    if (Lobby_GetPlace() != 0) {
        idx = Lobby_FindMember(from);
        if (idx == 0xFF) { /* not a member */
            return;
        }
    } else {
        switch (cmd) {
        default:
            return;
        case TOWN_MSG_09:
            break;
        }
    }
    switch (cmd) {
    case TOWN_MSG_WALK:
        TownMsg_OnMove(idx, data, cmd);
        break;
    case TOWN_MSG_FACING:
        memcpy(&yaw, data, sizeof(yaw));
        gPlayers[idx].rot[1] = yaw;
        gPlayers[idx].targetYaw = yaw;
        gPlayers[idx].netActive = 1;
        break;
    case TOWN_MSG_POOGIE_POS:
        TownMsg_OnPoogiePos(idx, data);
        break;
    case TOWN_MSG_KITCHEN:
        TownMsg_OnKitchen(idx, data);
        break;
    case TOWN_MSG_RECORDS_ASK:
        TownMsg_SendRecords(from, data[0]);
        break;
    case TOWN_MSG_RECORDS_LEGS:
        Records_RecvEquipBits(from, EQUIP_LEGS, data);
        break;
    case TOWN_MSG_RECORDS_HEAD:
        Records_RecvEquipBits(from, EQUIP_HEAD, data);
        break;
    case TOWN_MSG_RECORDS_BODY:
        Records_RecvEquipBits(from, EQUIP_BODY, data);
        break;
    case TOWN_MSG_RECORDS_ARMS:
        Records_RecvEquipBits(from, EQUIP_ARMS, data);
        break;
    case TOWN_MSG_RECORDS_WAIST:
        Records_RecvEquipBits(from, EQUIP_WAIST, data);
        break;
    case TOWN_MSG_RECORDS_BLADES:
        Records_RecvEquipBits(from, EQUIP_BLADE, data);
        break;
    case TOWN_MSG_RECORDS_GUNS:
        Records_RecvEquipBits(from, EQUIP_GUN, data);
        break;
    case TOWN_MSG_RECORDS_WEAPON_TALLY:
    case TOWN_MSG_RECORDS_ARMOR_TALLY:
        Records_RecvEquipTally(from, data);
        break;
    case TOWN_MSG_RECORDS_ITEMS:
        Records_RecvItemBits(from, data);
        break;
    case TOWN_MSG_RECORDS_TITLES:
        Records_RecvTitleBits(from, data);
        break;
    case TOWN_MSG_RECORDS_SIZE_MAX:
        Records_RecvMonsterRecords(from, data, 0);
        break;
    case TOWN_MSG_RECORDS_SIZE_MIN:
        Records_RecvMonsterRecords(from, data, 1);
        break;
    case TOWN_MSG_RECORDS_HUNTED:
        Records_RecvMonsterRecords(from, data, 2);
        break;
    case TOWN_MSG_ACTOR:
    case TOWN_MSG_ACTOR_TO:
        TownMsg_OnActor(idx, data);
        break;
    case TOWN_MSG_PLACE:
        TownMsg_OnMove(idx, data, cmd);
        break;
    case TOWN_MSG_AREA:
        TownMsg_OnArea(idx, data);
        break;
    case TOWN_MSG_SONG_ASK:
        TownMsg_OnSongAsk(from, data);
        break;
    case TOWN_MSG_SONG_ANSWER:
        TownMsg_OnSongAnswer((s8 *)data);
        break;
    case TOWN_MSG_BGM:
        if (!LobbyBgm_IsActive()) {
            memcpy(bgm, data, sizeof(bgm));
            if (gScene.area == AREA_TOWN_ARENA) {
                LobbyBgm_Play(bgm[0], bgm[1], 0);
            } else {
                LobbyBgm_Play(bgm[0], bgm[1], 1);
            }
        }
        break;
    case TOWN_MSG_BGM_POS:
        if (!LobbyBgm_IsActive()) {
            memcpy(&pos, data, sizeof(pos));
            if (gScene.area == AREA_TOWN_ARENA) {
                LobbyBgm_Play(0, pos, 0);
            } else {
                LobbyBgm_Play(0, pos, 1);
            }
        }
        break;
    case TOWN_MSG_ARENA_OPEN:
        switch (gPlayerText.card.arenaRole) {
        case 1:
        case 3:
            ArenaMenu_Open();
            break;
        default:
            gTownMode = TOWN_MODE_QUEST_PARTY;
            LobbyText_Open(0x8D);
            break;
        }
        break;
    case TOWN_MSG_ARENA_TEAM:
        TownMsg_OnArenaTeam(data);
        break;
    case TOWN_MSG_ARENA_TEAMS:
        memcpy(gNextQuest.teams, data, sizeof(gNextQuest.teams));
        memcpy(gNextQuest.teamItems, data + sizeof(gNextQuest.teams), sizeof(gNextQuest.teamItems));
        switch (gPlayerText.card.arenaRole) {
        default:
            gNextQuest.arenaRecord = -1; /* none */
            break;
        case 1:
        case 3:
            break;
        }
        break;
    case TOWN_MSG_ARENA_SIDES:
        memcpy(gNextQuest.sideIds, data, sizeof(gNextQuest.sideIds));
        break;
    case TOWN_MSG_CONTEST_SEAT_ASK:
        TownMsg_OnContestSeatAsk(from, data);
        break;
    case TOWN_MSG_CONTEST_SEAT_ANSWER:
        TownMsg_OnContestSeatAnswer(idx, data);
        break;
    case TOWN_MSG_CONTEST_SEAT_FREE:
        gTownSeats[data[0]] = 0;
        break;
    case TOWN_MSG_CONTEST_SEAT_TAKEN:
        TownMsg_OnContestSeatTaken(from, data);
        break;
    case TOWN_MSG_CONTEST_OPEN:
        idx = Lobby_FindMember(data);
        if (idx != 0xFF && gTownMode != TOWN_MODE_CONTEST) {
            self = gPlayers[gScene.localPlayer].state;
            if (self == TOWN_PC_CONTEST_SEAT_A || self == TOWN_PC_CONTEST_SEAT_B) {
                Contest_Open(&gPlayers[idx]);
            }
        }
        break;
    case TOWN_MSG_CONTEST_START:
        Contest_RecvStart(from);
        break;
    case TOWN_MSG_CONTEST_POWER:
        Contest_RecvPower(data);
        break;
    case TOWN_MSG_CONTEST_RESULT:
        Contest_RecvResult(idx, data);
        break;
    case TOWN_MSG_SEAT_ASK:
        TownMsg_OnSeatAsk(from, data);
        break;
    case TOWN_MSG_HOUSE_SEAT_ASK:
        TownMsg_OnHouseSeatAsk(from, data);
        break;
    case TOWN_MSG_SEAT_ANSWER:
        TownMsg_OnSeatAnswer(idx, data);
        break;
    case TOWN_MSG_HOUSE_SEAT_ANSWER:
        TownMsg_OnHouseSeatAnswer(idx, data);
        break;
    case TOWN_MSG_SEAT_FREE:
        ActSpot_Release(data[0]);
        break;
    case TOWN_MSG_HOUSE_SEAT_FREE:
        TownMsg_OnHouseSeatFree((s8 *)data);
        break;
    case TOWN_MSG_QUEST_INFO:
        if (LobbyMember_InMyParty(idx) == 1) {
            memcpy(buf, data, sizeof(buf));
            NetQuest_SetInfo(buf);
            NetQuest_SetRareRoll(buf[0x19]);
            gSeasonFlags = buf[0x18];
            gLobbyLastSeason = Season_Get();
            gLobbyLastPm = LobbyTime_IsPm();
        }
        break;
    case TOWN_MSG_19:
        self = gScene.localPlayer;
        if ((u8)gLobby->memberSlot[idx] == (u8)gLobby->memberSlot[self]) {
            pl = &gPlayers[self];
            if (pl->state == TOWN_PC_SIT_DOWN_SEATED && pl->stateGroup == TOWN_PC_GROUP_ACTION && MyHouse_IsOwner() == 0) {
                memcpy(&unused, data, sizeof(unused)); /* copied and not used further, as in the game's code */
            }
        }
        break;
    case TOWN_MSG_OFFER_END:
        pl = &gPlayers[idx];
        if (pl->stateGroup == TOWN_PC_GROUP_ACTION && pl->state == TOWN_PC_TAKE_ITEM_WAIT) {
            Actor_ReturnToIdle(pl, 0, 2, 0);
        }
        break;
    case TOWN_MSG_SEAT_TAKEN:
        TownMsg_OnSeatTaken(data);
        break;
    case TOWN_MSG_HOUSE_SEAT_TAKEN:
        TownMsg_OnHouseSeatTaken(data);
        break;
    case TOWN_MSG_HOUSE_VISIT:
        TownMsg_OnHouseVisit(idx, data);
        break;
    case TOWN_MSG_SOBER:
        TownMsg_OnSober(idx);
        break;
    case TOWN_MSG_OFFER:
        TownMsg_OnOffer(idx, data);
        break;
    case TOWN_MSG_OFFER_TAKE:
        TownMsg_OnOfferTake(from, data);
        break;
    case TOWN_MSG_OFFER_RESULT:
        TownMsg_OnOfferResult(data);
        break;
    case TOWN_MSG_SYNC_ASK:
        TownMsg_OnSyncAsk(from);
        break;
    case TOWN_MSG_NAME_NOTICE:
        Lobby_ShowMemberName(idx, data);
        break;
    case TOWN_MSG_09:
    case TOWN_MSG_11:
    case TOWN_MSG_13:
        break;
    }
}

/* The kitchen's town message (enum KitchenMsg, its first byte), heeded only
 * in an online house and from the member that the player's
 * LobbyState.memberSlot entry names (the house's owner, by the look of it):
 * opens the guest's menu, sends the cook to the table, takes the meal
 * chosen (KitchenMeal), or ends the meal. */
void TownMsg_OnKitchen(s8 idx, u8 *data)
{
    if (Area_IsOnlineHouse(gScene.area) != 0 && idx == (u8)gLobby->memberSlot[gScene.localPlayer] - '0') {
        switch (data[0]) {
        case KITCHEN_MSG_OPEN:
            KitchenGuest_Open();
            break;
        case KITCHEN_MSG_ROUTE:
            KitchenGuest_SendCook(data[1]); /* the spot (enum KitchenSpot) */
            break;
        case KITCHEN_MSG_DONE:
            KitchenGuest_OnMealEnd();
            break;
        case KITCHEN_MSG_CHOSEN:
            KitchenGuest_OnMealChosen((struct KitchenMeal *)(data + 1));
            break;
        }
    }
}

void *memcpy(void *dst, const void *src, u32 n);

/* TOWN_MSG_ARENA_TEAM: the second side's monsters and its item, into
 * gNextQuest. */
void TownMsg_OnArenaTeam(u8 *data)
{
    memcpy(&gNextQuest.teams[1], data, sizeof(gNextQuest.teams[1]));
    memcpy(&gNextQuest.teamItems[1], data + sizeof(gNextQuest.teams[1]), sizeof(gNextQuest.teamItems[1]));
}


extern NetUser gNetUsers[];

void *memcpy(void *dst, const void *src, u32 n);
/* unit's own declaration: the count is declared s32 (the definition takes an s16): with the s16, TownMsg_OnOfferResult would narrow the count before the call, which the game's code doesn't */
u8 SavePouch_Add(u16 item, s32 n);
u8 Notice_Push(u8, s16, s16);
s16 Player_IsState(void *obj, u8 state, u8 sub);
u8 Lobby_FindMember(u8 *arg0);
void Chat_AddReceived(u8 arg0, s8 *arg1);
void ChatLog_Receive(ChatMsg *msg);
Em *Npc_Find(u8 kind);
void Actor_SetStateNow(struct Player *obj, u8 state, u8 sub, u16 flags);
s32 memcmp(void *, void *, s32);
s32 Lobby_GetPlace(void);
void Chat_SystemMsg(s32 msg, char *arg);
extern u8 gSongRequesterId[];
s32 LobbyBgm_IsActive(void);
void TownMsg_SendSongAnswer(void *src, s32 x, u8 c);
extern u8 gMyUserId[];
void SongMenu_OnBusy(void);
void SongMenu_OnAccepted(void);
void TownMsg_SendBgm(s32 arg0);

void TownPc_StartMoveTo(struct Player *obj, s16 n);
f32 Math_Atan2(f32 y, f32 x);
f32 Vec_Length(f32 *v);
s32 TownPc_IsSeatedState(u8 arg0, u8 arg1);
s32 Area_IsTownHall(s32 arg0);
void Mem_Copy(void *, void *, s32);
extern s16 gTownMoveFrames;

typedef struct PosMsg {
    s16 x;
    s16 z;
    u8 area;
} PosMsg;

extern u16 gMsgAreaIds[];


/* TOWN_MSG_NAME_NOTICE: in a town place, the chat's system line that member
 * idx is here, with its name (its id while gChatHideNames hides names). */
void Lobby_ShowMemberName(s8 idx, void *data)
{
    if (Lobby_GetPlace() != 0 && gNetUsers[idx].name[0] != 0) {
        if (gChatHideNames != 0) {
            Chat_SystemMsg(3, gNetUsers[idx].id); /* 3: the member is here */
        } else {
            Chat_SystemMsg(3, gNetUsers[idx].name);
        }
    }
}

/* TOWN_MSG_SONG_ASK, heeded by the town host: when no tune plays and no
 * song is asked for, member src becomes the asker (gSongRequesterId) and
 * TownMsg_SendSongAnswer answers yes to everyone; else it answers busy. */
void TownMsg_OnSongAsk(void *src, void *data)
{
    s32 song;

    if (memcmp(gLobby->myId, gLobby->hostId, LOBBY_USER_ID_LEN) == 0) {
        memcpy(&song, data, sizeof(song));
        if (LobbyBgm_IsActive() == 0 && gSongRequesterId[0] == 0) {
            memcpy(gSongRequesterId, src, LOBBY_USER_ID_LEN);
            TownMsg_SendSongAnswer(src, song, 0);
        } else {
            TownMsg_SendSongAnswer(src, song, 1);
        }
    }
}

/* TOWN_MSG_SONG_ANSWER (data: busy, the song, the asker's id): a busy
 * answer to the player tells the song menu so; else the asker is kept, and
 * when it is the player the song plays for everyone (TownMsg_SendBgm). */
void TownMsg_OnSongAnswer(s8 *data)
{
    s32 song;

    if (data[0] == 1 && memcmp(data + 5, gMyUserId, LOBBY_USER_ID_LEN) == 0) {
        SongMenu_OnBusy();
        return;
    }
    memcpy(gSongRequesterId, data + 5, LOBBY_USER_ID_LEN);
    if (memcmp(gSongRequesterId, gMyUserId, LOBBY_USER_ID_LEN) == 0) {
        memcpy(&song, data + 1, sizeof(song));
        TownMsg_SendBgm(song);
        SongMenu_OnAccepted();
    }
}


/* TOWN_MSG_OFFER: member idx holds out items (OfferMsg) to the member whose
 * id it gives; it takes TOWN_PC_OFFER_ITEM with the item, count and taker. */
void TownMsg_OnOffer(s8 idx, void *src)
{
    Player *pl = &gPlayers[idx];
    OfferMsg offer;

    if (pl->active != 0) {
        memcpy(&offer, src, sizeof(offer));
        pl->offerTo = Lobby_FindMember(offer.id);
        pl->offerItem = offer.item;
        pl->offerCount = offer.count;
        Actor_SetStateNow(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_OFFER_ITEM, 0x10);
    }
}

s16 Pouch_Count(Player *p, u16 item);
void TownMsg_SendOfferResult(void *to, u8 c);

/* TOWN_MSG_OFFER_TAKE, from the member taking the player's offer (an
 * OfferMsg): when the player still holds out that item and count to that
 * member (TOWN_PC_OFFER_ITEM, PL_OFFER_WAITING) and has them, they leave the
 * pouch, TownMsg_SendOfferResult answers 0 and the offer is PL_OFFER_GIVEN; else it
 * answers 1 (refused). */
void TownMsg_OnOfferTake(void *from, void *src)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    OfferMsg offer;

    memcpy(&offer, src, sizeof(offer));
    if (Player_IsState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_OFFER_ITEM) != 0 && pl->offerTo == Lobby_FindMember(offer.id)
        && pl->offerItem == offer.item) {
        u16 n = offer.count;

        if (Pouch_Count(pl, offer.item) >= n && pl->offerCount == n && pl->offerState == PL_OFFER_WAITING) {
            SavePouch_Add(offer.item, (s16)-n);
            TownMsg_SendOfferResult(from, 0);
            pl->offerState = PL_OFFER_GIVEN;
            return;
        }
    }
    TownMsg_SendOfferResult(from, 1);
}

/* TOWN_MSG_OFFER_RESULT, the giver's answer to the player waiting in
 * TOWN_PC_TAKE_ITEM_WAIT: the items go into the pouch with a notice
 * (PL_OFFER_RECEIVED), or the offer was refused (PL_OFFER_REFUSED). */
void TownMsg_OnOfferResult(void *src)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    OfferResultMsg result;

    if (Player_IsState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_TAKE_ITEM_WAIT) != 0) {
        memcpy(&result, src, sizeof(result));
        if (result.refused == 0) {
            pl->offerState = PL_OFFER_RECEIVED;
            SavePouch_Add(result.item, result.count);
            Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_RECEIVED, result.item);
        } else {
            pl->offerState = PL_OFFER_REFUSED;
        }
    }
}

void HunterCard_Apply(s8 idx, void *src);
void TownPc_Spawn(u8 pl, NetUser *u, char *name);
void TownPc_FreeChangedModels(Player *a0);

/* A member's hunter card received (rec, the 0x100 bytes of NetUser.text):
 * when its weapon or armour changed, HunterCard_Apply and TownPc_Spawn make
 * the member's model again (after TownPc_FreeChangedModels when it was
 * loaded, returning 1); else only HunterCard_Apply. The card is then kept
 * in LobbyState.members, and the player's own copied to gPlayerText. */
s32 LobbyMember_SetRecord(s8 idx, NetUser *user, u8 *rec)
{
    HunterCard *kept;
    HunterCard *card = (HunterCard *)rec;
    s32 ret;
    s32 off;

    off = idx * sizeof(NetUser);
    kept = LOBBY_MEMBER_CARD_AT(off);
    ret = 0;
    if (memcmp(kept->armour, card->armour, sizeof(kept->armour)) != 0
        || memcmp(&kept->weapon, &card->weapon, sizeof(kept->weapon)) != 0) {
        if ((s8)gLobby->pcLoaded[idx] == 0) {
            HunterCard_Apply(idx, rec);
            TownPc_Spawn(idx, user, LOBBY_MEMBER_AT(off).name);
        } else {
            HunterCard_Apply(idx, rec);
            TownPc_FreeChangedModels(&gPlayers[idx]);
            TownPc_Spawn(idx, user, LOBBY_MEMBER_AT(off).name);
            ret = 1;
        }
        gLobby->pcLoaded[idx] = 0;
    } else {
        HunterCard_Apply(idx, rec);
    }
    memcpy(kept, card, sizeof(user->text)); /* the whole record, past the card */
    if (idx == gScene.localPlayer) {
        memcpy(&gPlayerText.card, rec, sizeof(HunterCard));
    }
    return ret;
}

/* A chat message received (its kind at byte 0x120): 2, a member's line
 * (Chat_AddReceived, by the sender's member index); 1, a line for the chat
 * log (ChatLog_Receive). */
void Chat_Receive(u8 *msg)
{
    u8 idx;

    if (msg[0x120] == 2) {
        idx = Lobby_FindMember(msg);
        Chat_AddReceived(idx, (s8 *)msg);
    } else if (msg[0x120] == 1) {
        ChatLog_Receive((ChatMsg *)msg);
    }
}

/* The member index (0 to LOBBY_MEMBER_COUNT - 1) of the user whose id is
 * given, by its first NET_USER_ID_MATCH bytes; 0xFF for none or an empty id. */
u8 Lobby_FindMember(u8 *id)
{
    u8 i;

    if (*(s8 *)id == 0) {
        return 0xFF;
    }
    for (i = 0; i < LOBBY_MEMBER_COUNT; i++) {
        if (memcmp(gNetUsers[i].id, id, NET_USER_ID_MATCH) == 0) {
            return i;
        }
    }
    return 0xFF;
}

extern u8 gPoogieLeft;
extern s32 gTownMode;
extern M2C_UNK D_008C0148;
void ActSpot_Take(u8 slot);
void ActSpot_Release(s32 slot);
s32 TownPc_IsLeaveSeatState(s32 a, s32 b);
void *memset(void *d, s32 c, u32 n);
void Actor_SetState(void *, s32, s32);
typedef struct ActMsg {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 z;
    /* 0x4 */ u8 angle;
    /* 0x5 */ u8 slot;
    /* 0x6 */ u8 group;
    /* 0x7 */ u8 state;
    /* 0x8 */ u8 item;
} ActMsg;

/* TOWN_MSG_ACTOR and TOWN_MSG_ACTOR_TO: a member's position, angle,
 * held item and state. The member walks there and takes the state; a
 * seated (or leaving) state marks its seat taken (or free): a house seat in
 * LobbyState.houseSeatBits when the member is in the player's house, else the
 * town's table seat (ActSpot). */
void TownMsg_OnActor(u8 pl, void *data)
{
    Player *member = &gPlayers[pl];
    u8 buf[10];
    ActMsg *msg;
    u16 yaw;

    Mem_Copy(buf, data, sizeof(buf));
    msg = (ActMsg *)buf;
    if (msg->item != 0 && Area_IsTownHall(member->area) == 0) {
        return;
    }
    yaw = msg->angle << 8;
    member->netActive = 1;
    if (member->pos[0] == 0.0f && member->pos[1] == 0.0f) {
        member->rot[1] = yaw;
        member->targetYaw = yaw;
    } else {
        member->targetYaw = yaw;
    }
    member->moveTo[0] = msg->x;
    member->moveTo[2] = msg->z;
    member->drunk = msg->item;
    TownPc_StartMoveTo((struct Player *)((u8 *) member), 10);
    if (TownPc_IsSeatedState(msg->group, msg->state) == 1) {
        if ((u8)gLobby->memberSlot[pl] == (s8)(gScene.localPlayer + '0')) {
            gLobby->houseSeatBits = gLobby->houseSeatBits | (1 << msg->slot);
            memcpy(&gLobby->houseSeatIds[msg->slot * LOBBY_USER_ID_LEN], gNetUsers[pl].id, LOBBY_USER_ID_LEN);
        } else {
            ActSpot_Take(msg->slot);
        }
    } else if (TownPc_IsLeaveSeatState(msg->group, msg->state) == 1) {
        if ((u8)gLobby->memberSlot[pl] == (s8)(gScene.localPlayer + '0')) {
            gLobby->houseSeatBits = gLobby->houseSeatBits & ~(1 << msg->slot);
            memset(&gLobby->houseSeatIds[msg->slot * LOBBY_USER_ID_LEN], 0, LOBBY_USER_ID_LEN);
        } else {
            ActSpot_Release(msg->slot);
        }
    }
    if (msg->state == TOWN_PC_SIT_DOWN && member->stateGroup == TOWN_PC_GROUP_ACTION && msg->state != member->state) {
        Actor_SetStateNow((struct Player *)((u8 *) member), TOWN_PC_GROUP_ACTION, TOWN_PC_SIT_DOWN, 0);
        return;
    }
    if (member->state == msg->state && member->stateGroup == msg->group) {
        return;
    }
    if (gTownMode == TOWN_MODE_CONTEST && msg->group == TOWN_PC_GROUP_ACTION && msg->state == TOWN_PC_CONTEST_GRIP) {
        return;
    }
    Actor_SetState(member, msg->group, msg->state);
}

/* TOWN_MSG_WALK and TOWN_MSG_PLACE: a member's position (x, z) and area
 * index. TOWN_MSG_WALK walks the member there when it isn't seated in the
 * player's area; otherwise it turns toward the point and is put there. */
void TownMsg_OnMove(u8 pl, void *data, u8 cmd)
{
    Player *member = &gPlayers[pl];
    PosMsg msg;
    f32 delta[3];
    PosMsg *posMsg;

    Mem_Copy(&msg, data, 5); /* the message's 5 bytes: x, z and the area index */
    if (member->netActive != 0) {
        posMsg = &msg;
        if (cmd == TOWN_MSG_WALK && member->area == gScene.area) {
            if (TownPc_IsSeatedState(member->stateGroup, member->state) == 0) {
                member->moveTo[0] = posMsg->x;
                member->moveTo[2] = posMsg->z;
                TownPc_StartMoveTo((struct Player *)((u8 *) member), gTownMoveFrames);
                member->area = gMsgAreaIds[posMsg->area];
                if (Area_IsTownHall(member->area) == 0) {
                    member->drunk = 0;
                    member->drinkCount = 0;
                }
            }
        } else {
            delta[0] = member->moveTo[0] - member->pos[0];
            delta[1] = 0.0f;
            delta[2] = member->moveTo[2] - member->pos[2];
            member->moveToFrames = 0;
            if (!(Vec_Length(delta) <= 5.0f)) {
                s32 yaw = 0.5f + 65536.0f * Math_Atan2(delta[0], delta[2]) / 6.2831855f;
                member->targetYaw = yaw;
                member->rot[1] = yaw & 0xFFFF;
            }
            member->moveTo[0] = posMsg->x;
            member->moveTo[2] = posMsg->z;
            *(Vec3f *)member->pos = *(Vec3f *)member->moveTo;
            *(Vec3f *)member->prevPos = *(Vec3f *)member->pos;
        }
    }
}

/* TOWN_MSG_POOGIE_POS's data. */
typedef struct PoogiePosMsg {
    s16 x;
    s16 z;
    s16 pad4;
} PoogiePosMsg;

/* TOWN_MSG_POOGIE_POS: where the house owner's Poogie stands (x, z), for
 * the Poogie in the player's view of the house. */
void TownMsg_OnPoogiePos(u8 unused, void *data)
{
    Em *em = Npc_Find(NPC_POOGIE);
    PoogiePosMsg pos;

    memcpy(&pos, data, sizeof(pos));
    if (em != NULL) {
        em->posX = pos.x;
        em->posZ = pos.z;
    }
}

void TownMsg_SendPoogiePos(Em *pl, void *to);
void Actor_SetPosture(void *a, u16 mode, s32 b, s32 c);
void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);

/* TOWN_MSG_AREA: the area a member moved to (an index
 * into gMsgAreaIds) and its id: its group slot, its area, and the name
 * shown when it comes back to the town (area 0). */
void TownMsg_OnArea(u8 pl, u8 *msg)
{
    Player *member = &gPlayers[pl];
    Player *self = &gPlayers[gScene.localPlayer];
    u16 area = gMsgAreaIds[msg[0]];
    Em *em = Npc_Find(NPC_POOGIE);
    s8 slot;

    if (Area_IsOnlineHouse(area) == 1) {
        slot = Lobby_FindMember(msg + 1);
        gLobby->memberSlot[pl] = slot + '0';
        if (slot == gScene.localPlayer && gPoogieErrand != 1 && gPoogieLeft != 1) {
            TownMsg_SendPoogiePos(em, &gNetUsers[pl]);
        }
    } else {
        gLobby->memberSlot[pl] = 0;
    }
    if (area != member->area) {
        if (member->area == 0) {
            member->netActive = 1;
        }
        member->rot[1] = (u16)member->targetYaw;
        member->area = area;
        if (Area_IsTownHall(member->area) == 0 && member->active != 0) {
            member->drunk = 0;
            Actor_SetPosture(member, 0, 0, 0);
            Actor_SetMotion(member, 1, 0, 0);
            Actor_SetState(member, 0, 1);
        }
        member->drunk = 0;
        member->drinkCount = 0;
        if (self->target != NULL && ((Player *)self->target)->index == pl && ((Player *)self->target)->ownTask == 0) {
            self->target = NULL;
        }
        if (area == 0 && Lobby_GetPlace() != 0 && gNetUsers[pl].name[0] != 0) {
            if (gChatHideNames != 0) {
                Chat_SystemMsg(7, (char *) gNetUsers[pl].id); /* 7: the member is back in the town */
            } else {
                Chat_SystemMsg(7, (char *) gNetUsers[pl].name);
            }
        }
    }
}

extern u8 gTownSeats[];
void TownMsg_SendContestSeatAnswer(char *name, u8 a, u8 b);
void TownMsg_SendContestOpen(void *a, void *b);

/* TOWN_MSG_CONTEST_SEAT_ASK, heeded by the town host: contest seat msg[0]
 * of gTownSeats ('0' + the member index) is given to the asker
 * (TownMsg_SendContestSeatAnswer answers 1), or refused when taken (0); with seats 1 and
 * 2 both taken, TownMsg_SendContestOpen sends each contestant the other. */
void TownMsg_OnContestSeatAsk(void *id, u8 *msg)
{
    u8 seatIndex;
    u8 *seat;

    if (memcmp(gLobby->myId, gLobby->hostId, LOBBY_USER_ID_LEN) == 0) {
        seatIndex = msg[0];
        seat = gTownSeats + seatIndex;
        if (*seat != 0) {
            TownMsg_SendContestSeatAnswer(id, 0, seatIndex);
            return;
        }
        *seat = Lobby_FindMember(id) + '0';
        TownMsg_SendContestSeatAnswer(id, 1, seatIndex);
        if (gTownSeats[1] != 0 && gTownSeats[2] != 0) {
            TownMsg_SendContestOpen(&gNetUsers[(u8)(gTownSeats[2] - '0')], &gNetUsers[(u8)(gTownSeats[1] - '0')]);
        }
    }
}

void Actor_ReturnToIdle(void *, s32, s32, s32);
void Contest_TakeSeat(void);
void TownMsg_SendContestSeatFree(struct Player *arg0);
extern s32 gTownMode;
extern s32 gTownMenuOpen;

/* TOWN_MSG_CONTEST_SEAT_ANSWER (msg: yes or no, the seat, the asker's id):
 * a no to the player waiting for a contest seat ends the wait; a yes marks
 * the seat, and the player, when it asked, sits down (Contest_TakeSeat in
 * AREA_TAVERN while waiting) or says it took the seat (TownMsg_SendContestSeatFree). */
void TownMsg_OnContestSeatAnswer(s8 idx, u8 *msg)
{
    s32 self;
    s32 seatIndex;

    if (msg[0] == 0) {
        self = gScene.localPlayer;
        if (self == Lobby_FindMember(msg + 2) && gTownMode == TOWN_MODE_CONTEST_SEAT_WAIT) {
            gTownMode = 0;
            gTownMenuOpen = 0;
            gFieldControl = 1;
            Actor_ReturnToIdle(&gPlayers[(u8)self], 0, 0x14, 0);
        }
        return;
    }
    seatIndex = msg[1];
    gTownSeats[(u8)seatIndex] = Lobby_FindMember(msg + 2) + '0';
    self = gScene.localPlayer;
    if (self == Lobby_FindMember(msg + 2)) {
        if (gScene.area == AREA_TAVERN && gTownMode == TOWN_MODE_CONTEST_SEAT_WAIT) {
            Contest_TakeSeat();
            return;
        }
        TownMsg_SendContestSeatFree(&gPlayers[(u8)self]);
        return;
    }
}


/* TOWN_MSG_CONTEST_SEAT_TAKEN: member id sits in contest seat data[0]. */
void TownMsg_OnContestSeatTaken(u8 *id, u8 *data)
{
    u8 idx = *data;

    gTownSeats[idx] = Lobby_FindMember(id) + '0';
}

void TownMsg_SendSeatAnswer(char *name, u8 a, u8 b);
void ActSpot_Take(u8);
s32 ActSpot_IsFree(u32 n);
s32 memcmp(void *, void *, s32);

/* TOWN_MSG_SEAT_ASK, heeded by the town host: table seat data[0] is
 * taken for the asker (TownMsg_SendSeatAnswer answers 1), or refused when taken
 * (0). */
void TownMsg_OnSeatAsk(void *name, u8 *data)
{
    u8 seat;

    if (memcmp(gLobby->myId, gLobby->hostId, LOBBY_USER_ID_LEN) == 0) {
        seat = *data;
        if (ActSpot_IsFree(seat) == 0) {
            TownMsg_SendSeatAnswer(name, 0, seat);
            return;
        }
        ActSpot_Take(seat);
        TownMsg_SendSeatAnswer(name, 1, seat);
    }
}

void TownPc_TakeSeat(void);
void TownMsg_SendHouseSeatFree(void *pl);
void TownMsg_SendSeatFree(Player *pl);

void TownMsg_SendHouseSeatAnswer(char *name, u8 a, u8 b);
extern u8 gHouseAccess;

/* TOWN_MSG_HOUSE_SEAT_ASK, heeded by the house's owner: house seat msg[0]
 * is asked for the member id. It is refused (TownMsg_SendHouseSeatAnswer answers 0)
 * when the seat is taken, when the owner is in the kitchen, or when the
 * house is closed and the asker is not the owner; otherwise the seat's bit
 * is set in LobbyState.houseSeatBits, the asker's id kept by seat in
 * LobbyState.houseSeatIds, and the answer is 1. */
void TownMsg_OnHouseSeatAsk(char *id, u8 *msg)
{
    u8 slot = msg[0];
    s32 bit = 1 << slot;

    if ((gLobby->houseSeatBits & bit) || gTownMode == TOWN_MODE_KITCHEN
        || (gHouseAccess == HOUSE_ACCESS_CLOSED && gScene.localPlayer != (u8)Lobby_FindMember((u8 *)id))) {
        TownMsg_SendHouseSeatAnswer(id, 0, slot);
    } else {
        gLobby->houseSeatBits = gLobby->houseSeatBits | bit;
        memcpy(gLobby->houseSeatIds + (u8)slot * LOBBY_USER_ID_LEN, id, LOBBY_USER_ID_LEN);
        TownMsg_SendHouseSeatAnswer(id, 1, slot);
    }
}

/* TOWN_MSG_SEAT_ANSWER, the host's answer to a table seat's ask: data[0]
 * is yes (1) or no (0), data[1] the seat, data + 2 the asker's id. A no
 * ends the local player's wait for the seat. A yes marks the seat taken
 * (ActSpot_Take); when the asker is the local player, it sits down if it
 * is still waiting in a town hall, and otherwise gives the seat back
 * (TownMsg_SendSeatFree, TOWN_MSG_SEAT_FREE). */
void TownMsg_OnSeatAnswer(s8 unused, u8 *data)
{
    if (data[0] == 0) {
        if (gScene.localPlayer == (u8)Lobby_FindMember(data + 2) && gTownMode == TOWN_MODE_SEAT_WAIT) {
            gTownMode = TOWN_MODE_FREE;
            gTownMenuOpen = 0;
        }
        return;
    }
    ActSpot_Take(data[1]);
    if (gScene.localPlayer == (u8)Lobby_FindMember(data + 2)) {
        if (Area_IsTownHall(gScene.area) == 1 && gTownMode == TOWN_MODE_SEAT_WAIT) {
            TownPc_TakeSeat();
            return;
        }
        TownMsg_SendSeatFree(&gPlayers[gScene.localPlayer]);
        return;
    }
}

/* TOWN_MSG_HOUSE_SEAT_ANSWER, the owner's answer to a house seat's ask,
 * laid out as TownMsg_OnSeatAnswer's. A no ends the local player's wait;
 * a yes seats the local player if it asked and still waits in an online
 * house, and otherwise gives the seat back (TownMsg_SendHouseSeatFree,
 * TOWN_MSG_HOUSE_SEAT_FREE). */
void TownMsg_OnHouseSeatAnswer(s8 unused, u8 *data)
{
    if (data[0] == 0) {
        if (gScene.localPlayer == (u8)Lobby_FindMember(data + 2) && gTownMode == TOWN_MODE_SEAT_WAIT) {
            gTownMode = TOWN_MODE_FREE;
            gTownMenuOpen = 0;
        }
        return;
    }
    if (gScene.localPlayer == (u8)Lobby_FindMember(data + 2)) {
        if (Area_IsOnlineHouse(gScene.area) == 1 && gTownMode == TOWN_MODE_SEAT_WAIT) {
            TownPc_TakeSeat();
            return;
        }
        TownMsg_SendHouseSeatFree(&gPlayers[gScene.localPlayer]);
        return;
    }
}

/* The bit and the mask of house seat n. Written as plain expressions in
 * the two handlers below, the compiler takes the `or` and the `and`
 * operands the other way round from the game's code. */
static inline s32 Bit(s32 n)
{
    return 1 << n;
}

static inline s32 BitMask(s32 n)
{
    return ~(1 << n);
}

/* TOWN_MSG_HOUSE_SEAT_FREE: msg[0] is a house seat, msg + 1 the sender's
 * id. When the sender is in the local player's house
 * (LobbyState.memberSlot holds '0' + the index of the house's owner), the
 * seat's bit in LobbyState.houseSeatBits is cleared. memberSlot (offset
 * 0x1538) is read through a byte pointer to the whole state: read
 * through the field, the compiler keeps the state's address in other
 * registers than the game's code does. */
void TownMsg_OnHouseSeatFree(s8 *msg)
{
    u8 *p = (u8 *)gLobby;

    if ((p + 0x1538)[(u8)Lobby_FindMember((u8 *)msg + 1)] == (s8)(gScene.localPlayer + '0')) {
        gLobby->houseSeatBits = BitMask(msg[0]) & gLobby->houseSeatBits;
    }
}

/* TOWN_MSG_SEAT_TAKEN: table seat a0[0] is taken. */
void TownMsg_OnSeatTaken(u8 *a0)
{
    ActSpot_Take(*(u8 *)a0);
}

/* TOWN_MSG_HOUSE_SEAT_TAKEN: as TownMsg_OnHouseSeatFree, but the seat's bit
 * is set. */
void TownMsg_OnHouseSeatTaken(u8 *msg)
{
    u8 *p = (u8 *)gLobby;

    if ((p + 0x1538)[(u8)Lobby_FindMember(msg + 1)] == (s8)(gScene.localPlayer + '0')) {
        gLobby->houseSeatBits = Bit(msg[0]) | gLobby->houseSeatBits;
    }
}

/* 1 when the town's players are shown in the area: the three town halls,
 * AREA_HUB_0x84 and the online houses. */
s32 Area_HasTownPcs(u16 area)
{
    s32 id;

    id = area & 0xFFFF;
    switch (id) {
    case AREA_TAVERN:
    case AREA_TOWN_ARENA:
    case AREA_ELDER_HALL:
    case AREA_HUB_0x84:
        return 1;
    case AREA_ONLINE_HOUSE_0:
    case AREA_ONLINE_HOUSE_1:
    case AREA_ONLINE_HOUSE_2:
        return 1;
    default:
        return 0;
    }
}

s32 TownPc_IsInContest(Player *p);
void TownMsg_SendBgmTo(s32 unused, void *to);
void TownMsg_SendActor(Player *a, void *to);
void TownMsg_SendAreaTo(void *to, u16 id);
void TownMsg_SendContestSeatTaken(void);
void TownMsg_SendOffer(Player *p, void *x);
s32 LobbyBgm_GetTrack(void);
extern s8 gLobbyScreenMode;

/* TOWN_MSG_SYNC_ASK: a newcomer asks for the local player's place. While
 * the town is up, the answer goes to the asker alone: the area; where
 * the town's players are shown, the player's state, and the item it is
 * offering if it is just starting an offer (TOWN_PC_OFFER_ITEM); its
 * contest seat; and the town's tune if one plays. */
void TownMsg_OnSyncAsk(u8 *from)
{
    s32 track;
    u8 localPlayer;

    track = LobbyBgm_GetTrack();
    if (gLobbyScreenMode == LOBBY_SCREEN_TOWN) {
        TownMsg_SendAreaTo(from, (s32) gScene.area);
        if (Area_HasTownPcs(gScene.area) == 1) {
            TownMsg_SendActor(&gPlayers[gScene.localPlayer], from);
            localPlayer = gScene.localPlayer;
            if ((gPlayers[localPlayer].offerStarted != 0) && (gPlayers[localPlayer].state == TOWN_PC_OFFER_ITEM) && (gPlayers[localPlayer].stateGroup == 0)) {
                TownMsg_SendOffer(&gPlayers[localPlayer], from);
            }
        }
        if (TownPc_IsInContest(&gPlayers[gScene.localPlayer]) == 1) {
            TownMsg_SendContestSeatTaken();
        }
        if (track != -1) {
            TownMsg_SendBgmTo(track, from);
        }
    }
}

/* TOWN_MSG_HOUSE_VISIT: member idx went into the house of the member whose
 * id msg gives; LobbyState.memberSlot[idx] becomes '0' + that member's
 * index. */
void TownMsg_OnHouseVisit(s8 idx, u8 *msg)
{
    u8 id[LOBBY_USER_ID_LEN + 1];

    memcpy(id, msg, LOBBY_USER_ID_LEN);
    id[LOBBY_USER_ID_LEN] = 0;
    gLobby->memberSlot[idx] = (s8)Lobby_FindMember(id) + '0';
}

/* TOWN_MSG_SOBER: member idx is no longer drunk. */
void TownMsg_OnSober(s8 idx)
{
    Player *pl;

    pl = &gPlayers[idx];
    pl->drunk = 0;
    pl->drinkCount = 0;
}
