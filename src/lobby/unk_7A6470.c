#include "common.h"
#include "quest.h"
#include "syssettings.h"
#include "em.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "objhead.h"
#include "save.h"
#include "pad.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_npc.h"
void *memset(void *, s32, u32);

void Bgm_ForgetCurrent(void);
void Bgm_PlayAreaMusic(u16);
void SeasonBanner_CreateOnline(s32, s32); /* unit's own declaration: with the definition's void (u8 a, u8 b) (sub_main/unk_386160.c), Town_StartAreaMusic 2 of 115 words differ */
s32 ElderRaid_InState1(void);
s32 LobbyTime_IsPm(void);
s32 LobbyWorld_GetSeason(void);
void Bgm_PlayField(s32, s32);
s32 Game_IsOnline(void);
extern s32 gTownMode;
u16 Vec_Heading(f32 *a, f32 *b);
AreaRecord *Stage_GetAreaRecord(u16 x);
void LobbySe_Play(s32);
u16 LobbyPad_Get(s32);
s32 LoadCompressedFile(u8 *dst, s32 id);
u8 Area_GetSetIndex(u16 id);
extern M2C_UNK gAreaFiles;

void Save_SetEventFlag(s32 flag);
s32 Save_TestEventFlag(s32 flag);
s32 MyHouse_IsOwner(void);
void LobbyProp_Spawn(void);
void Effect06_Start(s16 k);
void Effect17_SpawnArea(s32);

void Town_AreaEffects(u16 area)
{
    switch (area) {
    case 0xAC:
        if (Save_TestEventFlag(0x89) == 0) {
            Effect06_Start(0);
        }
        break;
    case 0x57:
        if (Save_TestEventFlag(0x8A) == 0) {
            Effect06_Start(0);
        }
        break;
    case 0xAE:
        if ((MyHouse_IsOwner() == 1) && (Save_TestEventFlag(0xB3) == 0)) {
            LobbyProp_Spawn();
            Save_SetEventFlag(0xB3);
        }
        break;
    case 0xAF:
        if ((MyHouse_IsOwner() == 1) && (Save_TestEventFlag(0xB4) == 0)) {
            LobbyProp_Spawn();
            Save_SetEventFlag(0xB4);
        }
        break;
    }
    Effect17_SpawnArea(area);
}

void Town_StartAreaMusic(s32 mode)
{
    s32 season;
    s32 temp_s0_2;
    s32 i;
    s32 cur;
    s32 played;

    played = 0;
    Bgm_ForgetCurrent();
    if (gScene.area == 0x87) {
        if (gTownMode == 0) {
            season = LobbyWorld_GetSeason() & 0xFF;
            SeasonBanner_CreateOnline(season, LobbyTime_IsPm() & 0xFF);
        }
        Bgm_PlayAreaMusic(gScene.area);
        return;
    }
    if (ElderRaid_InState1() == 0) {
        cur = LobbyWorld_GetSeason();
        i = LobbyTime_IsPm();
        if (Game_IsOnline() == 0) {
            if (gSysSettings.offline == 0) {
                Bgm_PlayField(1, cur);
                played = 1;
            } else if (cur != gLobbyLastSeason) {
                Bgm_PlayField(1, cur);
                played = 1;
            } else if (i != gLobbyLastPm) {
                Bgm_PlayField(2, i);
                played = 1;
            }
        } else if (mode == 1) {
            Bgm_PlayField(1, cur);
            played = 1;
        } else if (mode == 5) {
            if (cur != gLobbyLastSeason) {
                Bgm_PlayField(1, cur);
                played = 1;
            } else if (i != gLobbyLastPm) {
                Bgm_PlayField(2, i);
                played = 1;
            }
        }
    }
    if (played == 0) {
        Bgm_PlayAreaMusic(gScene.area);
    }
    if (gTownMode == 0) {
        temp_s0_2 = LobbyWorld_GetSeason() & 0xFF;
        SeasonBanner_CreateOnline(temp_s0_2, LobbyTime_IsPm() & 0xFF);
    }
}

void Town_LoadAreaFile(u16 area)
{
    s32 file;

    if (area != 0) {
        file = ((s32 *)&gAreaFiles)[Area_GetSetIndex(area)];
        if (file != -1) {
            LoadCompressedFile((u8 *) (*(s32 *)&gLbguildFree), file);
        }
    }
}

typedef struct AreaSet {
    s32 kind;
    s32 arg;
} AreaSet;
void Mem_Compact(void);
s32 Area_HasTownPcs(u16);
extern AreaSet *gTownAreaSets[];
extern NetUser gNetUsers[];
extern s32 gTownMenuOpen;
void TownPc_LoadAllModels(void);
void Anim_LoadCommon(s32 a);
s8 MyHouse_LocalSlot(void);
void EmSlot_LoadModel(s32 a, s32 b, s32 c);
void Area_Enter(u16 area);
void func_007A6460(s8 arg0);
void Camera_SetPreset(s32 preset);
void Area_Load(s32 area);
void Lobby_ReceiveWhileWaiting(void);
void TownPc_InitAll(s32 arg0);
void Light_InitAll(s32 unused0);
void Area_ApplyFog(u16 area);
void Area_StartObjects(u16 area);
void Town_PlaceLocalPlayer(u16 area); /* unit's own declaration: with the definition's void (s32 area) (lobby/unk_7A44D0.c), Town_EnterArea 78 of 269 words differ */
void TownMsg_Send02Zero(void);
void TownMsg_SendArea(u16 area);
void TownMsg_Send02(Player *obj);
void TownMsg_Send10(Player *p);
void TownMsg_SendStateNow(Player *p);
void TownPc_StartLoad(u8 *obj);
void CamCtl_Init(void);
void Players_UpdateMatrices(void);
void CamCtl_Update(s32);

/* enters area: returns 1 when it is the other half of the same place
   (0x56/0x57), where the area set-up is skipped */
u8 Town_EnterArea(u16 area, s32 v)
{
    Player *me = &gPlayers[gScene.localPlayer];
    AreaSet *set;
    u8 same = 0;
    s8 i;
    s16 k;
    u8 c;
    Player *p;

    if (gScene.area == 0x57) {
        if (gPlayers[gScene.localPlayer].nextArea == 0x56) {
            same = 1;
        }
    } else if (gScene.area == 0x56 && gPlayers[gScene.localPlayer].nextArea == 0x57) {
        same = 1;
    }
    me->area = area;
    if (same == 0) {
        if (gSysSettings.galleryPlay == 0) {
            Town_LoadAreaFile(area);
        }
        if (gLobby->modelLoaded[me->index] == 0) {
            TownPc_LoadAllModels();
        }
        if (gLobby->areaLoaded == 0) {
            gLobby->areaLoaded = 1;
            if (Game_IsOnline() == 1) {
                Anim_LoadCommon(2);
            } else {
                switch (area) {
                case 0x57:
                case 0x56:
                    Anim_LoadCommon(3);
                    break;
                default:
                    Anim_LoadCommon(1);
                    break;
                }
            }
        }
        Mem_Compact();
        k = Area_GetSetIndex(area);
        set = gTownAreaSets[k];
        for (i = 0; i < 4; i++, set++) {
            if (set->kind == -2) {
                if (Game_IsOnline() == 1) {
                    c = ((HunterCard *)gNetUsers[MyHouse_LocalSlot()].text)->poogieOutfit;
                    if (c != 0xFF) {
                        EmSlot_LoadModel(c + 0xD, 0, i);
                    } else {
                        EmSlot_LoadModel(0xD, 0, i);
                    }
                } else {
                    EmSlot_LoadModel(0xC, 0, i);
                }
            } else if (set->kind != -1) {
                EmSlot_LoadModel(set->kind, set->arg, i);
            }
        }
    }
    gTownMenuOpen = 0;
    Area_Enter(area);
    if (same == 0) {
        func_007A6460(1);
    }
    Camera_SetPreset(0);
    if (same == 0) {
        Area_Load(area);
        Lobby_ReceiveWhileWaiting();
        TownPc_InitAll(v);
    }
    Light_InitAll(area);
    gCameraPreset = 0xFF;
    Area_ApplyFog(area);
    Area_StartObjects(area);
    if (gTownMode != 0x22) {
        gTownMode = 0;
        me->areaMoveState = 0;
    }
    Town_AreaEffects(area);
    Town_PlaceLocalPlayer(area);
    gScene.area = area;
    if (Game_IsOnline() == 1) {
        TownMsg_Send02Zero();
        TownMsg_SendArea(gScene.area);
        TownMsg_Send02(me);
        TownMsg_Send10(me);
        TownMsg_SendStateNow(me);
    }
    if (Area_HasTownPcs(gScene.area) == 1) {
        for (i = 0, p = gPlayers; i < 10; i++, p++) {
            if (p->active != 0 && (s8)gLobby->pcLoaded[i] == 0) {
                TownPc_StartLoad((u8 *)p);
            }
        }
    }
    CamCtl_Init();
    Players_UpdateMatrices();
    CamCtl_Update(0);
    return same;
}

void Town_SetNpcArea(u16 x)
{
    s32 i;
    Em *p = gEms;

    for (i = 0; i < 20; i++, p++) {
        if (p->active != 0) {
            p->area = x;
        }
    }
}

extern u8 *gAreaNpcTables[];
extern u8 gAreaNpcCounts[];
/* unit's own declaration: with the definition's s32 (s32 idx, s32 id) (lobby/unk_77A960.c), Town_SpawnNpcs: 6 of 106 words differ */
s32 AreaNpc_IsPresent(s32 idx, u16 id);
Em *EmSlot_Alloc(void);
void Npc_SetupFromArea(Em *npc, u8 keep, s32 id);
s32 Area_IsHouse(u16 area);
s32 Village_NpcRecB(u16 id);

/* spawns the NPCs of the area (the byte at +0x1E is inside Em.roundsLoaded) */
void Town_SpawnNpcs(u16 area)
{
    u8 n;
    s32 i;
    s32 j;
    Em *e;
    u8 *ent;
    u8 k;

    n = gAreaNpcCounts[(u8)Area_GetSetIndex(area)];
    for (i = 0; i < n; i++) {
        if (AreaNpc_IsPresent(i, area) == 1) {
            e = EmSlot_Alloc();
            if (e != NULL) {
                *((u8 *)e + 0x1E) = 1;
                e->modelSlot = 0;
                e->index = i;
                e->area = gScene.area;
                Npc_SetupFromArea((Em *)((u8 *) e), 0, gScene.area);
            }
        }
    }
    if (Game_IsOnline() == 0 && Area_IsHouse(gScene.area) == 1) {
        k = Area_GetSetIndex(0x89);
        ent = gAreaNpcTables[k];
        if (ent == NULL) {
            return;
        }
        n = gAreaNpcCounts[k];
        for (j = 0; j < n; j++, ent += 0x48) {
            if (Village_NpcRecB(*ent) == 1) {
                e = EmSlot_Alloc();
                if (e != NULL) {
                    *((u8 *)e + 0x1E) = 1;
                    e->modelSlot = 0;
                    e->index = j;
                    e->area = gScene.area;
                    Npc_SetupFromArea((Em *)((u8 *) e), 0, 0x89);
                }
                break;
            }
        }
    }
    memset(&gTutorial, 0, 0x1C);
}

void DrawSlot_ResetAll(void);
void EmSlot_FreeAll(void);
void Mem_Compact(void);
void Obj_FreeAll(void);
s32 Area_HasTownPcs(u16);
void FreeList_InitAll(void);
void Motion_FreeCommon(void);
void Area_FreeAll(void);
void Notice_ResetQueue(void);

void Town_FreeArea(void)
{
    if (Area_HasTownPcs(gScene.area) == 1) {
        Notice_ResetQueue();
    }
    if (gScene.area == 0x57) {
        switch (gPlayers[gScene.localPlayer].nextArea) {
        case 0x56:
            return;
        case 0x89:
            Motion_FreeCommon();
            gLobby->areaLoaded = 0;
            break;
        }
    } else if (gScene.area == 0x56) {
        if (gPlayers[gScene.localPlayer].nextArea == 0x57) {
            return;
        }
    } else if (gScene.area == 0x89) {
        if (gPlayers[gScene.localPlayer].nextArea == 0x57) {
            Motion_FreeCommon();
            gLobby->areaLoaded = 0;
        }
    }
    EmSlot_FreeAll();
    Area_FreeAll();
    Obj_FreeAll();
    FreeList_InitAll();
    DrawSlot_ResetAll();
    Mem_Compact();
    gPlayers[gScene.localPlayer].spot = NULL;
}

s32 Actor_IsFacingWithin(u8 *pl, void *x, s32 deg)
{
    s32 diff;
    s32 range;

    diff = (((*(s32 *)((u8 *)pl + 0xA4) - (((Vec_Heading((f32 *) ((u8 *) (pl + 0xAC)), (f32 *) x) & 0xFFFF) + 0x4000) & 0xFFFF)) & 0xFFFF) - 0x8000) & 0xFFFF;
    range = (s32) (0.5f + ((65536.0f * (f32) deg) / 360.0f)) & 0xFFFF;
    if (diff > 0xFFFF - range || diff < range) {
        return 1;
    }
    return 0;
}

/* The heading from an object (a Player, an Em or an NPC: their shared header) to pos, less its
 * yaw, as a u16 angle. */
s32 Actor_HeadingDiff(void *obj, void *pos)
{
    return ((((Vec_Heading((f32 *) pos, ((ObjHead *)obj)->pos) & 0xFFFF) + 0x4000) & 0xFFFF) - ((ObjHead *)obj)->rot[1]) & 0xFFFF;
}

void Npc_UpdateAreaPos(Em *npc)
{
    AreaRecord *rec;

    rec = Stage_GetAreaRecord(npc->area);
    npc->mapPos[0] = npc->posX + rec->originX;
    npc->mapPos[1] = npc->posY + (rec->bottom + rec->top) / 2.0f;
    npc->mapPos[2] = npc->posZ + rec->originZ;
}

s32 Menu_WrapUpDown(s32 cur, s32 n)
{
    s32 pad;
    s32 ret;

    ret = cur;
    pad = LobbyPad_Get(0) & 0xFFFF;
    if (pad & 0x2000) {
        if (n > 1) {
            LobbySe_Play(1);
        }
        if (ret == 0) {
            ret = n - 1;
        } else {
            ret -= 1;
        }
    } else if (pad & 0x1000) {
        if (n > 1) {
            LobbySe_Play(1);
        }
        ret += 1;
        if (ret >= n) {
            ret = 0;
        }
    }
    return ret;
}

s32 Menu_WrapUpDownPad(s32 cur, s32 n, u16 pad)
{
    s32 btn;
    s32 ret;

    btn = pad & 0xFFFF;
    ret = cur;
    if (btn & 0x2000) {
        if (n > 1) {
            LobbySe_Play(1);
        }
        if (ret == 0) {
            ret = n - 1;
        } else {
            ret -= 1;
        }
    } else if (btn & 0x1000) {
        if (n > 1) {
            LobbySe_Play(1);
        }
        ret += 1;
        if (ret >= n) {
            ret = 0;
        }
    }
    return ret;
}

s32 Menu_WrapUpDownSe(s32 cur, s32 n, M2C_UNK se)
{
    s32 pad;
    s32 ret;

    ret = cur;
    pad = LobbyPad_Get(0) & 0xFFFF;
    if (pad & 0x2000) {
        if (n > 1) {
            LobbySe_Play(se);
        }
        if (ret == 0) {
            ret = n - 1;
        } else {
            ret -= 1;
        }
    } else if (pad & 0x1000) {
        if (n > 1) {
            LobbySe_Play(se);
        }
        ret += 1;
        if (ret >= n) {
            ret = 0;
        }
    }
    return ret;
}

s32 Menu_WrapLeftRight(s32 cur, s32 n)
{
    s32 pad;
    s32 ret;

    ret = cur;
    pad = LobbyPad_Get(0) & 0xFFFF;
    if (pad & 0x800) {
        if (n > 1) {
            LobbySe_Play(1);
        }
        if (ret == 0) {
            ret = n - 1;
        } else {
            ret -= 1;
        }
    } else if (pad & 0x400) {
        if (n > 1) {
            LobbySe_Play(1);
        }
        ret += 1;
        if (ret >= n) {
            ret = 0;
        }
    }
    return ret;
}

s32 Menu_WrapLeftRightSe(s32 cur, s32 n, M2C_UNK se)
{
    s32 pad;
    s32 ret;

    ret = cur;
    pad = LobbyPad_Get(0) & 0xFFFF;
    if (pad & 0x800) {
        if (n > 1) {
            LobbySe_Play(se);
        }
        if (ret == 0) {
            ret = n - 1;
        } else {
            ret -= 1;
        }
    } else if (pad & 0x400) {
        if (n > 1) {
            LobbySe_Play(se);
        }
        ret += 1;
        if (ret >= n) {
            ret = 0;
        }
    }
    return ret;
}

s32 Menu_WrapLeftRightPad(s32 cur, s32 n, u16 pad)
{
    s32 btn;
    s32 ret;

    btn = pad & 0xFFFF;
    ret = cur;
    if (btn & 0x800) {
        if (n > 1) {
            LobbySe_Play(1);
        }
        if (ret == 0) {
            ret = n - 1;
        } else {
            ret -= 1;
        }
    } else if (btn & 0x400) {
        if (n > 1) {
            LobbySe_Play(1);
        }
        ret += 1;
        if (ret >= n) {
            ret = 0;
        }
    }
    return ret;
}
