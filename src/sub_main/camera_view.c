/*
 * The camera's view (gCamera, a Camera): its eye, target, roll and FOV,
 * the presets (CamPreset: far and near planes, FOV, aspect), the projection
 * and, each frame, the view direction, the up vector and the view matrix.
 * The camera controller (CamCtl) hands it the view it computes.
 */
#include "common.h"
#include "camera.h"
#include "em.h"
#include "render_state.h"
#include "engine_math.h"
#include "engine_util.h"
#include "vec_math.h"
/* A camera preset (gCameraPresets, Camera_SetPreset). */
typedef struct CamPreset {
    f32 farZ;           /* copied to Camera.farZ */
    f32 nearZ;          /* copied to Camera.nearZ */
    f32 fov;
    f32 aspect;         /* seen: the aspect the code passes; nothing reads it */
} CamPreset;
extern f32 gCamDefaultEye[];       /* the default eye, then (gCamDefaultAt) the default target */
extern f32 gCamDefaultAt[3];       /* default target */
extern f32 gCamDefaultDir[3];    /* the default view direction (Camera.viewDir) */
extern CamPreset gCameraPresets[];
extern u8 gCameraPreset;           /* the current camera preset */
extern f32 gFrustumPlanes[4][4];    /* the projection matrix */
extern f32 gCamYawMtx[4][4];    /* rotation by the camera's yaw */
extern f32 gCamMatrix[];
extern f32 gViewMatrix[4][4];
struct MtxSlot;
/* end of prototypes */

/* Initialises the camera: a 512 x 448 viewport, then the defaults. */
void Camera_Init(void)
{
    gCamera = &gCameraData;
    gCamera->viewport[0] = 0;
    gCamera->viewport[1] = 0;
    gCamera->viewport[2] = 512;
    gCamera->viewport[3] = 448;
    gCamera->viewport[4] = 0;
    gCamera->viewportFarZ = 1.0f;
    Gs_SetState(RS_ORTHO, (u32)gCamera->viewport);
    Camera_Reset();
}

/* Default eye, target and direction, a FOV of 50 degrees, no roll. */
void Camera_Reset(void)
{
    Vec_Copy(gCamera->eye, gCamDefaultEye);
    Vec_Copy(gCamera->at, gCamDefaultAt);
    Vec_Copy(gCamera->viewDir, gCamDefaultDir);
    gCamera->fov = 0.87266463f;
    gCamera->fovPrev = 0.87266463f;
    Camera_ApplyProjection();
    gCamera->roll = 0.0f;
}

/*
 * The camera update: the view direction, the projection again when the FOV
 * changed, the up vector from the roll, the view matrix (RS_VIEW_MTX),
 * and gCamYawMtx, a rotation by the camera's yaw (+ 90 degrees).
 */
void Camera_Update(void)
{
    f32 dir[4];
    f32 viewMtx[4][4];
    s32 changed = 0;
    f32 angle;
    f32 *fov;

    Vec_Diff(dir, gCamera->at, gCamera->eye);
    if (dir[0] != 0.0f || dir[2] != 0.0f) {
        Vec_Copy(gCamera->viewDir, dir);
    } else {
        Vec_Add(gCamera->at, gCamera->eye, gCamera->viewDir);
    }
    fov = &gCamera->fovPrev;
    if (*fov != gCamera->fov) {
        *fov = gCamera->fov;
        changed++;
    }
    if (changed) {
        Camera_ApplyProjection();
    }
    Camera_ComputeUp(gCamera->up, gCamera->eye, gCamera->at, gCamera->roll);
    Mtx_LookAt((f32 *) viewMtx, (f32 *) gCamera->eye, (f32 *) gCamera->at, (f32 *) gCamera->up);
    Gs_SetState(RS_VIEW_MTX, (u32)viewMtx);
    MtxSlot_Load((struct MtxSlot *) gViewMatrix, 33);
    Mtx_Inverse(gCamMatrix, (f32 *) gViewMatrix);
    angle = 6.2831855f * (s32)(u16)(Mtx_Heading(gCamMatrix) + ANGLE_TURN / 4) / 65536.0f;
    Mtx_Identity((f32 *) gCamYawMtx);
    Mtx_SetRotXYZ((f32 *) gCamYawMtx, 0.0f, angle, 0.0f);
}

/* Selects camera preset id (CAM_PRESET_NONE: only reapplies the projection). */
void Camera_SetPreset(s32 id)
{
    CamPreset *preset;

    if (id != CAM_PRESET_NONE) {
        if (gCameraPreset == id) {
            return;
        }
        gCameraPreset = id;
        preset = &gCameraPresets[id];
        gCamera->farZ = preset->farZ;
        gCamera->nearZ = preset->nearZ;
        gCamera->fov = gCamera->fovPrev = preset->fov;
        gCamera->roll = 0.0f;
    }
    Camera_ApplyProjection();
}

void Camera_BuildFrustum(f32 farZ)
{
    Frustum_BuildPlanes((f32 *) gFrustumPlanes, gCamera->fov, 1.4285715f, gCamera->nearZ, farZ);
}

/* Applies the projection (aspect 640/448), the far value 3000. */
void Camera_ApplyProjection(void)
{
    Camera_SetPerspective(32, gCamera->farZ, gCamera->nearZ, gCamera->fov, 1.4285715f);
    Frustum_BuildPlanes((f32 *) gFrustumPlanes, gCamera->fov, 1.4285715f, gCamera->nearZ, 3000.0f);
}

void Camera_SetFov(f32 fov)
{
    gCamera->fov = fov;
    Camera_ApplyProjection();
}

void Camera_RestoreFov(void)
{
    gCamera->fov = gCamera->fovPrev;
    Camera_ApplyProjection();
}

void Camera_SetRoll(f32 roll)
{
    gCamera->roll = roll;
}

void Camera_StoreFov(f32 fov)
{
    gCamera->fov = fov;
}

f32 Camera_Distance(void *pos)
{
    return Vec_Distance((f32 *) pos, (f32 *) gCamera);
}

/* The camera's yaw as a 16-bit angle. */
u16 Camera_GetYaw(void)
{
    return (s32)(0.5f + 65536.0f * Math_Atan2(-(gCamera->at[2] - gCamera->eye[2]),
                                                 gCamera->at[0] - gCamera->eye[0]) / 6.2831855f);
}

/* up = (0, 1, 0) rotated by roll around the view direction. */
void Camera_ComputeUp(f32 *up, f32 *eye, f32 *at, f32 roll)
{
    f32 vec[4];
    f32 rot[4][4];
    f32 quat[4];

    Vec_Diff(vec, at, eye);
    Quat_Set(vec, quat, roll);
    Quat_ToMatrix(quat, rot);
    vec[0] = 0.0f;
    vec[1] = 1.0f;
    vec[2] = 0.0f;
    Vec_Rotate((f32 *) up, (f32 *) vec, (f32 *) rot);
}

/* The file's initialised data, in the game's order */
f32 gCamDefaultEye[6] = {
#include "sub_main/0043ED20.inc"
};
f32 gCamDefaultDir[3] = {
#include "sub_main/0043ED38.inc"
};
CamPreset gCameraPresets[3] = {
#include "sub_main/0043ED50.inc"
};
