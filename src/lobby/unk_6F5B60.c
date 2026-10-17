/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "player.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"
#include "area.h"

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s16 gClockWobble;

void AreaMenu_Close(void) {
    gTownMode = 0;
    gTownMenuOpen = 0;
}

/* The area menu (gMenuWork): two destinations (AREA_HUB_0x85, AREA_HUB_0x86, AREA_HUB_0x88). */
typedef struct AreaMenu {
    /* 0x0 */ s8 step;
    /* 0x1 */ s8 sel;
    /* 0x2 */ u16 area[2];
    /* 0x6 */ u16 unk6;
    /* 0x8 */ u16 unk8;
} AreaMenu;
extern u8 gMenuWork[];
extern LobbyMenu *gLobbyMenu;
u16 LobbyPad_Get(s32 a);
s32 Cond_Check(u16 id);
void LobbyText_Open(s32 a);
void LobbyDialog_Grow(void);
void Menu_SetYesNo(u8 a); /* unit's own declaration: with the definition's void (s8 choice) (lobby/unk_7B0D80.c), AreaMenu_Update 1 of 132 words differ */
void LobbySe_Play(s32 se);
s32 Menu_WrapUpDown(s32 cur, s32 n);
void AreaMenu_GoTo(u16 area);
void AreaMenu_Update(void)
{
    AreaMenu *m = (AreaMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    switch (m->step) {
    case 0:
        switch (m->unk8) {
        case 0x85:
            if (m->unk6 == 0x86) {
                if (Cond_Check(0x64) == 0) {
                    LobbyText_Open(0x7E);
                } else {
                    LobbyText_Open(0x7F);
                }
                LobbySe_Play(8);
                LobbyDialog_Grow();
                m->step++;
                m->sel = 0;
                m->area[0] = 0x85;
                m->area[1] = 0x88;
                break;
            }
            goto other;
        case 0x86:
            if (m->unk6 == 0x85) {
                if (Cond_Check(0x64) == 0) {
                    LobbyText_Open(0x80);
                } else {
                    LobbyText_Open(0x81);
                }
                LobbySe_Play(8);
                LobbyDialog_Grow();
                m->step++;
                m->sel = 0;
                m->area[0] = 0x86;
                m->area[1] = 0x88;
                break;
            }
        default:
        other:
            AreaMenu_GoTo(m->unk8);
            break;
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            if (m->sel == 1 && Cond_Check(0x64) == 0) {
                LobbySe_Play(9);
            } else {
                LobbySe_Play(0);
                AreaMenu_GoTo(m->area[m->sel]);
            }
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            AreaMenu_Close();
        } else {
            m->sel = Menu_WrapUpDown(m->sel, 2);
            Menu_SetYesNo(m->sel);
        }
        break;
    }
}

typedef struct SpawnPt {
    f32 pos[3];
    u16 angle;
    u16 pad;
} SpawnPt;
extern SpawnPt gSpawnPoints[];
extern SpawnPt gAreaGoSpawn89FromAA;
extern SpawnPt gAreaGoSpawn86;
extern SpawnPt gAreaGoSpawn85;
extern SpawnPt gAreaGoSpawn84From87;
extern SpawnPt gAreaGoSpawn84From88;
extern SpawnPt gAreaGoSpawn84From85;
extern SpawnPt gAreaGoSpawn88From85;
extern SpawnPt gAreaGoSpawn88From86;
extern SpawnPt gAreaGoSpawn84FromAD;
extern SpawnPt gAreaGoSpawn87;
extern SpawnPt gAreaGoSpawn57;
extern s32 gLobbyStartPosIdx;
extern s8 gLobbyScreenMode;
extern s8 gLobbyScreenStep;
extern s8 gTownSpawnSet;
s32 Game_IsOnline(void);
void QuestList_Rebuild(void);
u8 Village_FacilityLevel(s32 k);
void Save_ClearEventFlag(s32 flag);
void Game_ResetSubsystems(void);
s32 MyHouse_IsOwner(void);
u8 Area_GetSetIndex(u16 id);
void Player_AddSpawnOffset(Player *p, f32 *pos, u16 angle);

void AreaMenu_GoTo(u16 area)
{
    AreaMenu *m = (AreaMenu *)gMenuWork;
    Player *p = &gPlayers[gScene.localPlayer];
    SpawnPt *s = NULL;
    u16 idx;

    switch (area) {
    case AREA_HUB_0x89:
        if (Game_IsOnline() == 1) {
            area = AREA_HUB_0x84;
            switch (m->unk6) {
            case 0xAD:
            case 0xAE:
            case 0xAF:
                s = &gAreaGoSpawn84FromAD;
                break;
            }
        } else {
            switch (m->unk6) {
            case 0xAA:
            case 0xAB:
            case 0xAC:
                s = &gAreaGoSpawn89FromAA;
                break;
            case 0x57:
                Save_ClearEventFlag(0x1CE);
                gSave.unk36E8 = 0;
                gLobbyStartPosIdx = -1;
                QuestList_Rebuild();
                Game_ResetSubsystems();
                gLobbyScreenMode = 0;
                gLobbyScreenStep = 0;
                return;
            }
        }
        break;
    case AREA_HUB_0x57:
        gSave.unk36E8 = 1;
        s = &gAreaGoSpawn57;
        if (m->unk6 == 0x89) {
            gLobbyStartPosIdx = -1;
            QuestList_Rebuild();
        }
        break;
    case AREA_HUB_0x86:
        if (m->unk6 == 0x85 || m->unk6 == 0x88) {
            s = &gAreaGoSpawn86;
        }
        break;
    case AREA_HUB_0x87:
        if (p->pos[2] < 3500.0f) {
            s = &gAreaGoSpawn87;
        }
        break;
    case AREA_HUB_0x88:
        switch (m->unk6) {
        case 0x85:
            s = &gAreaGoSpawn88From85;
            break;
        case 0x86:
            s = &gAreaGoSpawn88From86;
            break;
        }
        break;
    case AREA_ONLINE_HOUSE_0:
        if (Game_IsOnline() == 0 || MyHouse_IsOwner() == 1) {
            area = (u8)Village_FacilityLevel(4) + 0xAD;
        }
        break;
    case AREA_HUB_0xAA:
        area = (u8)Village_FacilityLevel(0) / 3 + 0xAA;
        break;
    case AREA_HUB_0x85:
        if (m->unk6 != 0x84) {
            s = &gAreaGoSpawn85;
        }
        break;
    case AREA_HUB_0x84:
        switch (m->unk6) {
        case 0x88:
            s = &gAreaGoSpawn84From88;
            break;
        case 0x87:
            s = &gAreaGoSpawn84From87;
            break;
        case 0x85:
            s = &gAreaGoSpawn84From85;
            break;
        }
        break;
    }
    idx = Area_GetSetIndex(area);
    if (s == NULL) {
        s = &gSpawnPoints[idx];
    }
    p->startPos[0] = s->pos[0];
    p->startPos[1] = s->pos[1];
    p->startPos[2] = s->pos[2];
    p->startAngle = s->angle + 0x4000;
    Player_AddSpawnOffset(p, p->startPos, p->startAngle);
    gLobbyScreenMode = 3;
    gTownSpawnSet = 1;
    p->nextArea = area;
    gTownMode = 0x15;
}

void VillageClock_Wobble(void) {
    gClockWobble = 0xF;
}

/* unk_6F5B60's .data (0x7EBE10-0x7EBFF0) */
SpawnPt gSpawnPoints[15] = {
#include "lobby/007EBE10.inc"
};
SpawnPt gAreaGoSpawn89FromAA = {
#include "lobby/007EBF00.inc"
};
u32 D_007EBF10[4] = {
#include "lobby/007EBF10.inc"
};
SpawnPt gAreaGoSpawn86 = {
#include "lobby/007EBF20.inc"
};
SpawnPt gAreaGoSpawn85 = {
#include "lobby/007EBF30.inc"
};
SpawnPt gAreaGoSpawn84From87 = {
#include "lobby/007EBF40.inc"
};
SpawnPt gAreaGoSpawn84From88 = {
#include "lobby/007EBF50.inc"
};
SpawnPt gAreaGoSpawn84From85 = {
#include "lobby/007EBF60.inc"
};
SpawnPt gAreaGoSpawn88From85 = {
#include "lobby/007EBF70.inc"
};
SpawnPt gAreaGoSpawn88From86 = {
#include "lobby/007EBF80.inc"
};
SpawnPt gAreaGoSpawn84FromAD = {
#include "lobby/007EBF90.inc"
};
SpawnPt gAreaGoSpawn87 = {
#include "lobby/007EBFA0.inc"
};
SpawnPt gAreaGoSpawn57 = {
#include "lobby/007EBFB0.inc"
};
u32 D_007EBFC0[12] = {
#include "lobby/007EBFC0.inc"
};
