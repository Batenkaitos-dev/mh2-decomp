#include "common.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "sprite.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
char *strcpy(char *, const char *);
void *memset(void *, s32, u32);
void *memcpy(void *, const void *, u32);

struct MotionView;
void Actor_SetMotionLayer(struct MotionView *obj, s32 motion, s32 frames, s32 c, s32 i);
void TownPc_Free(Player *p, s32 arg1);
void TownPc_LoadModel(Player *p, s32 arg1);
extern NetUser gNetUsers[];
s32 func_193670(u16 slot);
s32 memcmp(void *a, void *b, s32 n);
void Player_FreeWeaponModel(Player *p);
void Player_FreeModels(Player *p);
void DrawSlot_Free(s32 i);
s32 LobbyMember_SetRecord(s8 idx, NetUser *u, u8 *rec);
void Player_CalcSkills(Player *p);
void TownMsg_Send0DTo(u8 i);
s32 Game_IsOnline(void);
void HunterCard_Apply(s8 idx, void *src);
void TownPc_Spawn(u8 i, NetUser *u, char *name);
s32 TownPc_IsSeatedState(u8 arg0, u8 arg1);
s32 Area_Is86to88(s32 arg0);
s32 Area_IsOnlineHouse(s32 arg0);
s32 TownPc_IsInContest(Player *p);
void TownMsg_Send0F(void);
void TownMsg_Send18(void);
void TownMsg_Send1E(void);
extern u8 gTownSyncWait;
extern u8 gTownMsg0DPending;
s32 Area_HasTownPcs(u16 arg0);
void Mem_Compact(void);
void TownPc_LoadModelNow(Player *a0);
void TownPc_StartLoad(u8 *obj);
void Lobby_ReceiveWhileWaiting(void);
void TownMsg_Send0D(void);
void Anim_LoadCommon(s32 a);
void EmSlot_LoadModel(s32 a, s32 b, s32 c);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void DrawCb_Player(DrawSlot *r); /* sub_main: takes its draw slot */
extern u8 D_44A190[];
void Actor_SetIdle(void *obj, s32 a1, s32 n, s32 m);
void Hit_ResetAll(void);
void Effect1A_Spawn(Player *p);
extern s32 gTownMode;
void Player_InitPartTable(s32 idx);
void Player_LoadWeaponModel(s32 kind, s32 pl);
void Player_LoadPartModels(Player *p);
void Player_ClearFxBlock(Player *p);
void Player_InitRecords(Player *p);
void Player_StartMotionLayer(Player *p, u16 motion, s32 a, s16 b, s32 layer);
void Player_FreeWeaponModelIfChanged(Player *p);
void Player_FreeChangedPartModels(Player *p);
s32 Player_QueueLookFiles(Player *p);

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

/* drops the players whose member (gNetUsers) no longer matches the id in
   gLobby->unkA16 (10 NetUsers); off steps through those */
s32 TownPc_RemoveLeft(void)
{
    s8 i;
    s32 found = 0;
    s32 off;
    NetUser *u;
    Player *p;
    s32 j;

    for (i = 0, off = 0, u = gNetUsers, p = gPlayers; i < 10; i++, off += 0x11C, u++, p++) {
        if (memcmp(u->id, (u8 *)gLobby + off + 0xA16, 8) != 0
            && *(s8 *)((u8 *)gLobby + off + 0xA16) != 0
            && func_193670(p->index) == 0) {
            j = i;
            found = 1;
            gLobby->modelLoaded[j] = 0;
            gLobby->modelLoading[j] = 0;
            p->visible = 0;
            p->active = 0;
            Player_FreeWeaponModel(p);
            Player_FreeModels(p);
            if (p->drawSlot != NULL) {
                DrawSlot_Free(p->drawSlotId);
                p->drawSlot = NULL;
            }
            memset(p, 0, sizeof(Player));
            memset((u8 *)gLobby + off + 0xA16, 0, 0x11C);
            gLobby->pcLoaded[j] = 0;
            gLobby->lookPending[j] = 0;
        }
    }
    return found;
}

/* gLobby->unkA16 holds 10 NetUsers (0x11C bytes each); the check compares
   their texts with the members at gNetUsers */
s32 TownPc_RefreshChanged(void)
{
    s32 found = 0;
    s8 i;

    for (i = 0; i < 10; i++) {
        if (func_193670(gPlayers[i].index) == 0 && gPlayers[i].active != 0
            && memcmp(gNetUsers[i].text, gLobby->members[i].text, 0x100) != 0
            && LobbyMember_SetRecord(i, &gNetUsers[i], gNetUsers[i].text) == 1) {
            Player_CalcSkills(&gPlayers[i]);
            TownMsg_Send0DTo(i);
            gPlayers[i].unk3D6[0x16] = 1;
            if (func_193670(gPlayers[i].index) != 0) {
                gLobby->lookPending[gPlayers[i].index] = 2;
            }
            found = 1;
        }
    }
    return found;
}

/* copies the new members of gNetUsers into the free NetUsers of
   gLobby->unkA16 (off steps through those) and sets their players up */
void TownPc_AddJoined(void)
{
    s8 i;
    Player *me = &gPlayers[gScene.localPlayer];
    NetUser *u;
    s32 off;
    NetUser *v;

    if (Game_IsOnline() != 0) {
        for (i = 0, u = gNetUsers, off = 0, v = gNetUsers; i < 10; i++, u++, off += 0x11C, v++) {
            if ((s8)gLobby->lookPending[i] == 0 && u->id[0] != 0) {
                u8 *base = (u8 *)gLobby + off;
                if (*(s8 *)(base + 0xA16) == 0) {
                    strcpy((char *)base + 0xA16, v->id);
                    strcpy((char *)gLobby + off + 0xA1E, u->name);
                    memcpy((u8 *)gLobby + off + 0xA32, u->text, 0x100);
                    HunterCard_Apply(i, u->text);
                    TownPc_Spawn(i, u, u->name);
                    if (func_193670(me->index) != 0) {
                        gLobby->lookPending[me->index] = 2;
                    }
                    if (TownPc_IsSeatedState(me->stateGroup, me->state) == 1) {
                        if (Area_Is86to88(gScene.area) == 1) {
                            TownMsg_Send0F();
                        } else if (Area_IsOnlineHouse(gScene.area) == 1) {
                            TownMsg_Send18();
                        }
                    } else if (TownPc_IsInContest((Player *)me) == 1) {
                        TownMsg_Send1E();
                    }
                }
            }
        }
    }
}

void TownPc_SyncMembers(void)
{
    s8 i;
    Player *p;

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
        for (i = 0, p = gPlayers; i < 10; i++, p++) {
            if (p->active != 0 && (s8)gLobby->pcLoaded[i] == 0 && func_193670(p->index) == 0) {
                if (Area_IsOnlineHouse(p->area) == 1) {
                    TownPc_LoadModelNow(p);
                } else {
                    TownPc_StartLoad((u8 *)p);
                }
                Lobby_ReceiveWhileWaiting();
            }
        }
    }
    if (gTownMsg0DPending == 1) {
        TownMsg_Send0D();
        gTownMsg0DPending = 0;
    }
}

void TownPc_LoadAllModels(void)
{
    u32 i;
    Player *p;

    if (gLobby->areaLoaded == 0) {
        gLobby->areaLoaded = 1;
        if (Game_IsOnline() == 1) {
            Anim_LoadCommon(2);
        } else if (gSave.unk36E8 != 0) {
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
    for (i = 0, p = gPlayers; i < 10; i++, p++) {
        if (p->active != 0 && (s8)gLobby->pcLoaded[i] == 0) {
            TownPc_StartLoad((u8 *)p);
            Lobby_ReceiveWhileWaiting();
        }
    }
}

void TownPc_Spawn(u8 pl, NetUser *u, char *name)
{
    Player *p;
    s16 slot;
    u16 i = pl;

    p = &gPlayers[i];
    p->active = 1;
    p->index = i;
    memcpy(p->name, name, 0x11);
    p->scale[2] = 1.0f;
    p->scale[1] = 1.0f;
    p->scale[0] = 1.0f;
    p->motionLayerCount = 2;
    p->alpha = 1.0f;
    p->motion[0].step = 1.0f;
    p->motion[1].step = 1.0f;
    p->drawSlotId = DrawSlot_Alloc();
    p->fieldCollide = 1;
    slot = p->drawSlotId;
    if (slot != -1) {
        p->drawSlot = DrawSlot_Get(slot);
        p->drawSlot->player = p->index;
        p->drawSlot->draw = DrawCb_Player;
    }
    Actor_SetIdle(p, 0, 0, 0);
    p->state = 1;
    p->handlers = (struct PlayerVt *)D_44A190;
    (*(void (**)(Player *))((u8 *)p->handlers + 0xC))(p);
}

void TownPc_InitAll(s32 v)
{
    s32 j;
    Player *p;
    s16 i;

    j = 0;
    p = gPlayers;
    do {
        if (p->active != 0) {
            if ((gTownMode != 0xE) && (p->index == gScene.localPlayer)) {
                Effect1A_Spawn((void *) p);
            }
            p->actRequest = 0;
            if (v & 0xFF) {
                p->drawSlotId = DrawSlot_Alloc();
                i = p->drawSlotId;
                if (i != -1) {
                    p->drawSlot = DrawSlot_Get((s32) i);
                    p->drawSlot->player = p->index;
                    p->drawSlot->draw = DrawCb_Player;
                }
            }
        }
        j += 1;
        p++;
    } while (j < 0xA);
    Hit_ResetAll();
}

void TownPc_LoadModel(Player *p, s32 arg1)
{
    u8 mode = arg1;

    if (mode == 1) {
        gLobby->modelLoading[p->index] = 1;
    }
    if (mode == 0) {
        Player_InitPartTable(p->index);
    }
    Player_LoadWeaponModel(p->weaponKind, p->index);
    Player_LoadPartModels(p);
    Player_ClearFxBlock(p);
    Player_StartMotionLayer(p, 1, 0, 0, 0);
    Player_StartMotionLayer(p, 101, 0, 0, 1);
    Player_InitRecords(p);
    if (mode == 1) {
        gLobby->modelLoading[p->index] = 0;
        gLobby->modelLoaded[p->index] = 1;
        gLobby->pcLoaded[p->index] = 1;
        p->visible = 1;
    }
}

void TownPc_StartLoad(u8 *obj)
{
    u8 *s;
    if (Player_QueueLookFiles((Player *)obj) == 1) {
        s = (u8 *)gLobby;
        s[*(u16 *)(obj + 0xC) + 0x3954] = 1;
    }
}

void TownPc_LoadModelNow(Player *a0)
{
    TownPc_LoadModel(a0, 0x1);
}

void TownPc_Free(Player *p, s32 arg1)
{
    u8 mode = arg1;
    Player *q;
    s32 i;

    if (mode == 0) {
        p->visible = 0;
        p->active = 0;
        Player_FreeWeaponModel(p);
        Player_FreeModels(p);
    } else {
        Player_FreeWeaponModelIfChanged(p);
        Player_FreeChangedPartModels(p);
    }
    if (p->drawSlot != NULL) {
        DrawSlot_Free(p->drawSlotId);
        p->drawSlot = NULL;
    }
    if (Area_HasTownPcs(gScene.area) == 1) {
        Mem_Compact();
    }
    if (mode != 1) {
        for (i = 0, q = gPlayers; i < 10; i++, q++) {
            if (q == p) {
                gLobby->modelLoaded[i] = 0;
                gLobbyState.modelLoading[i] = 0;
                gLobbyState.lookPending[i] = 0;
                return;
            }
        }
    }
}

void TownPc_Remove(Player *a0)
{
    TownPc_Free(a0, 0);
}

void TownPc_FreeChangedModels(Player *a0)
{
    TownPc_Free(a0, 0x1);
}

/* obj: a Player or an Em (the motion layers at 0x29C, their count at 0x2C0) */
void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c)
{
    u16 layers = *(u16 *)((u8 *)obj + 0x2C0);

    if (layers <= 1 && layers != 0) {
        ((u8 *)obj)[0x771] = 0;
        Actor_SetMotionLayer(obj, motion, frames, c, 0);
    } else {
        ((u8 *)obj)[0x771] = 0;
        Actor_SetMotionLayer(obj, motion, frames, c, 0);
        Actor_SetMotionLayer(obj, motion + 100, frames, c, 1);
    }
}

void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i)
{
    ((u8 *)obj)[0x771] = 0;
    Actor_SetMotionLayer(obj, motion, frames, c, i);
}

typedef struct MotionView {
    u8 pad0[0x29C];
    s16 motion[4];
    s16 unk2A4[4];
    s16 unk2AC[4];
    u8 pad2B4[8];
    u8 unk2BC[4];
} MotionView;

void Actor_SetMotionLayer(MotionView *obj, s32 motion, s32 frames, s32 c, s32 i)
{
    obj->motion[i] = motion;
    obj->unk2AC[i] = frames / 2;
    obj->unk2A4[i] = c;
    obj->unk2BC[i] = 0;
}