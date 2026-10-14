#include "common.h"
#include "player.h"
#include "scene.h"
#include "lobby_globals.h"
#include "save.h"
#include "pad.h"
#include "lobby/unk_79AB50.h"   /* the texts this file uses (tools/content_gen.py) */

/* prototypes */
void TownPc_State01(Player *a);
void TownPc_State03(Player *arg0, s32 arg1);
void TownPc_State04(Player *a);
void TownPc_State06(Player *arg0);
void TownPc_State0A(Player *pl);
void TownPc_State0B(Player *arg0);
void TownPc_State0C(Player *arg0);
void TownPc_State0D(Player *arg0);
void TownPc_State0E(Player *arg0);
void TownPc_State0F(Player *a, s32 k);
void TownPc_State10(Player *arg0, s32 arg1);
void TownPc_State11(Player *arg0);
void TownPc_State12(Player *arg0);
void TownPc_State13(Player *arg0);
void TownPc_State9A(Player *arg0);
void TownPc_State14(Player *a, s32 k);
void TownPc_State15(Player *arg0);
void TownPc_State17(Player *arg0);
void TownPc_State19(Player *a, s8 k);
void TownPc_State1B(Player *arg0);
void TownPc_State1C(Player *arg0);
void TownPc_State09(Player *arg0);
void TownPc_State1D(Player *arg0);
void TownPc_State1E(Player *arg0);
void TownPc_State1F(Player *arg0);
void TownPc_State20(Player *arg0);
void TownPc_State21(Player *arg0);
void TownPc_State22(Player *arg0);
void TownPc_State16(Player *arg0);
void TownPc_State24(Player *arg0);
void TownPc_State26(Player *arg0);
void TownPc_State07(Player *arg0);
void TownPc_State27(Player *a, s8 flag);
void TownPc_State28(Player *arg0);
void TownPc_State08(Player *arg0);
void TownPc_State29(Player *arg0);
void TownPc_State2A(Player *a);
void TownPc_State2B(Player *arg0);
;
;
void TownPc_State63(Player *pl, s32 k);
;
;
;
;
void TownPc_State75(Player *a);
void TownPc_State74(Player *a);
void TownPc_State80(Player *a);
;
;
;
void TownPc_State9C(Player *a);
;
void TownPc_SteerToDest(Player *pl);
/* end of prototypes */

void Items_Take(s32 item, s32 n);
s32 Motion_PassedFrame(void *obj, s32 layer, f32 frame);
s32 Player_IsLocal(void *obj);
void Player_SetFlag(void *obj, u32 flag);
void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);
void Player_SetMoveMode(Player *p, u16 mode, s16 flag1, s16 flag2);
void Actor_SetIdle(void *, s32, s32, s32);
s32 ItemGet_Reserve(s32, u16 *);
void Jingle_Play(void *obj, s32 id);
u8 Notice_Push(u8, s16, s16);

void Actor_SetStateNow(Player *, u8, u8, u16);
void Lure_Spawn(Player *pl, u8 k);

void TownPc_FishInput(Player *pl);

void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);

void Actor_SetAction(void *a, u16 mode, s32 b, s32 c);
void TownMsg_SendStateNow(Player *p);
void TownMsg_Send1D(Player *arg0);
extern s32 gTownMode;
extern s32 gTownMenuOpen;

void VPart_Spawn(void *arg0);

void Actor_SetState(void *, s32, s32);

s32 Motion_ReachedFrame(void *, s32, f32);

void TownPc_State61(Player *pl, s32 unused)
{
    u8 step;

    ItemGet_Reserve((s32) pl, (u16 *)&pl->gotItem);
    pl->noPushTimer = 2;
    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->motionTime = 0;
        Player_SetMoveMode(pl, 0U, 0, 0);
        Player_SetFlag(pl, 0x80000U);
        pl->weaponOut = 0;
        pl->timer = 0;
        pl->stateVar1 = 0U;
        Player_SetMotionPair(pl, 0x326, 2, 0);
        break;
    case 1:
        if ((pl->fishTimer != 0) && (pl->stateVar1 == 0)) {
            pl->stateVar1 = 1U;
        }
        TownPc_FishInput(pl);
        break;
    }
}

void TownPc_State62(Player *pl, s32 unused)
{
    u8 step;

    ItemGet_Reserve((s32) pl, (u16 *)&pl->gotItem);
    pl->noPushTimer = 2;
    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->motionTime = 0;
        Player_SetMoveMode(pl, 0U, 0, 0);
        Player_SetFlag(pl, 0x80000U);
        pl->weaponOut = 0;
        pl->fishTimer = 0;
        pl->gotItemId = 0;
        pl->baited = 1;
        Player_SetMotionPair(pl, 0x325, -2, 0);
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, 104.0f) != 0) {
            Lure_Spawn(pl, 0);
        }
        if (pl->motion[0].playing == 0) {
            Actor_SetStateNow(pl, 0, 0x61, 0);
        }
        break;
    }
}

extern s32 gPoogieDigSpots;
extern u8 gPoogieDigSpotIdx;
extern u8 gDigOverLimit;
void Save_RestorePouch(Player *p);
/* unit's own declaration: with the definition's s16 (Player *p, u16 item, s16 key, u8 flags) (sub_main/unk_33E620.c), TownPc_State63: 2 of 104 words differ */
s16 Pouch_GiveItem(Player *p, u16 item, s32 key, u8 flags);

/* The player uses an item in the village: motions 0x327 and 0x328, then the
 * item is taken from the pouch (messages 0xB and 3 of Notice_Push). */
void TownPc_State63(Player *pl, s32 k)
{
    u8 step;

    ItemGet_Reserve((s32)pl, (u16 *)&pl->gotItem);
    pl->noHitTimer = pl->noPushTimer = 2;
    step = pl->step;
    switch (step) {
    case 0:
        pl->step = step + 1;
        pl->motionTime = 0;
        Player_SetMoveMode(pl, 0, 0, 0);
        Player_SetFlag(pl, 0x80000);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, 0x327, 2, 0);
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, 118.0f) != 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x328, 2, 0);
        }
        break;
    case 2:
        if (Motion_PassedFrame(pl, 0, 120.0f) != 0 && Player_IsLocal(pl) == 1) {
            Notice_Push(1, 0xB, pl->gotItemId);
            Jingle_Play(pl, 7);
            Items_Take(pl->useItem, 1);
            Save_RestorePouch(pl);
            if (Pouch_GiveItem(pl, pl->gotItemId, pl->gotItem, 0) == 3) {
                Notice_Push(1, 3, pl->gotItemId);
            }
        }
        if (pl->motion[0].playing == 0) {
            Actor_SetIdle(pl, 0, 2, 0);
        }
        break;
    }
}

void TownPc_State64(Player *pl, s32 variant)
{
    u8 step;

    ItemGet_Reserve((s32) pl, (u16 *)&pl->gotItem);
    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->motionTime = 0;
        Player_SetMoveMode(pl, 0U, 0, 0);
        Player_SetFlag(pl, 0x80000U);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, 0x329, 2, 0);
        break;
    case 1:
        if ((Motion_PassedFrame(pl, 0, 120.0f) != 0) && (Player_IsLocal(pl) == 1)) {
            if (variant == 0) {
                Jingle_Play(pl, 0xA);
                Items_Take((s32) pl->useItem, 1);
                Notice_Push(0, 3, 0);
            } else {
                Notice_Push(0, 0xD, 0);
            }
        }
        if (pl->motion[0].playing == 0) {
            Actor_SetIdle(pl, 0, 2, 0);
        }
        break;
    }
}

void TownPc_State79(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Player_SetMoveMode(pl, 0U, 0, 0);
        Actor_SetMotion(pl, 0x349, 4, 0);
        
    case 1:
        break;
    }
}

void TownPc_State70(Player *pl, s32 variant)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->timer = 0x3C;
        Actor_SetAction(pl, 0, 0, 0);
        if (!(variant & 0xFF)) {
            if (pl->motionId[0] != 0x37A) {
                Actor_SetMotion(pl, 0x37A, -0xA, 0);
                break;
            }
        } else {
            if (pl->motionId[0] != 0x37E) {
                Actor_SetMotion(pl, 0x37E, -6, 0);
            }
            break;
        }
        break;
    case 1:
        timer = pl->timer - 1;
        pl->timer = timer;
        if (timer <= 0) {
            pl->step = (u8) (pl->step + 1);
        }
        break;
    case 2:
        if ((gScene.localPlayer == pl->index) && (gTownMode != 0x30) && (gPadState[0].pressed & 0x40)) {
            gTownMode = 0;
            gFieldControl = 1;
            gTownMenuOpen = 0;
            Actor_SetIdle(pl, 0, 0x14, 0);
            TownMsg_Send1D(pl);
            TownMsg_SendStateNow((void *)((s32) pl));
        }
        break;
    }
}

s32 TownPc_IsInContest(Player *pl)
{
    if (pl->stateGroup == 0) {
        switch (pl->state) {
        case 0x6F:
        case 0x70:
        case 0x71:
            return 1;
        }
    }
    return 0;
}

void TownPc_State6F(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            if (pl->motionId[0] == 0x37A) {
                Actor_SetMotion(pl, 0x37B, 4, 0);
            } else {
                Actor_SetMotion(pl, 0x37F, 4, 0);
            }
            VPart_Spawn(pl);
        }
        
    case 1:
        break;
    }
}

s32 Award_Grant(s32 n);

void TownPc_State75(Player *pl)
{
    u8 step = pl->step;

    switch (step) {
    case 0:
        pl->step = step + 1;
        Actor_SetAction(pl, 0, 0, 0);
        if (pl->motionId[0] == 0x37B) {
            pl->yawCenter = 0;
            Actor_SetMotion(pl, 0x37C, 0, 0);
        } else {
            pl->yawCenter = 0;
            Actor_SetMotion(pl, 0x380, 0, 0);
        }
        if (pl->index == gScene.localPlayer && gSave.recordCounts[34] < 65000) {
            gSave.recordCounts[34] = gSave.recordCounts[34] + 1;
            if (gSave.recordCounts[34] >= 0x32) {
                Award_Grant(0x24);
            }
        }
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            if ((pl->yawCenter)++ > 3) {
                pl->step++;
            } else {
                Actor_SetMotion(pl, 0x26E, 0, 0x18);
            }
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            Actor_SetIdle(pl, 0, 0xC, 0);
            if (gScene.localPlayer == pl->index) {
                TownMsg_SendStateNow((void *)((s32)pl));
            }
        }
        break;
    }
}

void TownPc_State74(Player *pl)
{
    u8 step = pl->step;

    switch (step) {
    case 0:
        pl->step = step + 1;
        Actor_SetAction(pl, 0, 0, 0);
        if (pl->motionId[0] == 0x37B) {
            Actor_SetMotion(pl, 0x37D, 4, 0);
        } else {
            pl->step = 3;
            Actor_SetMotion(pl, 0x381, 4, 0);
        }
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = step + 1;
            Actor_SetMotion(pl, 0x270, 0, 0);
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            pl->step = step + 1;
            Actor_SetMotion(pl, 0xCF, 0, 0x3C);
        }
        break;
    case 3:
        if (pl->motion[0].playing <= 0) {
            Actor_SetIdle(pl, 0, 0xC, 0);
            if (gScene.localPlayer == pl->index) {
                TownMsg_SendStateNow((void *)((s32)pl));
            }
            pl->step++;
        }
        break;
    case 4:
        break;
    }
}

u8 Notice_PushItem(char *);
u8 *ActSpot_GetList(u16 area);
u16 VillageGather_RollItem(u16 id, u8 k);

/* Gathering in the village: the motion, then a find (VillageGather_RollItem from the
 * spot's table) or "nothing here". */
void TownPc_State80(Player *pl)
{
    u8 step = pl->step;
    u8 *g;
    u8 k;
    u16 n;

    switch (step) {
    case 0:
        pl->step = step + 1;
        pl->timer = 0;
        pl->action = 1;
        gTownMode = 0x4D;
        Actor_SetMotion(pl, 0x296, 0x28, 0);
        break;
    case 1:
        if ((pl->timer)++ > 0x5A) {
            pl->step++;
            if (gDigOverLimit == 1) {
                Notice_PushItem(TXT(0x0083B880) /* the notice that nothing more is left to dig */);
                Actor_SetMotion(pl, 1, 0x1E, 0);
                pl->step = 3;
            }
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            Save_RestorePouch(pl);
            if (gScene.area == 0x89) {
                g = ActSpot_GetList(pl->area) + 0x108;
                k = 0;
            } else if (gScene.area == 0x83) {
                g = ActSpot_GetList(pl->area) + 0x48;
                k = 1;
            } else {
                Actor_SetMotion(pl, 1, 0x1E, 0);
                pl->step++;
                break;
            }
            if (gPoogieDigSpots == 0) {
                Actor_SetMotion(pl, 1, 0x1E, 0);
                pl->step++;
                break;
            }
            g += ((u8 *)gPoogieDigSpots)[gPoogieDigSpotIdx] * 0x18;
            n = VillageGather_RollItem(*(u16 *)g, k);
            if (n <= 0) {
                Notice_PushItem(TXT(0x0083B8A0) /* the notice that nothing is there to dig */);
            } else {
                Pouch_GiveItem(pl, n, pl->gotItem, 1);
            }
            Actor_SetMotion(pl, 1, 0x1E, 0);
            pl->step++;
        }
        break;
    case 3:
        if (pl->motion[0].blend == 0) {
            Actor_SetIdle(pl, 0, 0, 0);
            if (gTownMode != 0) {
                gTownMode = 0;
            }
        }
        break;
    }
}

/* the padding at the end of the file's .rodata */
const u8 D_0083B8B8[8] = { 0 };

void TownPc_State8D(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetAction(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x363, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, 0x261, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

void TownPc_State90(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetAction(pl, 1, 0, 0);
        if (pl->motionId[0] != 0x265) {
            Actor_SetMotion(pl, 0x265, 0, 0);
            break;
        }
        Actor_SetMotion(pl, 0x266, 6, 0);
        break;
    case 1:
        if ((pl->motionId[0] == 0x266) && (pl->motion[0].playing <= 0)) {
            Actor_SetMotion(pl, 0x261, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

void TownPc_State91(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        if (pl->motionId[0] != 0x26A) {
            Actor_SetMotion(pl, 0x26A, 0, 0);
            break;
        }
        Actor_SetMotion(pl, 0x282, 4, 0);
        break;
    case 1:
        if ((pl->motionId[0] == 0x282) && (pl->motion[0].playing <= 0)) {
            Actor_SetMotion(pl, 0x261, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

f32 Vec_Distance(f32 *a, f32 *b);
void Actor_StartWalk(Player *obj, s16 n);
s32 Actor_HeadingDiff(void *obj, void *pos);

void TownPc_State9C(Player *pl)
{
    u8 step = pl->step;

    switch (step) {
    case 0:
        pl->step = step + 1;
        pl->fieldCollide = 0;
        pl->rot[1] += (s16)Actor_HeadingDiff(pl, (u8 *)pl + 0x754);
        pl->targetYaw = pl->rot[1];
        if (pl->gender == 1) {
            Actor_SetMotion(pl, 0x40, 4, 0);
        } else {
            Actor_SetMotion(pl, 2, 4, 0);
        }
        break;
    case 1:
        if (Vec_Distance(&pl->pos[0], &pl->moveTo[0]) < 50.0f) {
            pl->fieldCollide = 1;
            pl->timer = 10;
            pl->targetYaw = 0x8E39;
            Actor_StartWalk(pl, pl->timer);
            Actor_SetMotion(pl, 1, 4, -4);
            pl->step++;
        }
        break;
    case 2:
        if (--pl->timer <= 0) {
            pl->pos[0] = 2480.0f;
            pl->pos[1] = 0.0f;
            pl->pos[2] = 2876.0f;
            Actor_SetState(pl, 0, 1);
        }
        break;
    }
}

void TownPc_StateA0(Player *pl)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x37E, 4, 0xA0);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 216.0f) != 0) {
            pl->step = (u8) (pl->step + 1);
            pl->timer = 5;
            Actor_SetMotion(pl, 0x273, 0x14, 0);
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            timer = pl->timer - 1;
            pl->timer = timer;
            if ((timer <= 0) && (pl->index == gScene.localPlayer)) {
                Actor_SetIdle(pl, 0, 0xC, 0);
                TownMsg_SendStateNow((void *)((s32) pl));
            }
        }
        break;
    }
}

void TownPc_RunGroup0(Player *pl)
{
    u8 state;

    state = pl->state;
    switch (state) {
    case 0x1:
        TownPc_State01(pl);
        break;
    case 0x3:
        TownPc_State03(pl, 1);
        break;
    case 0x4:
        TownPc_State04(pl);
        break;
    case 0x5:
        TownPc_State03(pl, 6);
        break;
    case 0x7:
        TownPc_State07(pl);
        break;
    case 0x8:
        TownPc_State08(pl);
        break;
    case 0x9:
        TownPc_State09(pl);
        break;
    case 0x6:
        TownPc_State06(pl);
        break;
    case 0xA:
        TownPc_State0A(pl);
        break;
    case 0xB:
        TownPc_State0B(pl);
        break;
    case 0xC:
        TownPc_State0C(pl);
        break;
    case 0xD:
        TownPc_State0D(pl);
        break;
    case 0xE:
        TownPc_State0E(pl);
        break;
    case 0xF:
        TownPc_State0F(pl, 0);
        break;
    case 0x10:
        TownPc_State10(pl, 0);
        break;
    case 0x11:
        TownPc_State11(pl);
        break;
    case 0x12:
        TownPc_State12(pl);
        break;
    case 0x13:
        TownPc_State13(pl);
        break;
    case 0x14:
        TownPc_State14(pl, 0);
        break;
    case 0x15:
        TownPc_State15(pl);
        break;
    case 0x16:
        TownPc_State16(pl);
        break;
    case 0x17:
        TownPc_State17(pl);
        break;
    case 0x19:
        TownPc_State19(pl, 1);
        break;
    case 0x1A:
        TownPc_State19(pl, 0);
        break;
    case 0x1B:
        TownPc_State1B(pl);
        break;
    case 0x1C:
        TownPc_State1C(pl);
        break;
    case 0x1D:
        TownPc_State1D(pl);
        break;
    case 0x1E:
        TownPc_State1E(pl);
        break;
    case 0x1F:
        TownPc_State1F(pl);
        break;
    case 0x20:
        TownPc_State20(pl);
        break;
    case 0x21:
        TownPc_State21(pl);
        break;
    case 0x22:
        TownPc_State22(pl);
        break;
    case 0x23:
        TownPc_State03(pl, 9);
        break;
    case 0x24:
        TownPc_State24(pl);
        break;
    case 0x25:
        TownPc_State14(pl, 1);
        break;
    case 0x26:
        TownPc_State26(pl);
        break;
    case 0x27:
        TownPc_State27(pl, 0);
        break;
    case 0x18:
        TownPc_State27(pl, 1);
        break;
    case 0x28:
        TownPc_State28(pl);
        break;
    case 0x29:
        TownPc_State29(pl);
        break;
    case 0x2A:
        TownPc_State2A(pl);
        break;
    case 0x2B:
        TownPc_State2B(pl);
        break;
    case 0x61:
        TownPc_State61(pl, 0);
        break;
    case 0x62:
        TownPc_State62(pl, 0);
        break;
    case 0x63:
        TownPc_State63(pl, 0);
        break;
    case 0x64:
        TownPc_State64(pl, 0);
        break;
    case 0x65:
        TownPc_State64(pl, 1);
        break;
    case 0x6F:
        TownPc_State6F(pl);
        break;
    case 0x70:
        TownPc_State70(pl, 0);
        break;
    case 0x71:
        TownPc_State70(pl, 1);
        break;
    case 0x74:
        TownPc_State74(pl);
        break;
    case 0x75:
        TownPc_State75(pl);
        break;
    case 0x79:
        TownPc_State79(pl);
        break;
    case 0x80:
        TownPc_State80(pl);
        break;
    case 0x8D:
        TownPc_State8D(pl);
        break;
    case 0x90:
        TownPc_State90(pl);
        break;
    case 0x91:
        TownPc_State91(pl);
        break;
    case 0x9A:
        TownPc_State9A(pl);
        break;
    case 0x9C:
        TownPc_State9C(pl);
        break;
    case 0xA0:
        TownPc_StateA0(pl);
        break;
    }
    if (*(u16 *)((u8 *)pl + 0xC) != gScene.localPlayer) {
        TownPc_SteerToDest(pl);
    }
}
