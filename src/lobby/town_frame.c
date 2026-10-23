/*
 * The town's frame and its modes.
 *
 * Town_Frame is the town's (and the village's) frame: the timers, the
 * music, the members' sync and the townspeople's spawns, the town host
 * (the member with the lowest user id), the players and the townspeople
 * (gEms), then Town_UpdateMode, the camera, the lights, the objects, the
 * HUD and the players' draw slots.
 *
 * Town_UpdateMode runs the mode the town is in (gTownMode, enum TownMode,
 * lobby_menu.h): one counter's or menu's update per mode, the quest gate's
 * questions and the set-out, the departure and the quit, the exit to the
 * village (Town_ModeDisconnect). While a mode sets gLobbyMenu->redraw it
 * holds the player: the chat keyboard and the game menu are closed each
 * frame (Town_IsPlayerHeld reads it).
 *
 * Around them: the reload of the local player's look, the label list the
 * counters draw, the online house's host check, the hunter card's parts
 * sent as they change, and the placing of the local player at an area's
 * entry (Town_PlaceLocalPlayer). The file's data is the entry points of
 * the town's areas.
 */
#include "common.h"
#include "sprite.h"
#include "syssettings.h"
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
#include "lobby_npc.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "lobby_net.h"
#include "quest.h"
#include "chat_log.h"
#include "area_anim.h"
#include "area_draw.h"
#include "bgm.h"
#include "obj.h"
#include "box_menu.h"
#include "camera.h"
#include "card.h"
#include "game_menu.h"
#include "hud.h"
#include "light.h"
#include "lobby_group_board.h"
#include "stream.h"
#include "village_cond.h"
#include "town_frame.h"
#include "area_board.h"
#include "area_list.h"
#include "area_menu.h"
#include "arena_cup.h"
#include "arena_menu.h"
#include "arm_wrestling.h"
#include "bait_menu.h"
#include "bed_menu.h"
#include "combine_shop.h"
#include "dojo_menu.h"
#include "draw_slots.h"
#include "drink_contest.h"
#include "elder_forecast.h"
#include "equip_shop.h"
#include "font.h"
#include "gallery_objects.h"
#include "game_util.h"
#include "hair_menu.h"
#include "house_access.h"
#include "hunter_card.h"
#include "ingredient_list.h"
#include "item_shop.h"
#include "kitchen.h"
#include "kitchen_guest.h"
#include "lobby_dialog_box.h"
#include "lobby_disconnect.h"
#include "lobby_leave_menus.h"
#include "lobby_room.h"
#include "lobby_screen.h"
#include "lobby_steps.h"
#include "lobby_util.h"
#include "lobby_village.h"
#include "lobby_world.h"
#include "market.h"
#include "mem_util.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "move_menu.h"
#include "my_house.h"
#include "name_tags.h"
#include "offer_menu.h"
#include "official_desk.h"
#include "pets.h"
#include "quest_counter.h"
#include "quest_desk.h"
#include "quest_offers.h"
#include "quest_order_terms.h"
#include "rand.h"
#include "records_menu.h"
#include "ship_mine_menus.h"
#include "shooting_gallery.h"
#include "song_menu.h"
#include "talk_scripts.h"
#include "town_guide.h"
#include "town_lock_on.h"
#include "town_members.h"
#include "town_player_states.h"
#include "town_player_update.h"
#include "town_players.h"
#include "village_board.h"
#include "village_records.h"
#include "wanted.h"
#include "workshop_menu.h"
#include "screen_fade.h"
#include "libc.h"
#include "sound.h"

extern LobbyMenu *gLobbyMenu;

extern u8 gTownSyncWait;
extern u8 gLobbyTextDirty;
extern NetUser gNetUsers[];
extern char gMyUserId[];
extern char gMyName[];

/* Rebuilds the local player from the save after its equipment or look
 * changed: its set-up and skills, its models (the changed ones freed and
 * loaded again at once), its town character, and its hunter card in both
 * member lists. */
void Town_ReloadLocalPlayer(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];

    if (TownReload_CheckStub(0) == 1) {
        TownReload_MarkStub(1);
    } else {
        Loading_Reset();
    }
    Player_SetupFromSave(&gPlayers[gScene.localPlayer], &gSave);
    Player_CalcSkills(&gPlayers[gScene.localPlayer]);
    Equip_Refresh(&gSave);
    Player_LoadFromSave(pl);
    TownPc_FreeChangedModels(pl);
    Mem_Compact();
    gTownSyncWait = 3;
    TownPc_Spawn(gScene.localPlayer, (NetUser *) gMyUserId, gMyName);
    TownPc_LoadModelNow(pl);
    gLobbyTextDirty = 1;
    HunterCard_Build(gNetUsers[pl->index].text);
    HunterCard_Build(gLobby->members[pl->index].text);
    TownReload_MarkStub(0);
}

/* Draws a counter's list of n label bars from x to the screen's right
 * edge, 36 apart from y 60: the cursor's line `cur` in `color`, the others
 * grey. With no lines, the bars are drawn empty. The colour parameter is
 * reassigned each row, its first value kept in curColor: the game's code
 * keeps the colour in the register it came in. */
void Menu_DrawLabelList(s16 x, char **lines, s16 n, s16 cur, u32 color)
{
    u32 curColor = color;
    s16 y;
    s16 i;
    s16 w;
    s32 sel;

    Font_SetSpacing(20, 20);
    w = 620 - x;
    for (y = 60, i = 0; i < n; y += 36, i++) {
        if (i == cur) {
            color = curColor;
            sel = 1;
        } else {
            color = 0xFF505050;
            sel = 0;
        }
        if (lines == NULL) {
            Menu_DrawLabelBar(x, y, w, color, NULL, sel, i);
        } else {
            Menu_DrawLabelBar(x, y, w, color, *lines, sel, i);
            lines++;
        }
    }
}

extern s8 gNoticeCooldown;
extern u8 gTownActionDelay;

/* Counts the town's short timers down: the notice's cooldown, the delay
 * before the next action, the wait for the members' sync. */
void Town_TickTimers(void)
{
    if (gNoticeCooldown != 0) {
        gNoticeCooldown -= 1;
    }
    if (gTownActionDelay != 0) {
        gTownActionDelay -= 1;
    }
    if (gTownSyncWait != 0) {
        gTownSyncWait -= 1;
    }
}

extern u16 gTownPad;
extern s32 gDelayedSeTimer;
extern s32 gDelayedSeId;
extern Em *gDelayedSeNpc;
extern s32 gTownMode;
s32 func_905930(Em *e); /* gm_sub's GmEm_Task, the game overlay's runner of an Em. Named by its address: nethttp, which overlaps gm_sub's addresses and is loaded with the lobby, reads the lobby's symbol files, and a name at 0x905930 would split one of its functions */
struct DrawSlot;

/* The town's frame (the village's too): the timers and the music, the
 * members' sync (in a town or a room), the townspeople's spawns, the
 * player count and the town host (gLobby->hostId: the lowest user id of
 * the members present), a sound a townsperson asked for after a delay,
 * the players, the townspeople and other Ems, Town_UpdateMode, the
 * camera, lights and objects, the HUD (but in TOWN_MODE_EXIT), and the
 * loaded players' draw slots. */
void Town_Frame(void)
{
    Em *em = gEms;
    u32 i;
    NetUser *user;
    Player *pl;
    u32 j;
    Player *drawn;

    Town_TickTimers();
    Bgm_TownUpdate();
    gTownPad = LobbyPad_Get(0);
    if (Lobby_GetPlace() != LOBBY_PLACE_LEVEL0) {
        TownPc_SyncMembers();
    }
    Npc_SpawnArea();
    memset(gLobby->hostId, 0, LOBBY_USER_ID_LEN);
    gScene.playerCount = 0;
    for (i = 0, user = gNetUsers, pl = gPlayers; i < LOBBY_MEMBER_COUNT; i++, user++, pl++) {
        if (user->id[0] != 0 && pl->area != 0) {
            s8 *lowestId;

            gScene.playerCount++;
            lowestId = gLobby->hostId;
            if (*lowestId == 0 || 0 > memcmp(lowestId, user->id, LOBBY_USER_ID_LEN)) {
                memcpy(gLobby->hostId, user->id, LOBBY_USER_ID_LEN);
            }
        }
    }
    if (gDelayedSeTimer != 0) {
        if (--gDelayedSeTimer == 0 && gDelayedSeNpc != NULL) {
            Em_PlayEmBankSe(gDelayedSeNpc, gDelayedSeId, &gDelayedSeNpc->posX, 2);
        }
    }
    Save_RestorePouch(&gPlayers[gScene.localPlayer]);
    Notice_Update();
    TownPc_UpdateAll();
    gNpcScene.ticked = 0;
    for (j = 0; j < EM_COUNT; j++, em++) {
        if (em->active != 0) {
            Em_CopyBoneMatrices(em);
            /* an Em the game's runner owns (ownTask 0) runs through
             * GmEm_Task; a townsperson through Npc_Update */
            if (em->ownTask == 0) {
                if (func_905930(em) == 0) {
                    Em_UpdateMatrix(em);
                    Em_CarryRiders(em);
                }
            } else if (Npc_Update(em) == 0) {
                Npc_UpdateMatrix(em);
            }
        }
    }
    Players_UpdateMatrices();
    Players_RunEquipEffects();
    LockOn_Update();
    Town_UpdateMode();
    CamCtl_Update(0);
    LightSet_UpdateAll();
    StageObj_UpdateAll();
    Obj_UpdateAll();
    Area_UpdateTimers();
    switch (gTownMode) {
    case TOWN_MODE_GALLERY:
        Hud_Update();
        break;
    case TOWN_MODE_EXIT:
        break;
    default:
        Hud_Update();
        break;
    }
    LobbyMenu_AddHudLayers();
    for (j = 0, drawn = gPlayers; j < LOBBY_MEMBER_COUNT; j++, drawn++) {
        if (drawn->active != 0 && drawn->visible != 0 && TownPc_IsLoaded(j) != 0) {
            drawn->drawSlot->pos[0] = drawn->pos[0];
            drawn->drawSlot->pos[1] = drawn->pos[1];
            drawn->drawSlot->pos[2] = drawn->pos[2];
            OT_AddByDepth(gDrawOt32, drawn->drawSlot, DRAW_OT32_LEN, 0);
        }
    }
    LobbyBgm_StepPlay();
}

/* 1 while the town's mode holds the player (gLobbyMenu->redraw). */
s32 Town_IsPlayerHeld(void)
{
    return gLobbyMenu->redraw != 0;
}

/* 1 when the player is free in the town (TOWN_MODE_FREE), with no quest
 * taken, and on the lobby server. */
s32 Town_IsIdleOnline(void)
{
    if (gTownMode != TOWN_MODE_FREE) {
        return 0;
    }
    if (gLobby->questTaken != 0) {
        /* the game's code tests gTownMode again here, both ways returning 0 */
        if (gTownMode != TOWN_MODE_FREE) {
            return 0;
        }
        return 0;
    }
    return gLobby->connected == 1;
}

/* In the player's own online house, free in it: once a second member is
 * in the player's slot (a guest), sends the hunter card's parts that
 * changed (gLobbyTextDirty). The member slots are read as bytes past
 * gLobby (LobbyState.memberSlot, '0' + the slot): indexed through the
 * struct, the compiler loads them differently. */
void MyHouse_RefreshGuests(void)
{
    s32 i;
    s32 guests;
    u8 *lobby;
    u8 mySlot;

    if (gTownMode == TOWN_MODE_FREE) {
        guests = 0;
        lobby = (u8 *)gLobby;
        mySlot = gScene.localPlayer;
        for (i = 0; i < LOBBY_MEMBER_COUNT; i++, lobby++) {
            if (i != mySlot && lobby[5432] == (s8)(mySlot + '0')) {
                guests++;
            }
        }
        if (guests != 0 && gLobbyTextDirty != 0) {
            HunterCard_SendChanged();
            gLobbyTextDirty = 0;
        }
    }
}

s32 Kbd_IsOpen(void); /* unit's own declaration: the definition returns s8; with it,
 * Town_UpdateMode's tests of the result compile differently */
void Kbd_Cancel(void);
extern s16 gMenuOffsetY;
extern s16 gMenuOffsetX;
extern s16 gHouseCloseTimer;

/* In an online house: in the player's own, MyHouse_RefreshGuests. In
 * another's, when its owner is gone or no longer lets visitors in
 * (HunterCard.houseAccess), the player stands up, a notice says so, and
 * 300 frames (10 s) later the menus close and the player is sent to
 * AREA_HUB_0x89. Offline the timer is off. */
void MyHouse_CheckHost(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    s8 slot;
    NetUser *owner;
    HunterCard *card;

    if (Game_IsOnline() == 0) {
        gHouseCloseTimer = -1;
        return;
    }
    switch (gScene.area) {
    case AREA_ONLINE_HOUSE_0:
    case AREA_ONLINE_HOUSE_1:
    case AREA_ONLINE_HOUSE_2:
        if (MyHouse_InOwnHouse() == 1) {
            MyHouse_RefreshGuests();
            return;
        }
        break;
    default:
        return;
    }
    slot = MyHouse_LocalSlot();
    owner = &gNetUsers[slot];
    card = (HunterCard *)owner->text;
    if (slot == -1) {
        if (gHouseCloseTimer == -1) {
            gHouseCloseTimer = 300;
            if (TownPc_IsSeatedState(pl->stateGroup, pl->state) == 1) {
                Actor_SetState(pl, 0, TOWN_PC_STAND_UP);
            }
            Menu_PushNotice(LOBBY_NOTICE_HOUSE_CLOSED);
        }
    } else if (owner->id[0] == 0 || card->houseAccess == 0) {
        if (gHouseCloseTimer == -1) {
            gHouseCloseTimer = 300;
            if (TownPc_IsSeatedState(pl->stateGroup, pl->state) == 1) {
                Actor_SetState(pl, 0, TOWN_PC_STAND_UP);
            }
            Menu_PushNotice(LOBBY_NOTICE_HOUSE_CLOSED);
        }
    } else {
        gHouseCloseTimer = -1;
    }
    if (gHouseCloseTimer != -1 && --gHouseCloseTimer <= 0) {
        GameMenu_Clear();
        gScene.menuOpen = 0;
        if (Kbd_IsOpen()) {
            Kbd_Cancel();
        }
        EquipPreview_Free();
        gMenuOffsetY = 0;
        gFieldControl = 1;
        gMenuOffsetX = 0;
        AreaMenu_Open(AREA_HUB_0x89);
        return;
    }
}

extern u8 gChatToLobby;
extern u8 gLobbyLevel1Sel;

/* Online, once a second (every 30 frames): sends each part of the
 * player's hunter card (gPlayerText.card) that no longer matches the save,
 * by its offset in the card: the friend limit; in a town, also the chat's
 * target (the lobby's chat or the town's), the title and the symbol's
 * colour (255 with no style bits). */
void Town_UpdateTextParts(void)
{
    HunterCard *card = &gPlayerText.card;
    u8 color;

    if (Game_IsOnline() != 0 && gFrameTick % 30 == 0) {
        if (card->friendLimit != gSave.friendLimit) {
            HunterCard_SendPart(CARD_PART_FRIEND_LIMIT, 1);
        }
        if (gLobbyLevel1Sel != 0) {
            if (gChatWindowSel == CHAT_SEND_LOBBY) {
                gChatToLobby = 1;
            } else {
                gChatToLobby = 0;
            }
            if (card->chatToLobby != gChatToLobby) {
                HunterCard_SendPart(CARD_PART_CHAT_TO_LOBBY, 1);
            }
            if (card->title != gSave.title) {
                HunterCard_SendPart(CARD_PART_TITLE, 1);
            }
            if (gSave.styleBits == 0) {
                color = 255;
            } else {
                color = gSave.symbolColor;
            }
            if (card->symbolColor != color) {
                HunterCard_SendPart(CARD_PART_SYMBOL_COLOR, 1);
            }
        }
    }
}

extern u8 gLobbyLevel2Sel;
extern u8 gOffersRefreshPending;
extern u8 gTownCamEndOnClose;
extern u8 gMemberNamesOff;
extern s16 gSpotModeTimer;
extern s8 gQuestMinMembers;
extern u16 gLobbyQuestId;
extern s32 gTownMenuOpen;
extern s8 gTownModeStep;
extern s8 gHouseAccess;

/* Runs the town's mode (gTownMode) for a frame. Away from the area menu
 * it first checks the online house's host, sends the hunter card's
 * changed parts and refreshes the townspeople's offers when asked. While
 * the mode holds the player (gLobbyMenu->redraw), the chat keyboard and
 * the game menu are closed after it. */
void Town_UpdateMode(void)
{
    u16 pad;
    LevelEntry *room;
    Player *pl;
    s32 j;

    pad = LobbyPad_Get(0);
    room = LobbyLevel2_Entry((s16)(gLobbyLevel2Sel - 1));
    pl = &gPlayers[gScene.localPlayer];
    if (gTownMode != TOWN_MODE_AREA_MENU) {
        MyHouse_CheckHost();
        Town_UpdateTextParts();
        if (gOffersRefreshPending != 0 && gTownMode == TOWN_MODE_FREE) {
            Npc_RefreshOffers();
            gOffersRefreshPending = 0;
        }
    }
    switch (gTownMode) {
    case TOWN_MODE_FREE:
    case 15:
    case TOWN_MODE_AREA_CHANGE:
    case TOWN_MODE_SCENE:
    case TOWN_MODE_QUIT_DONE:
    case TOWN_MODE_SLEEP:
    case TOWN_MODE_CONTEST_SEATED:
    case TOWN_MODE_DIG:
    default:
        if (gTownMode == TOWN_MODE_FREE) {
            if (Kbd_IsOpen() != 0) {
                gTownMode = TOWN_MODE_KEYBOARD;
            } else if ((pad & PAD_START) && gScene.menuOpen == 0) {
                GameMenu_Open();
            } else if ((pad & PAD_R1) && gLobby->questTaken == 0 && Game_IsOnline() == 1 && gLobbyLevel1Sel != 0) {
                gMemberNamesOff ^= 1;
                gMemberNamesOffSaved = gMemberNamesOff;
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        }
        break;
    case TOWN_MODE_QUEST_PARTY:
        gLobbyMenu->redraw = 1;
        if (gLobby->questPaid != 0) {
            Town_CheckQuestParty();
        }
        break;
    case TOWN_MODE_ARENA_MENU:
        ArenaMenu_Update();
        break;
    case TOWN_MODE_SHIP_MENU:
        ShipMenu_Update();
        break;
    case TOWN_MODE_MOVE_MENU:
        MoveMenu_Update();
        break;
    case TOWN_MODE_DOJO:
        DojoMenu_Update();
        break;
    case TOWN_MODE_AREA_BOARD:
        AreaBoard_Update();
        break;
    case TOWN_MODE_GROUP_BOARD:
        GroupBoard_Update();
        break;
    case TOWN_MODE_MEAL_SCENE:
        MealScene_Update();
        break;
    case TOWN_MODE_NPC_GIFT:
        NpcGift_Update();
        break;
    case TOWN_MODE_AREA_MENU:
        AreaMenu_Update();
        break;
    case TOWN_MODE_HAIR_MENU:
        HairMenu_Update();
        break;
    case TOWN_MODE_ELDER_FORECAST:
        ElderForecast_Update();
        break;
    case TOWN_MODE_BAIT_MENU:
        BaitMenu_Update();
        break;
    case TOWN_MODE_SONG_MENU:
        SongMenu_Update();
        break;
    case TOWN_MODE_PRICE_INFO:
        PriceInfo_Update();
        break;
    case TOWN_MODE_OFFER_MENU:
        OfferMenu_Update();
        break;
    case TOWN_MODE_GALLERY:
        Gallery_Run();
        break;
    case TOWN_MODE_CONTEST:
        Contest_Update();
        break;
    case TOWN_MODE_POOGIE_OUTSIDE:
        PoogieMenu_UpdateOutside();
        break;
    case TOWN_MODE_DRINK_CONTEST:
        DrinkContest_Update();
        break;
    case TOWN_MODE_MINE:
        Mine_UpdateMenu();
        break;
    case TOWN_MODE_ARENA_CUP:
        ArenaCup_Update();
        break;
    case TOWN_MODE_QUEST_DESK:
        QuestDesk_Update();
        break;
    case TOWN_MODE_OFFICIAL_DESK:
        OfficialDesk_Update();
        break;
    case TOWN_MODE_WAIT_CANCEL:
        TownMode_WaitCancel();
        break;
    case TOWN_MODE_INGREDIENTS:
        IngredientList_Update();
        break;
    case TOWN_MODE_KITCHEN:
        Kitchen_Update();
        break;
    case TOWN_MODE_KITCHEN_GUEST:
        KitchenGuest_Update();
        break;
    case TOWN_MODE_VILLAGE_BOARD:
        VillageBoard_Update();
        break;
    case TOWN_MODE_TOWN_GUIDE:
        TownGuide_Update();
        break;
    case TOWN_MODE_RECORDS:
        Records_Update();
        break;
    case TOWN_MODE_KEYBOARD:
        if (Kbd_IsOpen() == 0) {
            gTownMode = TOWN_MODE_FREE;
            j = gScene.localPlayer;
            gPlayers[j].input.held = gPlayers[j].input.held & ~(PAD_CIRCLE | PAD_SQUARE);
            gPlayers[j].target = 0;
        }
        break;
    case TOWN_MODE_HOUSE_ACCESS:
        HouseAccess_Update();
        break;
    case TOWN_MODE_ROOM_LEAVE:
        gLobbyMenu->redraw = 1;
        gTownMode = TOWN_MODE_ROOM_LEAVING;
        gTownMenuOpen = 1;
        gLobby->subStep = 0;
        gTownModeStep = 0;
        break;
    case TOWN_MODE_ROOM_LEAVING:
        gLobbyMenu->redraw = 1;
        if (LobbyStep_Level2Leave() == 1) {
            gTownMode = TOWN_MODE_MESSAGE_WAIT;
        }
        break;
    case TOWN_MODE_MESSAGE_WAIT:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            gFieldControl = 1;
            gTownMode = TOWN_MODE_FREE;
            gTownMenuOpen = 0;
            gPlayers[gScene.localPlayer].input.pressed = 0;
            if (pl->state != TOWN_PC_SIT_DOWN && gTownCamEndOnClose == 1) {
                CamFocus_End();
                CamReq_Cancel();
            }
            LobbyMenu_SetDraw(0, 0, NULL, 0);
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        break;
    case TOWN_MODE_DEPART:
        gTownMode = TOWN_MODE_DEPART_FADE;
        Stream_FadeOut(0, 15); /* frames */
        break;
    case TOWN_MODE_DEPART_FADE:
        if ((u8)FadeA_GetState() != 1) {
            gTownMode = TOWN_MODE_DEPARTED;
            LobbyMenu_SetDraw(0, 0, NULL, 0);
            Hud_StartQuest();
        }
        break;
    case TOWN_MODE_QUIT:
        gTownMode = TOWN_MODE_QUIT_FADE;
        Stream_FadeOut(0, 15); /* frames */
        break;
    case TOWN_MODE_QUIT_FADE:
        if ((u8)FadeA_GetState() != 1) {
            gTownMode = TOWN_MODE_QUIT_DONE;
            LobbyMenu_SetDraw(0, 0, NULL, 0);
            Hud_StartQuest();
        }
        break;
    case TOWN_MODE_GAME_MENU:
        if (GameMenu_Poll() == 1) {
            gScene.menuOpen = 0;
            gTownMode = TOWN_MODE_FREE;
            LobbyMenu_Init();
            j = gScene.localPlayer;
            gPlayers[j].input.held = gPlayers[j].input.held & ~(PAD_CIRCLE | PAD_SQUARE);
            gPlayers[j].target = 0;
        }
        break;
    case TOWN_MODE_SET_OUT_ASK:
        if (gQuestMinMembers + 1 > 1 && gLobby->roomMembers + 1 == room->users) {
            gLobbyMenu->redraw = 0;
            gFieldControl = 1;
            gTownMenuOpen = 0;
            gTownMode = TOWN_MODE_FREE;
        } else {
            gLobbyMenu->redraw = 1;
            switch (Menu_YesNoInput()) {
            case MENU_DONE:
                switch (Quest_CheckOrder(gLobbyQuestId)) {
                case QUEST_ORDER_OK:
                    gFieldControl = 1;
                    gTownMenuOpen = 0;
                    gTownMode = TOWN_MODE_SET_OUT_PAID;
                    pl->questStatus |= QUEST_STATUS_SET_OUT;
                    gHouseAccess = 0;
                    gChatToLobby = 0;
                    HunterCard_SendChanged();
                    TownMsg_SendStatus();
                    break;
                case QUEST_ORDER_WRONG_TIME:
                    LobbyText_Open(LOBBY_TEXT_GATE_CLOSED);
                    gFieldControl = 0;
                    gTownMode = TOWN_MODE_GATE_MESSAGE_WAIT;
                    gTownMenuOpen = 1;
                    break;
                default:
                    LobbyText_Open(LOBBY_TEXT_GATE_ORDER_TERMS);
                    gFieldControl = 0;
                    gTownMode = TOWN_MODE_GATE_MESSAGE_WAIT;
                    gTownMenuOpen = 1;
                    break;
                }
                break;
            case MENU_CANCEL:
                gTownMenuOpen = 0;
                gTownMode = TOWN_MODE_FREE;
                gFieldControl = 1;
                break;
            }
        }
        break;
    case TOWN_MODE_SET_OUT_ASK_FULL:
        if (gQuestMinMembers + 1 > 1 && gLobby->roomMembers + 1 < room->users) {
            gLobbyMenu->redraw = 0;
            gFieldControl = 1;
            gTownMenuOpen = 0;
            gTownMode = TOWN_MODE_FREE;
        } else {
            gLobbyMenu->redraw = 1;
            switch (Menu_YesNoInput()) {
            case MENU_DONE:
                switch (Quest_CheckOrder(gLobbyQuestId)) {
                case QUEST_ORDER_OK:
                    gFieldControl = 1;
                    gTownMenuOpen = 0;
                    gTownMode = TOWN_MODE_SET_OUT_PAID;
                    pl->questStatus |= QUEST_STATUS_SET_OUT;
                    gHouseAccess = 0;
                    gChatToLobby = 0;
                    HunterCard_SendChanged();
                    TownMsg_SendStatus();
                    break;
                case QUEST_ORDER_WRONG_TIME:
                    LobbyText_Open(LOBBY_TEXT_GATE_CLOSED);
                    gFieldControl = 0;
                    gTownMode = TOWN_MODE_GATE_MESSAGE_WAIT;
                    gTownMenuOpen = 1;
                    break;
                default:
                    LobbyText_Open(LOBBY_TEXT_GATE_ORDER_TERMS);
                    gFieldControl = 0;
                    gTownMode = TOWN_MODE_GATE_MESSAGE_WAIT;
                    gTownMenuOpen = 1;
                    break;
                }
                break;
            case MENU_CANCEL:
                gTownMenuOpen = 0;
                gTownMode = TOWN_MODE_FREE;
                gFieldControl = 1;
                break;
            }
        }
        break;
    case TOWN_MODE_SEAT_WAIT:
    case TOWN_MODE_CONTEST_SEAT_WAIT:
        if (--gSpotModeTimer <= 0) {
            Actor_ReturnToIdle(&gPlayers[gScene.localPlayer], 0, 20, 0);
            gTownMode = TOWN_MODE_FREE;
            gTownMenuOpen = 0;
        }
        break;
    case TOWN_MODE_ITEM_SHOP:
        ItemShop_Update();
        break;
    case TOWN_MODE_COMBINE_SHOP:
        CombShop_Update();
        break;
    case TOWN_MODE_EQUIP_SHOP:
        EquipShop_Update();
        break;
    case TOWN_MODE_WORKSHOP:
        Workshop_Update();
        break;
    case TOWN_MODE_SAVE:
        TownSave_Update();
        break;
    case TOWN_MODE_QUEST_COUNTER:
        QuestCounter_Step();
        break;
    case TOWN_MODE_SET_OUT_JOIN:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_SetOut()) {
        case LOBBY_STEP_DONE:
            Chat_SystemMsg(LOBBY_EVENT_QUEST_READY, 0);
            gPlayers[gScene.localPlayer].questStatus |= QUEST_STATUS_SET_OUT;
            gHouseAccess = 0;
            gChatToLobby = 0;
            HunterCard_SendChanged();
            gTownMode = TOWN_MODE_SET_OUT;
            break;
        case LOBBY_STEP_FAILED:
            gTownMode = TOWN_MODE_MESSAGE_WAIT;
            LobbyDialog_OpenWait(gLobby->errText);
            break;
        }
        break;
    case TOWN_MODE_SET_OUT_VERSUS:
        gLobbyMenu->redraw = 1;
    case TOWN_MODE_SET_OUT_PAID:
        switch (LobbyStep_SetOut()) {
        case LOBBY_STEP_DONE:
            gTownMode = TOWN_MODE_SET_OUT;
            Quest_SendPaid();
            break;
        case LOBBY_STEP_FAILED:
            gTownMode = TOWN_MODE_MESSAGE_WAIT;
            LobbyDialog_OpenWait(gLobby->errText);
            break;
        }
        break;
    case TOWN_MODE_SET_OUT_CANCEL_ASK:
        gLobbyMenu->redraw = 1;
        switch (gTownModeStep) {
        case 0:
            gTownModeStep++;
            break;
        case 1:
            switch (Menu_YesNoInput()) {
            case MENU_DONE:
                gTownModeStep = 0;
                gTownMode = TOWN_MODE_SET_OUT_CANCEL;
                LobbyText_Open(LOBBY_TEXT_ROOM_LEAVING);
                break;
            case MENU_CANCEL:
                gTownMenuOpen = 0;
                gTownModeStep = 0;
                gFieldControl = 1;
                gTownMode = TOWN_MODE_SET_OUT;
                break;
            }
            break;
        }
        break;
    case TOWN_MODE_SET_OUT_CANCEL:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_SetOutLeave()) {
        case LOBBY_STEP_DONE:
            gFieldControl = 1;
            gTownMenuOpen = 0;
            pl->questStatus = pl->questStatus & ~QUEST_STATUS_SET_OUT;
            pl->questStatus |= QUEST_STATUS_TAKEN;
            HunterCard_SendChanged();
            gTownMode = TOWN_MODE_FREE;
            break;
        case LOBBY_STEP_FAILED:
            gTownMode = TOWN_MODE_SET_OUT_MESSAGE_WAIT;
            break;
        }
        break;
    case TOWN_MODE_SET_OUT_MESSAGE_WAIT:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            gFieldControl = 1;
            gTownMode = TOWN_MODE_SET_OUT;
            gTownMenuOpen = 0;
        }
        break;
    case TOWN_MODE_PC_CARD:
        if (PcCard_Update(pad) == 1) {
            gTownMode = TOWN_MODE_FREE;
            gTownMenuOpen = 0;
            LobbyMenu_SetDraw(0, 0, NULL, 0);
            gPlayers[gScene.localPlayer].target = 0;
        }
        break;
    case TOWN_MODE_GATE_MESSAGE_WAIT:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            gFieldControl = 1;
            gTownMenuOpen = 0;
            gTownMode = TOWN_MODE_FREE;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        break;
    case TOWN_MODE_TALK:
    case 5:
        Talk_Finish();
        break;
    case TOWN_MODE_BOX_OPEN:
        gTownMode = TOWN_MODE_BOX_MENU;
        break;
    case TOWN_MODE_BOX_MENU:
        if (BoxMenu_Update(pad) == 0) {
            gTownMode = TOWN_MODE_FREE;
            gTownMenuOpen = 0;
        }
        break;
    case TOWN_MODE_EXIT_ASK:
        gTownMode = TOWN_MODE_EXIT_YES_NO;
        gLobbyMenu->redraw = 1;
        break;
    case TOWN_MODE_EXIT_YES_NO:
        gLobbyMenu->redraw = 1;
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            LobbyText_Open(LOBBY_TEXT_TOWN_LEAVING);
            gTownMode = TOWN_MODE_EXIT;
            gLobby->seqStep = 0;
            gLobby->spareStep0 = 0;
            gLobby->subStep2 = 0;
            gLobby->subStep = 0;
            break;
        case MENU_CANCEL:
            gTownMenuOpen = 0;
            gTownMode = TOWN_MODE_FREE;
            gFieldControl = 1;
            break;
        }
        break;
    case TOWN_MODE_EXIT:
        switch (Town_ModeDisconnect()) {
        case LOBBY_STEP_DONE:
            gLobby->connected = 1;
            LobbyVillage_BeginReturn(0);
            LobbyText_Open(LOBBY_TEXT_TOWN_LEAVING);
            gLobbyMenu->redraw = 1;
            break;
        }
        break;
    case TOWN_MODE_POOGIE_PET:
        PoogieNaming_Update();
        break;
    case TOWN_MODE_POOGIE_MENU:
        PoogieMenu_Update();
        break;
    case TOWN_MODE_POOGIE_CARE:
        PoogieMenu_UpdateCare();
        break;
    case TOWN_MODE_HOUSE_PET:
        HousePet_Update();
        break;
    case TOWN_MODE_SET_OUT:
    case TOWN_MODE_DEPARTED:
        break;
    }
    if (gLobbyMenu->redraw != 0) {
        if (Kbd_IsOpen() != 0) {
            Kbd_Cancel();
        }
        GameMenu_Clear();
        gScene.menuOpen = 0;
    }
}

/* TOWN_MODE_EXIT: leaves the town for the village, the player held. Step
 * by step (gTownModeStep): the file loads are cancelled and the player is
 * off the lobby server; the town (level 1) is left, back at level 0; the
 * level-0 list is fetched again; the screen fades out for the village's
 * return (LOBBY_ENTER_VILLAGE_RETURN). A failed leave of the town keeps the
 * player there with a notice; a failed list fetch ends the lobby
 * (LOBBY_QUIT_FAILED). Returns LOBBY_STEP_DONE once the fade is over (the
 * house's access and the chat's target reset and sent), else
 * LOBBY_STEP_BUSY. */
s32 Town_ModeDisconnect(void)
{
    gLobbyMenu->redraw = 1;
    switch (gTownModeStep) {
    case 0:
        FileQueue_Cancel();
        gLobby->connected = 0;
        Kbd_Cancel();
        LobbyMenu_SetDraw(0, 0, NULL, 0);
        gTownModeStep++;
    case 1:
        switch (LobbyStep_Level1Leave()) {
        case LOBBY_STEP_DONE:
            Lobby_SetPlace(LOBBY_PLACE_LEVEL0);
            gTownModeStep++;
            gLobby->seqStep = 0;
            break;
        case LOBBY_STEP_FAILED:
            gTownMode = TOWN_MODE_FREE;
            Menu_PushNoticeOnce(LOBBY_NOTICE_TOWN_LEAVE_FAILED);
            break;
        }
        break;
    case 2:
        switch (LobbyStep_Level0List()) {
        case LOBBY_STEP_DONE:
            gTownModeStep++;
            break;
        case LOBBY_STEP_FAILED:
            Lobby_Disconnect(LOBBY_QUIT_FAILED);
            break;
        }
        break;
    case 3:
        switch (LobbyScreen_FadeOut(LOBBY_ENTER_VILLAGE_RETURN)) {
        case LOBBY_STEP_DONE:
            gTownModeStep = 0;
            gHouseAccess = 0;
            gChatToLobby = 0;
            HunterCard_SendChanged();
            return LOBBY_STEP_DONE;
        }
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
    return LOBBY_STEP_BUSY;
}

extern s8 gTownSpawnSet;
extern u8 gLockOnNoCycle;
extern SpawnPt gTownSpawns84[];
extern u8 gTavernSpawn[];
extern u8 gTownArenaSpawn[];
extern u8 gElderHallSpawn[];
extern SpawnPt gTownSpawn89Alt;
extern SpawnPt gTownSpawn89;
extern SpawnPt gTownSpawn89Early;
extern u8 gTownSpawn83[];
extern SpawnPt gTownSpawn57Alt;
extern SpawnPt gTownSpawn57;
u8 Area_GetSetIndex(s32 id); /* unit's own declaration: declared with the s32 the caller passes; with the definition's u16, Town_PlaceLocalPlayer compiles one instruction differently */

/* Puts the local player in `area`, out of any seat, offer or drink: at
 * its start position, or, unless the spawn was already set
 * (gTownSpawnSet), at the area's entry point. The village's areas take
 * theirs from gSpawnPoints by the area's number (a quarter turn added to
 * the facing); AREA_HUB_0x84 one of five at random; AREA_HUB_0x89 and
 * AREA_HUB_0x57 one by the system settings' offline flag, or, in
 * AREA_HUB_0x89 before the village's first story step (Cond_Check 1), an
 * early one, with the first Em (gEms) as the player's target, the menus
 * held and the lock-on kept on it (gLockOnNoCycle). The halls and AREA_ONLINE_VILLAGE have
 * one each, declared as bytes and read through a cast. The player then
 * goes back to idle, except during a scene (TOWN_MODE_SCENE). */
void Town_PlaceLocalPlayer(s32 area)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    SpawnPt *spawn;
    Vec3f *pos;
    s32 k;

    pl->drunk = 0;
    pl->drinkCount = 0;
    pl->spot = NULL;
    pl->offerNoticeTimer = 0;
    pl->weaponSeMute = 0;
    *(Vec3f *)pl->pos = *(Vec3f *)pl->startPos;
    pl->rot[1] = pl->targetYaw = pl->startAngle;
    pos = (Vec3f *)pl->pos;
    if (gTownSpawnSet == 0) {
        switch ((u16)area) {
        default:
            k = Area_GetSetIndex(area);
            *(Vec3f *)pl->pos = gSpawnPoints[k].pos;
            pl->rot[1] = pl->targetYaw = gSpawnPoints[k].angle + ANGLE_TURN / 4;
            break;
        case AREA_HUB_0x84:
            spawn = &gTownSpawns84[(u16)Rand_Next(1) % 5];
            *(Vec3f *)pl->pos = spawn->pos;
            pl->rot[1] = pl->targetYaw = spawn->angle;
            break;
        case AREA_HUB_0x89:
            if (Cond_Check(1) == 0) {
                pl->target = gEms;
                spawn = &gTownSpawn89Early;
                gTownMenuOpen = 1;
                gLockOnNoCycle = 1;
            } else if (gSysSettings.offline == 1) {
                spawn = &gTownSpawn89Alt;
            } else {
                spawn = &gTownSpawn89;
            }
            *(Vec3f *)pl->pos = spawn->pos;
            pl->rot[1] = pl->targetYaw = spawn->angle;
            break;
        case AREA_ONLINE_VILLAGE:
            spawn = (SpawnPt *)gTownSpawn83;
            *pos = spawn->pos;
            pl->rot[1] = pl->targetYaw = spawn->angle;
            break;
        case AREA_TAVERN:
            spawn = (SpawnPt *)gTavernSpawn;
            *pos = spawn->pos;
            pl->rot[1] = pl->targetYaw = spawn->angle;
            break;
        case AREA_TOWN_ARENA:
            spawn = (SpawnPt *)gTownArenaSpawn;
            *pos = spawn->pos;
            pl->rot[1] = pl->targetYaw = spawn->angle;
            break;
        case AREA_ELDER_HALL:
            spawn = (SpawnPt *)gElderHallSpawn;
            *pos = spawn->pos;
            pl->rot[1] = pl->targetYaw = spawn->angle;
            break;
        case AREA_HUB_0x57:
            if (gSysSettings.offline == 1) {
                spawn = &gTownSpawn57Alt;
            } else {
                spawn = &gTownSpawn57;
            }
            *(Vec3f *)pl->pos = spawn->pos;
            pl->rot[1] = pl->targetYaw = spawn->angle;
            break;
        }
    }
    if (Game_IsOnline() == 0 && Cond_Check(1) == 1) {
        pl->target = NULL;
    }
    if (gTownMode != TOWN_MODE_SCENE) {
        Actor_ReturnToIdle(pl, 0, 0, 0);
    }
}

/* Turns the area's sky drawing on or off. */
void Town_SetDrawSky(s8 enabled)
{
    gAreaState.drawSky = enabled;
}

/* The file's data: the town's entry points, by area (Town_PlaceLocalPlayer). */
SpawnPt gTownSpawns84[5] = {
#include "lobby/0080DC70.inc"
};
u8 gTavernSpawn[16] = {
#include "lobby/0080DCC0.inc"
};
u8 gTownArenaSpawn[16] = {
#include "lobby/0080DCD0.inc"
};
u8 gElderHallSpawn[16] = {
#include "lobby/0080DCE0.inc"
};
SpawnPt gTownSpawn89Alt = {
#include "lobby/0080DCF0.inc"
};
SpawnPt gTownSpawn89 = {
#include "lobby/0080DD00.inc"
};
SpawnPt gTownSpawn89Early = {
#include "lobby/0080DD10.inc"
};
u8 gTownSpawn83[16] = {
#include "lobby/0080DD20.inc"
};
SpawnPt gTownSpawn57Alt = {
#include "lobby/0080DD30.inc"
};
SpawnPt gTownSpawn57 = {
#include "lobby/0080DD40.inc"
};
