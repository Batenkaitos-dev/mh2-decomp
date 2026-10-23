/*
 * The town's lock-on: the local player's target among the townspeople and
 * the other shown players, run each frame by LockOn_Update. The candidates
 * in reach go into a list ordered by their sector (the angle from the
 * player) and distance; the d-pad cycles through it, and the action spot
 * the player stands at is the fallback target.
 *
 * LockOn is the view of an actor's ObjHead these functions use.
 *
 * Built at optimization level 4, as the town players' other code.
 */

#include "common.h"
#include "lobby_menu.h"
#include "quest.h"
#include "player.h"
#include "em.h"
#include "pad.h"
#include "lobby_npc.h"
#include "town_lock_on.h"
#include "engine_math.h"
#include "game_util.h"
#include "hud.h"
#include "lobby_util.h"
#include "my_house.h"
#include "town_area_change.h"
#include "town_players.h"
#include "libc.h"

#pragma optimization_level 4

extern s32 gTownMode;
extern s32 gTownMenuOpen;

/*
 * Inserts candidate obj into the lock-on list, ordered by sector and then
 * by distance (nearest first in sectors 0-2, farthest first in 3-5).
 */
void LockOn_Insert(LockOn **list, LockOn *cand)
{
    LockOn *cur;
    LockOn *prev;
    s32 prioSector;
    LockOn *head;

    if (list == NULL) {
        *list = cand;
        return;
    }
    head = *list;
    prev = NULL;
    cur = head;
    if (cur != NULL) {
        prioSector = cand->sector;
        do {
            if ((u16)cur->sector > prioSector) {
                break;
            }
            if ((u16)cur->sector == prioSector) {
                if (prioSector < 3) {
                    if (!(cur->dist <= cand->dist)) {
                        break;
                    }
                } else if (cur->dist < cand->dist) {
                    break;
                }
            }
            prev = cur;
            cur = cur->next;
        } while (cur != NULL);
    }
    if (prev == NULL) {
        cand->next = head;
        *list = cand;
        return;
    }
    cand->next = prev->next;
    prev->next = cand;
}

/*
 * Sets the candidate's sector (0 to 5) from its angle off the player's
 * heading within the half arc range; with a target already locked, the
 * cycle direction (1 or 2) decides from which side the sectors are
 * counted.
 */
void LockOn_SetSector(Player *pl, LockOn *cand, u32 angle, u32 halfArc, u8 cycleDir)
{
    if (pl->target != 0 && cycleDir != 0) {
        if (cycleDir == 1) {
            if ((u16)angle < (u16)halfArc) {
                angle = (u16)(s32)(angle * (1.0f / (halfArc / 3.0f)));
            } else {
                angle = (u16)(u32)(6.0f - (s32)((0xFFFF - (u16)angle) * (1.0f / (halfArc / 3.0f))) - 1.0f);
            }
        } else {
            if ((u16)angle < (u16)halfArc) {
                angle = (u16)(u32)(6.0f - (s32)(angle * (1.0f / (halfArc / 3.0f))) - 1.0f);
            } else {
                angle = (u16)(s32)((0xFFFF - (u16)angle) * (1.0f / (halfArc / 3.0f)));
            }
        }
    } else {
        if ((u16)angle < (u16)halfArc) {
            angle = (u16)(s32)(angle * (1.0f / (halfArc / 3.0f)));
        } else {
            angle = (u16)(s32)((0xFFFF - (u16)angle) * (1.0f / (halfArc / 3.0f)));
        }
    }
    cand->sector = angle;
}

/* unit's own declaration: LockOn_Insert reads a1 = arg1, the candidate (left by LockOn_SetSector(arg0, arg1, ...), which keeps a1) */
void LockOn_Insert();

/*
 * Tests a lock-on candidate: within 200 in height, inside the half arc
 * range of the player's heading and nearer than maxDist. Then returns 1
 * and inserts it in the list with its sector and distance; else marks it
 * out of reach (sector -1) and returns 0.
 */
s32 LockOn_TestCandidate(Player *pl, LockOn *cand, LockOn **list, s32 halfArc, u8 cycleDir, f32 maxDist)
{
    f32 dist;
    s32 arc;
    s32 angle;
    s32 angle16;

    cand->next = NULL;
    cand->targeted = 0;
    if (!((f32) abs((s32) (cand->pos[1] - pl->pos[1])) <= 200.0f)) {
        cand->sector = -1;
        return 0;
    }
    angle = Actor_HeadingDiff(pl, cand->pos) & 0xFFFF;
    dist = Vec_Distance(&pl->pos[0], cand->pos);
    arc = halfArc & 0xFFFF;
    angle16 = angle & 0xFFFF;
    if (((angle16 < arc) || ((0xFFFF - arc) < angle16)) && (dist < maxDist)) {
        cand->dist = dist;
        LockOn_SetSector((Player *)((u8 *) pl), cand, angle, halfArc, cycleDir);
        LockOn_Insert(list);
        return 1;
    }
    cand->sector = -1;
    return 0;
}

s32 Kbd_IsOpen(void); /* unit's own declaration: the definition returns s8; with s8 here, LockOn_Update's test of it compiles differently */
s32 Npc_IsPet(Em *em);
extern u8 gLockOnCycleDir;
extern s8 gLockOnMarkSkip;
extern s8 gLockOnNoCycle;

/*
 * Each frame, picks the local player's lock-on target among the
 * townspeople and the other shown players, or the action spot it stands at
 * (the player itself as target). PAD_LEFT and PAD_RIGHT cycle the target,
 * PAD_CROSS goes back to the spot, and the lock-on sound plays when the
 * target changes. Suspended while a menu or the keyboard is open, in town
 * modes other than free and quest departure, and while crouched.
 */
void LockOn_Update(void)
{
    Em *em;
    Player *localPl;
    NpcWork *work;
    s32 i;
    u16 count;
    LockOn *old;
    f32 dist;
    LockOn *target;
    Player *player;
    Player *local;

    target = NULL;
    localPl = &gPlayers[gScene.localPlayer];
    old = ((LockOn *)localPl->target);
    em = gEms;
    if (Hud_ItemGetBusy() == 1) {
        localPl->target = NULL;
        return;
    }
    if (gTownMenuOpen == 1 || (gTownMode != TOWN_MODE_FREE && gTownMode != TOWN_MODE_SET_OUT)) {
        if (gScene.menuOpen != 0 || Kbd_IsOpen() != 0 || gTownMode != TOWN_MODE_FREE) {
            gLockOnMarkSkip = 1;
        }
        return;
    }
    if (gTownMode == TOWN_MODE_SET_OUT) {
        if (localPl->spot == NULL) {
            gLockOnMarkSkip = 1;
            return;
        }
        if (((ActionSpot *)localPl->spot)->kind != 17) {
            gLockOnMarkSkip = 1;
            return;
        }
    }
    if (localPl->action == 1) {
        localPl->target = (LockOn *)localPl;
        return;
    }
    if (localPl->input.pressed & PAD_LEFT) {
        gLockOnCycleDir = 2;
    } else if (localPl->input.pressed & PAD_RIGHT) {
        gLockOnCycleDir = 1;
    }
    count = 0;
    for (i = 0; i < 20; i++, em++) {
        if (em->active != 0) {
            if (Area_IsOnlineHouse(gScene.area) != 0 || Npc_IsPet(em) != 0) {
                dist = 200.0f;
            } else {
                dist = 400.0f;
            }
            work = (NpcWork *)&em->work;
            if (Game_IsOnline() == 1 && MyHouse_InOwnHouse() == 0 && work->id == 20) {
                continue;
            }
            if ((s8)work->noTalk == 0) {
                if (em->variant == 77) {
                    count += (u8)LockOn_TestCandidate(localPl, (LockOn *)em, &target, 10923, gLockOnCycleDir, 1.5f * dist);
                } else {
                    count += (u8)LockOn_TestCandidate(localPl, (LockOn *)em, &target, 10923, gLockOnCycleDir, dist);
                }
            }
        }
    }
    player = gPlayers;
    local = &gPlayers[gScene.localPlayer];
    for (i = 0; i < 10; i++, player++) {
        if (player->active != 0 && player->index != gScene.localPlayer && (u8)TownPc_IsInMyArea(player) &&
            gLobby->modelLoaded[i] != 0) {
            count += (u8)LockOn_TestCandidate((Player *)local, (LockOn *)player, &target, 9102, gLockOnCycleDir, 300.0f);
        }
    }
    if (count >= 2 || (count > 0 && local->spot != NULL)) {
        gLockOnNoCycle = 0;
    } else {
        gLockOnNoCycle = 1;
    }
    if (gTownMode == TOWN_MODE_SET_OUT) {
        target = NULL;
    }
    if (target == NULL) {
        if (local->spot == NULL) {
            if (((LockOn *)local->target) != NULL) {
                ((LockOn *)local->target)->targeted = 0;
            }
            local->target = NULL;
            return;
        }
        local->target = (LockOn *)local;
        return;
    }
    if (((LockOn *)local->target) != NULL) {
        ((LockOn *)local->target)->targeted = 0;
        if ((local->input.pressed & PAD_CROSS) && local->spot != NULL) {
            local->target = (LockOn *)local;
        } else if (local->input.pressed & (PAD_LEFT | PAD_RIGHT)) {
            LockOn *cur = ((LockOn *)local->target);
            LockOn *next = cur->next;
            if (next == NULL && local->spot == NULL) {
                local->target = target;
            } else if (next == NULL && local->spot != NULL) {
                local->target = (LockOn *)local;
            } else if (cur == (LockOn *)local) {
                local->target = target;
            } else {
                local->target = next;
            }
        } else {
            LockOn *cur = ((LockOn *)local->target);
            if (cur->sector == -1) {
                local->target = target;
            } else if (local->spot == NULL && cur == (LockOn *)local) {
                local->target = target;
            }
        }
        if (old != ((LockOn *)local->target)) {
            LobbySe_Play(12);
        }
    } else {
        local->target = target;
        LobbySe_Play(12);
    }
    ((LockOn *)local->target)->targeted = 1;
}
