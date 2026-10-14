#include "common.h"
#include "lobby_text.h"
#include "stageobj.h"
#include "lobby_state.h"
#include "em.h"
#include "m2c_macros.h"
#include "lobby_vobj.h"
#include "player.h"
#include "save.h"
#include "lobby_globals.h"
#include "scene.h"
#include "quest.h"
#include "lobby_menu.h"
char *strcpy(char *, const char *);
void *memset(void *, s32, u32);
void *memcpy(void *, const void *, u32);

/* What a villager offers when talked to (OfferMenu at 0x849F80, 0x398
 * bytes): up to 6 entries (an item offer copied from gOfferMenuItems wants up
 * to 3 item stacks) and up to 6 quests. */
typedef struct OfferItem {
    /* 0x0 */ u16 item;
    /* 0x2 */ s16 n;
} OfferItem;
typedef struct Offer {
    /* 0x00 */ OfferItem want[3];
    /* 0x0C */ s16 unkC;
    /* 0x0E */ char title[0x1D];
    /* 0x2B */ char text[0x57];
} Offer;
/* A villager's request in the save (16 at gSave + 0x299C, 0x14 bytes). */
typedef struct Request {
    /* 0x00 */ OfferItem want[3];
    /* 0x0C */ u8 which;            /* which of the entry's three titles and texts */
    /* 0x0D */ s8 used;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 offer;           /* index into gOfferTable */
    /* 0x12 */ u8 pad12[2];
} Request;
typedef struct OfferTask {
    /* 0x000 */ u8 unk0[0xC];
    /* 0x00C */ u16 id;
    /* 0x00E */ u8 unkE[0x438 - 0xE];
    /* 0x438 */ u8 unk438;          /* 3: another message when a request is done */
} OfferTask;
typedef struct OfferMenu {
    /* 0x000 */ Offer offers[6];
    /* 0x30C */ WinSlide win;
    /* 0x314 */ WinShake shake;      /* the windows' shake (Win_Shake): dx, dy */
    /* 0x31C */ OfferTask *task;
    /* 0x320 */ s16 kind[6];
    /* 0x32C */ s16 idx[6];
    /* 0x338 */ s16 unk338[6];
    /* 0x344 */ s16 unk344[6];
    /* 0x350 */ s16 unk350;
    /* 0x352 */ s16 qarg[6];
    /* 0x35E */ u16 quest[6];
    /* 0x36A */ s16 qflag[6];
    /* 0x376 */ s16 qval[6];
    /* 0x382 */ u8 unk382[2];
    /* 0x384 */ s16 unk384;         /* the cursor of the top menu */
    /* 0x386 */ s16 cur;
    /* 0x388 */ s16 unk388;
    /* 0x38A */ s16 unk38A;         /* the length of the list shown (n or nq) */
    /* 0x38C */ u16 unk38C;         /* a random nonzero value (Rand_Next(1)) */
    /* 0x38E */ u8 unk38E[2];
    /* 0x390 */ s8 unk390;          /* 0: the item list, else the quest list */
    /* 0x391 */ s8 unk391;
    /* 0x392 */ s8 n;
    /* 0x393 */ s8 nq;
    /* 0x394 */ s8 open;            /* 1 the top menu, 2 and 3 the two lists */
    /* 0x395 */ s8 unk395;          /* the step of the list shown */
    /* 0x396 */ u8 unk396[2];
} OfferMenu;

extern OfferMenu gOfferMenu;
s32 Items_HaveEnough(u16 item, s32 n);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void CamFocus_End(void);
void OfferMenu_Draw(MenuTask *t);
extern Offer gOfferMenuItems[];
QuestInfo *Quest_GetDiscInfo(u16 id);
s32 LobbyWorld_GetSeason(void);
s32 LobbyTime_IsPm(void);
s32 Quest_MatchesSeasonTime(u16 quest, s32 a, s32 b);
u16 Area_MapGroup(u16 area);
void DrinkMug_End(VObj *o);
void DrinkMug_Free(StageObj *o);
void DrinkMug_Spawn(s32 arg0);
void OfferMenu_Close(void);
s32 BaitMenu_IsOpen(void);
extern u8 gStageObjs;
u8 SavePouch_Add(u16 item, s16 n);
s32 SavePouch_Count(u16 item);
s32 SavePouch_Space(u16 item);
s32 ItemBox_Total(u16 item);
s32 ItemBox_Room(u16 item);
s16 ItemBox_Remove(u16 item, s16 n);
u32 ItemBox_AddStacks(u16 item, u32 n);
void DrawSlot_Free(s32);
void DrinkMug_Update(void *arg0);
void Npc_RefreshOffers(void);
void LobbySe_Play(s32);
void LobbyMenu_Init(void);

u16 LobbyPad_Get(s32 a);
s32 Talk_Update(void);
void Talk_Say(s32 msg, s8 a);
s32 OfferMenu_QuestList(u16 pad);
s32 OfferMenu_ItemList(u16 pad);
s32 Equip_GunHasNoAmmo(Player *pl, SaveData *save);
s32 Menu_PushNoticeOnce(s32);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
void CamFocus_SetAside(s8 a);
s32 Menu_WrapUpDown(s32 cur, s32 n);
s32 OfferMenu_HaveItems(void);
void Menu_SetYesNo(s8 a);
s32 Menu_YesNoInput(void);
s32 Win_Shake(WinShake *s);
u16 Rand_Next(s32 stream);
void Jingle_Play(void *obj, s32 id);
void OfferMenu_FinishRequest(void);
extern char *D_0080C418[];
extern char *gYesNoTexts[];
extern LobbyMenu *gLobbyMenu;
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
void Font_SetPos(s16 x, s16 y);
void Text_Printf(char *s, ...);
extern char D_00826000[];
extern char *D_0080C074;
extern char *D_0080C044;
extern char *gItemNames[];
void Font_SetColor(s32 color);
void Text_PrintfColored(char *s, ...);
void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item);
extern char *gPriceMarks[];
void Text_SetSize(s32 size);
extern char *gDeliveryTexts[];
extern char D_0080C404[];
void Menu_DrawLabelList(s16 x, char **lines, s16 n, s16 cur, u32 color);
void OfferMenu_DrawDelivery(s16 x, s16 y);
void OfferMenu_DrawQuest(s16 x, s16 y);

void DrinkMug_End(VObj *o)
{
    o->state++;
    if (o->node != NULL) {
        DrawSlot_Free(o->slot);
    }
}

void DrinkMug_Free(StageObj *o)
{
    StageObj_Free(o);
}

typedef struct V28Target {
    u8 unk0[0xA4];
    s32 ang;                    /* 0x10000 a turn */
} V28Target;
typedef struct V28Data {
    /* 0x00 */ u8 unk0[4];
    /* 0x04 */ V28Target *target;
    /* 0x08 */ u8 unk8[7];
    /* 0x0F */ s8 unkF;
    /* 0x10 */ u8 unk10[0xA];
    /* 0x1A */ s16 unk1A;
    /* 0x1C */ u8 unk1C[0xC];
    /* 0x28 */ Vec3f pos;
    /* 0x34 */ u8 unk34[8];
    /* 0x3C */ f32 heading;
    /* 0x40 */ u8 unk40[0xC];
    /* 0x4C */ s16 timer;
} V28Data;
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
void DrinkMug_Draw(DrawSlot *dr)
{
    VObj *o = dr->owner;
    Model *mdl = gCommonModels[0];
    V28Data *d = (V28Data *)o->buf;
    ModelMat *mats;
    ModelMatSet *mesh;
    f32 m[16];
    f32 scale[3];
    f32 rot[3];
    u8 sel;
    u8 i;

    if (mdl != NULL && mdl->active != 0 && o->visible != 0) {
        mats = mdl->mats;
        switch (o->type) {
        case 0:
        case 8:
            mesh = &mdl->matSets[41];
            rot[0] = 0.0f;
            rot[1] = 6.2831855f * (f32)d->unk1A / 65536.0f;
            rot[2] = 0.0f;
            break;
        case 1:
        case 9:
            mesh = &mdl->matSets[44];
            rot[0] = 0.0f;
            if (o->type == 1) {
                rot[1] = 6.2831855f * (f32)d->target->ang / 65536.0f;
            } else {
                rot[1] = d->heading;
            }
            rot[2] = 0.0f;
            sel = 1.0f + d->unkF / 2.0f;
            for (i = 1; i < mesh->count; i++) {
                ModelMat *mt = &mats[mesh->mat[i]];

                if (i == sel) {
                    mt->alpha = 1.0f;
                } else {
                    mt->alpha = 0.0f;
                }
            }
            break;
        case 2:
        case 4:
        case 5: {
            f32 dx = o->pos[0] - d->pos.x;
            f32 dy = o->pos[1] - d->pos.y;
            f32 dz = o->pos[2] - d->pos.z;

            mesh = &mdl->matSets[42];
            if (o->type == 2) {
                rot[0] = -0.5f;
            } else {
                rot[0] = 0.0f;
            }
            if (dx != 0.0f || dy != 0.0f || dz != 0.0f) {
                d->heading = 6.2831855f * (f32)(Math_Heading(dx, dz, 0.0f, 0.0f) + 0x4000) / 65536.0f;
            }
            rot[1] = d->heading;
            rot[2] = 0.0f;
            sel = (f32)d->unkF;
            for (i = 1; i < mesh->count; i++) {
                ModelMat *mt = &mats[mesh->mat[i]];

                if (i == sel) {
                    mt->alpha = 1.0f;
                } else {
                    mt->alpha = 0.0f;
                }
            }
            break;
        }
        case 3:
            mesh = &mdl->matSets[43];
            rot[0] = 0.0f;
            rot[1] = d->heading;
            rot[2] = 0.0f;
            sel = (f32)d->unkF;
            for (i = 1; i < mesh->count; i++) {
                ModelMat *mt = &mats[mesh->mat[i]];

                if (i == sel) {
                    mt->alpha = 1.0f;
                } else {
                    mt->alpha = 0.0f;
                }
            }
            break;
        case 6:
        case 7:
            mesh = &mdl->matSets[41];
            rot[0] = 0.0f;
            rot[1] = d->heading;
            rot[2] = 0.0f;
            break;
        }
        scale[0] = 0.7f;
        scale[1] = 0.7f;
        scale[2] = 0.7f;
        Mtx_FromScaleRotTrans((f32 *) scale, (f32 *) rot, dr->pos, 4, (f32 *) m);
        Effect_DrawModel(mesh, m, -1, 0, mats);
    }
}

void DrinkMug_Spawn(s32 npc)
{
    VObj *o = (VObj *)StageObj_Alloc(1);

    if (o != NULL) {
        o->kind = 0x28;
        o->type = 0;
        o->update = (StageObjFn)DrinkMug_Update;
        ((s32 *)o->buf)[1] = npc;
    }
}

VObj *VObj_FindType(s32 type);
void Em_BonePos(void *t, s16 bone, f32 *out);
void Vec_RotateY(f32 *v, f32 a);
s32 DrinkMug_IsBack(void)
{
    VObj *o = VObj_FindType(0x28);
    V28Data *w = (V28Data *)o->buf;
    Vec3f pos;
    Vec3f d;

    if (o == NULL || o->active != 1) {
        if (--w->timer < 0) {
            return 1;
        }
    } else {
        Em_BonePos(w->target, 0xB, (f32 *)&pos);
        d.x = 14.0f;
        d.y = 0.0f;
        d.z = 0.0f;
        Vec_RotateY((f32 *) &d, 6.2831855f * (f32)w->target->ang / 65536.0f);
        pos.x += d.x;
        pos.y += d.y;
        pos.z += d.z;
        if (--w->timer < 0 ||
            (pos.x - 5.0f <= o->pos[0] && !(5.0f + pos.x < o->pos[0]) &&
             pos.y - 5.0f <= o->pos[1] && !(5.0f + pos.y < o->pos[1]) &&
             pos.z - 5.0f <= o->pos[2] && !(5.0f + pos.z < o->pos[2]))) {
            return 1;
        }
    }
    return 0;
}

/* The first object of a type in sub_main's pool of 0x80 (gStageObjs). */
VObj *VObj_FindType(s32 type)
{
    VObj *o;
    s32 i;

    o = (VObj *)&gStageObjs;
    i = 0;
loop_1:
    if (o->kind == (type & 0xFF)) {
        return o;
    }
    i = (i + 1) & 0xFF;
    o++;
    if (i >= 0x80) {
        return NULL;
    }
    goto loop_1;
}

s32 Game_IsOnline(void);
s32 Save_TestEventFlag(s32 flag);
s32 Cond_IsLatest(u16 id);
s32 Save_TestListBit(u16 id);
s16 Village_GetTimerState(s32);
s32 Offer_FindActive(u8 unused0);
u16 Offer_GetQuest(u8 unused0);
void OfferMenu_AddQuest(OfferMenu *m, s16 a, u16 quest);
void OfferMenu_AddEntry(OfferMenu *m, s16 kind, s16 idx);
void OfferMenu_AddRequest(OfferMenu *m, s16 kind, s32 idx);

s16 OfferMenu_BuildList(OfferTask *t, s32 b)
{
    OfferMenu *m;
    u8 *kind = &t->unk438;
    OfferMenu tmp;
    s32 r;
    u16 q;

    if (Game_IsOnline() == 1) {
        return 0;
    }
    if (b == 0) {
        m = &gOfferMenu;
        memset(m, 0, sizeof(OfferMenu));
    } else {
        m = &tmp;
        memset(m, 0, sizeof(OfferMenu));
    }
    m->task = t;
    switch (*kind) {
    case 1:
        if (Cond_IsLatest(2) == 1) {
            OfferMenu_AddEntry(m, 0, 0xA);
        } else if (Cond_IsLatest(6) == 1) {
            OfferMenu_AddEntry(m, 0, 0xC);
        } else if (Cond_IsLatest(7) == 1) {
            OfferMenu_AddEntry(m, 0, 0xD);
        } else if (Cond_IsLatest(9) == 1) {
            OfferMenu_AddQuest(m, 0, 0x1389);
        } else if (Cond_IsLatest(0x14) == 1) {
            OfferMenu_AddQuest(m, 0, 0x138C);
        } else if (Cond_IsLatest(0x17) == 1) {
            OfferMenu_AddEntry(m, 0, 0x10);
        } else if (Cond_IsLatest(0x19) == 1) {
            if (Save_TestListBit(0x138D) == 0) {
                OfferMenu_AddQuest(m, 0, 0x138D);
            } else if (Save_TestListBit(0x138E) == 0) {
                OfferMenu_AddQuest(m, 0, 0x138E);
            } else if (Save_TestListBit(0x138F) == 0) {
                OfferMenu_AddQuest(m, 0, 0x138F);
            } else {
                break;
            }
        } else {
            break;
        }
        if (b == 1 && m->nq + m->n != 0) {
            return 4;
        }
        break;
    case 4:
        if (Save_TestEventFlag(0x79) != 0) {
            if (Save_TestEventFlag(0x30) == 0) {
                OfferMenu_AddQuest(m, 0, 0x1771);
            } else if (Village_GetTimerState(6)) {
                OfferMenu_AddQuest(m, 0, 0x1771);
            }
        }
        break;
    case 5:
        if (Save_TestEventFlag(0x3C) == 1 && Save_TestEventFlag(0x3D) == 0) {
            OfferMenu_AddEntry(m, 0, 5);
        } else if (Save_TestEventFlag(0x3E) == 1 && Save_TestEventFlag(0x3F) == 0) {
            OfferMenu_AddEntry(m, 0, 6);
        } else if (Save_TestEventFlag(0x40) == 1 && Save_TestEventFlag(0x41) == 0) {
            OfferMenu_AddEntry(m, 0, 7);
        } else if (Save_TestEventFlag(0x42) == 1 && Save_TestEventFlag(0x43) == 0) {
            OfferMenu_AddEntry(m, 0, 8);
        } else if (Save_TestEventFlag(0x44) == 1 && Save_TestEventFlag(0x45) == 0) {
            OfferMenu_AddEntry(m, 0, 9);
        }
        if (b == 1 && m->nq + m->n != 0) {
            return 4;
        }
        break;
    case 7:
        if (Cond_IsLatest(0x12) == 1) {
            OfferMenu_AddEntry(m, 0, 0xF);
            if (b == 1 && m->nq + m->n != 0) {
                return 4;
            }
        }
        break;
    case 8:
        if (Cond_IsLatest(4) == 1) {
            OfferMenu_AddEntry(m, 0, 0xB);
            if (b == 1 && m->nq + m->n != 0) {
                return 4;
            }
        }
        break;
    case 14:
        if (Save_TestEventFlag(0x32) == 1 && Save_TestEventFlag(0x33) == 0) {
            OfferMenu_AddEntry(m, 0, 0);
        } else if (Save_TestEventFlag(0x34) == 1 && Save_TestEventFlag(0x35) == 0) {
            OfferMenu_AddEntry(m, 0, 1);
        } else if (Save_TestEventFlag(0x36) == 1 && Save_TestEventFlag(0x37) == 0) {
            OfferMenu_AddEntry(m, 0, 2);
        } else if (Save_TestEventFlag(0x38) == 1 && Save_TestEventFlag(0x39) == 0) {
            OfferMenu_AddEntry(m, 0, 3);
        } else if (Save_TestEventFlag(0x3A) == 1 && Save_TestEventFlag(0x3B) == 0) {
            OfferMenu_AddEntry(m, 0, 4);
        }
        if (b == 1 && m->nq + m->n != 0) {
            return 4;
        }
        break;
    case 15:
        OfferMenu_AddQuest(m, 0, 0x1772);
        break;
    case 16:
        if (Cond_IsLatest(0xD) == 1) {
            OfferMenu_AddQuest(m, 0, 0x138A);
        } else if (Cond_IsLatest(0xE) == 1) {
            OfferMenu_AddQuest(m, 0, 0x138B);
        } else if (Cond_IsLatest(0x18) == 1) {
            OfferMenu_AddEntry(m, 0, 0x11);
        } else {
            break;
        }
        if (b == 1 && m->nq + m->n != 0) {
            return 4;
        }
        break;
    case 18:
        if (Cond_IsLatest(0xC) == 1) {
            OfferMenu_AddEntry(m, 0, 0xE);
            if (b == 1 && m->nq + m->n != 0) {
                return 4;
            }
        }
        break;
    case 0x69:
        if (Cond_IsLatest(0x52) == 1) {
            OfferMenu_AddQuest(m, 0, 0x1390);
            if (b == 1 && m->nq + m->n != 0) {
                return 4;
            }
        }
        break;
    }
    r = Offer_FindActive(*kind);
    if (r != -1) {
        OfferMenu_AddRequest(m, 1, r);
    }
    q = Offer_GetQuest(*kind);
    if (q != 0xFFFF) {
        OfferMenu_AddQuest(m, 1, q);
    }
    if (m->n != 0 && m->nq != 0) {
        m->unk391 = 1;
    } else if (m->n != 0) {
        m->unk391 = 2;
    } else if (m->nq != 0) {
        m->unk391 = 3;
    } else {
        m->unk391 = 0;
    }
    return m->unk391;
}

void OfferMenu_AddQuest(OfferMenu *m, s16 a, u16 quest)
{
    QuestInfo *q = Quest_GetDiscInfo(quest);
    s32 x;
    s32 y;

    if (m->nq != 6) {
        x = LobbyWorld_GetSeason();
        y = LobbyTime_IsPm();
        if (Quest_MatchesSeasonTime(quest, x, y) != 0) {
            m->qarg[m->nq] = a;
            m->quest[m->nq] = quest;
            m->qval[m->nq] = Area_MapGroup(q->map);
            if (a == 0) {
                m->qflag[m->nq] = 0;
            } else {
                m->qflag[m->nq] = 1;
            }
            m->nq++;
        }
    }
}

void OfferMenu_AddEntry(OfferMenu *m, s16 kind, s16 idx)
{
    if (m->n != 6) {
        m->kind[m->n] = kind;
        m->idx[m->n] = idx;
        m->unk344[m->n] = 0x10;
        if (kind == 0) {
            m->offers[m->n] = gOfferMenuItems[idx];
            m->unk338[m->n] = 0;
        }
        m->n++;
    }
}

char *Request_Title(s16 i);
char *Request_Text(s16 i);
/* The save's villager requests (16 at gSave + 0x299C). */
static inline Request *Save_Request(u8 *save, s32 idx)
{
    return (Request *)(save + idx * 0x14 + 0x299C);
}
void OfferMenu_AddRequest(OfferMenu *m, s16 kind, s32 idx)
{
    Request *r;

    if (m->n != 6) {
        m->kind[m->n] = kind;
        m->idx[m->n] = 0;
        m->unk344[m->n] = 0x10;
        r = Save_Request((u8 *)&gSave, idx);
        memcpy(&m->offers[m->n], r, 0xC);
        m->offers[m->n].unkC = r->unkE;
        strcpy(m->offers[m->n].title, Request_Title(idx));
        strcpy(m->offers[m->n].text, Request_Text(idx));
        m->unk338[m->n] = 1;
        m->n++;
    }
}

void OfferMenu_Open(OfferTask *t)
{
    OfferMenu *m = &gOfferMenu;

    if (m->task->id != t->id) {
        gTownMode = 0;
        LobbyMenu_SetDraw(0, 0, 0, 0);
        gTownMenuOpen = 0;
        CamFocus_End();
    }
    m->open = 1;
    gTownMode = 0x43;
    LobbyMenu_SetDraw(OfferMenu_Draw, 0, 0, 0);
}

void OfferMenu_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    LobbySe_Play(4);
    Npc_RefreshOffers();
}

void OfferMenu_Update(void)
{
    OfferMenu *m = &gOfferMenu;
    u16 pad = LobbyPad_Get(0);

    switch (m->open) {
    case 0:
        switch (Talk_Update()) {
        case 0:
        case 3:
            m->open++;
            LobbySe_Play(0x11);
            break;
        }
        break;
    case 1:
        if (pad & 0x20) {
            m->unk390 = m->unk384;
            m->cur = 0;
            if (m->unk391 == 3) {
                m->unk390 = 1;
            }
            if (m->unk390 == 0) {
                m->open = 3;
                m->unk38A = m->n;
            } else {
                m->open = 2;
                m->unk38A = m->nq;
                m->unk388 = 0;
                if (Equip_GunHasNoAmmo(&gPlayers[gScene.localPlayer], &gSave) == 1) {
                    Menu_PushNoticeOnce(0x13);
                }
            }
            WinSlide_Step(&m->win, 0x280, 0x118);
            CamFocus_SetAside(1);
            LobbySe_Play(0x17);
        } else if (pad & 0x40) {
            m->open = 4;
            Talk_Say(1, 0);
            LobbySe_Play(4);
        } else if (m->unk391 == 1) {
            m->unk384 = Menu_WrapUpDown(m->unk384, 2);
        }
        break;
    case 2:
        switch (OfferMenu_QuestList(pad)) {
        case 0:
            m->open = 4;
            break;
        case 3:
            m->open = 1;
            break;
        }
        break;
    case 3:
        switch (OfferMenu_ItemList(pad)) {
        case 0:
            m->open++;
            break;
        case 3:
            m->open = 1;
            break;
        }
        break;
    case 4:
        switch (Talk_Update()) {
        case 0:
            OfferMenu_Close();
            break;
        }
        break;
    }
}

extern u16 gQuestStampRand;
s32 Menu_WrapUpDownSe(s32 cur, s32 n, M2C_UNK k);
s32 Menu_WrapLeftRight(s32 cur, s32 n);
void QuestDesk_Accept(u16 id, s16 money, s32 pay);
s32 OfferMenu_QuestList(u16 pad)
{
    OfferMenu *m = &gOfferMenu;
    QuestInfo *q = Quest_GetDiscInfo(m->quest[m->cur]);

    switch (m->unk395) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            m->unk395++;
            break;
        }
        break;
    case 1:
        if (pad & 0x20) {
            if (gLobby->questTaken == 0) {
                if (q->fee > gSave.money) {
                    LobbySe_Play(9);
                } else {
                    m->unk395++;
                    Menu_SetYesNo(0);
                    LobbySe_Play(8);
                }
            } else {
                LobbySe_Play(9);
            }
        } else if (pad & 0x40) {
            m->unk395 = 0;
            CamFocus_SetAside(0);
            LobbySe_Play(4);
            return 3;
        } else {
            m->cur = Menu_WrapUpDownSe(m->cur, m->unk38A, 0x17);
            m->unk388 = Menu_WrapLeftRight(m->unk388, 3);
        }
        break;
    case 2:
        switch (Menu_YesNoInput()) {
        case 0:
            m->unk395++;
            m->unk38C = Rand_Next(1);
            if (m->unk38C == 0) {
                m->unk38C = 1;
            }
            gQuestStampRand = m->unk38C;
            Win_Shake(&m->shake);
            break;
        case 3:
            m->unk395 = 1;
            break;
        }
        break;
    case 3:
        switch (Win_Shake(&m->shake)) {
        case 0:
            m->unk395++;
            QuestDesk_Accept(q->id, 0, 1);
            gQuestOfferId = m->task->unk438;
            break;
        }
        break;
    case 4:
        if (pad & 0x20) {
            m->unk395 = 0;
            CamFocus_SetAside(0);
            Talk_Say(1, 0);
            LobbySe_Play(8);
            return 0;
        }
        break;
    }
    return 2;
}

s32 OfferMenu_ItemList(u16 pad)
{
    OfferMenu *m = &gOfferMenu;
    u8 *kind = &m->task->unk438;

    switch (m->unk395) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            m->unk395++;
            break;
        }
        break;
    case 1:
        if (pad & 0x20) {
            if (OfferMenu_HaveItems() == 1) {
                m->unk395++;
                Menu_SetYesNo(1);
                LobbySe_Play(8);
            } else {
                LobbySe_Play(9);
            }
        } else if (pad & 0x40) {
            m->unk395 = 0;
            CamFocus_SetAside(0);
            LobbySe_Play(4);
            return 3;
        } else {
            m->cur = Menu_WrapUpDown(m->cur, m->unk38A);
        }
        break;
    case 2:
        switch (Menu_YesNoInput()) {
        case 0:
            m->unk395++;
            m->unk38C = Rand_Next(1);
            if (m->unk38C == 0) {
                m->unk38C = 1;
            }
            Win_Shake(&m->shake);
            break;
        case 3:
            m->unk395 = 1;
            break;
        }
        break;
    case 3:
        switch (Win_Shake(&m->shake)) {
        case 0:
            m->unk395++;
            Jingle_Play(&gPlayers[gScene.localPlayer], 0xB);
            break;
        }
        break;
    case 4:
        if (pad & 0x20) {
            m->unk395 = 0;
            CamFocus_SetAside(0);
            if (*kind == 3) {
                Talk_Say(0x12, 0);
            } else {
                Talk_Say(2, 0);
            }
            LobbySe_Play(8);
            OfferMenu_FinishRequest();
            return 0;
        }
        break;
    }
    return 2;
}

void Items_Take(s32 item, s32 n)
{
    s32 have;

    if (item != 0) {
        have = SavePouch_Count(item);
        if (have != 0) {
            if (have >= n) {
                SavePouch_Add(item, -n);
                return;
            }
            SavePouch_Add(item, -have);
            n -= have;
        }
        ItemBox_Total(item);
        ItemBox_Remove(item, n);
    }
}

s8 Items_Give(s32 item, s32 n)
{
    s32 space;

    if (item == 0) {
        return 0;
    }
    space = SavePouch_Space(item);
    if (space != 0 && space != -1) {
        if (space < n) {
            SavePouch_Add(item, space);
            n -= space;
        } else {
            SavePouch_Add(item, n);
            return 0;
        }
    }
    ItemBox_AddStacks(item, n);
    return 1;
}

s32 Items_Room(s32 item)
{
    s32 space;
    s32 total;

    space = SavePouch_Space(item);
    if (space == -1) {
        space = 0;
    }
    total = space + ItemBox_Room(item);
    if (total > 999) {
        return 999;
    }
    return total;
}

void Save_SetEventFlag(s32 flag);
void Npc_ResetTalked(void);
void QuestList_Rebuild(void);
void Village_UpdateProgress(s32 a);
void Offer_AddPoints(s32 id, s32 v);
void Offer_ClearSaved(s32 unused0);
void OfferMenu_FinishRequest(void)
{
    OfferMenu *m = &gOfferMenu;
    s8 i;
    u8 *kind;

    for (i = 0; i < 3; i++) {
        if (m->offers[m->cur].want[i].item != 0) {
            Items_Take(m->offers[m->cur].want[i].item, m->offers[m->cur].want[i].n);
        }
    }
    kind = &m->task->unk438;
    Offer_AddPoints(*kind, m->offers[m->cur].unkC);
    if (m->kind[m->cur] == 0) {
        switch (m->idx[m->cur]) {
        case 10:
            Save_SetEventFlag(6);
            Npc_ResetTalked();
            QuestList_Rebuild();
            break;
        case 11:
            Save_SetEventFlag(8);
            Npc_ResetTalked();
            break;
        case 12:
            Save_SetEventFlag(0xB);
            Npc_ResetTalked();
            QuestList_Rebuild();
            break;
        case 13:
            Save_SetEventFlag(0xC);
            Npc_ResetTalked();
            break;
        case 14:
            Save_SetEventFlag(0x48);
            Npc_ResetTalked();
            break;
        case 15:
            Save_SetEventFlag(0x50);
            Npc_ResetTalked();
            break;
        case 16:
            Save_SetEventFlag(0x55);
            Npc_ResetTalked();
            break;
        case 17:
            Save_SetEventFlag(0x57);
            Npc_ResetTalked();
            break;
        case 0:
            Save_SetEventFlag(0x33);
            break;
        case 1:
            Save_SetEventFlag(0x35);
            break;
        case 2:
            Save_SetEventFlag(0x37);
            break;
        case 3:
            Save_SetEventFlag(0x39);
            break;
        case 4:
            Save_SetEventFlag(0x3B);
            break;
        case 5:
            Save_SetEventFlag(0x3D);
            break;
        case 6:
            Save_SetEventFlag(0x3F);
            break;
        case 7:
            Save_SetEventFlag(0x41);
            break;
        case 8:
            Save_SetEventFlag(0x43);
            break;
        case 9:
            Save_SetEventFlag(0x45);
            break;
        }
        Village_UpdateProgress(1);
    } else {
        Offer_ClearSaved(*kind);
    }
}

/* item + n: enough of the item in the pouch and the box together */
INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6C0D00", D_00826000);

s32 Items_HaveEnough(u16 item, s32 n)
{
    s32 have;

    if (item == 0) {
        return 1;
    }
    have = SavePouch_Count(item);
    return have + ItemBox_Total(item) >= n;
}

s32 Items_Total(u16 item)
{
    s32 total;

    if (item == 0) {
        return 0;
    }
    total = SavePouch_Count(item);
    total += ItemBox_Total(item);
    if (total > 999) {
        total = 999;
    }
    return total;
}

s32 OfferMenu_HaveItems(void)
{
    OfferMenu *m = &gOfferMenu;
    s8 i;

    for (i = 0; i < 3; i++) {
        OfferItem *w = &m->offers[m->cur].want[i];

        if (w->item != 0 && Items_HaveEnough(w->item, w->n) == 0) {
            return 0;
        }
    }
    return 1;
}

void OfferMenu_DrawDeliverHint(s16 x, s16 y, s8 kind)
{
    Win_DrawText(x, y, 0x152, 0x3C, 0xBDB59F6C, 0xC0101010, D_0080C418[kind]);
    if (kind == 2) {
        Font_SetPos(x + 0x8C, y + 0x1E);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
    }
}

s32 Items_DrawNeedLine(s16 x, s16 y, u16 item, s32 n, u32 color)
{
    s32 pouch;
    s32 box;
    s8 lack = 0;

    if (item == 0) {
        return 1;
    }
    pouch = SavePouch_Count(item);
    box = ItemBox_Total(item);
    if (box > 999) {
        box = 999;
    }
    if (Items_HaveEnough(item, n) == 0) {
        Font_SetColor(0x80303030);
        lack = 1;
    } else {
        Font_SetColor(color);
    }
    Font_SetPos(x, y);
    Text_PrintfColored(D_00826000, gItemNames[item], D_0080C074, pouch, box, n, D_0080C044);
    Item_DrawIcon(x - 0x1A, y - 2, 0x14, item);
    if (lack != 0) {
        return 0;
    }
    return 1;
}

s32 Items_DrawNeedLineAfter(s16 x, s16 y, u16 item, s32 n, u32 color)
{
    s32 need;
    s32 pouch;
    s32 box;

    if (item == 0) {
        return 1;
    }
    need = n;
    pouch = SavePouch_Count(item);
    box = ItemBox_Total(item);
    Font_SetColor(color);
    if (pouch >= n) {
        pouch -= n;
    } else {
        n -= pouch;
        box -= n;
        pouch = 0;
    }
    if (box > 999) {
        box = 999;
    }
    Font_SetPos(x, y);
    Text_PrintfColored(D_00826000, gItemNames[item], D_0080C074, pouch, box, need, D_0080C044);
    Item_DrawIcon(x - 0x1A, y - 2, 0x14, item);
    return 1;
}

void Menu_DrawItemCountRow(s16 x, s16 y, u16 item, u32 color)
{
    s32 pouch;
    s32 box;

    if (item != 0) {
        pouch = SavePouch_Count(item);
        box = ItemBox_Total(item);
        if (box > 999) {
            box = 999;
        }
        Font_SetColor(color);
        Font_SetPos(x, y);
        Text_PrintfColored("%-16s%s%2d(%3d)", gItemNames[item], D_0080C074, pouch, box);
        Item_DrawIcon(x - 0x24, y - 2, 0x14, item);
    }
}

void Menu_DrawItemPriceRow(s16 x, s16 y, u16 item, u32 color, s32 price)
{
    if (item != 0) {
        Font_SetColor(color);
        Font_SetPos(x, y);
        Text_PrintfColored("%-16s %7d%s", gItemNames[item], price, gPriceMarks[0]);
        Item_DrawIcon(x - 0x24, y - 2, 0x14, item);
    }
}

void Font_SetSpacing(u8 a, u8 b);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Menu_DrawStamp(s16 x, s16 y, s16 w, s16 h, u16 a, s16 b);
void Menu_DrawQuestSheet(s16 x, s16 y, s16 w, s16 h, s16 *offs, s16 sel, s8 kind, s32 b);
s32 Items_DrawNeedLine(s16 x, s16 y, u16 item, s32 n, u32 color);
void OfferMenu_DrawDeliverHint(s16 x, s16 y, s8 kind);
extern char *D_0080C408;
extern char *D_0080C40C;
extern char *D_0080C424;
extern char *gQuestPeriodTexts[];
/* The title is centred on x + 0xB4 (the parameter, not its copy sx): with sx there, the want
 * lines' x + 0x3C cost a `move` (round 25: a permuter find; it replaced round 19's (s32)x
 * casts). Menu_DrawStamp is declared as unk_6EF380.c has it (u16 a, s16 b): with s32 y was
 * loaded later. */
void OfferMenu_DrawDelivery(s16 x, s16 y)
{
    s32 tx;
    s16 offs[7];
    s32 sx;
    OfferMenu *m = &gOfferMenu;
    s16 cy = y;
    s32 ux;
    u16 item;
    u8 *p = &m->task->unk438;
    s8 lack;
    s16 i;

    for (i = 0; i < 7; i++) {
        offs[i] = *(u16 *)&m->unk344[i];
        if (m->cur == i) {
            offs[i] = -offs[i];
        }
    }
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xB4, 0xFF106060, gDeliveryTexts[0], 2, 1);
    Menu_DrawQuestSheet(x, y, 0x168, 0x12C, offs, m->cur, -1, -2);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80101010);
    sx = x;
    Font_SetPos(Text_CenterX(m->offers[m->cur].title, x + 0xB4, 0x12), cy += 0x22);
    Text_PrintfColored(m->offers[m->cur].title);
    Font_SetColor(0x80301010);
    cy += 0x10;
    lack = 0;
    i = 0;
    for (; i < 3; i++) {
        item = m->offers[m->cur].want[i].item;
        Font_SetPos(x + 0x3C, cy += 0x14);
        if (item != 0) {
            if (Items_DrawNeedLine(x + 0x3C, cy, item, m->offers[m->cur].want[i].n, 0x80101010) != 1) {
                lack = 1;
            }
        }
    }
    Font_SetColor(0x80301010);
    tx = sx + 0x28;
    Font_SetPos(tx, cy += 0x24);
    Text_PrintfColored(D_0080C408);
    Font_SetColor(0x80101010);
    ux = sx + 0x8C;
    Font_SetPos(ux, cy);
    Text_PrintfColored(gQuestPeriodTexts[m->unk338[m->cur]]);
    Font_SetColor(0x80301010);
    Font_SetPos(tx, cy += 0x1A);
    Text_PrintfColored(D_0080C40C);
    Font_SetColor(0x80101010);
    Font_SetPos(ux, cy);
    Text_PrintfColored(*(char **)(p + 8));
    Font_SetColor(0x80101010);
    Font_SetPos(sx + 0x32, (s16)(cy + 8) + 0x14);
    Text_PrintfColored(m->offers[m->cur].text);
    switch (m->unk395) {
    case 3:
    case 4:
        Win_DrawText(sx + 0xA, 0x174, 0x152, 0x3C, 0xBDB59F6C, 0xC0101010, D_0080C424);
        break;
    case 2:
        OfferMenu_DrawDeliverHint(sx + 0xA, 0x174, 2);
        break;
    default:
        OfferMenu_DrawDeliverHint(sx + 0xA, 0x174, lack);
        break;
    }
    if (m->unk38C != 0) {
        Menu_DrawStamp(x, y, 0x168, 0x12C, m->unk38C, m->shake.timer);
    }
}

void Menu_DrawMoney(s16 x, s16 y, s8 flag);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Quest_DrawOverview(s16 x, s16 y, QuestInfo *q);
void Quest_DrawTerms(s16 x, s16 y, QuestInfo *q);
void Quest_DrawDetails(s16 x, s16 y, QuestInfo *q);
void Quest_DrawClearMark(s16 x, s16 y);
s32 Save_TestListBit(u16 id);
extern s8 gMapGroupMapIds[][2];
extern char *gQuestOrderPromptTexts[];
extern char *D_0080C434;
extern char *gQuestBusyTexts[];
extern char *D_0080C43C;
extern char *gQuestOrderedTexts[];
void OfferMenu_DrawQuest(s16 x, s16 y)
{
    OfferMenu *m = &gOfferMenu;
    QuestInfo *info;
    s16 offs[7];
    s16 i;

    info = Quest_GetDiscInfo(m->quest[m->cur]);
    for (i = 0; i < 7; i++) {
        offs[i] = *(u16 *)&m->qval[i];
        if (m->cur == i) {
            offs[i] = -offs[i];
        }
    }
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xB4, 0xFF106060, *(char **)D_0080C404, 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    Menu_DrawQuestSheet(x, y, 0x154, 0x168, offs, m->cur, gMapGroupMapIds[(u16)m->qval[m->cur]][0], -2);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80101010);
    Font_SetColor(0x80101010);
    if (m->unk395 == 1) {
        Menu_DrawPageNum(x + 0x122, y + 0xE, m->unk388, 3, 1);
    }
    Font_SetSpacing(0x12, 0x12);
    switch (m->unk388) {
    case 0:
        Quest_DrawOverview(x + 0x28, y + 0xE, info);
        break;
    case 1:
        Quest_DrawTerms(x + 0x28, y + 0xE, info);
        break;
    case 2:
        Quest_DrawDetails(x + 0x28, y + 0xE, info);
        break;
    }
    if (gLobby->questTaken != 0 && m->unk395 != 3 && m->unk395 != 4) {
        Win_DrawText(x, 0x174, 0x152, 0x3C, 0xBDB59F6C, 0xC0101010, gQuestBusyTexts[0]);
    } else {
        switch (m->unk395) {
        case 2:
            Win_DrawText(x, 0x174, 0x152, 0x3C, 0xBDB59F6C, 0xC0101010, D_0080C43C);
            Font_SetPos(x + 0x8C, 0x192);
            Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
            break;
        case 3:
        case 4:
            Win_DrawText(x, 0x174, 0x152, 0x3C, 0xBDB59F6C, 0xC0101010, gQuestOrderedTexts[0]);
            break;
        default:
            if (info->fee > gSave.money) {
                Win_DrawText(x, 0x174, 0x152, 0x3C, 0xBDB59F6C, 0xC0101010, D_0080C434);
            } else {
                Win_DrawText(x, 0x174, 0x152, 0x3C, 0xBDB59F6C, 0xC0101010, gQuestOrderPromptTexts[0]);
            }
            break;
        }
    }
    if (m->unk38C != 0) {
        Menu_DrawStamp(x + 0x28, y + 0xE, 0x154, 0x168, m->unk38C, m->shake.timer);
    }
    if (Save_TestListBit(info->id) != 0) {
        Quest_DrawClearMark((s16)(x + 0x28) + 0x1E, -0xD6);
    }
}

void OfferMenu_Draw(MenuTask *t)
{
    OfferMenu *m = &gOfferMenu;
    s16 x;
    s16 y;

    Text_SetSize(t->textSize);
    Font_SetColor(0x80101010);
    x = m->win.pos + m->shake.dx;
    y = m->shake.dy + 0x3A;
    switch (m->open) {
    case 1:
        switch (m->unk391) {
        case 2:
            Menu_DrawLabelList(0x1CC, gDeliveryTexts, 1, m->unk384, 0xFF106060);
            break;
        case 3:
            Menu_DrawLabelList(0x1CC, (char **) D_0080C404, 1, m->unk384, 0xFF106060);
            break;
        default:
            Menu_DrawLabelList(0x1CC, gDeliveryTexts, 2, m->unk384, 0xFF106060);
            break;
        }
        break;
    case 3:
        OfferMenu_DrawDelivery(x, y);
        break;
    case 2:
        OfferMenu_DrawQuest(x, y);
        break;
    }
}

s32 BaitMenu_IsOpen(void) {
    return gTownMode == 0x2C;
}

/* unk_6C0D00's .data (0x7E7010-0x7E7940) */
Offer gOfferMenuItems[18] = {
#include "lobby/007E7010.inc"
};
