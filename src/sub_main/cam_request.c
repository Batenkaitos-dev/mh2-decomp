/*
 * The requests of the state machine at gCamView (a request byte in
 * gCamCtl.request with its argument in gCamCtl.requestArg).
 */
#include "common.h"
#include "em.h"
#include "camera.h"

/* prototypes */
/* end of prototypes */


void Hud_SetCamReqStarted(void);
s32 CamScript_Run(CamCtl *cam, CamView *obj);

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

s32 CamReq_State(void);
void CamReq_Cancel(void);

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
