/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
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
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void *memset(void *, s32, u32);
void *memcpy(void *, const void *, u32);

extern LobbyMenu *gLobbyMenu;

u16 LobbyPad_Get(s32 a) {
    return gLobby->buttons;
}

extern u8 gTownSyncWait;
extern u8 gLobbyTextDirty;
extern NetUser gNetUsers[];
extern char gMyUserId[];
extern char gMyName[];
s32 func_3D54C0(s32 a);
void func_3D54D0(s32 a);
void Loading_Reset(void);
void Player_SetupFromSave(Player *p, SaveData *save);
void Player_CalcSkills(Player *p);
void Equip_Refresh(SaveData *save);
void Player_LoadFromSave(Player *p);
void TownPc_FreeChangedModels(Player *p);
void Mem_Compact(void);
void TownPc_Spawn(u8 pl, NetUser *u, char *name);
void TownPc_LoadModelNow(Player *p);
void HunterCard_Build(void *out);

void Town_ReloadLocalPlayer(void)
{
    Player *me = &gPlayers[gScene.localPlayer];

    if (func_3D54C0(0) == 1) {
        func_3D54D0(1);
    } else {
        Loading_Reset();
    }
    Player_SetupFromSave(&gPlayers[gScene.localPlayer], &gSave);
    Player_CalcSkills(&gPlayers[gScene.localPlayer]);
    Equip_Refresh(&gSave);
    Player_LoadFromSave(me);
    TownPc_FreeChangedModels(me);
    Mem_Compact();
    gTownSyncWait = 3;
    TownPc_Spawn(gScene.localPlayer, (NetUser *) gMyUserId, gMyName);
    TownPc_LoadModelNow(me);
    gLobbyTextDirty = 1;
    HunterCard_Build(gNetUsers[me->index].text);
    HunterCard_Build(gLobby->members[me->index].text);
    func_3D54D0(0);
}

void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 sel, u8 kind);
void Font_SetSpacing(u8 a, u8 b);

/* The colour parameter is reassigned each row (its first value kept in c):
 * the game keeps it in t0, the register it came in. */
void Menu_DrawLabelList(s16 x, char **lines, s16 n, s16 cur, u32 color)
{
    u32 c = color;
    s16 y;
    s16 i;
    s16 w;
    s32 sel;

    Font_SetSpacing(20, 20);
    w = 0x26C - x;
    for (y = 60, i = 0; i < n; y += 36, i++) {
        if (i == cur) {
            color = c;
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
extern u8 gTownSyncWait;
void Town_TickTimers(void) {
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
void Bgm_Update(void);
s32 Lobby_GetPlace3928(void);
void TownPc_SyncMembers(void);
void Npc_SpawnArea(void);
s32 memcmp(void *a, void *b, s32 n);
void Em_PlaySeSlot6(Em *em, s32 id, void *pos, s32 b);
void Save_RestorePouch(Player *p);
void Notice_Update(void);
void TownPc_UpdateAll(void);
void Em_CopyBoneMatrices(Em *e);
s32 func_905930(Em *e);
void Em_UpdateMatrix(Em *e);
void Em_CarryRiders(Em *e);
s32 Npc_Update(Em *a);
void Npc_UpdateMatrix(Em *npc);
void Players_UpdateMatrices(void);
void Players_RunEquipEffects(void);
void LockOn_Update(void);
void Town_UpdateMode(void);
void CamCtl_Update(s32);
void LightSet_UpdateAll(void);
void StageObj_UpdateAll(void);
void Obj_UpdateAll(void);
void Area_UpdateTimers(void);
void Hud_Update(void);
void LobbyMenu_AddHudLayers(void);
s32 TownPc_IsLoaded(s8 pl);
struct DrawSlot;
s32 OT_AddByDepth(u32 *table, struct DrawSlot *node, u32 n, s32 clamp);
s32 LobbyBgm_StepPlay(void);

/* the village lobby's frame: members, NPCs, objects, then the players' draw slots
   (gLobby->hostId ends up with the lowest member id) */
void Town_Frame(void)
{
    Em *e = gEms;
    u32 i;
    NetUser *u;
    Player *p;
    u32 j;
    Player *q;

    Town_TickTimers();
    Bgm_Update();
    gTownPad = LobbyPad_Get(0);
    if (Lobby_GetPlace3928() != 0) {
        TownPc_SyncMembers();
    }
    Npc_SpawnArea();
    memset(gLobby->hostId, 0, 8);
    gScene.playerCount = 0;
    for (i = 0, u = gNetUsers, p = gPlayers; i < 10; i++, u++, p++) {
        if (u->id[0] != 0 && p->area != 0) {
            s8 *low;

            gScene.playerCount++;
            low = gLobby->hostId;
            if (*low == 0 || 0 > memcmp(low, u->id, 8)) {
                memcpy(gLobby->hostId, u->id, 8);
            }
        }
    }
    if (gDelayedSeTimer != 0) {
        if (--gDelayedSeTimer == 0 && gDelayedSeNpc != NULL) {
            Em_PlaySeSlot6(gDelayedSeNpc, gDelayedSeId, &gDelayedSeNpc->posX, 2);
        }
    }
    Save_RestorePouch(&gPlayers[gScene.localPlayer]);
    Notice_Update();
    TownPc_UpdateAll();
    gTutorial.ticked = 0;
    for (j = 0; j < 20; j++, e++) {
        if (e->active != 0) {
            Em_CopyBoneMatrices(e);
            if (*((u8 *)e + 0x1E) == 0) {
                if (func_905930(e) == 0) {
                    Em_UpdateMatrix(e);
                    Em_CarryRiders(e);
                }
            } else if (Npc_Update((Em *)((u8 *) e)) == 0) {
                Npc_UpdateMatrix((Em *)((u8 *) e));
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
    case 47:
        Hud_Update();
        break;
    case 14:
        break;
    default:
        Hud_Update();
        break;
    }
    LobbyMenu_AddHudLayers();
    for (j = 0, q = gPlayers; j < 10; j++, q++) {
        if (q->active != 0 && q->visible != 0 && TownPc_IsLoaded(j) != 0) {
            ((f32 *)q->drawSlot)[2] = q->pos[0];
            ((f32 *)q->drawSlot)[3] = q->pos[1];
            ((f32 *)q->drawSlot)[4] = q->pos[2];
            OT_AddByDepth(gDrawOt32, q->drawSlot, 0x20, 0);
        }
    }
    LobbyBgm_StepPlay();
}

s32 Town_IsPlayerHeld(void) {
    return gLobbyMenu->redraw != 0;
}

extern s32 gTownMode;

s32 Town_IsIdleOnline(void)
{
    if (gTownMode != 0) {
        return 0;
    }
    if (gLobby->questTaken != 0) {
        /* the original tests gTownMode again here, both ways returning 0 */
        if (gTownMode != 0) {
            return 0;
        }
        return 0;
    }
    return gLobby->connected == 1;
}

s32 LobbyText_Update(void);
extern u8 gLobbyTextDirty;

void MyHouse_RefreshGuests(void)
{
    s32 i;
    s32 n;
    u8 *p;
    u8 me;

    if (gTownMode == 0) {
        n = 0;
        p = (u8 *)gLobby;
        me = gScene.localPlayer;
        for (i = 0; i < 10; i++, p++) {
            if (i != me && p[0x1538] == (s8)(me + 0x30)) {
                n++;
            }
        }
        if (n != 0 && gLobbyTextDirty != 0) {
            LobbyText_Update();
            gLobbyTextDirty = 0;
        }
    }
}

void AreaMenu_Open(u16 id);
void GameMenu_Clear(void);
void Actor_SetState(void *, s32, s32);
s32 MyHouse_InOwnHouse(void);
s8 MyHouse_LocalSlot(void);
s32 TownPc_IsSeatedState(u8 arg0, u8 arg1);
void Menu_PushNotice(s32 msg);
s32 Kbd_IsOpen(void); /* unit's own declaration: the definition (sub_main) returns s8;
 * Town_UpdateMode is 805 words off with s8 */
void Kbd_Cancel(void);
void StageModels_Free(void);
s32 Game_IsOnline(void);
extern s16 gMenuOffsetY;
extern s16 gMenuOffsetX;
extern s16 gHouseCloseTimer;
extern NetUser gNetUsers[];

void MyHouse_CheckHost(void)
{
    Player *me = &gPlayers[gScene.localPlayer];
    s8 slot;
    NetUser *rec;
    HunterCard *info;

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
    rec = &gNetUsers[slot];
    info = (HunterCard *)rec->text;
    if (slot == -1) {
        if (gHouseCloseTimer == -1) {
            gHouseCloseTimer = 300;
            if (TownPc_IsSeatedState(me->stateGroup, me->state) == 1) {
                Actor_SetState(me, 0, 0x15);
            }
            Menu_PushNotice(0x24);
        }
    } else if (rec->id[0] == 0 || info->houseAccess == 0) {
        if (gHouseCloseTimer == -1) {
            gHouseCloseTimer = 300;
            if (TownPc_IsSeatedState(me->stateGroup, me->state) == 1) {
                Actor_SetState(me, 0, 0x15);
            }
            Menu_PushNotice(0x24);
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
        StageModels_Free();
        gMenuOffsetY = 0;
        gFieldControl = 1;
        gMenuOffsetX = 0;
        AreaMenu_Open(AREA_HUB_0x89);
        return;
    }
}

void LobbyText_UpdatePart(u16 off, u16 n);
s32 Game_IsOnline(void);
extern u8 D_008BEB75;
extern u8 gLobbyLevel1Sel;

void Town_UpdateTextParts(void)
{
    HunterCard *s = &gPlayerText.card;
    u8 v;

    if (Game_IsOnline() != 0 && gFrameTick % 30 == 0) {
        if (s->restriction != gSave.restriction) {
            LobbyText_UpdatePart(0xF, 1);
        }
        if (gLobbyLevel1Sel != 0) {
            if (gChatWindowSel == 0) {
                D_008BEB75 = 1;
            } else {
                D_008BEB75 = 0;
            }
            if (s->unkD7 != D_008BEB75) {
                LobbyText_UpdatePart(0xD7, 1);
            }
            if (s->title != gSave.title) {
                LobbyText_UpdatePart(0xE, 1);
            }
            if (gSave.styleBits == 0) {
                v = 0xFF;
            } else {
                v = gSave.symbolColor;
            }
            if (s->symbolColor != v) {
                LobbyText_UpdatePart(0xC, 1);
            }
        }
    }
}

#define FLAG39 (gLobbyMenu->redraw)
void *LobbyLevel2_Entry(s32 arg0);
void MyHouse_CheckHost(void);
void Town_UpdateTextParts(void);
void Npc_RefreshOffers(void);
s32 Menu_YesNoInput(void);
s32 LobbyStep_6504(void);
s32 LobbyStep_6504Leave(void);
s32 Quest_CheckOrder(u16 id);
s32 BoxMenu_Update(u16 pad);
s32 GameMenu_Poll(void);
s32 PcCard_Update(u16 pad);
s32 Town_ModeDisconnect(void);
s32 LobbyStep_6501(void);
void LobbySe_Play(s32 se);
void TownMsg_SendStatus(void);
void Actor_SetIdle(void *, s32 a1, s32 n, s32 m);
void Chat_SystemMsg(s32 msg, char *arg);
void LobbyDialog_OpenWait(void *arg0);
void LobbyText_Open(s32 msg);
void LobbyVillage_Begin3(s32 arg0);
void GameMenu_Open(void);

void CamFocus_End(void);
void CamReq_Cancel(void);
u8 FadeA_GetState(void);
void Stream_FadeOut(s32 ch, s32 frames);
void Hud_StartQuest(void);
void LobbyMenu_Init(void);
void Town_CheckQuestParty(void);
void ArenaMenu_Update(void);
void PickMenu_Update(void);
void MoveMenu_Update(void);
void DojoMenu_Update(void);
void AreaBoard_Update(void);
void GroupBoard_Update(void);
void MealScene_Update(void);
void Menu4E_Update(void);
void AreaMenu_Update(void);
void HairMenu_Update(void);
void ElderForecast_Update(void);
void BaitMenu_Update(void);
void SongMenu_Update(void);
void PriceInfo_Update(void);
void OfferMenu_Update(void);
void Gallery_Run(void);
void Contest_Update(void);
void Menu31_Update(void);
void DrinkContest_Update(void);
void Mine_UpdateMenu(void);
void ArenaCup_Update(void);
void QuestDesk_Update(void);
void OfficialDesk_Update(void);
void lobby_entry(void);
void IngredientList_Update(void);
void Kitchen_Update(void);
void KitchenGuest_Update(void);
void VillageBoard_Update(void);
void TownGuide_Update(void);
void Records_Update(void);
void HouseAccess_Update(void);
void ItemShop_Update(void);
void CombShop_Update(void);
void EquipShop_Update(void);
void Workshop_Update(void);
s32 TownSave_Update(void);
void QuestCounter_Step(void);
void Quest_SendPaid(void);
s32 Talk_Finish(void);
void PoogiePet_Update(void);
void PoogieMenu_Update(void);
void PoogieMenu_UpdateCare(void);
void Menu13_Update(void);
extern u8 gLobbyLevel2Sel;
extern u8 gOffersRefreshPending;
extern u8 D_008BEB63;
extern u8 gMemberNamesOff;
extern s16 gSpotModeTimer;
extern s8 gQuestMinMembers;
extern u16 gLobbyQuestId;
extern s32 gTownMenuOpen;
extern s8 gTownModeStep;
extern s8 gHouseAccess;

/* The town's per-frame update: runs the current mode (gTownMode), then the
 * common end of frame while the mode holds the player (flag 0x39). */
void Town_UpdateMode(void)
{
    u16 pad;
    void *room;
    Player *pl;
    s32 j;

    pad = LobbyPad_Get(0);
    room = LobbyLevel2_Entry((s16)(gLobbyLevel2Sel - 1));
    pl = &gPlayers[gScene.localPlayer];
    if (gTownMode != 0x3A) {
        MyHouse_CheckHost();
        Town_UpdateTextParts();
        if (gOffersRefreshPending != 0 && gTownMode == 0) {
            Npc_RefreshOffers();
            gOffersRefreshPending = 0;
        }
    }
    switch (gTownMode) {
    case 0:
    case 0xF:
    case 0x15:
    case 0x22:
    case 0x25:
    case 0x26:
    case 0x37:
    case 0x4D:
    default:
        if (gTownMode == 0) {
            if (Kbd_IsOpen() != 0) {
                gTownMode = 0x1A;
            } else if ((pad & 0x8000) && gScene.menuOpen == 0) {
                GameMenu_Open();
            } else if ((pad & 0x80) && gLobby->questTaken == 0 && Game_IsOnline() == 1 && gLobbyLevel1Sel != 0) {
                gMemberNamesOff ^= 1;
                gMemberNamesOffSaved = gMemberNamesOff;
                LobbySe_Play(1);
            }
        }
        break;
    case 0x40:
        FLAG39 = 1;
        if (gLobby->questPaid != 0) {
            Town_CheckQuestParty();
        }
        break;
    case 0x41:
        ArenaMenu_Update();
        break;
    case 0x4C:
        PickMenu_Update();
        break;
    case 0x3B:
        MoveMenu_Update();
        break;
    case 0x3F:
        DojoMenu_Update();
        break;
    case 0x3D:
        AreaBoard_Update();
        break;
    case 0x3E:
        GroupBoard_Update();
        break;
    case 0x4F:
        MealScene_Update();
        break;
    case 0x4E:
        Menu4E_Update();
        break;
    case 0x3A:
        AreaMenu_Update();
        break;
    case 0x3C:
        HairMenu_Update();
        break;
    case 0x44:
        ElderForecast_Update();
        break;
    case 0x2C:
        BaitMenu_Update();
        break;
    case 0x38:
        SongMenu_Update();
        break;
    case 0x2D:
        PriceInfo_Update();
        break;
    case 0x43:
        OfferMenu_Update();
        break;
    case 0x2F:
        Gallery_Run();
        break;
    case 0x30:
        Contest_Update();
        break;
    case 0x31:
        Menu31_Update();
        break;
    case 0x32:
        DrinkContest_Update();
        break;
    case 0x33:
        Mine_UpdateMenu();
        break;
    case 0x34:
        ArenaCup_Update();
        break;
    case 0x35:
        QuestDesk_Update();
        break;
    case 0x45:
        OfficialDesk_Update();
        break;
    case 0x2E:
        lobby_entry();
        break;
    case 0x47:
        IngredientList_Update();
        break;
    case 0x48:
        Kitchen_Update();
        break;
    case 0x49:
        KitchenGuest_Update();
        break;
    case 0x4A:
        VillageBoard_Update();
        break;
    case 0x4B:
        TownGuide_Update();
        break;
    case 0x46:
        Records_Update();
        break;
    case 0x1A:
        if (Kbd_IsOpen() == 0) {
            gTownMode = 0;
            j = gScene.localPlayer;
            gPlayers[j].input.held = gPlayers[j].input.held & ~0x220;
            gPlayers[j].target = 0;
        }
        break;
    case 0x2B:
        HouseAccess_Update();
        break;
    case 0x16:
        FLAG39 = 1;
        gTownMode = 0x17;
        gTownMenuOpen = 1;
        gLobby->subStep = 0;
        gTownModeStep = 0;
        break;
    case 0x17:
        FLAG39 = 1;
        if (LobbyStep_6501() == 1) {
            gTownMode = 0x18;
        }
        break;
    case 0x18:
        FLAG39 = 1;
        if (pad & 0x20) {
            gFieldControl = 1;
            gTownMode = 0;
            gTownMenuOpen = 0;
            gPlayers[gScene.localPlayer].input.pressed = 0;
            if (pl->state != 0x14 && D_008BEB63 == 1) {
                CamFocus_End();
                CamReq_Cancel();
            }
            LobbyMenu_SetDraw(0, 0, NULL, 0);
            LobbySe_Play(8);
        }
        break;
    case 0x1F:
        gTownMode = 0x20;
        Stream_FadeOut(0, 0xF);
        break;
    case 0x20:
        if ((u8)FadeA_GetState() != 1) {
            gTownMode = 0x21;
            LobbyMenu_SetDraw(0, 0, NULL, 0);
            Hud_StartQuest();
        }
        break;
    case 0x23:
        gTownMode = 0x24;
        Stream_FadeOut(0, 0xF);
        break;
    case 0x24:
        if ((u8)FadeA_GetState() != 1) {
            gTownMode = 0x25;
            LobbyMenu_SetDraw(0, 0, NULL, 0);
            Hud_StartQuest();
        }
        break;
    case 0xB:
        if (GameMenu_Poll() == 1) {
            gScene.menuOpen = 0;
            gTownMode = 0;
            LobbyMenu_Init();
            j = gScene.localPlayer;
            gPlayers[j].input.held = gPlayers[j].input.held & ~0x220;
            gPlayers[j].target = 0;
        }
        break;
    case 0x1E:
        if (gQuestMinMembers + 1 > 1 && gLobby->roomMembers + 1 == *(u16 *)((u8 *)room + 6)) {
            FLAG39 = 0;
            gFieldControl = 1;
            gTownMenuOpen = 0;
            gTownMode = 0;
        } else {
            FLAG39 = 1;
            switch (Menu_YesNoInput()) {
            case 0:
                switch (Quest_CheckOrder(gLobbyQuestId)) {
                case 0:
                    gFieldControl = 1;
                    gTownMenuOpen = 0;
                    gTownMode = 6;
                    pl->questStatus |= 0x20;
                    gHouseAccess = 0;
                    D_008BEB75 = 0;
                    LobbyText_Update();
                    TownMsg_SendStatus();
                    break;
                case 1:
                    LobbyText_Open(0x82);
                    gFieldControl = 0;
                    gTownMode = 0x39;
                    gTownMenuOpen = 1;
                    break;
                default:
                    LobbyText_Open(0x83);
                    gFieldControl = 0;
                    gTownMode = 0x39;
                    gTownMenuOpen = 1;
                    break;
                }
                break;
            case 3:
                gTownMenuOpen = 0;
                gTownMode = 0;
                gFieldControl = 1;
                break;
            }
        }
        break;
    case 0x2A:
        if (gQuestMinMembers + 1 > 1 && gLobby->roomMembers + 1 < *(u16 *)((u8 *)room + 6)) {
            FLAG39 = 0;
            gFieldControl = 1;
            gTownMenuOpen = 0;
            gTownMode = 0;
        } else {
            FLAG39 = 1;
            switch (Menu_YesNoInput()) {
            case 0:
                switch (Quest_CheckOrder(gLobbyQuestId)) {
                case 0:
                    gFieldControl = 1;
                    gTownMenuOpen = 0;
                    gTownMode = 6;
                    pl->questStatus |= 0x20;
                    gHouseAccess = 0;
                    D_008BEB75 = 0;
                    LobbyText_Update();
                    TownMsg_SendStatus();
                    break;
                case 1:
                    LobbyText_Open(0x82);
                    gFieldControl = 0;
                    gTownMode = 0x39;
                    gTownMenuOpen = 1;
                    break;
                default:
                    LobbyText_Open(0x83);
                    gFieldControl = 0;
                    gTownMode = 0x39;
                    gTownMenuOpen = 1;
                    break;
                }
                break;
            case 3:
                gTownMenuOpen = 0;
                gTownMode = 0;
                gFieldControl = 1;
                break;
            }
        }
        break;
    case 0x19:
    case 0x36:
        if (--gSpotModeTimer <= 0) {
            Actor_SetIdle(&gPlayers[gScene.localPlayer], 0, 0x14, 0);
            gTownMode = 0;
            gTownMenuOpen = 0;
        }
        break;
    case 0x2:
        ItemShop_Update();
        break;
    case 0xA:
        CombShop_Update();
        break;
    case 0x8:
        EquipShop_Update();
        break;
    case 0x9:
        Workshop_Update();
        break;
    case 0x14:
        TownSave_Update();
        break;
    case 0x1:
        QuestCounter_Step();
        break;
    case 0x1B:
        FLAG39 = 1;
        switch (LobbyStep_6504()) {
        case 0:
            Chat_SystemMsg(9, 0);
            gPlayers[gScene.localPlayer].questStatus |= 0x20;
            gHouseAccess = 0;
            D_008BEB75 = 0;
            LobbyText_Update();
            gTownMode = 7;
            break;
        case 1:
            gTownMode = 0x18;
            LobbyDialog_OpenWait(gLobby->errText);
            break;
        }
        break;
    case 0x42:
        FLAG39 = 1;
    case 0x6:
        switch (LobbyStep_6504()) {
        case 0:
            gTownMode = 7;
            Quest_SendPaid();
            break;
        case 1:
            gTownMode = 0x18;
            LobbyDialog_OpenWait(gLobby->errText);
            break;
        }
        break;
    case 0x27:
        FLAG39 = 1;
        switch (gTownModeStep) {
        case 0:
            gTownModeStep++;
            break;
        case 1:
            switch (Menu_YesNoInput()) {
            case 0:
                gTownModeStep = 0;
                gTownMode = 0x28;
                LobbyText_Open(0x77);
                break;
            case 3:
                gTownMenuOpen = 0;
                gTownModeStep = 0;
                gFieldControl = 1;
                gTownMode = 7;
                break;
            }
            break;
        }
        break;
    case 0x28:
        FLAG39 = 1;
        switch (LobbyStep_6504Leave()) {
        case 0:
            gFieldControl = 1;
            gTownMenuOpen = 0;
            pl->questStatus = pl->questStatus & ~0x20;
            pl->questStatus |= 0x40;
            LobbyText_Update();
            gTownMode = 0;
            break;
        case 1:
            gTownMode = 0x29;
            break;
        }
        break;
    case 0x29:
        FLAG39 = 1;
        if (pad & 0x20) {
            gFieldControl = 1;
            gTownMode = 7;
            gTownMenuOpen = 0;
        }
        break;
    case 0x3:
        if (PcCard_Update(pad) == 1) {
            gTownMode = 0;
            gTownMenuOpen = 0;
            LobbyMenu_SetDraw(0, 0, NULL, 0);
            gPlayers[gScene.localPlayer].target = 0;
        }
        break;
    case 0x39:
        FLAG39 = 1;
        if (pad & 0x20) {
            gFieldControl = 1;
            gTownMenuOpen = 0;
            gTownMode = 0;
            LobbySe_Play(8);
        }
        break;
    case 0x4:
    case 0x5:
        Talk_Finish();
        break;
    case 0x1C:
        gTownMode = 0x1D;
        break;
    case 0x1D:
        if (BoxMenu_Update(pad) == 0) {
            gTownMode = 0;
            gTownMenuOpen = 0;
        }
        break;
    case 0xC:
        gTownMode = 0xD;
        FLAG39 = 1;
        break;
    case 0xD:
        FLAG39 = 1;
        switch (Menu_YesNoInput()) {
        case 0:
            LobbyText_Open(0x1A);
            gTownMode = 0xE;
            gLobby->seqStep = 0;
            gLobby->unk393C = 0;
            gLobby->subStep2 = 0;
            gLobby->subStep = 0;
            break;
        case 3:
            gTownMenuOpen = 0;
            gTownMode = 0;
            gFieldControl = 1;
            break;
        }
        break;
    case 0xE:
        switch (Town_ModeDisconnect()) {
        case 0:
            gLobby->connected = 1;
            LobbyVillage_Begin3(0);
            LobbyText_Open(0x1A);
            FLAG39 = 1;
            break;
        }
        break;
    case 0x10:
        PoogiePet_Update();
        break;
    case 0x11:
        PoogieMenu_Update();
        break;
    case 0x12:
        PoogieMenu_UpdateCare();
        break;
    case 0x13:
        Menu13_Update();
        break;
    case 7:
    case 0x21:
        break;
    }
    if (FLAG39 != 0) {
        if (Kbd_IsOpen() != 0) {
            Kbd_Cancel();
        }
        GameMenu_Clear();
        gScene.menuOpen = 0;
    }
}

void FileQueue_Cancel(void);
s32 Menu_PushNoticeOnce(s32);
s32 LobbyStep_6408(void);
s32 LobbyScreen_FadeOut(s32);
void Lobby_SetPlace3928(s32 arg0);
void Lobby_Disconnect(s32);
s32 LobbyStep_Level0List(void);
void Kbd_Cancel(void);
extern s8 gTownModeStep;
extern s8 gHouseAccess;
extern u8 D_008BEB75;

s32 Town_ModeDisconnect(void)
{
    *(s8 *)((u8 *)gLobbyMenu + 0x39) = 1;
    switch (gTownModeStep) {
    case 0:
        FileQueue_Cancel();
        gLobby->connected = 0;
        Kbd_Cancel();
        LobbyMenu_SetDraw(0, 0, NULL, 0);
        gTownModeStep++;
    case 1:
        switch (LobbyStep_6408()) {
        case 0:
            Lobby_SetPlace3928(0);
            gTownModeStep++;
            gLobby->seqStep = 0;
            break;
        case 1:
            gTownMode = 0;
            Menu_PushNoticeOnce(0x12);
            break;
        }
        break;
    case 2:
        switch (LobbyStep_Level0List()) {
        case 0:
            gTownModeStep++;
            break;
        case 1:
            Lobby_Disconnect(5);
            break;
        }
        break;
    case 3:
        switch (LobbyScreen_FadeOut(3)) {
        case 0:
            gTownModeStep = 0;
            gHouseAccess = 0;
            D_008BEB75 = 0;
            LobbyText_Update();
            return 0;
        }
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
    return 2;
}

typedef struct Vec3f {
    f32 x, y, z;
} Vec3f;
typedef struct SpawnPt {
    Vec3f pos;
    u16 angle;
    u16 pad;
} SpawnPt;
extern s8 gTownSpawnSet;
extern s32 gTownMenuOpen;
extern u8 gLockOnNoCycle;
extern SpawnPt gSpawnPoints[];
extern SpawnPt gTownSpawns84[];
extern u8 gTownSpawn86[];
extern u8 gTownSpawn87[];
extern u8 gTownSpawn88[];
extern SpawnPt gTownSpawn89Alt;
extern SpawnPt gTownSpawn89;
extern SpawnPt gTownSpawn89Early;
extern u8 gTownSpawn83[];
extern SpawnPt gTownSpawn57Alt;
extern SpawnPt gTownSpawn57;
s32 Cond_Check(u16 id);
u16 Rand_Next(s32 stream);
void Actor_SetIdle(void *, s32 a1, s32 n, s32 m);
u8 Area_GetSetIndex(s32 id); /* unit's own declaration: with the definition's u8 (u16 id) (lobby/unk_79EA30.c), Town_PlaceLocalPlayer 1 of 222 words differ */

/* Puts the local player at the entry point of the area (SpawnPt tables;
   some are declared as bytes and read through a cast). (&p->index)[1] is
   the u16 angle at 0xE (player.h has bytes there); the Em at 0x3B4 is the
   one the player follows in the tutorial. */
void Town_PlaceLocalPlayer(s32 area)
{
    Player *p = &gPlayers[gScene.localPlayer];
    SpawnPt *sp;
    Vec3f *pos;
    s32 k;

    p->unk844 = 0;
    p->unk867 = 0;
    p->spot = NULL;
    p->offerNoticeTimer = 0;
    p->weaponSeMute = 0;
    *(Vec3f *)p->pos = *(Vec3f *)p->startPos;
    p->rot[1] = (&p->index)[1] = p->startAngle;
    pos = (Vec3f *)p->pos;
    if (gTownSpawnSet == 0) {
        switch ((u16)area) {
        default:
            k = Area_GetSetIndex(area);
            *(Vec3f *)p->pos = gSpawnPoints[k].pos;
            p->rot[1] = (&p->index)[1] = gSpawnPoints[k].angle + 0x4000;
            break;
        case AREA_HUB_0x84:
            sp = &gTownSpawns84[(u16)Rand_Next(1) % 5];
            *(Vec3f *)p->pos = sp->pos;
            p->rot[1] = (&p->index)[1] = sp->angle;
            break;
        case AREA_HUB_0x89:
            if (Cond_Check(1) == 0) {
                ((Em **)p)[0x3B4 / 4] = gEms;
                sp = &gTownSpawn89Early;
                gTownMenuOpen = 1;
                gLockOnNoCycle = 1;
            } else if (gSysSettings.offline == 1) {
                sp = &gTownSpawn89Alt;
            } else {
                sp = &gTownSpawn89;
            }
            *(Vec3f *)p->pos = sp->pos;
            p->rot[1] = (&p->index)[1] = sp->angle;
            break;
        case AREA_HUB_0x83:
            sp = (SpawnPt *)gTownSpawn83;
            *pos = sp->pos;
            p->rot[1] = (&p->index)[1] = sp->angle;
            break;
        case AREA_HUB_0x86:
            sp = (SpawnPt *)gTownSpawn86;
            *pos = sp->pos;
            p->rot[1] = (&p->index)[1] = sp->angle;
            break;
        case AREA_HUB_0x87:
            sp = (SpawnPt *)gTownSpawn87;
            *pos = sp->pos;
            p->rot[1] = (&p->index)[1] = sp->angle;
            break;
        case AREA_HUB_0x88:
            sp = (SpawnPt *)gTownSpawn88;
            *pos = sp->pos;
            p->rot[1] = (&p->index)[1] = sp->angle;
            break;
        case AREA_HUB_0x57:
            if (gSysSettings.offline == 1) {
                sp = &gTownSpawn57Alt;
            } else {
                sp = &gTownSpawn57;
            }
            *(Vec3f *)p->pos = sp->pos;
            p->rot[1] = (&p->index)[1] = sp->angle;
            break;
        }
    }
    if (Game_IsOnline() == 0 && Cond_Check(1) == 1) {
        ((Em **)p)[0x3B4 / 4] = NULL;
    }
    if (gTownMode != 0x22) {
        Actor_SetIdle(p, 0, 0, 0);
    }
}

void func_007A6460(s8 arg0) {
    gAreaState.drawSky = arg0;
}

/* unk_7A44D0's .data (0x80DC70-0x80DD50) */
SpawnPt gTownSpawns84[5] = {
#include "lobby/0080DC70.inc"
};
u8 gTownSpawn86[16] = {
#include "lobby/0080DCC0.inc"
};
u8 gTownSpawn87[16] = {
#include "lobby/0080DCD0.inc"
};
u8 gTownSpawn88[16] = {
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
