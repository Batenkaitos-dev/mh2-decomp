#include "common.h"
#include "area.h"
#include "area_anim.h"
#include "capsule.h"
#include "em.h"
#include "obj.h"
#include "player.h"
#include "prim.h"
#include "quest.h"
#include "save.h"
#include "scene.h"
#include "sprite.h"
#include "window.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

/* prototypes */
void Camera_RestoreFov(void);
void Camera_SetFov(f32 fov);
s32 Model_Draw(u32 model, s32 flags);
void Obj_Free(Obj *p);
void Draw_ApplyStates(u32 a);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void DrawSlot_Free(s32 id);
void DrawSlotB_Free(s32 id);
void NoticeBanner_Start(Obj *o);
u32 NoticeBanner_Run(Obj *o);
void NoticeBanner_End(Obj *o);
void NoticeBanner_Free(Obj *o);
void AreaProp_Start(Obj *o);
void AreaProp_Run(Obj *o);
void AreaProp_End(Obj *o);
void AreaProp_Free(Obj *o);
void SoundObj_Init(Obj *o);
void SoundObj_Run(Obj *o);
void SoundObj_End(Obj *o);
void SoundObj_Free(Obj *o);
void AmbientSound_Setup(Obj *o);
void AmbientSound_Update(Obj *o);
void AmbientSound_End(Obj *o);
void AmbientSound_Free(Obj *o);
s32 Gs_SetState(u8 a, u32 b);
extern u8 gSkyDayTable[];
extern u8 gSkyDayTable84[];
extern u8 gSkyDayTableAD[];
extern f32 gCamDir[3];    /* the camera direction */
f32 Math_Atan2(f32 y, f32 x);
s32 Prim_FillGradRect(PrimGradRect *r);
/* end of prototypes */

s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);

/* The text of a notice (the object's buffer). */
typedef struct TextBuf {
    char *msg;                  /* 0x00: type 4's text (NoticeBanner_SpawnKind4) */
    s8 width[3];                /* 0x04: of each line, in characters */
    char text[0x80];            /* 0x07: its lines, each ended by a newline */
} TextBuf;

/* A prop of an area (gAreaProps): 12 bytes each, ended by model 0xFF. */
typedef struct PropRec {
    u8 model;                   /* 0x00: its model in the set, 0xFF ends the list */
    u8 alpha;                   /* 0x01: Gs_SetState 0x60 */
    u8 type;                    /* 0x02: its animation */
    u8 flags;                   /* 0x03 */
    void *data;                 /* 0x04: the animations' parameters, one block per copy */
    s16 count;                  /* 0x08: copies */
    u16 late;                   /* 0x0A: drawn after the others */
} PropRec;

void AreaAnim_TranslateSpin(void *m, struct AnimSpin *arg, u8 flags);
void AreaAnim_UvScroll(struct UvScroll *arg, u8 flags);
void AreaAnim_UvWave(struct UvWave *arg);
void AreaAnim_TranslateRotate(void *m, struct AnimRot *arg, u8 flags);
void AreaAnim_Translate(void *m, f32 *pos);
void AreaAnim_ScaleTranslateRotate(void *m, struct AnimRotScale *arg, u8 flags);
void AreaAnim_TranslateXZ(void *m, f32 *p);
void AreaAnim_UvScrollTranslate(void *m, struct AnimScrollPos *arg, u8 flags);
void AreaAnim_UvScrollSpin(void *m, struct AnimScrollSpin *arg, u8 flags);
void AreaAnim_UvScrollRotate(void *m, struct AnimScrollRot *arg, u8 flags);
void AreaAnim_SpinScroll(void *m, struct AnimSpin2 *arg);
void AreaAnim_UvStep(struct UvStep *arg);
void Mtx_Identity(f32 *m);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_RotateZ(f32 *m, f32 a);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Vec_Copy(f32 *dst, f32 *src);
f32 Math_Sin(f32 a);
u8 Village_FacilityLevel(s32 k);

/* unk_37B570's .rodata (0x490B70-0x49FB98) */
#pragma rodata on
char D_00490B70[0x10] = {
#include "sub_main/00490B70.inc"
};
char D_00490B80[0x20] = {
#include "sub_main/00490B80.inc"
};
char D_00490BA0[0x10] = {
#include "sub_main/00490BA0.inc"
};
char D_00490BB0[0x10] = {
#include "sub_main/00490BB0.inc"
};
char D_00490BC0[0x10] = {
#include "sub_main/00490BC0.inc"
};
#pragma explicit_zero_data on
char gEmClassNames_0[0x10] = {
#include "sub_main/00490BD0.inc"
};
#pragma explicit_zero_data reset
char D_00490BE0[0x20] = {
#include "sub_main/00490BE0.inc"
};
char D_00490C00[0x20] = {
#include "sub_main/00490C00.inc"
};
char D_00490C20[0x20] = {
#include "sub_main/00490C20.inc"
};
char D_00490C40[0x20] = {
#include "sub_main/00490C40.inc"
};
char D_00490C60[0x20] = {
#include "sub_main/00490C60.inc"
};
char D_00490C80[0x20] = {
#include "sub_main/00490C80.inc"
};
char D_00490CA0[0x18] = {
#include "sub_main/00490CA0.inc"
};
char D_00490CB8[15] = {
#include "sub_main/00490CB8.inc"
};
char D_00490CD0[0x20] = {
#include "sub_main/00490CD0.inc"
};
char D_00490CF0[0x18] = {
#include "sub_main/00490CF0.inc"
};
char D_00490D08[13] = {
#include "sub_main/00490D08.inc"
};
char D_00490D20[0x20] = {
#include "sub_main/00490D20.inc"
};
char D_00490D40[0x20] = {
#include "sub_main/00490D40.inc"
};
char D_00490D60[0x20] = {
#include "sub_main/00490D60.inc"
};
char D_00490D80[0x20] = {
#include "sub_main/00490D80.inc"
};
char D_00490DA0[0x20] = {
#include "sub_main/00490DA0.inc"
};
char D_00490DC0[0x20] = {
#include "sub_main/00490DC0.inc"
};
char D_00490DE0[0x20] = {
#include "sub_main/00490DE0.inc"
};
char D_00490E00[0x20] = {
#include "sub_main/00490E00.inc"
};
char D_00490E20[0x20] = {
#include "sub_main/00490E20.inc"
};
char D_00490E40[0x30] = {
#include "sub_main/00490E40.inc"
};
char D_00490E70[0x30] = {
#include "sub_main/00490E70.inc"
};
char D_00490EA0[0x20] = {
#include "sub_main/00490EA0.inc"
};
char D_00490EC0[0x20] = {
#include "sub_main/00490EC0.inc"
};
char D_00490EE0[0x10] = {
#include "sub_main/00490EE0.inc"
};
char D_00490EF0[0x30] = {
#include "sub_main/00490EF0.inc"
};
char D_00490F20[0x20] = {
#include "sub_main/00490F20.inc"
};
char D_00490F40[0x20] = {
#include "sub_main/00490F40.inc"
};
char D_00490F60[0x30] = {
#include "sub_main/00490F60.inc"
};
char D_00490F90[0x20] = {
#include "sub_main/00490F90.inc"
};
char D_00490FB0[0x20] = {
#include "sub_main/00490FB0.inc"
};
char D_00490FD0[0x20] = {
#include "sub_main/00490FD0.inc"
};
char D_00490FF0[0x20] = {
#include "sub_main/00490FF0.inc"
};
char D_00491010[0x8] = {
#include "sub_main/00491010.inc"
};
char D_00491018[0x8] = {
#include "sub_main/00491018.inc"
};
char D_00491020[0x8] = {
#include "sub_main/00491020.inc"
};
char D_00491028[0x8] = {
#include "sub_main/00491028.inc"
};
char D_00491030[0x8] = {
#include "sub_main/00491030.inc"
};
char D_00491038[0x8] = {
#include "sub_main/00491038.inc"
};
char D_00491040[0x8] = {
#include "sub_main/00491040.inc"
};
char D_00491048[0x8] = {
#include "sub_main/00491048.inc"
};
char D_00491050[0x8] = {
#include "sub_main/00491050.inc"
};
char D_00491058[15] = {
#include "sub_main/00491058.inc"
};
char D_00491068[0xC] = {
#include "sub_main/00491068.inc"
};
char D_00491078[13] = {
#include "sub_main/00491078.inc"
};
char D_00491088[0x8] = {
#include "sub_main/00491088.inc"
};
char D_00491090[0x8] = {
#include "sub_main/00491090.inc"
};
char D_00491098[13] = {
#include "sub_main/00491098.inc"
};
char D_004910A8[13] = {
#include "sub_main/004910A8.inc"
};
char D_004910B8[15] = {
#include "sub_main/004910B8.inc"
};
char D_004910C8[13] = {
#include "sub_main/004910C8.inc"
};
char D_004910D8[0xC] = {
#include "sub_main/004910D8.inc"
};
char D_004910E8[0x8] = {
#include "sub_main/004910E8.inc"
};
char D_004910F0[0x10] = {
#include "sub_main/004910F0.inc"
};
char D_00491100[0x10] = {
#include "sub_main/00491100.inc"
};
char D_00491110[0x10] = {
#include "sub_main/00491110.inc"
};
char D_00491120[0x10] = {
#include "sub_main/00491120.inc"
};
char D_00491130[0x10] = {
#include "sub_main/00491130.inc"
};
char D_00491140[0x10] = {
#include "sub_main/00491140.inc"
};
char D_00491150[0x10] = {
#include "sub_main/00491150.inc"
};
char D_00491160[0x10] = {
#include "sub_main/00491160.inc"
};
char D_00491170[0x10] = {
#include "sub_main/00491170.inc"
};
char D_00491180[0x10] = {
#include "sub_main/00491180.inc"
};
char D_00491190[0x10] = {
#include "sub_main/00491190.inc"
};
char D_004911A0[0x10] = {
#include "sub_main/004911A0.inc"
};
char D_004911B0[0x10] = {
#include "sub_main/004911B0.inc"
};
char D_004911C0[0x10] = {
#include "sub_main/004911C0.inc"
};
char D_004911D0[0x10] = {
#include "sub_main/004911D0.inc"
};
char D_004911E0[0x10] = {
#include "sub_main/004911E0.inc"
};
char D_004911F0[0x10] = {
#include "sub_main/004911F0.inc"
};
char D_00491200[0x10] = {
#include "sub_main/00491200.inc"
};
char D_00491210[0x8] = {
#include "sub_main/00491210.inc"
};
char D_00491218[0xC] = {
#include "sub_main/00491218.inc"
};
char D_00491228[13] = {
#include "sub_main/00491228.inc"
};
char D_00491238[0x8] = {
#include "sub_main/00491238.inc"
};
char D_00491240[0x8] = {
#include "sub_main/00491240.inc"
};
char D_00491248[0xC] = {
#include "sub_main/00491248.inc"
};
char D_00491260[0x20] = {
#include "sub_main/00491260.inc"
};
char D_00491280[0x20] = {
#include "sub_main/00491280.inc"
};
char D_004912A0[0x18] = {
#include "sub_main/004912A0.inc"
};
char D_004912B8[15] = {
#include "sub_main/004912B8.inc"
};
char D_004912D0[0x20] = {
#include "sub_main/004912D0.inc"
};
char D_004912F0[0x20] = {
#include "sub_main/004912F0.inc"
};
char D_00491310[0x20] = {
#include "sub_main/00491310.inc"
};
char D_00491330[0x20] = {
#include "sub_main/00491330.inc"
};
char D_00491350[0x20] = {
#include "sub_main/00491350.inc"
};
char D_00491370[0x20] = {
#include "sub_main/00491370.inc"
};
char D_00491390[0x18] = {
#include "sub_main/00491390.inc"
};
char D_004913A8[13] = {
#include "sub_main/004913A8.inc"
};
char D_004913B8[0xC] = {
#include "sub_main/004913B8.inc"
};
char D_004913C8[0xC] = {
#include "sub_main/004913C8.inc"
};
char D_004913D8[15] = {
#include "sub_main/004913D8.inc"
};
char D_004913E8[15] = {
#include "sub_main/004913E8.inc"
};
char D_004913F8[0xC] = {
#include "sub_main/004913F8.inc"
};
char D_00491408[13] = {
#include "sub_main/00491408.inc"
};
char D_00491418[0xC] = {
#include "sub_main/00491418.inc"
};
char D_00491428[0xC] = {
#include "sub_main/00491428.inc"
};
char D_00491438[0xC] = {
#include "sub_main/00491438.inc"
};
char D_00491448[0x8] = {
#include "sub_main/00491448.inc"
};
char D_00491450[0x10] = {
#include "sub_main/00491450.inc"
};
char D_00491460[0x20] = {
#include "sub_main/00491460.inc"
};
char D_00491480[0x18] = {
#include "sub_main/00491480.inc"
};
char D_00491498[0xC] = {
#include "sub_main/00491498.inc"
};
char D_004914B0[0x18] = {
#include "sub_main/004914B0.inc"
};
char D_004914C8[13] = {
#include "sub_main/004914C8.inc"
};
char D_004914D8[0xC] = {
#include "sub_main/004914D8.inc"
};
char D_004914E8[0x8] = {
#include "sub_main/004914E8.inc"
};
char D_004914F0[0x10] = {
#include "sub_main/004914F0.inc"
};
char D_00491500[0x10] = {
#include "sub_main/00491500.inc"
};
char D_00491510[0x8] = {
#include "sub_main/00491510.inc"
};
char D_00491518[0xC] = {
#include "sub_main/00491518.inc"
};
char D_00491528[13] = {
#include "sub_main/00491528.inc"
};
char D_00491538[0xC] = {
#include "sub_main/00491538.inc"
};
char D_00491548[0xC] = {
#include "sub_main/00491548.inc"
};
char D_00491558[0xC] = {
#include "sub_main/00491558.inc"
};
char D_00491568[13] = {
#include "sub_main/00491568.inc"
};
char D_00491578[0xC] = {
#include "sub_main/00491578.inc"
};
char D_00491588[0xC] = {
#include "sub_main/00491588.inc"
};
char D_00491598[0xC] = {
#include "sub_main/00491598.inc"
};
char D_004915A8[0xC] = {
#include "sub_main/004915A8.inc"
};
char D_004915B8[15] = {
#include "sub_main/004915B8.inc"
};
char D_004915C8[13] = {
#include "sub_main/004915C8.inc"
};
char D_004915D8[13] = {
#include "sub_main/004915D8.inc"
};
char D_004915E8[0xC] = {
#include "sub_main/004915E8.inc"
};
char D_004915F8[0x8] = {
#include "sub_main/004915F8.inc"
};
char D_00491600[0x8] = {
#include "sub_main/00491600.inc"
};
char D_00491608[0x8] = {
#include "sub_main/00491608.inc"
};
char D_00491610[0x10] = {
#include "sub_main/00491610.inc"
};
char D_00491620[0x20] = {
#include "sub_main/00491620.inc"
};
char D_00491640[0x20] = {
#include "sub_main/00491640.inc"
};
char D_00491660[0x8] = {
#include "sub_main/00491660.inc"
};
char D_00491668[0x8] = {
#include "sub_main/00491668.inc"
};
char D_00491670[0x10] = {
#include "sub_main/00491670.inc"
};
char D_00491680[0x10] = {
#include "sub_main/00491680.inc"
};
char D_00491690[0x10] = {
#include "sub_main/00491690.inc"
};
char D_004916A0[0x10] = {
#include "sub_main/004916A0.inc"
};
char D_004916B0[0x10] = {
#include "sub_main/004916B0.inc"
};
char D_004916C0[0x10] = {
#include "sub_main/004916C0.inc"
};
char D_004916D0[0x10] = {
#include "sub_main/004916D0.inc"
};
char D_004916E0[0x10] = {
#include "sub_main/004916E0.inc"
};
char D_004916F0[0x20] = {
#include "sub_main/004916F0.inc"
};
char D_00491710[0x18] = {
#include "sub_main/00491710.inc"
};
char D_00491728[0xC] = {
#include "sub_main/00491728.inc"
};
char D_00491740[0x18] = {
#include "sub_main/00491740.inc"
};
char D_00491758[0xC] = {
#include "sub_main/00491758.inc"
};
char D_00491768[13] = {
#include "sub_main/00491768.inc"
};
char D_00491780[0x20] = {
#include "sub_main/00491780.inc"
};
char D_004917A0[0x10] = {
#include "sub_main/004917A0.inc"
};
char D_004917B0[0x20] = {
#include "sub_main/004917B0.inc"
};
char D_004917D0[0x20] = {
#include "sub_main/004917D0.inc"
};
char D_004917F0[0x30] = {
#include "sub_main/004917F0.inc"
};
char D_00491820[0x30] = {
#include "sub_main/00491820.inc"
};
char D_00491850[0x30] = {
#include "sub_main/00491850.inc"
};
char D_00491880[0x28] = {
#include "sub_main/00491880.inc"
};
char D_004918A8[0x8] = {
#include "sub_main/004918A8.inc"
};
char D_004918B0[0x10] = {
#include "sub_main/004918B0.inc"
};
char D_004918C0[0x10] = {
#include "sub_main/004918C0.inc"
};
char D_004918D0[0x10] = {
#include "sub_main/004918D0.inc"
};
char D_004918E0[0x10] = {
#include "sub_main/004918E0.inc"
};
char D_004918F0[0x10] = {
#include "sub_main/004918F0.inc"
};
char D_00491900[0x8] = {
#include "sub_main/00491900.inc"
};
char D_00491908[0xC] = {
#include "sub_main/00491908.inc"
};
char D_00491918[0xC] = {
#include "sub_main/00491918.inc"
};
char D_00491930[0x20] = {
#include "sub_main/00491930.inc"
};
char D_00491950[0x20] = {
#include "sub_main/00491950.inc"
};
char D_00491970[0x18] = {
#include "sub_main/00491970.inc"
};
char D_00491988[15] = {
#include "sub_main/00491988.inc"
};
char D_00491998[0xC] = {
#include "sub_main/00491998.inc"
};
char D_004919B0[0x18] = {
#include "sub_main/004919B0.inc"
};
char D_004919C8[0xC] = {
#include "sub_main/004919C8.inc"
};
char D_004919E0[0x20] = {
#include "sub_main/004919E0.inc"
};
char D_00491A00[0x18] = {
#include "sub_main/00491A00.inc"
};
char D_00491A18[0xC] = {
#include "sub_main/00491A18.inc"
};
char D_00491A28[0xC] = {
#include "sub_main/00491A28.inc"
};
char D_00491A38[0xC] = {
#include "sub_main/00491A38.inc"
};
char D_00491A48[15] = {
#include "sub_main/00491A48.inc"
};
char D_00491A58[15] = {
#include "sub_main/00491A58.inc"
};
char D_00491A68[15] = {
#include "sub_main/00491A68.inc"
};
char D_00491A78[0xC] = {
#include "sub_main/00491A78.inc"
};
char D_00491A88[0xC] = {
#include "sub_main/00491A88.inc"
};
char D_00491A98[0xC] = {
#include "sub_main/00491A98.inc"
};
char D_00491AA8[0xC] = {
#include "sub_main/00491AA8.inc"
};
char D_00491AB8[0xC] = {
#include "sub_main/00491AB8.inc"
};
char D_00491AD0[0x20] = {
#include "sub_main/00491AD0.inc"
};
char D_00491AF0[0x18] = {
#include "sub_main/00491AF0.inc"
};
char D_00491B08[13] = {
#include "sub_main/00491B08.inc"
};
char D_00491B18[13] = {
#include "sub_main/00491B18.inc"
};
char D_00491B28[13] = {
#include "sub_main/00491B28.inc"
};
char D_00491B38[15] = {
#include "sub_main/00491B38.inc"
};
char D_00491B48[15] = {
#include "sub_main/00491B48.inc"
};
char D_00491B60[0x20] = {
#include "sub_main/00491B60.inc"
};
char D_00491B80[0x18] = {
#include "sub_main/00491B80.inc"
};
char D_00491B98[0xC] = {
#include "sub_main/00491B98.inc"
};
char D_00491BB0[0x60] = {
#include "sub_main/00491BB0.inc"
};
char D_00491C10[0x20] = {
#include "sub_main/00491C10.inc"
};
char D_00491C30[0x10] = {
#include "sub_main/00491C30.inc"
};
char D_00491C40[0x20] = {
#include "sub_main/00491C40.inc"
};
char D_00491C60[0x28] = {
#include "sub_main/00491C60.inc"
};
char D_00491C88[0x8] = {
#include "sub_main/00491C88.inc"
};
char D_00491C90[0x8] = {
#include "sub_main/00491C90.inc"
};
char D_00491C98[0x8] = {
#include "sub_main/00491C98.inc"
};
char D_00491CA0[0x10] = {
#include "sub_main/00491CA0.inc"
};
char D_00491CB0[0x30] = {
#include "sub_main/00491CB0.inc"
};
char D_00491CE0[0x28] = {
#include "sub_main/00491CE0.inc"
};
char D_00491D08[0xC] = {
#include "sub_main/00491D08.inc"
};
char D_00491D18[0xC] = {
#include "sub_main/00491D18.inc"
};
char D_00491D28[0xC] = {
#include "sub_main/00491D28.inc"
};
char D_00491D38[0xC] = {
#include "sub_main/00491D38.inc"
};
char D_00491D48[0xC] = {
#include "sub_main/00491D48.inc"
};
char D_00491D58[13] = {
#include "sub_main/00491D58.inc"
};
char D_00491D68[0x8] = {
#include "sub_main/00491D68.inc"
};
char D_00491D70[0x10] = {
#include "sub_main/00491D70.inc"
};
char D_00491D80[0x20] = {
#include "sub_main/00491D80.inc"
};
char D_00491DA0[0x20] = {
#include "sub_main/00491DA0.inc"
};
char D_00491DC0[0x18] = {
#include "sub_main/00491DC0.inc"
};
char D_00491DD8[15] = {
#include "sub_main/00491DD8.inc"
};
char D_00491DF0[0x18] = {
#include "sub_main/00491DF0.inc"
};
char D_00491E08[13] = {
#include "sub_main/00491E08.inc"
};
char D_00491E20[0x20] = {
#include "sub_main/00491E20.inc"
};
char D_00491E40[0x20] = {
#include "sub_main/00491E40.inc"
};
char D_00491E60[0x28] = {
#include "sub_main/00491E60.inc"
};
char D_00491E88[0x8] = {
#include "sub_main/00491E88.inc"
};
char D_00491E90[0x20] = {
#include "sub_main/00491E90.inc"
};
char D_00491EB0[0x20] = {
#include "sub_main/00491EB0.inc"
};
char D_00491ED0[0x20] = {
#include "sub_main/00491ED0.inc"
};
char D_00491EF0[0x20] = {
#include "sub_main/00491EF0.inc"
};
char D_00491F10[0x20] = {
#include "sub_main/00491F10.inc"
};
char D_00491F30[0xC0] = {
#include "sub_main/00491F30.inc"
};
char D_00491FF0[16] = {
#include "sub_main/00491FF0.inc"
};
char D_00492000[0x90] = {
#include "sub_main/00492000.inc"
};
char D_00492090[0xD0] = {
#include "sub_main/00492090.inc"
};
char D_00492160[0xC8] = {
#include "sub_main/00492160.inc"
};
char D_00492228[13] = {
#include "sub_main/00492228.inc"
};
char D_00492238[0xC] = {
#include "sub_main/00492238.inc"
};
char D_00492248[0xC] = {
#include "sub_main/00492248.inc"
};
char D_00492258[0xC] = {
#include "sub_main/00492258.inc"
};
char D_00492268[0x8] = {
#include "sub_main/00492268.inc"
};
char D_00492270[0x20] = {
#include "sub_main/00492270.inc"
};
char D_00492290[0xB0] = {
#include "sub_main/00492290.inc"
};
char D_00492340[0x28] = {
#include "sub_main/00492340.inc"
};
char D_00492368[0x8] = {
#include "sub_main/00492368.inc"
};
char D_00492370[0x20] = {
#include "sub_main/00492370.inc"
};
char D_00492390[0x20] = {
#include "sub_main/00492390.inc"
};
char D_004923B0[0x20] = {
#include "sub_main/004923B0.inc"
};
char D_004923D0[0x18] = {
#include "sub_main/004923D0.inc"
};
char D_004923E8[15] = {
#include "sub_main/004923E8.inc"
};
char D_004923F8[15] = {
#include "sub_main/004923F8.inc"
};
char D_00492408[15] = {
#include "sub_main/00492408.inc"
};
char D_00492418[15] = {
#include "sub_main/00492418.inc"
};
u32 D_00492428[3] = {
#include "sub_main/00492428.inc"
};
char D_00492438[0xC] = {
#include "sub_main/00492438.inc"
};
char D_00492448[0x8] = {
#include "sub_main/00492448.inc"
};
char D_00492450[0x8] = {
#include "sub_main/00492450.inc"
};
char D_00492458[0x8] = {
#include "sub_main/00492458.inc"
};
char D_00492460[0x8] = {
#include "sub_main/00492460.inc"
};
char D_00492468[0x8] = {
#include "sub_main/00492468.inc"
};
char D_00492470[0x8] = {
#include "sub_main/00492470.inc"
};
char D_00492478[0x8] = {
#include "sub_main/00492478.inc"
};
char D_00492480[0x8] = {
#include "sub_main/00492480.inc"
};
char D_00492488[0x8] = {
#include "sub_main/00492488.inc"
};
char D_00492490[0x8] = {
#include "sub_main/00492490.inc"
};
char D_00492498[0x8] = {
#include "sub_main/00492498.inc"
};
char D_004924A0[0x8] = {
#include "sub_main/004924A0.inc"
};
char D_004924A8[0x8] = {
#include "sub_main/004924A8.inc"
};
char D_004924B0[0x18] = {
#include "sub_main/004924B0.inc"
};
char D_004924C8[0x8] = {
#include "sub_main/004924C8.inc"
};
char D_004924D0[0x18] = {
#include "sub_main/004924D0.inc"
};
char D_004924E8[0x8] = {
#include "sub_main/004924E8.inc"
};
char D_004924F0[0x10] = {
#include "sub_main/004924F0.inc"
};
char D_00492500[0x20] = {
#include "sub_main/00492500.inc"
};
char D_00492520[0x20] = {
#include "sub_main/00492520.inc"
};
char D_00492540[0x20] = {
#include "sub_main/00492540.inc"
};
char D_00492560[0x20] = {
#include "sub_main/00492560.inc"
};
char D_00492580[0x20] = {
#include "sub_main/00492580.inc"
};
char D_004925A0[0x20] = {
#include "sub_main/004925A0.inc"
};
char D_004925C0[0x20] = {
#include "sub_main/004925C0.inc"
};
char D_004925E0[0x40] = {
#include "sub_main/004925E0.inc"
};
char D_00492620[0x8] = {
#include "sub_main/00492620.inc"
};
char D_00492628[0x8] = {
#include "sub_main/00492628.inc"
};
char D_00492630[0xE0] = {
#include "sub_main/00492630.inc"
};
char D_00492710[0x110] = {
#include "sub_main/00492710.inc"
};
char D_00492820[0x100] = {
#include "sub_main/00492820.inc"
};
char D_00492920[0xD0] = {
#include "sub_main/00492920.inc"
};
char D_004929F0[0xF0] = {
#include "sub_main/004929F0.inc"
};
char D_00492AE0[0xE0] = {
#include "sub_main/00492AE0.inc"
};
char D_00492BC0[0xF0] = {
#include "sub_main/00492BC0.inc"
};
char D_00492CB0[0x110] = {
#include "sub_main/00492CB0.inc"
};
char D_00492DC0[0xF0] = {
#include "sub_main/00492DC0.inc"
};
char D_00492EB0[0xC0] = {
#include "sub_main/00492EB0.inc"
};
char D_00492F70[0xD0] = {
#include "sub_main/00492F70.inc"
};
char D_00493040[0xD0] = {
#include "sub_main/00493040.inc"
};
char D_00493110[0xE0] = {
#include "sub_main/00493110.inc"
};
char D_004931F0[0xE0] = {
#include "sub_main/004931F0.inc"
};
char D_004932D0[0xE0] = {
#include "sub_main/004932D0.inc"
};
char D_004933B0[0xB0] = {
#include "sub_main/004933B0.inc"
};
char D_00493460[0xA0] = {
#include "sub_main/00493460.inc"
};
char D_00493500[0xC0] = {
#include "sub_main/00493500.inc"
};
char D_004935C0[0xA0] = {
#include "sub_main/004935C0.inc"
};
char D_00493660[0xD0] = {
#include "sub_main/00493660.inc"
};
char D_00493730[0x100] = {
#include "sub_main/00493730.inc"
};
char D_00493830[0xE0] = {
#include "sub_main/00493830.inc"
};
char D_00493910[0xE0] = {
#include "sub_main/00493910.inc"
};
char D_004939F0[0xF0] = {
#include "sub_main/004939F0.inc"
};
char D_00493AE0[0x100] = {
#include "sub_main/00493AE0.inc"
};
char D_00493BE0[0xD0] = {
#include "sub_main/00493BE0.inc"
};
char D_00493CB0[0xC0] = {
#include "sub_main/00493CB0.inc"
};
char D_00493D70[0xC0] = {
#include "sub_main/00493D70.inc"
};
char D_00493E30[0xE0] = {
#include "sub_main/00493E30.inc"
};
char D_00493F10[0xC0] = {
#include "sub_main/00493F10.inc"
};
char D_00493FD0[0xC0] = {
#include "sub_main/00493FD0.inc"
};
char D_00494090[0xC0] = {
#include "sub_main/00494090.inc"
};
char D_00494150[0xD0] = {
#include "sub_main/00494150.inc"
};
char D_00494220[0xF0] = {
#include "sub_main/00494220.inc"
};
char D_00494310[0xB0] = {
#include "sub_main/00494310.inc"
};
char D_004943C0[0xF0] = {
#include "sub_main/004943C0.inc"
};
char D_004944B0[0x100] = {
#include "sub_main/004944B0.inc"
};
char D_004945B0[0xF0] = {
#include "sub_main/004945B0.inc"
};
char D_004946A0[0xE0] = {
#include "sub_main/004946A0.inc"
};
char D_00494780[0xD0] = {
#include "sub_main/00494780.inc"
};
char D_00494850[0x140] = {
#include "sub_main/00494850.inc"
};
char D_00494990[0xD0] = {
#include "sub_main/00494990.inc"
};
char D_00494A60[0xF0] = {
#include "sub_main/00494A60.inc"
};
char D_00494B50[0xD0] = {
#include "sub_main/00494B50.inc"
};
char D_00494C20[0xE0] = {
#include "sub_main/00494C20.inc"
};
char D_00494D00[0xF0] = {
#include "sub_main/00494D00.inc"
};
char D_00494DF0[0xE0] = {
#include "sub_main/00494DF0.inc"
};
char D_00494ED0[0x100] = {
#include "sub_main/00494ED0.inc"
};
char D_00494FD0[0x100] = {
#include "sub_main/00494FD0.inc"
};
char D_004950D0[0xB0] = {
#include "sub_main/004950D0.inc"
};
char D_00495180[0xF0] = {
#include "sub_main/00495180.inc"
};
char D_00495270[0x80] = {
#include "sub_main/00495270.inc"
};
char D_004952F0[0x10] = {
#include "sub_main/004952F0.inc"
};
char D_00495300[0x10] = {
#include "sub_main/00495300.inc"
};
char D_00495310[0x10] = {
#include "sub_main/00495310.inc"
};
char D_00495320[0x18] = {
#include "sub_main/00495320.inc"
};
char D_00495338[0xC] = {
#include "sub_main/00495338.inc"
};
char D_00495348[0xC] = {
#include "sub_main/00495348.inc"
};
char D_00495358[13] = {
#include "sub_main/00495358.inc"
};
char D_00495370[0x18] = {
#include "sub_main/00495370.inc"
};
char D_00495388[15] = {
#include "sub_main/00495388.inc"
};
char D_00495398[13] = {
#include "sub_main/00495398.inc"
};
char D_004953A8[15] = {
#include "sub_main/004953A8.inc"
};
char D_004953B8[13] = {
#include "sub_main/004953B8.inc"
};
char D_004953C8[13] = {
#include "sub_main/004953C8.inc"
};
char D_004953E0[0x20] = {
#include "sub_main/004953E0.inc"
};
char D_00495400[0x20] = {
#include "sub_main/00495400.inc"
};
char D_00495420[0x18] = {
#include "sub_main/00495420.inc"
};
char D_00495438[0xC] = {
#include "sub_main/00495438.inc"
};
char D_00495450[0x18] = {
#include "sub_main/00495450.inc"
};
char D_00495468[13] = {
#include "sub_main/00495468.inc"
};
char D_00495480[0x20] = {
#include "sub_main/00495480.inc"
};
char D_004954A0[0x20] = {
#include "sub_main/004954A0.inc"
};
char D_004954C0[0x20] = {
#include "sub_main/004954C0.inc"
};
char D_004954E0[0x20] = {
#include "sub_main/004954E0.inc"
};
char D_00495500[0x18] = {
#include "sub_main/00495500.inc"
};
char D_00495518[13] = {
#include "sub_main/00495518.inc"
};
char D_00495530[0x20] = {
#include "sub_main/00495530.inc"
};
char D_00495550[0x20] = {
#include "sub_main/00495550.inc"
};
char D_00495570[0x18] = {
#include "sub_main/00495570.inc"
};
char D_00495588[15] = {
#include "sub_main/00495588.inc"
};
char D_00495598[0xC] = {
#include "sub_main/00495598.inc"
};
char D_004955B0[0x18] = {
#include "sub_main/004955B0.inc"
};
char D_004955C8[13] = {
#include "sub_main/004955C8.inc"
};
char D_004955D8[0xC] = {
#include "sub_main/004955D8.inc"
};
char D_004955E8[0xC] = {
#include "sub_main/004955E8.inc"
};
char D_004955F8[13] = {
#include "sub_main/004955F8.inc"
};
char D_00495608[15] = {
#include "sub_main/00495608.inc"
};
char D_00495620[0x20] = {
#include "sub_main/00495620.inc"
};
char D_00495640[0x18] = {
#include "sub_main/00495640.inc"
};
char D_00495658[0x8] = {
#include "sub_main/00495658.inc"
};
char D_00495660[0x8] = {
#include "sub_main/00495660.inc"
};
char D_00495668[0x8] = {
#include "sub_main/00495668.inc"
};
char D_00495670[0x10] = {
#include "sub_main/00495670.inc"
};
char D_00495680[0x10] = {
#include "sub_main/00495680.inc"
};
char D_00495690[0x8] = {
#include "sub_main/00495690.inc"
};
char D_00495698[0x8] = {
#include "sub_main/00495698.inc"
};
char D_004956A0[0x8] = {
#include "sub_main/004956A0.inc"
};
char D_004956A8[0x8] = {
#include "sub_main/004956A8.inc"
};
char D_004956B0[0x8] = {
#include "sub_main/004956B0.inc"
};
char D_004956B8[0x8] = {
#include "sub_main/004956B8.inc"
};
char D_004956C0[0x8] = {
#include "sub_main/004956C0.inc"
};
char D_004956C8[0x8] = {
#include "sub_main/004956C8.inc"
};
char D_004956D0[0x18] = {
#include "sub_main/004956D0.inc"
};
char D_004956E8[0xC] = {
#include "sub_main/004956E8.inc"
};
char D_004956F8[0xC] = {
#include "sub_main/004956F8.inc"
};
char D_00495708[0xC] = {
#include "sub_main/00495708.inc"
};
char D_00495718[13] = {
#include "sub_main/00495718.inc"
};
char D_00495728[13] = {
#include "sub_main/00495728.inc"
};
char D_00495740[0x20] = {
#include "sub_main/00495740.inc"
};
char D_00495760[0x8] = {
#include "sub_main/00495760.inc"
};
char D_00495768[0xC] = {
#include "sub_main/00495768.inc"
};
char D_00495780[0x20] = {
#include "sub_main/00495780.inc"
};
char D_004957A0[0x18] = {
#include "sub_main/004957A0.inc"
};
char D_004957B8[0x8] = {
#include "sub_main/004957B8.inc"
};
char D_004957C0[0x30] = {
#include "sub_main/004957C0.inc"
};
char D_004957F0[0x30] = {
#include "sub_main/004957F0.inc"
};
char D_00495820[0x30] = {
#include "sub_main/00495820.inc"
};
char D_00495850[0x40] = {
#include "sub_main/00495850.inc"
};
char D_00495890[0x20] = {
#include "sub_main/00495890.inc"
};
char D_004958B0[0x20] = {
#include "sub_main/004958B0.inc"
};
char D_004958D0[0x50] = {
#include "sub_main/004958D0.inc"
};
char D_00495920[0x20] = {
#include "sub_main/00495920.inc"
};
char D_00495940[0x10] = {
#include "sub_main/00495940.inc"
};
char D_00495950[0x10] = {
#include "sub_main/00495950.inc"
};
char D_00495960[0x20] = {
#include "sub_main/00495960.inc"
};
char D_00495980[0x20] = {
#include "sub_main/00495980.inc"
};
char D_004959A0[0x20] = {
#include "sub_main/004959A0.inc"
};
char D_004959C0[0x20] = {
#include "sub_main/004959C0.inc"
};
char D_004959E0[0x20] = {
#include "sub_main/004959E0.inc"
};
char D_00495A00[0x8] = {
#include "sub_main/00495A00.inc"
};
char D_00495A08[15] = {
#include "sub_main/00495A08.inc"
};
char D_00495A18[0xC] = {
#include "sub_main/00495A18.inc"
};
char D_00495A28[0x8] = {
#include "sub_main/00495A28.inc"
};
char D_00495A30[0x8] = {
#include "sub_main/00495A30.inc"
};
char D_00495A38[15] = {
#include "sub_main/00495A38.inc"
};
char D_00495A48[0x8] = {
#include "sub_main/00495A48.inc"
};
char D_00495A50[0x8] = {
#include "sub_main/00495A50.inc"
};
char D_00495A58[0x8] = {
#include "sub_main/00495A58.inc"
};
char D_00495A60[0x10] = {
#include "sub_main/00495A60.inc"
};
char D_00495A70[0x8] = {
#include "sub_main/00495A70.inc"
};
char D_00495A78[13] = {
#include "sub_main/00495A78.inc"
};
char D_00495A88[15] = {
#include "sub_main/00495A88.inc"
};
char D_00495AA0[0x30] = {
#include "sub_main/00495AA0.inc"
};
char D_00495AD0[0x8] = {
#include "sub_main/00495AD0.inc"
};
char D_00495AD8[13] = {
#include "sub_main/00495AD8.inc"
};
char D_00495AE8[0x8] = {
#include "sub_main/00495AE8.inc"
};
char D_00495AF0[0x10] = {
#include "sub_main/00495AF0.inc"
};
char D_00495B00[0x10] = {
#include "sub_main/00495B00.inc"
};
char D_00495B10[0x20] = {
#include "sub_main/00495B10.inc"
};
char D_00495B30[0x20] = {
#include "sub_main/00495B30.inc"
};
char D_00495B50[0x20] = {
#include "sub_main/00495B50.inc"
};
char D_00495B70[0x10] = {
#include "sub_main/00495B70.inc"
};
char D_00495B80[0x28] = {
#include "sub_main/00495B80.inc"
};
char D_00495BA8[0xC] = {
#include "sub_main/00495BA8.inc"
};
char D_00495BC0[0x10] = {
#include "sub_main/00495BC0.inc"
};
char D_00495BD0[0x30] = {
#include "sub_main/00495BD0.inc"
};
char D_00495C00[0x28] = {
#include "sub_main/00495C00.inc"
};
char D_00495C28[0x8] = {
#include "sub_main/00495C28.inc"
};
char D_00495C30[0x8] = {
#include "sub_main/00495C30.inc"
};
char D_00495C38[13] = {
#include "sub_main/00495C38.inc"
};
char D_00495C48[0xC] = {
#include "sub_main/00495C48.inc"
};
char D_00495C58[0xC] = {
#include "sub_main/00495C58.inc"
};
char D_00495C70[0x18] = {
#include "sub_main/00495C70.inc"
};
char D_00495C88[0xC] = {
#include "sub_main/00495C88.inc"
};
char D_00495C98[0xC] = {
#include "sub_main/00495C98.inc"
};
char D_00495CB0[0x18] = {
#include "sub_main/00495CB0.inc"
};
char D_00495CC8[0xC] = {
#include "sub_main/00495CC8.inc"
};
u32 D_00495CD8[3] = {
#include "sub_main/00495CD8.inc"
};
char D_00495CE8[0xC] = {
#include "sub_main/00495CE8.inc"
};
char D_00495CF8[0xC] = {
#include "sub_main/00495CF8.inc"
};
u32 D_00495D08[3] = {
#include "sub_main/00495D08.inc"
};
char D_00495D20[0x20] = {
#include "sub_main/00495D20.inc"
};
char D_00495D40[0x20] = {
#include "sub_main/00495D40.inc"
};
char D_00495D60[0x20] = {
#include "sub_main/00495D60.inc"
};
char D_00495D80[0x10] = {
#include "sub_main/00495D80.inc"
};
char D_00495D90[0x8] = {
#include "sub_main/00495D90.inc"
};
char D_00495D98[0xC] = {
#include "sub_main/00495D98.inc"
};
char D_00495DA8[0x8] = {
#include "sub_main/00495DA8.inc"
};
char D_00495DB0[0x10] = {
#include "sub_main/00495DB0.inc"
};
char D_00495DC0[0x8] = {
#include "sub_main/00495DC0.inc"
};
char D_00495DC8[0x8] = {
#include "sub_main/00495DC8.inc"
};
char D_00495DD0[0x8] = {
#include "sub_main/00495DD0.inc"
};
char D_00495DD8[0x8] = {
#include "sub_main/00495DD8.inc"
};
char D_00495DE0[0x8] = {
#include "sub_main/00495DE0.inc"
};
char D_00495DE8[0x8] = {
#include "sub_main/00495DE8.inc"
};
char D_00495DF0[0x8] = {
#include "sub_main/00495DF0.inc"
};
char D_00495DF8[0x8] = {
#include "sub_main/00495DF8.inc"
};
char D_00495E00[0x8] = {
#include "sub_main/00495E00.inc"
};
char D_00495E08[0xC] = {
#include "sub_main/00495E08.inc"
};
char D_00495E18[15] = {
#include "sub_main/00495E18.inc"
};
char D_00495E28[0xC] = {
#include "sub_main/00495E28.inc"
};
char D_00495E38[15] = {
#include "sub_main/00495E38.inc"
};
char D_00495E50[0x20] = {
#include "sub_main/00495E50.inc"
};
char D_00495E70[0x18] = {
#include "sub_main/00495E70.inc"
};
char D_00495E88[13] = {
#include "sub_main/00495E88.inc"
};
char D_00495E98[13] = {
#include "sub_main/00495E98.inc"
};
char D_00495EA8[13] = {
#include "sub_main/00495EA8.inc"
};
char D_00495EB8[13] = {
#include "sub_main/00495EB8.inc"
};
char D_00495EC8[0x8] = {
#include "sub_main/00495EC8.inc"
};
char D_00495ED0[0x28] = {
#include "sub_main/00495ED0.inc"
};
char D_00495EF8[0x8] = {
#include "sub_main/00495EF8.inc"
};
char D_00495F00[0x10] = {
#include "sub_main/00495F00.inc"
};
char D_00495F10[0x18] = {
#include "sub_main/00495F10.inc"
};
char D_00495F28[13] = {
#include "sub_main/00495F28.inc"
};
char D_00495F40[0x20] = {
#include "sub_main/00495F40.inc"
};
char D_00495F60[0x18] = {
#include "sub_main/00495F60.inc"
};
char D_00495F78[0xC] = {
#include "sub_main/00495F78.inc"
};
char D_00495F90[0x20] = {
#include "sub_main/00495F90.inc"
};
char D_00495FB0[0x30] = {
#include "sub_main/00495FB0.inc"
};
char D_00495FE0[0x18] = {
#include "sub_main/00495FE0.inc"
};
char D_00495FF8[0xC] = {
#include "sub_main/00495FF8.inc"
};
char D_00496010[0x20] = {
#include "sub_main/00496010.inc"
};
char D_00496030[0x30] = {
#include "sub_main/00496030.inc"
};
char D_00496060[0x20] = {
#include "sub_main/00496060.inc"
};
char D_00496080[0x18] = {
#include "sub_main/00496080.inc"
};
char D_00496098[0xC] = {
#include "sub_main/00496098.inc"
};
char D_004960A8[0x8] = {
#include "sub_main/004960A8.inc"
};
char D_004960B0[0x10] = {
#include "sub_main/004960B0.inc"
};
char D_004960C0[0x10] = {
#include "sub_main/004960C0.inc"
};
char D_004960D0[0x10] = {
#include "sub_main/004960D0.inc"
};
char D_004960E0[0x10] = {
#include "sub_main/004960E0.inc"
};
char D_004960F0[0x10] = {
#include "sub_main/004960F0.inc"
};
char D_00496100[0x8] = {
#include "sub_main/00496100.inc"
};
char D_00496108[0x8] = {
#include "sub_main/00496108.inc"
};
char D_00496110[0x8] = {
#include "sub_main/00496110.inc"
};
char D_00496118[0x8] = {
#include "sub_main/00496118.inc"
};
char D_00496120[0x8] = {
#include "sub_main/00496120.inc"
};
char D_00496128[0x8] = {
#include "sub_main/00496128.inc"
};
char D_00496130[0x8] = {
#include "sub_main/00496130.inc"
};
char D_00496138[0x8] = {
#include "sub_main/00496138.inc"
};
char D_00496140[0x8] = {
#include "sub_main/00496140.inc"
};
char D_00496148[13] = {
#include "sub_main/00496148.inc"
};
char D_00496158[13] = {
#include "sub_main/00496158.inc"
};
char D_00496168[13] = {
#include "sub_main/00496168.inc"
};
char D_00496178[13] = {
#include "sub_main/00496178.inc"
};
char D_00496188[0x8] = {
#include "sub_main/00496188.inc"
};
char D_00496190[0x8] = {
#include "sub_main/00496190.inc"
};
char D_00496198[0x8] = {
#include "sub_main/00496198.inc"
};
char D_004961A0[0x8] = {
#include "sub_main/004961A0.inc"
};
char D_004961A8[0x8] = {
#include "sub_main/004961A8.inc"
};
char D_004961B0[0x8] = {
#include "sub_main/004961B0.inc"
};
char D_004961B8[0x8] = {
#include "sub_main/004961B8.inc"
};
char D_004961C0[0x10] = {
#include "sub_main/004961C0.inc"
};
char D_004961D0[0x20] = {
#include "sub_main/004961D0.inc"
};
char D_004961F0[0x18] = {
#include "sub_main/004961F0.inc"
};
char D_00496208[15] = {
#include "sub_main/00496208.inc"
};
char D_00496218[15] = {
#include "sub_main/00496218.inc"
};
char D_00496228[15] = {
#include "sub_main/00496228.inc"
};
char D_00496238[15] = {
#include "sub_main/00496238.inc"
};
char D_00496248[0x8] = {
#include "sub_main/00496248.inc"
};
char D_00496250[0x8] = {
#include "sub_main/00496250.inc"
};
char D_00496258[13] = {
#include "sub_main/00496258.inc"
};
char D_00496268[0xC] = {
#include "sub_main/00496268.inc"
};
char D_00496278[0x8] = {
#include "sub_main/00496278.inc"
};
char D_00496280[0x8] = {
#include "sub_main/00496280.inc"
};
char D_00496288[0xC] = {
#include "sub_main/00496288.inc"
};
char D_00496298[0xC] = {
#include "sub_main/00496298.inc"
};
char D_004962A8[0xC] = {
#include "sub_main/004962A8.inc"
};
char D_004962B8[0xC] = {
#include "sub_main/004962B8.inc"
};
char D_004962C8[0x8] = {
#include "sub_main/004962C8.inc"
};
char D_004962D0[0x8] = {
#include "sub_main/004962D0.inc"
};
char D_004962D8[0x8] = {
#include "sub_main/004962D8.inc"
};
char D_004962E0[0x8] = {
#include "sub_main/004962E0.inc"
};
char D_004962E8[0x8] = {
#include "sub_main/004962E8.inc"
};
char D_004962F0[0x8] = {
#include "sub_main/004962F0.inc"
};
char D_004962F8[0x8] = {
#include "sub_main/004962F8.inc"
};
char D_00496300[0x8] = {
#include "sub_main/00496300.inc"
};
char D_00496308[0x8] = {
#include "sub_main/00496308.inc"
};
char D_00496310[0x8] = {
#include "sub_main/00496310.inc"
};
char D_00496318[0x8] = {
#include "sub_main/00496318.inc"
};
char D_00496320[0x8] = {
#include "sub_main/00496320.inc"
};
char D_00496328[0x8] = {
#include "sub_main/00496328.inc"
};
char D_00496330[0x8] = {
#include "sub_main/00496330.inc"
};
char D_00496338[0x8] = {
#include "sub_main/00496338.inc"
};
char D_00496340[0x8] = {
#include "sub_main/00496340.inc"
};
char D_00496348[0x8] = {
#include "sub_main/00496348.inc"
};
char D_00496350[0x10] = {
#include "sub_main/00496350.inc"
};
char D_00496360[0x18] = {
#include "sub_main/00496360.inc"
};
char D_00496378[0xC] = {
#include "sub_main/00496378.inc"
};
char D_00496388[15] = {
#include "sub_main/00496388.inc"
};
char D_00496398[0x8] = {
#include "sub_main/00496398.inc"
};
char D_004963A0[0x30] = {
#include "sub_main/004963A0.inc"
};
char D_004963D0[0x20] = {
#include "sub_main/004963D0.inc"
};
char D_004963F0[0x10] = {
#include "sub_main/004963F0.inc"
};
char D_00496400[0x10] = {
#include "sub_main/00496400.inc"
};
char D_00496410[0x10] = {
#include "sub_main/00496410.inc"
};
char D_00496420[0x10] = {
#include "sub_main/00496420.inc"
};
char D_00496430[0x10] = {
#include "sub_main/00496430.inc"
};
char D_00496440[0x10] = {
#include "sub_main/00496440.inc"
};
char D_00496450[0x10] = {
#include "sub_main/00496450.inc"
};
char D_00496460[0x10] = {
#include "sub_main/00496460.inc"
};
char D_00496470[0x10] = {
#include "sub_main/00496470.inc"
};
char D_00496480[0x10] = {
#include "sub_main/00496480.inc"
};
char D_00496490[0x10] = {
#include "sub_main/00496490.inc"
};
char D_004964A0[0x10] = {
#include "sub_main/004964A0.inc"
};
char D_004964B0[0x10] = {
#include "sub_main/004964B0.inc"
};
char D_004964C0[0x10] = {
#include "sub_main/004964C0.inc"
};
char D_004964D0[0x10] = {
#include "sub_main/004964D0.inc"
};
char D_004964E0[0x10] = {
#include "sub_main/004964E0.inc"
};
char D_004964F0[0x8] = {
#include "sub_main/004964F0.inc"
};
char D_004964F8[0x8] = {
#include "sub_main/004964F8.inc"
};
char D_00496500[0x8] = {
#include "sub_main/00496500.inc"
};
char D_00496508[0x8] = {
#include "sub_main/00496508.inc"
};
char D_00496510[0x8] = {
#include "sub_main/00496510.inc"
};
char D_00496518[0x8] = {
#include "sub_main/00496518.inc"
};
char D_00496520[0x8] = {
#include "sub_main/00496520.inc"
};
char D_00496528[0x8] = {
#include "sub_main/00496528.inc"
};
char D_00496530[0x8] = {
#include "sub_main/00496530.inc"
};
char D_00496538[0x8] = {
#include "sub_main/00496538.inc"
};
char D_00496540[0x8] = {
#include "sub_main/00496540.inc"
};
char D_00496548[0x8] = {
#include "sub_main/00496548.inc"
};
char D_00496550[0x8] = {
#include "sub_main/00496550.inc"
};
char D_00496558[0xC] = {
#include "sub_main/00496558.inc"
};
char D_00496568[0x8] = {
#include "sub_main/00496568.inc"
};
char D_00496570[0x8] = {
#include "sub_main/00496570.inc"
};
char D_00496578[0x8] = {
#include "sub_main/00496578.inc"
};
char D_00496580[0x8] = {
#include "sub_main/00496580.inc"
};
char D_00496588[0x8] = {
#include "sub_main/00496588.inc"
};
char D_00496590[0x8] = {
#include "sub_main/00496590.inc"
};
char D_00496598[0xC] = {
#include "sub_main/00496598.inc"
};
char D_004965A8[0x8] = {
#include "sub_main/004965A8.inc"
};
char D_004965B0[0x8] = {
#include "sub_main/004965B0.inc"
};
char D_004965B8[0x8] = {
#include "sub_main/004965B8.inc"
};
char D_004965C0[0x8] = {
#include "sub_main/004965C0.inc"
};
char D_004965C8[0x8] = {
#include "sub_main/004965C8.inc"
};
char D_004965D0[0x8] = {
#include "sub_main/004965D0.inc"
};
char D_004965D8[0x8] = {
#include "sub_main/004965D8.inc"
};
char D_004965E0[0x8] = {
#include "sub_main/004965E0.inc"
};
char D_004965E8[0x8] = {
#include "sub_main/004965E8.inc"
};
char D_004965F0[0x8] = {
#include "sub_main/004965F0.inc"
};
char D_004965F8[0x8] = {
#include "sub_main/004965F8.inc"
};
char D_00496600[0x8] = {
#include "sub_main/00496600.inc"
};
char D_00496608[0x8] = {
#include "sub_main/00496608.inc"
};
char D_00496610[0x8] = {
#include "sub_main/00496610.inc"
};
char D_00496618[0x8] = {
#include "sub_main/00496618.inc"
};
char D_00496620[0x8] = {
#include "sub_main/00496620.inc"
};
char D_00496628[0xC] = {
#include "sub_main/00496628.inc"
};
char D_00496638[0xC] = {
#include "sub_main/00496638.inc"
};
char D_00496648[0x8] = {
#include "sub_main/00496648.inc"
};
char D_00496650[0x8] = {
#include "sub_main/00496650.inc"
};
char D_00496658[0x8] = {
#include "sub_main/00496658.inc"
};
char D_00496660[0x8] = {
#include "sub_main/00496660.inc"
};
char D_00496668[0x8] = {
#include "sub_main/00496668.inc"
};
char D_00496670[0x8] = {
#include "sub_main/00496670.inc"
};
char D_00496678[0x8] = {
#include "sub_main/00496678.inc"
};
char D_00496680[0x8] = {
#include "sub_main/00496680.inc"
};
char D_00496688[0x8] = {
#include "sub_main/00496688.inc"
};
char D_00496690[0x8] = {
#include "sub_main/00496690.inc"
};
char D_00496698[0xC] = {
#include "sub_main/00496698.inc"
};
char D_004966A8[0xC] = {
#include "sub_main/004966A8.inc"
};
char D_004966B8[0x8] = {
#include "sub_main/004966B8.inc"
};
char D_004966C0[0x8] = {
#include "sub_main/004966C0.inc"
};
char D_004966C8[0x8] = {
#include "sub_main/004966C8.inc"
};
char D_004966D0[0x8] = {
#include "sub_main/004966D0.inc"
};
char D_004966D8[0x8] = {
#include "sub_main/004966D8.inc"
};
char D_004966E0[0x8] = {
#include "sub_main/004966E0.inc"
};
char D_004966E8[0x8] = {
#include "sub_main/004966E8.inc"
};
char D_004966F0[0x8] = {
#include "sub_main/004966F0.inc"
};
char D_004966F8[0x8] = {
#include "sub_main/004966F8.inc"
};
char D_00496700[0x10] = {
#include "sub_main/00496700.inc"
};
char D_00496710[0x10] = {
#include "sub_main/00496710.inc"
};
char D_00496720[0x8] = {
#include "sub_main/00496720.inc"
};
char D_00496728[0x8] = {
#include "sub_main/00496728.inc"
};
char D_00496730[0x8] = {
#include "sub_main/00496730.inc"
};
char D_00496738[0x8] = {
#include "sub_main/00496738.inc"
};
char D_00496740[0x8] = {
#include "sub_main/00496740.inc"
};
char D_00496748[0xC] = {
#include "sub_main/00496748.inc"
};
char D_00496758[0xC] = {
#include "sub_main/00496758.inc"
};
char D_00496768[0x8] = {
#include "sub_main/00496768.inc"
};
char D_00496770[0x8] = {
#include "sub_main/00496770.inc"
};
char D_00496778[0xC] = {
#include "sub_main/00496778.inc"
};
char D_00496788[0xC] = {
#include "sub_main/00496788.inc"
};
char D_00496798[0x8] = {
#include "sub_main/00496798.inc"
};
char D_004967A0[0x8] = {
#include "sub_main/004967A0.inc"
};
char D_004967A8[0x8] = {
#include "sub_main/004967A8.inc"
};
char D_004967B0[0x10] = {
#include "sub_main/004967B0.inc"
};
char D_004967C0[0x10] = {
#include "sub_main/004967C0.inc"
};
char D_004967D0[0x10] = {
#include "sub_main/004967D0.inc"
};
char D_004967E0[0x10] = {
#include "sub_main/004967E0.inc"
};
char D_004967F0[0x8] = {
#include "sub_main/004967F0.inc"
};
char D_004967F8[0x8] = {
#include "sub_main/004967F8.inc"
};
char D_00496800[0x8] = {
#include "sub_main/00496800.inc"
};
char D_00496808[0x8] = {
#include "sub_main/00496808.inc"
};
char D_00496810[0x8] = {
#include "sub_main/00496810.inc"
};
char D_00496818[0x8] = {
#include "sub_main/00496818.inc"
};
char D_00496820[0x10] = {
#include "sub_main/00496820.inc"
};
char D_00496830[0x10] = {
#include "sub_main/00496830.inc"
};
char D_00496840[0x10] = {
#include "sub_main/00496840.inc"
};
char D_00496850[0x10] = {
#include "sub_main/00496850.inc"
};
char D_00496860[0x10] = {
#include "sub_main/00496860.inc"
};
char D_00496870[0x10] = {
#include "sub_main/00496870.inc"
};
char D_00496880[0x10] = {
#include "sub_main/00496880.inc"
};
char D_00496890[0x10] = {
#include "sub_main/00496890.inc"
};
char D_004968A0[0x10] = {
#include "sub_main/004968A0.inc"
};
char D_004968B0[0x8] = {
#include "sub_main/004968B0.inc"
};
char D_004968B8[0xC] = {
#include "sub_main/004968B8.inc"
};
char D_004968C8[0xC] = {
#include "sub_main/004968C8.inc"
};
char D_004968D8[0xC] = {
#include "sub_main/004968D8.inc"
};
char D_004968E8[0xC] = {
#include "sub_main/004968E8.inc"
};
char D_004968F8[0x8] = {
#include "sub_main/004968F8.inc"
};
char D_00496900[0x8] = {
#include "sub_main/00496900.inc"
};
char D_00496908[0xC] = {
#include "sub_main/00496908.inc"
};
char D_00496918[0xC] = {
#include "sub_main/00496918.inc"
};
char D_00496928[0xC] = {
#include "sub_main/00496928.inc"
};
char D_00496938[0xC] = {
#include "sub_main/00496938.inc"
};
char D_00496948[0xC] = {
#include "sub_main/00496948.inc"
};
char D_00496958[0xC] = {
#include "sub_main/00496958.inc"
};
char D_00496968[0xC] = {
#include "sub_main/00496968.inc"
};
char D_00496978[0xC] = {
#include "sub_main/00496978.inc"
};
char D_00496988[13] = {
#include "sub_main/00496988.inc"
};
char D_00496998[13] = {
#include "sub_main/00496998.inc"
};
char D_004969A8[0xC] = {
#include "sub_main/004969A8.inc"
};
char D_004969B8[0xC] = {
#include "sub_main/004969B8.inc"
};
char D_004969C8[13] = {
#include "sub_main/004969C8.inc"
};
char D_004969D8[13] = {
#include "sub_main/004969D8.inc"
};
char D_004969E8[13] = {
#include "sub_main/004969E8.inc"
};
char D_00496A00[0x20] = {
#include "sub_main/00496A00.inc"
};
char D_00496A20[0x18] = {
#include "sub_main/00496A20.inc"
};
char D_00496A38[13] = {
#include "sub_main/00496A38.inc"
};
char D_00496A50[0x20] = {
#include "sub_main/00496A50.inc"
};
char D_00496A70[0x18] = {
#include "sub_main/00496A70.inc"
};
char D_00496A88[13] = {
#include "sub_main/00496A88.inc"
};
char D_00496AA0[0x20] = {
#include "sub_main/00496AA0.inc"
};
char D_00496AC0[0x20] = {
#include "sub_main/00496AC0.inc"
};
char D_00496AE0[0x20] = {
#include "sub_main/00496AE0.inc"
};
char D_00496B00[0x20] = {
#include "sub_main/00496B00.inc"
};
char D_00496B20[0x20] = {
#include "sub_main/00496B20.inc"
};
char D_00496B40[0x20] = {
#include "sub_main/00496B40.inc"
};
char D_00496B60[0x20] = {
#include "sub_main/00496B60.inc"
};
char D_00496B80[0x20] = {
#include "sub_main/00496B80.inc"
};
char D_00496BA0[0x20] = {
#include "sub_main/00496BA0.inc"
};
char D_00496BC0[0x20] = {
#include "sub_main/00496BC0.inc"
};
char D_00496BE0[0x20] = {
#include "sub_main/00496BE0.inc"
};
char D_00496C00[0x20] = {
#include "sub_main/00496C00.inc"
};
char D_00496C20[0x30] = {
#include "sub_main/00496C20.inc"
};
char D_00496C50[0x20] = {
#include "sub_main/00496C50.inc"
};
char D_00496C70[0x20] = {
#include "sub_main/00496C70.inc"
};
char D_00496C90[0x20] = {
#include "sub_main/00496C90.inc"
};
char D_00496CB0[0x20] = {
#include "sub_main/00496CB0.inc"
};
char D_00496CD0[0x30] = {
#include "sub_main/00496CD0.inc"
};
char D_00496D00[0x20] = {
#include "sub_main/00496D00.inc"
};
char D_00496D20[0x20] = {
#include "sub_main/00496D20.inc"
};
char D_00496D40[0x20] = {
#include "sub_main/00496D40.inc"
};
char D_00496D60[0x20] = {
#include "sub_main/00496D60.inc"
};
char D_00496D80[0x20] = {
#include "sub_main/00496D80.inc"
};
char D_00496DA0[0x30] = {
#include "sub_main/00496DA0.inc"
};
char D_00496DD0[0x20] = {
#include "sub_main/00496DD0.inc"
};
char D_00496DF0[0x20] = {
#include "sub_main/00496DF0.inc"
};
char D_00496E10[0x20] = {
#include "sub_main/00496E10.inc"
};
char D_00496E30[0x18] = {
#include "sub_main/00496E30.inc"
};
char D_00496E48[15] = {
#include "sub_main/00496E48.inc"
};
char D_00496E58[15] = {
#include "sub_main/00496E58.inc"
};
char D_00496E68[15] = {
#include "sub_main/00496E68.inc"
};
char D_00496E78[0xC] = {
#include "sub_main/00496E78.inc"
};
char D_00496E90[0x18] = {
#include "sub_main/00496E90.inc"
};
char D_00496EA8[15] = {
#include "sub_main/00496EA8.inc"
};
char D_00496EB8[0x8] = {
#include "sub_main/00496EB8.inc"
};
char D_00496EC0[0x10] = {
#include "sub_main/00496EC0.inc"
};
char D_00496ED0[0x18] = {
#include "sub_main/00496ED0.inc"
};
char D_00496EE8[15] = {
#include "sub_main/00496EE8.inc"
};
char D_00496EF8[0xC] = {
#include "sub_main/00496EF8.inc"
};
char D_00496F10[0x18] = {
#include "sub_main/00496F10.inc"
};
char D_00496F28[13] = {
#include "sub_main/00496F28.inc"
};
char D_00496F38[13] = {
#include "sub_main/00496F38.inc"
};
char D_00496F48[0xC] = {
#include "sub_main/00496F48.inc"
};
char D_00496F60[0x20] = {
#include "sub_main/00496F60.inc"
};
char D_00496F80[0x28] = {
#include "sub_main/00496F80.inc"
};
char D_00496FA8[0xC] = {
#include "sub_main/00496FA8.inc"
};
char D_00496FB8[13] = {
#include "sub_main/00496FB8.inc"
};
char D_00496FD0[0x20] = {
#include "sub_main/00496FD0.inc"
};
char D_00496FF0[0x20] = {
#include "sub_main/00496FF0.inc"
};
char D_00497010[0x20] = {
#include "sub_main/00497010.inc"
};
char D_00497030[0x18] = {
#include "sub_main/00497030.inc"
};
char D_00497048[0xC] = {
#include "sub_main/00497048.inc"
};
char D_00497060[0x20] = {
#include "sub_main/00497060.inc"
};
char D_00497080[0x20] = {
#include "sub_main/00497080.inc"
};
char D_004970A0[0x10] = {
#include "sub_main/004970A0.inc"
};
char D_004970B0[0x8] = {
#include "sub_main/004970B0.inc"
};
char D_004970B8[0xC] = {
#include "sub_main/004970B8.inc"
};
char D_004970D0[0x30] = {
#include "sub_main/004970D0.inc"
};
char D_00497100[0x20] = {
#include "sub_main/00497100.inc"
};
char D_00497120[0x18] = {
#include "sub_main/00497120.inc"
};
char D_00497138[13] = {
#include "sub_main/00497138.inc"
};
char D_00497150[0x20] = {
#include "sub_main/00497150.inc"
};
char D_00497170[0x20] = {
#include "sub_main/00497170.inc"
};
char D_00497190[0x20] = {
#include "sub_main/00497190.inc"
};
char D_004971B0[0x20] = {
#include "sub_main/004971B0.inc"
};
char D_004971D0[0x20] = {
#include "sub_main/004971D0.inc"
};
char D_004971F0[0x20] = {
#include "sub_main/004971F0.inc"
};
char D_00497210[0x20] = {
#include "sub_main/00497210.inc"
};
char D_00497230[0x20] = {
#include "sub_main/00497230.inc"
};
char D_00497250[0x50] = {
#include "sub_main/00497250.inc"
};
char D_004972A0[0x60] = {
#include "sub_main/004972A0.inc"
};
char D_00497300[0x40] = {
#include "sub_main/00497300.inc"
};
char D_00497340[0x40] = {
#include "sub_main/00497340.inc"
};
char D_00497380[0x60] = {
#include "sub_main/00497380.inc"
};
char D_004973E0[0x60] = {
#include "sub_main/004973E0.inc"
};
char D_00497440[0x70] = {
#include "sub_main/00497440.inc"
};
char D_004974B0[0x30] = {
#include "sub_main/004974B0.inc"
};
char D_004974E0[0x60] = {
#include "sub_main/004974E0.inc"
};
char D_00497540[0x50] = {
#include "sub_main/00497540.inc"
};
char D_00497590[0x40] = {
#include "sub_main/00497590.inc"
};
char D_004975D0[0x40] = {
#include "sub_main/004975D0.inc"
};
char D_00497610[0x70] = {
#include "sub_main/00497610.inc"
};
char D_00497680[0x60] = {
#include "sub_main/00497680.inc"
};
char D_004976E0[0x70] = {
#include "sub_main/004976E0.inc"
};
char D_00497750[0x60] = {
#include "sub_main/00497750.inc"
};
char D_004977B0[0x60] = {
#include "sub_main/004977B0.inc"
};
char D_00497810[0x70] = {
#include "sub_main/00497810.inc"
};
char D_00497880[0x40] = {
#include "sub_main/00497880.inc"
};
char D_004978C0[0x40] = {
#include "sub_main/004978C0.inc"
};
char D_00497900[0x70] = {
#include "sub_main/00497900.inc"
};
char D_00497970[0x60] = {
#include "sub_main/00497970.inc"
};
char D_004979D0[0x50] = {
#include "sub_main/004979D0.inc"
};
char D_00497A20[0x40] = {
#include "sub_main/00497A20.inc"
};
char D_00497A60[0x40] = {
#include "sub_main/00497A60.inc"
};
char D_00497AA0[0x70] = {
#include "sub_main/00497AA0.inc"
};
char D_00497B10[0x60] = {
#include "sub_main/00497B10.inc"
};
char D_00497B70[0x50] = {
#include "sub_main/00497B70.inc"
};
char D_00497BC0[0x70] = {
#include "sub_main/00497BC0.inc"
};
char D_00497C30[0x60] = {
#include "sub_main/00497C30.inc"
};
char D_00497C90[0x50] = {
#include "sub_main/00497C90.inc"
};
char D_00497CE0[0x50] = {
#include "sub_main/00497CE0.inc"
};
char D_00497D30[0x60] = {
#include "sub_main/00497D30.inc"
};
char D_00497D90[0x50] = {
#include "sub_main/00497D90.inc"
};
char D_00497DE0[0x60] = {
#include "sub_main/00497DE0.inc"
};
char D_00497E40[0x50] = {
#include "sub_main/00497E40.inc"
};
char D_00497E90[0x70] = {
#include "sub_main/00497E90.inc"
};
char D_00497F00[0x80] = {
#include "sub_main/00497F00.inc"
};
char D_00497F80[0x80] = {
#include "sub_main/00497F80.inc"
};
char D_00498000[0x90] = {
#include "sub_main/00498000.inc"
};
char D_00498090[0x90] = {
#include "sub_main/00498090.inc"
};
char D_00498120[0x80] = {
#include "sub_main/00498120.inc"
};
char D_004981A0[0x80] = {
#include "sub_main/004981A0.inc"
};
char D_00498220[0xB0] = {
#include "sub_main/00498220.inc"
};
char D_004982D0[0x70] = {
#include "sub_main/004982D0.inc"
};
char D_00498340[0x80] = {
#include "sub_main/00498340.inc"
};
char D_004983C0[0x70] = {
#include "sub_main/004983C0.inc"
};
char D_00498430[0x70] = {
#include "sub_main/00498430.inc"
};
char D_004984A0[0xA0] = {
#include "sub_main/004984A0.inc"
};
char D_00498540[0x80] = {
#include "sub_main/00498540.inc"
};
char D_004985C0[0x70] = {
#include "sub_main/004985C0.inc"
};
char D_00498630[0xA0] = {
#include "sub_main/00498630.inc"
};
char D_004986D0[0x80] = {
#include "sub_main/004986D0.inc"
};
char D_00498750[0x70] = {
#include "sub_main/00498750.inc"
};
char D_004987C0[0x90] = {
#include "sub_main/004987C0.inc"
};
char D_00498850[0x70] = {
#include "sub_main/00498850.inc"
};
char D_004988C0[0x70] = {
#include "sub_main/004988C0.inc"
};
char D_00498930[0x70] = {
#include "sub_main/00498930.inc"
};
char D_004989A0[0x60] = {
#include "sub_main/004989A0.inc"
};
char D_00498A00[0x70] = {
#include "sub_main/00498A00.inc"
};
char D_00498A70[0x70] = {
#include "sub_main/00498A70.inc"
};
char D_00498AE0[0x80] = {
#include "sub_main/00498AE0.inc"
};
char D_00498B60[0x70] = {
#include "sub_main/00498B60.inc"
};
char D_00498BD0[0x60] = {
#include "sub_main/00498BD0.inc"
};
char D_00498C30[0x60] = {
#include "sub_main/00498C30.inc"
};
char D_00498C90[0x60] = {
#include "sub_main/00498C90.inc"
};
char D_00498CF0[0x80] = {
#include "sub_main/00498CF0.inc"
};
char D_00498D70[0x70] = {
#include "sub_main/00498D70.inc"
};
char D_00498DE0[0x80] = {
#include "sub_main/00498DE0.inc"
};
char D_00498E60[0x90] = {
#include "sub_main/00498E60.inc"
};
char D_00498EF0[0x60] = {
#include "sub_main/00498EF0.inc"
};
char D_00498F50[0x60] = {
#include "sub_main/00498F50.inc"
};
char D_00498FB0[0x80] = {
#include "sub_main/00498FB0.inc"
};
char D_00499030[0x70] = {
#include "sub_main/00499030.inc"
};
char D_004990A0[0x90] = {
#include "sub_main/004990A0.inc"
};
char D_00499130[0x70] = {
#include "sub_main/00499130.inc"
};
char D_004991A0[0x70] = {
#include "sub_main/004991A0.inc"
};
char D_00499210[0x90] = {
#include "sub_main/00499210.inc"
};
char D_004992A0[0x70] = {
#include "sub_main/004992A0.inc"
};
char D_00499310[0x80] = {
#include "sub_main/00499310.inc"
};
char D_00499390[0x70] = {
#include "sub_main/00499390.inc"
};
char D_00499400[0x80] = {
#include "sub_main/00499400.inc"
};
char D_00499480[0x60] = {
#include "sub_main/00499480.inc"
};
char D_004994E0[0x70] = {
#include "sub_main/004994E0.inc"
};
char D_00499550[0x70] = {
#include "sub_main/00499550.inc"
};
char D_004995C0[0x70] = {
#include "sub_main/004995C0.inc"
};
char D_00499630[0x70] = {
#include "sub_main/00499630.inc"
};
char D_004996A0[0x70] = {
#include "sub_main/004996A0.inc"
};
char D_00499710[0x80] = {
#include "sub_main/00499710.inc"
};
char D_00499790[0x50] = {
#include "sub_main/00499790.inc"
};
char D_004997E0[0x90] = {
#include "sub_main/004997E0.inc"
};
char D_00499870[0x70] = {
#include "sub_main/00499870.inc"
};
char D_004998E0[0x60] = {
#include "sub_main/004998E0.inc"
};
char D_00499940[0x20] = {
#include "sub_main/00499940.inc"
};
char D_00499960[0x60] = {
#include "sub_main/00499960.inc"
};
char D_004999C0[0x20] = {
#include "sub_main/004999C0.inc"
};
char D_004999E0[0x70] = {
#include "sub_main/004999E0.inc"
};
char D_00499A50[0x30] = {
#include "sub_main/00499A50.inc"
};
char D_00499A80[0x40] = {
#include "sub_main/00499A80.inc"
};
char D_00499AC0[0x40] = {
#include "sub_main/00499AC0.inc"
};
char D_00499B00[0x60] = {
#include "sub_main/00499B00.inc"
};
char D_00499B60[0x70] = {
#include "sub_main/00499B60.inc"
};
char D_00499BD0[0x80] = {
#include "sub_main/00499BD0.inc"
};
char D_00499C50[0x78] = {
#include "sub_main/00499C50.inc"
};
char D_00499CC8[0x8] = {
#include "sub_main/00499CC8.inc"
};
char D_00499CD0[0x8] = {
#include "sub_main/00499CD0.inc"
};
char D_00499CD8[0x8] = {
#include "sub_main/00499CD8.inc"
};
char D_00499CE0[0x8] = {
#include "sub_main/00499CE0.inc"
};
char D_00499CE8[0x8] = {
#include "sub_main/00499CE8.inc"
};
char D_00499CF0[0x8] = {
#include "sub_main/00499CF0.inc"
};
char D_00499CF8[0x8] = {
#include "sub_main/00499CF8.inc"
};
char D_00499D00[0x10] = {
#include "sub_main/00499D00.inc"
};
char D_00499D10[0x8] = {
#include "sub_main/00499D10.inc"
};
char D_00499D18[0xC] = {
#include "sub_main/00499D18.inc"
};
char D_00499D28[0x8] = {
#include "sub_main/00499D28.inc"
};
char D_00499D30[0x8] = {
#include "sub_main/00499D30.inc"
};
char D_00499D38[0x8] = {
#include "sub_main/00499D38.inc"
};
char D_00499D40[0x8] = {
#include "sub_main/00499D40.inc"
};
char D_00499D48[0x8] = {
#include "sub_main/00499D48.inc"
};
char D_00499D50[0x30] = {
#include "sub_main/00499D50.inc"
};
char D_00499D80[0x30] = {
#include "sub_main/00499D80.inc"
};
char D_00499DB0[0x8] = {
#include "sub_main/00499DB0.inc"
};
char D_00499DB8[0x8] = {
#include "sub_main/00499DB8.inc"
};
char D_00499DC0[0x10] = {
#include "sub_main/00499DC0.inc"
};
char D_00499DD0[0x8] = {
#include "sub_main/00499DD0.inc"
};
char D_00499DD8[0x8] = {
#include "sub_main/00499DD8.inc"
};
char D_00499DE0[0x8] = {
#include "sub_main/00499DE0.inc"
};
char D_00499DE8[0x8] = {
#include "sub_main/00499DE8.inc"
};
char D_00499DF0[0x8] = {
#include "sub_main/00499DF0.inc"
};
char D_00499DF8[0x8] = {
#include "sub_main/00499DF8.inc"
};
char D_00499E00[0x8] = {
#include "sub_main/00499E00.inc"
};
char D_00499E08[0x8] = {
#include "sub_main/00499E08.inc"
};
char D_00499E10[0x10] = {
#include "sub_main/00499E10.inc"
};
char D_00499E20[0x20] = {
#include "sub_main/00499E20.inc"
};
char D_00499E40[0x20] = {
#include "sub_main/00499E40.inc"
};
char D_00499E60[0x20] = {
#include "sub_main/00499E60.inc"
};
char D_00499E80[0x20] = {
#include "sub_main/00499E80.inc"
};
char D_00499EA0[0x20] = {
#include "sub_main/00499EA0.inc"
};
char D_00499EC0[0x30] = {
#include "sub_main/00499EC0.inc"
};
char D_00499EF0[0x58] = {
#include "sub_main/00499EF0.inc"
};
char D_00499F48[13] = {
#include "sub_main/00499F48.inc"
};
char D_00499F60[0x18] = {
#include "sub_main/00499F60.inc"
};
char D_00499F78[0x8] = {
#include "sub_main/00499F78.inc"
};
char D_00499F80[0x20] = {
#include "sub_main/00499F80.inc"
};
char D_00499FA0[0x20] = {
#include "sub_main/00499FA0.inc"
};
char D_00499FC0[0x10] = {
#include "sub_main/00499FC0.inc"
};
char D_00499FD0[0x10] = {
#include "sub_main/00499FD0.inc"
};
char D_00499FE0[0x40] = {
#include "sub_main/00499FE0.inc"
};
char D_0049A020[0x40] = {
#include "sub_main/0049A020.inc"
};
char D_0049A060[0x38] = {
#include "sub_main/0049A060.inc"
};
char D_0049A098[0x8] = {
#include "sub_main/0049A098.inc"
};
char D_0049A0A0[0x20] = {
#include "sub_main/0049A0A0.inc"
};
char D_0049A0C0[0x20] = {
#include "sub_main/0049A0C0.inc"
};
char D_0049A0E0[0x20] = {
#include "sub_main/0049A0E0.inc"
};
char D_0049A100[0x20] = {
#include "sub_main/0049A100.inc"
};
char D_0049A120[0x20] = {
#include "sub_main/0049A120.inc"
};
char D_0049A140[0x20] = {
#include "sub_main/0049A140.inc"
};
char D_0049A160[0x20] = {
#include "sub_main/0049A160.inc"
};
char D_0049A180[0x20] = {
#include "sub_main/0049A180.inc"
};
char D_0049A1A0[0x20] = {
#include "sub_main/0049A1A0.inc"
};
char D_0049A1C0[0x20] = {
#include "sub_main/0049A1C0.inc"
};
char D_0049A1E0[0x20] = {
#include "sub_main/0049A1E0.inc"
};
char D_0049A200[0x20] = {
#include "sub_main/0049A200.inc"
};
char D_0049A220[0x20] = {
#include "sub_main/0049A220.inc"
};
char D_0049A240[0x20] = {
#include "sub_main/0049A240.inc"
};
char D_0049A260[0x20] = {
#include "sub_main/0049A260.inc"
};
char D_0049A280[0x20] = {
#include "sub_main/0049A280.inc"
};
char D_0049A2A0[0x20] = {
#include "sub_main/0049A2A0.inc"
};
char D_0049A2C0[0x20] = {
#include "sub_main/0049A2C0.inc"
};
char D_0049A2E0[0x20] = {
#include "sub_main/0049A2E0.inc"
};
char D_0049A300[0x20] = {
#include "sub_main/0049A300.inc"
};
char D_0049A320[0x20] = {
#include "sub_main/0049A320.inc"
};
char D_0049A340[0x20] = {
#include "sub_main/0049A340.inc"
};
char D_0049A360[0x20] = {
#include "sub_main/0049A360.inc"
};
char D_0049A380[0x20] = {
#include "sub_main/0049A380.inc"
};
char D_0049A3A0[0x20] = {
#include "sub_main/0049A3A0.inc"
};
char D_0049A3C0[0x20] = {
#include "sub_main/0049A3C0.inc"
};
char D_0049A3E0[0x20] = {
#include "sub_main/0049A3E0.inc"
};
char D_0049A400[0x20] = {
#include "sub_main/0049A400.inc"
};
char D_0049A420[0x30] = {
#include "sub_main/0049A420.inc"
};
char D_0049A450[0x40] = {
#include "sub_main/0049A450.inc"
};
char D_0049A490[0x40] = {
#include "sub_main/0049A490.inc"
};
char D_0049A4D0[0x30] = {
#include "sub_main/0049A4D0.inc"
};
char D_0049A500[0x30] = {
#include "sub_main/0049A500.inc"
};
char D_0049A530[0x30] = {
#include "sub_main/0049A530.inc"
};
char D_0049A560[0x40] = {
#include "sub_main/0049A560.inc"
};
char D_0049A5A0[0x30] = {
#include "sub_main/0049A5A0.inc"
};
char D_0049A5D0[0x40] = {
#include "sub_main/0049A5D0.inc"
};
char D_0049A610[0x30] = {
#include "sub_main/0049A610.inc"
};
char D_0049A640[0x40] = {
#include "sub_main/0049A640.inc"
};
char D_0049A680[0x20] = {
#include "sub_main/0049A680.inc"
};
char D_0049A6A0[0x20] = {
#include "sub_main/0049A6A0.inc"
};
char D_0049A6C0[0x20] = {
#include "sub_main/0049A6C0.inc"
};
char D_0049A6E0[0x20] = {
#include "sub_main/0049A6E0.inc"
};
char D_0049A700[0x20] = {
#include "sub_main/0049A700.inc"
};
char D_0049A720[0x20] = {
#include "sub_main/0049A720.inc"
};
char D_0049A740[0x20] = {
#include "sub_main/0049A740.inc"
};
char D_0049A760[0x20] = {
#include "sub_main/0049A760.inc"
};
char D_0049A780[0x20] = {
#include "sub_main/0049A780.inc"
};
char D_0049A7A0[0x20] = {
#include "sub_main/0049A7A0.inc"
};
char D_0049A7C0[0x20] = {
#include "sub_main/0049A7C0.inc"
};
char D_0049A7E0[0x10] = {
#include "sub_main/0049A7E0.inc"
};
char D_0049A7F0[0x20] = {
#include "sub_main/0049A7F0.inc"
};
char D_0049A810[0x20] = {
#include "sub_main/0049A810.inc"
};
char D_0049A830[0x20] = {
#include "sub_main/0049A830.inc"
};
char D_0049A850[0x20] = {
#include "sub_main/0049A850.inc"
};
char D_0049A870[0x20] = {
#include "sub_main/0049A870.inc"
};
char D_0049A890[0x20] = {
#include "sub_main/0049A890.inc"
};
char D_0049A8B0[0x20] = {
#include "sub_main/0049A8B0.inc"
};
char D_0049A8D0[0x40] = {
#include "sub_main/0049A8D0.inc"
};
char D_0049A910[0x20] = {
#include "sub_main/0049A910.inc"
};
char D_0049A930[0x20] = {
#include "sub_main/0049A930.inc"
};
char D_0049A950[0x20] = {
#include "sub_main/0049A950.inc"
};
char D_0049A970[0x30] = {
#include "sub_main/0049A970.inc"
};
char D_0049A9A0[0x40] = {
#include "sub_main/0049A9A0.inc"
};
char D_0049A9E0[0x20] = {
#include "sub_main/0049A9E0.inc"
};
char D_0049AA00[0x20] = {
#include "sub_main/0049AA00.inc"
};
char D_0049AA20[0x20] = {
#include "sub_main/0049AA20.inc"
};
char D_0049AA40[0x20] = {
#include "sub_main/0049AA40.inc"
};
char D_0049AA60[0x20] = {
#include "sub_main/0049AA60.inc"
};
char D_0049AA80[0x20] = {
#include "sub_main/0049AA80.inc"
};
char D_0049AAA0[0x20] = {
#include "sub_main/0049AAA0.inc"
};
char D_0049AAC0[0x20] = {
#include "sub_main/0049AAC0.inc"
};
char D_0049AAE0[0x20] = {
#include "sub_main/0049AAE0.inc"
};
char D_0049AB00[0x20] = {
#include "sub_main/0049AB00.inc"
};
char D_0049AB20[0x20] = {
#include "sub_main/0049AB20.inc"
};
char D_0049AB40[0x20] = {
#include "sub_main/0049AB40.inc"
};
char D_0049AB60[0x20] = {
#include "sub_main/0049AB60.inc"
};
char D_0049AB80[0x20] = {
#include "sub_main/0049AB80.inc"
};
char D_0049ABA0[0x30] = {
#include "sub_main/0049ABA0.inc"
};
char D_0049ABD0[0x30] = {
#include "sub_main/0049ABD0.inc"
};
char D_0049AC00[0x30] = {
#include "sub_main/0049AC00.inc"
};
char D_0049AC30[0x20] = {
#include "sub_main/0049AC30.inc"
};
char D_0049AC50[0x20] = {
#include "sub_main/0049AC50.inc"
};
char D_0049AC70[0x20] = {
#include "sub_main/0049AC70.inc"
};
char D_0049AC90[0x20] = {
#include "sub_main/0049AC90.inc"
};
char D_0049ACB0[0x20] = {
#include "sub_main/0049ACB0.inc"
};
char D_0049ACD0[0x20] = {
#include "sub_main/0049ACD0.inc"
};
char D_0049ACF0[0x20] = {
#include "sub_main/0049ACF0.inc"
};
char D_0049AD10[0x20] = {
#include "sub_main/0049AD10.inc"
};
char D_0049AD30[0x20] = {
#include "sub_main/0049AD30.inc"
};
char D_0049AD50[0x20] = {
#include "sub_main/0049AD50.inc"
};
char D_0049AD70[0x20] = {
#include "sub_main/0049AD70.inc"
};
char D_0049AD90[0x30] = {
#include "sub_main/0049AD90.inc"
};
char D_0049ADC0[0x30] = {
#include "sub_main/0049ADC0.inc"
};
char D_0049ADF0[0x10] = {
#include "sub_main/0049ADF0.inc"
};
char D_0049AE00[0x10] = {
#include "sub_main/0049AE00.inc"
};
char D_0049AE10[0x10] = {
#include "sub_main/0049AE10.inc"
};
char D_0049AE20[0x10] = {
#include "sub_main/0049AE20.inc"
};
char D_0049AE30[0x10] = {
#include "sub_main/0049AE30.inc"
};
char D_0049AE40[0x10] = {
#include "sub_main/0049AE40.inc"
};
char D_0049AE50[0x10] = {
#include "sub_main/0049AE50.inc"
};
char D_0049AE60[0x10] = {
#include "sub_main/0049AE60.inc"
};
char D_0049AE70[0x10] = {
#include "sub_main/0049AE70.inc"
};
char D_0049AE80[0x10] = {
#include "sub_main/0049AE80.inc"
};
char D_0049AE90[0x8] = {
#include "sub_main/0049AE90.inc"
};
char D_0049AE98[0x8] = {
#include "sub_main/0049AE98.inc"
};
char D_0049AEA0[0x8] = {
#include "sub_main/0049AEA0.inc"
};
char D_0049AEA8[0x8] = {
#include "sub_main/0049AEA8.inc"
};
char D_0049AEB0[0x8] = {
#include "sub_main/0049AEB0.inc"
};
char D_0049AEB8[0xC] = {
#include "sub_main/0049AEB8.inc"
};
char D_0049AEC8[0x8] = {
#include "sub_main/0049AEC8.inc"
};
char D_0049AED0[0x8] = {
#include "sub_main/0049AED0.inc"
};
char D_0049AED8[0x8] = {
#include "sub_main/0049AED8.inc"
};
char D_0049AEE0[0x8] = {
#include "sub_main/0049AEE0.inc"
};
char D_0049AEE8[0x8] = {
#include "sub_main/0049AEE8.inc"
};
char D_0049AEF0[0x8] = {
#include "sub_main/0049AEF0.inc"
};
char D_0049AEF8[0x8] = {
#include "sub_main/0049AEF8.inc"
};
char D_0049AF00[0x8] = {
#include "sub_main/0049AF00.inc"
};
char D_0049AF08[0x8] = {
#include "sub_main/0049AF08.inc"
};
char D_0049AF10[0x8] = {
#include "sub_main/0049AF10.inc"
};
char D_0049AF18[0x8] = {
#include "sub_main/0049AF18.inc"
};
char D_0049AF20[0x20] = {
#include "sub_main/0049AF20.inc"
};
char D_0049AF40[0x20] = {
#include "sub_main/0049AF40.inc"
};
char D_0049AF60[0x20] = {
#include "sub_main/0049AF60.inc"
};
char D_0049AF80[0x18] = {
#include "sub_main/0049AF80.inc"
};
char D_0049AF98[0x8] = {
#include "sub_main/0049AF98.inc"
};
char D_0049AFA0[0x10] = {
#include "sub_main/0049AFA0.inc"
};
char D_0049AFB0[0x10] = {
#include "sub_main/0049AFB0.inc"
};
char D_0049AFC0[0x10] = {
#include "sub_main/0049AFC0.inc"
};
char D_0049AFD0[0x8] = {
#include "sub_main/0049AFD0.inc"
};
char D_0049AFD8[0xC] = {
#include "sub_main/0049AFD8.inc"
};
char D_0049AFE8[0xC] = {
#include "sub_main/0049AFE8.inc"
};
char D_0049AFF8[15] = {
#include "sub_main/0049AFF8.inc"
};
char D_0049B008[13] = {
#include "sub_main/0049B008.inc"
};
char D_0049B018[13] = {
#include "sub_main/0049B018.inc"
};
char D_0049B028[13] = {
#include "sub_main/0049B028.inc"
};
char D_0049B038[13] = {
#include "sub_main/0049B038.inc"
};
char D_0049B048[13] = {
#include "sub_main/0049B048.inc"
};
char D_0049B058[13] = {
#include "sub_main/0049B058.inc"
};
char D_0049B068[0xC] = {
#include "sub_main/0049B068.inc"
};
char D_0049B078[0xC] = {
#include "sub_main/0049B078.inc"
};
char D_0049B088[0xC] = {
#include "sub_main/0049B088.inc"
};
char D_0049B098[0xC] = {
#include "sub_main/0049B098.inc"
};
char D_0049B0A8[0xC] = {
#include "sub_main/0049B0A8.inc"
};
char D_0049B0B8[0xC] = {
#include "sub_main/0049B0B8.inc"
};
char D_0049B0C8[0x8] = {
#include "sub_main/0049B0C8.inc"
};
char D_0049B0D0[0x10] = {
#include "sub_main/0049B0D0.inc"
};
char D_0049B0E0[0x10] = {
#include "sub_main/0049B0E0.inc"
};
char D_0049B0F0[0x10] = {
#include "sub_main/0049B0F0.inc"
};
char D_0049B100[0x10] = {
#include "sub_main/0049B100.inc"
};
char D_0049B110[0x10] = {
#include "sub_main/0049B110.inc"
};
char D_0049B120[0x20] = {
#include "sub_main/0049B120.inc"
};
char D_0049B140[0x20] = {
#include "sub_main/0049B140.inc"
};
char D_0049B160[0x20] = {
#include "sub_main/0049B160.inc"
};
char D_0049B180[0x20] = {
#include "sub_main/0049B180.inc"
};
char D_0049B1A0[0x18] = {
#include "sub_main/0049B1A0.inc"
};
char D_0049B1B8[15] = {
#include "sub_main/0049B1B8.inc"
};
char D_0049B1D0[0x10] = {
#include "sub_main/0049B1D0.inc"
};
char D_0049B1E0[0x20] = {
#include "sub_main/0049B1E0.inc"
};
char D_0049B200[0x20] = {
#include "sub_main/0049B200.inc"
};
char D_0049B220[0x20] = {
#include "sub_main/0049B220.inc"
};
char D_0049B240[0x20] = {
#include "sub_main/0049B240.inc"
};
char D_0049B260[0x20] = {
#include "sub_main/0049B260.inc"
};
char D_0049B280[0x20] = {
#include "sub_main/0049B280.inc"
};
char D_0049B2A0[0x20] = {
#include "sub_main/0049B2A0.inc"
};
char D_0049B2C0[0x18] = {
#include "sub_main/0049B2C0.inc"
};
char D_0049B2D8[0x8] = {
#include "sub_main/0049B2D8.inc"
};
char D_0049B2E0[0x10] = {
#include "sub_main/0049B2E0.inc"
};
char D_0049B2F0[0x10] = {
#include "sub_main/0049B2F0.inc"
};
char D_0049B300[0x20] = {
#include "sub_main/0049B300.inc"
};
char D_0049B320[0x18] = {
#include "sub_main/0049B320.inc"
};
char D_0049B338[0xC] = {
#include "sub_main/0049B338.inc"
};
char D_0049B348[0xC] = {
#include "sub_main/0049B348.inc"
};
char D_0049B358[15] = {
#include "sub_main/0049B358.inc"
};
char D_0049B368[15] = {
#include "sub_main/0049B368.inc"
};
char D_0049B378[15] = {
#include "sub_main/0049B378.inc"
};
char D_0049B388[0x8] = {
#include "sub_main/0049B388.inc"
};
char D_0049B390[0x8] = {
#include "sub_main/0049B390.inc"
};
char D_0049B398[0x8] = {
#include "sub_main/0049B398.inc"
};
char D_0049B3A0[0x8] = {
#include "sub_main/0049B3A0.inc"
};
char D_0049B3A8[0x8] = {
#include "sub_main/0049B3A8.inc"
};
char D_0049B3B0[0x8] = {
#include "sub_main/0049B3B0.inc"
};
char D_0049B3B8[0xC] = {
#include "sub_main/0049B3B8.inc"
};
char D_0049B3C8[0xC] = {
#include "sub_main/0049B3C8.inc"
};
char D_0049B3D8[0x8] = {
#include "sub_main/0049B3D8.inc"
};
char D_0049B3E0[0x10] = {
#include "sub_main/0049B3E0.inc"
};
char D_0049B3F0[0x10] = {
#include "sub_main/0049B3F0.inc"
};
char D_0049B400[0x10] = {
#include "sub_main/0049B400.inc"
};
char D_0049B410[0x10] = {
#include "sub_main/0049B410.inc"
};
char D_0049B420[0x10] = {
#include "sub_main/0049B420.inc"
};
char D_0049B430[0x10] = {
#include "sub_main/0049B430.inc"
};
char D_0049B440[0x10] = {
#include "sub_main/0049B440.inc"
};
char D_0049B450[0x10] = {
#include "sub_main/0049B450.inc"
};
char D_0049B460[0x10] = {
#include "sub_main/0049B460.inc"
};
char D_0049B470[0x10] = {
#include "sub_main/0049B470.inc"
};
char D_0049B480[0x10] = {
#include "sub_main/0049B480.inc"
};
char D_0049B490[0x10] = {
#include "sub_main/0049B490.inc"
};
char D_0049B4A0[0x10] = {
#include "sub_main/0049B4A0.inc"
};
char D_0049B4B0[0x10] = {
#include "sub_main/0049B4B0.inc"
};
char D_0049B4C0[0x10] = {
#include "sub_main/0049B4C0.inc"
};
char D_0049B4D0[0x10] = {
#include "sub_main/0049B4D0.inc"
};
char D_0049B4E0[0x10] = {
#include "sub_main/0049B4E0.inc"
};
char D_0049B4F0[0x10] = {
#include "sub_main/0049B4F0.inc"
};
char D_0049B500[0x10] = {
#include "sub_main/0049B500.inc"
};
char D_0049B510[0x10] = {
#include "sub_main/0049B510.inc"
};
char D_0049B520[0x10] = {
#include "sub_main/0049B520.inc"
};
char D_0049B530[0x10] = {
#include "sub_main/0049B530.inc"
};
char D_0049B540[0x10] = {
#include "sub_main/0049B540.inc"
};
char D_0049B550[0x10] = {
#include "sub_main/0049B550.inc"
};
char D_0049B560[0x10] = {
#include "sub_main/0049B560.inc"
};
char D_0049B570[0x10] = {
#include "sub_main/0049B570.inc"
};
char D_0049B580[0x10] = {
#include "sub_main/0049B580.inc"
};
char D_0049B590[0x10] = {
#include "sub_main/0049B590.inc"
};
char D_0049B5A0[0x10] = {
#include "sub_main/0049B5A0.inc"
};
char D_0049B5B0[0x10] = {
#include "sub_main/0049B5B0.inc"
};
char D_0049B5C0[0x10] = {
#include "sub_main/0049B5C0.inc"
};
char D_0049B5D0[0x10] = {
#include "sub_main/0049B5D0.inc"
};
char D_0049B5E0[0x10] = {
#include "sub_main/0049B5E0.inc"
};
char D_0049B5F0[0x10] = {
#include "sub_main/0049B5F0.inc"
};
char D_0049B600[0x10] = {
#include "sub_main/0049B600.inc"
};
char D_0049B610[0x10] = {
#include "sub_main/0049B610.inc"
};
char D_0049B620[0x10] = {
#include "sub_main/0049B620.inc"
};
char D_0049B630[0x10] = {
#include "sub_main/0049B630.inc"
};
char D_0049B640[0x10] = {
#include "sub_main/0049B640.inc"
};
char D_0049B650[0x10] = {
#include "sub_main/0049B650.inc"
};
char D_0049B660[0x10] = {
#include "sub_main/0049B660.inc"
};
char D_0049B670[0x10] = {
#include "sub_main/0049B670.inc"
};
char D_0049B680[0x10] = {
#include "sub_main/0049B680.inc"
};
char D_0049B690[0x10] = {
#include "sub_main/0049B690.inc"
};
char D_0049B6A0[0x10] = {
#include "sub_main/0049B6A0.inc"
};
char D_0049B6B0[0x10] = {
#include "sub_main/0049B6B0.inc"
};
char D_0049B6C0[0x10] = {
#include "sub_main/0049B6C0.inc"
};
char D_0049B6D0[0x10] = {
#include "sub_main/0049B6D0.inc"
};
char D_0049B6E0[0x10] = {
#include "sub_main/0049B6E0.inc"
};
char D_0049B6F0[0x10] = {
#include "sub_main/0049B6F0.inc"
};
char D_0049B700[0x10] = {
#include "sub_main/0049B700.inc"
};
char D_0049B710[0x10] = {
#include "sub_main/0049B710.inc"
};
char D_0049B720[0x10] = {
#include "sub_main/0049B720.inc"
};
char D_0049B730[0x10] = {
#include "sub_main/0049B730.inc"
};
char D_0049B740[0x10] = {
#include "sub_main/0049B740.inc"
};
char D_0049B750[0x10] = {
#include "sub_main/0049B750.inc"
};
char D_0049B760[0x8] = {
#include "sub_main/0049B760.inc"
};
char D_0049B768[0x8] = {
#include "sub_main/0049B768.inc"
};
char D_0049B770[0x8] = {
#include "sub_main/0049B770.inc"
};
char D_0049B778[0x8] = {
#include "sub_main/0049B778.inc"
};
char D_0049B780[0x18] = {
#include "sub_main/0049B780.inc"
};
char D_0049B798[15] = {
#include "sub_main/0049B798.inc"
};
char D_0049B7A8[13] = {
#include "sub_main/0049B7A8.inc"
};
char D_0049B7B8[15] = {
#include "sub_main/0049B7B8.inc"
};
char D_0049B7C8[15] = {
#include "sub_main/0049B7C8.inc"
};
char D_0049B7D8[0x8] = {
#include "sub_main/0049B7D8.inc"
};
char D_0049B7E0[0x8] = {
#include "sub_main/0049B7E0.inc"
};
char D_0049B7E8[0x8] = {
#include "sub_main/0049B7E8.inc"
};
char D_0049B7F0[0x8] = {
#include "sub_main/0049B7F0.inc"
};
char D_0049B7F8[0xC] = {
#include "sub_main/0049B7F8.inc"
};
char D_0049B808[0x8] = {
#include "sub_main/0049B808.inc"
};
char D_0049B810[0x10] = {
#include "sub_main/0049B810.inc"
};
char D_0049B820[0x20] = {
#include "sub_main/0049B820.inc"
};
char D_0049B840[0x18] = {
#include "sub_main/0049B840.inc"
};
char D_0049B858[14] = {
#include "sub_main/0049B858.inc"
};
char D_0049B868[15] = {
#include "sub_main/0049B868.inc"
};
char D_0049B878[15] = {
#include "sub_main/0049B878.inc"
};
char D_0049B888[14] = {
#include "sub_main/0049B888.inc"
};
char D_0049B898[15] = {
#include "sub_main/0049B898.inc"
};
char D_0049B8A8[13] = {
#include "sub_main/0049B8A8.inc"
};
char D_0049B8B8[0x8] = {
#include "sub_main/0049B8B8.inc"
};
char D_0049B8C0[0x10] = {
#include "sub_main/0049B8C0.inc"
};
char D_0049B8D0[0x10] = {
#include "sub_main/0049B8D0.inc"
};
char D_0049B8E0[0x10] = {
#include "sub_main/0049B8E0.inc"
};
char D_0049B8F0[0x10] = {
#include "sub_main/0049B8F0.inc"
};
char D_0049B900[0x10] = {
#include "sub_main/0049B900.inc"
};
char D_0049B910[0x10] = {
#include "sub_main/0049B910.inc"
};
char D_0049B920[0x10] = {
#include "sub_main/0049B920.inc"
};
char D_0049B930[0x10] = {
#include "sub_main/0049B930.inc"
};
char D_0049B940[0x20] = {
#include "sub_main/0049B940.inc"
};
char D_0049B960[0x18] = {
#include "sub_main/0049B960.inc"
};
char D_0049B978[0xC] = {
#include "sub_main/0049B978.inc"
};
char D_0049B988[0x8] = {
#include "sub_main/0049B988.inc"
};
char D_0049B990[0x10] = {
#include "sub_main/0049B990.inc"
};
char D_0049B9A0[0x10] = {
#include "sub_main/0049B9A0.inc"
};
char D_0049B9B0[0x10] = {
#include "sub_main/0049B9B0.inc"
};
char D_0049B9C0[0x10] = {
#include "sub_main/0049B9C0.inc"
};
char D_0049B9D0[0x8] = {
#include "sub_main/0049B9D0.inc"
};
char D_0049B9D8[13] = {
#include "sub_main/0049B9D8.inc"
};
char D_0049B9E8[13] = {
#include "sub_main/0049B9E8.inc"
};
char D_0049BA00[0x20] = {
#include "sub_main/0049BA00.inc"
};
char D_0049BA20[0x20] = {
#include "sub_main/0049BA20.inc"
};
char D_0049BA40[0x20] = {
#include "sub_main/0049BA40.inc"
};
char D_0049BA60[0x18] = {
#include "sub_main/0049BA60.inc"
};
char D_0049BA78[0x8] = {
#include "sub_main/0049BA78.inc"
};
char D_0049BA80[0x10] = {
#include "sub_main/0049BA80.inc"
};
char D_0049BA90[0x10] = {
#include "sub_main/0049BA90.inc"
};
char D_0049BAA0[0x10] = {
#include "sub_main/0049BAA0.inc"
};
char D_0049BAB0[0x10] = {
#include "sub_main/0049BAB0.inc"
};
char D_0049BAC0[0x10] = {
#include "sub_main/0049BAC0.inc"
};
char D_0049BAD0[0x8] = {
#include "sub_main/0049BAD0.inc"
};
char D_0049BAD8[0x8] = {
#include "sub_main/0049BAD8.inc"
};
char D_0049BAE0[0x8] = {
#include "sub_main/0049BAE0.inc"
};
char D_0049BAE8[0x8] = {
#include "sub_main/0049BAE8.inc"
};
char D_0049BAF0[0x10] = {
#include "sub_main/0049BAF0.inc"
};
char D_0049BB00[0x10] = {
#include "sub_main/0049BB00.inc"
};
char D_0049BB10[0x8] = {
#include "sub_main/0049BB10.inc"
};
char D_0049BB18[0xC] = {
#include "sub_main/0049BB18.inc"
};
char D_0049BB28[0xC] = {
#include "sub_main/0049BB28.inc"
};
char D_0049BB38[0x8] = {
#include "sub_main/0049BB38.inc"
};
char D_0049BB40[0x8] = {
#include "sub_main/0049BB40.inc"
};
char D_0049BB48[0xC] = {
#include "sub_main/0049BB48.inc"
};
char D_0049BB58[0xC] = {
#include "sub_main/0049BB58.inc"
};
char D_0049BB68[0x8] = {
#include "sub_main/0049BB68.inc"
};
char D_0049BB70[0x10] = {
#include "sub_main/0049BB70.inc"
};
char D_0049BB80[0x8] = {
#include "sub_main/0049BB80.inc"
};
char D_0049BB88[0xC] = {
#include "sub_main/0049BB88.inc"
};
char D_0049BB98[0x8] = {
#include "sub_main/0049BB98.inc"
};
char D_0049BBA0[0x8] = {
#include "sub_main/0049BBA0.inc"
};
char D_0049BBA8[0xC] = {
#include "sub_main/0049BBA8.inc"
};
char D_0049BBB8[0xC] = {
#include "sub_main/0049BBB8.inc"
};
char D_0049BBC8[0x8] = {
#include "sub_main/0049BBC8.inc"
};
char D_0049BBD0[0x8] = {
#include "sub_main/0049BBD0.inc"
};
char D_0049BBD8[0xC] = {
#include "sub_main/0049BBD8.inc"
};
char D_0049BBE8[0xC] = {
#include "sub_main/0049BBE8.inc"
};
char D_0049BBF8[0xC] = {
#include "sub_main/0049BBF8.inc"
};
char D_0049BC08[0x8] = {
#include "sub_main/0049BC08.inc"
};
char D_0049BC10[0x8] = {
#include "sub_main/0049BC10.inc"
};
char D_0049BC18[0x8] = {
#include "sub_main/0049BC18.inc"
};
char D_0049BC20[0x8] = {
#include "sub_main/0049BC20.inc"
};
char D_0049BC28[0x8] = {
#include "sub_main/0049BC28.inc"
};
char D_0049BC30[0x8] = {
#include "sub_main/0049BC30.inc"
};
char D_0049BC38[0xC] = {
#include "sub_main/0049BC38.inc"
};
char D_0049BC48[0x8] = {
#include "sub_main/0049BC48.inc"
};
char D_0049BC50[0x8] = {
#include "sub_main/0049BC50.inc"
};
char D_0049BC58[0xC] = {
#include "sub_main/0049BC58.inc"
};
char D_0049BC68[0xC] = {
#include "sub_main/0049BC68.inc"
};
char D_0049BC78[0xC] = {
#include "sub_main/0049BC78.inc"
};
char D_0049BC88[0xC] = {
#include "sub_main/0049BC88.inc"
};
char D_0049BC98[0x8] = {
#include "sub_main/0049BC98.inc"
};
char D_0049BCA0[0x10] = {
#include "sub_main/0049BCA0.inc"
};
char D_0049BCB0[0x10] = {
#include "sub_main/0049BCB0.inc"
};
char D_0049BCC0[0x8] = {
#include "sub_main/0049BCC0.inc"
};
char D_0049BCC8[0x8] = {
#include "sub_main/0049BCC8.inc"
};
char D_0049BCD0[0x10] = {
#include "sub_main/0049BCD0.inc"
};
char D_0049BCE0[0x8] = {
#include "sub_main/0049BCE0.inc"
};
char D_0049BCE8[0xC] = {
#include "sub_main/0049BCE8.inc"
};
char D_0049BCF8[0xC] = {
#include "sub_main/0049BCF8.inc"
};
char D_0049BD08[0x8] = {
#include "sub_main/0049BD08.inc"
};
char D_0049BD10[0x8] = {
#include "sub_main/0049BD10.inc"
};
char D_0049BD18[0x8] = {
#include "sub_main/0049BD18.inc"
};
char D_0049BD20[0x8] = {
#include "sub_main/0049BD20.inc"
};
char D_0049BD28[0x8] = {
#include "sub_main/0049BD28.inc"
};
char D_0049BD30[0x8] = {
#include "sub_main/0049BD30.inc"
};
char D_0049BD38[0x8] = {
#include "sub_main/0049BD38.inc"
};
char D_0049BD40[0x10] = {
#include "sub_main/0049BD40.inc"
};
char D_0049BD50[0x10] = {
#include "sub_main/0049BD50.inc"
};
char D_0049BD60[0x10] = {
#include "sub_main/0049BD60.inc"
};
char D_0049BD70[0x8] = {
#include "sub_main/0049BD70.inc"
};
char D_0049BD78[0xC] = {
#include "sub_main/0049BD78.inc"
};
char D_0049BD88[0xC] = {
#include "sub_main/0049BD88.inc"
};
char D_0049BD98[0xC] = {
#include "sub_main/0049BD98.inc"
};
char D_0049BDA8[0xC] = {
#include "sub_main/0049BDA8.inc"
};
char D_0049BDB8[0x8] = {
#include "sub_main/0049BDB8.inc"
};
char D_0049BDC0[0x10] = {
#include "sub_main/0049BDC0.inc"
};
char D_0049BDD0[0x8] = {
#include "sub_main/0049BDD0.inc"
};
char D_0049BDD8[0xC] = {
#include "sub_main/0049BDD8.inc"
};
char D_0049BDE8[0x8] = {
#include "sub_main/0049BDE8.inc"
};
char D_0049BDF0[0x8] = {
#include "sub_main/0049BDF0.inc"
};
char D_0049BDF8[0x8] = {
#include "sub_main/0049BDF8.inc"
};
char D_0049BE00[0x8] = {
#include "sub_main/0049BE00.inc"
};
char D_0049BE08[0x8] = {
#include "sub_main/0049BE08.inc"
};
char D_0049BE10[0x8] = {
#include "sub_main/0049BE10.inc"
};
char D_0049BE18[0x8] = {
#include "sub_main/0049BE18.inc"
};
char D_0049BE20[0x10] = {
#include "sub_main/0049BE20.inc"
};
char D_0049BE30[0x40] = {
#include "sub_main/0049BE30.inc"
};
char D_0049BE70[0x20] = {
#include "sub_main/0049BE70.inc"
};
char D_0049BE90[0x30] = {
#include "sub_main/0049BE90.inc"
};
char D_0049BEC0[0x30] = {
#include "sub_main/0049BEC0.inc"
};
char D_0049BEF0[0x20] = {
#include "sub_main/0049BEF0.inc"
};
char D_0049BF10[0x20] = {
#include "sub_main/0049BF10.inc"
};
char D_0049BF30[0x20] = {
#include "sub_main/0049BF30.inc"
};
char D_0049BF50[0x20] = {
#include "sub_main/0049BF50.inc"
};
char D_0049BF70[0x20] = {
#include "sub_main/0049BF70.inc"
};
char D_0049BF90[0x30] = {
#include "sub_main/0049BF90.inc"
};
char D_0049BFC0[0x30] = {
#include "sub_main/0049BFC0.inc"
};
char D_0049BFF0[0x30] = {
#include "sub_main/0049BFF0.inc"
};
char D_0049C020[0x30] = {
#include "sub_main/0049C020.inc"
};
char D_0049C050[0x30] = {
#include "sub_main/0049C050.inc"
};
char D_0049C080[0x30] = {
#include "sub_main/0049C080.inc"
};
char D_0049C0B0[0x20] = {
#include "sub_main/0049C0B0.inc"
};
char D_0049C0D0[0x20] = {
#include "sub_main/0049C0D0.inc"
};
char D_0049C0F0[0x20] = {
#include "sub_main/0049C0F0.inc"
};
char D_0049C110[0x30] = {
#include "sub_main/0049C110.inc"
};
char D_0049C140[0x30] = {
#include "sub_main/0049C140.inc"
};
char D_0049C170[0x20] = {
#include "sub_main/0049C170.inc"
};
char D_0049C190[0x30] = {
#include "sub_main/0049C190.inc"
};
char D_0049C1C0[0x20] = {
#include "sub_main/0049C1C0.inc"
};
char D_0049C1E0[0x40] = {
#include "sub_main/0049C1E0.inc"
};
char D_0049C220[0x40] = {
#include "sub_main/0049C220.inc"
};
char D_0049C260[0x30] = {
#include "sub_main/0049C260.inc"
};
char D_0049C290[0x40] = {
#include "sub_main/0049C290.inc"
};
char D_0049C2D0[0x30] = {
#include "sub_main/0049C2D0.inc"
};
char D_0049C300[0x30] = {
#include "sub_main/0049C300.inc"
};
char D_0049C330[0x30] = {
#include "sub_main/0049C330.inc"
};
char D_0049C360[0x40] = {
#include "sub_main/0049C360.inc"
};
char D_0049C3A0[0x40] = {
#include "sub_main/0049C3A0.inc"
};
char D_0049C3E0[0x40] = {
#include "sub_main/0049C3E0.inc"
};
char D_0049C420[0x40] = {
#include "sub_main/0049C420.inc"
};
char D_0049C460[0x30] = {
#include "sub_main/0049C460.inc"
};
char D_0049C490[0x30] = {
#include "sub_main/0049C490.inc"
};
char D_0049C4C0[0x30] = {
#include "sub_main/0049C4C0.inc"
};
char D_0049C4F0[0x30] = {
#include "sub_main/0049C4F0.inc"
};
char D_0049C520[0x20] = {
#include "sub_main/0049C520.inc"
};
char D_0049C540[0x30] = {
#include "sub_main/0049C540.inc"
};
char D_0049C570[0x70] = {
#include "sub_main/0049C570.inc"
};
char D_0049C5E0[0x20] = {
#include "sub_main/0049C5E0.inc"
};
char D_0049C600[0x70] = {
#include "sub_main/0049C600.inc"
};
char D_0049C670[0x20] = {
#include "sub_main/0049C670.inc"
};
char D_0049C690[0x10] = {
#include "sub_main/0049C690.inc"
};
char D_0049C6A0[0x30] = {
#include "sub_main/0049C6A0.inc"
};
char D_0049C6D0[0x40] = {
#include "sub_main/0049C6D0.inc"
};
char D_0049C710[0x30] = {
#include "sub_main/0049C710.inc"
};
char D_0049C740[0x20] = {
#include "sub_main/0049C740.inc"
};
char D_0049C760[0x40] = {
#include "sub_main/0049C760.inc"
};
char D_0049C7A0[0x30] = {
#include "sub_main/0049C7A0.inc"
};
char D_0049C7D0[0x20] = {
#include "sub_main/0049C7D0.inc"
};
char D_0049C7F0[0x40] = {
#include "sub_main/0049C7F0.inc"
};
char D_0049C830[0x30] = {
#include "sub_main/0049C830.inc"
};
char D_0049C860[0x30] = {
#include "sub_main/0049C860.inc"
};
char D_0049C890[0x30] = {
#include "sub_main/0049C890.inc"
};
char D_0049C8C0[0x30] = {
#include "sub_main/0049C8C0.inc"
};
char D_0049C8F0[0x50] = {
#include "sub_main/0049C8F0.inc"
};
char D_0049C940[0x30] = {
#include "sub_main/0049C940.inc"
};
char D_0049C970[0x30] = {
#include "sub_main/0049C970.inc"
};
char D_0049C9A0[0x40] = {
#include "sub_main/0049C9A0.inc"
};
char D_0049C9E0[0x30] = {
#include "sub_main/0049C9E0.inc"
};
char D_0049CA10[0x60] = {
#include "sub_main/0049CA10.inc"
};
char D_0049CA70[0x18] = {
#include "sub_main/0049CA70.inc"
};
char D_0049CA88[15] = {
#include "sub_main/0049CA88.inc"
};
char D_0049CA98[0xC] = {
#include "sub_main/0049CA98.inc"
};
char D_0049CAA8[0xC] = {
#include "sub_main/0049CAA8.inc"
};
char D_0049CAB8[0xC] = {
#include "sub_main/0049CAB8.inc"
};
char D_0049CAC8[0xC] = {
#include "sub_main/0049CAC8.inc"
};
char D_0049CAD8[0xC] = {
#include "sub_main/0049CAD8.inc"
};
char D_0049CAE8[0xC] = {
#include "sub_main/0049CAE8.inc"
};
char D_0049CAF8[0xC] = {
#include "sub_main/0049CAF8.inc"
};
char D_0049CB08[0xC] = {
#include "sub_main/0049CB08.inc"
};
char D_0049CB18[0xC] = {
#include "sub_main/0049CB18.inc"
};
char D_0049CB28[0xC] = {
#include "sub_main/0049CB28.inc"
};
char D_0049CB38[0xC] = {
#include "sub_main/0049CB38.inc"
};
char D_0049CB48[0xC] = {
#include "sub_main/0049CB48.inc"
};
char D_0049CB58[0x8] = {
#include "sub_main/0049CB58.inc"
};
char D_0049CB60[0x8] = {
#include "sub_main/0049CB60.inc"
};
char D_0049CB68[0x8] = {
#include "sub_main/0049CB68.inc"
};
char D_0049CB70[0x8] = {
#include "sub_main/0049CB70.inc"
};
char D_0049CB78[0x8] = {
#include "sub_main/0049CB78.inc"
};
char D_0049CB80[0x8] = {
#include "sub_main/0049CB80.inc"
};
char D_0049CB88[0x8] = {
#include "sub_main/0049CB88.inc"
};
char D_0049CB90[0x18] = {
#include "sub_main/0049CB90.inc"
};
char D_0049CBA8[0xC] = {
#include "sub_main/0049CBA8.inc"
};
char D_0049CBB8[0xC] = {
#include "sub_main/0049CBB8.inc"
};
char D_0049CBD0[0x20] = {
#include "sub_main/0049CBD0.inc"
};
char D_0049CBF0[0x18] = {
#include "sub_main/0049CBF0.inc"
};
char D_0049CC08[0xC] = {
#include "sub_main/0049CC08.inc"
};
char D_0049CC20[0x18] = {
#include "sub_main/0049CC20.inc"
};
char D_0049CC38[0x8] = {
#include "sub_main/0049CC38.inc"
};
char D_0049CC40[0x10] = {
#include "sub_main/0049CC40.inc"
};
char D_0049CC50[0x8] = {
#include "sub_main/0049CC50.inc"
};
char D_0049CC58[0xC] = {
#include "sub_main/0049CC58.inc"
};
char D_0049CC70[0x18] = {
#include "sub_main/0049CC70.inc"
};
char D_0049CC88[0xC] = {
#include "sub_main/0049CC88.inc"
};
char D_0049CCA0[0x20] = {
#include "sub_main/0049CCA0.inc"
};
char D_0049CCC0[0x20] = {
#include "sub_main/0049CCC0.inc"
};
char D_0049CCE0[0x20] = {
#include "sub_main/0049CCE0.inc"
};
char D_0049CD00[0x18] = {
#include "sub_main/0049CD00.inc"
};
char D_0049CD18[15] = {
#include "sub_main/0049CD18.inc"
};
char D_0049CD30[0x20] = {
#include "sub_main/0049CD30.inc"
};
char D_0049CD50[0x18] = {
#include "sub_main/0049CD50.inc"
};
char D_0049CD68[15] = {
#include "sub_main/0049CD68.inc"
};
char D_0049CD80[0x18] = {
#include "sub_main/0049CD80.inc"
};
char D_0049CD98[15] = {
#include "sub_main/0049CD98.inc"
};
char D_0049CDB0[0x20] = {
#include "sub_main/0049CDB0.inc"
};
char D_0049CDD0[0x20] = {
#include "sub_main/0049CDD0.inc"
};
char D_0049CDF0[0x20] = {
#include "sub_main/0049CDF0.inc"
};
char D_0049CE10[0x18] = {
#include "sub_main/0049CE10.inc"
};
char D_0049CE28[15] = {
#include "sub_main/0049CE28.inc"
};
char D_0049CE38[15] = {
#include "sub_main/0049CE38.inc"
};
char D_0049CE50[0x18] = {
#include "sub_main/0049CE50.inc"
};
char D_0049CE68[15] = {
#include "sub_main/0049CE68.inc"
};
char D_0049CE78[15] = {
#include "sub_main/0049CE78.inc"
};
char D_0049CE90[0x20] = {
#include "sub_main/0049CE90.inc"
};
char D_0049CEB0[0x20] = {
#include "sub_main/0049CEB0.inc"
};
char D_0049CED0[0x20] = {
#include "sub_main/0049CED0.inc"
};
char D_0049CEF0[0x18] = {
#include "sub_main/0049CEF0.inc"
};
char D_0049CF08[13] = {
#include "sub_main/0049CF08.inc"
};
char D_0049CF18[13] = {
#include "sub_main/0049CF18.inc"
};
char D_0049CF28[0xC] = {
#include "sub_main/0049CF28.inc"
};
char D_0049CF38[0xC] = {
#include "sub_main/0049CF38.inc"
};
char D_0049CF48[0xC] = {
#include "sub_main/0049CF48.inc"
};
char D_0049CF58[0xC] = {
#include "sub_main/0049CF58.inc"
};
char D_0049CF68[0xC] = {
#include "sub_main/0049CF68.inc"
};
char D_0049CF78[0xC] = {
#include "sub_main/0049CF78.inc"
};
char D_0049CF88[0xC] = {
#include "sub_main/0049CF88.inc"
};
char D_0049CF98[0xC] = {
#include "sub_main/0049CF98.inc"
};
char D_0049CFA8[0xC] = {
#include "sub_main/0049CFA8.inc"
};
char D_0049CFB8[0xC] = {
#include "sub_main/0049CFB8.inc"
};
char D_0049CFC8[0xC] = {
#include "sub_main/0049CFC8.inc"
};
char D_0049CFD8[0xC] = {
#include "sub_main/0049CFD8.inc"
};
char D_0049CFE8[0xC] = {
#include "sub_main/0049CFE8.inc"
};
char D_0049CFF8[0xC] = {
#include "sub_main/0049CFF8.inc"
};
char D_0049D008[0xC] = {
#include "sub_main/0049D008.inc"
};
char D_0049D018[0xC] = {
#include "sub_main/0049D018.inc"
};
char D_0049D028[0xC] = {
#include "sub_main/0049D028.inc"
};
char D_0049D038[0xC] = {
#include "sub_main/0049D038.inc"
};
char D_0049D048[0xC] = {
#include "sub_main/0049D048.inc"
};
char D_0049D058[0xC] = {
#include "sub_main/0049D058.inc"
};
char D_0049D068[0xC] = {
#include "sub_main/0049D068.inc"
};
char D_0049D078[0xC] = {
#include "sub_main/0049D078.inc"
};
char D_0049D088[0xC] = {
#include "sub_main/0049D088.inc"
};
char D_0049D098[0xC] = {
#include "sub_main/0049D098.inc"
};
char D_0049D0A8[0xC] = {
#include "sub_main/0049D0A8.inc"
};
char D_0049D0B8[0xC] = {
#include "sub_main/0049D0B8.inc"
};
char D_0049D0C8[0xC] = {
#include "sub_main/0049D0C8.inc"
};
char D_0049D0D8[0xC] = {
#include "sub_main/0049D0D8.inc"
};
char D_0049D0E8[0xC] = {
#include "sub_main/0049D0E8.inc"
};
char D_0049D0F8[0xC] = {
#include "sub_main/0049D0F8.inc"
};
char D_0049D108[0xC] = {
#include "sub_main/0049D108.inc"
};
char D_0049D118[0xC] = {
#include "sub_main/0049D118.inc"
};
char D_0049D128[0xC] = {
#include "sub_main/0049D128.inc"
};
char D_0049D138[13] = {
#include "sub_main/0049D138.inc"
};
char D_0049D148[13] = {
#include "sub_main/0049D148.inc"
};
char D_0049D158[15] = {
#include "sub_main/0049D158.inc"
};
char D_0049D170[0x18] = {
#include "sub_main/0049D170.inc"
};
char D_0049D188[15] = {
#include "sub_main/0049D188.inc"
};
char D_0049D1A0[0x18] = {
#include "sub_main/0049D1A0.inc"
};
char D_0049D1B8[0xC] = {
#include "sub_main/0049D1B8.inc"
};
char D_0049D1C8[0x8] = {
#include "sub_main/0049D1C8.inc"
};
char D_0049D1D0[0x10] = {
#include "sub_main/0049D1D0.inc"
};
char D_0049D1E0[0x18] = {
#include "sub_main/0049D1E0.inc"
};
char D_0049D1F8[13] = {
#include "sub_main/0049D1F8.inc"
};
char D_0049D208[0xC] = {
#include "sub_main/0049D208.inc"
};
char D_0049D218[13] = {
#include "sub_main/0049D218.inc"
};
char D_0049D228[15] = {
#include "sub_main/0049D228.inc"
};
char D_0049D238[0xC] = {
#include "sub_main/0049D238.inc"
};
char D_0049D250[0x20] = {
#include "sub_main/0049D250.inc"
};
char D_0049D270[0x18] = {
#include "sub_main/0049D270.inc"
};
char D_0049D288[0xC] = {
#include "sub_main/0049D288.inc"
};
char D_0049D298[0xC] = {
#include "sub_main/0049D298.inc"
};
char D_0049D2B0[0x18] = {
#include "sub_main/0049D2B0.inc"
};
char D_0049D2C8[13] = {
#include "sub_main/0049D2C8.inc"
};
char D_0049D2E0[0x20] = {
#include "sub_main/0049D2E0.inc"
};
char D_0049D300[0x18] = {
#include "sub_main/0049D300.inc"
};
char D_0049D318[0xC] = {
#include "sub_main/0049D318.inc"
};
char D_0049D328[15] = {
#include "sub_main/0049D328.inc"
};
char D_0049D340[0x20] = {
#include "sub_main/0049D340.inc"
};
char D_0049D360[0x18] = {
#include "sub_main/0049D360.inc"
};
char D_0049D378[13] = {
#include "sub_main/0049D378.inc"
};
char D_0049D388[0x8] = {
#include "sub_main/0049D388.inc"
};
char D_0049D390[0x10] = {
#include "sub_main/0049D390.inc"
};
char D_0049D3A0[0x10] = {
#include "sub_main/0049D3A0.inc"
};
char D_0049D3B0[0x10] = {
#include "sub_main/0049D3B0.inc"
};
char D_0049D3C0[0x10] = {
#include "sub_main/0049D3C0.inc"
};
char D_0049D3D0[0x10] = {
#include "sub_main/0049D3D0.inc"
};
char D_0049D3E0[0x10] = {
#include "sub_main/0049D3E0.inc"
};
char D_0049D3F0[0x20] = {
#include "sub_main/0049D3F0.inc"
};
char D_0049D410[0x18] = {
#include "sub_main/0049D410.inc"
};
char D_0049D428[0xC] = {
#include "sub_main/0049D428.inc"
};
char D_0049D438[0xC] = {
#include "sub_main/0049D438.inc"
};
char D_0049D448[0xC] = {
#include "sub_main/0049D448.inc"
};
char D_0049D460[0x20] = {
#include "sub_main/0049D460.inc"
};
char D_0049D480[0x10] = {
#include "sub_main/0049D480.inc"
};
char D_0049D490[0x10] = {
#include "sub_main/0049D490.inc"
};
char D_0049D4A0[0x18] = {
#include "sub_main/0049D4A0.inc"
};
char D_0049D4B8[15] = {
#include "sub_main/0049D4B8.inc"
};
char D_0049D4C8[0xC] = {
#include "sub_main/0049D4C8.inc"
};
char D_0049D4E0[0x20] = {
#include "sub_main/0049D4E0.inc"
};
char D_0049D500[0x20] = {
#include "sub_main/0049D500.inc"
};
char D_0049D520[0x20] = {
#include "sub_main/0049D520.inc"
};
char D_0049D540[0x20] = {
#include "sub_main/0049D540.inc"
};
char D_0049D560[0x28] = {
#include "sub_main/0049D560.inc"
};
char D_0049D588[0xC] = {
#include "sub_main/0049D588.inc"
};
char D_0049D5A0[0x20] = {
#include "sub_main/0049D5A0.inc"
};
char D_0049D5C0[0x20] = {
#include "sub_main/0049D5C0.inc"
};
char D_0049D5E0[0x18] = {
#include "sub_main/0049D5E0.inc"
};
char D_0049D5F8[0xC] = {
#include "sub_main/0049D5F8.inc"
};
char D_0049D610[0x20] = {
#include "sub_main/0049D610.inc"
};
char D_0049D630[0x20] = {
#include "sub_main/0049D630.inc"
};
char D_0049D650[0x20] = {
#include "sub_main/0049D650.inc"
};
char D_0049D670[0x20] = {
#include "sub_main/0049D670.inc"
};
char D_0049D690[0x20] = {
#include "sub_main/0049D690.inc"
};
char D_0049D6B0[0x20] = {
#include "sub_main/0049D6B0.inc"
};
char D_0049D6D0[0x20] = {
#include "sub_main/0049D6D0.inc"
};
char D_0049D6F0[0x18] = {
#include "sub_main/0049D6F0.inc"
};
char D_0049D708[13] = {
#include "sub_main/0049D708.inc"
};
char D_0049D718[15] = {
#include "sub_main/0049D718.inc"
};
char D_0049D730[0x20] = {
#include "sub_main/0049D730.inc"
};
char D_0049D750[0x18] = {
#include "sub_main/0049D750.inc"
};
char D_0049D768[0xC] = {
#include "sub_main/0049D768.inc"
};
char D_0049D778[13] = {
#include "sub_main/0049D778.inc"
};
char D_0049D788[15] = {
#include "sub_main/0049D788.inc"
};
char D_0049D7A0[0x18] = {
#include "sub_main/0049D7A0.inc"
};
char D_0049D7B8[13] = {
#include "sub_main/0049D7B8.inc"
};
char D_0049D7D0[0x18] = {
#include "sub_main/0049D7D0.inc"
};
char D_0049D7E8[0xC] = {
#include "sub_main/0049D7E8.inc"
};
char D_0049D7F8[15] = {
#include "sub_main/0049D7F8.inc"
};
char D_0049D810[0x18] = {
#include "sub_main/0049D810.inc"
};
char D_0049D828[13] = {
#include "sub_main/0049D828.inc"
};
char D_0049D838[15] = {
#include "sub_main/0049D838.inc"
};
char D_0049D848[0xC] = {
#include "sub_main/0049D848.inc"
};
char D_0049D860[0x20] = {
#include "sub_main/0049D860.inc"
};
char D_0049D880[0x18] = {
#include "sub_main/0049D880.inc"
};
char D_0049D898[0xC] = {
#include "sub_main/0049D898.inc"
};
char D_0049D8A8[0xC] = {
#include "sub_main/0049D8A8.inc"
};
char D_0049D8B8[0xC] = {
#include "sub_main/0049D8B8.inc"
};
char D_0049D8D0[0x18] = {
#include "sub_main/0049D8D0.inc"
};
char D_0049D8E8[0xC] = {
#include "sub_main/0049D8E8.inc"
};
char D_0049D8F8[13] = {
#include "sub_main/0049D8F8.inc"
};
char D_0049D908[0xC] = {
#include "sub_main/0049D908.inc"
};
char D_0049D918[13] = {
#include "sub_main/0049D918.inc"
};
char D_0049D928[0xC] = {
#include "sub_main/0049D928.inc"
};
char D_0049D938[0x8] = {
#include "sub_main/0049D938.inc"
};
char D_0049D940[0x10] = {
#include "sub_main/0049D940.inc"
};
char D_0049D950[0x10] = {
#include "sub_main/0049D950.inc"
};
char D_0049D960[0x10] = {
#include "sub_main/0049D960.inc"
};
char D_0049D970[0x10] = {
#include "sub_main/0049D970.inc"
};
char D_0049D980[0x10] = {
#include "sub_main/0049D980.inc"
};
char D_0049D990[0x10] = {
#include "sub_main/0049D990.inc"
};
char D_0049D9A0[0x10] = {
#include "sub_main/0049D9A0.inc"
};
char D_0049D9B0[0x10] = {
#include "sub_main/0049D9B0.inc"
};
char D_0049D9C0[0x10] = {
#include "sub_main/0049D9C0.inc"
};
char D_0049D9D0[0x10] = {
#include "sub_main/0049D9D0.inc"
};
char D_0049D9E0[0x10] = {
#include "sub_main/0049D9E0.inc"
};
char D_0049D9F0[0x10] = {
#include "sub_main/0049D9F0.inc"
};
char D_0049DA00[0x20] = {
#include "sub_main/0049DA00.inc"
};
char D_0049DA20[0x20] = {
#include "sub_main/0049DA20.inc"
};
char D_0049DA40[0x20] = {
#include "sub_main/0049DA40.inc"
};
char D_0049DA60[0x20] = {
#include "sub_main/0049DA60.inc"
};
char D_0049DA80[0x20] = {
#include "sub_main/0049DA80.inc"
};
char D_0049DAA0[0x20] = {
#include "sub_main/0049DAA0.inc"
};
char D_0049DAC0[0x20] = {
#include "sub_main/0049DAC0.inc"
};
char D_0049DAE0[0x20] = {
#include "sub_main/0049DAE0.inc"
};
char D_0049DB00[0x20] = {
#include "sub_main/0049DB00.inc"
};
char D_0049DB20[0x10] = {
#include "sub_main/0049DB20.inc"
};
char D_0049DB30[0x10] = {
#include "sub_main/0049DB30.inc"
};
char D_0049DB40[0x10] = {
#include "sub_main/0049DB40.inc"
};
char D_0049DB50[0x10] = {
#include "sub_main/0049DB50.inc"
};
char D_0049DB60[0x20] = {
#include "sub_main/0049DB60.inc"
};
char D_0049DB80[0x30] = {
#include "sub_main/0049DB80.inc"
};
char D_0049DBB0[0x40] = {
#include "sub_main/0049DBB0.inc"
};
char D_0049DBF0[0x30] = {
#include "sub_main/0049DBF0.inc"
};
char D_0049DC20[0x20] = {
#include "sub_main/0049DC20.inc"
};
char D_0049DC40[0x20] = {
#include "sub_main/0049DC40.inc"
};
char D_0049DC60[0x20] = {
#include "sub_main/0049DC60.inc"
};
char D_0049DC80[0x10] = {
#include "sub_main/0049DC80.inc"
};
char D_0049DC90[0x10] = {
#include "sub_main/0049DC90.inc"
};
char D_0049DCA0[0x10] = {
#include "sub_main/0049DCA0.inc"
};
char D_0049DCB0[0x10] = {
#include "sub_main/0049DCB0.inc"
};
char D_0049DCC0[0x10] = {
#include "sub_main/0049DCC0.inc"
};
char D_0049DCD0[0x8] = {
#include "sub_main/0049DCD0.inc"
};
char D_0049DCD8[0x8] = {
#include "sub_main/0049DCD8.inc"
};
char D_0049DCE0[0x8] = {
#include "sub_main/0049DCE0.inc"
};
char D_0049DCE8[0x8] = {
#include "sub_main/0049DCE8.inc"
};
char D_0049DCF0[0x8] = {
#include "sub_main/0049DCF0.inc"
};
char D_0049DCF8[0xC] = {
#include "sub_main/0049DCF8.inc"
};
char D_0049DD08[0xC] = {
#include "sub_main/0049DD08.inc"
};
char D_0049DD18[0x8] = {
#include "sub_main/0049DD18.inc"
};
char D_0049DD20[0x10] = {
#include "sub_main/0049DD20.inc"
};
char D_0049DD30[0x8] = {
#include "sub_main/0049DD30.inc"
};
char D_0049DD38[0xC] = {
#include "sub_main/0049DD38.inc"
};
char D_0049DD48[0xC] = {
#include "sub_main/0049DD48.inc"
};
char D_0049DD58[0x8] = {
#include "sub_main/0049DD58.inc"
};
char D_0049DD60[0x10] = {
#include "sub_main/0049DD60.inc"
};
char D_0049DD70[0x20] = {
#include "sub_main/0049DD70.inc"
};
char D_0049DD90[0x20] = {
#include "sub_main/0049DD90.inc"
};
char D_0049DDB0[0x20] = {
#include "sub_main/0049DDB0.inc"
};
char D_0049DDD0[0x20] = {
#include "sub_main/0049DDD0.inc"
};
char D_0049DDF0[0x20] = {
#include "sub_main/0049DDF0.inc"
};
char D_0049DE10[0x20] = {
#include "sub_main/0049DE10.inc"
};
char D_0049DE30[0x20] = {
#include "sub_main/0049DE30.inc"
};
char D_0049DE50[0x18] = {
#include "sub_main/0049DE50.inc"
};
char D_0049DE68[15] = {
#include "sub_main/0049DE68.inc"
};
char D_0049DE78[15] = {
#include "sub_main/0049DE78.inc"
};
char D_0049DE88[15] = {
#include "sub_main/0049DE88.inc"
};
char D_0049DE98[0xC] = {
#include "sub_main/0049DE98.inc"
};
char D_0049DEA8[0xC] = {
#include "sub_main/0049DEA8.inc"
};
char D_0049DEB8[15] = {
#include "sub_main/0049DEB8.inc"
};
char D_0049DED0[0x20] = {
#include "sub_main/0049DED0.inc"
};
char D_0049DEF0[0x18] = {
#include "sub_main/0049DEF0.inc"
};
char D_0049DF08[15] = {
#include "sub_main/0049DF08.inc"
};
char D_0049DF18[15] = {
#include "sub_main/0049DF18.inc"
};
char D_0049DF30[0x20] = {
#include "sub_main/0049DF30.inc"
};
char D_0049DF50[0x20] = {
#include "sub_main/0049DF50.inc"
};
char D_0049DF70[0x20] = {
#include "sub_main/0049DF70.inc"
};
char D_0049DF90[0x20] = {
#include "sub_main/0049DF90.inc"
};
char D_0049DFB0[0x20] = {
#include "sub_main/0049DFB0.inc"
};
char D_0049DFD0[0x20] = {
#include "sub_main/0049DFD0.inc"
};
char D_0049DFF0[0x20] = {
#include "sub_main/0049DFF0.inc"
};
char D_0049E010[0x20] = {
#include "sub_main/0049E010.inc"
};
char D_0049E030[0x20] = {
#include "sub_main/0049E030.inc"
};
char D_0049E050[0x20] = {
#include "sub_main/0049E050.inc"
};
char D_0049E070[0x20] = {
#include "sub_main/0049E070.inc"
};
char D_0049E090[0x20] = {
#include "sub_main/0049E090.inc"
};
char D_0049E0B0[0x20] = {
#include "sub_main/0049E0B0.inc"
};
char D_0049E0D0[0x20] = {
#include "sub_main/0049E0D0.inc"
};
char D_0049E0F0[0x18] = {
#include "sub_main/0049E0F0.inc"
};
char D_0049E108[0xC] = {
#include "sub_main/0049E108.inc"
};
char D_0049E120[0x20] = {
#include "sub_main/0049E120.inc"
};
char D_0049E140[0x20] = {
#include "sub_main/0049E140.inc"
};
char D_0049E160[0x30] = {
#include "sub_main/0049E160.inc"
};
char D_0049E190[0x30] = {
#include "sub_main/0049E190.inc"
};
char D_0049E1C0[0x30] = {
#include "sub_main/0049E1C0.inc"
};
char D_0049E1F0[0x20] = {
#include "sub_main/0049E1F0.inc"
};
char D_0049E210[0x30] = {
#include "sub_main/0049E210.inc"
};
char D_0049E240[0x30] = {
#include "sub_main/0049E240.inc"
};
char D_0049E270[0x30] = {
#include "sub_main/0049E270.inc"
};
char D_0049E2A0[0x30] = {
#include "sub_main/0049E2A0.inc"
};
char D_0049E2D0[0x50] = {
#include "sub_main/0049E2D0.inc"
};
char D_0049E320[0x30] = {
#include "sub_main/0049E320.inc"
};
char D_0049E350[0x30] = {
#include "sub_main/0049E350.inc"
};
char D_0049E380[0x50] = {
#include "sub_main/0049E380.inc"
};
char D_0049E3D0[0x30] = {
#include "sub_main/0049E3D0.inc"
};
char D_0049E400[0x30] = {
#include "sub_main/0049E400.inc"
};
char D_0049E430[0x70] = {
#include "sub_main/0049E430.inc"
};
char D_0049E4A0[0x30] = {
#include "sub_main/0049E4A0.inc"
};
char D_0049E4D0[0x30] = {
#include "sub_main/0049E4D0.inc"
};
char D_0049E500[0x30] = {
#include "sub_main/0049E500.inc"
};
char D_0049E530[0x50] = {
#include "sub_main/0049E530.inc"
};
char D_0049E580[0x30] = {
#include "sub_main/0049E580.inc"
};
char D_0049E5B0[0x30] = {
#include "sub_main/0049E5B0.inc"
};
char D_0049E5E0[0x30] = {
#include "sub_main/0049E5E0.inc"
};
char D_0049E610[0x30] = {
#include "sub_main/0049E610.inc"
};
char D_0049E640[0x50] = {
#include "sub_main/0049E640.inc"
};
char D_0049E690[0x30] = {
#include "sub_main/0049E690.inc"
};
char D_0049E6C0[0x30] = {
#include "sub_main/0049E6C0.inc"
};
char D_0049E6F0[0x30] = {
#include "sub_main/0049E6F0.inc"
};
char D_0049E720[0x30] = {
#include "sub_main/0049E720.inc"
};
char D_0049E750[0x50] = {
#include "sub_main/0049E750.inc"
};
char D_0049E7A0[0x50] = {
#include "sub_main/0049E7A0.inc"
};
char D_0049E7F0[0x40] = {
#include "sub_main/0049E7F0.inc"
};
char D_0049E830[0x40] = {
#include "sub_main/0049E830.inc"
};
char D_0049E870[0x40] = {
#include "sub_main/0049E870.inc"
};
char D_0049E8B0[0x40] = {
#include "sub_main/0049E8B0.inc"
};
char D_0049E8F0[0x20] = {
#include "sub_main/0049E8F0.inc"
};
char D_0049E910[0x30] = {
#include "sub_main/0049E910.inc"
};
char D_0049E940[0x30] = {
#include "sub_main/0049E940.inc"
};
char D_0049E970[0x40] = {
#include "sub_main/0049E970.inc"
};
char D_0049E9B0[0x40] = {
#include "sub_main/0049E9B0.inc"
};
char D_0049E9F0[0x30] = {
#include "sub_main/0049E9F0.inc"
};
char D_0049EA20[0x20] = {
#include "sub_main/0049EA20.inc"
};
char D_0049EA40[0x20] = {
#include "sub_main/0049EA40.inc"
};
char D_0049EA60[0x20] = {
#include "sub_main/0049EA60.inc"
};
char D_0049EA80[0x20] = {
#include "sub_main/0049EA80.inc"
};
char D_0049EAA0[0x20] = {
#include "sub_main/0049EAA0.inc"
};
char D_0049EAC0[0x20] = {
#include "sub_main/0049EAC0.inc"
};
char D_0049EAE0[0x20] = {
#include "sub_main/0049EAE0.inc"
};
char D_0049EB00[0x20] = {
#include "sub_main/0049EB00.inc"
};
char D_0049EB20[0x20] = {
#include "sub_main/0049EB20.inc"
};
char D_0049EB40[0x20] = {
#include "sub_main/0049EB40.inc"
};
char D_0049EB60[0x20] = {
#include "sub_main/0049EB60.inc"
};
char D_0049EB80[0x20] = {
#include "sub_main/0049EB80.inc"
};
char D_0049EBA0[0x20] = {
#include "sub_main/0049EBA0.inc"
};
char D_0049EBC0[0x20] = {
#include "sub_main/0049EBC0.inc"
};
char D_0049EBE0[0x20] = {
#include "sub_main/0049EBE0.inc"
};
char D_0049EC00[0x20] = {
#include "sub_main/0049EC00.inc"
};
char D_0049EC20[0x20] = {
#include "sub_main/0049EC20.inc"
};
char D_0049EC40[0x20] = {
#include "sub_main/0049EC40.inc"
};
char D_0049EC60[0x20] = {
#include "sub_main/0049EC60.inc"
};
char D_0049EC80[0x20] = {
#include "sub_main/0049EC80.inc"
};
char D_0049ECA0[0x20] = {
#include "sub_main/0049ECA0.inc"
};
char D_0049ECC0[0x20] = {
#include "sub_main/0049ECC0.inc"
};
char D_0049ECE0[0x20] = {
#include "sub_main/0049ECE0.inc"
};
char D_0049ED00[0x20] = {
#include "sub_main/0049ED00.inc"
};
char D_0049ED20[0x20] = {
#include "sub_main/0049ED20.inc"
};
char D_0049ED40[0x20] = {
#include "sub_main/0049ED40.inc"
};
char D_0049ED60[0x20] = {
#include "sub_main/0049ED60.inc"
};
char D_0049ED80[0x20] = {
#include "sub_main/0049ED80.inc"
};
char D_0049EDA0[0x20] = {
#include "sub_main/0049EDA0.inc"
};
char D_0049EDC0[0x20] = {
#include "sub_main/0049EDC0.inc"
};
char D_0049EDE0[0x20] = {
#include "sub_main/0049EDE0.inc"
};
char D_0049EE00[0x20] = {
#include "sub_main/0049EE00.inc"
};
char D_0049EE20[0x20] = {
#include "sub_main/0049EE20.inc"
};
char D_0049EE40[0x20] = {
#include "sub_main/0049EE40.inc"
};
char D_0049EE60[0x20] = {
#include "sub_main/0049EE60.inc"
};
char D_0049EE80[0x20] = {
#include "sub_main/0049EE80.inc"
};
char D_0049EEA0[0x20] = {
#include "sub_main/0049EEA0.inc"
};
char D_0049EEC0[0x20] = {
#include "sub_main/0049EEC0.inc"
};
char D_0049EEE0[0x20] = {
#include "sub_main/0049EEE0.inc"
};
char D_0049EF00[0x20] = {
#include "sub_main/0049EF00.inc"
};
char D_0049EF20[0x20] = {
#include "sub_main/0049EF20.inc"
};
char D_0049EF40[0x20] = {
#include "sub_main/0049EF40.inc"
};
char D_0049EF60[0x20] = {
#include "sub_main/0049EF60.inc"
};
char D_0049EF80[0x20] = {
#include "sub_main/0049EF80.inc"
};
char D_0049EFA0[0x20] = {
#include "sub_main/0049EFA0.inc"
};
char D_0049EFC0[0x20] = {
#include "sub_main/0049EFC0.inc"
};
char D_0049EFE0[0x20] = {
#include "sub_main/0049EFE0.inc"
};
char D_0049F000[0x20] = {
#include "sub_main/0049F000.inc"
};
char D_0049F020[0x20] = {
#include "sub_main/0049F020.inc"
};
char D_0049F040[0x20] = {
#include "sub_main/0049F040.inc"
};
char D_0049F060[0x20] = {
#include "sub_main/0049F060.inc"
};
char D_0049F080[0x20] = {
#include "sub_main/0049F080.inc"
};
char D_0049F0A0[0x20] = {
#include "sub_main/0049F0A0.inc"
};
char D_0049F0C0[0x20] = {
#include "sub_main/0049F0C0.inc"
};
char D_0049F0E0[0x20] = {
#include "sub_main/0049F0E0.inc"
};
char D_0049F100[0x20] = {
#include "sub_main/0049F100.inc"
};
char D_0049F120[0x20] = {
#include "sub_main/0049F120.inc"
};
char D_0049F140[0x20] = {
#include "sub_main/0049F140.inc"
};
char D_0049F160[0x40] = {
#include "sub_main/0049F160.inc"
};
char D_0049F1A0[0x30] = {
#include "sub_main/0049F1A0.inc"
};
char D_0049F1D0[0x30] = {
#include "sub_main/0049F1D0.inc"
};
char D_0049F200[0x30] = {
#include "sub_main/0049F200.inc"
};
char D_0049F230[0x30] = {
#include "sub_main/0049F230.inc"
};
char D_0049F260[0x30] = {
#include "sub_main/0049F260.inc"
};
char D_0049F290[0x30] = {
#include "sub_main/0049F290.inc"
};
char D_0049F2C0[0x40] = {
#include "sub_main/0049F2C0.inc"
};
char D_0049F300[0x40] = {
#include "sub_main/0049F300.inc"
};
char D_0049F340[0x40] = {
#include "sub_main/0049F340.inc"
};
char D_0049F380[0x40] = {
#include "sub_main/0049F380.inc"
};
char D_0049F3C0[0x30] = {
#include "sub_main/0049F3C0.inc"
};
char D_0049F3F0[0x40] = {
#include "sub_main/0049F3F0.inc"
};
char D_0049F430[0x40] = {
#include "sub_main/0049F430.inc"
};
char D_0049F470[0x30] = {
#include "sub_main/0049F470.inc"
};
char D_0049F4A0[0x50] = {
#include "sub_main/0049F4A0.inc"
};
char D_0049F4F0[0x50] = {
#include "sub_main/0049F4F0.inc"
};
char D_0049F540[0x30] = {
#include "sub_main/0049F540.inc"
};
char D_0049F570[0x30] = {
#include "sub_main/0049F570.inc"
};
char D_0049F5A0[0x40] = {
#include "sub_main/0049F5A0.inc"
};
char D_0049F5E0[0x40] = {
#include "sub_main/0049F5E0.inc"
};
char D_0049F620[0x60] = {
#include "sub_main/0049F620.inc"
};
char D_0049F680[0x40] = {
#include "sub_main/0049F680.inc"
};
char D_0049F6C0[0x40] = {
#include "sub_main/0049F6C0.inc"
};
char D_0049F700[0x30] = {
#include "sub_main/0049F700.inc"
};
char D_0049F730[0x30] = {
#include "sub_main/0049F730.inc"
};
char D_0049F760[0x30] = {
#include "sub_main/0049F760.inc"
};
char D_0049F790[0x30] = {
#include "sub_main/0049F790.inc"
};
char D_0049F7C0[0x30] = {
#include "sub_main/0049F7C0.inc"
};
char D_0049F7F0[0x30] = {
#include "sub_main/0049F7F0.inc"
};
char D_0049F820[0x20] = {
#include "sub_main/0049F820.inc"
};
char D_0049F840[0x30] = {
#include "sub_main/0049F840.inc"
};
char D_0049F870[0x30] = {
#include "sub_main/0049F870.inc"
};
char D_0049F8A0[0x30] = {
#include "sub_main/0049F8A0.inc"
};
char D_0049F8D0[0x30] = {
#include "sub_main/0049F8D0.inc"
};
char D_0049F900[0x30] = {
#include "sub_main/0049F900.inc"
};
char D_0049F930[0x30] = {
#include "sub_main/0049F930.inc"
};
char D_0049F960[0x30] = {
#include "sub_main/0049F960.inc"
};
char D_0049F990[0x30] = {
#include "sub_main/0049F990.inc"
};
char D_0049F9C0[0x30] = {
#include "sub_main/0049F9C0.inc"
};
char D_0049F9F0[0x30] = {
#include "sub_main/0049F9F0.inc"
};
char D_0049FA20[0x40] = {
#include "sub_main/0049FA20.inc"
};
char D_0049FA60[0x20] = {
#include "sub_main/0049FA60.inc"
};
char D_0049FA80[0x20] = {
#include "sub_main/0049FA80.inc"
};
char D_0049FAA0[0x30] = {
#include "sub_main/0049FAA0.inc"
};
char D_0049FAD0[0x30] = {
#include "sub_main/0049FAD0.inc"
};
char D_0049FB00[0x30] = {
#include "sub_main/0049FB00.inc"
};
char D_0049FB30[0x40] = {
#include "sub_main/0049FB30.inc"
};
char D_0049FB70[0x28] = {
#include "sub_main/0049FB70.inc"
};
#pragma rodata off

void Area_DrawProp(PropRec *pr, Model *set, u8 *alpha)
{
    AreaState *as = &gAreaState;
    ModelMatSet *e = &set->matSets[pr->model];
    f32 *d;
    s16 i;
    f32 mtx[4][4];
    f32 tm[4][4];
    f32 v[3];

    if (pr->flags & 8) {
        if (as->look[0] >= 0xFF) {
            return;
        }
        Gs_SetState(0x67, ((0xFF - as->look[0]) << 24) | 0xFFFFFF);
    } else if (pr->flags & 0x10) {
        if (as->look[0] <= 0) {
            return;
        }
        Gs_SetState(0x67, (as->look[0] << 24) | 0xFFFFFF);
    } else {
        Gs_SetState(0x67, -1);
    }
    if (pr->flags & 2) {
        Gs_SetState(0x6C, 0);
    }
    Draw_ApplyStates(e->flags);
    if (pr->alpha != *alpha) {
        Gs_SetState(0x60, pr->alpha);
        *alpha = pr->alpha;
    }
    d = pr->data;
    for (i = 0; i < pr->count; i++) {
        Mtx_Identity(mtx[0]);
        switch (pr->type) {
        case 0:
            Gs_SetState(0x1A, (s32)mtx);
            break;
        case 1:
            AreaAnim_TranslateSpin(mtx[0], (struct AnimSpin *)d, pr->flags);
            d += 4;
            break;
        case 2:
            AreaAnim_UvScroll((struct UvScroll *)d, pr->flags);
            Gs_SetState(0x1A, (s32)mtx);
            d += 1;
            break;
        case 3:
            AreaAnim_UvWave((struct UvWave *)d);
            Gs_SetState(0x1A, (s32)mtx);
            d += 6;
            break;
        case 5:
            AreaAnim_TranslateRotate(mtx[0], (struct AnimRot *)d, pr->flags);
            d += 4;
            break;
        case 4:
            AreaAnim_Translate(mtx[0], d);
            d += 3;
            break;
        case 6:
            AreaAnim_ScaleTranslateRotate(mtx[0], (struct AnimRotScale *)d, pr->flags);
            d += 4;
            break;
        case 7:
            AreaAnim_TranslateXZ(mtx[0], d);
            d += 2;
            break;
        case 8:
            AreaAnim_UvScrollTranslate(mtx[0], (struct AnimScrollPos *)d, pr->flags);
            d += 2;
            break;
        case 9:
            AreaAnim_UvScrollSpin(mtx[0], (struct AnimScrollSpin *)d, pr->flags);
            break;
        case 10:
            AreaAnim_UvScrollRotate(mtx[0], (struct AnimScrollRot *)d, pr->flags);
            d += 2;
            break;
        case 11:
            AreaAnim_SpinScroll(mtx[0], (struct AnimSpin2 *)d);
            d += 4;
            break;
        case 12:
            AreaAnim_UvStep((struct UvStep *)d);
            Gs_SetState(0x1A, (s32)mtx);
            d += 4;
            break;
        case 13:
            Mtx_Translation(mtx[0], d[0] + as->propOfs[0], d[1] + as->propOfs[1], d[2] + as->propOfs[2]);
            switch (as->propState) {
            case 1:
                if (as->propTimer > 0x1E) {
                    Mtx_RotateX(mtx[0], 6.2831855f * (-2.0f * Math_Sin(6.2831855f * ((as->propTimer - 0x1E) << 8) / 65536.0f)) / 360.0f);
                }
                break;
            case 2:
                if (as->propTimer < 0x100) {
                    Mtx_RotateX(mtx[0], 6.2831855f * (-2.0f * Math_Sin(6.2831855f * (as->propTimer << 8) / 65536.0f)) / 360.0f);
                }
                break;
            }
            Gs_SetState(0x1A, (s32)mtx);
            d += 3;
            break;
        case 0x64:
            if (as->frame & 0x20) {
                Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.125f * ((as->frame >> 2) & 7), 0.0f);
            } else {
                Mtx_Translation(tm[0], 0.75f - 0.25f * (as->frame & 3), 0.875f - 0.125f * ((as->frame >> 2) & 7), 0.0f);
            }
            Gs_SetState(0x19, (s32)tm);
            Gs_SetState(0x1A, (s32)mtx);
            break;
        case 0x65:
            Mtx_Translation(mtx[0], d[0], d[1], d[2]);
            Mtx_RotateY(mtx[0], d[3]);
            Mtx_RotateZ(mtx[0], d[4]);
            Gs_SetState(0x1A, (s32)mtx);
            d += 5;
            break;
        case 0x66: {
            f32 f = 16.0f * (1.0f + Math_Sin(6.2831855f * (as->frame << 14) / 65536.0f));
            u8 a = 0.8745098f * (u32)as->look[0];
            u8 b = f * ((u32)as->look[0] / 255.0f);
            a += b;
            Gs_SetState(0x67, (a << 24) | 0xFFFFFF);
            Gs_SetState(0x1A, (s32)mtx);
            break;
        }
        case 0x67: {
            u8 r, g, b;
            f32 f;

            if (as->look[0] == 0) {
                b = g = r = 0xFF;
            } else if (as->look[0] == 0xFF) {
                b = g = r = 0x30;
            } else {
                f = 255.0f - 207.0f * ((u32)as->look[0] / 255.0f);
                r = f;
                g = f;
                b = f;
            }
            Gs_SetState(0x67, (r << 16) | 0xFF000000 | (g << 8) | b);
            AreaAnim_TranslateSpin(mtx[0], (struct AnimSpin *)d, pr->flags);
            d += 4;
            break;
        }
        case 0x68: {
            u8 a = 95.0f * (0.5f * (1.0f + Math_Sin(6.2831855f * (as->frame << 10) / 65536.0f)));

            Gs_SetState(0x67, ((u8)(a + 0xA0) << 24) | 0xFFFFFF);
            Gs_SetState(0x1A, (s32)mtx);
            break;
        }
        case 0x69:
            Vec_Copy(v, d);
            if (!(as->frame & 0x100)) {
                if ((as->frame & 0xFF) < 0x80) {
                    v[2] += 2600.0f * ((as->frame & 0x7F) / 128.0f);
                } else {
                    v[2] += 2600.0f;
                }
            } else if ((as->frame & 0xFF) < 0x80) {
                v[2] += 2600.0f * (1.0f - (as->frame & 0x7F) / 128.0f);
            }
            Mtx_SetTrans(mtx[0], v[0], v[1], v[2]);
            Gs_SetState(0x1A, (s32)mtx);
            d += 3;
            break;
        case 0x6A:
            Mtx_SetTrans(mtx[0], d[0], d[1], d[2]);
            Mtx_RotateZ(mtx[0], 6.2831855f * (2.5f * Math_Sin(6.2831855f * (as->frame << 9) / 65536.0f)) / 360.0f);
            Mtx_RotateY(mtx[0], d[3]);
            Gs_SetState(0x1A, (s32)mtx);
            d += 4;
            break;
        case 0x6B: {
            s16 sh = *(s16 *)&d[3];
            u16 ang = (((as->frame & (0xFFFF >> sh)) << sh) + (0x10000 / pr->count) * i);

            if (ang > 0x471C && ang < 0xB1C7) {
                continue;
            }
            Mtx_Translation(mtx[0], d[0], d[1], d[2]);
            Mtx_RotateZ(mtx[0], 6.2831855f * (2.5f * Math_Sin(6.2831855f * (as->frame << 9) / 65536.0f)) / 360.0f);
            Mtx_RotateY(mtx[0], 6.2831855f * ang / 65536.0f);
            Gs_SetState(0x1A, (s32)mtx);
            break;
        }
        case 0x6C:
            Mtx_SetTrans(mtx[0], d[0], d[1], d[2]);
            Mtx_RotateXYZ(mtx[0], d[3], d[4], d[5]);
            Gs_SetState(0x1A, (s32)mtx);
            d += 6;
            break;
        case 0x6D: {
            f32 s = 0.35f * Math_Sin(6.2831855f * (3.0f * ((as->frame & 0x1FF) / 512.0f)));
            f32 r;

            r = s + 0.06f * Math_Sin(6.2831855f * (7.0f * ((as->frame & 0x1FF) / 512.0f)));
            s = 0.85f + r;
            r = 5.0f * Math_Sin(6.2831855f * (as->frame << 8) / 65536.0f);
            Mtx_Scale(mtx[0], 1.0f, 1.0f, s);
            Mtx_SetTrans(mtx[0], d[0], d[1], d[2]);
            Mtx_RotateY(mtx[0], d[3] + 6.2831855f * r / 360.0f);
            Gs_SetState(0x1A, (s32)mtx);
            d += 4;
            break;
        }
        case 0x6E: {
            f32 s = 0.35f * Math_Sin(6.2831855f * (5.0f * ((as->frame & 0x1FF) / 512.0f)));

            Mtx_Scale(mtx[0], 1.0f, 1.0f, 0.85f + (s + 0.09f * Math_Sin(6.2831855f * (9.0f * ((as->frame & 0x1FF) / 512.0f)))));
            Mtx_SetTrans(mtx[0], d[0], d[1], d[2]);
            Mtx_RotateY(mtx[0], d[3]);
            Gs_SetState(0x1A, (s32)mtx);
            d += 4;
            break;
        }
        case 0x6F:
            Gs_SetState(0x67, ((0xFF - (as->look[0] >> 1)) << 24) | 0xFFFFFF);
            AreaAnim_UvScrollTranslate(mtx[0], (struct AnimScrollPos *)d, pr->flags);
            d += 2;
            break;
        case 0x73:
        case 0x74: {
            f32 f = 24.0f * (1.0f + Math_Sin(6.2831855f * (as->frame << 12) / 65536.0f));
            u32 k = as->look[0];
            u8 a = 0.8117647f * k;
            a += f * (k / 255.0f);
            Gs_SetState(0x67, (a << 24) | 0xFFFFFF);
            Gs_SetState(0x1A, (s32)mtx);
            break;
        }
        case 0x70:
        case 0x71:
        case 0x72: {
            f32 f = 16.0f * (1.0f + Math_Sin(6.2831855f * (as->frame << 14) / 65536.0f));
            u32 k = as->look[0];
            u8 a = 0.8745098f * k;
            u8 b = f * (k / 255.0f);
            a += b;
            Gs_SetState(0x67, (a << 24) | 0xFFFFFF);
            Gs_SetState(0x1A, (s32)mtx);
            break;
        }
        case 0x75:
            if (*(s16 *)d != (u8)Village_FacilityLevel(0)) {
                continue;
            }
            Gs_SetState(0x1A, (s32)mtx);
            break;
        case 0x76:
            if (*(u8 *)d != as->look[1]) {
                continue;
            }
            Gs_SetState(0x1A, (s32)mtx);
            break;
        case 0x77: {
            u8 *h = (u8 *)gEms;

            for (i = 0; i < 20; i++, h += 0x9D0) {
                if (h[0] != 0 && h[1] != 0 && (h[2] == 0x36 || h[2] == 0x3C)) {
                    AreaAnim_UvScroll((struct UvScroll *)d, pr->flags);
                    Gs_SetState(0x1A, (s32)mtx);
                    break;
                }
            }
            d += 1;
            if (i == 20) {
                continue;
            }
            break;
        }
        default:
            continue;
        }
        Model_Draw(e->id, 0);
    }
    if (pr->flags & 2) {
        Gs_SetState(0x6C, 1);
    }
}

/* An area model's animation (gAreaAnimTbls): 8 bytes each, sorted by model. */
typedef struct AreaAnim {
    u8 model;                   /* 0x00: its model in the area's set */
    u8 alpha;                   /* 0x01: Gs_SetState 0x60 */
    u8 type;                    /* 0x02 */
    u8 flags;                   /* 0x03 */
    void *arg;                  /* 0x04 */
} AreaAnim;

/* A choice of props by the village's facilities (gVillagePropChoices). */
typedef struct PropChoice {
    u16 mask;                   /* 0x00 */
    u8 pad02[2];
    PropRec *props;                /* 0x04 */
} PropChoice;

typedef struct AreaProps {
    u16 area;                   /* 0x00: 0xFFFF ends the table */
    s16 count;                  /* 0x02 */
    PropChoice *choices;        /* 0x04 */
} AreaProps;

/* The props drawn after the others (AreaSky_Draw). */
typedef struct PropDefer {
    PropRec *prop[2];
    u8 set[2];                  /* their model sets */
} PropDefer;

extern AreaAnim *gAreaAnimTbls[];
extern PropRec *gAreaProps[];
extern AreaProps gVillagePropChoices[];
void LightSet_Apply(s32 a);
s32 Tex_CountLoaded(s32 a);
void Tex_Prepare(s32 a, s32 b);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetRotXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_Identity(f32 *m);
void AreaAnim_TranslateSpin(void *m, struct AnimSpin *arg, u8 flags);
void AreaAnim_UvScroll(struct UvScroll *arg, u8 flags);
void AreaAnim_UvWave(struct UvWave *arg);
void AreaAnim_UvScrollSpin(void *m, struct AnimScrollSpin *arg, u8 flags);
void AreaAnim_UvStep(struct UvStep *arg);
void AreaAnim_Translate(void *m, f32 *pos);
void AreaAnim_UvScrollTranslate(void *m, struct AnimScrollPos *arg, u8 flags);
void AreaAnim_SpinScroll(void *m, struct AnimSpin2 *arg);
f32 Math_Sin(f32 a);
s32 Gs_ClearColor(u8 a, u32 b);
u8 Village_FacilityLevel(s32 k);
void Draw_SetDefaultState(void);
void Area_DrawProp(PropRec *pr, Model *set, u8 *alpha);

void AreaSky_Draw(void)
{
    s32 i;
    AreaState *as = &gAreaState;
    Model *set;
    ModelMat *lights;
    u8 alpha;
    PropRec *p;
    s32 k;
    s32 j;
    ModelMatSet *m;
    f32 mtx[4][4];
    PropDefer defer;
    u16 nd;
    AreaAnim *an;
    AreaProps *ap;
    PropChoice *pc;
    Model **sets;

    if (as->active == 0 || as->drawSky == 0) {
        return;
    }
    if (gScene.area != 0) {
    LightSet_Apply(0);
    set = as->model;
    if (set != NULL && set->active != 0) {
    Tex_CountLoaded(0x108);
    Tex_Prepare(0x10, 0x108);
    Gs_SetState(0x60, 0x80);
    alpha = 0x80;
    Gs_SetState(0x67, -1);
    lights = set->mats;
    m = set->matSets;
    Mtx_Translation(mtx[0], as->pos[0], as->pos[1], as->pos[2]);
    Mtx_SetRotXYZ(mtx[0], as->rot[0], as->rot[1], as->rot[2]);
    Gs_SetState(0x1A, (s32)mtx);
    i = set->matSetCount;
    an = gAreaAnimTbls[as->area];
    for (k = 0; k < i; k++, m++) {
        if (k == 0) {
            Gs_SetState(0x6D, 7);
        }
        Gs_SetLightList(lights, m);
        Draw_ApplyStates(m->flags);
        if (an != NULL && k == an->model) {
            for (;;) {
                if (!(an->flags & 1) != 0 || gScene.eventRunning != 1) {
                    if (an->flags & 2) {
                        Gs_SetState(0x6C, 0);
                    }
                    if (an->flags & 8) {
                        Gs_SetState(0x67, ((0xFF - as->look[0]) << 24) | 0xFFFFFF);
                    } else if (an->flags & 0x10) {
                        Gs_SetState(0x67, (as->look[0] << 24) | 0xFFFFFF);
                    } else {
                        Gs_SetState(0x67, -1);
                    }
                    if (an->alpha != alpha) {
                        Gs_SetState(0x60, an->alpha);
                        alpha = an->alpha;
                    }
                    switch (an->type) {
                    case 0:
                        break;
                    case 3:
                        goto next;
                    case 1:
                        AreaAnim_TranslateSpin(mtx[0], (struct AnimSpin *)an->arg, an->flags);
                        break;
                    case 4:
                        AreaAnim_UvScroll((struct UvScroll *)an->arg, an->flags);
                        break;
                    case 5:
                        AreaAnim_UvWave((struct UvWave *)an->arg);
                        break;
                    case 6:
                        AreaAnim_UvScrollSpin(mtx[0], (struct AnimScrollSpin *)an->arg, an->flags);
                        break;
                    case 7:
                        AreaAnim_UvStep((struct UvStep *)an->arg);
                        break;
                    case 2:
                        AreaAnim_Translate(mtx[0], an->arg);
                        break;
                    case 8:
                        AreaAnim_UvScrollTranslate(mtx[0], (struct AnimScrollPos *)an->arg, an->flags);
                        break;
                    case 9:
                        AreaAnim_SpinScroll(mtx[0], (struct AnimSpin2 *)an->arg);
                        break;
                    case 0x64:
                        for (j = 0; j < m->count; j++) {
                            ModelMat *l = &lights[m->mat[j]];
                            switch (j) {
                            case 0:
                                l->alpha = 1.0f;
                                break;
                            case 1:
                                l->alpha = 0.0f;
                                break;
                            }
                            Gs_SetState((u8)(j + 0x3A), (s32)l);
                        }
                        break;
                    case 0x65:
                        if (as->frame & 0xC0) {
                            goto next;
                        }
                        AreaAnim_UvScroll((struct UvScroll *)an->arg, an->flags);
                        break;
                    case 0x66:
                    case 0x67: {
                        u8 r;
                        u8 g;
                        u8 b;
                        f32 f;

                        if (as->look[0] == 0) {
                            b = g = r = 0xFF;
                        } else if (as->look[0] == 0xFF) {
                            b = g = r = 0x30;
                        } else {
                            f = 255.0f - 207.0f * ((u32)as->look[0] / 255.0f);
                            r = f;
                            g = f;
                            b = f;
                        }
                        Gs_SetState(0x67, (r << 16) | 0xFF000000 | (g << 8) | b);
                        AreaAnim_UvScroll((struct UvScroll *)an->arg, an->flags);
                        break;
                    }
                    case 0x68:
                    case 0x69:
                    case 0x6A:
                    case 0x6B: {
                        u8 *arg = an->arg;

                        AreaAnim_TranslateSpin(mtx[0], (struct AnimSpin *)arg, an->flags);
                        if (an->type == 0x69) {
                            mtx[0][0] *= 1.25f;
                            mtx[0][1] *= 1.25f;
                            mtx[0][2] *= 1.25f;
                            mtx[1][0] *= 1.25f;
                            mtx[1][1] *= 1.25f;
                            mtx[1][2] *= 1.25f;
                            mtx[2][0] *= 1.25f;
                            mtx[2][1] *= 1.25f;
                            mtx[2][2] *= 1.25f;
                        }
                        Mtx_RotateY(mtx[0], (6.2831855f * (u32)*(u16 *)(arg + 0x10)) / 65536.0f);
                        Gs_SetState(0x1A, (s32)mtx);
                        break;
                    }
                    case 0x6C: {
                        f32 f = 0.5f * (1.0f + Math_Sin((6.2831855f * (as->frame << 10)) / 65536.0f));
                        u8 a = 32.0f * f;
                        u8 c = 64.0f * f;

                        Gs_SetState(0x67, (((u8)(a + 0xC0)) << 8) | 0xFFFF0000 | (u8)(c + 0xA0));
                        break;
                    }
                    }
                    Model_Draw(m->id, 0);
                    switch (an->type) {
                    case 1:
                    case 2:
                    case 6:
                    case 8:
                    case 9:
                    case 0x68:
                    case 0x69:
                    case 0x6A:
                    case 0x6B:
                        Mtx_Translation(mtx[0], as->pos[0], as->pos[1], as->pos[2]);
                        Mtx_SetRotXYZ(mtx[0], as->rot[0], as->rot[1], as->rot[2]);
                        Gs_SetState(0x1A, (s32)mtx);
                        break;
                    }
                }
            next:
                if (an->flags & 2) {
                    Gs_SetState(0x6C, 1);
                }
                an++;
                if (an->model == k) {
                    continue;
                }
                break;
            }
        } else {
            if (alpha != 0x80) {
                Gs_SetState(0x60, 0x80);
                alpha = 0x80;
            }
            Model_Draw(m->id, 0);
        }
        if (k == 0) {
            Gs_SetState(0x6C, 1);
            Gs_ClearColor(0, 0);
            Gs_SetState(0x6D, 3);
        }
    }
    alpha = 0x80;
    Gs_SetState(0x60, 0x80);
    set = gAreaSubModels[0];
    if (set == NULL || set->active == 0) {
        goto end;
    }
    Tex_Prepare(0x10, 0x128);
    p = gAreaProps[as->area];
    nd = 0;
    if (p != NULL) {
        while (p->model != 0xFF) {
            if (p->late == 0) {
                Area_DrawProp(p, set, &alpha);
            } else if (nd < 2) {
                defer.prop[nd] = p;
                defer.set[nd] = 0;
                nd++;
            } else {
                Area_DrawProp(p, set, &alpha);
            }
            p++;
        }
    }
    for (ap = gVillagePropChoices; ap->area != 0xFFFF; ap++) {
        if (ap->area == as->area) {
            break;
        }
    }
    if (ap->area != 0xFFFF) {
        sets = &gAreaSubModels[1];
        for (i = 1; i < 4; i++, sets++) {
            set = *sets;
            if (set == NULL || set->active == 0) {
                goto end;
            }
            pc = ap->choices;
            switch (as->area) {
            case 0x89: {
                u8 lvl = Village_FacilityLevel(i);
                u16 mask;

                k = ap->count; /* the choice count */
                mask = (1 << i) | (0x100 << lvl);

                for (j = 0; j < k; j++, pc++) {
                    if ((pc->mask & mask) == mask) {
                        p = pc->props;
                        break;
                    }
                }
                if (j == k) {
                    p = NULL;
                }
                if (p != NULL) {
                    while (p->model != 0xFF) {
                        if (p->late == 0) {
                            Area_DrawProp(p, set, &alpha);
                        } else if (nd < 2) {
                            defer.prop[nd] = p;
                            defer.set[nd] = i;
                            nd++;
                        } else {
                            Area_DrawProp(p, set, &alpha);
                        }
                        p++;
                    }
                } else {
                    s32 cnt = set->matSetCount;

                    for (k = 0; k < cnt; k++) {
                        ModelMatSet *e = &set->matSets[k];

                        Draw_ApplyStates(e->flags);
                        Mtx_Identity(mtx[0]);
                        Gs_SetState(0x1A, (s32)mtx);
                        Model_Draw(e->id, 0);
                    }
                }
                break;
            }
            }
        }
    }
    if (nd != 0) {
        for (k = 0; k < nd; k++) {
            set = gAreaSubModels[defer.set[k]];
            if (set != NULL && set->active != 0) {
                Area_DrawProp(defer.prop[k], set, &alpha);
            }
        }
    }
end:
    Draw_SetDefaultState();
    Gs_SetState(0x60, 0);
    }
    }
}

/* Colour c0 to c1 by t (0-1), per byte: c0 + t * (c1 - c0). */
#pragma optimization_level 3
static inline u8 Lerp8(u8 x, u8 y, f32 t)
{
    return x + (u8)(t * (s16)(y - x));
}
/* The colour's three upper bytes (round 29: or-ing them in place leaves the last OR's operands
 * the other way round). */
static inline u32 Color_PackARG(u8 r, u8 g, u8 a)
{
    return (a << 24) | (r << 16) | (g << 8);
}
/* The g and a bytes of c0 are shifted in place in s (round 29: s = c0 >> 8 in one statement is
 * forward-substituted, and the registers of the second and third channels differ). */
static u32 Color_Lerp(u32 c0, u32 c1, f32 t)
{
    u8 r;
    u8 g;
    s32 a;
    u32 s;

    r = Lerp8(c0 >> 16, c1 >> 16, t);
    s = c0;
    s >>= 8;
    g = Lerp8(s, c1 >> 8, t);
    s = c0;
    s >>= 24;
    a = Lerp8(s, c1 >> 24, t);
    return Lerp8(c0, c1, t) | Color_PackARG(r, g, a);
}
#pragma optimization_level reset

static s32 Sky_GetGradient(void)
{
    switch (gScene.area) {
    case 0x56:
    case 0x88:
    case 0xAD:
    case 0xAE:
    case 0xAF:
        return (s32) gSkyDayTableAD;
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
        return (s32) gSkyDayTable84;
    default:
        return (s32) gSkyDayTable;
    }
}

extern f32 gDayStepTimes[];

/* The time of day of step k of the area's day table (Sky_GetGradient: 8 steps of 3 words; the first
 * word a row of gDayStepTimes, or 5 for "until the next step", other: "since the previous"), wrapped
 * into [0, 1]. The previous of step 0 reads word 7 (entry 2's second word). */
static f32 DayTable_GetTime(s16 k, f32 t)
{
    s32 base = Sky_GetGradient();
    s32 n = k;
    s32 type;
    s32 *e = (s32 *)(base + n * 12);
    f32 r;

    type = e[0];

    if ((u32)type <= 3) {
        r = gDayStepTimes[type];
    } else if (type == 5) {
        if (n >= 7) {
            r = gDayStepTimes[*(s32 *)base] - t;
        } else {
            r = gDayStepTimes[e[3]] - t;
        }
    } else if (n == 0) {
        r = t + gDayStepTimes[*(s32 *)(base + 0x1C)];
    } else {
        r = t + gDayStepTimes[e[-3]];
    }
    if (r < 0.0f) {
        r += 1.0f;
    } else if (r > 1.0f) {
        r -= 1.0f;
    }
    return r;
}

/* A step of an area's day table (Sky_GetGradient): 8 steps. */
typedef struct DayStep {
    s32 type;                   /* 0x0: a row of gDayStepTimes, or 5, see DayTable_GetTime */
    u32 top;                    /* 0x4: the sky's top colour */
    u32 bottom;                 /* 0x8: its bottom colour */
} DayStep;

f32 LobbyTime_GetDayLen30(void);
f32 LobbyTime_GetDayFrac(void);

#pragma optimization_level 3
/* Matched in round 29 (level 3). DayTable_GetTime is static: the game keeps f2, f12 and t0-t2
 * across its calls. Colour 1 is evaluated r, g, b, a (LerpF into locals), colour 2 a, r, g, b
 * (one expression). */
/* A channel of colour c as a float (0-255). */
#define CH(c, s) ((f32)(((c) >> (s)) & 0xFF))
/* x (a float channel) moved to y by t. */
static inline u8 LerpF(f32 x, f32 y, f32 t)
{
    return x + t * (y - x);
}

/* Draws the sky of the areas with a day cycle: a full-screen gradient whose top and bottom
 * colours blend between the two steps of the area's day table around the current time. */
void Sky_DrawGradient(void)
{
    PrimGradRect r;
    DayStep *steps;
    f32 now;
    f32 t;
    f32 tm;
    f32 prevTime;
    f32 num;
    f32 den;
    f32 f;
    s16 i;
    s16 next;
    s16 prev;
    f32 fr;
    f32 fg;
    f32 fb;
    f32 fa;
    u8 cr;
    u8 cg;
    u8 cb;
    u8 ca;
    u32 c;
    u8 *pcr;
    u32 ta;
    u32 tb;
    u32 *sp;
    DayStep *ps;
    DayStep *pn;
    u32 *sn;

    Gs_SetState(0x60, 0);
    r.x1 = 0x280;
    r.x0 = 0;
    r.y1 = 0x1C0;
    r.y0 = 0;
    if (gScene.online != 0) {
        tb = LobbyTime_GetDayLen30();
        now = LobbyTime_GetDayFrac();
    } else {
        return;
    }
    steps = (DayStep *)Sky_GetGradient();
    t = 30.0f / tb;
    prev = 7;
    next = 8;
    for (i = 0; i < 8; i++) {
        tm = DayTable_GetTime(i, t);
        if (tm > now) {
            next = i;
            break;
        } else {
            prev = i;
            prevTime = tm;
        }
    }
    if (next == 0) {
        prevTime = DayTable_GetTime(prev, t);
    }
    if (next >= 8) {
        next = 0;
        tm = DayTable_GetTime(0, t);
    }
    ps = &steps[prev];
    pn = &steps[next];
    if (next <= 0) {
        num = now - prevTime;
        den = 1.0f - prevTime + tm;
        if (num < 0.0f) {
            num += 1.0f;
        }
    } else {
        den = tm - prevTime;
        num = now - prevTime;
    }
    f = num / den;
    ta = ps->top;

    fr = CH(ta, 16);
    fg = CH(ta, 8);
    fb = CH(ta, 0);
    fa = CH(ta, 24);
    tb = pn->top;
    cr = LerpF(fr, CH(tb, 16), f);
    cg = LerpF(fg, CH(tb, 8), f);
    cb = LerpF(fb, CH(tb, 0), f);
    ca = LerpF(fa, CH(tb, 24), f);
    /* FAKEMATCH: red read through a pointer (a permuter find): the address taken moves
     * cr's numbering so that ps, pn and next get the game's v1, v0 and a0 (ours a0, v1, a1
     * without it); it emits no code. */
    pcr = &cr;
    c = cb | ((cg << 8) | ((ca << 24) | (*pcr << 16)));
    r.color[0] = c;
    r.color[1] = c;
    /* FAKEMATCH: each walker stepped by two ++ (the bottom colour, 8 bytes on): the game
     * steps the pointers in their own registers (addiu 8) and reads at 0(); one += 2,
     * &ps->bottom or any single step folds the 8 into the loads. */
    sn = (u32 *)pn;
    sn++;
    sn++;
    sp = (u32 *)ps;
    sp++;
    sp++;
    ta = *sp;
    fr = CH(ta, 16);
    fg = CH(ta, 8);
    fb = CH(ta, 0);
    fa = CH(ta, 24);
    tb = *sn;
    c = LerpF(fb, CH(tb, 0), f) | ((LerpF(fg, CH(tb, 8), f) << 8) |
        ((LerpF(fa, CH(tb, 24), f) << 24) | (LerpF(fr, CH(tb, 16), f) << 16)));
    r.color[2] = c;
    r.color[3] = c;
    Prim_FillGradRect(&r);
}
#pragma optimization_level reset

/* Color_Lerp is static, so that MWCC keeps t in f12 across the calls. */
/* Draws a sky band as a gradient rectangle: its corner colours blend between the band's four
 * headings by the camera's heading relative to dir. */
void Sky_DrawBand(SkyBand *s, f32 *dir)
{
    PrimGradRect r;
    u16 cam;
    u16 d;
    f32 t;

    r.x0 = 0;
    r.y0 = s->y;
    r.x1 = 0x280;
    r.y1 = s->h;
    cam = (s32)(0.5f + 65536.0f * Math_Atan2(-gCamDir[0], -gCamDir[2]) / 6.2831855f);
    d = (u16)(s32)(0.5f + 65536.0f * Math_Atan2(dir[0], dir[2]) / 6.2831855f) - cam;
    if (d < 0x4000) {
        t = d / 16384.0f;
        r.color[0] = Color_Lerp(s->c[0], s->c[2], t);
        r.color[1] = Color_Lerp(s->c[0], s->c[3], t);
        r.color[2] = Color_Lerp(s->c[4], s->c[6], t);
        r.color[3] = Color_Lerp(s->c[4], s->c[7], t);
    } else if (d < 0x8000) {
        t = (d - 0x4000) / 16384.0f;
        r.color[0] = Color_Lerp(s->c[2], s->c[1], t);
        r.color[1] = Color_Lerp(s->c[3], s->c[1], t);
        r.color[2] = Color_Lerp(s->c[6], s->c[5], t);
        r.color[3] = Color_Lerp(s->c[7], s->c[5], t);
    } else if (d < 0xC000) {
        t = (d - 0x8000) / 16384.0f;
        r.color[0] = Color_Lerp(s->c[1], s->c[3], t);
        r.color[1] = Color_Lerp(s->c[1], s->c[2], t);
        r.color[2] = Color_Lerp(s->c[5], s->c[7], t);
        r.color[3] = Color_Lerp(s->c[5], s->c[6], t);
    } else {
        t = (d - 0xC000) / 16384.0f;
        r.color[0] = Color_Lerp(s->c[3], s->c[0], t);
        r.color[1] = Color_Lerp(s->c[2], s->c[0], t);
        r.color[2] = Color_Lerp(s->c[7], s->c[4], t);
        r.color[3] = Color_Lerp(s->c[6], s->c[4], t);
    }
    Prim_FillGradRect(&r);
}

void Sky_DrawGradient(void);
void Sky_DrawBand(SkyBand *s, f32 *dir);
void Gfx_SetBlend(s32 a, s32 b);
void Gfx_SetBlendOp(s32 a);
s32 Prim_FillRect(struct PrimLine *rect);
f32 Math_Sin(f32 x);
extern s16 gAreaFade;
extern s16 gAreaFadeTimer;

typedef struct FillRect {
    s16 x, y, w, h;
    u32 color;
} FillRect;

/* Draws the area's sky: Sky_DrawGradient in areas 0x56, 0x57, 0x83-0x89 and 0xAD-0xAF, else the sky
 * band of the area state; then a full-screen fade by gAreaFade (1: black, 2: fading with
 * gAreaFadeTimer, else pulsing). */
void Sky_Draw(void)
{
    AreaState *as = &gAreaState;
    SkyBand band;
    FillRect r;
    u8 a;

    if (as->active == 0 || as->drawSky == 0) {
        return;
    }
    if (gScene.area == 0) {
        return;
    }
    Gs_SetState(0x6C, 0);
    Gs_SetState(0x60, 0);
    switch (as->area) {
    case 0x56:
    case 0x57:
    case 0x83:
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
    case 0x88:
    case 0x89:
    case 0xAD:
    case 0xAE:
    case 0xAF:
        Sky_DrawGradient();
        break;
    default:
        band = as->band;
        Sky_DrawBand(&band, as->skyPos);
        break;
    }
    if (gAreaFade != 0) {
        r.x = 0;
        r.w = 0x280;
        r.h = 0x1C0;
        r.y = 0;
        if (gAreaFade == 1) {
            Gfx_SetBlend(4, 5);
            a = 0xFF;
            Gfx_SetBlend(1, 1);
            Gfx_SetBlendOp(1);
        } else if (gAreaFade == 2) {
            Gfx_SetBlend(4, 5);
            a = 0xFF - (u8)(127.5f * gAreaFadeTimer);
            r.color = a << 24;
            Prim_FillRect((struct PrimLine *)&r);
            Gfx_SetBlend(1, 1);
            Gfx_SetBlendOp(1);
        } else {
            a = 255.0f * (1.0f + Math_Sin(6.2831855f * (gAreaFadeTimer * 0xB6 + 0xC000) / 65536.0f));
            Gfx_SetBlend(4, 5);
        }
        if (gAreaFade == 1 || gAreaFade == 2) {
            r.color = -1;
        } else {
            r.color = (a << 24) | 0xFFFFFF;
        }
        Prim_FillRect((struct PrimLine *)&r);
        if (gAreaFade == 1 || gAreaFade == 2) {
            Gfx_SetBlendOp(0);
        }
    }
    Gs_SetState(0x6C, 1);
    Gs_SetState(0x60, 0x80);
}

void Area_DrawEntry(ModelMatSet *list, f32 *v, ModelMat *base)
{
    Gs_SetState(0x1A, (s32) v);
    if ((list != NULL) && (list->id != -1)) {
        if (base != NULL) {
            Gs_SetLightList(base, list);
        }
        Draw_ApplyStates(list->flags);
        Model_Draw(list->id, 0);
    }
}

extern f32 gCamMatrix[];
extern f32 gCamYawMtx[];
extern f32 gGlowPosArea56[][3];
extern s16 gGlowEntryArea56[];
extern f32 gGlowPosArea83[][3];
extern s16 gGlowEntryArea83[];
extern f32 gGlowPosArea84[][3];
extern s16 gGlowEntryArea84[];
extern f32 gGlowPosArea85[][3];
extern s16 gGlowEntryArea85[];
extern s16 gGlowEntryArea86[];
extern f32 gGlowPosArea86[][3];
extern f32 gGlowPosArea87[][3];
extern f32 gGlowScaleArea87[];
extern f32 gGlowPosArea88[][3];
extern f32 gGlowScaleArea88[];
extern s16 gGlowEntryArea88[];
extern f32 gGlowPosArea89[][3];
extern f32 gGlowScaleArea89[];
extern s16 gGlowEntryArea89[];
extern s16 gGlowVillageEntries[];
extern s16 gGlowEntryAreaAA[];
extern f32 gGlowPosAreaAA[][3];
extern f32 gGlowRotYAreaAA[];
extern f32 gGlowPosAreaAE[][3];
extern s16 gGlowEntryAreaAE[];
extern f32 gGlowPosAreaAF[][3];
extern s16 gGlowEntryAreaAF[];
void Mtx_Concat3x3(f32 *m, f32 *n);
s32 Overlay_IsLoaded(s32 a);
void Draw_SetDefaultState(void);
u8 func_7D96D0(void); /* func_007D96D0 (lobby) */
u32 LobbyWorld_GetFadeColor(s32 i);

void Area_DrawGlowProps(void)
{
    AreaState *as = &gAreaState;
    Model *set;
    Model *set2;
    ModelMatSet *e;
    ModelMat *lights;
    s16 i;
    s16 k;
    u8 c;
    u8 a;
    u32 f8;
    f32 f;
    f32 m[4][4];
    f32 tm[4][4];

    if (gScene.online == 0) {
        return;
    }
    if (as->drawSky == 0) {
        return;
    }
    switch (as->area) {
    case 0x56:
    case 0x57:
    case 0x83:
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
    case 0x88:
    case 0x89:
    case 0xAA:
    case 0xAB:
    case 0xAC:
    case 0xAD:
    case 0xAE:
    case 0xAF:
        break;
    default:
        return;
    }
    Gs_SetState(0x6C, 0);
    Gs_SetState(0x60, 0);
    f = 24.0f * (1.0f + Math_Sin(6.2831855f * (as->frame << 12) / 65536.0f));
    f8 = as->look[0];
    a = 0.8117647f * f8;
    a += f * (f8 / 255.0f);
    set = gAreaSubModels[0];
    switch (as->area) {
    case 0x56:
    case 0x57:
        set2 = as->model;
        if (set2 != NULL && set2->active != 0) {
            lights = set2->mats;
            for (i = 0; i < 12; i++) {
                Mtx_Identity(m[0]);
                Mtx_SetTrans(m[0], gGlowPosArea56[i][0], gGlowPosArea56[i][1], gGlowPosArea56[i][2]);
                switch (gGlowEntryArea56[i]) {
                case 6:
                    Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.25f * ((as->frame >> 2) & 3), 0.0f);
                    Gs_SetState(0x19, (s32)tm);
                    Gs_SetState(0x67, (as->look[0] << 24) | 0xFFFFFF);
                    Mtx_Concat3x3(m[0], (f32 *)gCamYawMtx);
                    break;
                case 7:
                    Gs_SetState(0x67, (a << 24) | 0xFFFFFF);
                    Mtx_Concat3x3(m[0], (f32 *)gCamYawMtx);
                    break;
                case 8:
                    Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.25f * ((as->frame >> 2) & 3), 0.0f);
                    Gs_SetState(0x19, (s32)tm);
                    Gs_SetState(0x67, (as->look[0] << 24) | 0xFFFFFF);
                    break;
                default:
                    continue;
                }
                Area_DrawEntry(&set2->matSets[gGlowEntryArea56[i]], m[0], lights);
            }
        }
        break;
    case 0x83:
        set2 = as->model;
        if (set2 != NULL && set2->active != 0) {
            lights = set2->mats;
            for (i = 0; i < 11; i++) {
                Mtx_Identity(m[0]);
                Mtx_SetTrans(m[0], gGlowPosArea83[i][0], gGlowPosArea83[i][1], gGlowPosArea83[i][2]);
                switch (gGlowEntryArea83[i]) {
                case 11:
                    Mtx_Concat3x3(m[0], gCamMatrix);
                case 10:
                    Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.25f * ((as->frame >> 2) & 3), 0.0f);
                    Gs_SetState(0x19, (s32)tm);
                    Gs_SetState(0x67, (as->look[0] << 24) | 0xFFFFFF);
                    break;
                case 12:
                    Gs_SetState(0x67, (a << 24) | 0xFFFFFF);
                    Mtx_Concat3x3(m[0], gCamMatrix);
                    break;
                default:
                    continue;
                }
                Area_DrawEntry(&set2->matSets[gGlowEntryArea83[i]], m[0], lights);
            }
        }
        break;
    case 0x84:
        set2 = as->model;
        if (set2 != NULL && set2->active != 0) {
            lights = set2->mats;
            for (i = 0; i < 14; i++) {
                Mtx_Identity(m[0]);
                Mtx_SetTrans(m[0], gGlowPosArea84[i][0], gGlowPosArea84[i][1], gGlowPosArea84[i][2]);
                switch (gGlowEntryArea84[i]) {
                case 13:
                case 15:
                    Mtx_Concat3x3(m[0], (f32 *)gCamYawMtx);
                case 12:
                    Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.25f * ((as->frame >> 2) & 3), 0.0f);
                    Gs_SetState(0x19, (s32)tm);
                    Gs_SetState(0x67, (as->look[0] << 24) | 0xFFFFFF);
                    break;
                case 14:
                    Gs_SetState(0x67, (a << 24) | 0xFFFFFF);
                    Mtx_Concat3x3(m[0], (f32 *)gCamYawMtx);
                    break;
                default:
                    continue;
                }
                Area_DrawEntry(&set2->matSets[gGlowEntryArea84[i]], m[0], lights);
            }
        }
        break;
    case 0x85:
        set2 = as->model;
        if (set2 != NULL && set2->active != 0) {
            lights = set2->mats;
            Mtx_Identity(m[0]);
            Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.25f * ((as->frame >> 2) & 3), 0.0f);
            Gs_SetState(0x19, (s32)tm);
            Gs_SetState(0x67, -1);
            Area_DrawEntry(&set2->matSets[5], m[0], lights);
        }
        c = 207.0f + f;
        set2 = gAreaSubModels[0];
        if (set2 != NULL && set2->active != 0) {
            for (i = 0; i < 10; i++) {
                Mtx_Identity(m[0]);
                Mtx_SetTrans(m[0], gGlowPosArea85[i][0], gGlowPosArea85[i][1], gGlowPosArea85[i][2]);
                switch (gGlowEntryArea85[i]) {
                case 0:
                    Mtx_Translation(tm[0], 0.0f, (as->frame & 0xFF) / 256.0f, 0.0f);
                    Gs_SetState(0x19, (s32)tm);
                    Gs_SetState(0x67, -1);
                    break;
                case 1:
                    Mtx_Translation(tm[0], 0.5f * (as->frame & 1), 0.25f * ((as->frame >> 1) & 3), 0.0f);
                    Gs_SetState(0x19, (s32)tm);
                    Gs_SetState(0x67, -1);
                    break;
                case 2:
                    Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.25f * ((as->frame >> 2) & 3), 0.0f);
                    Gs_SetState(0x19, (s32)tm);
                    Gs_SetState(0x67, -1);
                    Mtx_Concat3x3(m[0], gCamMatrix);
                    break;
                case 3:
                    Gs_SetState(0x67, (c << 24) | 0xFFFFFF);
                    Mtx_Concat3x3(m[0], gCamMatrix);
                    break;
                default:
                    continue;
                }
                Area_DrawEntry(&set2->matSets[gGlowEntryArea85[i]], m[0], 0);
            }
        }
        break;
    case 0x86:
        if (set != NULL && set->active != 0) {
            for (i = 0; i < 9; i++) {
                Mtx_Translation(m[0], gGlowPosArea86[i][0], gGlowPosArea86[i][1], gGlowPosArea86[i][2]);
                k = gGlowEntryArea86[i];
                switch (k) {
                case 1:
                default:
                    c = a;
                    Mtx_Concat3x3(m[0], gCamMatrix);
                    break;
                case 2:
                    Mtx_Concat3x3(m[0], gCamMatrix);
                case 3:
                    c = as->look[0];
                    Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.25f * ((as->frame >> 2) & 3), 0.0f);
                    Gs_SetState(0x19, (s32)tm);
                    break;
                }
                Gs_SetState(0x67, (c << 24) | 0xFFFFFF);
                Area_DrawEntry(&set->matSets[k], m[0], 0);
            }
        }
        break;
    case 0x87:
        set2 = as->model;
        if (set2 != NULL && set2->active != 0) {
            lights = set2->mats;
            Mtx_Identity(m[0]);
            Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.25f * ((as->frame >> 2) & 3), 0.0f);
            Gs_SetState(0x19, (s32)tm);
            Gs_SetState(0x67, (as->look[0] << 24) | 0xFFFFFF);
            Area_DrawEntry(&set2->matSets[12], m[0], lights);
        }
        set2 = gAreaSubModels[0];
        if (set2 != NULL && set2->active != 0) {
            for (i = 0; i < 7; i++) {
                Mtx_Scale(m[0], gGlowScaleArea87[i], gGlowScaleArea87[i], gGlowScaleArea87[i]);
                Mtx_SetTrans(m[0], gGlowPosArea87[i][0], gGlowPosArea87[i][1], gGlowPosArea87[i][2]);
                Mtx_Concat3x3(m[0], gCamMatrix);
                Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.25f * ((as->frame >> 2) & 3), 0.0f);
                Gs_SetState(0x19, (s32)tm);
                if (i < 3) {
                    Gs_SetState(0x67, (as->look[0] << 24) | 0xFFFFFF);
                } else {
                    Gs_SetState(0x67, ((u8)func_7D96D0() << 24) | 0xFFFFFF);
                }
                Area_DrawEntry(&set2->matSets[0], m[0], 0);
            }
            if (Overlay_IsLoaded(4) == 1) {
                for (i = 0; i < 3; i++) {
                    Mtx_Translation(m[0], 6600.0f, 170.0f, 8260.0f);
                    switch (i) {
                    case 1:
                    case 2:
                        Mtx_Concat3x3(m[0], (f32 *)gCamYawMtx);
                        break;
                    }
                    Gs_SetState(0x67, LobbyWorld_GetFadeColor(i));
                    Area_DrawEntry(&set2->matSets[2] + i, m[0], 0);
                }
            }
        }
        break;
    case 0x88:
        set2 = as->model;
        if (set2 != NULL && set2->active != 0) {
            lights = set2->mats;
            Mtx_Identity(m[0]);
            Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.25f * ((as->frame >> 2) & 3), 0.0f);
            Gs_SetState(0x19, (s32)tm);
            Gs_SetState(0x67, (as->look[0] << 24) | 0xFFFFFF);
            Area_DrawEntry(&set2->matSets[11], m[0], lights);
        }
        set2 = gAreaSubModels[0];
        if (set2 != NULL && set2->active != 0) {
            for (i = 0; i < 16; i++) {
                Mtx_Scale(m[0], gGlowScaleArea88[i], gGlowScaleArea88[i], gGlowScaleArea88[i]);
                Mtx_SetTrans(m[0], gGlowPosArea88[i][0], gGlowPosArea88[i][1], gGlowPosArea88[i][2]);
                if (gGlowEntryArea88[i] == 10) {
                    Gs_SetState(0x67, (a << 24) | 0xFFFFFF);
                } else {
                    Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.25f * ((as->frame >> 2) & 3), 0.0f);
                    Gs_SetState(0x19, (s32)tm);
                    Gs_SetState(0x67, (as->look[0] << 24) | 0xFFFFFF);
                }
                Area_DrawEntry(&set2->matSets[gGlowEntryArea88[i]], m[0], 0);
            }
        }
        break;
    case 0x89:
        if (set != NULL && set->active != 0) {
            for (i = 0; i < 12; i++) {
                Mtx_Scale(m[0], gGlowScaleArea89[i], gGlowScaleArea89[i], gGlowScaleArea89[i]);
                Mtx_SetTrans(m[0], gGlowPosArea89[i][0], gGlowPosArea89[i][1], gGlowPosArea89[i][2]);
                Mtx_Concat3x3(m[0], (f32 *)gCamYawMtx);
                e = &set->matSets[gGlowEntryArea89[i]];
                if (gGlowEntryArea89[i] == 3) {
                    Gs_SetState(0x67, (a << 24) | 0xFFFFFF);
                } else {
                    Gs_SetState(0x67, (as->look[0] << 24) | 0xFFFFFF);
                    Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.25f * ((as->frame >> 2) & 3), 0.0f);
                    Gs_SetState(0x19, (s32)tm);
                }
                Area_DrawEntry(e, m[0], 0);
            }
            e = &set->matSets[7];
            Mtx_Identity(m[0]);
            Gs_SetState(0x67, (as->look[0] << 24) | 0xFFFFFF);
            Area_DrawEntry(e, m[0], 0);
        }
        {
        u8 lv = Village_FacilityLevel(2);
        Model *vset;
        switch (lv) {
        case 0:
        case 1:
        case 2:
            vset = gAreaSubModels[2];
            if (vset != NULL && vset->active != 0) {
                Mtx_Identity(m[0]);
                e = &vset->matSets[gGlowVillageEntries[lv]];
                Mtx_Translation(tm[0], 0.0f, (as->frame & 0x1F) / 32.0f, 0.0f);
                Gs_SetState(0x19, (s32)tm);
                Area_DrawEntry(e, m[0], 0);
            }
            break;
        }
        }
        break;
    case 0xAA:
    case 0xAB:
    case 0xAC:
        for (i = 0; i < 4; i++) {
            e = &set->matSets[gGlowEntryAreaAA[i]];
            Mtx_Identity(m[0]);
            Mtx_SetTrans(m[0], gGlowPosAreaAA[i][0], gGlowPosAreaAA[i][1], gGlowPosAreaAA[i][2]);
            Mtx_RotateY(m[0], gGlowRotYAreaAA[i]);
            if (gGlowEntryAreaAA[i] == 1) {
                Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.25f * ((as->frame & 0xC) >> 2), 0.0f);
                Gs_SetState(0x19, (s32)tm);
            }
            Gs_SetState(0x67, -1);
            Area_DrawEntry(e, m[0], 0);
            Draw_SetDefaultState();
        }
        break;
    case 0xAD:
        if (set != NULL && set->active != 0) {
            Mtx_Translation(m[0], 2576.0f, 298.0f, 2452.0f);
            Mtx_Concat3x3(m[0], gCamMatrix);
            e = &set->matSets[1];
            Gs_SetState(0x67, (a << 24) | 0xFFFFFF);
            Area_DrawEntry(e, m[0], 0);
            e = &set->matSets[2];
            Gs_SetState(0x67, (as->look[0] << 24) | 0xFFFFFF);
            Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.25f * ((as->frame >> 2) & 3), 0.0f);
            Gs_SetState(0x19, (s32)tm);
            Area_DrawEntry(e, m[0], 0);
        }
        break;
    case 0xAE:
        if (set != NULL && set->active != 0) {
            for (i = 0; i < 12; i++) {
                Mtx_Translation(m[0], gGlowPosAreaAE[i][0], gGlowPosAreaAE[i][1], gGlowPosAreaAE[i][2]);
                Mtx_Concat3x3(m[0], (f32 *)gCamYawMtx);
                e = &set->matSets[gGlowEntryAreaAE[i]];
                if (gGlowEntryAreaAE[i] == 1) {
                    Gs_SetState(0x1A, (s32)m);
                    Gs_SetState(0x67, (a << 24) | 0xFFFFFF);
                } else {
                    Gs_SetState(0x1A, (s32)m);
                    Gs_SetState(0x67, (as->look[0] << 24) | 0xFFFFFF);
                    Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.25f * ((as->frame >> 2) & 3), 0.0f);
                    Gs_SetState(0x19, (s32)tm);
                }
                Area_DrawEntry(e, m[0], 0);
            }
        }
        break;
    case 0xAF:
        if (set != NULL && set->active != 0) {
            for (i = 0; i < 20; i++) {
                Mtx_Translation(m[0], gGlowPosAreaAF[i][0], gGlowPosAreaAF[i][1], gGlowPosAreaAF[i][2]);
                Mtx_Concat3x3(m[0], (f32 *)gCamYawMtx);
                e = &set->matSets[gGlowEntryAreaAF[i]];
                if (gGlowEntryAreaAF[i] == 1) {
                    Gs_SetState(0x1A, (s32)m);
                    Gs_SetState(0x67, (a << 24) | 0xFFFFFF);
                } else {
                    Gs_SetState(0x1A, (s32)m);
                    Gs_SetState(0x67, (as->look[0] << 24) | 0xFFFFFF);
                    Mtx_Translation(tm[0], 0.25f * (as->frame & 3), 0.25f * ((as->frame >> 2) & 3), 0.0f);
                    Gs_SetState(0x19, (s32)tm);
                }
                Area_DrawEntry(e, m[0], 0);
            }
        }
        break;
    }
    Gs_SetState(0x6C, 1);
    Gs_SetState(0x60, 0x80);
}

/* A queued notice (20 of 0x30 bytes at gNoticeQueue, a ring: gNoticeHead the head, gNoticeCount the
 * count; Notice_Update shows one every 60 frames). */
typedef struct Notice {
    char *item;                 /* 0x00: the text of kind 0xFF (an item's name, or text) (NoticeBanner_SpawnKind4) */
    s16 unk4;                   /* 0x04 */
    s16 unk6;                   /* 0x06 */
    u8 kind;                    /* 0x08: 0xFF: an item */
    u8 id;                      /* 0x09: from gScene.noticeId */
    s16 delay;                  /* 0x0A: frames before it shows */
    char text[0x22];            /* 0x0C: a text notice (Notice_PushText; item points here) */
    u8 pad2E[0x30 - 0x2E];
} Notice;

extern s8 gNoticeCount;           /* the notices queued */
extern s8 gNoticeHead;           /* the first one */
extern s8 gNoticeWait;           /* frames until the next notice */
extern Notice gNoticeQueue[20];

void *memset(void *dst, s32 c, u32 n);
void Notice_ResetQueue(void)
{
    gNoticeCount = 0;
    gNoticeHead = 0;
    gScene.bannerNotice = 0xFF;
    gNoticeWait = 0;
    gScene.noticeId = 0;
    memset(gNoticeQueue, 0, 0x3C0);
}

/* 1 when a queued notice has id k. */
s32 Notice_IsQueued(u8 k)
{
    s16 i;

    if (gNoticeCount == 0) {
        return 0;
    }
    for (i = 0; i < gNoticeCount; i++) {
        if (gNoticeQueue[(s16)((gNoticeHead + i) % 20)].id == k) {
            return 1;
        }
    }
    return 0;
}

void NoticeBanner_Spawn(u8 a, s16 b, s16 c, u8 d);
void NoticeBanner_SpawnKind4(char *msg, u8 k);

/* Shows the first queued notice (60 frames apart, not while gScene.eventRunning is set, nor while
 * gQuest.inputLock is positive offline) once its delay has run out. */
void Notice_Update(void)
{
    Notice *e;

    if (gScene.eventRunning != 0) {
        return;
    }
    if (gQuest.inputLock > 0 && gScene.online == 0) {
        return;
    }
    if (gNoticeWait > 0) {
        gNoticeWait--;
        return;
    }
    if (gNoticeCount <= 0) {
        return;
    }
    e = &gNoticeQueue[gNoticeHead];
    if (e->delay > 0) {
        e->delay--;
        if (e->delay < 0) {
            e->delay = 0;
        }
        return;
    }
    if (e->kind == 0xFF) {
        NoticeBanner_SpawnKind4(e->item, e->id);
    } else {
        NoticeBanner_Spawn(e->kind, e->unk4, e->unk6, e->id);
    }
    if (++gNoticeHead >= 20) {
        gNoticeHead = 0;
    }
    gNoticeWait = 60;
    gNoticeCount--;
}

Obj *Obj_Alloc(s32 n);
void NoticeBanner_Task(Obj *o);

void NoticeBanner_Spawn(u8 a, s16 b, s16 c, u8 d)
{
    Obj *e = Obj_Alloc(1);

    if (e != NULL) {
        e->kind = 1;
        e->keep = 1;
        e->update = NoticeBanner_Task;
        e->type = a;
        e->var5 = b;
        e->timer = c;
        gScene.bannerNotice = d;
    }
}

/* Queues a notice of the given kind; its id, or 0xFF when the queue is full. */
u8 Notice_Push(u8 kind, s16 a, s16 b)
{
    s8 i;

    if (gNoticeCount >= 19) {
        return 0xFF;
    }
    gScene.noticeId = (gScene.noticeId + 1) & 0x7F;
    i = (gNoticeHead + gNoticeCount) % 20;
    gNoticeQueue[i].kind = kind;
    gNoticeQueue[i].unk4 = a;
    gNoticeQueue[i].id = gScene.noticeId;
    gNoticeQueue[i].delay = 0;
    gNoticeQueue[i].unk6 = b;
    gNoticeCount++;
    return gScene.noticeId;
}

Obj *Obj_Alloc(s32 n);
void NoticeBanner_Task(Obj *o);

void NoticeBanner_SpawnKind4(char *msg, u8 k)
{
    Obj *e = Obj_Alloc(1);

    if (e != NULL) {
        TextBuf *tb;

        e->kind = 1;
        e->keep = 1;
        e->update = NoticeBanner_Task;
        e->type = 4;
        tb = e->buf;
        tb->msg = msg;
        gScene.bannerNotice = k;
    }
}

/* Queues an item notice; its id, or 0xFF when the queue is full. */
u8 Notice_PushItem(char *item)
{
    s8 i;

    if (gNoticeCount >= 19) {
        return 0xFF;
    }
    gScene.noticeId = (gScene.noticeId + 1) & 0x7F;
    i = (gNoticeHead + gNoticeCount) % 20;
    gNoticeQueue[i].id = gScene.noticeId;
    gNoticeQueue[i].kind = 0xFF;
    gNoticeQueue[i].item = item;
    gNoticeQueue[i].delay = 0;
    gNoticeCount++;
    return gScene.noticeId;
}

char *strcpy(char *dst, const char *src);
u32 strlen(const char *s);
/* Queues a text notice (copied, at most 0x20 characters); its id, or 0xFF when the queue
 * is full. */
u8 Notice_PushText(char *s)
{
    s32 i;
    s32 k;

    if (gNoticeCount >= 19) {
        return 0xFF;
    }
    gScene.noticeId = (gScene.noticeId + 1) & 0x7F;
    i = (s8)((gNoticeHead + gNoticeCount) % 20);
    gNoticeQueue[i].id = gScene.noticeId;
    gNoticeQueue[i].kind = 0xFF;
    gNoticeQueue[i].item = gNoticeQueue[i].text;
    gNoticeQueue[i].delay = 0;
    if ((u32)strlen(s) < 0x20) {
        strcpy(gNoticeQueue[i].text, s);
        gNoticeQueue[i].text[0x20] = 0;
        gNoticeQueue[i].text[0x21] = 0;
    } else {
        for (k = 0; k < 0x20; k++) {
            gNoticeQueue[i].text[k] = s[k];
        }
        gNoticeQueue[i].text[0x20] = 0;
        gNoticeQueue[i].text[0x21] = 0;
    }
    gNoticeCount++;
    return gScene.noticeId;
}

void NoticeBanner_Task(Obj *o)
{
    u8 step;

    step = o->state;
    switch (step) {
    case 0:
        NoticeBanner_Start(o);
        break;
    case 1:
        NoticeBanner_Run(o);
        break;
    case 2:
        NoticeBanner_End(o);
        break;
    case 3:
        NoticeBanner_Free(o);
        break;
    }
}

s32 sprintf(char *dst, char *fmt, ...);     /* sprintf */
s32 Text_DisplayLength(char *s);
s16 DrawSlotB_Alloc(void);
DrawSlot *DrawSlotB_Get(s32 i);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void Obj_Free(Obj *p);
void TextNotice_Draw(DrawSlot *slot);
extern char *gNoticeBannerTexts[];
extern char *gNoticeBannerItemTexts[];
extern char *gNoticeBannerTextsItem[];
extern char *gNoticeBannerPlayerTexts[];
extern char *gItemNames[];
extern char *gNoticeMadeTexts[];
extern char *gNoticeMadeCountText[];

/* Starts a notice banner (kind 1, NoticeBanner_Spawn / NoticeBanner_SpawnKind4): its text by type (item names
 * gItemNames, the player's name, the messages gNoticeBannerTexts...), split into lines at newlines and
 * measured, then slid in from the top (NoticeBanner_Run) and drawn by TextNotice_Draw. */
void NoticeBanner_Start(Obj *o)
{
    TextBuf *tb = o->buf;
    char line[0x80];
    s16 w;
    s32 i;
    s32 n;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->keep = 1;
    Sound_PlaySeFlat(0, 0x10, 0);
    switch (o->type) {
    case 0:
        sprintf(tb->text, "~C00%s", gNoticeBannerTexts[o->var5]);
        break;
    case 1:
        sprintf(tb->text, "~C02%s~C00%s", gItemNames[(u16)o->timer], gNoticeBannerItemTexts[o->var5]);
        break;
    case 2:
        sprintf(tb->text, "~C00%s~C02%s", gNoticeBannerTextsItem[o->var5], gItemNames[(u16)o->timer]);
        break;
    case 3:
        sprintf(tb->text, "~C02%s~C00%s", gPlayers[o->timer].name, gNoticeBannerPlayerTexts[o->var5]);
        break;
    case 4:
        sprintf(tb->text, "~C00%s", tb->msg);
        break;
    case 5:
        sprintf(tb->text, "~C02%s~C00%s~C05%d~C00%s", gItemNames[(u16)o->var5], gNoticeMadeTexts[0], o->timer, gNoticeMadeCountText[0]);
        break;
    }
    o->var1 = 1;
    w = 0;
    for (i = 0, n = 0; i < 0x80; i++) {
        if (tb->text[i] == '\n') {
            line[n] = 0;
            tb->width[o->var1 - 1] = Text_DisplayLength(line);
            if (w < tb->width[o->var1 - 1]) {
                w = tb->width[o->var1 - 1];
            }
            o->var1++;
            n = 0;
        } else {
            line[n] = tb->text[i];
            if (line[n] == 0) {
                break;
            }
            n++;
        }
    }
    tb->width[o->var1 - 1] = Text_DisplayLength(line);
    if (w < tb->width[o->var1 - 1]) {
        w = tb->width[o->var1 - 1];
    }
    o->pos[0] = 320.0f - 10.0f * (w / 2.0f);
    o->pos[1] = -20.0f;
    o->pos[2] = w;
    o->fvar = 40.0f;
    o->var3 = 60;
    o->slot = DrawSlotB_Alloc();
    if (o->slot != -1) {
        o->ds = DrawSlotB_Get(o->slot);
        o->ds->owner = o;
        o->ds->draw = TextNotice_Draw;
        NoticeBanner_Run(o);
    } else {
        Obj_Free(o);
    }
}

s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);

/* A notice banner's run state: it slides down to y 224, stays for its time (shorter once more
 * notices wait), then slides up out of the screen and ends. */
u32 NoticeBanner_Run(Obj *o)
{
    switch (o->var0) {
    case 0:
        if ((o->pos[1] += o->fvar) >= 224.0f) {
            o->var0++;
            o->pos[1] = 224.0f;
        }
        break;
    case 1:
        o->var3--;
        if (o->var3 <= 0 || (o->var3 <= 30 && gNoticeCount > 0)) {
            o->fvar *= -1.0f;
            gScene.bannerNotice = 0xFF;
            o->var0++;
        }
        break;
    case 2:
        if ((o->pos[1] += o->fvar) <= -20.0f) {
            o->state++;
        }
        break;
    }
    return OT_AddByDepth(gDrawOtText3, o->ds, 0x10, 1);
}

void NoticeBanner_End(Obj *o)
{
    o->state++;
    o->visible = 0;
    DrawSlotB_Free(o->slot);
}

void NoticeBanner_Free(Obj *o)
{
    Obj_Free(o);
}

struct Frame;
void WindowFrame_Draw(struct Frame *b, s16 *off);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Text_SetSize(s32 size);
void Text_Printf(const char *fmt, ...);

/* Draws a text notice (its draw slot's update): a window sized to its lines (object +6) and
 * width (the object's z), then each line of the text (buffer +7, lines split at '\n') centered by
 * its width (buffer +4). */
void TextNotice_Draw(DrawSlot *slot)
{
    Obj *o = slot->owner;
    TextBuf *tb = o->buf;
    Frame box;
    char line[0x80];
    s32 n;
    s32 pos;
    s32 i;

    Gs_SetState(0x60, 0);
    box.x = o->pos[0] - 4.0f;
    box.y = o->pos[1] - 2.0f;
    box.w = (s16)o->pos[2] * 10 + 8;
    box.h = o->var1 * 22 + 4;
    box.style = 0x80000001;
    box.color = 0xFF7F7052;
    box.color2 = 0xBD101010;
    WindowFrame_Draw(&box, 0);
    Font_SetSpacing(0x14, 0x14);
    Text_SetSize(3);
    pos = 0;
    for (n = 0; n < o->var1; n++) {
        for (i = 0; pos < 0x80; pos++, i++) {
            if (tb->text[pos] == '\n') {
                line[i] = 0;
                pos++;
                break;
            }
            line[i] = tb->text[pos];
            if (line[i] == 0) {
                break;
            }
        }
        Font_SetPos(320.0f - 10.0f * (tb->width[n] / 2.0f), (s16)o->pos[1] + n * 20);
        Text_Printf("%s", line);
    }
    Gs_SetState(0x60, 0x80);
}

void AreaProp_Task(Obj *o);

/* Starts an object of kind 6 (update AreaProp_Task) of type k. */
void Effect06_Start(s16 k)
{
    Obj *o = Obj_Alloc(0);

    if (o != NULL) {
        o->kind = 6;
        o->type = k;
        o->update = AreaProp_Task;
    }
}

void AreaProp_Task(Obj *o)
{
    u8 step;

    step = o->state;
    switch (step) {
    case 0:
        AreaProp_Start(o);
        break;
    case 1:
        AreaProp_Run(o);
        break;
    case 2:
        AreaProp_End(o);
        break;
    case 3:
        AreaProp_Free(o);
        break;
    }
}

extern f32 gAreaPropPosArea56[];
extern f32 gAreaPropPosArea83[][3];
extern f32 gAreaPropPosArea84[][3];
extern f32 gAreaPropPosArea85[][3];
extern f32 gAreaPropPosArea88[][3];
extern f32 gAreaPropPosArea89[][3];
extern f32 gAreaPropPosAreaAA[];
extern f32 gAreaPropPosAreaAE[];
extern f32 gAreaPropPosAreaAF[];
u16 Rand_Next(s32 a);
void Vec_Copy(f32 *dst, f32 *src);
void AreaProp_Draw(DrawSlot *slot);

/* Starts an object of kind 6 (Effect06_Start): placed by area and type from the tables above,
 * with a timer in some areas, and a draw slot drawn by AreaProp_Draw; freed in other areas. */
void AreaProp_Start(Obj *o)
{
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->var3 = 0;
    o->timer = 0;
    o->fvar = 1.0f;
    switch (gScene.area) {
    case 0x56:
    case 0x57:
        o->pos[0] = gAreaPropPosArea56[0];
        o->pos[1] = gAreaPropPosArea56[1];
        o->pos[2] = gAreaPropPosArea56[2];
        break;
    case 0x83:
        Vec_Copy(o->pos, gAreaPropPosArea83[o->type]);
        switch (o->type) {
        case 6:
            o->var3 = 800;
            break;
        default:
            o->var3 = (Rand_Next(1) & 0x7F) + 200;
            break;
        }
        break;
    case 0x84:
        Vec_Copy(o->pos, gAreaPropPosArea84[o->type]);
        o->var3 = (Rand_Next(1) & 0x7F) + 200;
        break;
    case 0x85:
        o->fvar = 0.99f;
        Vec_Copy(o->pos, gAreaPropPosArea85[o->type]);
        break;
    case 0x87:
        o->fvar = 0.0f;
        o->var0 = 0;
        break;
    case 0x88:
        Vec_Copy(o->pos, gAreaPropPosArea88[o->type]);
        o->var3 = (Rand_Next(1) & 0x7F) + 200;
        break;
    case 0x89:
        Vec_Copy(o->pos, gAreaPropPosArea89[o->type]);
        break;
    case 0xAA:
    case 0xAB:
    case 0xAC:
        Vec_Copy(o->pos, &gAreaPropPosAreaAA[o->type]);
        break;
    case 0xAE:
        Vec_Copy(o->pos, &gAreaPropPosAreaAE[o->type]);
        break;
    case 0xAF:
        Vec_Copy(o->pos, &gAreaPropPosAreaAF[o->type]);
        break;
    default:
        AreaProp_Free(o);
        return;
    }
    o->slot = DrawSlot_Alloc();
    o->ds = DrawSlot_Get(o->slot);
    if (o->ds != NULL) {
        o->ds->owner = o;
        o->ds->draw = AreaProp_Draw;
        AreaProp_Run(o);
    } else {
        AreaProp_Free(o);
    }
}

s32 Motion_PassedFrame(void *obj, s32 layer, f32 frame);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void Vec_Scale(f32 *out, f32 *in, f32 s);
void Vec_Copy(f32 *dst, f32 *src);
void Mtx_GetTrans(f32 *out, f32 *m);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
extern f32 gCamMatrix[];

/* The run state of a kind 6 object: by area, it hides at a frame of the local player's motion,
 * cycles its timers, or (area 0x87) fades in and out in front of the camera; then it is drawn in
 * the layer its +0x1E picks. */
void AreaProp_Run(Obj *o)
{
    Player *p = &gPlayers[gScene.localPlayer];
    f32 v[3];
    f32 t[3];

    o->timer++;
    o->var4 = 1;
    switch (gScene.area) {
    case 0x56:
    case 0x57:
        if (p->motionId[0] == 0x287 && Motion_PassedFrame(p, 0, 232.0f)) {
            o->visible = 0;
        }
        break;
    case 0x83:
        switch (o->type) {
        case 6:
            o->var4 = 2;
        default:
            if (o->timer >= o->var3) {
                o->timer = 0;
            }
            break;
        }
        break;
    case 0x84:
        switch (o->type) {
        case 5:
            o->var4 = 2;
            if (++o->var3 >= 800) {
                o->var3 = 0;
            }
            break;
        default:
            if (o->timer >= o->var3) {
                o->timer = 0;
            }
            break;
        }
        break;
    case 0x85:
        o->var4 = 0;
        break;
    case 0x87:
        switch (o->var0) {
        case 0:
            if ((o->fvar += 0.022222223f) >= 1.0f) {
                o->var0++;
                o->fvar = 1.0f;
                o->timer = 0;
            }
            break;
        case 1:
            if (++o->timer > 6) {
                o->var0++;
            }
            break;
        case 2:
            if ((o->fvar -= 0.022222223f) <= 0.0f) {
                o->state++;
                o->fvar = 0.0f;
                o->timer = 0;
            }
            break;
        }
        if (o->fvar == 0.0f) {
            return;
        }
        Vec_Copy(v, gCamDir);
        Vec_Scale(v, v, -30.0f);
        Mtx_GetTrans(t, gCamMatrix);
        Vec_Add(o->pos, v, t);
        o->var4 = 3;
        break;
    case 0x88:
        if (o->timer >= o->var3) {
            o->timer = 0;
        }
        break;
    case 0x89:
        o->var4 = 2;
        break;
    case 0xAC:
        if (p->motionId[0] == 0xFA && Motion_PassedFrame(p, 0, 154.0f)) {
            o->visible = 0;
        }
        break;
    }
    o->ds->pos[0] = o->pos[0];
    o->ds->pos[1] = o->pos[1];
    o->ds->pos[2] = o->pos[2];
    switch (o->var4) {
    default:
        OT_AddByDepth(gDrawOtSorted, o->ds, 0x40, 0);
        break;
    case 1:
        OT_AddByDepth(gDrawOt32, o->ds, 0x20, 0);
        break;
    case 2:
        OT_AddByDepth(gDrawOt8, o->ds, 8, 1);
        break;
    case 3:
        OT_AddByDepth(gDrawListStage, o->ds, 1, 0);
        break;
    }
}

void AreaProp_End(Obj *o)
{
    o->state++;
    o->visible = 0;
    DrawSlot_Free(o->slot);
}

void AreaProp_Free(Obj *o)
{
    Obj_Free(o);
}

/* The color of the notice banners: white by day, grey 0x30 at night (look[0] 0xFF), else darker
 * by the night level (255 - 207 x v / 255). */
u32 Notice_GetColor(void)
{
    AreaState *as = &gAreaState;
    u8 r, g, b;
    f32 f;

    if (gAreaState.look[0] == 0) {
        b = g = r = 0xFF;
    } else if (as->look[0] == 0xFF) {
        b = g = r = 0x30;
    } else {
        f = 255.0f - 207.0f * (as->look[0] / 255.0f);
        r = f;
        g = f;
        b = f;
    }
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

extern f32 gAreaPropRotArea56[];
extern f32 gAreaPropScaleArea83[];
extern s16 gAreaPropEntryArea83[];
extern s16 D_004492D2[];
extern f32 gAreaPropScaleArea84[];
extern s16 gAreaPropEntryArea84[];
extern s16 D_0044934A[];
extern s16 gAreaPropEntryArea85[];
extern f32 gAreaPropScaleArea88[];
extern s16 gAreaPropEntryArea88[];
void Mtx_Identity(f32 *m);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_RotateZ(f32 *m, f32 a);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Copy(f32 *dst, f32 *src);
u8 Village_FacilityLevel(s32 k);
void Draw_SetDefaultState(void);

#define DEG2RAD(d) (6.2831855f * (d) / 360.0f)

/* Draws an area prop (AreaProp_Start's draw slot): its models by area and type. */
void AreaProp_Draw(DrawSlot *slot)
{
    f32 m[16];
    f32 uv[16];
    AreaState *as;
    Model *set;
    s16 i;
    ModelMatSet *e;
    ModelMat *lights;
    s16 k;
    Obj *o;

    o = slot->owner;
    set = gAreaSubModels[0];
    as = &gAreaState;
    if (o->visible == 0) {
        return;
    }
    Mtx_Identity(m);
    switch (gScene.area) {
    case 0x56:
    case 0x57:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        Gs_SetState(0x1A, (s32)m);
        e = &set->matSets[1];
        Gs_SetState(0x60, 0x80);
        Gs_SetState(0x67, -1);
        if (e != NULL && e->id != -1) {
            Draw_ApplyStates(e->flags);
            Model_Draw(e->id, 0);
        }
        Mtx_RotateXYZ(m, gAreaPropRotArea56[0], gAreaPropRotArea56[1], gAreaPropRotArea56[2]);
        e = &set->matSets[o->type];
        break;
    case 0x83:
        set = *(Model **)((u8 *)as + 0x3C);
        if (set == NULL || set->active == 0) {
            goto end;
        }
        lights = set->mats;
        e = &set->matSets[gAreaPropEntryArea83[o->type]];
        Gs_SetLightList(lights, e);
        Mtx_Scale(m, gAreaPropScaleArea83[o->type], gAreaPropScaleArea83[o->type], gAreaPropScaleArea83[o->type]);
        switch (o->type) {
        case 6:
            Mtx_SetTrans(m, o->pos[0], o->pos[1], o->pos[2]);
            Gs_SetState(0x1A, (s32)m);
            Gs_SetState(0x67, Notice_GetColor());
            Gs_SetState(0x60, 0);
            Mtx_Translation(uv, (f32)o->timer / (f32)o->var3, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32)uv);
            if (e != NULL && e->id != -1) {
                Draw_ApplyStates(e->flags);
                Model_Draw(e->id, 0);
            }
            e = &set->matSets[D_004492D2[o->type]];
            Gs_SetLightList(lights, e);
            Gs_SetState(0x67, -1);
            break;
        default:
            Mtx_RotateY(m, DEG2RAD(360.0f * (1.0f - (f32)o->timer / (f32)o->var3)));
            break;
        }
        break;
    case 0x84:
        set = *(Model **)((u8 *)as + 0x3C);
        if (set == NULL || set->active == 0) {
            goto end;
        }
        lights = set->mats;
        e = &set->matSets[gAreaPropEntryArea84[o->type]];
        Gs_SetLightList(lights, e);
        Mtx_Scale(m, gAreaPropScaleArea84[o->type], gAreaPropScaleArea84[o->type], gAreaPropScaleArea84[o->type]);
        switch (o->type) {
        case 5:
            Mtx_SetTrans(m, o->pos[0], o->pos[1], o->pos[2]);
            Gs_SetState(0x1A, (s32)m);
            Gs_SetState(0x67, Notice_GetColor());
            Gs_SetState(0x60, 0);
            Mtx_Translation(uv, (f32)o->var3 / 800.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32)uv);
            if (e != NULL && e->id != -1) {
                Draw_ApplyStates(e->flags);
                Model_Draw(e->id, 0);
            }
            e = &set->matSets[D_0044934A[o->type]];
            Gs_SetLightList(lights, e);
            Mtx_Translation(uv, (f32)(o->timer & 0x3FF) / 1024.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32)uv);
            break;
        default:
            Mtx_RotateY(m, DEG2RAD(360.0f * (1.0f - (f32)o->timer / (f32)o->var3)));
            break;
        }
        break;
    case 0x85:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        e = &set->matSets[gAreaPropEntryArea85[o->type]];
        if (o->type == 0) {
            Mtx_Translation(uv, 0.0f, (f32)(o->timer & 0x7F) / 128.0f, 0.0f);
        } else {
            Mtx_Translation(uv, 0.5f * (f32)(o->timer & 1), 0.25f * (f32)((o->timer & 7) >> 1), 0.0f);
        }
        Gs_SetState(0x19, (s32)uv);
        Gs_SetState(0x6C, 0);
        break;
    case 0x87:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        e = &set->matSets[1];
        Mtx_Copy(m, gCamMatrix);
        Gs_SetState(0x6C, 0);
        Gs_SetState(0x6D, 7);
        Gs_SetState(0x67, ((u8)(255.0f * o->fvar) << 24) | 0xFFFFFF);
        break;
    case 0x88:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        e = &set->matSets[gAreaPropEntryArea88[o->type]];
        Mtx_Scale(m, gAreaPropScaleArea88[o->type], gAreaPropScaleArea88[o->type], gAreaPropScaleArea88[o->type]);
        Mtx_RotateY(m, DEG2RAD(360.0f * (1.0f - (f32)o->timer / (f32)o->var3)));
        Gs_SetState(0x67, Notice_GetColor());
        break;
    case 0x89:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        Mtx_Translation(uv, 0.0f, (f32)(s32)*(u8 *)&o->timer / 256.0f, 0.0f);
        Gs_SetState(0x19, (s32)uv);
        Mtx_SetTrans(m, o->pos[0], o->pos[1], o->pos[2]);
        Gs_SetState(0x1A, (s32)m);
        Gs_SetState(0x60, 0);
        for (i = 0; i < 2; i++) {
            e = &set->matSets[i];
            if (e != NULL && e->id != -1) {
                Draw_ApplyStates(e->flags);
                Model_Draw(e->id, 0);
            }
        }
        switch ((u8)Village_FacilityLevel(1)) {
        case 1:
            k = 4;
            break;
        case 2:
            k = 5;
            break;
        default:
            goto end;
        }
        if (gAreaSubModels[1] == NULL || gAreaSubModels[1]->active == 0) {
            goto end;
        }
        e = &gAreaSubModels[1]->matSets[k];
        Mtx_Translation(uv, 0.0f, (f32)(o->timer & 0x3F) / 64.0f, 0.0f);
        Gs_SetState(0x19, (s32)uv);
        if (e != NULL && e->id != -1) {
            Draw_ApplyStates(e->flags);
            Model_Draw(e->id, 0);
        }
        Draw_SetDefaultState();
        goto end;
    case 0xAA:
    case 0xAB:
    case 0xAC:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        e = &set->matSets[5];
        Mtx_RotateX(m, -0.40142572f);
        Mtx_RotateY(m, 2.8274333f);
        Mtx_RotateZ(m, -1.2217306f);
        break;
    case 0xAE:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        e = &set->matSets[18];
        Mtx_RotateX(m, -1.2566371f);
        Mtx_RotateZ(m, 1.5707964f);
        break;
    case 0xAF:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        e = &set->matSets[21];
        Mtx_RotateX(m, -1.2566371f);
        Mtx_RotateZ(m, 1.5707964f);
        break;
    default:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        Gs_SetState(0x67, -1);
        e = &set->matSets[o->type];
        break;
    }
    Mtx_SetTrans(m, o->pos[0], o->pos[1], o->pos[2]);
    Gs_SetState(0x1A, (s32)m);
    switch (o->var4) {
    case 0:
    case 2:
    case 3:
    default:
        Gs_SetState(0x60, 0);
        break;
    case 1:
        Gs_SetState(0x60, 0x80);
        break;
    }
    if (e != NULL && e->id != -1) {
        Draw_ApplyStates(e->flags);
        Model_Draw(e->id, 0);
    }
end:
    Draw_SetDefaultState();
    Gs_SetState(0x6C, 1);
    Gs_SetState(0x6D, 3);
}

s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void SoundObj_Task(Obj *o);

/* Sound se of bank at pos (unless quiet) and an object of kind 0xC (update SoundObj_Task) there,
 * of type k, that lasts n frames. */
void SoundObj_Start(s32 bank, s32 se, s32 k, f32 *pos, s16 n, u8 quiet)
{
    Obj *o;

    if (quiet == 0) {
        Sound_PlaySeAtPosChecked(bank, se, 0, pos, k, 0);
    }
    o = Obj_Alloc(0);
    if (o != NULL) {
        o->kind = 0xC;
        o->update = SoundObj_Task;
        o->type = k;
        o->var0 = quiet;
        o->var1 = bank;
        o->var2 = se;
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2];
        o->timer = n;
    }
}

void SoundObj_Task(Obj *o)
{
    u8 step;

    step = o->state;
    switch (step) {
    case 0:
        SoundObj_Init(o);
        break;
    case 1:
        SoundObj_Run(o);
        break;
    case 2:
        SoundObj_End(o);
        break;
    case 3:
        SoundObj_Free(o);
        break;
    }
}

void SoundObj_Init(Obj *o)
{
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    if (o->var0 == 0) {
        o->var3 = 0;
        return;
    }
    o->var3 = o->timer;
}

f32 Seg_NearestPointDist(f32 *out, f32 *a, f32 *b, f32 *p);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
f32 Math_Absf(f32 x);
f32 Vec_Dot(f32 *a, f32 *b);
s32 Seg_ClipPlane(f32 *s, f32 *p, f32 *nrm, f32 *out);
void Mtx_GetTrans(f32 *out, f32 *m);
u16 Area_DayId(u16 a);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void Vec_Copy(f32 *dst, f32 *src);
u16 Rand_Next(s32 a);
extern f32 gRiverPtsDay48[];
extern f32 gRiverPtsDay4B[];
extern f32 gRiverPtsDay6E[];
extern f32 gRiverPtsDay71[];
extern f32 gRiverPtsDay72[];
extern f32 gRiverPtsDay78[];
extern f32 gRiverPtsDayA1[];
extern f32 gCamMatrix[];


/* The run state of a sound object (kind 0xC, SoundObj_Start): type 0 plays sound unk06/unk07 every
 * 5 frames for unk08 frames (0xFFFF: forever); types 1 and 3 every unk08 frames (1: +-16 at
 * random) at the point of the area's river (three points by area) nearest to the camera; type 2
 * every unk08 frames. */
void SoundObj_Run(Obj *o)
{
    f32 cam[3];
    f32 d[3];
    f32 out[3];
    f32 e[3];
    f32 pts[2][3];
    HitCapsule cap;
    f32 *seg;
    f32 *q;
    f32 da;
    f32 db;
    s32 near;
    u8 t;

    switch (o->var0) {
    case 0:
        if ((u16)o->timer != 0xFFFF && --o->timer <= 0) {
            o->state++;
            break;
        }
        o->var3++;
        if (o->var3 >= 5) {
            o->var3 = 0;
            Sound_PlaySeAtPosChecked(o->var1, o->var2, 0, o->pos, o->type, 1);
        }
        break;
    case 1:
    case 3:
        if (--o->var3 <= 0) {
            if (o->var0 == 1) {
                o->var3 = o->timer + (s16)((Rand_Next(1) & 0x1F) - 16);
            } else {
                o->var3 = o->timer;
            }
            switch (Area_DayId(gScene.area)) {
            case 0x48:
            case 0x9E:
                seg = gRiverPtsDay48;
                break;
            case 0x4B:
            case 0xA0:
                seg = gRiverPtsDay4B;
                break;
            case 0x6E:
                seg = gRiverPtsDay6E;
                break;
            case 0x71:
                seg = gRiverPtsDay71;
                break;
            case 0x72:
                seg = gRiverPtsDay72;
                break;
            case 0x78:
                seg = gRiverPtsDay78;
                break;
            case 0xA1:
                seg = gRiverPtsDayA1;
                break;
            default:
                o->state++;
                return;
            }
            Mtx_GetTrans(cam, gCamMatrix);
            da = Seg_NearestPointDist(pts[0], seg, seg + 3, cam);
            q = pts[1];
            db = Seg_NearestPointDist(q, seg + 3, seg + 6, cam);
            Vec_Diff(d, pts[0], q);
            if (Math_Absf(d[0]) < 0.001f && Math_Absf(d[1]) < 0.001f && Math_Absf(d[2]) < 0.001f) {
                Vec_Diff(d, seg, seg + 6);
                Vec_Diff(e, cam, seg + 3);
                if (Vec_Dot(d, e) >= 0.0f) {
                    t = 0;
                } else {
                    t = 1;
                }
            } else if (da <= db) {
                t = 0;
            } else {
                t = 1;
            }
            near = t;
            cap.radius = 0.0f;
            if (!near) {
                Vec_Copy(cap.a, seg);
                Vec_Copy(cap.b, seg + 3);
            } else {
                Vec_Copy(cap.a, seg + 3);
                Vec_Copy(cap.b, seg + 6);
            }
            if (Seg_ClipPlane(cap.a, cam, d, out)) {
                Vec_Copy(o->pos, out);
            } else if (!near) {
                Vec_Copy(o->pos, seg);
            } else {
                Vec_Copy(o->pos, seg + 6);
            }
            Sound_PlaySeAtPosChecked(o->var1, o->var2, 0, o->pos, o->type, 0);
        }
        break;
    case 2:
        o->var3++;
        if (o->var3 >= o->timer) {
            o->var3 = 0;
            Sound_PlaySeAtPosChecked(o->var1, o->var2, 0, o->pos, o->type, 0);
        }
        break;
    }
}

void SoundObj_End(Obj *o)
{
    o->state++;
    o->visible = 0;
}

void SoundObj_Free(Obj *o)
{
    Obj_Free(o);
}

/* An object from the pool: one block for kinds 8, 0xA, 0xB, 0xD and 0xE, none otherwise. */
Obj *AreaObj_AllocForKind(s32 k)
{
    switch ((s16) k) {
    default:
        return Obj_Alloc(0);
    case 8:
    case 0xA:
    case 0xB:
    case 0xD:
    case 0xE:
        return Obj_Alloc(1);
    }
}

Obj *AreaObj_AllocForKind(s32 k);
void AmbientSound_Task(Obj *);

void AmbientSound_Spawn(s32 v)
{
    Obj *o = AreaObj_AllocForKind(v);

    if (o != NULL) {
        o->kind = 0xD;
        o->type = v;
        o->update = AmbientSound_Task;
    }
}

void AmbientSound_Task(Obj *p)
{
    u8 step;

    step = p->state;
    switch (step) {
    case 0:
        AmbientSound_Setup(p);
        break;
    case 1:
        AmbientSound_Update(p);
        break;
    case 2:
        AmbientSound_End(p);
        break;
    case 3:
        AmbientSound_Free(p);
        break;
    }
}

/* A sound source of a sound object's buffer (types 8 and 14), 0x14 bytes. */
typedef struct SoundSrc {
    f32 pos[3];                 /* 0x00 */
    f32 unk0C;                  /* 0x0C: AmbientSound_AreaValue0C */
    f32 unk10;                  /* 0x10: AmbientSound_AreaValue10 */
} SoundSrc;

/* The buffer of the types 10, 11 and 13: the camera's position, then points. */
typedef struct SoundPts {
    u8 pad0[4];
    f32 cam[3];                 /* 0x04 */
    f32 pts[1][3];              /* 0x10 */
} SoundPts;

s16 Area_MonsterState(u16 area);
f32 AmbientSound_AreaValue0C(u8 i, s16 n);
f32 AmbientSound_AreaValue10(u8 i, s16 n);
s16 Area_BoundaryZone(Player *p);
void AmbientSound_Draw(DrawSlot *slot);
extern s16 gAmbSoundCounts[];

/* Sets up an ambient sound object (state 0): its volume by the area and the player's position,
 * its sources by its type (gAmbSoundCounts: their count), and a draw slot that runs AmbientSound_Draw.
 * The zone switch's `default: case 0:` keeps the original's second `li v0, 2`. */
void AmbientSound_Setup(Obj *o)
{
    Player *p = &gPlayers[gScene.localPlayer];
    s16 i;
    s16 n;
    SoundSrc *src;
    SoundPts *pts;
    f32 *pt;
    s16 count;

    o->state++;
    o->visible = 1;
    o->timer = 0;
    o->var3 = 0;
    switch (gScene.area) {
    case 0x30:
    case 0x31:
    case 0x49:
    case 0x4B:
    case 0x64:
    case 0x6D:
    case 0x77:
    case 0x9F:
    case 0xA8:
        o->fvar = 1.0f;
        break;
    case 0x22:
    case 0xBB:
        o->fvar = 0.1f;
        break;
    case 0x23:
    case 0xBC:
        o->fvar = 0.3f;
        break;
    case 0x34:
        if (p->pos[0] > 3000.0f && p->pos[0] < 15000.0f && p->pos[2] > 3000.0f && p->pos[2] < 8000.0f) {
            o->fvar = 0.0f;
        } else {
            o->fvar = 1.0f;
        }
        break;
    case 0x37:
        if (p->pos[0] > 0.0f && p->pos[0] < 6200.0f && p->pos[2] > 0.0f && p->pos[2] < 9300.0f) {
            o->fvar = 0.0f;
        } else {
            o->fvar = 1.0f;
        }
        break;
    case 0x39:
        if (p->pos[2] > 100.0f && p->pos[2] < 10400.0f) {
            o->fvar = 1.0f;
        } else {
            o->fvar = 0.0f;
        }
        break;
    case 0x1D:
    case 0x2F:
    case 0x33:
    case 0x8D:
    case 0x91:
        if (gScene.eventRunning == 1) {
            o->var1 = 2;
            o->fvar = 1.0f;
        } else {
            switch (Area_BoundaryZone(p)) {
            default:
            case 0:
                o->var1 = 2;
                o->fvar = 1.0f;
                break;
            case 2:
                o->var1 = 3;
                o->fvar = 0.0f;
                break;
            case 1:
                o->var1 = 0;
                o->fvar = 1.0f;
                break;
            }
            if (gScene.area == 0x91 && p->pos[2] < 2200.0f) {
                o->fvar = 0.0f;
            }
        }
        break;
    case 0x59:
        if (p->pos[0] < 8832.0f || p->pos[2] > 10675.0f) {
            o->fvar = 0.0f;
        } else {
            o->fvar = 1.0f;
        }
        break;
    case 0x5A:
        if (p->pos[2] > 11000.0f) {
            o->fvar = 0.0f;
        } else {
            o->fvar = 1.0f;
        }
        break;
    case 0x5B:
        if (p->pos[0] < 8437.0f || p->pos[2] > 11400.0f) {
            o->fvar = 0.0f;
        } else {
            o->fvar = 1.0f;
        }
        break;
    case 0x36:
    case 0x94:
        if (p->pos[0] > 6916.0f || p->pos[2] < 5555.0f) {
            o->fvar = 0.0f;
        } else {
            o->fvar = 1.0f;
        }
        break;
    default:
        o->fvar = 0.4f;
        break;
    }
    count = gAmbSoundCounts[o->type];
    switch (o->type) {
    case 8:
    case 14:
        src = o->buf;
        n = Area_MonsterState(gScene.area);
        for (i = 0; i < count; i++) {
            src->pos[0] = 0.0f;
            src->pos[1] = 0.0f;
            src->pos[2] = 0.0f;
            src->unk0C = AmbientSound_AreaValue0C(i, n);
            src->unk10 = AmbientSound_AreaValue10(i, n);
            src++;
        }
        break;
    case 10:
    case 11:
    case 13:
        pts = o->buf;
        pt = pts->pts[0];
        Mtx_GetTrans(pts->cam, gCamMatrix);
        n = Area_MonsterState(gScene.area);
        for (i = 0; i < count; i++) {
            pt[0] = 0.0f;
            pt[1] = 0.0f;
            pt[2] = AmbientSound_AreaValue10(i, n);
            pt += 3;
        }
        break;
    case 12:
        o->fvar = 0.0f;
        break;
    default:
        o->var3 = Rand_Next(1) & 0x7FF;
        break;
    }
    o->draw = NULL;
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->ds = DrawSlot_Get(o->slot);
        o->ds->owner = o;
        o->ds->draw = AmbientSound_Draw;
        AmbientSound_Update(o);
    } else {
        Obj_Free(o);
    }
}

void Vec_FromCamera(f32 *out, f32 *v, f32 s);
s32 Area_LineBlocked(f32 *pos);
s32 Cyl_Contains(f32 *p, f32 *c, f32 r, f32 top, f32 bottom);
f32 Math_Acos(f32 x);
f32 Math_Asin(f32 x);
f32 Math_Sqrtf(f32 x);
f32 Math_Absf(f32 x);
f32 Math_Floor(f32 x);
f32 Vec_Length(f32 *v);
f32 Vec_Dot(f32 *a, f32 *b);
void Vec_Normalize(f32 *v);
void UvScroll_Update(struct UvScroll *h, s16 n, s16 *speed, u16 mode);
extern f32 gCamPos[3];
extern s16 D_00449F00[];
extern s16 *D_00449F20[];

/* Updates an ambient sound object (state 1): its volume (+0x30) by its type, the area and the
 * hunter's position, its position around the camera (the draw slot's), and adds the slot to the
 * ordering table. Type 1 reads its distances and limits unset outside areas 0x22 and 0xBB (as
 * the original does); the 0x30/0x31 case of type 0 falls through into 0x77's. The zone
 * switches' `default: case 0:` keeps the original's second `li v0, 2`. */
void AmbientSound_Update(Obj *o)
{
    Player *p = &gPlayers[gScene.localPlayer];
    s16 count;
    s16 lim;
    f32 s[3];
    f32 t[3];
    f32 v[3];
    u8 blocked;
    f32 a, b, r, rr, mx, mn, dx, dz, d2, d, c, f, tg;
    SoundSrc *src;
    s16 i;
    s16 n;
    s16 k;
    SoundPts *pts;
    f32 *pt;
    s16 *sp;
    u16 ang;
    u16 aa;

    o->timer++;
    if (o->type != 5) {
        s[0] = gAreaState.skyPos[0];
        s[1] = gAreaState.skyPos[1];
        s[2] = gAreaState.skyPos[2];
    }
    count = gAmbSoundCounts[o->type];
    switch (o->type) {
    case 0:
        v[0] = s[0] - gCamPos[0];
        v[1] = s[1] - gCamPos[1];
        v[2] = s[2] - gCamPos[2];
        blocked = Area_LineBlocked(s);
        Vec_Normalize(v);
        switch (gScene.area) {
        case 0x34:
            if (p->pos[0] > 3000.0f && p->pos[0] < 15000.0f && p->pos[2] > 3000.0f && p->pos[2] < 8000.0f) {
                blocked = 1;
            }
            break;
        case 0x37:
            if (p->pos[0] > 0.0f && p->pos[0] < 6200.0f && p->pos[2] > 0.0f && p->pos[2] < 9300.0f) {
                blocked = 1;
            }
            break;
        case 0x39:
            if (p->pos[2] <= 100.0f || p->pos[2] >= 10400.0f) {
                blocked = 1;
            }
            break;
        case 0x59:
            if (p->pos[0] < 8832.0f || p->pos[2] > 10675.0f) {
                blocked = 1;
            }
            break;
        case 0x5A:
            if (p->pos[2] > 11000.0f) {
                blocked = 1;
            }
            break;
        case 0x5B:
            if (p->pos[0] < 8437.0f || p->pos[2] > 11400.0f) {
                blocked = 1;
            }
            break;
        case 0x36:
        case 0x94:
            if (p->pos[0] > 6916.0f || p->pos[2] < 5555.0f) {
                blocked = 1;
            }
            break;
        }
        if (blocked) {
            if ((o->fvar -= 0.07f) < 0.0f) {
                o->fvar = 0.0f;
            }
        } else {
            if ((o->fvar += 0.07f) > 1.0f) {
                o->fvar = 1.0f;
            }
        }
        Vec_FromCamera(t, v, 100.0f);
        switch (gScene.area) {
        case 0x30:
        case 0x31:
            if (o->timer >= 400) {
                o->timer = 0;
            }
            /* fallthrough */
        case 0x77:
            if (o->timer >= 200) {
                o->timer = 0;
            }
            break;
        case 0x35:
        case 0x93:
            if (o->timer >= 100) {
                o->timer = 0;
            }
            break;
        case 0x38:
        case 0x96:
            if (o->timer >= 100) {
                o->timer = 0;
            }
            break;
        }
        break;
    case 1:
        Vec_Copy(v, gCamDir);
        Vec_FromCamera(t, v, -50.0f);
        switch (gScene.area) {
        case 0x22:
        case 0xBB:
            lim = 500;
            mx = 0.4f;
            mn = 0.1f;
            r = 2000.0f;
            rr = 4000000.0f;
            dx = 9600.0f - gCamPos[0];
            dz = 10180.0f - gCamPos[2];
            break;
        default:
            lim = 0x400;
            break;
        }
        if (o->timer >= lim) {
            o->timer = 0;
        }
        /* the game reads registers it never set: dx, dz, rr, r, mx and mn are set only for areas 0x22 and 0xBB */
        d2 = dx * dx + dz * dz;
        if (d2 <= rr) {
            o->var0 = 0;
        } else {
            d = Math_Sqrtf(d2);
            a = Math_Asin(r / d);
            c = (-v[0] * dx + -v[2] * dz) / (d * Math_Sqrtf(v[0] * v[0] + v[2] * v[2]));
            if (c <= -1.0f) {
                c = -1.0f;
            } else if (c >= 1.0f) {
                c = 1.0f;
            }
            b = Math_Acos(c);
            o->var0 = 1;
        }
        if (o->var0 == 0) {
            if ((o->fvar += 0.02f) > mx) {
                o->fvar = mx;
            }
        } else {
            f = mx + mn - mx * (b - a) / 3.1415927f;
            if (f > mx) {
                f = mx;
            }
            if (o->fvar > f) {
                if ((o->fvar -= 0.02f) < f) {
                    o->fvar = f;
                }
            } else {
                o->fvar = f;
            }
        }
        break;
    case 2:
        if (o->timer >= 400) {
            o->timer = 0;
        }
        Vec_Copy(v, gCamDir);
        Vec_FromCamera(t, v, -30.0f);
        break;
    case 3:
        o->var3++;
        switch (gScene.area) {
        case 0x36:
            lim = 1600;
            k = 700;
            break;
        default:
            lim = 800;
            k = 300;
            break;
        }
        if (o->timer >= lim) {
            o->timer = 0;
        }
        if (o->var3 >= k) {
            o->var3 = 0;
        }
        Vec_Copy(v, gCamDir);
        Vec_FromCamera(t, v, -30.0f);
        break;
    case 4:
        t[0] = 13960.0f;
        t[1] = 41.0f;
        t[2] = 8560.0f;
        if (Cyl_Contains(p->pos, t, 4000.0f, -1.0f, -1.0f)) {
            if ((o->fvar -= 0.015f) < 0.125f) {
                o->fvar = 0.125f;
            }
        } else {
            v[0] = s[0] - gCamPos[0];
            v[1] = s[1] - gCamPos[1];
            v[2] = s[2] - gCamPos[2];
            d = Vec_Length(v);
            if (d < 0.001f) {
                ang = 0;
            } else {
                ang = (s32)(0.5f + 65536.0f * Math_Acos(-Vec_Dot(v, gCamDir) / d) / 6.2831855f);
            }
            aa = Math_Absf((s16)ang);
            tg = 0.75f - 0.5f * (aa / 32768.0f);
            if (tg < o->fvar) {
                if (tg > (o->fvar -= 0.015f)) {
                    o->fvar = tg;
                }
            } else {
                if (tg < (o->fvar += 0.015f)) {
                    o->fvar = tg;
                }
            }
        }
        Vec_Copy(v, gCamDir);
        Vec_FromCamera(t, v, -30.0f);
        break;
    case 5:
        if ((s8)--o->var2 <= 0) {
            o->state++;
            return;
        }
        if (o->var2 < 4) {
            o->fvar = o->var2 / 4.0f;
        }
        v[0] = o->pos[0] - gCamPos[0];
        v[1] = o->pos[1] - gCamPos[1];
        v[2] = o->pos[2] - gCamPos[2];
        Vec_Normalize(v);
        Vec_FromCamera(t, v, 200.0f);
        break;
    case 6:
        Vec_Copy(v, gCamDir);
        Vec_FromCamera(t, v, -50.0f);
        switch (gScene.area) {
        case 0x23:
        case 0xBC:
            lim = 400;
            break;
        case 0x49:
            lim = 200;
            break;
        case 0x4B:
            lim = 200;
            break;
        case 0x9F:
            lim = 200;
            break;
        default:
            lim = 0x400;
            break;
        }
        if (o->timer >= lim) {
            o->timer = 0;
        }
        break;
    case 7:
        f = 1.0f;
        o->var3++;
        lim = 60;
        if (o->timer >= lim) {
            o->timer = 0;
        }
        switch (o->var1) {
        case 0:
            if (gScene.eventRunning == 1 || Area_BoundaryZone(p) != 1) {
                o->var1 = 1;
            }
            break;
        case 3:
            if (gScene.eventRunning == 1) {
                o->var1 = 2;
            } else {
                switch (Area_BoundaryZone(p)) {
                default:
                case 0:
                    o->var1 = 2;
                    break;
                case 2:
                    break;
                case 1:
                    o->var1 = 0;
                    break;
                }
            }
            break;
        case 1:
            if (o->fvar == 0.0f) {
                o->fvar = 0.0f;
                o->var1 = 3;
            }
            if (gScene.eventRunning == 1) {
                o->var1 = 2;
            } else {
                switch (Area_BoundaryZone(p)) {
                default:
                case 0:
                    o->var1 = 2;
                    break;
                case 2:
                    break;
                case 1:
                    o->var1 = 0;
                    break;
                }
            }
            break;
        case 2:
            if (gScene.eventRunning == 1) {
                o->var1 = 2;
            } else if (Area_BoundaryZone(p) != 0) {
                o->var1 = 3;
            }
            break;
        }
        switch (o->var1) {
        case 3:
            v[0] = s[0] - gCamPos[0];
            v[1] = s[1] - gCamPos[1];
            v[2] = s[2] - gCamPos[2];
            Vec_Normalize(v);
            Vec_FromCamera(t, v, 100.0f);
            f = -0.08f;
            break;
        case 2:
            v[0] = s[0] - gCamPos[0];
            v[1] = s[1] - gCamPos[1];
            v[2] = s[2] - gCamPos[2];
            blocked = Area_LineBlocked(s);
            if (gScene.area == 0x91 && p->pos[2] < 2200.0f) {
                blocked = 1;
            }
            Vec_Normalize(v);
            Vec_FromCamera(t, v, 100.0f);
            if (blocked) {
                f = -0.07f;
            } else {
                f = 0.07f;
            }
            break;
        case 0:
            f = 0.08f;
            Mtx_GetTrans(t, gCamMatrix);
            break;
        case 1:
            f = -0.08f;
            Mtx_GetTrans(t, gCamMatrix);
            break;
        }
        if (Math_Absf(f) < 1.0f) {
            tg = o->fvar + f;
            o->fvar = tg;
            if (tg < 0.0f) {
                o->fvar = 0.0f;
            } else if (tg > 1.0f) {
                o->fvar = 1.0f;
            }
        }
        break;
    case 8:
    case 14:
        src = o->buf;
        n = Area_MonsterState(gScene.area);
        for (i = 0; i < count; i++) {
            f = AmbientSound_AreaValue0C(i, n);
            if (src->unk0C < f) {
                if ((src->unk0C += 0.001f) > f) {
                    src->unk0C = f;
                }
            } else if (src->unk0C > f) {
                if ((src->unk0C -= 0.001f) < f) {
                    src->unk0C = f;
                }
            }
            src->pos[1] += src->unk0C;
            src->pos[1] -= Math_Floor(src->pos[1]);
            f = AmbientSound_AreaValue10(i, n);
            if (src->unk10 != f) {
                if (src->unk10 < f) {
                    if ((src->unk10 += 0.025f) > f) {
                        src->unk10 = f;
                    }
                } else {
                    if ((src->unk10 -= 0.025f) < f) {
                        src->unk10 = f;
                    }
                }
            }
            src++;
        }
        Mtx_GetTrans(t, gCamMatrix);
        break;
    case 9:
        Mtx_GetTrans(t, gCamMatrix);
        break;
    case 10:
        pts = o->buf;
        pt = pts->pts[0];
        n = Area_MonsterState(gScene.area);
        sp = D_00449F00;
        i = 0;
        do {
            f = AmbientSound_AreaValue10(i, n);
            if (pt[2] != f) {
                if (pt[2] < f) {
                    if ((pt[2] += 0.025f) > f) {
                        pt[2] = f;
                    }
                } else {
                    if ((pt[2] -= 0.025f) < f) {
                        pt[2] = f;
                    }
                }
            }
            i++;
            pt += 3;
        } while (i < 2);
        UvScroll_Update((struct UvScroll *)pts, 2, sp, 2);
        Mtx_GetTrans(t, gCamMatrix);
        break;
    case 11:
        pts = o->buf;
        switch (gScene.area) {
        case 0x9A:
            lim = 200;
            break;
        case 0x9B:
        default:
            lim = 200;
            break;
        }
        UvScroll_Update((struct UvScroll *)pts, 2, &lim, 1);
        Vec_Copy(v, gCamDir);
        Vec_FromCamera(t, v, -50.0f);
        break;
    case 12:
        switch (o->var0) {
        case 0:
            if (++o->timer >= o->var3) {
                o->timer = 0;
                o->var0++;
            }
            return;
        case 1:
            if (++o->timer >= 0x100) {
                o->timer = 0;
                o->var3 = (Rand_Next(1) & 0x3FF) + 1200;
                o->var0 = 0;
                return;
            }
            o->fvar = 1.0f;
            Vec_Copy(v, gCamDir);
            Vec_FromCamera(t, v, -30.0f);
            break;
        }
        break;
    case 13:
        pts = o->buf;
        pt = pts->pts[0];
        n = Area_MonsterState(gScene.area);
        sp = D_00449F20[n];
        i = 0;
        do {
            f = AmbientSound_AreaValue10(i, n);
            if (pt[2] != f) {
                if (pt[2] < f) {
                    if ((pt[2] += 0.025f) > f) {
                        pt[2] = f;
                    }
                } else {
                    if ((pt[2] -= 0.025f) < f) {
                        pt[2] = f;
                    }
                }
            }
            i++;
            pt += 3;
        } while (i < 3);
        UvScroll_Update((struct UvScroll *)pts, 3, sp, 2);
        Mtx_GetTrans(t, gCamMatrix);
        break;
    case 15:
        Vec_Copy(t, gCamPos);
        lim = 0x80;
        if (o->timer >= lim) {
            o->timer = 0;
        }
        break;
    }
    if (o->ds != NULL) {
        o->ds->pos[0] = t[0];
        o->ds->pos[1] = t[1];
        o->ds->pos[2] = t[2];
        OT_AddByDepth(gDrawListStage, o->ds, 1, 0);
    }
}

void AmbientSound_End(Obj *o)
{
    o->state++;
    o->visible = 0;
}

void AmbientSound_Free(Obj *o)
{
    if (o->ds != NULL) {
        DrawSlot_Free(o->slot);
    }
    Obj_Free(o);
}

void CamFov_Narrow(s8 *fov)
{
    Camera_SetFov(0.87266463f);
    *fov = 1;
}

void CamFov_Restore(u8 *fov)
{
    if (*fov != 0) {
        Camera_RestoreFov();
        *fov = 0;
    }
}

/* A source of an ambient sound's buffer (types 8 and 14), as AmbientSound_Draw sees it. */
typedef struct AmbSrc {
    f32 pos[3];                 /* 0x00 */
    f32 unk0C;                  /* 0x0C */
    f32 alpha;                  /* 0x10 */
} AmbSrc;

extern s16 gAmbSoundCounts[];
f32 Math_Sqrtf(f32 x);
f32 Math_Acos(f32 x);
void Mtx_Concat3x3(f32 *m, f32 *n);

void AmbientSound_Draw(DrawSlot *slot)
{
    AmbSrc *src;
    Obj *o = slot->owner;
    ModelMatSet *e;
    s16 idx;
    s16 cnt;
    s16 i;
    s32 div;
    s32 div2;
    f32 u;
    f32 dx, dz, cx, cz;
    f32 len;
    f32 c, a;
    f32 lx, lz;
    f32 *pt;
    Model *set = gAreaSubModels[0];
    f32 mtx[4][4];
    f32 tm[4][4];
    f32 m2[4][4];
    s8 fov = 0;

    if (set == NULL) {
        return;
    }
    if (set->active == 0) {
        return;
    }
    if (gAreaFade == 1 || gAreaFade == 2) {
        return;
    }
    idx = gAreaAmbModels[gScene.area][1];
    if (idx < 0) {
        return;
    }
    e = &set->matSets[idx];
    if (o->type != 5) {
        lx = gAreaState.skyPos[0];
        lz = gAreaState.skyPos[2];
    }
    Gs_SetState(0x60, 0);
    Gs_SetState(0x6C, 0);
    Gs_SetState(0x6D, 7);
    if ((u32)(o->type - 7) <= 2 || o->type == 14) {
        Mtx_Identity(mtx[0]);
    } else {
        Mtx_Copy(mtx[0], gCamMatrix);
    }
    Mtx_SetTrans(mtx[0], slot->pos[0], slot->pos[1], slot->pos[2]);
    cnt = gAmbSoundCounts[o->type];
    switch (o->type) {
    case 0:
        Mtx_Identity(mtx[0]);
        u = 0.0f;
        switch (gScene.area) {
        case 0x30:
        case 0x31:
            u = 0.0025f * o->timer;
            break;
        case 0x28:
        case 0xC1:
            u = 0.0078125f * (o->timer & 0x7F);
            break;
        case 0x77:
            u = 0.005f * o->timer;
            break;
        case 0x35:
        case 0x93:
            u = 0.01f * o->timer;
            break;
        case 0x38:
        case 0x96:
            u = 0.01f * o->timer;
            break;
        default:
            dx = lx - slot->pos[0];
            dz = lz - slot->pos[2];
            cx = -gCamDir[0];
            cz = -gCamDir[2];
            c = dx * cx + dz * cz;
            len = Math_Sqrtf(dx * dx + dz * dz);
            c = c / (len * Math_Sqrtf(cx * cx + cz * cz));
            if (!(c < 1.0f)) {
                c = 1.0f;
            }
            a = Math_Acos(c);
            if (dx * cz - dz * cx < 0.0f) {
                a = -a;
            }
            Mtx_RotateZ(mtx[0], 0.2f * a);
            break;
        }
        Mtx_Translation(tm[0], u, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32)tm);
        Mtx_Concat3x3(mtx[0], gCamMatrix);
        Mtx_SetTrans(mtx[0], slot->pos[0], slot->pos[1], slot->pos[2]);
        {
            u8 v = 255.0f * o->fvar;
            Gs_SetState(0x67, (v << 16) | 0xFF000000 | (v << 8) | v);
        }
        break;
    case 1:
        CamFov_Narrow(&fov);
        switch (gScene.area) {
        case 0x18:
        case 0x22:
        case 0xBB:
            div = 500;
            break;
        default:
            div = 0x400;
            break;
        }
        Mtx_Translation(tm[0], (1.0f / div) * (o->timer % (s16)div), 0.0f, 0.0f);
        Gs_SetState(0x19, (s32)tm);
        Gs_SetState(0x67, ((u8)(255.0f * o->fvar) << 24) | 0xFFFFFF);
        break;
    case 2:
        CamFov_Narrow(&fov);
        Mtx_Translation(tm[0], 0.0025f * o->timer, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32)tm);
        Gs_SetState(0x1A, (s32)mtx);
        if (e != NULL && e->id != -1) {
            Draw_ApplyStates(e->flags);
            Model_Draw(e->id, 0);
        }
        break;
    case 3:
        CamFov_Narrow(&fov);
        switch (gScene.area) {
        case 0x36:
            div = 1600;
            div2 = 700;
            break;
        default:
            div = 800;
            div2 = 300;
            break;
        }
        Mtx_Translation(tm[0], (1.0f / div) * o->timer, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32)tm);
        Gs_SetState(0x1A, (s32)mtx);
        Gs_SetState(0x6D, 7);
        if (e != NULL && e->id != -1) {
            Draw_ApplyStates(e->flags);
            Model_Draw(e->id, 0);
        }
        e = &set->matSets[idx] + 1;
        Mtx_Translation(tm[0], (1.0f / div2) * o->var3, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32)tm);
        break;
    case 4:
        CamFov_Narrow(&fov);
        Mtx_Translation(tm[0], 0.0009765625f * (o->timer & 0x3FF), 0.0f, 0.0f);
        Gs_SetState(0x19, (s32)tm);
        Gs_SetState(0x67, ((u8)(255.0f * o->fvar) << 24) | 0xFFFFFF);
        break;
    case 5:
        CamFov_Narrow(&fov);
        {
            u8 v = 255.0f * o->fvar;
            Gs_SetState(0x67, (v << 16) | 0xFF000000 | (v << 8) | v);
        }
        break;
    case 6:
        CamFov_Narrow(&fov);
        switch (gScene.area) {
        case 0x23:
        case 0xBC:
            div = 400;
            break;
        case 0x49:
            div = 200;
            break;
        case 0x4B:
            div = 200;
            break;
        case 0x9F:
            div = 200;
            break;
        default:
            div = 0x400;
            break;
        }
        Mtx_Translation(tm[0], (1.0f / div) * (o->timer % (s16)div), 0.0f, 0.0f);
        Gs_SetState(0x19, (s32)tm);
        Gs_SetState(0x67, ((u8)(255.0f * o->fvar) << 24) | 0xFFFFFF);
        break;
    case 7:
        switch (o->var1) {
        case 2:
        case 3:
            Mtx_Identity(mtx[0]);
            dx = lx - slot->pos[0];
            dz = lz - slot->pos[2];
            cx = -gCamDir[0];
            cz = -gCamDir[2];
            c = dx * cx + dz * cz;
            len = Math_Sqrtf(dx * dx + dz * dz);
            c = c / (len * Math_Sqrtf(cx * cx + cz * cz));
            if (!(c < 1.0f)) {
                c = 1.0f;
            }
            a = Math_Acos(c);
            if (dx * cz - dz * cx < 0.0f) {
                a = -a;
            }
            Mtx_RotateZ(mtx[0], 0.2f * a);
            Mtx_Concat3x3(mtx[0], gCamMatrix);
            Mtx_SetTrans(mtx[0], slot->pos[0], slot->pos[1], slot->pos[2]);
            {
                u8 v = 255.0f * o->fvar;
                Gs_SetState(0x67, (v << 16) | 0xFF000000 | (v << 8) | v);
            }
            break;
        case 0:
        case 1:
            CamFov_Narrow(&fov);
            e = &set->matSets[idx] + 1;
            Gs_SetState(0x67, ((u8)(128.0f * o->fvar) << 24) | 0xFFFFFF);
            Mtx_Translation(tm[0], 0.0f, 1.0f - 0.016666668f * o->timer, 0.0f);
            Gs_SetState(0x19, (s32)tm);
            break;
        }
        break;
    case 8:
    case 14:
        src = o->buf;
        Mtx_Identity(m2[0]);
        for (i = 0; i < cnt; i++, src++) {
            if (i == 2) {
                Gs_SetState(0x1A, (s32)m2);
            } else {
                CamFov_Narrow(&fov);
                Gs_SetState(0x1A, (s32)mtx);
            }
            e = &set->matSets[idx] + i;
            if (i == 0) {
                Mtx_Translation(tm[0], 0.0f, 0.0f, 0.0f);
            } else {
                Mtx_Translation(tm[0], 0.0f, src->pos[1], 0.0f);
            }
            Gs_SetState(0x19, (s32)tm);
            Gs_SetState(0x67, ((u8)(255.0f * src->alpha) << 24) | 0xFFFFFF);
            if (e != NULL && e->id != -1) {
                Draw_ApplyStates(e->flags);
                Model_Draw(e->id, 0);
            }
            CamFov_Restore((u8 *)&fov);
        }
        goto done;
    case 9:
        Gs_SetState(0x1A, (s32)mtx);
        e = &set->matSets[idx];
        break;
    case 10:
    case 13:
        CamFov_Narrow(&fov);
        pt = (f32 *)((u8 *)o->buf + 0x10);
        Gs_SetState(0x1A, (s32)mtx);
        for (i = 0; i < cnt; i++, pt += 3) {
            e = &set->matSets[idx] + i;
            Mtx_Translation(tm[0], pt[0], pt[1], 0.0f);
            Gs_SetState(0x19, (s32)tm);
            Gs_SetState(0x67, ((u8)(255.0f * pt[2]) << 24) | 0xFFFFFF);
            if (e != NULL && e->id != -1) {
                Draw_ApplyStates(e->flags);
                Model_Draw(e->id, 0);
            }
        }
        goto done;
    case 11:
        CamFov_Narrow(&fov);
        Mtx_Translation(tm[0], *(f32 *)((u8 *)o->buf + 0x10), 0.0f, 0.0f);
        Gs_SetState(0x19, (s32)tm);
        break;
    case 12:
        CamFov_Narrow(&fov);
        Mtx_Translation(tm[0], 0.0f, 1.0f - o->timer / 256.0f, 0.0f);
        Gs_SetState(0x19, (s32)tm);
        Gs_SetState(0x67, ((u8)(255.0f * o->fvar) << 24) | 0xFFFFFF);
        Gs_SetState(0x1A, (s32)mtx);
        if (e != NULL && e->id != -1) {
            Draw_ApplyStates(e->flags);
            Model_Draw(e->id, 0);
        }
        if (o->timer > 0x80) {
            goto done;
        }
        Mtx_Translation(tm[0], 0.0f, 1.0f - o->timer / 128.0f, 0.0f);
        Gs_SetState(0x19, (s32)tm);
        break;
    case 15:
        Mtx_Identity(mtx[0]);
        Mtx_Translation(tm[0], 0.0f, o->timer / 128.0f, 0.0f);
        Gs_SetState(0x19, (s32)tm);
        Gs_SetState(0x67, -1);
        break;
    }
    Gs_SetState(0x1A, (s32)mtx);
    if (e != NULL && e->id != -1) {
        Draw_ApplyStates(e->flags);
        Model_Draw(e->id, 0);
    }
done:
    Draw_SetDefaultState();
    Gs_SetState(0x6C, 1);
    Gs_SetState(0x6D, 3);
    CamFov_Restore((u8 *)&fov);
}

/* unk_37B570's .data (0x441150-0x449530) */
extern u32 gDefaultStageHeader_3[];
extern u32 D_006AE9A0[];
extern u32 D_006AE9A8[];
extern u32 D_006AE9B0[];
extern char *gDefaultStageHeader_12[];
extern AreaAnim gAreaAnimTbls_128[];
s16 gAreaAmbModels_0[16] = {
#include "sub_main/00441150.inc"
};
/* gm_sub (loaded with game): its 119 areas' rows of gAreaAmbModels (model.h), 16 set
 * indices each (config/ld/gm_sub_aliases.ld). */
extern s16 D_0092AD60[119][16];
s16 *gAreaAmbModels[200] = {
    gAreaAmbModels_0, D_0092AD60[0], D_0092AD60[1], D_0092AD60[2],
    D_0092AD60[3], D_0092AD60[4], D_0092AD60[5], D_0092AD60[6],
    gAreaAmbModels_0, D_0092AD60[7], gAreaAmbModels_0, D_0092AD60[8],
    D_0092AD60[9], gAreaAmbModels_0, D_0092AD60[10], D_0092AD60[11],
    D_0092AD60[99], gAreaAmbModels_0, D_0092AD60[12], D_0092AD60[13],
    gAreaAmbModels_0, D_0092AD60[14], D_0092AD60[78], D_0092AD60[79],
    D_0092AD60[15], D_0092AD60[16], D_0092AD60[80], D_0092AD60[58],
    D_0092AD60[17], D_0092AD60[18], D_0092AD60[19], D_0092AD60[20],
    D_0092AD60[21], D_0092AD60[22], D_0092AD60[23], D_0092AD60[24],
    D_0092AD60[25], D_0092AD60[26], D_0092AD60[27], D_0092AD60[28],
    D_0092AD60[29], D_0092AD60[30], D_0092AD60[31], D_0092AD60[32],
    D_0092AD60[33], D_0092AD60[34], D_0092AD60[35], D_0092AD60[36],
    D_0092AD60[37], D_0092AD60[38], D_0092AD60[39], D_0092AD60[40],
    D_0092AD60[41], D_0092AD60[95], D_0092AD60[42], D_0092AD60[43],
    D_0092AD60[98], D_0092AD60[44], gAreaAmbModels_0, D_0092AD60[45],
    D_0092AD60[46], D_0092AD60[47], D_0092AD60[48], D_0092AD60[49],
    D_0092AD60[50], D_0092AD60[51], D_0092AD60[52], D_0092AD60[53],
    D_0092AD60[102], D_0092AD60[103], D_0092AD60[54], D_0092AD60[55],
    D_0092AD60[56], D_0092AD60[57], D_0092AD60[58], D_0092AD60[59],
    gAreaAmbModels_0, D_0092AD60[8], D_0092AD60[60], D_0092AD60[10],
    D_0092AD60[61], D_0092AD60[19], D_0092AD60[20], gAreaAmbModels_0,
    gAreaAmbModels_0, gAreaAmbModels_0, gAreaAmbModels_0, gAreaAmbModels_0,
    gAreaAmbModels_0, D_0092AD60[62], D_0092AD60[63], D_0092AD60[64],
    gAreaAmbModels_0, gAreaAmbModels_0, D_0092AD60[65], D_0092AD60[66],
    D_0092AD60[67], D_0092AD60[68], gAreaAmbModels_0, D_0092AD60[69],
    D_0092AD60[70], gAreaAmbModels_0, gAreaAmbModels_0, D_0092AD60[65],
    D_0092AD60[66], D_0092AD60[67], D_0092AD60[68], gAreaAmbModels_0,
    D_0092AD60[69], D_0092AD60[70], D_0092AD60[71], D_0092AD60[72],
    D_0092AD60[73], D_0092AD60[74], D_0092AD60[75], D_0092AD60[76],
    D_0092AD60[77], D_0092AD60[78], D_0092AD60[79], D_0092AD60[80],
    D_0092AD60[81], gAreaAmbModels_0, D_0092AD60[82], D_0092AD60[83],
    D_0092AD60[84], gAreaAmbModels_0, gAreaAmbModels_0, gAreaAmbModels_0,
    D_0092AD60[85], D_0092AD60[86], D_0092AD60[87], gAreaAmbModels_0,
    gAreaAmbModels_0, gAreaAmbModels_0, gAreaAmbModels_0, gAreaAmbModels_0,
    gAreaAmbModels_0, gAreaAmbModels_0, gAreaAmbModels_0, gAreaAmbModels_0,
    D_0092AD60[88], D_0092AD60[89], D_0092AD60[90], D_0092AD60[91],
    D_0092AD60[92], D_0092AD60[93], D_0092AD60[94], D_0092AD60[95],
    D_0092AD60[96], D_0092AD60[97], D_0092AD60[98], D_0092AD60[99],
    D_0092AD60[100], D_0092AD60[101], D_0092AD60[102], D_0092AD60[103],
    D_0092AD60[104], D_0092AD60[105], D_0092AD60[106], D_0092AD60[107],
    D_0092AD60[108], D_0092AD60[109], D_0092AD60[110], D_0092AD60[111],
    D_0092AD60[112], D_0092AD60[113], D_0092AD60[114], D_0092AD60[115],
    D_0092AD60[116], D_0092AD60[117], gAreaAmbModels_0, gAreaAmbModels_0,
    gAreaAmbModels_0, gAreaAmbModels_0, gAreaAmbModels_0, gAreaAmbModels_0,
    gAreaAmbModels_0, gAreaAmbModels_0, gAreaAmbModels_0, gAreaAmbModels_0,
    gAreaAmbModels_0, gAreaAmbModels_0, gAreaAmbModels_0, gAreaAmbModels_0,
    D_0092AD60[14], D_0092AD60[118], D_0092AD60[22], D_0092AD60[23],
    D_0092AD60[24], D_0092AD60[25], D_0092AD60[26], D_0092AD60[27],
    D_0092AD60[28], D_0092AD60[29], D_0092AD60[30], D_0092AD60[31],
    D_0092AD60[32], NULL, NULL, NULL,
};
u32 gDefaultStageHeader_8[8] = {
#include "sub_main/00441490.inc"
};
StageHeader gDefaultStageHeader = {
#include "sub_main/004414B0.inc"
};
u32 D_00441530[4] = {
#include "sub_main/00441530.inc"
};
AreaAnim gAreaAnimTbls_1[4] = {
#include "sub_main/00441540.inc"
};
u32 D_00441560[4] = {
#include "sub_main/00441560.inc"
};
AreaAnim gAreaAnimTbls_2[2] = {
#include "sub_main/00441570.inc"
};
u32 D_00441580[4] = {
#include "sub_main/00441580.inc"
};
AreaAnim gAreaAnimTbls_4[2] = {
#include "sub_main/00441590.inc"
};
u32 D_004415A0[4] = {
#include "sub_main/004415A0.inc"
};
AreaAnim gAreaAnimTbls_5[2] = {
#include "sub_main/004415B0.inc"
};
AreaAnim gAreaAnimTbls_7[4] = {
#include "sub_main/004415C0.inc"
};
u32 D_004415E0[4] = {
#include "sub_main/004415E0.inc"
};
u32 D_004415F0[4] = {
#include "sub_main/004415F0.inc"
};
AreaAnim gAreaAnimTbls_8[4] = {
#include "sub_main/00441600.inc"
};
AreaAnim gAreaAnimTbls_9[2] = {
#include "sub_main/00441620.inc"
};
u32 D_00441630[4] = {
#include "sub_main/00441630.inc"
};
u32 D_00441640[4] = {
#include "sub_main/00441640.inc"
};
AreaAnim gAreaAnimTbls_14[4] = {
#include "sub_main/00441650.inc"
};
u32 D_00441670[8] = {
#include "sub_main/00441670.inc"
};
AreaAnim gAreaAnimTbls_21[2] = {
#include "sub_main/00441690.inc"
};
AreaAnim gAreaAnimTbls_24[4] = {
#include "sub_main/004416A0.inc"
};
u32 D_004416C0[4] = {
#include "sub_main/004416C0.inc"
};
AnimSpin D_004416D0[1] = {
#include "sub_main/004416D0.inc"
};
UvScroll D_004416E0[2] = {
#include "sub_main/004416E0.inc"
};
AnimScrollSpin D_004416E8[1] = {
    {D_004416E0, D_004416D0},
};
AreaAnim gAreaAnimTbls_25[4] = {
#include "sub_main/004416F0.inc"
};
u32 D_00441710[2] = {
#include "sub_main/00441710.inc"
};
u32 D_00441718[2] = {
#include "sub_main/00441718.inc"
};
AreaAnim gAreaAnimTbls_27[8] = {
#include "sub_main/00441720.inc"
};
u32 D_00441760[4] = {
#include "sub_main/00441760.inc"
};
u32 D_00441770[4] = {
#include "sub_main/00441770.inc"
};
AreaAnim gAreaAnimTbls_31[4] = {
#include "sub_main/00441780.inc"
};
u32 D_004417A0[4] = {
#include "sub_main/004417A0.inc"
};
AreaAnim gAreaAnimTbls_32[4] = {
#include "sub_main/004417B0.inc"
};
u32 D_004417D0[4] = {
#include "sub_main/004417D0.inc"
};
AreaAnim gAreaAnimTbls_33[2] = {
#include "sub_main/004417E0.inc"
};
u32 D_004417F0[8] = {
#include "sub_main/004417F0.inc"
};
AreaAnim gAreaAnimTbls_34[2] = {
#include "sub_main/00441810.inc"
};
u32 D_00441820[8] = {
#include "sub_main/00441820.inc"
};
AreaAnim gAreaAnimTbls_35[2] = {
#include "sub_main/00441840.inc"
};
u32 D_00441850[8] = {
#include "sub_main/00441850.inc"
};
AreaAnim gAreaAnimTbls_36[2] = {
#include "sub_main/00441870.inc"
};
u32 D_00441880[4] = {
#include "sub_main/00441880.inc"
};
u32 D_00441890[4] = {
#include "sub_main/00441890.inc"
};
AreaAnim gAreaAnimTbls_37[4] = {
#include "sub_main/004418A0.inc"
};
u32 D_004418C0[4] = {
#include "sub_main/004418C0.inc"
};
u32 D_004418D0[4] = {
#include "sub_main/004418D0.inc"
};
AreaAnim gAreaAnimTbls_38[4] = {
#include "sub_main/004418E0.inc"
};
u32 D_00441900[4] = {
#include "sub_main/00441900.inc"
};
u32 D_00441910[4] = {
#include "sub_main/00441910.inc"
};
u32 D_00441920[4] = {
#include "sub_main/00441920.inc"
};
AreaAnim gAreaAnimTbls_39[4] = {
#include "sub_main/00441930.inc"
};
AreaAnim gAreaAnimTbls_40[2] = {
#include "sub_main/00441950.inc"
};
u32 D_00441960[8] = {
#include "sub_main/00441960.inc"
};
AreaAnim gAreaAnimTbls_41[2] = {
#include "sub_main/00441980.inc"
};
u32 D_00441990[8] = {
#include "sub_main/00441990.inc"
};
AreaAnim gAreaAnimTbls_42[2] = {
#include "sub_main/004419B0.inc"
};
u32 D_004419C0[8] = {
#include "sub_main/004419C0.inc"
};
AreaAnim gAreaAnimTbls_43[2] = {
#include "sub_main/004419E0.inc"
};
AreaAnim gAreaAnimTbls_45[6] = {
#include "sub_main/004419F0.inc"
};
AreaAnim gAreaAnimTbls_46[3] = {
#include "sub_main/00441A20.inc"
};
u32 D_00441A38[2] = {
#include "sub_main/00441A38.inc"
};
AreaAnim gAreaAnimTbls_47[4] = {
#include "sub_main/00441A40.inc"
};
AreaAnim gAreaAnimTbls_48[6] = {
#include "sub_main/00441A60.inc"
};
AreaAnim gAreaAnimTbls_49[4] = {
#include "sub_main/00441A90.inc"
};
AreaAnim gAreaAnimTbls_50[4] = {
#include "sub_main/00441AB0.inc"
};
AreaAnim gAreaAnimTbls_51[4] = {
#include "sub_main/00441AD0.inc"
};
AreaAnim gAreaAnimTbls_52[4] = {
#include "sub_main/00441AF0.inc"
};
AreaAnim gAreaAnimTbls_57[3] = {
#include "sub_main/00441B10.inc"
};
u32 D_00441B28[2] = {
#include "sub_main/00441B28.inc"
};
AreaAnim gAreaAnimTbls_58[4] = {
#include "sub_main/00441B30.inc"
};
u32 D_00441B50[2] = {
#include "sub_main/00441B50.inc"
};
u32 D_00441B58[2] = {
#include "sub_main/00441B58.inc"
};
AreaAnim gAreaAnimTbls_60[7] = {
#include "sub_main/00441B60.inc"
};
u32 D_00441B98[2] = {
#include "sub_main/00441B98.inc"
};
AreaAnim gAreaAnimTbls_61[6] = {
#include "sub_main/00441BA0.inc"
};
u32 D_00441BD0[2] = {
#include "sub_main/00441BD0.inc"
};
u32 D_00441BD8[2] = {
#include "sub_main/00441BD8.inc"
};
AreaAnim gAreaAnimTbls_62[8] = {
#include "sub_main/00441BE0.inc"
};
u32 D_00441C20[2] = {
#include "sub_main/00441C20.inc"
};
u32 D_00441C28[2] = {
#include "sub_main/00441C28.inc"
};
AreaAnim gAreaAnimTbls_65[6] = {
#include "sub_main/00441C30.inc"
};
AreaAnim gAreaAnimTbls_66[4] = {
#include "sub_main/00441C60.inc"
};
u32 D_00441C80[4] = {
#include "sub_main/00441C80.inc"
};
u32 D_00441C90[2] = {
#include "sub_main/00441C90.inc"
};
u32 D_00441C98[2] = {
#include "sub_main/00441C98.inc"
};
AreaAnim gAreaAnimTbls_74[8] = {
#include "sub_main/00441CA0.inc"
};
AreaAnim gAreaAnimTbls_75[2] = {
#include "sub_main/00441CE0.inc"
};
u32 D_00441CF0[4] = {
#include "sub_main/00441CF0.inc"
};
AreaAnim gAreaAnimTbls_84[5] = {
#include "sub_main/00441D00.inc"
};
u32 D_00441D28[2] = {
#include "sub_main/00441D28.inc"
};
AreaAnim gAreaAnimTbls_85[4] = {
#include "sub_main/00441D30.inc"
};
AreaAnim gAreaAnimTbls_86[6] = {
#include "sub_main/00441D50.inc"
};
AreaAnim gAreaAnimTbls_87[6] = {
#include "sub_main/00441D80.inc"
};
u32 D_00441DB0[4] = {
#include "sub_main/00441DB0.inc"
};
u32 D_00441DC0[2] = {
#include "sub_main/00441DC0.inc"
};
u32 D_00441DC8[2] = {
#include "sub_main/00441DC8.inc"
};
AreaAnim gAreaAnimTbls_88[6] = {
#include "sub_main/00441DD0.inc"
};
u32 D_00441E00[4] = {
#include "sub_main/00441E00.inc"
};
AreaAnim gAreaAnimTbls_89[4] = {
#include "sub_main/00441E10.inc"
};
u32 D_00441E30[4] = {
#include "sub_main/00441E30.inc"
};
AreaAnim gAreaAnimTbls_90[4] = {
#include "sub_main/00441E40.inc"
};
u32 D_00441E60[4] = {
#include "sub_main/00441E60.inc"
};
AreaAnim gAreaAnimTbls_91[4] = {
#include "sub_main/00441E70.inc"
};
AreaAnim gAreaAnimTbls_92[4] = {
#include "sub_main/00441E90.inc"
};
AreaAnim gAreaAnimTbls_93[6] = {
#include "sub_main/00441EB0.inc"
};
AreaAnim gAreaAnimTbls_94[6] = {
#include "sub_main/00441EE0.inc"
};
AreaAnim gAreaAnimTbls_6[2] = {
#include "sub_main/00441F10.inc"
};
u32 D_00441F20[4] = {
#include "sub_main/00441F20.inc"
};
AreaAnim gAreaAnimTbls_96[4] = {
#include "sub_main/00441F30.inc"
};
u32 D_00441F50[4] = {
#include "sub_main/00441F50.inc"
};
AreaAnim gAreaAnimTbls_97[6] = {
#include "sub_main/00441F60.inc"
};
AreaAnim gAreaAnimTbls_98[2] = {
#include "sub_main/00441F90.inc"
};
AreaAnim gAreaAnimTbls_99[2] = {
#include "sub_main/00441FA0.inc"
};
u32 D_00441FB0[2] = {
#include "sub_main/00441FB0.inc"
};
u32 D_00441FB8[2] = {
#include "sub_main/00441FB8.inc"
};
u32 D_00441FC0[4] = {
#include "sub_main/00441FC0.inc"
};
AreaAnim gAreaAnimTbls_100[8] = {
#include "sub_main/00441FD0.inc"
};
AreaAnim gAreaAnimTbls_105[4] = {
#include "sub_main/00442010.inc"
};
AreaAnim gAreaAnimTbls_110[4] = {
#include "sub_main/00442030.inc"
};
u32 D_00442050[4] = {
#include "sub_main/00442050.inc"
};
AreaAnim gAreaAnimTbls_111[2] = {
#include "sub_main/00442060.inc"
};
u32 D_00442070[4] = {
#include "sub_main/00442070.inc"
};
u32 D_00442080[4] = {
#include "sub_main/00442080.inc"
};
AreaAnim gAreaAnimTbls_3[4] = {
#include "sub_main/00442090.inc"
};
AreaAnim gAreaAnimTbls_18[4] = {
#include "sub_main/004420B0.inc"
};
AreaAnim gAreaAnimTbls_19[2] = {
#include "sub_main/004420D0.inc"
};
u32 D_004420E0[4] = {
#include "sub_main/004420E0.inc"
};
AreaAnim gAreaAnimTbls_22[4] = {
#include "sub_main/004420F0.inc"
};
u32 D_00442110[4] = {
#include "sub_main/00442110.inc"
};
AreaAnim gAreaAnimTbls_23[4] = {
#include "sub_main/00442120.inc"
};
AreaAnim gAreaAnimTbls_120[4] = {
#include "sub_main/00442140.inc"
};
AreaAnim gAreaAnimTbls_121[2] = {
#include "sub_main/00442160.inc"
};
u32 D_00442170[4] = {
#include "sub_main/00442170.inc"
};
AreaAnim gAreaAnimTbls_122[6] = {
#include "sub_main/00442180.inc"
};
AreaAnim gAreaAnimTbls_123[6] = {
#include "sub_main/004421B0.inc"
};
u32 D_004421E0[2] = {
#include "sub_main/004421E0.inc"
};
u32 D_004421E8[2] = {
#include "sub_main/004421E8.inc"
};
u32 D_004421F0[2] = {
#include "sub_main/004421F0.inc"
};
u32 D_004421F8[2] = {
#include "sub_main/004421F8.inc"
};
AreaAnim gAreaAnimTbls_124[10] = {
#include "sub_main/00442200.inc"
};
u32 D_00442250[4] = {
#include "sub_main/00442250.inc"
};
AreaAnim gAreaAnimTbls_83[2] = {
#include "sub_main/00442260.inc"
};
AreaAnim gAreaAnimTbls_127[4] = {
#include "sub_main/00442270.inc"
};
u32 D_00442290[2] = {
#include "sub_main/00442290.inc"
};
u32 D_00442298[2] = {
#include "sub_main/00442298.inc"
};
AreaAnim gAreaAnimTbls_128[10] = {
#include "sub_main/004422A0.inc"
};
u32 D_004422F0[2] = {
#include "sub_main/004422F0.inc"
};
u32 D_004422F8[2] = {
#include "sub_main/004422F8.inc"
};
AreaAnim gAreaAnimTbls_129[8] = {
#include "sub_main/00442300.inc"
};
u32 D_00442340[4] = {
#include "sub_main/00442340.inc"
};
AreaAnim gAreaAnimTbls_131[10] = {
#include "sub_main/00442350.inc"
};
u32 D_004423A0[4] = {
#include "sub_main/004423A0.inc"
};
AreaAnim gAreaAnimTbls_132[14] = {
#include "sub_main/004423B0.inc"
};
u32 D_00442420[20] = {
#include "sub_main/00442420.inc"
};
AreaAnim gAreaAnimTbls_133[8] = {
#include "sub_main/00442470.inc"
};
AreaAnim gAreaAnimTbls_134[4] = {
#include "sub_main/004424B0.inc"
};
AreaAnim gAreaAnimTbls_135[8] = {
#include "sub_main/004424D0.inc"
};
u32 D_00442510[4] = {
#include "sub_main/00442510.inc"
};
AreaAnim gAreaAnimTbls_136[8] = {
#include "sub_main/00442520.inc"
};
AreaAnim gAreaAnimTbls_137[4] = {
#include "sub_main/00442560.inc"
};
u32 D_00442580[2] = {
#include "sub_main/00442580.inc"
};
u32 D_00442588[2] = {
#include "sub_main/00442588.inc"
};
u32 D_00442590[4] = {
#include "sub_main/00442590.inc"
};
AreaAnim gAreaAnimTbls_138[8] = {
#include "sub_main/004425A0.inc"
};
AreaAnim gAreaAnimTbls_140[2] = {
#include "sub_main/004425E0.inc"
};
u32 D_004425F0[4] = {
#include "sub_main/004425F0.inc"
};
AreaAnim gAreaAnimTbls_141[6] = {
#include "sub_main/00442600.inc"
};
u32 D_00442630[4] = {
#include "sub_main/00442630.inc"
};
AreaAnim gAreaAnimTbls_142[6] = {
#include "sub_main/00442640.inc"
};
AreaAnim gAreaAnimTbls_143[4] = {
#include "sub_main/00442670.inc"
};
AreaAnim gAreaAnimTbls_144[4] = {
#include "sub_main/00442690.inc"
};
AreaAnim gAreaAnimTbls_145[2] = {
#include "sub_main/004426B0.inc"
};
AreaAnim gAreaAnimTbls_146[4] = {
#include "sub_main/004426C0.inc"
};
AreaAnim gAreaAnimTbls_53[2] = {
#include "sub_main/004426E0.inc"
};
AreaAnim gAreaAnimTbls_54[4] = {
#include "sub_main/004426F0.inc"
};
AreaAnim gAreaAnimTbls_55[2] = {
#include "sub_main/00442710.inc"
};
AreaAnim gAreaAnimTbls_56[2] = {
#include "sub_main/00442720.inc"
};
AreaAnim gAreaAnimTbls_16[4] = {
#include "sub_main/00442730.inc"
};
AreaAnim gAreaAnimTbls_152[2] = {
#include "sub_main/00442750.inc"
};
AreaAnim gAreaAnimTbls_67[3] = {
#include "sub_main/00442760.inc"
};
u32 D_00442778[3] = {
#include "sub_main/00442778.inc"
};
AreaAnim gAreaAnimTbls_68[4] = {
#include "sub_main/00442790.inc"
};
u32 D_004427B0[4] = {
#include "sub_main/004427B0.inc"
};
AreaAnim gAreaAnimTbls_69[6] = {
#include "sub_main/004427C0.inc"
};
u32 D_004427F0[4] = {
#include "sub_main/004427F0.inc"
};
u32 D_00442800[4] = {
#include "sub_main/00442800.inc"
};
AreaAnim gAreaAnimTbls_70[3] = {
#include "sub_main/00442810.inc"
};
u32 D_00442828[2] = {
#include "sub_main/00442828.inc"
};
AreaAnim gAreaAnimTbls_71[4] = {
#include "sub_main/00442830.inc"
};
AreaAnim gAreaAnimTbls_158[2] = {
#include "sub_main/00442850.inc"
};
u32 D_00442860[4] = {
#include "sub_main/00442860.inc"
};
AreaAnim gAreaAnimTbls_73[4] = {
#include "sub_main/00442870.inc"
};
AreaAnim gAreaAnimTbls_160[2] = {
#include "sub_main/00442890.inc"
};
u32 D_004428A0[4] = {
#include "sub_main/004428A0.inc"
};
u32 D_004428B0[4] = {
#include "sub_main/004428B0.inc"
};
AreaAnim gAreaAnimTbls_161[4] = {
#include "sub_main/004428C0.inc"
};
u32 D_004428E0[4] = {
#include "sub_main/004428E0.inc"
};
AreaAnim gAreaAnimTbls_59[5] = {
#include "sub_main/004428F0.inc"
};
u32 D_00442918[2] = {
#include "sub_main/00442918.inc"
};
u32 D_00442920[4] = {
#include "sub_main/00442920.inc"
};
AreaAnim gAreaAnimTbls_163[8] = {
#include "sub_main/00442930.inc"
};
u32 D_00442970[4] = {
#include "sub_main/00442970.inc"
};
u32 D_00442980[4] = {
#include "sub_main/00442980.inc"
};
AreaAnim gAreaAnimTbls_164[4] = {
#include "sub_main/00442990.inc"
};
u32 D_004429B0[4] = {
#include "sub_main/004429B0.inc"
};
u32 D_004429C0[2] = {
#include "sub_main/004429C0.inc"
};
u32 D_004429C8[2] = {
#include "sub_main/004429C8.inc"
};
AreaAnim gAreaAnimTbls_165[7] = {
#include "sub_main/004429D0.inc"
};
u32 D_00442A08[2] = {
#include "sub_main/00442A08.inc"
};
u32 D_00442A10[4] = {
#include "sub_main/00442A10.inc"
};
AreaAnim gAreaAnimTbls_63[8] = {
#include "sub_main/00442A20.inc"
};
u32 D_00442A60[4] = {
#include "sub_main/00442A60.inc"
};
u32 D_00442A70[2] = {
#include "sub_main/00442A70.inc"
};
u32 D_00442A78[2] = {
#include "sub_main/00442A78.inc"
};
AreaAnim gAreaAnimTbls_64[8] = {
#include "sub_main/00442A80.inc"
};
u32 D_00442AC0[2] = {
#include "sub_main/00442AC0.inc"
};
u32 D_00442AC8[2] = {
#include "sub_main/00442AC8.inc"
};
u32 D_00442AD0[4] = {
#include "sub_main/00442AD0.inc"
};
AreaAnim gAreaAnimTbls_168[8] = {
#include "sub_main/00442AE0.inc"
};
u32 D_00442B20[4] = {
#include "sub_main/00442B20.inc"
};
UvScroll D_00442B30[2] = {
#include "sub_main/00442B30.inc"
};
u32 D_00442B38[2] = {
#include "sub_main/00442B38.inc"
};
f32 D_00442B40[6] = {
#include "sub_main/00442B40.inc"
};
AnimScrollPos D_00442B58[1] = {
    {D_00442B30, D_00442B40},
};
AreaAnim gAreaAnimTbls_169[8] = {
#include "sub_main/00442B60.inc"
};
AreaAnim gAreaAnimTbls_170[4] = {
#include "sub_main/00442BA0.inc"
};
AreaAnim gAreaAnimTbls_171[4] = {
#include "sub_main/00442BC0.inc"
};
AreaAnim gAreaAnimTbls_172[4] = {
#include "sub_main/00442BE0.inc"
};
AreaAnim gAreaAnimTbls_173[4] = {
#include "sub_main/00442C00.inc"
};
AreaAnim gAreaAnimTbls_174[2] = {
#include "sub_main/00442C20.inc"
};
AreaAnim gAreaAnimTbls_175[4] = {
#include "sub_main/00442C30.inc"
};
u32 D_00442C50[4] = {
#include "sub_main/00442C50.inc"
};
u32 D_00442C60[4] = {
#include "sub_main/00442C60.inc"
};
AreaAnim gAreaAnimTbls_177[4] = {
#include "sub_main/00442C70.inc"
};
u32 D_00442C90[8] = {
#include "sub_main/00442C90.inc"
};
AreaAnim gAreaAnimTbls_178[3] = {
#include "sub_main/00442CB0.inc"
};
u32 D_00442CC8[2] = {
#include "sub_main/00442CC8.inc"
};
AreaAnim gAreaAnimTbls_179[14] = {
#include "sub_main/00442CD0.inc"
};
AreaAnim gAreaAnimTbls_180[2] = {
#include "sub_main/00442D40.inc"
};
u32 D_00442D50[4] = {
#include "sub_main/00442D50.inc"
};
u32 D_00442D60[4] = {
#include "sub_main/00442D60.inc"
};
AreaAnim gAreaAnimTbls_181[6] = {
#include "sub_main/00442D70.inc"
};
u32 D_00442DA0[8] = {
#include "sub_main/00442DA0.inc"
};
AreaAnim gAreaAnimTbls_182[4] = {
#include "sub_main/00442DC0.inc"
};
AreaAnim gAreaAnimTbls_185[4] = {
#include "sub_main/00442DE0.inc"
};
AreaAnim gAreaAnimTbls_186[2] = {
#include "sub_main/00442E00.inc"
};
AreaAnim gAreaAnimTbls_190[2] = {
#include "sub_main/00442E10.inc"
};
u32 D_00442E20[4] = {
#include "sub_main/00442E20.inc"
};
AreaAnim gAreaAnimTbls_191[3] = {
#include "sub_main/00442E30.inc"
};
u32 D_00442E48[2] = {
#include "sub_main/00442E48.inc"
};
u32 D_00442E50[4] = {
#include "sub_main/00442E50.inc"
};
AreaAnim gAreaAnimTbls_192[4] = {
#include "sub_main/00442E60.inc"
};
AreaAnim *gAreaAnimTbls[200] = {
    NULL, gAreaAnimTbls_1, gAreaAnimTbls_2, gAreaAnimTbls_3,
    gAreaAnimTbls_4, gAreaAnimTbls_5, gAreaAnimTbls_6, gAreaAnimTbls_7,
    gAreaAnimTbls_8, gAreaAnimTbls_9, NULL, NULL,
    NULL, NULL, gAreaAnimTbls_14, gAreaAnimTbls_6,
    gAreaAnimTbls_16, NULL, gAreaAnimTbls_18, gAreaAnimTbls_19,
    NULL, gAreaAnimTbls_21, gAreaAnimTbls_22, gAreaAnimTbls_23,
    gAreaAnimTbls_24, gAreaAnimTbls_25, NULL, gAreaAnimTbls_27,
    NULL, gAreaAnimTbls_9, NULL, gAreaAnimTbls_31,
    gAreaAnimTbls_32, gAreaAnimTbls_33, gAreaAnimTbls_34, gAreaAnimTbls_35,
    gAreaAnimTbls_36, gAreaAnimTbls_37, gAreaAnimTbls_38, gAreaAnimTbls_39,
    gAreaAnimTbls_40, gAreaAnimTbls_41, gAreaAnimTbls_42, gAreaAnimTbls_43,
    NULL, gAreaAnimTbls_45, gAreaAnimTbls_46, gAreaAnimTbls_47,
    gAreaAnimTbls_48, gAreaAnimTbls_49, gAreaAnimTbls_50, gAreaAnimTbls_51,
    gAreaAnimTbls_52, gAreaAnimTbls_53, gAreaAnimTbls_54, gAreaAnimTbls_55,
    gAreaAnimTbls_56, gAreaAnimTbls_57, gAreaAnimTbls_58, gAreaAnimTbls_59,
    gAreaAnimTbls_60, gAreaAnimTbls_61, gAreaAnimTbls_62, gAreaAnimTbls_63,
    gAreaAnimTbls_64, gAreaAnimTbls_65, gAreaAnimTbls_66, gAreaAnimTbls_67,
    gAreaAnimTbls_68, gAreaAnimTbls_69, gAreaAnimTbls_70, gAreaAnimTbls_71,
    NULL, gAreaAnimTbls_73, gAreaAnimTbls_74, gAreaAnimTbls_75,
    NULL, NULL, NULL, NULL,
    NULL, NULL, gAreaAnimTbls_31, gAreaAnimTbls_83,
    gAreaAnimTbls_84, gAreaAnimTbls_85, gAreaAnimTbls_86, gAreaAnimTbls_87,
    gAreaAnimTbls_88, gAreaAnimTbls_89, gAreaAnimTbls_90, gAreaAnimTbls_91,
    gAreaAnimTbls_92, gAreaAnimTbls_93, gAreaAnimTbls_94, gAreaAnimTbls_6,
    gAreaAnimTbls_96, gAreaAnimTbls_97, gAreaAnimTbls_98, gAreaAnimTbls_99,
    gAreaAnimTbls_100, gAreaAnimTbls_92, gAreaAnimTbls_93, gAreaAnimTbls_94,
    gAreaAnimTbls_6, gAreaAnimTbls_105, gAreaAnimTbls_97, gAreaAnimTbls_98,
    gAreaAnimTbls_99, gAreaAnimTbls_100, gAreaAnimTbls_110, gAreaAnimTbls_111,
    gAreaAnimTbls_3, NULL, NULL, gAreaAnimTbls_18,
    gAreaAnimTbls_19, gAreaAnimTbls_22, gAreaAnimTbls_23, NULL,
    gAreaAnimTbls_120, gAreaAnimTbls_121, gAreaAnimTbls_122, gAreaAnimTbls_123,
    gAreaAnimTbls_124, gAreaAnimTbls_83, NULL, gAreaAnimTbls_127,
    gAreaAnimTbls_128, gAreaAnimTbls_129, NULL, gAreaAnimTbls_131,
    gAreaAnimTbls_132, gAreaAnimTbls_133, gAreaAnimTbls_134, gAreaAnimTbls_135,
    gAreaAnimTbls_136, gAreaAnimTbls_137, gAreaAnimTbls_138, NULL,
    gAreaAnimTbls_140, gAreaAnimTbls_141, gAreaAnimTbls_142, gAreaAnimTbls_143,
    gAreaAnimTbls_144, gAreaAnimTbls_145, gAreaAnimTbls_146, gAreaAnimTbls_53,
    gAreaAnimTbls_54, gAreaAnimTbls_55, gAreaAnimTbls_56, gAreaAnimTbls_16,
    gAreaAnimTbls_152, gAreaAnimTbls_67, gAreaAnimTbls_68, gAreaAnimTbls_69,
    gAreaAnimTbls_70, gAreaAnimTbls_71, gAreaAnimTbls_158, gAreaAnimTbls_73,
    gAreaAnimTbls_160, gAreaAnimTbls_161, gAreaAnimTbls_59, gAreaAnimTbls_163,
    gAreaAnimTbls_164, gAreaAnimTbls_165, gAreaAnimTbls_63, gAreaAnimTbls_64,
    gAreaAnimTbls_168, gAreaAnimTbls_169, gAreaAnimTbls_170, gAreaAnimTbls_171,
    gAreaAnimTbls_172, gAreaAnimTbls_173, gAreaAnimTbls_174, gAreaAnimTbls_175,
    NULL, gAreaAnimTbls_177, gAreaAnimTbls_178, gAreaAnimTbls_179,
    gAreaAnimTbls_180, gAreaAnimTbls_181, gAreaAnimTbls_182, gAreaAnimTbls_179,
    gAreaAnimTbls_21, gAreaAnimTbls_185, gAreaAnimTbls_186, gAreaAnimTbls_34,
    gAreaAnimTbls_35, gAreaAnimTbls_36, gAreaAnimTbls_190, gAreaAnimTbls_191,
    gAreaAnimTbls_192, gAreaAnimTbls_40, gAreaAnimTbls_41, gAreaAnimTbls_42,
    gAreaAnimTbls_43, NULL, NULL, NULL,
};
u32 D_004431A0[2] = {
#include "sub_main/004431A0.inc"
};
u32 D_004431A8[2] = {
#include "sub_main/004431A8.inc"
};
extern f32 D_0092A730[];
extern f32 D_0092A740[];
extern f32 D_0092A770[];
extern f32 D_0092A7B0[];
extern f32 D_0092A7D0[];
extern f32 D_0092A7F0[];
extern f32 D_0092A810[];
extern f32 D_0092A840[];
extern f32 D_0092A860[];
extern f32 D_0092A880[];
extern f32 D_0092A8B0[];
extern f32 D_0092A910[];
extern f32 D_0092A940[];
extern f32 D_0092A950[];
extern f32 D_0092A9A0[];
extern f32 D_0092A9C0[];
extern f32 D_0092A9E0[];
extern f32 D_0092AA30[];
extern f32 D_0092AA80[];
extern f32 D_0092AA90[];
extern f32 D_0092AAB0[];
extern f32 D_0092AAF0[];
extern f32 D_0092AB10[];
extern f32 D_0092AB30[];
extern f32 D_0092AB50[];
extern f32 D_0092AB90[];
extern f32 D_0092ABD0[];
extern f32 D_0092AC40[];
extern f32 D_0092ACA0[];
extern f32 D_0092ACF0[];
extern f32 D_0092AD30[];
extern AnimRot D_0092ABF0[];
extern AnimRot D_0092AC00[];
extern AnimRot D_0092AC10[];
extern AnimRot D_0092AC20[];
extern AnimRot D_0092AC30[];
PropRec gAreaProps_1[8] = {
#include "sub_main/004431B0.inc"
};
u32 D_00443210[4] = {
#include "sub_main/00443210.inc"
};
u32 D_00443220[4] = {
#include "sub_main/00443220.inc"
};
PropRec gAreaProps_7[3] = {
#include "sub_main/00443230.inc"
};
u32 D_00443258[2] = {
#include "sub_main/00443258.inc"
};
PropRec gAreaProps_18[4] = {
#include "sub_main/00443260.inc"
};
u32 D_00443290[4] = {
#include "sub_main/00443290.inc"
};
PropRec gAreaProps_19[4] = {
#include "sub_main/004432A0.inc"
};
u32 D_004432D0[6] = {
#include "sub_main/004432D0.inc"
};
#pragma explicit_zero_data on
u32 D_004432E8[2] = {
    0, 0,
};
#pragma explicit_zero_data reset
PropRec gAreaProps_20[4] = {
#include "sub_main/004432F0.inc"
};
PropRec gAreaProps_23[2] = {
#include "sub_main/00443320.inc"
};
u32 D_00443338[2] = {
#include "sub_main/00443338.inc"
};
u32 D_00443340[4] = {
#include "sub_main/00443340.inc"
};
PropRec gAreaProps_24[4] = {
#include "sub_main/00443350.inc"
};
PropRec gAreaProps_28[2] = {
#include "sub_main/00443380.inc"
};
PropRec gAreaProps_33[4] = {
#include "sub_main/004433A0.inc"
};
PropRec gAreaProps_34[5] = {
#include "sub_main/004433D0.inc"
};
PropRec gAreaProps_36[2] = {
#include "sub_main/00443410.inc"
};
PropRec gAreaProps_38[4] = {
#include "sub_main/00443430.inc"
};
u32 D_00443460[4] = {
#include "sub_main/00443460.inc"
};
PropRec gAreaProps_39[4] = {
#include "sub_main/00443470.inc"
};
u32 D_004434A0[4] = {
#include "sub_main/004434A0.inc"
};
PropRec gAreaProps_43[4] = {
#include "sub_main/004434B0.inc"
};
u32 D_004434E0[2] = {
#include "sub_main/004434E0.inc"
};
u32 D_004434E8[2] = {
#include "sub_main/004434E8.inc"
};
PropRec gAreaProps_45[5] = {
#include "sub_main/004434F0.inc"
};
u32 D_00443530[2] = {
#include "sub_main/00443530.inc"
};
u32 D_00443538[2] = {
#include "sub_main/00443538.inc"
};
PropRec gAreaProps_46[5] = {
#include "sub_main/00443540.inc"
};
u32 D_00443580[2] = {
#include "sub_main/00443580.inc"
};
u32 D_00443588[2] = {
#include "sub_main/00443588.inc"
};
u32 D_00443590[4] = {
#include "sub_main/00443590.inc"
};
PropRec gAreaProps_47[8] = {
#include "sub_main/004435A0.inc"
};
u32 D_00443600[2] = {
#include "sub_main/00443600.inc"
};
u32 D_00443608[2] = {
#include "sub_main/00443608.inc"
};
PropRec gAreaProps_48[8] = {
#include "sub_main/00443610.inc"
};
PropRec gAreaProps_49[4] = {
#include "sub_main/00443670.inc"
};
u32 D_004436A0[2] = {
#include "sub_main/004436A0.inc"
};
u32 D_004436A8[2] = {
#include "sub_main/004436A8.inc"
};
u32 D_004436B0[4] = {
#include "sub_main/004436B0.inc"
};
PropRec gAreaProps_51[9] = {
#include "sub_main/004436C0.inc"
};
u32 D_00443730[8] = {
#include "sub_main/00443730.inc"
};
u32 D_00443750[2] = {
#include "sub_main/00443750.inc"
};
u32 D_00443758[2] = {
#include "sub_main/00443758.inc"
};
u32 D_00443760[4] = {
#include "sub_main/00443760.inc"
};
PropRec gAreaProps_52[12] = {
#include "sub_main/00443770.inc"
};
u32 D_00443800[2] = {
#include "sub_main/00443800.inc"
};
u32 D_00443808[2] = {
#include "sub_main/00443808.inc"
};
u32 gAreaProps_54[20] = {
#include "sub_main/00443810.inc"
};
u32 D_00443860[2] = {
#include "sub_main/00443860.inc"
};
u32 D_00443868[2] = {
#include "sub_main/00443868.inc"
};
u32 D_00443870[4] = {
#include "sub_main/00443870.inc"
};
PropRec gAreaProps_55[8] = {
#include "sub_main/00443880.inc"
};
u32 D_004438E0[2] = {
#include "sub_main/004438E0.inc"
};
u32 D_004438E8[2] = {
#include "sub_main/004438E8.inc"
};
PropRec gAreaProps_57[5] = {
#include "sub_main/004438F0.inc"
};
u32 D_00443930[4] = {
#include "sub_main/00443930.inc"
};
PropRec gAreaProps_60[3] = {
#include "sub_main/00443940.inc"
};
u32 D_00443968[2] = {
#include "sub_main/00443968.inc"
};
PropRec gAreaProps_63[3] = {
#include "sub_main/00443970.inc"
};
u32 D_00443998[2] = {
#include "sub_main/00443998.inc"
};
u32 gAreaProps_65[8] = {
#include "sub_main/004439A0.inc"
};
u32 D_004439C0[2] = {
#include "sub_main/004439C0.inc"
};
u32 D_004439C8[2] = {
#include "sub_main/004439C8.inc"
};
PropRec gAreaProps_66[5] = {
#include "sub_main/004439D0.inc"
};
u32 gAreaProps_75[8] = {
#include "sub_main/00443A10.inc"
};
UvScroll D_00443A30[2] = {
#include "sub_main/00443A30.inc"
};
u32 D_00443A38[2] = {
#include "sub_main/00443A38.inc"
};
f32 D_00443A40[8] = {
#include "sub_main/00443A40.inc"
};
AnimScrollPos D_00443A60[1] = {
    {D_00443A30, D_00443A40},
};
AnimScrollPos D_00443A68[1] = {
    {&D_00443A30[1], &D_00443A40[3]},
};
PropRec gAreaProps_84[5] = {
#include "sub_main/00443A70.inc"
};
u32 D_00443AB0[4] = {
#include "sub_main/00443AB0.inc"
};
PropRec gAreaProps_92[3] = {
#include "sub_main/00443AC0.inc"
};
u32 D_00443AE8[2] = {
#include "sub_main/00443AE8.inc"
};
PropRec gAreaProps_93[4] = {
#include "sub_main/00443AF0.inc"
};
UvScroll D_00443B20[2] = {
#include "sub_main/00443B20.inc"
};
f32 D_00443B28[3] = {
#include "sub_main/00443B28.inc"
};
AnimScrollPos D_00443B38[1] = {
    {&D_00443B20[1], D_00443B28},
};
PropRec gAreaProps_94[4] = {
#include "sub_main/00443B40.inc"
};
u32 D_00443B70[2] = {
#include "sub_main/00443B70.inc"
};
u32 D_00443B78[2] = {
#include "sub_main/00443B78.inc"
};
u32 D_00443B80[4] = {
#include "sub_main/00443B80.inc"
};
PropRec gAreaProps_6[6] = {
#include "sub_main/00443B90.inc"
};
u32 D_00443BD8[2] = {
#include "sub_main/00443BD8.inc"
};
PropRec gAreaProps_96[2] = {
#include "sub_main/00443BE0.inc"
};
PropRec gAreaProps_97[2] = {
#include "sub_main/00443C00.inc"
};
u32 D_00443C18[2] = {
#include "sub_main/00443C18.inc"
};
u32 D_00443C20[4] = {
#include "sub_main/00443C20.inc"
};
PropRec gAreaProps_98[4] = {
#include "sub_main/00443C30.inc"
};
u32 D_00443C60[4] = {
#include "sub_main/00443C60.inc"
};
u32 D_00443C70[2] = {
#include "sub_main/00443C70.inc"
};
u32 D_00443C78[2] = {
#include "sub_main/00443C78.inc"
};
u32 gAreaProps_99[20] = {
#include "sub_main/00443C80.inc"
};
u32 D_00443CD0[2] = {
#include "sub_main/00443CD0.inc"
};
u32 D_00443CD8[2] = {
#include "sub_main/00443CD8.inc"
};
u32 gAreaProps_15[20] = {
#include "sub_main/00443CE0.inc"
};
u32 D_00443D30[2] = {
#include "sub_main/00443D30.inc"
};
u32 D_00443D38[2] = {
#include "sub_main/00443D38.inc"
};
PropRec gAreaProps_107[5] = {
#include "sub_main/00443D40.inc"
};
u32 D_00443D80[4] = {
#include "sub_main/00443D80.inc"
};
u32 D_00443D90[2] = {
#include "sub_main/00443D90.inc"
};
u32 D_00443D98[2] = {
#include "sub_main/00443D98.inc"
};
u32 gAreaProps_108[20] = {
#include "sub_main/00443DA0.inc"
};
u32 D_00443DF0[2] = {
#include "sub_main/00443DF0.inc"
};
u32 D_00443DF8[2] = {
#include "sub_main/00443DF8.inc"
};
u32 D_00443E00[4] = {
#include "sub_main/00443E00.inc"
};
u32 D_00443E10[4] = {
#include "sub_main/00443E10.inc"
};
u32 D_00443E20[4] = {
#include "sub_main/00443E20.inc"
};
PropRec gAreaProps_110[10] = {
#include "sub_main/00443E30.inc"
};
u32 D_00443EB0[4] = {
#include "sub_main/00443EB0.inc"
};
PropRec gAreaProps_111[2] = {
#include "sub_main/00443EC0.inc"
};
PropRec gAreaProps_3[2] = {
#include "sub_main/00443EE0.inc"
};
u32 D_00443F00[4] = {
#include "sub_main/00443F00.inc"
};
PropRec gAreaProps_4[3] = {
#include "sub_main/00443F10.inc"
};
u32 D_00443F38[2] = {
#include "sub_main/00443F38.inc"
};
u32 D_00443F40[4] = {
#include "sub_main/00443F40.inc"
};
PropRec gAreaProps_5[3] = {
#include "sub_main/00443F50.inc"
};
u32 D_00443F78[2] = {
#include "sub_main/00443F78.inc"
};
u32 D_00443F80[4] = {
#include "sub_main/00443F80.inc"
};
PropRec gAreaProps_115[5] = {
#include "sub_main/00443F90.inc"
};
u32 D_00443FD0[4] = {
#include "sub_main/00443FD0.inc"
};
PropRec gAreaProps_116[3] = {
#include "sub_main/00443FE0.inc"
};
u32 D_00444008[2] = {
#include "sub_main/00444008.inc"
};
PropRec gAreaProps_22[4] = {
#include "sub_main/00444010.inc"
};
u32 D_00444040[4] = {
#include "sub_main/00444040.inc"
};
PropRec gAreaProps_118[2] = {
#include "sub_main/00444050.inc"
};
u32 D_00444068[3] = {
#include "sub_main/00444068.inc"
};
PropRec gAreaProps_26[2] = {
#include "sub_main/00444080.inc"
};
u32 D_004440A0[4] = {
#include "sub_main/004440A0.inc"
};
PropRec gAreaProps_120[4] = {
#include "sub_main/004440B0.inc"
};
u32 D_004440E0[4] = {
#include "sub_main/004440E0.inc"
};
u32 D_004440F0[4] = {
#include "sub_main/004440F0.inc"
};
PropRec gAreaProps_121[4] = {
#include "sub_main/00444100.inc"
};
u32 D_00444130[4] = {
#include "sub_main/00444130.inc"
};
PropRec gAreaProps_122[2] = {
#include "sub_main/00444140.inc"
};
u32 D_00444158[2] = {
#include "sub_main/00444158.inc"
};
PropRec gAreaProps_123[3] = {
#include "sub_main/00444160.inc"
};
u32 D_00444188[2] = {
#include "sub_main/00444188.inc"
};
PropRec gAreaProps_126[4] = {
#include "sub_main/00444190.inc"
};
u32 D_004441C0[2] = {
#include "sub_main/004441C0.inc"
};
u32 D_004441C8[2] = {
#include "sub_main/004441C8.inc"
};
PropRec gAreaProps_127[5] = {
#include "sub_main/004441D0.inc"
};
PropRec gAreaProps_131[8] = {
#include "sub_main/00444210.inc"
};
UvScroll D_00444270[2] = {
#include "sub_main/00444270.inc"
};
f32 D_00444278[3] = {
#include "sub_main/00444278.inc"
};
u32 D_00444290[4] = {
#include "sub_main/00444290.inc"
};
u32 D_004442A0[4] = {
#include "sub_main/004442A0.inc"
};
u32 D_004442B0[4] = {
#include "sub_main/004442B0.inc"
};
u32 D_004442C0[4] = {
#include "sub_main/004442C0.inc"
};
u32 D_004442D0[8] = {
#include "sub_main/004442D0.inc"
};
u32 D_004442F0[8] = {
#include "sub_main/004442F0.inc"
};
AnimScrollPos D_00444310[1] = {
    {D_00444270, D_00444278},
};
PropRec gAreaProps_136[8] = {
#include "sub_main/00444320.inc"
};
u32 D_00444380[4] = {
#include "sub_main/00444380.inc"
};
u32 D_00444390[24] = {
#include "sub_main/00444390.inc"
};
u32 D_004443F0[8] = {
#include "sub_main/004443F0.inc"
};
PropRec gAreaProps_137[6] = {
#include "sub_main/00444410.inc"
};
u32 D_00444460[4] = {
#include "sub_main/00444460.inc"
};
u32 D_00444470[4] = {
#include "sub_main/00444470.inc"
};
PropRec gAreaProps_140[4] = {
#include "sub_main/00444480.inc"
};
u32 D_004444B0[4] = {
#include "sub_main/004444B0.inc"
};
u32 D_004444C0[4] = {
#include "sub_main/004444C0.inc"
};
u32 D_004444D0[4] = {
#include "sub_main/004444D0.inc"
};
PropRec gAreaProps_141[4] = {
#include "sub_main/004444E0.inc"
};
PropRec gAreaProps_142[4] = {
#include "sub_main/00444510.inc"
};
u32 D_00444540[4] = {
#include "sub_main/00444540.inc"
};
PropRec gAreaProps_143[5] = {
#include "sub_main/00444550.inc"
};
u32 D_00444590[4] = {
#include "sub_main/00444590.inc"
};
f32 D_004445A0[6] = {
#include "sub_main/004445A0.inc"
};
UvScroll D_004445B8[2] = {
#include "sub_main/004445B8.inc"
};
AnimScrollPos D_004445C0[2] = {
    {D_004445B8, &D_004445A0[3]},
    {NULL, NULL},
};
u32 D_004445D0[4] = {
#include "sub_main/004445D0.inc"
};
PropRec gAreaProps_145[6] = {
#include "sub_main/004445E0.inc"
};
u32 D_00444630[4] = {
#include "sub_main/00444630.inc"
};
u32 D_00444640[8] = {
#include "sub_main/00444640.inc"
};
u32 D_00444660[4] = {
#include "sub_main/00444660.inc"
};
PropRec gAreaProps_146[6] = {
#include "sub_main/00444670.inc"
};
u32 D_004446C0[4] = {
#include "sub_main/004446C0.inc"
};
PropRec gAreaProps_148[2] = {
#include "sub_main/004446D0.inc"
};
u32 D_004446E8[2] = {
#include "sub_main/004446E8.inc"
};
u32 D_004446F0[4] = {
#include "sub_main/004446F0.inc"
};
PropRec gAreaProps_149[3] = {
#include "sub_main/00444700.inc"
};
u32 D_00444728[2] = {
#include "sub_main/00444728.inc"
};
PropRec gAreaProps_158[4] = {
#include "sub_main/00444730.inc"
};
u32 D_00444760[4] = {
#include "sub_main/00444760.inc"
};
PropRec gAreaProps_159[2] = {
#include "sub_main/00444770.inc"
};
UvScroll D_00444788[2] = {
#include "sub_main/00444788.inc"
};
AnimScrollRot D_00444790[2] = {
    {D_00444788, D_0092ABF0},
    {D_00444788, D_0092AC00},
};
AnimScrollRot D_004447A0[3] = {
    {&D_00444788[1], D_0092AC10},
    {&D_00444788[1], D_0092AC20},
    {&D_00444788[1], D_0092AC30},
};
PropRec gAreaProps_160[5] = {
#include "sub_main/004447C0.inc"
};
u32 D_00444800[2] = {
#include "sub_main/00444800.inc"
};
u32 D_00444808[2] = {
#include "sub_main/00444808.inc"
};
PropRec gAreaProps_59[4] = {
#include "sub_main/00444810.inc"
};
u32 D_00444840[4] = {
#include "sub_main/00444840.inc"
};
PropRec gAreaProps_163[4] = {
#include "sub_main/00444850.inc"
};
PropRec gAreaProps_61[2] = {
#include "sub_main/00444880.inc"
};
PropRec gAreaProps_62[2] = {
#include "sub_main/004448A0.inc"
};
u32 D_004448B8[2] = {
#include "sub_main/004448B8.inc"
};
PropRec gAreaProps_166[3] = {
#include "sub_main/004448C0.inc"
};
s16 D_004448E8[4] = {
#include "sub_main/004448E8.inc"
};
PropRec gAreaProps_170[4] = {
#include "sub_main/004448F0.inc"
};
s16 D_00444920[8] = {
#include "sub_main/00444920.inc"
};
PropRec gAreaProps_171[4] = {
#include "sub_main/00444930.inc"
};
s16 D_00444960[8] = {
#include "sub_main/00444960.inc"
};
PropRec gAreaProps_172[4] = {
#include "sub_main/00444970.inc"
};
u8 D_004449A0[16] = {
#include "sub_main/004449A0.inc"
};
u32 D_004449B0[4] = {
#include "sub_main/004449B0.inc"
};
PropRec gAreaProps_173[9] = {
#include "sub_main/004449C0.inc"
};
u32 D_00444A30[4] = {
#include "sub_main/00444A30.inc"
};
u32 D_00444A40[4] = {
#include "sub_main/00444A40.inc"
};
PropRec gAreaProps_174[12] = {
#include "sub_main/00444A50.inc"
};
u32 D_00444AE0[4] = {
#include "sub_main/00444AE0.inc"
};
PropRec gAreaProps_175[11] = {
#include "sub_main/00444AF0.inc"
};
u32 D_00444B78[2] = {
#include "sub_main/00444B78.inc"
};
PropRec gAreaProps_176[2] = {
#include "sub_main/00444B80.inc"
};
PropRec gAreaProps_192[2] = {
#include "sub_main/00444BA0.inc"
};
PropRec *gAreaProps[200] = {
    NULL, gAreaProps_1, NULL, gAreaProps_3,
    gAreaProps_4, gAreaProps_5, gAreaProps_6, gAreaProps_7,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, (PropRec *)gAreaProps_15,
    NULL, NULL, gAreaProps_18, gAreaProps_19,
    gAreaProps_20, NULL, gAreaProps_22, gAreaProps_23,
    gAreaProps_24, NULL, gAreaProps_26, NULL,
    gAreaProps_28, NULL, NULL, NULL,
    NULL, gAreaProps_33, gAreaProps_34, NULL,
    gAreaProps_36, NULL, gAreaProps_38, gAreaProps_39,
    NULL, NULL, NULL, gAreaProps_43,
    NULL, gAreaProps_45, gAreaProps_46, gAreaProps_47,
    gAreaProps_48, gAreaProps_49, NULL, gAreaProps_51,
    gAreaProps_52, NULL, (PropRec *)gAreaProps_54, gAreaProps_55,
    NULL, gAreaProps_57, NULL, gAreaProps_59,
    gAreaProps_60, gAreaProps_61, gAreaProps_62, gAreaProps_63,
    NULL, (PropRec *)gAreaProps_65, gAreaProps_66, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, (PropRec *)gAreaProps_75,
    NULL, NULL, NULL, NULL,
    gAreaProps_28, NULL, NULL, NULL,
    gAreaProps_84, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    gAreaProps_92, gAreaProps_93, gAreaProps_94, gAreaProps_6,
    gAreaProps_96, gAreaProps_97, gAreaProps_98, (PropRec *)gAreaProps_99,
    NULL, gAreaProps_92, gAreaProps_93, gAreaProps_94,
    (PropRec *)gAreaProps_15, NULL, NULL, gAreaProps_107,
    (PropRec *)gAreaProps_108, NULL, gAreaProps_110, gAreaProps_111,
    gAreaProps_3, gAreaProps_4, gAreaProps_5, gAreaProps_115,
    gAreaProps_116, gAreaProps_22, gAreaProps_118, gAreaProps_26,
    gAreaProps_120, gAreaProps_121, gAreaProps_122, gAreaProps_123,
    NULL, NULL, gAreaProps_126, gAreaProps_127,
    NULL, NULL, NULL, gAreaProps_131,
    NULL, NULL, NULL, NULL,
    gAreaProps_136, gAreaProps_137, NULL, NULL,
    gAreaProps_140, gAreaProps_141, gAreaProps_142, gAreaProps_143,
    NULL, gAreaProps_145, gAreaProps_146, NULL,
    gAreaProps_148, gAreaProps_149, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, gAreaProps_158, gAreaProps_159,
    gAreaProps_160, NULL, gAreaProps_59, gAreaProps_163,
    gAreaProps_61, gAreaProps_62, gAreaProps_166, NULL,
    NULL, NULL, gAreaProps_170, gAreaProps_171,
    gAreaProps_172, gAreaProps_173, gAreaProps_174, gAreaProps_175,
    gAreaProps_176, NULL, NULL, NULL,
    gAreaProps_176, NULL, NULL, NULL,
    NULL, NULL, gAreaProps_33, gAreaProps_34,
    NULL, gAreaProps_36, NULL, gAreaProps_38,
    gAreaProps_192, NULL, NULL, NULL,
    gAreaProps_43, NULL, NULL, NULL,
};
u32 D_00444EE0[16] = {
#include "sub_main/00444EE0.inc"
};
PropRec D_00444F20[8] = {
#include "sub_main/00444F20.inc"
};
u32 D_00444F80[4] = {
#include "sub_main/00444F80.inc"
};
u32 D_00444F90[12] = {
#include "sub_main/00444F90.inc"
};
PropRec D_00444FC0[8] = {
#include "sub_main/00444FC0.inc"
};
u32 D_00445020[12] = {
#include "sub_main/00445020.inc"
};
PropRec D_00445050[8] = {
#include "sub_main/00445050.inc"
};
u32 D_004450B0[4] = {
#include "sub_main/004450B0.inc"
};
PropRec D_004450C0[8] = {
#include "sub_main/004450C0.inc"
};
u32 D_00445120[8] = {
#include "sub_main/00445120.inc"
};
PropRec D_00445140[8] = {
#include "sub_main/00445140.inc"
};
PropRec D_004451A0[5] = {
#include "sub_main/004451A0.inc"
};
u32 D_004451E0[4] = {
#include "sub_main/004451E0.inc"
};
u32 D_004451F0[4] = {
#include "sub_main/004451F0.inc"
};
u32 D_00445200[4] = {
#include "sub_main/00445200.inc"
};
u32 D_00445210[4] = {
#include "sub_main/00445210.inc"
};
PropRec D_00445220[9] = {
#include "sub_main/00445220.inc"
};
u32 D_00445290[20] = {
#include "sub_main/00445290.inc"
};
PropRec D_004452E0[12] = {
#include "sub_main/004452E0.inc"
};
u32 D_00445370[32] = {
#include "sub_main/00445370.inc"
};
u32 D_004453F0[12] = {
#include "sub_main/004453F0.inc"
};
PropRec D_00445420[13] = {
#include "sub_main/00445420.inc"
};
PropChoice gVillagePropChoices_0[10] = {
#include "sub_main/004454C0.inc"
};
AreaProps gVillagePropChoices[2] = {
#include "sub_main/00445510.inc"
};
char *gDefaultStageHeader_12[8] = {
    D_00490B70, D_00490B80, D_00490BA0, D_00490BB0,
    D_00490BC0, gEmClassNames_0, NULL, NULL,
};
char *gReturn20sTownText[2] = {
    D_00490BE0, D_00490C00,
};
char *gReturn1mTownText[2] = {
    D_00490C20, D_00490C40,
};
char *gQuestClearText[2] = {
    D_00490C60, D_00490C80,
};
char *gTimeLeftMinFmt[2] = {
    D_00490CA0, D_00490CB8,
};
char *gQuestNoticeTexts[8] = {
    D_00490CD0, D_00490CF0, D_00490D08, D_00490D20,
    D_00490D40, D_00490D60, D_00490D80, D_00490DA0,
};
char *gSubBSupplyText[4] = {
    D_00490DC0, D_00490DE0, D_00490E00, D_00490E20,
};
char *gMustOrderAllText[2] = {
    D_00490E40, D_00490E70,
};
char *gReturn10sTownText[2] = {
    D_00490EA0, D_00490EC0,
};
char *gDrillScoreTexts[16] = {
    D_00490EE0, D_00490EF0, D_00490F20, D_00490F40,
    D_00490EE0, D_00490EE0, D_00490EE0, D_00490EE0,
    D_00490EE0, D_00490EE0, D_00490EE0, D_00490F60,
    D_00490EE0, D_00490F90, D_00490FB0, D_00490FD0,
};
char *gElderFledText[4] = {
    D_00490FF0, NULL, NULL, NULL,
};
char *gEmClassNames[12] = {
    gEmClassNames_0, D_00491010, D_00491018, D_00491020,
    D_00491028, D_00491030, D_00491038, D_00491040,
    D_00491048, D_00491050, NULL, NULL,
};
char *gMonsterNames[76] = {
    D_00491058, D_00491068, D_00491078, D_00491088,
    D_00491090, D_00491098, D_004910A8, D_004910B8,
    D_004910C8, D_004910D8, D_004910E8, D_004910F0,
    D_00491100, D_00491110, D_00491120, D_00491130,
    D_00491140, D_00491150, D_00491160, D_00491170,
    D_00491180, D_00491190, D_004911A0, D_004911B0,
    D_004911C0, D_004911D0, D_004911E0, D_004911F0,
    D_00491200, D_00491210, D_00491218, D_00491228,
    D_00491238, D_00491240, D_00491248, D_00491140,
    D_00491078, D_00491260, D_00491280, D_004912A0,
    D_004912B8, D_004912D0, D_00491260, D_004912F0,
    D_00491310, D_00491330, D_00491350, D_00491370,
    D_00491390, D_004912D0, D_004910B8, D_004913A8,
    D_004913B8, D_004913C8, D_004913D8, D_004913E8,
    D_004913F8, D_00491408, D_00491418, D_00491428,
    D_004913D8, D_00491438, D_00491448, D_00491450,
    D_00491460, D_00491480, D_00491498, D_004914B0,
    D_004914C8, D_004914D8, D_004914E8, D_00491078,
    D_00491418, D_004914F0, NULL, NULL,
};
char *gFieldMenuItemsOnline[2][7] = {
    {D_00491500, D_00491510, D_00491518, D_00491528, D_00491538, D_00491548, D_00491558},
    {D_00491500, D_00491568, D_00491518, D_00491528, D_00491538, D_00491548, D_00491558},
};
char *gFieldMenuItemsOffline[5][9] = {
    {D_00491500, D_00491510, D_00491518, D_00491528, D_00491538, D_00491578, D_00491558, D_00491588, D_00491598},
    {D_00491500, D_004915A8, D_004915B8, D_00491528, D_00491538, D_00491578, D_00491558, D_00491588, D_00491598},
    {D_00491500, D_00491510, D_00491518, D_00491528, D_00491538, D_00491578, D_00491558, D_004915C8, D_00491598},
    {D_00491500, D_004915A8, D_004915B8, D_00491528, D_00491538, D_00491578, D_00491558, D_004915C8, D_00491598},
    {D_004915D8, D_00491510, D_004915B8, D_00491528, D_00491538, D_00491578, D_00491558, D_004915E8, D_00491598},
};
char *gPouchMenuPanel2_6[2] = {
    D_004915F8, D_00491600,
};
char *gDeliveryPanel_7[2] = {
    D_00491608, D_00491610,
};
char *gYesNoTexts[2] = {
    D_00491620, D_00491640,
};
char *gItemActionLabels[3][3] = {
    {D_00491660, D_00491668, D_00491670},
    {D_00491660, D_00491680, D_00491690},
    {D_004916A0, D_00491668, D_00491670},
};
char *D_00445888[2] = {
    D_004916B0, D_00491660,
};
char *gStatusMenuColors_1[6] = {
    D_004916C0, D_004916D0, D_004916E0, D_004916F0,
    D_00491710, NULL,
};
char *gInfoMenuLabelsOffline[2] = {
    D_00491728, D_00491740,
};
char *gInfoMenuLabels[4] = {
    D_00491758, D_00491728, D_00491740, D_00491768,
};
char *gCockpitToggleTexts[4] = {
    D_00491780, D_004917A0, NULL, NULL,
};
char *gQuestStatusTitle[2] = {
    D_004917B0, D_004917D0,
};
char *gQuestMenuRecordTitles[2] = {
    D_004917F0, D_00491820,
};
char *gQuestMenuTitles[2] = {
    D_00491850, D_00491880,
};
char *gQuestMenuStatusTexts[2] = {
    D_004918A8, D_004918B0,
};
char *gQuestMenuTargetTexts[2] = {
    D_004918C0, D_004918D0,
};
char *gDojoNoRecordText[2] = {
    D_004918E0, D_004918F0,
};
char *gQuestInfoFormats[8] = {
    D_00491900, D_00491908, D_00491918, D_00491930,
    D_00491950, D_00491970, NULL, NULL,
};
char *gQuestMenuItemLists[10][17] = {
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491998},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_004919B0, gEmClassNames_0, D_004919C8, gEmClassNames_0, D_004919E0, gEmClassNames_0, gEmClassNames_0, D_00491A00, gEmClassNames_0, gEmClassNames_0, D_00491A18, NULL, NULL},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A28, D_00491A38, D_00491A48, D_00491A58, D_00491A68, gEmClassNames_0, D_00491A78, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A88, NULL, NULL},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A98, D_00491AA8, gEmClassNames_0, D_00491AB8, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491AD0, D_00491AF0, D_00491B08, D_00491B18, D_00491B28, gEmClassNames_0, D_00491A78, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A88, NULL, NULL},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, D_00491A98, D_00491AA8, D_00491B38, D_00491B48, D_00491AB8, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, D_00491B60, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A28, D_00491A38, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A78, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A88, NULL, NULL},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491B80, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_004919C8, gEmClassNames_0, D_00491A18, NULL, NULL, NULL, NULL, NULL},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A28, D_00491A38, D_00491B98, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A78, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A88, NULL, NULL},
};
char *gQuestTargetLabels[3] = {
    D_00491BB0, D_00491950, D_00491C10,
};
#pragma explicit_zero_data on
u32 D_00445BD8[2] = {
    0, 0,
};
#pragma explicit_zero_data reset
char *gCombineSelectFmt[2] = {
    D_00491C40, gEmClassNames_0,
};
char *gCombineInfoLabels[2] = {
    D_00491C60, D_00491C88,
};
char *gCombineRateFmt[2] = {
    D_00491C90, D_00491C98,
};
char *gCombineSelectingText[2] = {
    D_00491CA0, D_00491CB0,
};
char *gCombineFailLabels[2] = {
    D_00491CE0, NULL,
};
char *gCombine2Text[2] = {
    D_00491D08, D_00491D18,
};
char *gCombine2MaterialLabels[4] = {
    D_00491D28, D_00491D38, NULL, NULL,
};
char *gCombine3MaterialLabels[4] = {
    D_00491D28, D_00491D38, D_00491D48, NULL,
};
char *gCombineDoText[2] = {
    D_00491D58, D_00491610,
};
char *gPouchInfiniteFmt[2] = {
    D_00491D68, NULL,
};
char *gAmmoTensFmt[2] = {
    D_00491D70, NULL,
};
char *gItemFullText[2] = {
    D_00491D80, D_00491DA0,
};
char *gFieldMenuTitle[2] = {
    D_00491DC0, NULL,
};
char *gPausedText[2] = {
    D_00491DD8, NULL,
};
char *gPhraseEditFmt[2] = {
    D_00491DF0, NULL,
};
char *gChatMenuPanel_7[2] = {
    D_00491E08, D_00491578,
};
char *gSupplySelectText[2] = {
    D_00491E20, D_00491E40,
};
char *gSupplyCannotCarryText[2] = {
    D_00491E60, D_00491D68,
};
char *gSupplyCountFmt[4] = {
    D_00491E88, NULL, NULL, NULL,
};
char *gTentMenuLabels[18] = {
    D_00491E90, D_00491EB0, D_00491ED0, D_00491E90,
    D_00491EB0, D_00491ED0, D_00491E90, D_00491EF0,
    D_00491ED0, D_00491E90, D_00491EF0, D_00491ED0,
    D_00491E90, D_00491F10, D_00491ED0, D_00491E90,
    D_00491F10, D_00491ED0,
};
char *gSubTargetHelpText[2] = {
    D_00491F30, D_00491FF0,
};
char *gQuestMenuHelpTexts[4] = {
    D_00492090, D_00492160, NULL, NULL,
};
char *gDeliveryMenuTexts[4] = {
    D_00492228, D_00492238, D_00492248, D_00492258,
};
char *gDeliveryDoneText[2] = {
    D_00492268, D_00492270,
};
char *gDeliveryHelpText[2] = {
    D_00492290, NULL,
};
char *gCombineListFmt[2] = {
    D_00492340, D_00492368,
};
char *gItemListLineTexts[2] = {
    D_00492370, D_00492390,
};
char *gRecipeMaterialsFmt[2] = {
    D_004923B0, D_004923D0,
};
char *gRecipeSwitchHint[2] = {
    D_004923E8, NULL,
};
char *gRecipeMaterialFmts[4] = {
    D_004923F8, D_00492408, D_00492418, NULL,
};
char gRecipeRateLines[0x10] = {
#include "sub_main/00445D40.inc"
};
char *gRecipeRateLabels[8] = {
    D_00492448, D_00492450, D_00492458, D_00492460,
    D_00492468, D_00492470, NULL, NULL,
};
char *gRecipeYieldLabels[12] = {
    D_00492478, D_00492480, D_00492488, D_00492488,
    D_00492490, D_00492498, D_004924A0, D_00492498,
    D_004924A8, NULL, NULL, NULL,
};
char *gMonsterListTitle[4] = {
    D_004924B0, D_004924C8, D_004924D0, D_004924E8,
};
char *gMonsterNoSizeText[2] = {
    D_004924F0, D_00492500,
};
char *gArenaBaitNoBookText[2] = {
    D_00492520, D_00492540,
};
char *gMonsterNoteLabels[2] = {
    D_00492560, D_00492580,
};
char *gArenaBaitHint[2] = {
    D_004925A0, D_004925C0,
};
char *gArenaBaitLabels[2] = {
    D_004925E0, D_00492620,
};
char *gMonsterNoteValueFmt[2] = {
    D_00492628, NULL,
};
char *gEmDescTexts[52] = {
    D_00492630, D_00492710, D_00492820, D_00492920,
    D_004929F0, D_00492AE0, D_00492BC0, D_00492CB0,
    D_00492DC0, D_00492EB0, D_00492F70, D_00493040,
    D_00493110, D_004931F0, D_004932D0, D_004933B0,
    D_00493460, D_00493500, D_004935C0, D_00493660,
    D_00493730, D_00493830, D_00493910, D_004939F0,
    D_00493AE0, D_00493BE0, D_00493CB0, D_00493D70,
    D_00493E30, D_00493F10, D_00493FD0, D_00494090,
    D_00494150, D_00494220, D_00494310, D_004943C0,
    D_004944B0, D_004945B0, D_004946A0, D_00494780,
    D_00494850, D_00494990, D_00494A60, D_00494B50,
    D_00494C20, D_00494D00, D_00494DF0, D_00494ED0,
    D_00494FD0, D_004950D0, D_00495180, D_00495270,
};
char *gEquipStatusTitle[2] = {
    D_00495550, NULL,
};
char *gOptionMenuTitle[2] = {
    D_00495570, NULL,
};
char *gFieldOptionLabels[2][9] = {
    {D_00495588, D_00495598, D_004955B0, D_004955C8, D_004955D8, D_004955E8, D_004955F8, D_00495608, D_00495620},
    {D_00495588, D_00495640, D_004955B0, D_004955C8, D_004955D8, D_004955E8, D_004955F8, D_00495608, D_00495620},
};
char *gFieldOptionValues[9][3] = {
    {D_00495658, D_00495660, gEmClassNames_0},
    {D_00495658, D_00495660, gEmClassNames_0},
    {D_00495668, D_00495670, D_00495680},
    {D_00495690, D_00495698, gEmClassNames_0},
    {D_00495668, D_00495670, D_00495680},
    {D_00495660, D_00495658, gEmClassNames_0},
    {D_004956A0, D_004956A8, D_004956B0},
    {D_004956B8, D_004956C0, gEmClassNames_0},
    {D_00495658, D_00495660, gEmClassNames_0},
};
char *gRarityFmt[8] = {
    D_004956C8, NULL, D_00491548, D_00491C30,
    D_004956D0, D_004956E8, D_004956F8, NULL,
};
char *gChatRecvText[4] = {
    D_00495708, D_00495718, NULL, NULL,
};
char *gTalkEventTitle[2] = {
    D_00495728, D_00491620,
};
char *gNoticeNoSelText[2] = {
    D_00495740, D_00495760,
};
char *gItemListTitles[2] = {
    D_00491500, D_00495768,
};
char *gItemListTitleFmt[2] = {
    D_00495780, D_004957A0,
};
char *gInfinityText[4] = {
    D_004957B8, NULL, NULL, NULL,
};
char *gStatusHeadLabels[4] = {
    D_004957C0, D_004957F0, D_00495820, D_00495850,
};
char *gSymbolColorLabel[2] = {
    D_00495890, D_004958B0,
};
char *gArenaRecordLabels[2] = {
    D_004958D0, D_00495920,
};
char *gArenaRecordFmt[4] = {
    D_00495940, D_00495950, NULL, NULL,
};
char *gStatusStatLabels[8] = {
    D_00495960, D_00495980, D_004959A0, D_004959C0,
    D_004959E0, NULL, NULL, NULL,
};
char *gWeaponTypeNames[12] = {
    D_00495A00, D_00495A08, D_00495A18, D_00495A28,
    D_00495A30, D_00495A38, D_00495A48, D_00495A50,
    D_00495A58, D_00495A60, D_00495A70, NULL,
};
char *gStatusBoxTitles[8] = {
    D_00495A78, gEmClassNames_0, D_00495A88, D_00495A88,
    D_00495A88, D_00495A88, D_00495A88, D_00495A88,
};
char *gStatusPageFmt[2] = {
    D_00495AA0, D_00495AD0,
};
char *gRankPointsFmt[2] = {
    D_00495AD8, D_00495AE8,
};
char *gSkillNameFmt[2] = {
    D_00495AF0, D_00495B00,
};
char *gSkillPartPtsFmt[2] = {
    D_00495B10, D_00495B30,
};
char *gSkillRowFmt[6] = {
    D_00495B50, D_00495B70, D_00495B80, D_00495BA8,
    D_00495BC0, D_00495BD0,
};
char *gPointsFmt[2] = {
    D_00491900, NULL,
};
char *gSkillListHeader[2] = {
    D_00495C00, NULL,
};
char *gSkillPartMarks[2] = {
    D_00495C28, D_00495C30,
};
char *gEquipWeaponFmt[2] = {
    D_00495C38, D_00495C38,
};
char *gUnknownEquipText[2] = {
    D_00495C48, D_00495C58,
};
char *gHunterTypeLabels[4] = {
    D_00495C70, D_00495C88, D_00495C98, NULL,
};
char *gEquipTypeLabels[4] = {
    D_00495CB0, D_00495C88, D_00495C98, NULL,
};
char gBladeDetailLabels[0x10] = {
#include "sub_main/00446100.inc"
};
u32 gBowgunDetailLabels[6] = {
#include "sub_main/00446110.inc"
};
char *gBowDetailLabels[2] = {
    D_00495CC8, NULL,
};
char *gArmorStatLabels[8] = {
    D_00495D20, D_00495D40, D_00495D60, D_00495D80,
    NULL, NULL, NULL, NULL,
};
char *gGunReloadTexts[8] = {
    D_00495D90, D_00495D98, D_00495DA8, D_00495DB0,
    D_00495DC0, NULL, NULL, NULL,
};
char *gGunRecoilTexts[8] = {
    D_00495DC8, D_00495DD0, D_00495DD8, D_00495DE0,
    D_00495DE8, D_00495DF0, D_00495DF8, NULL,
};
char *gHunterClassTexts[4] = {
    D_00495E00, D_00495E08, D_00495E18, D_00495E28,
};
char *gEquipNameFmt[2] = {
    D_00495E38, D_00495E50,
};
char *gScopeFixedText[2] = {
    D_00495E70, D_00495E88,
};
char *gPowerBarrelText[2] = {
    D_00495E98, D_00495EA8,
};
char *gShieldText[2] = {
    D_00495EB8, D_00495EC8,
};
char *gAmmoHeaderText[6] = {
    D_00495ED0, D_00495EF8, D_00495F00, D_00495F10,
    D_00495F28, D_00495F40,
};
char *gAmmoNameFmt[2] = {
    D_00495F60, D_00495F78,
};
char *gAmmoLineFormats[6] = {
    D_00495F90, D_00495FB0, D_00495FE0, D_00495FF8,
    D_00496010, D_00496030,
};
char *gDecoDetailTitle[2] = {
    D_00496060, D_00496080,
};
char *gAffinityLabel[4] = {
    D_00496098, D_004960A8, NULL, NULL,
};
char *gAmmoNames[20] = {
    D_004960B0, D_004960C0, D_004960D0, D_004960E0,
    D_004960F0, D_00496100, D_00496108, D_00496110,
    D_00496118, D_00496120, D_00496128, D_00496130,
    D_00496138, D_00496140, D_00496148, D_00496158,
    D_00496168, D_00496178, NULL, NULL,
};
char *gElementNames[8] = {
    D_00496188, D_00496190, D_00496198, D_004961A0,
    D_004961A8, D_004961B0, D_004961B8, D_004961C0,
};
char *gSkillPointsTitle[2] = {
    D_004961D0, D_004961F0,
};
char *gNoSkillText[2] = {
    D_00496208, NULL,
};
char *gDecoDetailLabels[4] = {
    D_00496218, D_00496228, D_00496238, NULL,
};
char *gRangeClassTexts[4] = {
    D_00496248, D_00496250, D_00496258, D_00496268,
};
char *gDecoSkillPtsFmt[2] = {
    D_00496278, D_00496280,
};
char *gDecoSkillFmt[2] = {
    D_00496288, NULL,
};
char *gShellTypeTexts[4] = {
    D_00496298, D_004962A8, D_004962B8, NULL,
};
char *gBowShotTexts[12] = {
    D_004962C8, D_004962D0, D_004962D8, D_004962E0,
    D_004962E8, D_004962F0, D_004962F8, D_00496300,
    D_00496308, D_00496310, D_00496318, D_00496320,
};
char *gTabPageFmt[2] = {
    D_00496328, D_00496330,
};
char *gSortHintText[2] = {
    D_00496338, D_00496340,
};
char *gSelectText[2] = {
    D_00496348, D_00492620,
};
char *gDecoModeFmt[2] = {
    D_00496350, NULL,
};
char *gCommentEditTitle[2] = {
    D_00496360, NULL,
};
char *gCommentEditText[2] = {
    D_00496378, NULL,
};
char *gChatTargetTexts[4] = {
    D_00496388, D_00496398, D_004963A0, D_004963D0,
};
char *gChatNgWordsA[44] = {
    D_004963F0, D_00496400, D_00496410, D_00496420,
    D_00496430, D_00496440, D_00496450, D_00496460,
    D_00496470, D_00496480, D_00496490, D_004964A0,
    D_004964B0, D_004964C0, D_004964D0, D_004964E0,
    D_004964F0, D_004964F8, D_00496500, D_00496508,
    D_00496510, D_00496518, D_00496520, D_00496528,
    D_00496530, D_00496538, D_00496540, D_00496548,
    D_00496550, D_00496558, D_00496568, D_00496570,
    D_00496578, D_00496580, D_00496588, D_00496590,
    D_00496598, D_004965A8, D_004965B0, D_004965B8,
    D_004965C0, D_004965C8, D_004965D0, NULL,
};
char *gChatNgWordsB[92] = {
    D_004965D8, D_004965E0, D_004965E8, D_004965F0,
    D_004965F8, D_00496600, D_00496608, D_00496610,
    D_00496618, D_00496620, D_00496628, D_00496638,
    D_00496648, D_00496650, D_00496658, D_00496660,
    D_00496668, D_00496670, D_00496678, D_00496680,
    D_00496688, D_00496690, D_00496698, D_004966A8,
    D_004966B8, D_004966C0, D_004966C8, D_004966D0,
    D_004966D8, D_004966E0, D_004966E8, D_004966F0,
    D_004966F8, D_00496700, D_00496710, D_00496720,
    D_00496728, D_00496730, D_00496738, D_00496740,
    D_00496748, D_00496758, D_00496768, D_00496770,
    D_00496778, D_00496788, D_00496798, D_004967A0,
    D_004967A8, D_004967B0, D_004967C0, D_004967D0,
    D_004967E0, D_004967F0, D_004967F8, D_00496800,
    D_00496808, D_00496810, D_00496818, D_00496820,
    D_00496830, D_00496840, D_00496850, D_00496860,
    D_00496870, D_00496880, D_00496890, D_004968A0,
    D_004968B0, D_004968B8, D_004968C8, D_004968D8,
    D_004968E8, D_004968F8, D_00496900, D_00496908,
    D_00496918, D_00496928, D_00496938, D_00496948,
    D_00496958, D_00496968, D_00496978, D_00496988,
    D_00496998, D_004969A8, D_004969B8, D_004969C8,
    D_004969D8, NULL, NULL, NULL,
};
char gDefaultPhrases[24][45] = {
#include "sub_main/00446560.inc"
};
char *D_004469A0[8] = {
    D_004969E8, D_00496A00, D_00496A20, D_00496A38,
    D_00492620, D_00496328, NULL, NULL,
};
char *gNoticeBannerTexts[32] = {
    D_00496A50, D_00496A70, D_00496A88, D_00496AA0,
    D_00496AC0, D_00496AE0, D_00496B00, D_00496B20,
    D_00496B40, D_00496B60, D_00496B80, D_00496BA0,
    D_00496BC0, D_00496BE0, D_00496C00, D_00496C20,
    D_00496C50, D_00496C70, D_00496C90, D_00496CB0,
    D_00496CD0, D_00496D00, D_00496D20, D_00496D40,
    D_00496D60, D_00496D80, D_00496DA0, D_00496DD0,
    D_00496DF0, D_00496E10, D_00496E30, D_00496E48,
};
char *gNoticeBannerItemTexts[24] = {
    D_00496E58, D_00496E68, D_00496E78, D_00496E90,
    D_00496EA8, D_00496EB8, D_00496EC0, D_00496ED0,
    D_00496EE8, D_00496EF8, D_00496F10, D_00496F28,
    D_00496F38, D_00496F48, D_00496F60, D_00496F80,
    D_00496FA8, D_00496FB8, D_00496FD0, D_00496FF0,
    D_00497010, NULL, NULL, NULL,
};
char *gNoticeBannerTextsItem[12] = {
    D_00497030, D_00496EF8, D_00496F10, D_00496F28,
    D_00496F48, D_00496F80, D_00496FA8, D_00496E90,
    D_00496F38, NULL, NULL, NULL,
};
char *gNoticeBannerPlayerTexts[4] = {
    D_00497048, D_00497060, D_00497080, D_004970A0,
};
char *gNoticeMadeTexts[10] = {
    D_004970B0, D_004970B8, NULL, NULL,
    D_004970D0, D_00497100, D_00497120, D_00497138,
    D_00497120, D_00497150,
};
char *gSharpnessDownText[2] = {
    D_00497170, D_00497190,
};
char *D_00446B10[2] = {
    D_004971B0, NULL,
};
char *D_00446B18[2] = {
    D_004971D0, D_004971F0,
};
char *D_00446B20[4] = {
    D_00497210, D_00497230, NULL, NULL,
};
EmTraderMsg gEmTraderMsgs[106] = {
#include "sub_main/00446B30.inc"
};
char *gStageNames[80] = {
    D_00499CC8, D_00499CD0, D_00499CD8, D_00499CE0,
    D_00499CE8, D_00499CF0, D_00499CF8, D_00499D00,
    D_00499D10, D_00499D18, D_00499D28, D_00499D30,
    D_00499D38, D_00499D40, D_00499D40, D_00499D40,
    D_00499CD8, D_00499CE0, D_00499CE8, D_00499CF0,
    D_00499CF8, D_00499D30, D_00499D38, D_00499CD0,
    D_00499D18, D_00499D28, NULL, NULL,
    D_00499D48, D_00499D48, D_00499D50, D_00499D80,
    D_00499D48, D_00499D48, D_00499D48, D_00499DB0,
    D_00499DB8, D_00499DC0, D_00499DD0, D_00499DD8,
    D_00499DE0, D_00499DE8, D_00499DF0, D_00499DF8,
    D_00499E00, D_00499E08, D_00499E10, D_00499E20,
    D_00499E40, D_00499E60, D_00499E80, D_00499EA0,
    D_00499EC0, D_00499EF0, D_00499F48, D_00499F60,
    D_00491900, D_00499F78, D_00499F80, D_00499FA0,
    D_00491500, D_004916C0, D_004916D0, D_00491528,
    D_00499FC0, NULL, NULL, NULL,
    D_00491500, D_004916C0, D_004916D0, D_00491528,
    D_00499FD0, NULL, NULL, NULL,
    D_00499FE0, D_0049A020, D_0049A060, NULL,
};
char *gQuestKindNames[96] = {
    D_0049A098, D_0049A0A0, D_0049A0C0, D_0049A0E0,
    D_0049A100, D_0049A120, D_0049A140, D_0049A160,
    D_0049A180, D_0049A1A0, D_0049A1C0, D_0049A1E0,
    D_0049A200, D_0049A220, D_0049A240, D_0049A260,
    D_0049A280, D_0049A2A0, D_0049A2C0, D_0049A2E0,
    D_0049A300, D_0049A320, D_0049A340, D_0049A360,
    D_0049A380, D_0049A3A0, D_0049A3C0, D_0049A3E0,
    D_0049A400, D_0049A420, D_0049A450, D_0049A490,
    D_0049A4D0, D_0049A500, D_0049A530, D_0049A560,
    D_0049A5A0, D_0049A5D0, D_0049A610, D_0049A640,
    D_0049A680, D_0049A6A0, D_0049A6C0, D_0049A6E0,
    D_0049A700, D_0049A720, D_0049A740, D_0049A760,
    D_0049A780, D_0049A7A0, D_0049A7C0, D_0049A7E0,
    D_0049A7F0, D_0049A810, D_0049A830, D_0049A850,
    D_0049A870, D_0049A890, D_0049A8B0, D_0049A8D0,
    D_0049A910, D_0049A930, D_0049A950, D_0049A970,
    D_0049A9A0, D_0049A9E0, D_0049AA00, D_0049AA20,
    D_0049AA40, D_0049AA60, D_0049AA80, D_0049AAA0,
    D_0049AAC0, D_0049AAE0, D_0049AB00, D_0049AB20,
    D_0049AB40, D_0049AB60, D_0049AB80, D_0049ABA0,
    D_0049ABD0, D_0049AC00, D_0049AC30, D_0049AC50,
    D_0049AC70, D_0049AC90, D_0049ACB0, D_0049ACD0,
    D_0049ACF0, D_0049AD10, D_0049AD30, D_0049AD50,
    D_0049AD70, D_0049AD90, D_0049ADC0, NULL,
};
char *gSkillEffectNames[204] = {
    D_0049ADF0, D_0049AE00, D_0049AE10, D_0049AE20,
    D_0049AE30, D_0049AE40, D_0049AE50, D_0049AE60,
    D_0049AE70, D_0049AE80, D_0049AE90, D_0049AE98,
    D_0049AEA0, D_0049AEA8, D_0049AEB0, D_0049AEB8,
    D_0049AEC8, D_0049AED0, D_0049AED8, D_0049AEE0,
    D_0049AEE8, D_0049AEF0, D_0049AEF8, D_0049AF00,
    D_0049AF08, D_0049AF10, D_0049AF18, D_0049AF20,
    D_0049AF40, D_0049AF60, D_0049AF80, D_0049AF98,
    D_0049AFA0, D_0049AFB0, D_0049AFC0, D_0049AFD0,
    D_0049AFD8, D_0049AFE8, D_0049AFF8, D_0049B008,
    D_0049B018, D_0049B028, D_0049B038, D_0049B048,
    D_0049B058, D_0049B068, D_0049B078, D_0049B088,
    D_0049B098, D_0049B0A8, D_0049B0B8, D_0049B0C8,
    D_0049B0D0, D_0049B0E0, D_0049B0F0, D_0049B100,
    D_0049B110, D_0049B120, D_0049B140, D_0049B160,
    D_0049B180, D_0049B1A0, D_0049B1B8, D_0049B1D0,
    D_0049B1E0, D_0049B200, D_0049B220, D_0049B240,
    D_0049B260, D_0049B280, D_0049B2A0, D_0049B2C0,
    D_0049B2D8, D_0049B2E0, D_0049B2F0, D_0049B300,
    D_0049B320, D_0049B338, D_0049B348, D_0049B358,
    D_0049B368, D_0049B378, D_0049B388, D_0049B390,
    D_0049B398, D_0049B3A0, D_0049B3A8, D_0049B3B0,
    D_0049B3B8, D_0049B3C8, D_0049B3D8, D_0049B3E0,
    D_0049B3F0, D_0049B400, D_0049B410, D_0049B420,
    D_0049B430, D_0049B440, D_0049B450, D_0049B460,
    D_0049B470, D_0049B480, D_0049B490, D_0049B4A0,
    D_0049B4B0, D_0049B4C0, D_0049B4D0, D_0049B4E0,
    D_0049B4F0, D_0049B500, D_0049B510, D_0049B520,
    D_0049B530, D_0049B540, D_0049B550, D_0049B560,
    D_0049B570, D_0049B580, D_0049B590, D_0049B5A0,
    D_0049B5B0, D_0049B5C0, D_0049B5D0, D_0049B5E0,
    D_0049B5F0, D_0049B600, D_0049B610, D_0049B620,
    D_0049B630, D_0049B640, D_0049B650, D_0049B660,
    D_0049B670, D_0049B680, D_0049B690, D_0049B6A0,
    D_0049B6B0, D_0049B6C0, D_0049B6D0, D_0049B6E0,
    D_0049B6F0, D_0049B700, D_0049B710, D_0049B720,
    D_0049B730, D_0049B740, D_0049B750, D_0049B760,
    D_0049B768, D_0049B770, D_0049B778, D_0049B780,
    D_0049B798, D_0049B7A8, D_0049B7B8, D_0049B7C8,
    D_0049B7D8, D_0049B7E0, D_0049B7E8, D_0049B7F0,
    D_0049B7F8, D_0049B808, D_0049B810, D_0049B820,
    D_0049B840, D_0049B858, D_0049B868, D_0049B878,
    D_0049B888, D_0049B898, D_0049B8A8, D_0049B8B8,
    D_0049B8C0, D_0049B8D0, D_0049B8E0, D_0049B8F0,
    D_0049B900, D_0049B910, D_0049B920, D_0049B930,
    D_0049B940, D_0049B960, D_0049B978, D_0049B988,
    D_0049B990, D_0049B9A0, D_0049B9B0, D_0049B9C0,
    D_0049B9D0, D_0049B9D8, D_0049B9E8, D_0049BA00,
    D_0049BA20, D_0049BA40, D_0049BA60, D_0049BA78,
    D_0049BA80, D_0049BA90, D_0049BAA0, D_0049BAB0,
    D_0049BAC0, D_0049BAD0, NULL, NULL,
};
char *gSkillNames[88] = {
    D_0049A098, D_0049BAD8, D_0049BAE0, D_0049BAE8,
    D_0049BAF0, D_0049BB00, D_0049BB10, D_0049BB18,
    D_0049BB28, D_0049BB38, D_0049BB40, D_0049BB48,
    D_0049BB58, D_0049BB68, D_0049BB70, D_0049BB80,
    D_0049BB88, D_0049BB98, D_0049BBA0, D_0049BBA8,
    D_0049BBB8, D_0049BBC8, D_0049BBD0, D_0049BBD8,
    D_0049BBE8, D_004961C0, D_0049BBF8, D_0049BC08,
    D_0049BC10, D_0049BC18, D_0049BC20, D_0049BC28,
    D_0049BC30, D_0049BC38, D_0049BC48, D_0049BC50,
    D_0049BC58, D_0049BC68, D_0049BC78, D_0049BC88,
    D_0049BC98, D_0049BCA0, D_0049BCB0, D_0049B8C0,
    D_0049BCC0, D_0049BCC8, D_004961B0, D_0049BCD0,
    D_0049BCE0, D_0049B3F0, D_0049BCE8, D_0049BCF8,
    D_0049BD08, D_0049BD10, D_0049BD18, D_0049BD20,
    D_0049BD28, D_004961B8, D_0049BD30, D_0049BD38,
    D_0049BD40, D_0049B8B8, D_0049B0C8, D_0049BD50,
    D_0049BD60, D_0049BD70, D_0049BD78, D_0049BD88,
    D_00495EF8, D_0049BD98, D_0049BDA8, D_0049BDB8,
    D_0049BDC0, D_0049BDD0, D_0049BDD8, D_0049BDE8,
    D_0049BDF0, D_0049BA78, D_0049BDF8, D_0049BE00,
    D_0049BE08, D_0049BAD0, D_0049BE10, D_0049BE18,
    D_0049BE20, NULL, NULL, NULL,
};
char *gHelpTexts_0[16] = {
    D_0049BE30, D_0049BE70, D_0049BE90, D_0049BEC0,
    D_0049BEF0, D_0049BF10, D_0049BF30, D_0049BF50,
    D_0049BF70, D_0049BF90, D_0049BFC0, D_0049BFF0,
    D_0049C020, D_0049C050, D_0049C080, NULL,
};
char *D_00447610[2] = {D_0049C0B0, D_0049C0D0};
char *gHelpTexts_3[2] = {
    D_0049C0F0, D_0049C110,
};
char *D_00447620[4] = {D_0049C140, D_0049C170, D_0049C190, D_0049C1C0};
char *D_00447630[8] = {
    D_0049C1E0, D_0049C220, D_0049C260, D_0049C1E0,
    D_0049C290, D_0049C2D0, NULL, NULL,
};
char *gHelpTexts_9[16] = {
    D_0049C300, D_0049BFF0, D_0049C330, D_0049C360,
    D_0049C3A0, D_0049C3E0, D_0049C420, D_0049C460,
    D_0049C490, D_0049C4C0, D_0049C4F0, D_0049C520,
    D_0049C540, D_0049C570, D_0049C5E0, D_0049C600,
};
char *gHelpTexts_10[8] = {
    D_0049C670, D_0049C0D0, D_0049C0B0, D_0049C690,
    D_0049C6A0, D_0049C6D0, NULL, NULL,
};
char *gHelpTexts_11[8] = {
    D_0049C710, D_0049C740, D_0049C760, D_0049C7A0,
    D_0049C7D0, D_0049C7F0, D_0049C830, NULL,
};
char *gHelpTexts_12[12] = {
    D_0049C860, D_0049C890, D_0049C8C0, D_0049C8F0,
    D_0049C940, D_0049C970, D_0049C9A0, D_0049C9E0,
    D_0049CA10, NULL, D_0049CA70, D_0049CA88,
};
char *gSymbolColorNames[12] = {
    D_0049CA98, D_0049CAA8, D_0049CAB8, D_0049CAC8,
    D_0049CAD8, D_0049CAE8, D_0049CAF8, D_0049CB08,
    D_0049CB18, D_0049CB28, D_0049CB38, D_0049CB48,
};
/* The seasons, then day and night (gDaytimeNames, an alias at + 0xC: as an object of its own
 * MWCC would 8-align it). */
char *gSeasonNames[5] = {
    D_0049CB58, D_0049CB60, D_0049CB68, D_0049CB70, D_0049CB78,
};
HunterTitle gHunterTitles[148] = {
#include "sub_main/00447750.inc"
};
char *gPriceLegendTexts[4] = {
    D_0049D500, D_0049D520, D_0049D540, NULL,
};
char *D_00447C00[4] = {
    D_0049D560, D_0049D588, D_0049D5A0, NULL,
};
char *D_00447C10[4] = {
    D_0049D5C0, D_0049D588, D_0049D5A0, NULL,
};
char *D_00447C20[4] = {
    D_0049D5E0, D_0049D5F8, D_0049D5A0, NULL,
};
char *D_00447C30[4] = {
    D_0049D610, D_0049D630, D_0049D650, NULL,
};
char *D_00447C40[4] = {
    D_0049D670, D_0049D690, NULL, NULL,
};
char *D_00447C50[8] = {
    D_0049D6B0, D_0049D6D0, D_0049D6F0, D_0049D708,
    D_0049D718, D_0049D730, NULL, NULL,
};
char *D_00447C70[4] = {
    D_0049D6B0, D_0049D6D0, D_0049D730, NULL,
};
char *D_00447C80[4] = {
    D_0049D6B0, D_0049D6F0, D_0049D730, NULL,
};
char *D_00447C90[4] = {
    D_0049D6B0, D_0049D708, D_0049D730, NULL,
};
char *D_00447CA0[4] = {
    D_0049D6B0, D_0049D718, D_0049D730, NULL,
};
char **gAwardNoticeLines[10] = {
    D_00447C00, D_00447C10, D_00447C20, D_00447C30,
    D_00447C40, D_00447C50, D_00447C70, D_00447C80,
    D_00447C90, D_00447CA0,
};
char *gAwardGalleryTitles[37] = {
    D_0049D750, D_0049D768, D_0049D778, D_0049D788,
    D_0049D7A0, D_0049D7B8, D_0049D7D0, D_0049D7E8,
    D_0049D7F8, D_0049D810, D_0049D828, D_0049D838,
    D_0049D848, D_0049D860, D_0049D880, D_0049D898,
    D_0049D8A8, D_0049D8B8, D_0049D8D0, D_0049D8E8,
    D_0049D8F8, D_0049D908, D_0049D918, D_0049D928,
    D_0049D938, D_0049D940, D_0049D950, D_0049D960,
    D_0049D970, D_0049D980, D_0049D990, D_0049D9A0,
    D_0049D9B0, D_0049D9C0, D_0049D9D0, D_0049D9E0,
    D_0049D9F0,
};
char *gAwardDojoTitles[8] = {
    D_0049DA00, D_0049DA20, D_0049DA40, D_0049DA60,
    D_0049DA80, D_0049DA20, D_0049DA40, D_0049DAA0,
};
char *gArenaEmPenMenuTexts[2] = {
    D_0049DAC0, D_0049DAE0,
};
char *gArenaEmReleasedText = D_0049DB00;
char *gArenaEmLineFmts[4] = {
    D_0049DB20, D_0049DB30, D_0049DB40, D_0049DB50,
};
/* One object: gRecordDashesText (+ 0x4) and gArenaEmPenHelp (+ 0xC, PlselWork.help1 and help2,
 * [0] none) are not 8-aligned, so neither can start an object of its own. */
char *gArenaEmBlankLine[10] = {
    D_0049DB60, D_004957A0, NULL, NULL,
    D_0049DB80, D_0049DBB0, D_0049DBF0, D_0049DC20,
    D_0049DC40, D_0049DC60,
};
char *gArenaEmTameNames[5] = {
    D_0049DC80, D_0049DC90, D_0049DCA0, D_0049DCB0,
    D_0049DCC0,
};
char *gArenaEmCondNames[5] = {
    D_0049DCD0, D_0049DCD8, D_00495DA8, D_0049DCE0,
    D_0049DCE8,
};
char *gArenaEmRankNames[10] = {
    D_0049DCF0, D_0049DCF8, D_0049DD08, D_0049DD18,
    D_00495DA8, D_0049DD20, D_0049DD30, D_0049DD38,
    D_0049DD48, D_0049DD58,
};
char *gHornSongTexts[53] = {
    D_0049DD60, D_0049DD70, D_0049DD90, D_0049DDB0,
    D_0049DDD0, D_0049DDF0, D_0049DDD0, D_0049DE10,
    D_0049DE30, D_0049DE50, D_0049DE30, D_0049DE68,
    D_0049DE68, D_0049DE78, D_0049DE68, D_0049DE88,
    D_0049DE88, D_0049DE98, D_0049DEA8, D_0049DEA8,
    D_0049DEB8, D_0049DEB8, D_0049DEB8, D_0049DED0,
    D_0049DED0, D_0049DEF0, D_0049DEF0, D_0049DF08,
    D_0049DF08, D_0049DF18, D_0049DF18, D_0049DF30,
    D_0049DF50, D_0049DF70, D_0049DF50, D_0049DF90,
    D_0049DFB0, D_0049DFD0, D_0049DFB0, D_0049DFF0,
    D_0049E010, D_0049E030, D_0049E010, D_0049E050,
    D_0049E070, D_0049E090, D_0049E070, D_0049E0B0,
    D_0049E0D0, D_0049E0F0, D_0049E0D0, D_0049E108,
    D_0049E120,
};
char *gHunterTitleDescs[149] = {
    D_0049E140, D_0049E160, D_0049E190, D_0049E1C0,
    D_0049E1F0, D_0049E210, D_0049E240, D_0049E270,
    D_0049E2A0, D_0049E2D0, D_0049E320, D_0049E350,
    D_0049E380, D_0049E3D0, D_0049E400, D_0049E430,
    D_0049E4A0, D_0049E4D0, D_0049E500, D_0049E530,
    D_0049E580, D_0049E5B0, D_0049E5E0, D_0049E610,
    D_0049E640, D_0049E690, D_0049E6C0, D_0049E6F0,
    D_0049E720, D_0049E750, D_0049E7A0, D_0049E7F0,
    D_0049E830, D_0049E870, D_0049E8B0, D_0049E8F0,
    D_0049E910, D_0049E940, D_0049E970, D_0049E9B0,
    D_0049E9F0, D_0049EA20, D_0049EA40, D_0049EA60,
    D_0049EA80, D_0049EAA0, D_0049EAC0, D_0049EAE0,
    D_0049EB00, D_0049EB20, D_0049EB40, D_0049EB60,
    D_0049EB80, D_0049EBA0, D_0049EBC0, D_0049EBE0,
    D_0049EC00, D_0049EC20, D_0049EC40, D_0049EC60,
    D_0049EC80, D_0049ECA0, D_0049ECC0, D_0049ECE0,
    D_0049ED00, D_0049ED20, D_0049ED40, D_0049ED60,
    D_0049ED80, D_0049EDA0, D_0049EDC0, D_0049EDE0,
    D_0049EE00, D_0049EE20, D_0049EE40, D_0049EE60,
    D_0049EE80, D_0049EEA0, D_0049EEC0, D_0049EEE0,
    D_0049EF00, D_0049EF20, D_0049EF40, D_0049EF60,
    D_0049EF80, D_0049EFA0, D_0049EFC0, D_0049EFE0,
    D_0049F000, D_0049F020, D_0049F040, D_0049F060,
    D_0049F080, D_0049F0A0, D_0049F0C0, D_0049F0E0,
    D_0049F100, D_0049F120, D_0049F140, D_0049F160,
    D_0049F1A0, D_0049F1D0, D_0049F200, D_0049F230,
    D_0049F260, D_0049F290, D_0049F2C0, D_0049F300,
    D_0049F340, D_0049F380, D_0049F3C0, D_0049F3F0,
    D_0049F430, D_0049F470, D_0049F4A0, D_0049F4F0,
    D_0049F540, D_0049F570, D_0049F5A0, D_0049F5E0,
    D_0049F620, D_0049F680, D_0049F6C0, D_0049F700,
    D_0049F730, D_0049F760, D_0049F790, D_0049F7C0,
    D_0049F7F0, D_0049F820, D_0049F840, D_0049F870,
    D_0049F8A0, D_0049F8D0, D_0049F900, D_0049F930,
    D_0049F960, D_0049F990, D_0049F9C0, D_0049F9F0,
    D_0049FA20, D_0049FA60, D_0049FA80, D_0049FAA0,
    D_0049FAD0, D_0049FB00, D_0049FB30, D_0049FB70,
    gEmClassNames_0,
};
char D_00448198[15] = {
#include "sub_main/00448198.inc"
};
char D_004481B0[0x30] = {
#include "sub_main/004481B0.inc"
};
char D_004481E0[0x20] = {
#include "sub_main/004481E0.inc"
};
char D_00448200[0x10] = {
#include "sub_main/00448200.inc"
};
char D_00448210[0x20] = {
#include "sub_main/00448210.inc"
};
char D_00448230[0x20] = {
#include "sub_main/00448230.inc"
};
char D_00448250[0x30] = {
#include "sub_main/00448250.inc"
};
char D_00448280[0x20] = {
#include "sub_main/00448280.inc"
};
char D_004482A0[0x20] = {
#include "sub_main/004482A0.inc"
};
char D_004482C0[0x10] = {
#include "sub_main/004482C0.inc"
};
char D_004482D0[0x20] = {
#include "sub_main/004482D0.inc"
};
char D_004482F0[0x30] = {
#include "sub_main/004482F0.inc"
};
char D_00448320[0x20] = {
#include "sub_main/00448320.inc"
};
char D_00448340[0x20] = {
#include "sub_main/00448340.inc"
};
char D_00448360[0x20] = {
#include "sub_main/00448360.inc"
};
char D_00448380[0x40] = {
#include "sub_main/00448380.inc"
};
char D_004483C0[0x20] = {
#include "sub_main/004483C0.inc"
};
char D_004483E0[0x30] = {
#include "sub_main/004483E0.inc"
};
f32 gDayStepTimes[4] = {
#include "sub_main/00448410.inc"
};
char gAmbSePosArea84[0x10] = {
#include "sub_main/00448420.inc"
};
u32 gAmbSeIdsArea84[2] = {
#include "sub_main/00448430.inc"
};
u32 gAmbSePeriodsArea84[2] = {
#include "sub_main/00448438.inc"
};
u32 gAmbSeKindsArea84[2] = {
#include "sub_main/00448440.inc"
};
u32 gAmbSeSlotsArea84[2] = {
#include "sub_main/00448448.inc"
};
u32 gAmbSePosArea88[6] = {
#include "sub_main/00448450.inc"
};
char gAmbSeIdsArea88[0x8] = {
#include "sub_main/00448468.inc"
};
char gAmbSePeriodsArea88[0x8] = {
#include "sub_main/00448470.inc"
};
u32 gAmbSeKindsArea88[2] = {
#include "sub_main/00448478.inc"
};
u32 gAmbSeSlotsArea88[4] = {
#include "sub_main/00448480.inc"
};
u32 gAmbSePosArea89[12] = {
#include "sub_main/00448490.inc"
};
u32 gAmbSeIdsArea89[4] = {
#include "sub_main/004484C0.inc"
};
u32 gAmbSePeriodsArea89[2] = {
#include "sub_main/004484D0.inc"
};
u32 gAmbSeKindsArea89[2] = {
#include "sub_main/004484D8.inc"
};
u32 gAmbSeSlotsArea89[2] = {
#include "sub_main/004484E0.inc"
};
u32 gAmbSeFlagsArea89[2] = {
#include "sub_main/004484E8.inc"
};
u32 gArea85EffectPos[48] = {
#include "sub_main/004484F0.inc"
};
FogParam gFogDefault = {
#include "sub_main/004485B0.inc"
};
FogParam gFogArea56 = {
#include "sub_main/004485C0.inc"
};
u32 gAreaInfo[200] = {
#include "sub_main/004485D0.inc"
};
u8 gSkyDayTable[96] = {
#include "sub_main/004488F0.inc"
};
u8 gSkyDayTable84[96] = {
#include "sub_main/00448950.inc"
};
u8 gSkyDayTableAD[96] = {
#include "sub_main/004489B0.inc"
};
f32 gGlowPosArea56[12][3] = {
#include "sub_main/00448A10.inc"
};
s16 gGlowEntryArea56[16] = {
#include "sub_main/00448AA0.inc"
};
f32 gGlowPosArea83[12][3] = {
#include "sub_main/00448AC0.inc"
};
s16 gGlowEntryArea83[16] = {
#include "sub_main/00448B50.inc"
};
f32 gGlowPosArea84[14][3] = {
#include "sub_main/00448B70.inc"
};
s16 gGlowEntryArea84[16] = {
#include "sub_main/00448C20.inc"
};
f32 gGlowPosArea85[10][3] = {
#include "sub_main/00448C40.inc"
};
s16 gGlowEntryArea85[16] = {
#include "sub_main/00448CC0.inc"
};
s16 gGlowEntryArea86[16] = {
#include "sub_main/00448CE0.inc"
};
f32 gGlowPosArea86[9][3] = {
#include "sub_main/00448D00.inc"
};
f32 gGlowPosArea87[8][3] = {
#include "sub_main/00448D70.inc"
};
f32 gGlowScaleArea87[8] = {
#include "sub_main/00448DD0.inc"
};
f32 gGlowPosArea88[16][3] = {
#include "sub_main/00448DF0.inc"
};
f32 gGlowScaleArea88[16] = {
#include "sub_main/00448EB0.inc"
};
s16 gGlowEntryArea88[16] = {
#include "sub_main/00448EF0.inc"
};
f32 gGlowPosArea89[12][3] = {
#include "sub_main/00448F10.inc"
};
f32 gGlowScaleArea89[12] = {
#include "sub_main/00448FA0.inc"
};
s16 gGlowEntryArea89[12] = {
#include "sub_main/00448FD0.inc"
};
s16 gGlowVillageEntries[4] = {
#include "sub_main/00448FE8.inc"
};
s16 gGlowEntryAreaAA[8] = {
#include "sub_main/00448FF0.inc"
};
f32 gGlowPosAreaAA[4][3] = {
#include "sub_main/00449000.inc"
};
f32 gGlowRotYAreaAA[4] = {
#include "sub_main/00449030.inc"
};
f32 gGlowPosAreaAE[12][3] = {
#include "sub_main/00449040.inc"
};
s16 gGlowEntryAreaAE[16] = {
#include "sub_main/004490D0.inc"
};
f32 gGlowPosAreaAF[20][3] = {
#include "sub_main/004490F0.inc"
};
s16 gGlowEntryAreaAF[24] = {
#include "sub_main/004491E0.inc"
};
f32 gAreaPropPosArea56[8] = {
#include "sub_main/00449210.inc"
};
f32 gAreaPropRotArea56[8] = {
#include "sub_main/00449230.inc"
};
f32 gAreaPropPosArea83[8][3] = {
#include "sub_main/00449250.inc"
};
f32 gAreaPropScaleArea83[8] = {
#include "sub_main/004492B0.inc"
};
s16 gAreaPropEntryArea83[8] = {
#include "sub_main/004492D0.inc"
};
f32 gAreaPropPosArea84[6][3] = {
#include "sub_main/004492E0.inc"
};
f32 gAreaPropScaleArea84[12] = {
#include "sub_main/00449330.inc"
};
f32 gAreaPropPosArea85[2][3] = {
#include "sub_main/00449360.inc"
};
s16 gAreaPropEntryArea85[4] = {
#include "sub_main/00449378.inc"
};
f32 gAreaPropPosArea88[2][3] = {
#include "sub_main/00449380.inc"
};
f32 gAreaPropScaleArea88[2] = {
#include "sub_main/00449398.inc"
};
s16 gAreaPropEntryArea88[4] = {
#include "sub_main/004493A0.inc"
};
f32 gAreaPropPosAreaAA[3] = {
#include "sub_main/004493A8.inc"
};
f32 gAreaPropPosAreaAE[3] = {
#include "sub_main/004493B8.inc"
};
f32 gAreaPropPosAreaAF[3] = {
#include "sub_main/004493C8.inc"
};
f32 gRiverPtsDay48[12] = {
#include "sub_main/004493E0.inc"
};
f32 gRiverPtsDay4B[12] = {
#include "sub_main/00449410.inc"
};
f32 gRiverPtsDay6E[12] = {
#include "sub_main/00449440.inc"
};
f32 gRiverPtsDay71[12] = {
#include "sub_main/00449470.inc"
};
f32 gRiverPtsDay72[12] = {
#include "sub_main/004494A0.inc"
};
f32 gRiverPtsDay78[12] = {
#include "sub_main/004494D0.inc"
};
f32 gRiverPtsDayA1[12] = {
#include "sub_main/00449500.inc"
};
