/*
 * The camera's small requests: one function per camera script that the game's other
 * code asks for (CamReq_Set stores the request, CAM_REQ_*, in gCamCtl.request and its
 * object in gCamCtl.requestArg, and the camera's state machine plays the script).
 * Some pick the request from a monster's kind or the area, some add the caller's index
 * to a base request. Most cancel the running request first (CamReq_Cancel).
 */
#include "common.h"
#include "scene.h"
#include "em.h"
#include "net_menu.h"
#include "area.h"
#include "camera.h"
#include "quest.h"
#include "save.h"
#include "libc.h"
#include "cam_helpers.h"

extern u8 gStageCamReqs[];
extern u8 gCamRequests[188];
extern u8 gSysBusy;
extern u8 gQuestTargetCamIds[];

/*
 * Starts the camera request that shows the quest's monster: CamReq_Start
 * when flag is 0 (CamReq_TargetDown), else CamReq_Change
 * (CamReq_BossHunted), with the monster as the request's object. Nothing
 * when the latest done goal (gQuest.goalIdx, from the end) is not a
 * monster goal (QUEST_GOAL_MONSTER) or not of downEm's kind. The monster
 * is gQuest.downEm, else gQuest.spawnedEm when that is a Lao-Shan Lung,
 * Fatalis or Shen Gaoren kind; it must be in the current area. Request
 * CAM_REQ_LAO_SHAN_DOWN for the Lao-Shan Lungs in AREA_FORTRESS_5 (by Area_DayId), CAM_REQ_FATALIS_DOWN
 * for the Fatalis kinds, CAM_REQ_SHEN_GAOREN_DOWN for Shen Gaoren, CAM_REQ_ELDER_DOWN for Kushala Daora, its
 * rusted kind, Chameleos, Lunastra and Teostra, each only when no monster
 * is left (emLeft 0). The other kinds take gQuestTargetCamIds by family
 * (MonsterGroup_First), none for CAM_REQ_NONE; Plesioth and Green Plesioth get
 * none in EM_GROUP_DEFEAT state 2.
 */
void CamReq_QuestTarget(s32 flag)
{
    Em *em;
    s32 i;
    u8 goalNo;
    s32 req;

    for (i = 2; i >= 0; i--) {
        if (gQuest.goalIdx[i] != QUEST_GOAL_NONE) {
            goalNo = gQuest.goalIdx[i];
            if (!(gQuest.goals[goalNo].flags & QUEST_GOAL_MONSTER)) {
                return;
            }
            if (gQuest.downEm != NULL && gQuest.downEm->type != gQuest.goals[goalNo].target) {
                return;
            }
            break;
        }
    }
    em = gQuest.downEm;
    if (em == NULL) {
        em = gQuest.spawnedEm;
        if (em == NULL) {
            return;
        }
        if (em->type != EM_KIND_LAO_SHAN_LUNG && em->type != EM_KIND_ASHEN_LAO_SHAN_LUNG && em->type != EM_KIND_FATALIS && em->type != EM_KIND_FATALIS_B && em->type != EM_KIND_FATALIS_C && em->type != EM_KIND_SHEN_GAOREN) {
            return;
        }
    }
    if (gScene.area != em->area) {
        return;
    }
    switch (em->type) {
    case EM_KIND_LAO_SHAN_LUNG:
    case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        if (Area_DayId(gScene.area) != AREA_FORTRESS_5) {
            return;
        }
        if (gQuest.emLeft == 0) {
            req = CAM_REQ_LAO_SHAN_DOWN;
        } else {
            return;
        }
        break;
    case EM_KIND_FATALIS:
    case EM_KIND_FATALIS_B:
    case EM_KIND_FATALIS_C:
        if (gQuest.emLeft == 0) {
            req = CAM_REQ_FATALIS_DOWN;
        } else {
            return;
        }
        break;
    case EM_KIND_SHEN_GAOREN:
        if (gQuest.emLeft == 0) {
            req = CAM_REQ_SHEN_GAOREN_DOWN;
        } else {
            return;
        }
        break;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_CHAMELEOS:
        if (gQuest.emLeft == 0) {
            req = CAM_REQ_ELDER_DOWN;
        } else {
            return;
        }
        break;
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        if (gQuest.emLeft == 0) {
            req = 7;
        } else {
            return;
        }
        break;
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
        if (em->stateGroup == EM_GROUP_DEFEAT && em->state == 2) {
            return;
        }
    default:
        req = gQuestTargetCamIds[MonsterGroup_First(em->type)];
        if (req == CAM_REQ_NONE) {
            return;
        }
        break;
    }
    if (flag == 0) {
        CamReq_Start(req, em);
    } else {
        CamReq_Change(req, em);
    }
}

/* The view of the quest's target monster when it goes down (CamReq_QuestTarget(0)). */
void CamReq_TargetDown(void)
{
    CamReq_QuestTarget(0);
}

/* The view of the quest's target monster when it is hunted (CamReq_QuestTarget(1)). */
void CamReq_BossHunted(void)
{
    CamReq_QuestTarget(1);
}

/* Requests em's own view by its kind (the elder dragons, Lao-Shan Lung, Shen Gaoren,
 * Fatalis); other kinds have none. */
void CamReq_ForMonster(Em *em)
{
    s32 req;

    switch (em->type) {
    case EM_KIND_LAO_SHAN_LUNG:
    case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        req = CAM_REQ_LAO_SHAN;
        break;
    case EM_KIND_FATALIS:
    case EM_KIND_FATALIS_B:
    case EM_KIND_FATALIS_C:
        if (em->stance != EM_STANCE_AIR) {
            req = CAM_REQ_FATALIS;
        } else {
            req = CAM_REQ_FATALIS_AIR;
        }
        break;
    case EM_KIND_SHEN_GAOREN:
        req = CAM_REQ_SHEN_GAOREN;
        break;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_CHAMELEOS:
        req = CAM_REQ_ELDER_DRAGON;
        break;
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        req = CAM_REQ_LUNASTRA;
        break;
    default:
        return;
    }
    CamReq_Set(req, em);
}

/* The player fainted or the quest ended: its view, unless the camera is on the area's
 * rail. */
void CamReq_PlayerDown(void)
{
    if (CamCtl_IsRailMode() != 1) {
        CamReq_Set(CAM_REQ_PLAYER_DOWN, NULL);
    }
}

/* The player was carted: its view, except in the battle town. */
void CamReq_Cart(void)
{
    if (gScene.map != MAP_BATTLE_TOWN && gScene.map != MAP_BATTLE_TOWN_NIGHT) {
        CamReq_Set(CAM_REQ_CART, NULL);
    }
}

/* The opening view of the three areas that have one (the quest's start). */
void CamReq_AreaIntro(void)
{
    s32 req;

    switch (Area_DayId(gScene.area)) {
    case AREA_FORTRESS_5:
        req = CAM_REQ_FORTRESS_INTRO;
        break;
    case AREA_CASTLE_1:
        req = CAM_REQ_CASTLE_INTRO;
        break;
    case AREA_BATTLE_TOWN_1:
        req = CAM_REQ_BATTLE_TOWN_INTRO;
        break;
    default:
        return;
    }
    CamReq_Set(req, NULL);
}

void CamReq_PlesiothScene(void *em)
{
    CamReq_Set(CAM_REQ_PLESIOTH, em);
}

void CamReq_LobbyEvent(void)
{
    CamReq_Set(CAM_REQ_LOBBY_EVENT, NULL);
}

void CamReq_TownPc(void)
{
    CamReq_Set(CAM_REQ_TOWN_PC, NULL);
}

/* The view of contest contest (0-2): its own at spot 0, else its spot view. */
void CamReq_Contest(s32 contest, s32 spot)
{
    if (spot == 0) {
        contest += CAM_REQ_CONTEST;
    } else {
        contest += CAM_REQ_CONTEST_SPOT;
    }
    CamReq_Cancel();
    CamReq_Set(contest, NULL);
}

void CamReq_Minigame(s32 game)
{
    CamReq_Cancel();
    CamReq_Set(game + CAM_REQ_MINIGAME, NULL);
}

void CamReq_Workshop(s32 level)
{
    CamReq_Set(level + CAM_REQ_WORKSHOP, NULL);
}

void CamReq_HairMenu(void)
{
    CamReq_Set(CAM_REQ_HAIR_MENU, NULL);
}

void CamReq_TownArea(s32 area)
{
    CamReq_Set(area + CAM_REQ_TOWN_AREA, NULL);
}

void CamReq_VillageBoard(void)
{
    CamReq_Set(CAM_REQ_VILLAGE_BOARD, NULL);
}

void CamReq_Kitchen(s32 index)
{
    CamReq_Set(index + CAM_REQ_KITCHEN, NULL);
}

/* The lobby stage's show starts (the lobby's music player turns its camera on). */
void CamReq_StageStart(void)
{
    CamReq_Cancel();
    CamReq_Set(CAM_REQ_STAGE, NULL);
}

/* Shot shot of the lobby stage's track track: the track's first request
 * (gStageCamReqs) + shot. */
void CamReq_StageShot(u16 track, u16 shot)
{
    CamReq_Cancel();
    CamReq_Set(gStageCamReqs[track] + shot, NULL);
}

/* A versus quest's defeat view (view 2 or 3) of monster em (Versus_DefeatCamera). */
void CamReq_VersusDefeat(void *em, u8 view)
{
    CamReq_Cancel();
    CamReq_Set(view + CAM_REQ_VERSUS, em);
}

void CamReq_TownTour(s32 tour)
{
    CamReq_Set(tour + CAM_REQ_TOWN_TOUR, NULL);
}

/* Clears the card menu (gCardMenu), gCamRequests and gSysBusy: the boot task's start
 * (Select_MainTask). */
void Boot_ResetMenus(void)
{
    memset(&gCardMenu, 0, sizeof(gCardMenu));
    memset(gCamRequests, 0, sizeof(gCamRequests));
    gSysBusy = 0;
}

/* Returns 1 and does nothing else. The options menu calls it on its way out, after
 * resetting the settings, the streams and the vibration. */
s32 CamReq_ResetStub(void)
{
    return 1;
}

/* This file's initialised data. */
u8 gQuestTargetCamIds[80] = {
#include "sub_main/0041CE60.inc"
};
u8 gStageCamReqs[16] = {
#include "sub_main/0041CEB0.inc"
};
u8 gEffectColors[8][4] = {
#include "sub_main/0041CEC0.inc"
};
u8 gFishColors[10][3] = {
#include "sub_main/0041CEE0.inc"
};
