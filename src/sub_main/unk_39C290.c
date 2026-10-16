#include "common.h"
#include "em.h"
#include "player.h"
#include "scene.h"
#include "save.h"
#include "item.h"
#include "quest.h"
#include "syssettings.h"

#pragma optimization_level 4

/* The monster array (em.h names it gEms, which only the game overlay links). */
extern Em gEms[];



/* The quest's settings (QuestState.info). */







extern u8 D_1BB9C8[16];
extern u16 gMonsterFlagBits[];
extern u8 gStyleSymbolColors[];
extern u8 gEmIsLarge[]; /* the game overlay's gEmIsLarge: 1 for the large monsters, by kind */
extern u16 gEmAppearMax[];
/* A key (QuestState.goals[].target) and the save counter it raises (Records_UpdateCounters). */
typedef struct CountEntry {
    u16 key;
    u16 idx;
} CountEntry;

extern CountEntry gRecordCountEntries[];


extern u8 *gArenaGaugeUpChances[];
extern s16 gPersistentEmHp[];
/* The versus settings at 0x6ADAD0 (unk_3A6130.c views the two teams as
 * QuestExtraEm gNextQuest[2]; D_006ADB14 is unk44). */
typedef struct NetTeam {
    s8 unk0;                    /* 0x00: of the team before it: see Versus_Init */
    s8 unk1;
    u8 unk2[0xE - 0x2];
    s16 kind;                   /* 0x0E: the team's monster kind */
    u16 unk10;                  /* 0x10: -> QuestTeam.sizePct */
    u8 unk12[0x1B - 0x12];
    s8 unk1B;                   /* 0x1B: -> QuestTeam.gaugeLevel */
} NetTeam;

typedef struct NetQuest {
    NetTeam team[2];            /* 0x00; team[i + 1].unk0/unk1 are read as team i's bytes 0x1C/0x1D */
    s8 unk38;
    s8 unk39;
    u8 unk3A[0x3C - 0x3A];
    u32 unk3C[2];               /* 0x3C: per team, an item (QuestTeam.item) */
    u8 unk44[0x5F - 0x44];
    u8 unk5F;                   /* 0x5F: the time limit in minutes */
} NetQuest;

/* An entry of the tables at 0x1BB3F0 and gMonsterNoteMarks[] (Versus_Init). */
typedef struct TeamEntry {
    u16 key;                    /* 0: an item; 0 ends the table */
    u16 val;
    u32 bits;                   /* for Versus_ApplyTeamBits */
} TeamEntry;

extern NetQuest gNextQuest;
extern TeamEntry gTeamItemBits[];
extern TeamEntry *gMonsterNoteMarks[];
extern s8 D_0044B448[];
extern s16 D_006ADB14;
extern u16 D_0044AF80[];
extern u16 D_0044AFB0[];

s32 Player_IsLocal(void *obj);
s32 Player_HasSkill(Player *p, u8 skill);
void Player_AddMaxStamina(Player *p, s16 n);
u16 Rand_Next(s32 stream);
s32 Game_IsOnline(void);
s32 Quest_IsDrill(void);
u8 Pouch_Add(Player *p, u16 item, s16 n);
void Em_ChangeStateRouted(Em *em, s32 a, s32 b, s32 c);
s32 Side_Match(u8 a, u8 b, u8 none);
void CamReq_Player(void *arg, u8 b);
void FadeA_Start(s32 a);
u8 Quest_FindSpecialMonster(u8 id);
u8 MonsterGroup_First(u8 id);
void Eff10_Create(f32 *pos, u8 kind, u16 area, Player *pl);
void Stats_CountItemUse(Player *pl);
s32 Quest_PhaseIsOver(s32 a);
s32 MsgEvent_Check(u16 kind);
void Record_AddHunted(SaveData *save, u8 monster, u16 n);
s32 func_003BCCB0(EquipItem *item, u8 which);
s32 RateFlag_SetAndNotify(s32 a);
s32 RateFlag_SetQuiet(u8 a);
u16 Area_BaseId(u16 area);
void Versus_UpdateRecords(s32 a);
void Records_CountQuest(void);
void Records_CountFlagged(void);
void Records_AddQuestKills(void);
void Records_UpdateCounters(s32 skip);
void Records_UpdateMonsterFlags(void);
s32 MonsterNote_Index(u16 id);
u16 Quest_Random(void);
void QuestGoal_Add(s32 flags, s16 target, s16 count);
void Versus_ApplyTeamBits(QuestTeam *t, u32 bits, u16 kind);
s32 Player_OnSameSide(Player *a, Player *b);
void Player_AddHp(Player *p, s16 n);
void QuestStats_Add(s32 a, s32 b);
/* unit's own declaration: with the definition's void (Player *p, s16 type, u8 sub, s16 bone, f32 scale) (sub_main/unk_3CA790.c), Item_Use: 1 of 1333 words differ */
void Effect06_SpawnAtBone(Player *p, s32 a, s32 b, s32 c, f32 scale);
u8 Player_GetSkill(Player *p, u8 group);
void Player_Heal(Player *p, s16 n, s16 quiet);
void Player_AddMaxHp(Player *p, s16 n, u8 temporary);
void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);
void Player_AddPoison(Player *p, s16 a);
s16 Pouch_GiveItem(Player *p, u16 item, s16 key, u8 flags);
u8 Notice_Push(u8 kind, s16 a, s16 b);
s16 Player_ScaleItemDuration(Player *pl, s16 n);
void Item_StaminaBonus(Player *pl);
s32 SpecialMine_Create(Player *p);
void Eff12_Create(Player *pl, s8 a);
void Eff03_Create(void *obj, s32 a, s8 b);
void StageObj_K0C_CreateOnPlayer(Player *p, s32 k, u8 c);
void CamReq_AreaIntro(void);
void QSync_SendState(u8 kind, u8 who);
void HuntingHorn_ClearNotes(Player *pl);
void Player_ClearMealEffects(Player *pl);
s8 Versus_Team(s8 side);
s32 Quest_IsVersus(void);
s32 Quest_KeepsRecords(void);
s32 Quest_IsTraining(void);

void Item_StaminaBonus(Player *pl)
{
    if (Player_IsLocal(pl) == 1 && Player_HasSkill(pl, 0x4E) == 1 && Rand_Next(1) % 3 == 0) {
        Player_AddMaxStamina(pl, 75);
    }
}

void Item_Use(Player *pl)
{
    s32 effect;
    Player *me;
    u16 item;
    u16 r;
    u16 n;
    u16 *tbl;

    me = &gPlayers[gScene.localPlayer];
    effect = 4;
    pl->teamItemMask = 15;
    item = pl->useItem;
    if (item != 0xF7 && item != 0xF8 && Quest_PhaseIsOver(1) == 0) {
        Pouch_Add(pl, pl->useItem, -1);
    }
    switch (item) {
    case 0x11:
        if (Player_IsLocal(pl) == 1) {
            pl->attackBuff = 25;
            pl->attackBuffTimer = Player_ScaleItemDuration(pl, 630);
        }
        Item_StaminaBonus(pl);
        Effect06_SpawnAtBone(pl, 0, 2, 10, 4.0f);
        goto motion197;
    case 0x14:
        if (Player_IsLocal(pl) == 1) {
            pl->defenseBuff = 50;
            pl->defenseBuffTimer = Player_ScaleItemDuration(pl, 630);
            pl->noFlinchItemTimer = pl->defenseBuffTimer;
        }
        Item_StaminaBonus(pl);
        Effect06_SpawnAtBone(pl, 0, 3, 10, 4.0f);
        goto motion197;
    case 0xF:
    case 0x10:
        if (Player_IsLocal(pl) == 1) {
            if (pl->useItem == 0xF) {
                if (pl->attackDrug < 3) {
                    pl->attackDrug = 3;
                }
            } else {
                if (pl->attackDrug < 5) {
                    pl->attackDrug = 5;
                }
            }
        }
        Item_StaminaBonus(pl);
        Effect06_SpawnAtBone(pl, 0, 2, 10, 4.0f);
        goto motion197;
    case 0x12:
    case 0x13:
        if (Player_IsLocal(pl) == 1) {
            if (pl->useItem == 0x12) {
                if (pl->defenseDrug < 6) {
                    pl->defenseDrug = 6;
                }
            } else {
                if (pl->defenseDrug < 10) {
                    pl->defenseDrug = 10;
                }
            }
            Item_StaminaBonus(pl);
        }
        Effect06_SpawnAtBone(pl, 0, 3, 10, 4.0f);
        goto motion197;
    case 0xCE:
        if (!Player_IsLocal(pl) && pl->area == me->area && Player_OnSameSide(pl, me) == 1 && Player_GetSkill(pl, 0x4E) != 0) {
            Effect06_SpawnAtBone(me, 0, 2, 10, 4.0f);
            me->attackBuffTimer = Player_ScaleItemDuration(pl, 5400);
            if (Player_HasSkill(pl, 0x5D) == 1) {
                me->attackBuff = 5;
            }
            if (Player_HasSkill(pl, 0x5E) == 1) {
                me->attackBuff = 10;
            }
        }
    attack:
        Item_StaminaBonus(pl);
        if (Player_IsLocal(pl) == 1) {
            pl->attackBuff = 10;
            pl->attackBuffTimer = Player_ScaleItemDuration(pl, 5400);
        }
        Effect06_SpawnAtBone(pl, 0, 2, 10, 4.0f);
        goto motion197;
    case 0xCF:
        if (!Player_IsLocal(pl) && pl->area == me->area && Player_OnSameSide(pl, me) == 1 && Player_GetSkill(pl, 0x4E) != 0) {
            Effect06_SpawnAtBone(me, 0, 3, 10, 4.0f);
            me->defenseBuffTimer = Player_ScaleItemDuration(pl, 5400);
            if (Player_HasSkill(pl, 0x5D) == 1) {
                me->defenseBuff = 20;
            }
            if (Player_HasSkill(pl, 0x5E) == 1) {
                me->defenseBuff = 40;
            }
        }
    defense:
        Item_StaminaBonus(pl);
        if (Player_IsLocal(pl) == 1) {
            pl->defenseBuff = 40;
            pl->defenseBuffTimer = Player_ScaleItemDuration(pl, 5400);
        }
        Effect06_SpawnAtBone(pl, 0, 3, 10, 4.0f);
        goto motion197;
    case 0x16:
        pl->hotDrinkTimer = Player_ScaleItemDuration(pl, 18000);
        Effect06_SpawnAtBone(pl, 0, 6, 10, 4.0f);
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 0x11, 0);
            Item_StaminaBonus(pl);
        }
        goto motion197;
    case 0x15:
        pl->coolDrinkTimer = Player_ScaleItemDuration(pl, 18000);
        Effect06_SpawnAtBone(pl, 0, 5, 10, 4.0f);
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 0x10, 0);
            Item_StaminaBonus(pl);
        }
        goto motion197;
    case 0x17:
        pl->detectTimer = Player_ScaleItemDuration(pl, 300);
        Effect06_SpawnAtBone(pl, 0, 9, 10, 4.0f);
        Item_StaminaBonus(pl);
        goto motion197;
    case 0xC:
        pl->regenTimer = Player_ScaleItemDuration(pl, 9000);
        Effect06_SpawnAtBone(pl, 0, 7, 10, 4.0f);
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 0x17, 0);
            Item_StaminaBonus(pl);
        }
        goto motion197;
    case 0xD:
    stamina:
        Item_StaminaBonus(pl);
        Player_AddMaxStamina(pl, 75);
        pl->staminaLockTimer = Player_ScaleItemDuration(pl, 5400);
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 0x18, 0);
        }
        goto effect0;
    case 0xE:
        Player_AddMaxStamina(pl, 150);
        pl->staminaLockTimer = Player_ScaleItemDuration(pl, 10800);
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 0x18, 0);
            Item_StaminaBonus(pl);
        }
        goto effect0;
    case 0x1C:
    maxhp:
        Player_AddMaxStamina(pl, 450);
    case 0x1B:
    case 0x298:
        Item_StaminaBonus(pl);
        Player_AddMaxHp(pl, 150, 0);
        Player_Heal(pl, 150, 0);
        goto effect0;
    case 0xB7:
        if (!Player_IsLocal(pl) && pl->area == me->area && Player_OnSameSide(pl, me) == 1 && Player_GetSkill(pl, 0x4E) != 0) {
            Effect06_SpawnAtBone(me, 0, 0, 10, 4.0f);
            if (Player_HasSkill(pl, 0x5D) == 1) {
                Player_Heal(me, 10, 1);
            }
            if (Player_HasSkill(pl, 0x5E) == 1) {
                Player_Heal(me, 20, 1);
            }
        }
    heal:
        Item_StaminaBonus(pl);
        Player_Heal(pl, 20, 0);
        goto effect0;
    case 0x22:
        Player_Heal(pl, 50, 0);
        Item_StaminaBonus(pl);
        if (!Player_IsLocal(pl) && pl->area == me->area && Player_OnSameSide(pl, me) == 1 && Player_OnSameSide(me, pl) == 1) {
            Player_Heal(me, 50, 1);
            Effect06_SpawnAtBone(me, 0, 0, 10, 4.0f);
        }
        goto effect0;
    case 0x7:
        Player_Heal(pl, 30, 0);
        Item_StaminaBonus(pl);
        if (!Player_IsLocal(pl) && pl->area == me->area && Player_OnSameSide(pl, me) == 1 && Player_GetSkill(pl, 0x4E) != 0) {
            Effect06_SpawnAtBone(me, 0, 0, 10, 4.0f);
            if (Player_HasSkill(pl, 0x5D) == 1) {
                Player_Heal(me, 15, 1);
            }
            if (Player_HasSkill(pl, 0x5E) == 1) {
                Player_Heal(me, 30, 1);
            }
        }
        goto effect0;
    case 0x292:
        Player_Heal(pl, 30, 0);
        Item_StaminaBonus(pl);
        goto effect0;
    case 0xC7:
        Item_StaminaBonus(pl);
        Player_AddPoison(pl, 10);
        if (pl->stateRand & 1) {
            Player_AddMaxHp(pl, 10, 0);
        } else {
            Player_AddMaxHp(pl, -10, 0);
        }
        Player_SetMotionPair(pl, 0x198, 4, 0);
        break;
    case 0x104:
        Player_AddHp(pl, -10);
        if (pl->hp <= 0) {
            pl->hp = 1;
        }
        if (pl->stateRand & 1) {
            pl->poison = 0;
            Player_SetMotionPair(pl, 0x197, 4, 0);
            Effect06_SpawnAtBone(pl, 0, 1, 10, 4.0f);
            Item_StaminaBonus(pl);
        } else {
            Player_SetMotionPair(pl, 0x198, 4, 0);
        }
        break;
    case 0xB8:
        if (pl->stateRand & 1) {
            pl->poison = 0;
            Player_SetMotionPair(pl, 0x197, 4, 0);
            Effect06_SpawnAtBone(pl, 0, 1, 10, 4.0f);
            Item_StaminaBonus(pl);
        } else {
            Player_SetMotionPair(pl, 0x198, 4, 0);
        }
        break;
    case 0x8:
        Player_Heal(pl, 50, 0);
        Item_StaminaBonus(pl);
    effect0:
        Effect06_SpawnAtBone(pl, 0, 0, 10, 4.0f);
    motion197:
        Player_SetMotionPair(pl, 0x197, 4, 0);
        break;
    case 0x9:
        Player_AddMaxHp(pl, 10, 0);
        Item_StaminaBonus(pl);
        goto effect0b;
    case 0xA:
        Player_AddMaxHp(pl, 20, 0);
        Item_StaminaBonus(pl);
    effect0b:
        Effect06_SpawnAtBone(pl, 0, 0, 10, 4.0f);
        Player_SetMotionPair(pl, 0x197, 4, 0);
        break;
    case 0x293:
    case 0xCA:
    case 0x1E:
        Player_AddMaxStamina(pl, 75);
        Item_StaminaBonus(pl);
        Effect06_SpawnAtBone(pl, 0, 4, 10, 4.0f);
        goto motion197;
    case 0x39:
    case 0x3E:
        pl->hp = pl->hpRecoverable;
        effect = 0;
        goto drink;
    case 0x3A:
        pl->hp = pl->hpRecoverable;
        pl->regenTimer = Player_ScaleItemDuration(pl, 3600);
        effect = 7;
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 0x17, 0);
        }
        goto drink;
    case 0x3B:
        pl->hp = pl->hpRecoverable;
        pl->regenTimer = Player_ScaleItemDuration(pl, 7200);
        effect = 7;
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 0x17, 0);
        }
        goto drink;
    case 0x3C:
        pl->hp = pl->hpRecoverable;
        pl->regenTimer = Player_ScaleItemDuration(pl, 7200);
        effect = 7;
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 0x17, 0);
        }
        goto drink;
    case 0x3D:
        pl->hp = pl->hpRecoverable;
        pl->regenTimer = Player_ScaleItemDuration(pl, 14400);
        effect = 7;
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 0x17, 0);
        }
        goto drink;
    case 0x37:
        pl->poison = 0;
        effect = 1;
        goto drink;
    case 0x38:
        pl->poison = 0;
        effect = 1;
        Player_AddMaxStamina(pl, 150);
        goto drink;
    case 0x33:
        pl->hotDrinkTimer = Player_ScaleItemDuration(pl, 9000);
        effect = 6;
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 0x11, 0);
        }
        goto drink;
    case 0x34:
        pl->hotDrinkTimer = Player_ScaleItemDuration(pl, 9000);
        Player_AddMaxStamina(pl, 150);
        effect = 6;
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 0x11, 0);
        }
        goto drink;
    case 0x35:
        pl->coolDrinkTimer = Player_ScaleItemDuration(pl, 9000);
        effect = 5;
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 0x10, 0);
        }
        goto drink;
    case 0x36:
        pl->coolDrinkTimer = Player_ScaleItemDuration(pl, 9000);
        Player_AddMaxStamina(pl, 150);
        effect = 5;
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 0x10, 0);
        }
        goto drink;
    case 0x2F:
        Player_AddMaxStamina(pl, 75);
        goto drink;
    case 0x31:
        Player_AddMaxStamina(pl, 450);
        goto drink;
    case 0x30:
        Player_AddMaxStamina(pl, 150);
    drink:
        if (Player_HasSkill(pl, 0x4D) == 1) {
            Player_AddMaxStamina(pl, 75);
        } else {
            Item_StaminaBonus(pl);
        }
        Effect06_SpawnAtBone(pl, 0, effect, 10, 4.0f);
        Player_SetMotionPair(pl, 0x192, 4, 0);
        break;
    case 0xEB:
    eat:
        Player_Heal(pl, 20, 0);
        Effect06_SpawnAtBone(pl, 0, 0, 10, 4.0f);
        Player_SetMotionPair(pl, 0x192, 4, 0);
        Item_StaminaBonus(pl);
        break;
    case 0xF7:
    case 0xF8:
        if (Player_IsLocal(pl) == 1) {
            Pouch_Add(pl, pl->useItem, -1);
        }
        if (pl->stateRand & 0x8000) {
            goto eat;
        }
        if (Player_IsLocal(pl) == 1) {
            if (item == 0xF7) {
                n = 0xFF;
                tbl = D_0044AF80;
            } else {
                n = 0x108;
                tbl = D_0044AFB0;
            }
            r = pl->stateRand % 100;
            while (tbl[0] != 0xFFFF) {
                if (r < tbl[0]) {
                    n = tbl[1];
                    break;
                }
                r -= tbl[0];
                tbl += 2;
            }
            Pouch_GiveItem(pl, n, pl->gotItem, 3);
        }
        Player_SetMotionPair(pl, 0x198, 4, 0);
        break;
    case 0x32:
        if (pl->stateRand & 1) {
            Player_AddMaxStamina(pl, 75);
            goto drink;
        }
        Player_AddMaxStamina(pl, -75);
        Player_SetMotionPair(pl, 0x198, 4, 0);
        break;
    case 0x1A:
        Player_Heal(pl, 10, 0);
    case 0xB:
        Item_StaminaBonus(pl);
        pl->poison = 0;
        Player_SetMotionPair(pl, 0x197, 4, 0);
        Effect06_SpawnAtBone(pl, 0, 1, 10, 4.0f);
        if (!Player_IsLocal(pl) && pl->area == me->area && (Player_HasSkill(pl, 0x5D) == 1 || Player_HasSkill(pl, 0x5E) == 1) && pl->useItem == 0xB && Player_OnSameSide(me, pl) == 1) {
            me->poison = 0;
            Effect06_SpawnAtBone(me, 0, 1, 10, 4.0f);
        }
        break;
    case 0xC8:
        r = pl->stateRand % 100;
        if (r <= 0) {
            Player_AddPoison(pl, 10);
            if (Player_HasSkill(pl, 0xC5) == 0) {
                if (Player_HasSkill(pl, 0xC4) == 1) {
                    pl->status7Timer = 900;
                } else {
                    pl->status7Timer = 1800;
                }
            }
            if (Player_HasSkill(pl, 0xE) == 0) {
                if (Player_HasSkill(pl, 0xD) == 1) {
                    pl->status6Timer = 900;
                } else if (Player_HasSkill(pl, 0xF) == 1) {
                    pl->status6Timer = 3600;
                } else {
                    pl->status6Timer = 1800;
                }
            }
            Player_SetMotionPair(pl, 0x198, 4, 0);
            break;
        }
        if (r < 6) {
            Player_AddHp(pl, -(pl->hp / 2));
            pl->hpRecoverable = pl->hp;
            Player_SetMotionPair(pl, 0x198, 4, 0);
            break;
        }
        if (r < 20) {
            Player_AddMaxStamina(pl, -75);
            Player_SetMotionPair(pl, 0x198, 4, 0);
            break;
        }
        if (r < 47) {
            goto heal;
        }
        if (r < 67) {
            goto attack;
        }
        if (r < 87) {
            goto defense;
        }
        if (r < 97) {
            goto stamina;
        }
        goto maxhp;
    }
    Stats_CountItemUse(pl);
}

void Item_UseTool(Player *pl)
{
    switch (pl->useItem) {
    case 0x56:
        Eff03_Create(pl, 0x16, 0);
        break;
    case 0x5B:
        Eff03_Create(pl, 4, 0);
        break;
    case 0x5C:
        Eff03_Create(pl, 0x10, 0);
        break;
    case 0x5F:
        Eff03_Create(pl, 0x14, 0);
        break;
    case 0x60:
        Eff03_Create(pl, 0x18, 0);
        break;
    case 0x5D:
        Eff10_Create(pl->pos, 0, pl->area, pl);
        break;
    case 0x28:
        StageObj_K0C_CreateOnPlayer(pl, 3, 0);
        break;
    case 0x2C:
        StageObj_K0C_CreateOnPlayer(pl, 3, 1);
        break;
    case 0x2E:
        StageObj_K0C_CreateOnPlayer(pl, 3, 2);
        break;
    case 0x2D:
        StageObj_K0C_CreateOnPlayer(pl, 3, 3);
        break;
    }
    Stats_CountItemUse(pl);
    if (Quest_PhaseIsOver(1) == 0) {
        Pouch_Add(pl, pl->useItem, -1);
    }
}

void Item_UsePlaced(Player *pl)
{
    switch (pl->useItem) {
    case 0x5D:
        Eff10_Create(pl->pos, 0, pl->area, pl);
        break;
    case 0x5E:
    case 0x29A:
        Eff10_Create(pl->pos, 1, pl->area, pl);
        break;
    case 0x61:
        Eff10_Create(pl->pos, 5, pl->area, pl);
        break;
    case 0x62:
        Eff10_Create(pl->pos, 6, pl->area, pl);
        break;
    }
    Stats_CountItemUse(pl);
    if (Quest_PhaseIsOver(1) == 0) {
        Pouch_Add(pl, pl->useItem, -1);
    }
}

void Item_UseMisc(Player *pl)
{
    if (pl->useItem == 0x2A6) {
        return;
    }
    switch (pl->useItem) {
    case 0x55:
    case 0x296:
        Eff10_Create(pl->pos, 4, pl->area, pl);
        break;
    case 0x54:
    case 0x299:
        Eff12_Create(pl, 0);
        break;
    case 0x2F8:
    case 0x2F9:
    case 0x30A:
    case 0x30B:
    case 0x30D:
    case 0x2FA:
    case 0x2FB:
    case 0x2FC:
    case 0x2FF:
    case 0x300:
    case 0x301:
    case 0x306:
    case 0x307:
    case 0x308:
    case 0x309:
    case 0x314:
    case 0x318:
    case 0x30E:
    case 0x30F:
    case 0x310:
    case 0x311:
    case 0x312:
    case 0x315:
    case 0x316:
    case 0x317:
    case 0x30C:
    case 0x319:
    case 0x313:
        SpecialMine_Create(pl);
        break;
    }
    Stats_CountItemUse(pl);
    if (Quest_PhaseIsOver(1) == 0) {
        Pouch_Add(pl, pl->useItem, -1);
    }
}

void Item_UseTeamPowder(Player *pl)
{
    Player *me = &gPlayers[gScene.localPlayer];

    pl->teamItemMask = 15;
    if (!Player_IsLocal(pl) && pl->area != me->area) {
        return;
    }
    switch (pl->useItem) {
    case 0xB0:
        if (Player_OnSameSide(me, pl) == 1) {
            Player_AddHp(me, 20);
            Effect06_SpawnAtBone(me, 0, 0, 10, 4.0f);
        }
        if (!Player_IsLocal(pl)) {
            Effect06_SpawnAtBone(pl, 0, 0, 10, 4.0f);
        } else {
            QuestStats_Add(2, 1);
        }
        break;
    case 0xB1:
        if (Player_OnSameSide(me, pl) == 1) {
            me->poison = 0;
            Effect06_SpawnAtBone(me, 0, 1, 10, 4.0f);
        }
        if (!Player_IsLocal(pl)) {
            Effect06_SpawnAtBone(pl, 0, 1, 10, 4.0f);
        }
        break;
    case 0xB2:
        if (Player_OnSameSide(me, pl) == 1) {
            me->attackBuff = 10;
            me->attackBuffTimer = 5400;
            Effect06_SpawnAtBone(me, 0, 2, 10, 4.0f);
        }
        if (!Player_IsLocal(pl)) {
            Effect06_SpawnAtBone(pl, 0, 2, 10, 4.0f);
        }
        break;
    case 0xB3:
        if (Player_OnSameSide(me, pl) == 1) {
            me->defenseBuff = 40;
            me->defenseBuffTimer = 5400;
            Effect06_SpawnAtBone(me, 0, 3, 10, 4.0f);
        }
        if (!Player_IsLocal(pl)) {
            Effect06_SpawnAtBone(pl, 0, 3, 10, 4.0f);
        }
        break;
    }
}

u16 Pouch_NextSelectable(Player *pl, u16 cur, u8 dir)
{
    s16 found = 0;
    s16 i;
    s16 j;

    if (dir == 0) {
        j = cur;
        for (i = 0; i < 20; i++) {
            j++;
            if (j >= 20) {
                j = 0;
            }
            if (pl->pouch[j].item != 0 && (gItemData[pl->pouch[j].item].flags & 1)) {
                cur = j;
                found = 1;
                break;
            }
        }
    } else {
        for (j = 0, i = cur; j < 20; j++) {
            if (i == 0) {
                i = 19;
            } else {
                i--;
            }
            if (pl->pouch[i].item != 0 && (gItemData[pl->pouch[i].item].flags & 1)) {
                cur = i;
                found = 1;
                break;
            }
        }
    }
    if (found == 0) {
        cur = 0;
    }
    return cur;
}

void Player_CheckAreaMsgEvent(Player *pl)
{
    if (Player_IsLocal(pl)) {
        switch (pl->area) {
        case 0x6F:
        case 2:
            MsgEvent_Check(5);
            break;
        case 0x70:
        case 3:
            MsgEvent_Check(6);
            break;
        case 0x71:
        case 4:
            MsgEvent_Check(7);
            break;
        case 0x72:
        case 5:
            MsgEvent_Check(8);
            break;
        case 0x76:
        case 0x17:
            MsgEvent_Check(9);
            break;
        case 0x74:
        case 0x13:
            MsgEvent_Check(10);
            break;
        case 0xA:
        case 0x4C:
            MsgEvent_Check(11);
            break;
        }
    }
}

s16 Player_ScaleItemDuration(Player *pl, s16 n)
{
    s16 r = n;

    if (Player_HasSkill(pl, 0xB4) == 1) {
        r = 1.5f * n;
    } else if (Player_HasSkill(pl, 0xB5) == 1) {
        r = 0.66f * n;
    }
    return r;
}

void Player_ClearMealEffects(Player *pl)
{
    s16 i;

    for (i = 0; i < 12; i++) {
        pl->mealEffect[i] = 0;
        pl->mealEffectTimer[i] = 0;
    }
}

s32 Player_HasMealEffect(Player *pl, s16 kind)
{
    return pl->mealEffectTimer[kind] > 0;
}

void Player_ClearBuffs(Player *pl)
{
    pl->staminaLockTimer = 0;
    pl->coolDrinkTimer = 0;
    pl->hotDrinkTimer = 0;
    pl->regenTimer = 0;
    pl->detectTimer = 0;
    pl->actDetectTimer = 0;
    pl->status5Delay = 0;
    pl->status5Buildup = 0;
    pl->poison = 0;
    pl->mashLeft = 0;
    pl->status7Timer = 0;
    pl->status6Timer = 0;
    pl->mashBuildup = 0;
    pl->status2Buildup = 0;
    pl->status4Buildup = 0;
    pl->recoverTimer = 0;
    pl->attackDrug = 0;
    pl->defenseDrug = 0;
    pl->attackBuff = 0;
    pl->defenseBuff = 0;
    pl->attackBuffTimer = 0;
    pl->defenseBuffTimer = 0;
    pl->swordGauge = 0;
    HuntingHorn_ClearNotes(pl);
    Player_ClearMealEffects(pl);
}

s32 Quest_IsVersus(void)
{
    if (gQuest.stage == 25001 && Game_IsOnline() == 1) {
        return 1;
    }
    return 0;
}

s8 Versus_Team(s8 side)
{
    if (side == 0) {
        return 0;
    }
    return (side - 1) & 1;
}

void Versus_MonsterDown(Em *em)
{
    s32 i;
    Em *e = gEms;

    gQuest.sideEmLeft[Versus_Team(em->side)]--;
    for (i = 0; i < 20; i++, e++) {
        if (e->active != 0 && Side_Match(em->side, e->side, 1) == 1 && e->remote == 0) {
            if (e->versusGauge < 30) {
                e->versusGauge = 0;
            } else {
                e->versusGauge -= 30;
            }
        }
    }
}

void Versus_ApplyTeamBits(QuestTeam *t, u32 bits, u16 kind)
{
    s32 i;
    u16 *max = &gEmAppearMax[kind];

    for (i = 0; i < 32; i++) {
        switch (bits & (1 << i)) {
        case 0:
            break;
        case 1:
            switch (Quest_Random() % 3) {
            case 1:
                t->rageScale1 = 125;
                t->sizeScale = 115;
                break;
            case 2:
                t->baseLevel++;
                if (t->baseLevel > 12) {
                    t->baseLevel = 12;
                }
                break;
            default:
                t->sizePct -= Quest_Random() % 10 + 1;
                if (t->sizePct < 75) {
                    t->sizePct = 75;
                }
                break;
            }
            break;
        case 2:
            if (t->sizePct < *max) {
                t->sizePct += Quest_Random() % 10 + 1;
                if (t->sizePct > *max) {
                    t->sizePct = *max;
                }
            }
            break;
        case 4:
            t->baseLevel++;
            if (t->baseLevel > 12) {
                t->baseLevel = 12;
            }
            break;
        case 8:
            switch (kind) {
            case 0x35:
            case 0x25:
            case 0x26:
            case 0x27:
            case 0x29:
            case 0x2A:
            case 0x2B:
            case 0x2D:
            case 0x2F:
            case 0x31:
                break;
            default:
                t->gaugeLevel++;
                if (t->gaugeLevel > 4) {
                    t->gaugeLevel = 4;
                }
                break;
            }
            break;
        case 0x10:
            t->flags |= 1;
            break;
        case 0x20:
            t->flags |= 2;
            break;
        case 0x40:
            t->flags |= 4;
            break;
        case 0x80:
            t->rageScale1 = 105;
            break;
        case 0x100:
            t->rageScale1 = 110;
            break;
        case 0x200:
            t->rageScale1 = 115;
            break;
        case 0x400:
            t->rageScale1 = 120;
            break;
        case 0x800:
            t->rageScale1 = 125;
            break;
        case 0x1000:
            t->rageScale2 = 97;
            break;
        case 0x2000:
            t->rageScale2 = 95;
            break;
        case 0x4000:
            t->rageScale2 = 90;
            break;
        case 0x8000:
            t->rageScale2 = 103;
            break;
        case 0x10000:
            t->rageScale2 = 105;
            break;
        case 0x20000:
            t->rageScale2 = 108;
            break;
        case 0x40000:
            t->rageScale2 = 110;
            break;
        case 0x80000:
            t->rageScale2 = 115;
            break;
        case 0x100000:
            t->rageScale2 = 120;
            break;
        case 0x200000:
            t->rageScale2 = 125;
            break;
        case 0x400000:
            t->sizeScale = 90;
            break;
        case 0x800000:
            t->sizeScale = 105;
            break;
        case 0x1000000:
            t->sizeScale = 110;
            break;
        case 0x2000000:
            t->sizeScale = 115;
            break;
        case 0x4000000:
            t->sizeScale = 120;
            break;
        case 0x8000000:
            t->sizeScale = 125;
            break;
        case 0x10000000:
            t->flags |= 8;
            break;
        case 0x20000000:
            t->flags |= 0x10;
            break;
        case 0x40000000:
            t->flags |= 0x20;
            break;
        }
    }
}

void Versus_Init(void)
{
    s32 i;
    u16 item;
    TeamEntry *e;
    s32 k;

    for (i = 0; i < 2; i++) {
        QuestGoal_Add(0x201, gNextQuest.team[i].kind, 1);
        gQuest.teams[i].unk0 = 0;
        gQuest.teams[i].gaugeLevel = gNextQuest.team[i].unk1B;
        gQuest.teams[i].baseLevel = gNextQuest.team[i + 1].unk1;
        gQuest.teams[i].rageScale1 = 100;
        gQuest.teams[i].rageScale2 = 100;
        gQuest.teams[i].sizeScale = 100;
        gQuest.teams[i].sizePct = gNextQuest.team[i].unk10;
        gQuest.teams[i].flags = 0;
        gQuest.teams[i].item = item = gNextQuest.unk3C[i];
        if (item == 0) {
            item = 0x2A7;
        }
        for (e = gTeamItemBits; e->key != 0; e++) {
            if (item == e->key) {
                break;
            }
        }
        if (e->key == 0) {
            k = MonsterNote_Index(gNextQuest.team[i].kind);
            if (k != 0xFF) {
                for (e = gMonsterNoteMarks[k]; e->key != 0; e++) {
                    if (item == e->key) {
                        break;
                    }
                }
            }
        }
        if (e->key != 0) {
            gQuest.teams[i].unk0 = e->val;
            Versus_ApplyTeamBits(&gQuest.teams[i], e->bits, gNextQuest.team[i].kind);
        } else {
            Versus_ApplyTeamBits(&gQuest.teams[i], 0, gNextQuest.team[i].kind);
        }
        gQuest.teams[i].emLevel = gQuest.teams[i].baseLevel + D_0044B448[gNextQuest.team[i + 1].unk0];
        if (gQuest.teams[i].emLevel > 15) {
            gQuest.teams[i].emLevel = 15;
        }
        if (gQuest.teams[i].emLevel < 0) {
            gQuest.teams[i].emLevel = 0;
        }
    }
    gQuest.nextSide = 1;
    gQuest.sideEmLeft[0] = 3;
    gQuest.sideEmLeft[1] = 3;
    gQuest.inputLock = 90;
    gQuest.info->timeLimit = gNextQuest.unk5F * 1800;
}

void Versus_ConsumeTeamItem(void)
{
    Player *pl;
    u16 item;

    if (D_006ADB14 == -1) {
        return;
    }
    pl = &gPlayers[gScene.localPlayer];
    item = ((u16 *)&gQuest.teams[Versus_Team(pl->side)])[7];
    if (item != 0) {
        Pouch_Add(pl, item, -1);
    }
}

void Versus_UpdateRecords(s32 lost)
{
    s32 i;
    ArenaEm *e;
    QuestTeam *t;
    u8 *tbl;
    s32 k;

    for (i = 0; i < 3; i++) {
        e = &gSave.arenaEms[i];
        if (e->emKind != 0) {
            if (D_006ADB14 == i) {
                e->unk8 = 0.85f * e->unk8;
                if (e->unk8 < 50) {
                    e->unk8 = 50;
                }
            } else {
                e->unk8 = 1.08f * e->unk8;
                if (e->unk8 > 300) {
                    e->unk8 = 300;
                }
            }
        }
    }
    gSave.versusMinutes += (s32)gQuest.elapsed / 1800;
    if (gSave.versusMinutes > 59999) {
        gSave.versusMinutes = 59999;
    }
    if (lost == 0) {
        if (++gSave.versusWins > 9999) {
            gSave.versusWins = 9999;
        }
    } else {
        if (++gSave.versusLosses > 9999) {
            gSave.versusLosses = 9999;
        }
    }
    if (D_006ADB14 == -1) {
        return;
    }
    e = &gSave.arenaEms[D_006ADB14];
    t = &gQuest.teams[Versus_Team(gPlayers[gScene.localPlayer].side)];
    e->minutes += (s32)gQuest.elapsed / 1800;
    if (e->minutes > 59999) {
        e->minutes = 59999;
    }
    e->level = t->baseLevel;
    e->sizePct = t->sizePct;
    e->gaugeLevel = t->gaugeLevel;
    if (lost == 0) {
        if (++e->wins > 9999) {
            e->wins = 9999;
        }
        if (e->gaugeLevel < 4) {
            k = MonsterNote_Index(e->emKind);
            if (k != 0xFF) {
                tbl = gArenaGaugeUpChances[k];
                if (e->minutes > 180) {
                    tbl += 16;
                } else if (e->minutes > 120) {
                    tbl += 12;
                } else if (e->minutes > 60) {
                    tbl += 8;
                } else if (e->minutes > 30) {
                    tbl += 4;
                }
                tbl += e->gaugeLevel;
                if ((Rand_Next(0) & 0x7FU) < *tbl) {
                    e->gaugeLevel++;
                }
            }
        }
    } else {
        if (++e->losses > 9999) {
            e->losses = 9999;
        }
    }
}

s32 Versus_AddGauge(Em *em, u8 n)
{
    s32 v;

    if (em->remote == 0) {
        v = em->versusGauge;
        if (v != 0xFF && n < 8) {
            switch (gQuest.teams[Versus_Team(em->side)].gaugeLevel) {
            case 0:
                v += 10;
                break;
            case 1:
                v += 15;
                break;
            case 2:
                v += 20;
                break;
            case 3:
                v += 25;
                break;
            case 4:
                v += 30;
                break;
            }
            if (v > 90) {
                v = 90;
            }
            em->versusGauge = v;
        }
    }
    return 1;
}

/* In a versus quest, whether bit `kind` (0-5) of the side's unkC is set. */
s32 Versus_TeamHasFlag(Em *em, s32 kind)
{
    if (Quest_IsVersus() == 1) {
        switch (kind) {
        case 0:
            if (gQuest.teams[Versus_Team(em->side)].flags & 1) {
                return 1;
            }
            break;
        case 1:
            if (gQuest.teams[Versus_Team(em->side)].flags & 2) {
                return 1;
            }
            break;
        case 2:
            if (gQuest.teams[Versus_Team(em->side)].flags & 4) {
                return 1;
            }
            break;
        case 3:
            if (gQuest.teams[Versus_Team(em->side)].flags & 8) {
                return 1;
            }
            break;
        case 4:
            if (gQuest.teams[Versus_Team(em->side)].flags & 0x10) {
                return 1;
            }
            break;
        case 5:
            if (gQuest.teams[Versus_Team(em->side)].flags & 0x20) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

void Versus_AssignEmTeam(Em *em)
{
    if (Quest_IsVersus() == 1) {
        em->side = gQuest.nextSide++;
        if (gQuest.nextSide > 2) {
            gQuest.nextSide = 1;
        }
        em->level = gQuest.teams[Versus_Team(em->side)].emLevel;
        em->sizePct = gQuest.teams[Versus_Team(em->side)].sizePct;
    }
}

Em *Em_FindOnSide(u8 side)
{
    Em *em;
    s32 i;

    if (side == 0) {
        return NULL;
    }
    em = gEms;
    for (i = 20; i != 0; i--, em++) {
        if (Side_Match(side, em->side, 1) == 1) {
            return em;
        }
    }
    return NULL;
}

void Versus_End(s8 side)
{
    gQuest.step = 0;
    gQuest.ackTimer = 1800;
    gQuest.acks |= 1 << gScene.localPlayer;
    gQuest.state = 3;
    if (Side_Match(gPlayers[gScene.localPlayer].side, side, 1) == 1) {
        gSysSettings.clearCapture = 0x81;
        gQuest.result = 4;
    } else {
        gQuest.result = 6;
    }
    gQuest.msgArg1 = side;
    gQuest.msgType = 17;
    gQuest.msgArg0 = 0;
    gQuest.msgArg2 = 0;
    gQuest.msgArea = 0;
    QSync_SendState(6, gScene.localPlayer);
}

void Versus_InitEm(Em *em)
{
    Em_ChangeStateRouted(em, 8, 0, 0);
    em->senseDelay = 10;
    em->scriptPending = 0;
    em->combatPlayers = 15;
}

void Versus_RouteEmByTeam(Em *em)
{
    switch (gQuest.teams[Versus_Team(em->side)].unk0) {
    case 0:
        Em_ChangeStateRouted(em, 8, 1, 0);
        break;
    case 1:
        Em_ChangeStateRouted(em, 8, 2, 0);
        break;
    case 2:
        Em_ChangeStateRouted(em, 8, 3, 0);
        break;
    default:
        Em_ChangeStateRouted(em, 8, 1, 0);
        break;
    }
    em->senseDelay = 10;
    em->scriptPending = 0;
}

s32 Versus_DefeatCamera(Em *pl)
{
    if (Versus_Team(pl->side) == 0) {
        gQuest.versusDefeat |= 4;
        CamReq_Player(pl, 2);
        return 1;
    }
    if (gQuest.versusDefeat & 1) {
        gQuest.versusDefeat |= 8;
        CamReq_Player(pl, 3);
        return 1;
    }
    return 0;
}

void Versus_MarkDefeat(Em *em)
{
    if ((gQuest.versusDefeat & 3) == 0) {
        FadeA_Start(10);
    }
    if (em->remote == 0) {
        if (Versus_Team(em->side) == 0) {
            gQuest.versusDefeat |= 1;
        } else {
            gQuest.versusDefeat |= 2;
        }
        gQuest.msgArg0 = 0;
        gQuest.msgArg2 = 0;
        gQuest.msgType = 20;
        gQuest.msgArg1 = gQuest.versusDefeat;
        QSync_SendState(6, gScene.localPlayer);
    }
}

s32 Versus_BothDefeated(void)
{
    return (gQuest.versusDefeat & 3) == 3;
}

s32 Quest_KeepsRecords(void)
{
    if (Game_IsOnline() == 1) {
        return 0;
    }
    if (Quest_IsDrill() == 1) {
        return 0;
    }
    if (Quest_IsTraining() == 1) {
        return 0;
    }
    return 1;
}

s32 Quest_IsTraining(void)
{
    return (gQuest.flags & 0x4000000) != 0;
}

s32 Quest_HasAdvice(void)
{
    return (gQuest.flags & 0xC000000) != 0;
}

s32 Quest_HasDrillFlag(void)
{
    return (gQuest.flags & 0x200000) != 0;
}

s32 Quest_IsMustOrder(void)
{
    return (gQuest.flags & 0x400000) != 0;
}

s32 Quest_IsMustOrderOrVersus(void)
{
    if (Quest_IsVersus() == 1) {
        return 1;
    }
    if (Game_IsOnline() == 0) {
        return 0;
    }
    if (gQuest.stage >= 50000) {
        return 0;
    }
    if (gQuest.flags & 0x400000) {
        return 1;
    }
    return 0;
}

void Records_UpdateMonsterFlags(void)
{
    s32 i;
    u16 *p = gMonsterFlagBits;

    if (Quest_KeepsRecords()) {
        for (i = 0; i < 32; i++, p += 2) {
            if (p[0] == 0) {
                break;
            }
            if (gQuest.hunted[MonsterGroup_First(p[0])] > 0) {
                gSave.monsterFlags |= 1 << p[1];
            }
            if (gQuest.captured[MonsterGroup_First(p[0])] > 0) {
                gSave.monsterFlags |= 1 << p[1];
            }
        }
    }
}

void Records_CountQuest(void)
{
    if (Quest_KeepsRecords() && gSave.questCount < 9999) {
        gSave.questCount++;
    }
}

void Records_CountFlagged(void)
{
    if ((gQuest.flags & 0x200000) && gSave.unk2C04 < 9999) {
        gSave.unk2C04++;
    }
}

void Records_AddQuestKills(void)
{
    s32 i;

    if (Quest_IsVersus() == 1) {
        return;
    }
    for (i = 0; i < 74; i++) {
        if (gQuest.hunted[i] > 0) {
            Record_AddHunted(&gSave, i, gQuest.hunted[i]);
        }
        if (gQuest.captured[i] > 0) {
            gSave.captured[i] += gQuest.captured[i];
        }
    }
    if (Quest_IsDrill() == 1) {
        return;
    }
    if (Quest_IsTraining() == 1) {
        return;
    }
    switch (gQuest.clearType) {
    case 2:
        for (i = 1; i < 3; i++) {
            if ((gQuest.goals[i].flags & 0x30) == 0x30 && gSave.questKills < 9999) {
                gSave.questKills++;
            }
        }
        break;
    case 1:
    case 3:
    case 4:
        break;
    }
}

void Records_UpdateCounters(s32 skip)
{
    s32 i;
    s32 j;
    s32 key;
    CountEntry *e;

    for (i = 0; i < 3; i++) {
        if ((gQuest.goals[i].flags & 0x32) == 0x32) {
            e = gRecordCountEntries;
            for (j = 0; (key = e[j].key) != 0; j++) {
                if (gQuest.goals[i].target == key && gSave.recordCounts[e[j].idx] < 9999) {
                    gSave.recordCounts[e[j].idx]++;
                }
            }
        }
    }
    for (i = 0; i < 74; i++) {
        if (gEmIsLarge[i] != 0 && gQuest.captured[i] > 0) {
            gSave.recordCounts[16] += gQuest.captured[i];
            if (gSave.recordCounts[16] > 9999) {
                gSave.recordCounts[16] = 9999;
            }
        }
    }
    if (skip != 0) {
        return;
    }
    switch (gQuest.stage) {
    case 20002:
    case 20003:
        if (gSave.recordCounts[14] < 9999) {
            gSave.recordCounts[14]++;
        }
        break;
    }
    switch (gQuest.stage) {
    case 20007:
    case 20008:
    case 20009:
    case 20010:
        if (gSave.recordCounts[15] < 9999) {
            gSave.recordCounts[15]++;
        }
        break;
    }
    switch (Area_BaseId(gScene.map)) {
    case 2:
        if (gSave.recordCounts[13] < 9999) {
            gSave.recordCounts[13]++;
        }
        break;
    case 6:
        if (gSave.recordCounts[8] < 9999) {
            gSave.recordCounts[8]++;
        }
        break;
    case 3:
        if (gSave.recordCounts[9] < 9999) {
            gSave.recordCounts[9]++;
        }
        break;
    case 4:
        if (gSave.recordCounts[10] < 9999) {
            gSave.recordCounts[10]++;
        }
        break;
    case 11:
        if (gSave.recordCounts[11] < 9999) {
            gSave.recordCounts[11]++;
        }
        break;
    case 5:
        if (gSave.recordCounts[12] < 9999) {
            gSave.recordCounts[12]++;
        }
        break;
    }
}

void Quest_UpdateSaveRecords(s32 skip)
{
    s32 i;
    ArenaEm *e;

    if (Quest_IsVersus() == 0) {
        for (i = 0; i < 3; i++) {
            e = &gSave.arenaEms[i];
            if (e->emKind != 0) {
                e->unk8 = 1.08f * e->unk8;
                if (e->unk8 > 300) {
                    e->unk8 = 300;
                }
            }
        }
    } else {
        Versus_UpdateRecords(skip);
    }
    for (i = 0; i < 3; i++) {
        e = &gSave.arenaEms[i];
        if (e->emKind != 0) {
            if (e->unk8 < 80) {
                if (Rand_Next(0) % 100 < 10) {
                    e->unk18 = 0;
                } else {
                    e->unk18 = 1;
                }
            } else if (e->unk8 < 150) {
                e->unk18 = 2;
            } else if (Rand_Next(0) % 100 < 10) {
                e->unk18 = 4;
            } else {
                e->unk18 = 3;
            }
        }
    }
    Records_CountQuest();
    Records_CountFlagged();
    Records_AddQuestKills();
    Records_UpdateCounters(skip);
    if (skip == 0) {
        Records_UpdateMonsterFlags();
        switch (gQuest.stage) {
        case 20007:
        case 20008:
        case 20009:
        case 20010:
            if (gSave.unk2724 < 200) {
                gSave.unk2724++;
            }
            break;
        }
    }
}

void Save_PickBestCounter(EquipItem *unused0, u8 unused1)
{
    u8 best;
    s32 i;

    if (gSave.styleBits != 0) {
        return;
    }
    if (func_003BCCB0(unused0, unused1) != 1) {
        return;
    }
    best = 0;
    for (i = 1; i < 3; i++) {
        if (gSave.stylePoints[best] < gSave.stylePoints[i]) {
            best = i;
        }
    }
    gSave.styleFlag = gStyleSymbolColors[best];
    RateFlag_SetAndNotify(gSave.styleFlag);
    gSave.symbolColor = gStyleSymbolColors[best];
    gSave.stylePoints[0] = 0;
    gSave.stylePoints[1] = 0;
    gSave.styleBits |= 1 << (best + 1);
    gSave.stylePoints[2] = 0;
    gSave.stylePoints[best] -= 30;
}

s32 Save_AddCounterPoints(void)
{
    s32 r = 0;
    s32 i;
    u8 k;

    if (Quest_IsVersus() == 0 && Quest_IsDrill() == 0 && Quest_IsTraining() == 0) {
        for (i = 0; i < 74; i++) {
            if (gEmIsLarge[i] != 0) {
                if (gQuest.hunted[i] > 0) {
                    gSave.stylePoints[2] += gQuest.hunted[i] * 2;
                }
                if (gQuest.captured[i] > 0) {
                    gSave.stylePoints[0] += gQuest.captured[i] * 5;
                }
            }
        }
        for (i = 0; i < 3; i++) {
            if ((gQuest.goals[i].flags & 0x32) == 0x32) {
                gSave.stylePoints[1] += 3;
            }
        }
        for (i = 1; i < 3; i++) {
            if ((gQuest.goals[i].flags & 0x31) == 0x31) {
                gSave.stylePoints[2] += 1;
            }
        }
    }
    if (gSave.styleBits == 0) {
        for (k = 0; k < 3; k++) {
            if (gSave.stylePoints[k] >= 30000) {
                gSave.stylePoints[0] /= 2;
                gSave.stylePoints[1] /= 2;
                gSave.stylePoints[2] /= 2;
                break;
            }
        }
    } else {
        for (k = 0; k < 3; k++) {
            if (gSave.stylePoints[k] >= 100) {
                gSave.styleFlag = gStyleSymbolColors[k];
                r = RateFlag_SetQuiet(gSave.styleFlag);
                gSave.stylePoints[0] = 0;
                gSave.stylePoints[1] = 0;
                gSave.stylePoints[2] = 0;
                gSave.styleBits |= 1 << (k + 1);
                gSave.stylePoints[k] -= 30;
            }
        }
    }
    return r;
}

s32 Item_UsableOutsideCamp(void)
{
    if (gScene.campArea == gScene.area) {
        return 0;
    }
    if (gQuest.flags & 0x1000) {
        return 0;
    }
    if (Quest_IsVersus() == 1) {
        return 0;
    }
    return 1;
}

void Quest_StartCountdown(Player *pl)
{
    gScene.countdownOn = 1;
    gQuest.countdown = 18000;
    if (Player_IsLocal(pl)) {
        CamReq_AreaIntro();
        gQuest.msgArg0 = 0;
        gQuest.msgType = 19;
        gQuest.msgArg1 = 0;
        gQuest.msgArg2 = 0;
        QSync_SendState(6, gScene.localPlayer);
    }
}

void Quest_UpdateCountdown(void)
{
    if (gScene.countdownOn == 1 && gQuest.countdown <= 0) {
        if (gQuest.countdown == 0) {
            gScene.countdownOn = 0;
            gQuest.msgType = 18;
            gQuest.msgArg0 = 0;
            gQuest.msgArg2 = 0;
            gQuest.countdownMsg++;
            gQuest.msgArg1 = gQuest.countdownMsg;
            QSync_SendState(6, gScene.localPlayer);
        } else {
            gScene.countdownOn = 0;
        }
    }
}

/* Marks the quest's event as triggered (gQuest.eventTriggered). Kushala,
 * Lao-Shan, Shen Gaoren and Yama Tsukami call it with themselves; the
 * monster is not used. */
void Quest_TriggerEvent(Em *em)
{
    gQuest.eventTriggered = 1;
}

u8 Quest_FindSpecialMonster(u8 id)
{
    u8 *p = D_1BB9C8;
    u8 i;

    if (Game_IsOnline() == 1) {
        return 0xFF;
    }
    if (gQuest.flags & 0x20) {
        for (i = 0; i < 16; i++, p++) {
            if (*p == 0) {
                return 0xFF;
            }
            if (*p == id) {
                return i;
            }
        }
    }
    return 0xFF;
}

void QuestEm_LoadPersistentHp(Em *em)
{
    s32 r = Quest_FindSpecialMonster(em->type);
    PersistentEm *rec;

    if (r != 0xFF) {
        rec = &gSave.persistentEms[r];
        if (rec->hp == 0) {
            rec->hp = gPersistentEmHp[r];
        }
        em->hpMax = gPersistentEmHp[r];
        em->hp = rec->hp;
        if (em->hp < gPersistentEmHp[r] / 10) {
            em->hp = gPersistentEmHp[r] / 10;
            if (em->hp > 1000) {
                em->hp = 1000;
            }
        }
        em->partsBroken = rec->partsBroken;
        gQuest.parts[em->type] = em->partsBroken;
        gQuest.savedEmHp = em->hp;
        gQuest.savedEmParts = em->partsBroken;
        gQuest.savedEmType = em->type;
    }
}

void QuestEm_SavePersistentHp(void)
{
    u8 r = Quest_FindSpecialMonster(gQuest.savedEmType);
    u16 old;

    if (r != 0xFF) {
        old = gQuest.parts[gQuest.savedEmType];
        gQuest.parts[gQuest.savedEmType] &= ~gQuest.savedEmParts;
        gSave.persistentEms[r].partsBroken = old;
        gSave.persistentEms[r].hp = gQuest.savedEmHp;
        if (gSave.persistentEms[r].hp <= 0) {
            gSave.persistentEms[r].hp = 0;
            gSave.persistentEms[r].partsBroken = 0;
        }
        gSave.lastPersistentEm = r;
    }
}

/* unk_39C290's .data (0x44AF80-0x44B450) */
extern u32 D_0044B250[];
u16 D_0044AF80[24] = {
#include "sub_main/0044AF80.inc"
};
u16 D_0044AFB0[16] = {
#include "sub_main/0044AFB0.inc"
};
u32 D_0044AFD0[36] = {
#include "sub_main/0044AFD0.inc"
};
u32 D_0044B060[24] = {
#include "sub_main/0044B060.inc"
};
u32 D_0044B0C0[24] = {
#include "sub_main/0044B0C0.inc"
};
u32 D_0044B120[36] = {
#include "sub_main/0044B120.inc"
};
u32 D_0044B1B0[16] = {
#include "sub_main/0044B1B0.inc"
};
u32 D_0044B1F0[12] = {
#include "sub_main/0044B1F0.inc"
};
u32 D_0044B220[12] = {
#include "sub_main/0044B220.inc"
};
u32 D_0044B250[32] = {
#include "sub_main/0044B250.inc"
};
CountEntry gRecordCountEntries[48] = {
#include "sub_main/0044B2D0.inc"
};
u32 gEmTallyClass[20] = {
#include "sub_main/0044B390.inc"
};
u32 gMarkMonsterCands[20] = {
#include "sub_main/0044B3E0.inc"
};
u32 gDrillPayoutThresholds[6] = {
#include "sub_main/0044B430.inc"
};
s8 D_0044B448[8] = {
#include "sub_main/0044B448.inc"
};
