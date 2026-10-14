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


s32 Player_IsLocal(void *obj);
void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);
void Actor_SetStateEx(Player *obj, u8 state, u8 sub, u16 flags);
void Actor_SetAction(void *a, u16 mode, s32 b, s32 c);
void TownPc_SetG1FromTable(Player *arg0);
void Actor_SetStateNow(Player *obj, u8 state, u8 sub, u16 flags);
s32 Motion_ReachedFrame(void *, s32, f32);
void Player_SetMoveMode(Player *p, u16 mode, s16 flag1, s16 flag2);
void TownMsg_Send0AAll(Player *);
void Actor_SetIdle(void *obj, s32 a1, s32 n, s32 m);
void Player_ClearFlag(void *obj, u32 flag);
void Player_SetFlag(void *obj, u32 flag);


extern u8 D_00802C60[];
extern u8 D_00802C62[];

void TownPc_IdleCheckOffer(Player *pl)
{
    pl->freeState = 1;
    if (pl->offerRequest != 0) {
        pl->offerStarted = 1;
        Actor_SetStateNow(pl, 0, 0xA, 0x10);
        TownMsg_Send0AAll(pl);
    }
}

void Vec_Copy(f32 *dst, f32 *src);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Vec_Set(f32 *out, f32 x, f32 y, f32 z);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void Actor_StartWalk(Player *obj, s16 n);

/* Put the local player at its action spot's point (+4) facing its angle
 * (+0x14), then walk (Actor_StartWalk) in state 0xE. */
void ActSpot_PlaceLocal(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    f32 m[16];
    f32 pos[3];
    f32 v[3];
    f32 d[3];
    s32 ang[3];
    u16 a;

    pos[0] = ((f32 *)pl->spot)[1];
    pos[1] = ((f32 *)pl->spot)[2];
    pos[2] = ((f32 *)pl->spot)[3];
    a = *(u16 *)((u8 *)pl->spot + 0x14);
    Vec_Copy(pl->moveTo, pos);
    ang[1] = a;
    ang[0] = 0;
    ang[2] = 0;
    Mtx_FromAnglesXYZ(ang, m);
    Vec_Set(v, 0.0f, 0.0f, 0.0f);
    Vec_Rotate(d, v, m);
    pl->moveTo[0] += d[0];
    pl->moveTo[1] += d[1];
    pl->moveTo[2] += d[2];
    *(u16 *)((u8 *)pl + 0xE) = a;
    Actor_StartWalk((Player *)((u8 *) pl), 0x14);
    Actor_SetStateEx((Player *)((u8 *) pl), 0, 0xE, 0);
    Player_SetFlag(pl, 0x20000);
}

s32 Cond_Check(u16 id);
s32 Monster_IsHunted(u16 em);
s32 Save_TestEventFlag(s32 flag);
u8 Village_FacilityLevel(s32 k);

s32 Town_AreaEventPending(void)
{
    switch (gScene.area) {
    case 0xAC:
        if (Save_TestEventFlag(0x89) == 0 && (Village_FacilityLevel(0) & 0xFF) >= 6 && Monster_IsHunted(7U) == 1) {
            return 1;
        }
        break;
    case 0x57:
        if (Save_TestEventFlag(0x8A) == 0 && Cond_Check(0x51U) == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* An action spot in the town (the player's +0x7D0): its id, its kind (the
 * switch in TownPc_Action), where to stand and the angle to face. */
typedef struct TownSpot {
    /* 0x00 */ u16 id;
    /* 0x02 */ u16 kind;
    /* 0x04 */ f32 pos[3];
    /* 0x10 */ u8 pad10[4];
    /* 0x14 */ u16 rot;
} TownSpot;



u16 LobbyPad_Get(s32);
u8 TownPc_MoveKind(Player *a);
u16 TownPc_InputYaw(Player *, s32);
s32 Hud_ItemGetBusy(void);
s32 TownPc_TakeOfferedItem(Player *p);
s32 Notice_OpenTextN(char *text, u32 width, s32 mode, s32 sel, s32 arg4);
void PcCard_Open(void);
void Mem_Fill(void *dst, u8 c, s32 n);
void Npc_PlaySeDelayed(Em *npc, void *p);
s32 Npc_IsAct4For50(Em *arg0);
void PoogieMenu_OpenCare(Em *task);
void Menu13_Open(Em *task);
void PoogiePet_Open(Em *task);
s32 Area_IsOnlineHouse(s32);
s32 Menu_PushNoticeOnce(s32 arg0);
void PoogieMenu_Init(Em *task);
void Menu31_Open(Em *task);
void Npc_PickIdleState(Em *p);
void TownMsg_Send06(Player *pl);
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
void TownMsg_Send1B(Player *arg0);
void TownPc_TakeSpot(void);
s32 MyHouse_IsOwner(void);
void TownMsg_Send15(Player *a);
s32 LobbyTime_IsAm(void);
void DrinkContest_Open(Em *npc);
s32 ActSpot_IsFree(u32 n);
s32 Area_Is86to88(s32 arg0);
void TownMsg_Send04(Player *arg0);
void PriceInfo_Open(void);
void VillageBoard_Open(void);
s32 VillageGather_OverLimit(u8);
void HouseAccess_Open(void);
s32 Quest_CheckOrder(u16 id);
s32 ArenaCup_QuestTaken(void);
void *LobbyLevel2_Entry(s32 arg0);
s32 LobbyTime_IsPm(void);
void TownMsg_Send27(void);
void LobbyText_Open(s32 msg);
void Menu_SetYesNo(s8 arg0);
void Daytime_Set(s32 a);
void Season_Set(u8 a); /* unit's own declaration: with the definition's void (s32 season) (main/main.c), TownPc_Action 1 of 1118 words differ */
s32 LobbyWorld_GetSeason(void);
void LobbyWorld_HoldClock(void);
void NetQuest_SetInfo(u8 *);
void NetQuest_SetUnk4C(s32); /* unit's own declaration: with the definition's void (s8 v) (main/unk_18DA50.c), TownPc_Action 19 of 1118 words differ */
s32 Lobby_EnterRollRare(void);
void PspLink_ClearFlags(void);
void SeqA_Start(s32);
void Save_SetEventFlag(s32 n);
void QuestCounter_Open(void);
void LobbySe_Play(s32 se);
void Records_Open(void);
void TownSave_Open(void);
u16 Box_FindFreeSlot(void *unused0);
void CamReq_TownPc(void);
u16 Box_AddItem(SaveData *save, u8 type, u16 id, u16 level);
void CamReq_Lobby44(s32 n);
void Npc_ResetTalked(void);
s32 BoxMenu_Open(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void BaitMenu_Open(void);
void *memset(void *, s32, u32);

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s32 gNpcKindSe[];
extern u8 gTalkNpcName[];
extern u8 gTownActionDelay;
extern s8 gTakeSpotId;
extern s8 gTownSpotId;
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

/* The local player's actions in the town, per frame: turns it by the input
 * (TownPc_MoveKind); with the action button (0x20 of LobbyPad_Get), talks to
 * the NPC it faces (+0x3B4: the dojo, Poogie and hair menus) or uses the
 * action spot it stands at (+0x7D0: by kind, the quest counter, the box, the
 * kitchen, the farm...); then TownPc_IdleCheckOffer. */
void TownPc_Action(Player *pc)
{
    SaveData *save = &gSave;
    u16 pad;
    Em *npc;
    TownSpot *spot;
    TownSpot *sp2;
    u16 rot;
    s32 r;
    u32 d;
    u8 n;
    f32 pos[3];

    pad = LobbyPad_Get(0);
    switch (TownPc_MoveKind((Player *)pc)) {
    case 0:
        break;
    case 1:
    case 2:
    case 3:
        if ((u32)(pc->state - 3) > 2) {
            if (pc->index == gScene.localPlayer) {
                pc->targetYaw = TownPc_InputYaw((Player *)pc, 0);
            }
            Actor_SetStateEx((Player *)((u8 *) pc), 0, 3, 0);
        }
        break;
    case 5:
        if (pc->state != 0x11 && pc->state != 0x13 && (u32)(pc->state - 4) > 1) {
            if (pc->index == gScene.localPlayer) {
                pc->targetYaw = TownPc_InputYaw((Player *)pc, 0);
            }
            d = (u16)(pc->targetYaw + 0x10000 - pc->rot[1]);
            if (d >= 0x6000 && d <= 0xA000) {
                Actor_SetStateEx((Player *)((u8 *) pc), 0, 0x11, 0);
            } else {
                Actor_SetStateEx((Player *)((u8 *) pc), 0, 4, 0);
            }
        }
        break;
    }
    if (pc->index != gScene.localPlayer) {
        return;
    }
    if (pc->actRequest != 0) {
        TownPc_SetG1FromTable((Player *)pc);
        return;
    }
    if (Hud_ItemGetBusy() == 1) {
        return;
    }
    if ((pc->input.pressed & 0x20) && gTownMode == 0 && TownPc_TakeOfferedItem((Player *)((u8 *) pc)) == 1) {
        return;
    }
    if (pc->target != (u8 *)pc && pc->target != NULL && (pad & 0x20) && gTownMode != 0xB) {
        Notice_OpenTextN(0, 0, 3, 0, 0);
        npc = pc->target;
        if (npc->unk01E == 0 && gTownMode == 0) {
            gTownMenuOpen = 0;
            gTownMode = 3;
            PcCard_Open();
            gLobby->buttons = 0;
        } else if (gTownMode == 0) {
            u8 *w;

            w = &npc->work;
            if (*(s8 *)(w + 2) == 1 || gTownActionDelay != 0) {
                return;
            }
            gLobby->buttons = 0;
            npc->contact = (Player *)pc; /* the player talking to it */
            Mem_Fill(gTalkNpcName, 0, 0x34);
            if (gNpcKindSe[w[0]] != -1) {
                Npc_PlaySeDelayed(npc, (void *)gNpcKindSe[w[0]]);
            }
            if (Npc_IsAct4For50(npc) == 1) {
                if (w[0] == 0x22) {
                    PoogieMenu_OpenCare(npc);
                    return;
                } else if (w[0] == 0x6D) {
                    Menu13_Open(npc);
                    return;
                } else if (w[0] == 0x14) {
                    if (*(s8 *)save->poogieName == 0 && npc->state != 0x58 && w[0x44] == 0) {
                        PoogiePet_Open(npc);
                        return;
                    }
                    if (Area_IsOnlineHouse(gScene.area) == 1 && *(s8 *)save->poogieName != 0) {
                        if (npc->state == 0xB6) {
                            Menu_PushNoticeOnce(0x26);
                        } else {
                            PoogieMenu_Init(npc);
                        }
                        return;
                    }
                    if (*(s8 *)save->poogieName != 0 && gScene.area == 0x89) {
                        Menu31_Open(npc);
                        return;
                    }
                    return;
                }
            } else if (npc->state != 0x43 && npc->state != 0x44) {
                Npc_PickIdleState(npc);
            }
            if (pc->unk844 != 0) {
                pc->unk844 = 0;
                pc->unk867 = 0;
                Actor_SetIdle(pc, 0, -4, 0);
                TownMsg_Send06(pc);
            }
            gTownMenuOpen = 1;
            gTownMode = 4;
            Npc_PickIdleState(npc);
            switch (w[0]) {
            case 0x70:
                CamFocus_Start(npc);
                Talk_Start(npc, 0);
                HairMenu_Open(npc, 1);
                break;
            }
        }
    }
    spot = pc->spot;
    if (spot != NULL && (pad & 0x20) && (gTownMode == 0 || gTownMode == 7)) {
        if (gTownActionDelay != 0) {
            return;
        }
        if (gTownMode == 7 && spot->kind != 0x11) {
            return;
        }
        gLobby->buttons = 0;
        pos[0] = ((TownSpot *)pc->spot)->pos[0];
        pos[1] = ((TownSpot *)pc->spot)->pos[1];
        pos[2] = ((TownSpot *)pc->spot)->pos[2];
        sp2 = pc->spot;
        rot = sp2->rot;
        switch (sp2->kind) {
        default:
            return;
        case 23:
            MoveMenu_Open();
            break;
        case 24:
            switch (sp2->id) {
            case 0xAD:
                AreaMenu_Open((u8)Village_FacilityLevel(4) + 0xAD);
                break;
            default:
                AreaMenu_Open(sp2->id);
                break;
            }
            break;
        case 16:
            if (LobbyLevel0_HasGroups() == 0) {
                AreaBoard_Open();
            } else {
                GroupBoard_Open();
            }
            break;
        case 14:
            pc->unk844 = 0;
            if (Game_IsOnline() == 0) {
                Em *n = Npc_Find(5);

                Actor_SetIdle(pc, 0, -4, 0);
                Actor_SetState(n, 0, 0x6E);
                Contest_Open(n);
            } else {
                gTownSpotId = ((TownSpot *)pc->spot)->id;
                gSpotModeTimer = 0x96;
                gTownMode = 0x36;
                gTownMenuOpen = 1;
                Actor_SetIdle(pc, 0, -4, 0);
                if (memcmp(gLobby->hostId, gLobby->myId, 8) == 0) {
                    TownMsg_SendSelf(2, 0x1B, pc->spot);
                } else {
                    TownMsg_Send1B((Player *)pc);
                }
            }
            break;
        case 10:
            if ((pc->state == 1 || pc->state == 0x1D) && gHouseCloseTimer <= 0) {
                gTakeSpotId = sp2->id;
                gSpotModeTimer = 0x96;
                gTownMode = 0x19;
                gTownMenuOpen = 1;
                if (Game_IsOnline() == 0) {
                    TownPc_TakeSpot();
                } else if (MyHouse_IsOwner() == 1) {
                    TownMsg_SendSelf(2, 0x15, pc->spot);
                } else {
                    TownMsg_Send15(pc);
                }
            }
            break;
        case 1:
            if (Game_IsOnline() == 0 && ((TownSpot *)pc->spot)->kind == 1 && LobbyTime_IsAm() == 0) {
                gTakeSpotId = ((TownSpot *)pc->spot)->id;
                DrinkContest_Open(Npc_Find(0x10));
                break;
            }
            if ((pc->state == 1 || pc->state == 0x1D) && ActSpot_IsFree(((TownSpot *)pc->spot)->id) == 1) {
                if (Area_Is86to88(gScene.area) == 1) {
                    gTakeSpotId = ((TownSpot *)pc->spot)->id;
                    if (memcmp(gLobby->hostId, gLobby->myId, 8) == 0) {
                        gSpotModeTimer = 0x96;
                        gTownMode = 0x19;
                        gTownMenuOpen = 1;
                        TownMsg_SendSelf(2, 4, pc->spot);
                    } else {
                        gSpotModeTimer = 0x96;
                        gTownMode = 0x19;
                        gTownMenuOpen = 1;
                        TownMsg_Send04((Player *)pc);
                    }
                } else {
                    gTakeSpotId = ((TownSpot *)pc->spot)->id;
                    TownPc_TakeSpot();
                }
            }
            return;
        case 12:
            gTownMode = 0x2D;
            gTownMenuOpen = 1;
            PriceInfo_Open();
            return;
        case 15:
            VillageBoard_Open();
            break;
        case 13:
            gPoogieDigTaken = 1;
            Actor_SetStateNow((Player *)((u8 *) pc), 0, 0x80, 0);
            gDigSpotUses[((TownSpot *)pc->spot)->id]++;
            if (gScene.area == 0x89) {
                n = 0;
            } else if (gScene.area == 0x83) {
                n = 1;
            } else {
                return;
            }
            if (VillageGather_OverLimit(n) == 1) {
                gDigOverLimit = 1;
            }
            return;
        case 9:
            if (gLobby->mealEaten == 0) {
                gTakeSpotId = sp2->id;
                TownPc_TakeSpot();
            }
            return;
        case 25:
            HouseAccess_Open();
            break;
        case 17:
            switch (gScene.area) {
            case 0x86:
            case 0x87:
            case 0x88:
                if (gLobby->questTaken != 0) {
                    r = Quest_CheckOrder(gLobbyQuestId);
                    if (gLobby->questPaid == 1) {
                        if (ArenaCup_QuestTaken() == 1) {
                            LobbyLevel2_Entry((s16)(gLobbyLevel2Sel - 1));
                            if (gTownMode != 0x40) {
                                if (LobbyTime_IsPm() == 0) {
                                    Menu_PushNoticeOnce(0x1B);
                                } else if (gLobby->roomMembers == gQuestMinMembers) {
                                    TownMsg_Send27();
                                } else if (gTownMode != 7) {
                                    Menu_PushNoticeOnce(0x1A);
                                }
                            }
                        } else if (r == 0) {
                            if (gTownMode != 6 && gTownMode != 7) {
                                u8 *q = LobbyLevel2_Entry((s16)(gLobbyLevel2Sel - 1));

                                if (gLobby->roomMembers + 1 >= *(u16 *)(q + 6)) {
                                    LobbyText_Open(0x6F);
                                    Menu_SetYesNo(0);
                                    gFieldControl = 0;
                                    gTownMode = 0x2A;
                                    gTownMenuOpen = 1;
                                } else {
                                    LobbyText_Open(0x6D);
                                    Menu_SetYesNo(1);
                                    gFieldControl = 0;
                                    gTownMode = 0x1E;
                                    gTownMenuOpen = 1;
                                }
                            }
                        } else {
                            if (r == 1) {
                                LobbyText_Open(0x82);
                            } else {
                                LobbyText_Open(0x83);
                            }
                            gFieldControl = 0;
                            gTownMode = 0x39;
                            gTownMenuOpen = 1;
                        }
                    } else if (gTownMode == 7) {
                        gTownMenuOpen = 1;
                        gTownMode = 0x27;
                        gTownModeStep = 0;
                        gFieldControl = 0;
                        LobbyText_Open(0x6E);
                        Menu_SetYesNo(1);
                    } else if (r == 2) {
                        LobbyText_Open(0x83);
                        gFieldControl = 0;
                        gTownMode = 0x39;
                        gTownMenuOpen = 1;
                        break;
                    } else {
                        gTownMode = 0x1B;
                        LobbyText_Open(0x78);
                    }
                } else {
                    LobbyText_Open(0x82);
                    gFieldControl = 0;
                    gTownMode = 0x29;
                    gTownMenuOpen = 1;
                }
                return;
            case 0x89:
            case 0x57:
                if (gLobby->questTaken != 0) {
                    if (Quest_CheckOrder(gLobbyQuestId) == 0) {
                        u8 *ql = (u8 *)&gTownQuestLists;

                        gTownMode = 0x1F;
                        if (LobbyTime_IsAm() == 0) {
                            Daytime_Set(4);
                        } else {
                            Daytime_Set(3);
                        }
                        Season_Set(LobbyWorld_GetSeason());
                        LobbyWorld_HoldClock();
                        if (gQuestOfferId != 0) {
                            memset(D_6ADB30, 0, 0x18);
                        } else {
                            NetQuest_SetInfo(ql + LobbyWorld_GetSeason() * 0x18 + 4);
                        }
                        NetQuest_SetUnk4C(Lobby_EnterRollRare());
                        PspLink_ClearFlags();
                        SeqA_Start(1);
                        if (gLobbyQuestId == 0x1771 && Save_TestEventFlag(0x30) == 0) {
                            Save_SetEventFlag(0x30);
                        }
                    } else {
                        LobbyText_Open(0x82);
                        gFieldControl = 0;
                        gTownMode = 0x39;
                        gTownMenuOpen = 1;
                    }
                }
                break;
            }
            return;
        case 3:
            if (gSave.hr != 0) {
                QuestCounter_Open();
            } else {
                Menu_PushNoticeOnce(0x10);
            }
            return;
        case 4:
            gTownMode = 0xC;
            LobbyText_Open(0x7B);
            Menu_SetYesNo(1);
            LobbySe_Play(8);
            gLobbyMenu->redraw = 1;
            gTownMenuOpen = 1;
            gFieldControl = 0;
            break;
        case 26:
            Records_Open();
            break;
        case 5:
            TownSave_Open();
            break;
        case 8:
            if (gScene.area != 0x57) {
                pos[0] = 5075.0f;
                pos[1] = 201.19f;
                pos[2] = 6438.0f;
                rot = 0x72A2;
            }
            Vec_Copy(pc->moveTo, pos);
            pc->targetYaw = rot;
            Actor_StartWalk((Player *)((u8 *) pc), 0x14);
            if (Town_AreaEventPending() == 1) {
                if ((u16)Box_FindFreeSlot(&gSave) == 0xFFFF) {
                    Menu_PushNoticeOnce(0xB);
                } else {
                    Actor_SetStateNow((Player *)((u8 *) pc), 0, 0x1A, 0);
                    if (gScene.area == 0x57) {
                        CamReq_TownPc();
                        gFieldControl = 0;
                        Player_SetFlag((Player *)pc, 0x20000);
                        Save_SetEventFlag(0x8A);
                        Box_AddItem(&gSave, 6, 0xBE, 0);
                    } else {
                        CamReq_Lobby44(0xA);
                        gFieldControl = 0;
                        Save_SetEventFlag(0x89);
                        Box_AddItem(&gSave, 6, 0xA3, 0);
                    }
                    Npc_ResetTalked();
                    gTownMode = 0x26;
                }
            } else {
                Actor_SetStateNow((Player *)((u8 *) pc), 0, 0x19, 0);
                gFieldControl = 0;
                if (gScene.area == 0x57) {
                    Player_SetFlag((Player *)pc, 0x20000);
                } else {
                    CamReq_Lobby44(0xB);
                }
                gTownMode = 0x26;
            }
            break;
        case 6:
            if (BoxMenu_Open() == 1) {
                gTownMenuOpen = 1;
                gTownMode = 0x1C;
                LobbyMenu_SetDraw(0, 0, 0, 0);
            } else {
                LobbySe_Play(9);
            }
            break;
        case 2:
            pc->unk844 = 0;
            if (Game_IsOnline() == 0) {
                Actor_SetIdle(pc, 0, -4, 0);
            }
            BaitMenu_Open();
            break;
        }
    }
    TownPc_IdleCheckOffer(pc);
}

f32 Math_Atan2(f32 y, f32 x);
f32 Vec_Distance(f32 *a, f32 *b);
void Actor_SetState(void *, s32, s32);
extern u16 gTownMoveFrames;

/* An actor walking to the point at +0x754: by its state (+0x15: 1, 3, 4) it
 * turns toward the point, changes state (Actor_SetState) or stops there. */
void TownPc_SteerToDest(Player *pl)
{
    f32 dist;
    f32 y;
    f32 x;
    u8 state;

    if (pl->action != 1) {
        pl->moveTo[1] = pl->pos[1];
        dist = Vec_Distance(pl->pos, pl->moveTo);
        state = pl->state;
        y = pl->moveTo[0] - pl->pos[0];
        x = pl->moveTo[2] - pl->pos[2];
        switch (state) {
        case 0xA:
        case 0xB:
        case 0xC:
        case 0xD:
        case 0x70:
        case 0x71:
        case 0x74:
        case 0xE:
        case 0x1F:
            break;
        default:
        case 0x1:
            if (!(dist <= 150.0f)) {
                Actor_SetState(pl, 0, 4);
                pl->targetYaw = (s16) (s32) (0.5f + ((65536.0f * Math_Atan2(y, x)) / 6.2831855f));
            } else if (!(dist <= 20.0f)) {
                pl->targetYaw = (s16) (s32) (0.5f + ((65536.0f * Math_Atan2(y, x)) / 6.2831855f));
                Actor_SetState(pl, 0, 3);
            }
            break;
        case 0x4:
            if ((pl->moveToFrames == (gTownMoveFrames - 1)) && (dist < 150.0f)) {
                Actor_SetState(pl, 0, 3);
            } else if (dist < 1.0f) {
                pl->moveToFrames = 0;
                Actor_SetState(pl, 0, 6);
            } else {
                pl->targetYaw = (s16) (s32) (0.5f + ((65536.0f * Math_Atan2(y, x)) / 6.2831855f));
            }
            break;
        case 0x3:
            if (dist < 1.0f) {
                pl->moveToFrames = 0;
                Actor_SetState(pl, 0, 1);
                Actor_SetIdle(pl, 0, 4, 0);
            } else if (!(dist <= 150.0f)) {
                pl->targetYaw = (s16) (s32) (0.5f + ((65536.0f * Math_Atan2(y, x)) / 6.2831855f));
                Actor_SetState(pl, 0, 4);
            } else {
                pl->targetYaw = (s16) (s32) (0.5f + ((65536.0f * Math_Atan2(y, x)) / 6.2831855f));
            }
            break;
        }
        Player_SetFlag(pl, 0x20000U);
    }
}

/* Turns the actor with the pad (0x800 left, 0x400 right). */
static void TownPc_TurnByPad(Player *pl)
{
    if (gPadState[0].dir & 0x800) {
        pl->rot[1] = (s32) (pl->rot[1] - 0x400);
        pl->targetYaw = (s16) pl->rot[1];
    }
    if (gPadState[0].dir & 0x400) {
        pl->rot[1] = (s32) (pl->rot[1] + 0x400);
        pl->targetYaw = (s16) pl->rot[1];
    }
}

extern s32 gTownMode;

void TownPc_CheckAction(Player *pl)
{
    if (pl->index == gScene.localPlayer) {
        if (gTownMode == 0x1D) {
            TownPc_TurnByPad(pl);
            return;
        }
        TownPc_Action(pl);
    }
}

void TownPc_G1State00(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0xD2, 4, 0);
        pl->motionTime = 0;
        Player_SetMoveMode(pl, 0U, 0, 0);
        break;
    case 1:
        if (Player_IsLocal(pl) == 1) {
            if ((s32) pl->input.stickTilt >= 0x28) {
                Actor_SetStateNow(pl, 1, 1, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_SetG1FromTable(pl);
            }
        } else {
        }
        break;
    }
}

void TownPc_G1State01(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0xCF, 0, 0);
        pl->motionTime = 0;
        Player_SetMoveMode(pl, 0U, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Actor_SetIdle(pl, 0, 4, 0);
        }
        break;
    }
}

void TownPc_G1State02(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x26C, 6, 0);
        pl->motionTime = 0;
        Actor_SetAction(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= 0x28) {
                Actor_SetStateEx((Player *)((u8 *) pl), 1U, 0x11U, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_SetG1FromTable(pl);
            }
        } else {
        }
        break;
    }
}

void TownPc_G1State03(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x26F, 6, 0);
        pl->motionTime = 0;
        Actor_SetAction(pl, 0, 0, 0);
        break;
    case 1:
        if ((Player_IsLocal(pl) == 1) && (Motion_ReachedFrame(pl, 0, 90.0f) != 0)) {
            if ((s32) pl->input.stickTilt >= 0x28) {
                Actor_SetStateNow(pl, 1, 4, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_SetG1FromTable(pl);
            }
        } else {
        }
        break;
    }
}

void TownPc_G1State04(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x270, 6, 0);
        pl->motionTime = 0;
        Actor_SetAction(pl, 0, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step = (u8) (step + 1);
            Player_SetFlag(pl, 0x20000U);
            Actor_SetMotion(pl, 0xCF, -4, 0x3C);
        }
        break;
    case 2:
        Player_SetFlag(pl, 0x20000U);
        if (pl->motion[0].playing == 0) {
            Player_ClearFlag(pl, 0x20000U);
            Actor_SetIdle(pl, 0, 4, 0);
        }
        break;
    }
}

void TownPc_G1State05(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x274, 6, 0);
        pl->motionTime = 0;
        Actor_SetAction(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= 0x28) {
                Actor_SetStateEx((Player *)((u8 *) pl), 1U, 0x11U, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_SetG1FromTable(pl);
            }
        } else {
        }
        break;
    }
}

void TownPc_G1State06(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x278, 4, 0);
        pl->motionTime = 0;
        Actor_SetAction(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= 0x28) {
                Actor_SetStateEx((Player *)((u8 *) pl), 1U, 0x11U, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_SetG1FromTable(pl);
            }
        } else {
        }
        break;
    }
}

void TownPc_G1State07(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x26E, 4, 0);
        pl->motionTime = 0;
        Actor_SetAction(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= 0x28) {
                Actor_SetStateEx((Player *)((u8 *) pl), 1U, 0x11U, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_SetG1FromTable(pl);
            }
        } else {
        }
        break;
    }
}

void TownPc_G1State08(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x273, 6, 0);
        pl->motionTime = 0;
        Actor_SetAction(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= 0x28) {
                Actor_SetStateEx((Player *)((u8 *) pl), 1U, 0x11U, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_SetG1FromTable(pl);
            }
        } else {
        }
        break;
    }
}

void TownPc_G1State09(Player *obj, s32 arg1)
{
    u8 step;

    step = obj->step;
    switch (step) {
    case 0:
        obj->step = (u8) (step + 1);
        Actor_SetMotion(obj, *(s16 *)(D_00802C60 + (arg1 * 4)), 4, 0);
        obj->motionTime = 0;
        Actor_SetAction(obj, 0, 0, 0);
        break;
    case 1:
        if (obj->motion[0].playing == 0) {
            Actor_SetIdle(obj, 0, (s32) *(s16 *)(D_00802C62 + (arg1 * 4)), 0);
        }
        break;
    }
}

void TownPc_G1State0A(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x271, 4, 0);
        pl->motionTime = 0;
        Actor_SetAction(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= 0x28) {
                Actor_SetStateEx((Player *)((u8 *) pl), 1U, 0x11U, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_SetG1FromTable(pl);
            }
        } else {
        }
        break;
    }
}

void TownPc_G1State0B(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x276, 0x10, 0);
        pl->motionTime = 0;
        Actor_SetAction(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= 0x28) {
                Actor_SetStateEx((Player *)((u8 *) pl), 1U, 0x11U, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_SetG1FromTable(pl);
            }
        } else {
        }
        break;
    }
}

void TownPc_G1State0C(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x272, 6, 0);
        pl->motionTime = 0;
        Actor_SetAction(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= 0x28) {
                Actor_SetStateEx((Player *)((u8 *) pl), 1U, 0x11U, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_SetG1FromTable(pl);
            }
        } else {
        }
        break;
    }
}

void TownPc_G1State10(Player *pl, s32 unused)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x279, 6, 0);
        pl->motionTime = 0;
        Actor_SetAction(pl, 0, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        timer = pl->timer;
        if (timer > 0) {
            pl->timer = (s32) (timer - 1);
        }
        if ((Player_IsLocal(pl) == 1) && (pl->timer == 0)) {
            if ((s32) pl->input.stickTilt >= 0x28) {
                Actor_SetStateEx((Player *)((u8 *) pl), 1U, 0x11U, 0);
                break;
            }
            if (pl->actRequest != 0) {
                TownPc_SetG1FromTable(pl);
            }
        } else {
        }
        break;
    }
}

void TownPc_G1State11(s32 a0, s32 k)
{
    Actor_SetIdle((void *)a0, 0, 0x4, 0);
}

void Debug_Message(char *msg, s32 a, s32 b, s32 c);

/* An actor's action by its state (+0x15): one function per state, the
 * argument picks a variant. */
void TownPc_RunGroup1(Player *pl)
{
    switch (pl->state) {
    case 0:
        TownPc_G1State00(pl, 0);
        break;
    case 1:
        TownPc_G1State01(pl, 0);
        break;
    case 2:
        TownPc_G1State02((void *)pl, 0);
        break;
    case 3:
        TownPc_G1State03(pl, 0);
        break;
    case 4:
        TownPc_G1State04(pl, 0);
        break;
    case 5:
        TownPc_G1State05((void *)pl, 0);
        break;
    case 6:
        TownPc_G1State06((void *)pl, 0);
        break;
    case 7:
        TownPc_G1State07((void *)pl, 0);
        break;
    case 8:
        TownPc_G1State08((void *)pl, 0);
        break;
    case 9:
        TownPc_G1State09((void *)pl, 0);
        break;
    case 10:
        TownPc_G1State0A((void *)pl, 0);
        break;
    case 11:
        TownPc_G1State0B((void *)pl, 0);
        break;
    case 12:
        TownPc_G1State0C((void *)pl, 0);
        break;
    case 13:
        TownPc_G1State09((void *)pl, 1);
        break;
    case 14:
        TownPc_G1State06((void *)pl, 2);
        break;
    case 15:
        TownPc_G1State09((void *)pl, 3);
        break;
    case 16:
        TownPc_G1State10((void *)pl, 0);
        break;
    case 17:
        TownPc_G1State11((s32)pl, 0);
        break;
    default:
        Debug_Message("NO PL ACTION", pl->stateGroup, pl->state, pl->prevState);
        break;
    }
}

/* unk_79C310's .data (0x802C60-0x80ACD0) */
u8 D_00802C60[16] = {
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
