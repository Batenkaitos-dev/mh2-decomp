/*
 * The camera controller's core (CamCtl gCamCtl; camera.h): clearing and
 * readying the controller, dropping a pending request, relocating the
 * camera data's offsets, and the frame. CamCtl_Update picks the mode and
 * runs each mode's init and update handlers from gCamModeInit and
 * gCamModeUpdate (CAM_MODE_*); CamCtl_Apply runs the script mode and hands
 * its view to the camera.
 */
#include "common.h"
#include "em.h"
#include "scene.h"
#include "player.h"
#include "camera.h"
#include "cam_areas.h"
#include "engine_math.h"
#include "mem_util.h"
#include "cam_helpers.h"

/* end of prototypes */

/* Clears the whole camera controller gCamCtl. */
void CamCtl_Clear(void)
{
    Mem_Fill(&gCamCtl, 0U, 1664);
}

/*
 * Drops any pending camera request and puts the follow camera back on the
 * default distance setting (CAM_DIST_DEFAULT).
 */
void CamReq_Reset(void)
{
    gCamCtl.request = 0;
    gCamCtl.requestArg = NULL;
    gCamCtl.follow.distNo = CAM_DIST_DEFAULT;
}

/*
 * Readies the camera controller for a scene: the next CamCtl_Update sets
 * the modes up again; the shake requests, the target object, the view flags
 * and the request are cleared; the local player becomes the one followed,
 * and CamCtl_LoadParams takes the grid parameters from the camera data.
 */
void CamCtl_Init(void)
{
    *(s32 *)&gCamCtl.inited = 0; /* a word store: inited and its padding */
    gCamCtl.shakeC.active = 0;
    gCamCtl.shakeB.active = 0;
    gCamCtl.shakeA.active = 0;
    gCamCtl.targetObj = NULL;
    gCamCtl.playerInView = 0;
    gCamCtl.followView = 0;
    gCamCtl.request = 0;
    gCamCtl.requestArg = NULL;
    gCamCtl.player = &gPlayers[gScene.localPlayer];
    CamCtl_LoadParams(&gCamCtl);
}

/*
 * Keeps the loaded camera data hdr in gCamCtl.data (NULL: none) and turns
 * the file's offsets into pointers: each area's faces and transition table,
 * the grid cells, each cell's area list and the area pointers in the lists.
 */
void CamData_Relocate(CamDataHdr *hdr)
{
    CamArea *area;
    CamCell *cell;
    CamArea **areaList;
    s32 left;
    s32 listTotal;

    gCamCtl.data = hdr; /* the offsets in the file become pointers below */
    if (hdr == NULL) {
        return;
    }
    area = (CamArea *)(hdr + 1);
    hdr->areas = area;
    left = hdr->areaCount;
    if (left != 0) {
        do {
            if (area->trans != NULL) {
                area->faces = (CamFace *)((u8 *)area + (s32)area->faces);
                area->trans = (CamTrans *)((u8 *)area + (s32)area->trans);
                area = (CamArea *)((u8 *)((s32)(area + 1) + area->faceCount * 64) + 32);
            } else {
                area->faces = (CamFace *)((u8 *)area + (s32)area->faces);
                area = (CamArea *)((s32)(area + 1) + area->faceCount * 64);
            }
        } while (--left != 0);
    }
    cell = (CamCell *)((u8 *)hdr + (s32)hdr->cells);
    hdr->cells = cell;
    areaList = (CamArea **)((u8 *)hdr + (s32)hdr->areaLists);
    hdr->areaLists = areaList;
    listTotal = hdr->gridH;
    left = hdr->gridW * listTotal;
    listTotal = 0;
    if (left != 0) {
        do {
            if (cell->count != 0) {
                cell->areas = (CamArea **)((u8 *)hdr + (s32)cell->areas);
                listTotal += cell->count;
            }
            cell++;
        } while (--left != 0);
    }
    if (listTotal != 0) {
        do {
            *areaList = (CamArea *)((u8 *)hdr + (s32)*areaList);
            listTotal--;
            areaList++;
        } while (listTotal != 0);
    }
}

/*
 * The camera controller's frame. mode (CamCtl.mode) holds flags that switch
 * off the monster push (bit 0, Cam_PushOutOfEms) and the wall line test
 * (bit 1, Cam_LineHitDist). Nothing happens while the followed player is
 * inactive. On the first frame it finds the player's camera area and runs
 * each mode's init handler (gCamModeInit). Every frame it tracks the area
 * the player is in and picks areaMode from the area's kind (CAM_AREA_FOLLOW
 * the follow camera, the others the rail), runs the modes' update handlers
 * in order, and CamCtl_RunMode hands the current or the overriding view to
 * the camera. The target object request lasts one frame.
 */
void CamCtl_Update(s32 mode)
{
    CamCtl *ctl = (CamCtl *)&gCamCtl;
    CamView *view;
    Player *pl;
    u32 i;

    gCamCtl.mode = mode;
    gCamCtl.followView = 0;
    if ((pl = gCamCtl.player)->active == 0) {
        return;
    }
    CamCtl_ReadPad(ctl);
    ctl->keepFirstArea = 0;
    ctl->cellZ = 0;
    ctl->cellX = 0;
    ctl->areaChanged = 0;
    if (ctl->inited == 0) {
        ctl->inited++;
        ctl->area = NULL;
        ctl->prevAreaId = CAM_AREA_ID_NONE;
        ctl->areaId = CAM_AREA_ID_NONE;
        Cam_FindPlayerArea(ctl);
        ctl->areaId = ctl->area->id;
        ctl->areaMode = CAM_MODE_FOLLOW;
        if (ctl->area->kind == CAM_AREA_FOLLOW) {
            ctl->areaMode = CAM_MODE_FOLLOW;
        } else {
            ctl->areaMode = CAM_MODE_RAIL;
        }
        ctl->prevAreaMode = ctl->areaMode;
        view = ctl->modes;
        for (i = 0; i < CAM_MODE_COUNT; i++, view++) {
            view->kind = i;
            *(s32 *)&view->step = 0;
            gCamModeInit[i](ctl, view);
        }
    }
    if (ctl->areaId == 0) {
        if (Cam_FindPlayerArea(ctl) > 0) {
            ctl->prevAreaId = ctl->areaId;
            ctl->areaId = ctl->area->id;
            ctl->areaChanged = 1;
        }
    } else if (CamArea_TestObj(ctl->area, pl, 2) != 0) {
        ctl->prevAreaId = ctl->areaId;
        ctl->areaChanged = 1;
        if (Cam_FindPlayerArea(ctl) >= 0) {
            ctl->areaId = ctl->area->id;
        }
    }
    ctl->prevAreaMode = ctl->areaMode;
    switch (ctl->area->kind) {
    case CAM_AREA_FOLLOW:
        ctl->areaMode = CAM_MODE_FOLLOW;
        break;
    case CAM_AREA_FIXED:
    case CAM_AREA_PATH:
    case CAM_AREA_OFFSET:
        ctl->areaMode = CAM_MODE_RAIL;
        break;
    }
    view = ctl->modes;
    for (i = CAM_MODE_COUNT; i != 0; i--) {
        gCamModeUpdate[view->kind](ctl, view);
        view++;
    }
    CamCtl_RunMode(ctl);
    ctl->targetObj = 0;
    Camera_Update();
    if (ctl->followView != 0) {
        CamCtl_CheckPlayerInView();
    }
}

extern Camera *gCamera;

/*
 * Runs only the camera script's mode (CamScript_Update), hands its eye,
 * target, roll and fov to the camera and updates it (seen: plsel).
 */
void CamCtl_Apply(void)
{
    CamView *view = &gCamCtl.modes[CAM_MODE_SCRIPT];

    CamScript_Update(&gCamCtl, view);
    Vec_Copy(gCamera->eye, view->eye);
    Vec_Copy(gCamera->at, view->at);
    Camera_SetRoll(view->roll);
    Camera_StoreFov(view->fov);
    Camera_Update();
}

s32 CamCtl_GetMode(void)
{
    return gCamCtl.mode;
}
