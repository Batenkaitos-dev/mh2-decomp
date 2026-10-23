/*
 * The camera's script mode as the rest of the game drives it: the mode's
 * two handlers and the requests they serve.
 * - CamReq_Set and CamReq_Start request a script: its id in
 *   gCamCtl.request, its argument in requestArg (CamReq_Start also marks
 *   the script started, CAM_SCRIPT_STARTED, and tells the HUD).
 *   CamReq_Change swaps the running script for another, CamReq_Cancel
 *   stops it, and CamReq_State and CamReq_Started report on it.
 * - CamScript_Init and CamScript_Update are the script mode's entries in
 *   gCamModeInit and gCamModeUpdate: the update takes the request, runs
 *   the script (CamScript_Run) until it ends, and keeps the script's
 *   state in gCamCtl.script for CamReq_State.
 *
 * The two handlers open this file rather than end the fixed camera's code
 * before it: they read and clear the same request and script work as the
 * requests (gCamCtl.request, gCamCtl.script), which the fixed and focus
 * views never touch. The file has no data of its own to tie them
 * elsewhere.
 */
#include "common.h"
#include "em.h"
#include "camera.h"
#include "hud.h"

/* prototypes */
/* end of prototypes */

/* The init handler of the script mode (gCamModeInit). */
void CamScript_Init(CamCtl *ctl, CamView *view)
{
    view->roll = 0.0f;
    view->fov = 0.9599311f;
    ctl->script.id = 0;
    ctl->script.state = 0;
    ctl->script.obj = NULL;
    view->flags = 0;
}

/* The script mode's update: step 0 waits for a request (ctl->request and
 * ctl->requestArg into the script work), step 1 runs
 * CamScript_Run until it returns a positive value (then resets); while it
 * returns 0 the script's state is 1, while negative -1. */
void CamScript_Update(CamCtl *ctl, CamView *view)
{
    CamScript *work = &ctl->script;
    s32 result;

    view->active = 0;
    switch (view->step) {
    case 0:
        if (ctl->request == 0) {
            break;
        }
        work->id = ctl->request;
        work->obj = ctl->requestArg;
        view->step++;
        ((CamStep *)&view->step)->count = 0;
        work->place = 1;
        work->eyeSpace = CAM_SPACE_WORLD;
        work->atSpace = CAM_SPACE_WORLD;
    case 1:
        ctl->request = 0;
        result = CamScript_Run(ctl, view);
        if (result <= 0) {
            view->active = 1;
            if (result == 0) {
                work->state = 1;
            } else {
                work->state = -1;
            }
        } else {
            view->step = 0;
            work->id = 0;
            work->state = 0;
            work->obj = NULL;
            view->flags = 0;
        }
        break;
    }
}

/* Requests script req with its argument, as an ordinary request. */
void CamReq_Set(s32 req, void *arg)
{
    gCamCtl.request = req;
    gCamCtl.requestArg = arg;
    gCamCtl.modes[CAM_MODE_SCRIPT].flags = 0;
}

/* Requests script req with its argument, marked started, and tells the
 * HUD (Hud_SetCamReqStarted). */
void CamReq_Start(s32 req, void *arg)
{
    gCamCtl.request = req;
    gCamCtl.requestArg = arg;
    gCamCtl.modes[CAM_MODE_SCRIPT].flags = CAM_SCRIPT_STARTED;
    Hud_SetCamReqStarted();
}

/* While a script runs, replaces it with script req unless it is req
 * already. */
void CamReq_Change(s32 req, void *arg)
{
    CamScript *work = &gCamCtl.script;

    if (CamReq_State()) {
        if (work->id != req) {
            CamReq_Cancel();
            CamReq_Start(req, arg);
        }
    }
}

/* Stops the script mode and drops the request. */
void CamReq_Cancel(void)
{
    CamView *modeView = &gCamCtl.modes[CAM_MODE_SCRIPT];
    CamScript *work = &gCamCtl.script;

    modeView->step = 0;
    work->id = 0;
    work->state = 0;
    work->obj = NULL;
    modeView->flags = 0;
    gCamCtl.request = 0;
}

s32 CamReq_State(void)
{
    return gCamCtl.script.state;
}

s32 CamReq_Started(void)
{
    return (gCamCtl.modes[CAM_MODE_SCRIPT].flags & CAM_SCRIPT_STARTED) != 0;
}
