/*
 * The drawing of an area and its small objects (sub_main).
 *
 * - The area's models: Area_DrawModels draws the area model (gAreaState.model),
 *   its sky first, with its animated parts (gAreaAnimTbls, AreaAnim), then the
 *   area's props (gAreaProps, PropRec; Area_DrawProp) and the village's props
 *   chosen by its facility levels (gVillagePropChoices). area_draw.h has the
 *   lists' records and their AREA_MODEL_ANIM_*, PROP_ANIM_* and AREA_DRAW_*
 *   values; area_anim.h has the animations' parameter blocks.
 * - The sky: Sky_Draw draws a gradient behind the area, Sky_DrawGradient by the
 *   time of day in the online hubs and houses (the day tables, DayStep), else
 *   Sky_DrawBand turned by the camera's heading; then the area's fade.
 * - The lights of the online hubs and houses that glow at night
 *   (Area_DrawGlowProps), billboards with animated textures.
 * - The notices: a queue of NOTICE_QUEUE_LEN (Notice_Push, Notice_PushItem,
 *   Notice_PushText; chat_log.h's Notice), of which Notice_Update shows one every
 *   60 frames as a banner sliding down from the top of the screen
 *   (NoticeBanner_*, TextNotice_Draw).
 * - Three kinds of object of the object pool (Obj), each run by a task function
 *   through its four states: the area props (kind 6, AreaProp_*), the sound
 *   objects (kind 0xC, SoundObj_*: a positioned sound, replayed or following a
 *   river) and the area atmosphere objects (kind 0xD, AreaAtmos_*: scrolled
 *   overlays drawn around the camera, whose layer tables are with the season
 *   banner's code).
 * - The data, much of it read by other files: the text tables of the quest, menu
 *   and notice messages, the areas' model animation and prop lists, the effect
 *   models by area (gAreaFxModels), the areas' object set-ups (gAreaInfo), fog and
 *   ambient sound tables, the default stage header, and the per-area tables the
 *   functions above read.
 */
#include "common.h"
#include "area.h"
#include "area_anim.h"
#include "area_draw.h"
#include "chat_log.h"
#include "capsule.h"
#include "em.h"
#include "obj.h"
#include "overlay.h"
#include "player.h"
#include "prim.h"
#include "render_state.h"
#include "quest.h"
#include "save.h"
#include "scene.h"
#include "sprite.h"
#include "window.h"

/* The game's code of this file was built at optimization level 4; the colour helpers and
 * Sky_DrawGradient at level 3. */
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
void AreaAtmos_Setup(Obj *o);
void AreaAtmos_Update(Obj *o);
void AreaAtmos_End(Obj *o);
void AreaAtmos_Free(Obj *o);
s32 Gs_SetState(u8 a, u32 b);
extern u8 gSkyDayTable[];
extern u8 gSkyDayTableHubs[];
extern u8 gSkyDayTableHouses[];
extern f32 gCamDir[3];    /* the camera direction */
f32 Math_Atan2(f32 y, f32 x);
s32 Prim_FillGradRect(PrimGradRect *r);
/* end of prototypes */

s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);

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

/* The file's read-only data. */
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

/*
 * Draws one entry of a prop list (PropRec) from model set `set`. Its colour
 * follows its AREA_DRAW_DAY or AREA_DRAW_NIGHT flag and the night level
 * (gAreaState.look[0]); its alpha test reference is set only when it differs
 * from *alphaRef, the one last set, which it updates. Each of its copies is
 * drawn with the matrix its type builds (area_anim.h's AREA_ANIM_CMD_*, or
 * PROP_ANIM_*), reading its parameters from prop->data in turn; a copy whose
 * condition fails, and an unknown type, is not drawn.
 */
void Area_DrawProp(PropRec *prop, Model *set, u8 *alphaRef)
{
    AreaState *state = &gAreaState;
    ModelMatSet *entry = &set->matSets[prop->model];
    f32 *param;
    s16 i;
    f32 mtx[4][4];
    f32 uvMtx[4][4];
    f32 pos[3];

    if (prop->flags & AREA_DRAW_DAY) {
        if (state->look[0] >= 0xFF) {
            return;
        }
        Gs_SetState(RS_COLOR2, ((0xFF - state->look[0]) << 24) | 0xFFFFFF);
    } else if (prop->flags & AREA_DRAW_NIGHT) {
        if (state->look[0] <= 0) {
            return;
        }
        Gs_SetState(RS_COLOR2, (state->look[0] << 24) | 0xFFFFFF);
    } else {
        Gs_SetState(RS_COLOR2, -1);
    }
    if (prop->flags & AREA_DRAW_NO_ZWRITE) {
        Gs_SetState(RS_ZWRITE, 0);
    }
    Draw_ApplyStates(entry->flags);
    if (prop->alphaRef != *alphaRef) {
        Gs_SetState(RS_ALPHA_REF, prop->alphaRef);
        *alphaRef = prop->alphaRef;
    }
    param = prop->data;
    for (i = 0; i < prop->count; i++) {
        Mtx_Identity(mtx[0]);
        switch (prop->type) {
        case PROP_ANIM_NONE:
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            break;
        case AREA_ANIM_CMD_SPIN:
            AreaAnim_TranslateSpin(mtx[0], (struct AnimSpin *)param, prop->flags);
            param += 4;
            break;
        case AREA_ANIM_CMD_UV_SCROLL:
            AreaAnim_UvScroll((struct UvScroll *)param, prop->flags);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            param += 1;
            break;
        case AREA_ANIM_CMD_UV_WAVE:
            AreaAnim_UvWave((struct UvWave *)param);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            param += 6;
            break;
        case AREA_ANIM_CMD_ROTATE:
            AreaAnim_TranslateRotate(mtx[0], (struct AnimRot *)param, prop->flags);
            param += 4;
            break;
        case AREA_ANIM_CMD_TRANSLATE:
            AreaAnim_Translate(mtx[0], param);
            param += 3;
            break;
        case AREA_ANIM_CMD_SCALE_ROTATE:
            AreaAnim_ScaleTranslateRotate(mtx[0], (struct AnimRotScale *)param, prop->flags);
            param += 4;
            break;
        case AREA_ANIM_CMD_TRANSLATE_XZ:
            AreaAnim_TranslateXZ(mtx[0], param);
            param += 2;
            break;
        case AREA_ANIM_CMD_UV_SCROLL_MOVE:
            AreaAnim_UvScrollTranslate(mtx[0], (struct AnimScrollPos *)param, prop->flags);
            param += 2;
            break;
        case AREA_ANIM_CMD_UV_SCROLL_SPIN:
            AreaAnim_UvScrollSpin(mtx[0], (struct AnimScrollSpin *)param, prop->flags);
            break;
        case AREA_ANIM_CMD_UV_SCROLL_ROTATE:
            AreaAnim_UvScrollRotate(mtx[0], (struct AnimScrollRot *)param, prop->flags);
            param += 2;
            break;
        case AREA_ANIM_CMD_SPIN_SCROLL:
            AreaAnim_SpinScroll(mtx[0], (struct AnimSpin2 *)param);
            param += 4;
            break;
        case AREA_ANIM_CMD_UV_STEP:
            AreaAnim_UvStep((struct UvStep *)param);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            param += 4;
            break;
        case PROP_ANIM_MOVING:
            Mtx_Translation(mtx[0], param[0] + state->propOfs[0], param[1] + state->propOfs[1], param[2] + state->propOfs[2]);
            switch (state->propState) {
            case 1:
                if (state->propTimer > 0x1E) {
                    Mtx_RotateX(mtx[0], 6.2831855f * (-2.0f * Math_Sin(6.2831855f * ((state->propTimer - 0x1E) << 8) / 65536.0f)) / 360.0f);
                }
                break;
            case 2:
                if (state->propTimer < 0x100) {
                    Mtx_RotateX(mtx[0], 6.2831855f * (-2.0f * Math_Sin(6.2831855f * (state->propTimer << 8) / 65536.0f)) / 360.0f);
                }
                break;
            }
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            param += 3;
            break;
        case PROP_ANIM_FLIPBOOK:
            if (state->frame & 0x20) {
                Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.125f * ((state->frame >> 2) & 7), 0.0f);
            } else {
                Mtx_Translation(uvMtx[0], 0.75f - 0.25f * (state->frame & 3), 0.875f - 0.125f * ((state->frame >> 2) & 7), 0.0f);
            }
            Gs_SetState(RS_UV_MTX, (s32)uvMtx);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            break;
        case PROP_ANIM_PLACE_YZ:
            Mtx_Translation(mtx[0], param[0], param[1], param[2]);
            Mtx_RotateY(mtx[0], param[3]);
            Mtx_RotateZ(mtx[0], param[4]);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            param += 5;
            break;
        case PROP_ANIM_NIGHT_GLOW: {
            f32 pulse = 16.0f * (1.0f + Math_Sin(6.2831855f * (state->frame << 14) / 65536.0f));
            u8 alpha = 0.8745098f * (u32)state->look[0];
            u8 glow = pulse * ((u32)state->look[0] / 255.0f);
            alpha += glow;
            Gs_SetState(RS_COLOR2, (alpha << 24) | 0xFFFFFF);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            break;
        }
        case PROP_ANIM_NIGHT_SPIN: {
            u8 red, green, blue;
            f32 shade;

            if (state->look[0] == 0) {
                blue = green = red = 0xFF;
            } else if (state->look[0] == 0xFF) {
                blue = green = red = 0x30;
            } else {
                shade = 255.0f - 207.0f * ((u32)state->look[0] / 255.0f);
                red = shade;
                green = shade;
                blue = shade;
            }
            Gs_SetState(RS_COLOR2, (red << 16) | 0xFF000000 | (green << 8) | blue);
            AreaAnim_TranslateSpin(mtx[0], (struct AnimSpin *)param, prop->flags);
            param += 4;
            break;
        }
        case PROP_ANIM_PULSE: {
            u8 alpha = 95.0f * (0.5f * (1.0f + Math_Sin(6.2831855f * (state->frame << 10) / 65536.0f)));

            Gs_SetState(RS_COLOR2, ((u8)(alpha + 0xA0) << 24) | 0xFFFFFF);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            break;
        }
        case PROP_ANIM_SLIDE:
            Vec_Copy(pos, param);
            if (!(state->frame & 0x100)) {
                if ((state->frame & 0xFF) < 0x80) {
                    pos[2] += 2600.0f * ((state->frame & 0x7F) / 128.0f);
                } else {
                    pos[2] += 2600.0f;
                }
            } else if ((state->frame & 0xFF) < 0x80) {
                pos[2] += 2600.0f * (1.0f - (state->frame & 0x7F) / 128.0f);
            }
            Mtx_SetTrans(mtx[0], pos[0], pos[1], pos[2]);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            param += 3;
            break;
        case PROP_ANIM_SWAY:
            Mtx_SetTrans(mtx[0], param[0], param[1], param[2]);
            Mtx_RotateZ(mtx[0], 6.2831855f * (2.5f * Math_Sin(6.2831855f * (state->frame << 9) / 65536.0f)) / 360.0f);
            Mtx_RotateY(mtx[0], param[3]);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            param += 4;
            break;
        case PROP_ANIM_WHEEL: {
            s16 shift = *(s16 *)&param[3];
            u16 angle = (((state->frame & (0xFFFF >> shift)) << shift) + (0x10000 / prop->count) * i);

            if (angle > 0x471C && angle < 0xB1C7) {
                continue;
            }
            Mtx_Translation(mtx[0], param[0], param[1], param[2]);
            Mtx_RotateZ(mtx[0], 6.2831855f * (2.5f * Math_Sin(6.2831855f * (state->frame << 9) / 65536.0f)) / 360.0f);
            Mtx_RotateY(mtx[0], 6.2831855f * angle / 65536.0f);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            break;
        }
        case PROP_ANIM_PLACE_XYZ:
            Mtx_SetTrans(mtx[0], param[0], param[1], param[2]);
            Mtx_RotateXYZ(mtx[0], param[3], param[4], param[5]);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            param += 6;
            break;
        case PROP_ANIM_FLICKER: {
            f32 stretch = 0.35f * Math_Sin(6.2831855f * (3.0f * ((state->frame & 0x1FF) / 512.0f)));
            f32 sway;

            sway = stretch + 0.06f * Math_Sin(6.2831855f * (7.0f * ((state->frame & 0x1FF) / 512.0f)));
            stretch = 0.85f + sway;
            sway = 5.0f * Math_Sin(6.2831855f * (state->frame << 8) / 65536.0f);
            Mtx_Scale(mtx[0], 1.0f, 1.0f, stretch);
            Mtx_SetTrans(mtx[0], param[0], param[1], param[2]);
            Mtx_RotateY(mtx[0], param[3] + 6.2831855f * sway / 360.0f);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            param += 4;
            break;
        }
        case PROP_ANIM_FLICKER_2: {
            f32 stretch = 0.35f * Math_Sin(6.2831855f * (5.0f * ((state->frame & 0x1FF) / 512.0f)));

            Mtx_Scale(mtx[0], 1.0f, 1.0f, 0.85f + (stretch + 0.09f * Math_Sin(6.2831855f * (9.0f * ((state->frame & 0x1FF) / 512.0f)))));
            Mtx_SetTrans(mtx[0], param[0], param[1], param[2]);
            Mtx_RotateY(mtx[0], param[3]);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            param += 4;
            break;
        }
        case PROP_ANIM_DAY_SCROLL:
            Gs_SetState(RS_COLOR2, ((0xFF - (state->look[0] >> 1)) << 24) | 0xFFFFFF);
            AreaAnim_UvScrollTranslate(mtx[0], (struct AnimScrollPos *)param, prop->flags);
            param += 2;
            break;
        case PROP_ANIM_NIGHT_GLOW_5:
        case PROP_ANIM_NIGHT_GLOW_6: {
            f32 pulse = 24.0f * (1.0f + Math_Sin(6.2831855f * (state->frame << 12) / 65536.0f));
            u32 night = state->look[0];
            u8 alpha = 0.8117647f * night;
            alpha += pulse * (night / 255.0f);
            Gs_SetState(RS_COLOR2, (alpha << 24) | 0xFFFFFF);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            break;
        }
        case PROP_ANIM_NIGHT_GLOW_2:
        case PROP_ANIM_NIGHT_GLOW_3:
        case PROP_ANIM_NIGHT_GLOW_4: {
            f32 pulse = 16.0f * (1.0f + Math_Sin(6.2831855f * (state->frame << 14) / 65536.0f));
            u32 night = state->look[0];
            u8 alpha = 0.8745098f * night;
            u8 glow = pulse * (night / 255.0f);
            alpha += glow;
            Gs_SetState(RS_COLOR2, (alpha << 24) | 0xFFFFFF);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            break;
        }
        case PROP_ANIM_FACILITY:
            if (*(s16 *)param != (u8)Village_FacilityLevel(0)) {
                continue;
            }
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            break;
        case PROP_ANIM_VARIANT:
            if (*(u8 *)param != state->look[1]) {
                continue;
            }
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            break;
        case PROP_ANIM_KUSHALA_WIND: {
            Em *em = gEms;

            for (i = 0; i < EM_COUNT; i++, em++) {
                if (em->active != 0 && em->visible != 0
                    && (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA)) {
                    AreaAnim_UvScroll((struct UvScroll *)param, prop->flags);
                    Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
                    break;
                }
            }
            param += 1;
            if (i == EM_COUNT) {
                continue;
            }
            break;
        }
        default:
            continue;
        }
        Model_Draw(entry->id, 0);
    }
    if (prop->flags & AREA_DRAW_NO_ZWRITE) {
        Gs_SetState(RS_ZWRITE, 1);
    }
}

extern AreaAnim *gAreaAnimTbls[];
extern PropRec *gAreaProps[];
extern AreaPropChoices gVillagePropChoices[];
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
void Area_DrawProp(PropRec *prop, Model *set, u8 *alphaRef);

/*
 * Draws the area, each frame from Frame_Draw; nothing until the area is set
 * up and its sky switched on (gAreaState.active, drawSky), nor in area 0.
 * First the area model (gAreaState.model), placed at the area's position and
 * rotation: its entry 0, the sky, with the depth test off, then
 * Gs_ClearColor(0, 0) and the depth test back on for the rest. An entry with
 * animations in the area's gAreaAnimTbls list is drawn once per animation
 * (AREA_MODEL_ANIM_*). Then the area's props (gAreaProps) from
 * gAreaSubModels[0], and in the village (AREA_HUB_0x89) the props its
 * facility levels choose (gVillagePropChoices) from sets 1 to 3, the whole
 * set when no choice fits. Up to two props marked late are drawn at the end.
 */
void Area_DrawModels(void)
{
    s32 i;
    AreaState *state = &gAreaState;
    Model *set;
    ModelMat *lights;
    u8 alphaRef;
    PropRec *prop;
    s32 k;
    s32 j;
    ModelMatSet *entry;
    f32 mtx[4][4];
    PropDefer defer;
    u16 deferCount;
    AreaAnim *anim;
    AreaPropChoices *village;
    PropChoice *choice;
    Model **sets;

    if (state->active == 0 || state->drawSky == 0) {
        return;
    }
    if (gScene.area != 0) {
        LightSet_Apply(0);
        set = state->model;
        if (set != NULL && set->active != 0) {
            Tex_CountLoaded(0x108);
            Tex_Prepare(0x10, 0x108);
            Gs_SetState(RS_ALPHA_REF, 0x80);
            alphaRef = 0x80;
            Gs_SetState(RS_COLOR2, -1);
            lights = set->mats;
            entry = set->matSets;
            Mtx_Translation(mtx[0], state->pos[0], state->pos[1], state->pos[2]);
            Mtx_SetRotXYZ(mtx[0], state->rot[0], state->rot[1], state->rot[2]);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            i = set->matSetCount;
            anim = gAreaAnimTbls[state->area];
            for (k = 0; k < i; k++, entry++) {
                if (k == 0) {
                    Gs_SetState(RS_ZTEST, RS_CMP_ALWAYS);
                }
                Gs_SetLightList(lights, entry);
                Draw_ApplyStates(entry->flags);
                if (anim != NULL && k == anim->model) {
                    for (;;) {
                        if (!(anim->flags & AREA_DRAW_NOT_IN_EVENT) != 0 || gScene.eventRunning != 1) {
                            if (anim->flags & AREA_DRAW_NO_ZWRITE) {
                                Gs_SetState(RS_ZWRITE, 0);
                            }
                            if (anim->flags & AREA_DRAW_DAY) {
                                Gs_SetState(RS_COLOR2, ((0xFF - state->look[0]) << 24) | 0xFFFFFF);
                            } else if (anim->flags & AREA_DRAW_NIGHT) {
                                Gs_SetState(RS_COLOR2, (state->look[0] << 24) | 0xFFFFFF);
                            } else {
                                Gs_SetState(RS_COLOR2, -1);
                            }
                            if (anim->alphaRef != alphaRef) {
                                Gs_SetState(RS_ALPHA_REF, anim->alphaRef);
                                alphaRef = anim->alphaRef;
                            }
                            switch (anim->type) {
                            case AREA_MODEL_ANIM_NONE:
                                break;
                            case AREA_MODEL_ANIM_HIDDEN:
                                goto next;
                            case AREA_MODEL_ANIM_SPIN:
                                AreaAnim_TranslateSpin(mtx[0], (struct AnimSpin *)anim->arg, anim->flags);
                                break;
                            case AREA_MODEL_ANIM_UV_SCROLL:
                                AreaAnim_UvScroll((struct UvScroll *)anim->arg, anim->flags);
                                break;
                            case AREA_MODEL_ANIM_UV_WAVE:
                                AreaAnim_UvWave((struct UvWave *)anim->arg);
                                break;
                            case AREA_MODEL_ANIM_UV_SCROLL_SPIN:
                                AreaAnim_UvScrollSpin(mtx[0], (struct AnimScrollSpin *)anim->arg, anim->flags);
                                break;
                            case AREA_MODEL_ANIM_UV_STEP:
                                AreaAnim_UvStep((struct UvStep *)anim->arg);
                                break;
                            case AREA_MODEL_ANIM_TRANSLATE:
                                AreaAnim_Translate(mtx[0], anim->arg);
                                break;
                            case AREA_MODEL_ANIM_UV_SCROLL_MOVE:
                                AreaAnim_UvScrollTranslate(mtx[0], (struct AnimScrollPos *)anim->arg, anim->flags);
                                break;
                            case AREA_MODEL_ANIM_SPIN_SCROLL:
                                AreaAnim_SpinScroll(mtx[0], (struct AnimSpin2 *)anim->arg);
                                break;
                            case AREA_MODEL_ANIM_MAT_ALPHAS:
                                for (j = 0; j < entry->count; j++) {
                                    ModelMat *mat = &lights[entry->mat[j]];
                                    switch (j) {
                                    case 0:
                                        mat->alpha = 1.0f;
                                        break;
                                    case 1:
                                        mat->alpha = 0.0f;
                                        break;
                                    }
                                    Gs_SetState((u8)RS_MATERIAL(j), (s32)mat);
                                }
                                break;
                            case AREA_MODEL_ANIM_UV_SCROLL_BLINK:
                                if (state->frame & 0xC0) {
                                    goto next;
                                }
                                AreaAnim_UvScroll((struct UvScroll *)anim->arg, anim->flags);
                                break;
                            case AREA_MODEL_ANIM_NIGHT_SCROLL:
                            case AREA_MODEL_ANIM_NIGHT_SCROLL_2: {
                                    u8 red;
                                    u8 green;
                                    u8 blue;
                                    f32 shade;

                                    if (state->look[0] == 0) {
                                        blue = green = red = 0xFF;
                                    } else if (state->look[0] == 0xFF) {
                                        blue = green = red = 0x30;
                                    } else {
                                        shade = 255.0f - 207.0f * ((u32)state->look[0] / 255.0f);
                                        red = shade;
                                        green = shade;
                                        blue = shade;
                                    }
                                    Gs_SetState(RS_COLOR2, (red << 16) | 0xFF000000 | (green << 8) | blue);
                                    AreaAnim_UvScroll((struct UvScroll *)anim->arg, anim->flags);
                                    break;
                                }
                            case AREA_MODEL_ANIM_SPIN_TURN:
                            case AREA_MODEL_ANIM_SPIN_TURN_BIG:
                            case AREA_MODEL_ANIM_SPIN_TURN_2:
                            case AREA_MODEL_ANIM_SPIN_TURN_3: {
                                    u8 *arg = anim->arg;

                                    AreaAnim_TranslateSpin(mtx[0], (struct AnimSpin *)arg, anim->flags);
                                    if (anim->type == AREA_MODEL_ANIM_SPIN_TURN_BIG) {
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
                                    Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
                                    break;
                                }
                            case AREA_MODEL_ANIM_PULSE: {
                                    f32 pulse = 0.5f * (1.0f + Math_Sin((6.2831855f * (state->frame << 10)) / 65536.0f));
                                    u8 green = 32.0f * pulse;
                                    u8 blue = 64.0f * pulse;

                                    Gs_SetState(RS_COLOR2, (((u8)(green + 0xC0)) << 8) | 0xFFFF0000 | (u8)(blue + 0xA0));
                                    break;
                                }
                            }
                            Model_Draw(entry->id, 0);
                            switch (anim->type) {
                            case AREA_MODEL_ANIM_SPIN:
                            case AREA_MODEL_ANIM_TRANSLATE:
                            case AREA_MODEL_ANIM_UV_SCROLL_SPIN:
                            case AREA_MODEL_ANIM_UV_SCROLL_MOVE:
                            case AREA_MODEL_ANIM_SPIN_SCROLL:
                            case AREA_MODEL_ANIM_SPIN_TURN:
                            case AREA_MODEL_ANIM_SPIN_TURN_BIG:
                            case AREA_MODEL_ANIM_SPIN_TURN_2:
                            case AREA_MODEL_ANIM_SPIN_TURN_3:
                                Mtx_Translation(mtx[0], state->pos[0], state->pos[1], state->pos[2]);
                                Mtx_SetRotXYZ(mtx[0], state->rot[0], state->rot[1], state->rot[2]);
                                Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
                                break;
                            }
                        }
next:
                        if (anim->flags & AREA_DRAW_NO_ZWRITE) {
                            Gs_SetState(RS_ZWRITE, 1);
                        }
                        anim++;
                        if (anim->model == k) {
                            continue;
                        }
                        break;
                    }
                } else {
                    if (alphaRef != 0x80) {
                        Gs_SetState(RS_ALPHA_REF, 0x80);
                        alphaRef = 0x80;
                    }
                    Model_Draw(entry->id, 0);
                }
                if (k == 0) {
                    Gs_SetState(RS_ZWRITE, 1);
                    Gs_ClearColor(0, 0);
                    Gs_SetState(RS_ZTEST, RS_CMP_LEQUAL);
                }
            }
            alphaRef = 0x80;
            Gs_SetState(RS_ALPHA_REF, 0x80);
            set = gAreaSubModels[0];
            if (set == NULL || set->active == 0) {
                goto end;
            }
            Tex_Prepare(0x10, 0x128);
            prop = gAreaProps[state->area];
            deferCount = 0;
            if (prop != NULL) {
                while (prop->model != PROP_END) {
                    if (prop->late == 0) {
                        Area_DrawProp(prop, set, &alphaRef);
                    } else if (deferCount < 2) {
                        defer.prop[deferCount] = prop;
                        defer.set[deferCount] = 0;
                        deferCount++;
                    } else {
                        Area_DrawProp(prop, set, &alphaRef);
                    }
                    prop++;
                }
            }
            for (village = gVillagePropChoices; village->area != 0xFFFF; village++) {
                if (village->area == state->area) {
                    break;
                }
            }
            if (village->area != 0xFFFF) {
                sets = &gAreaSubModels[1];
                for (i = 1; i < 4; i++, sets++) {
                    set = *sets;
                    if (set == NULL || set->active == 0) {
                        goto end;
                    }
                    choice = village->choices;
                    switch (state->area) {
                    case AREA_HUB_0x89: {
                            u8 level = Village_FacilityLevel(i);
                            u16 mask;

                            k = village->count; /* the choice count */
                            mask = (1 << i) | (0x100 << level);

                            for (j = 0; j < k; j++, choice++) {
                                if ((choice->mask & mask) == mask) {
                                    prop = choice->props;
                                    break;
                                }
                            }
                            if (j == k) {
                                prop = NULL;
                            }
                            if (prop != NULL) {
                                while (prop->model != PROP_END) {
                                    if (prop->late == 0) {
                                        Area_DrawProp(prop, set, &alphaRef);
                                    } else if (deferCount < 2) {
                                        defer.prop[deferCount] = prop;
                                        defer.set[deferCount] = i;
                                        deferCount++;
                                    } else {
                                        Area_DrawProp(prop, set, &alphaRef);
                                    }
                                    prop++;
                                }
                            } else {
                                s32 partCount = set->matSetCount;

                                for (k = 0; k < partCount; k++) {
                                    ModelMatSet *part = &set->matSets[k];

                                    Draw_ApplyStates(part->flags);
                                    Mtx_Identity(mtx[0]);
                                    Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
                                    Model_Draw(part->id, 0);
                                }
                            }
                            break;
                        }
                    }
                }
            }
            if (deferCount != 0) {
                for (k = 0; k < deferCount; k++) {
                    set = gAreaSubModels[defer.set[k]];
                    if (set != NULL && set->active != 0) {
                        Area_DrawProp(defer.prop[k], set, &alphaRef);
                    }
                }
            }
end:
            Draw_SetDefaultState();
            Gs_SetState(RS_ALPHA_REF, 0);
        }
    }
}

#pragma optimization_level 3
/* x moved toward y by the fraction frac, as a byte. */
static inline u8 Lerp8(u8 x, u8 y, f32 frac)
{
    return x + (u8)(frac * (s16)(y - x));
}
/* The colour's three upper bytes in one word. An inline function, as in the game's code: ORed
 * in place, the last OR's operands come out the other way round. */
static inline u32 Color_PackARG(u8 red, u8 green, u8 alpha)
{
    return (alpha << 24) | (red << 16) | (green << 8);
}
/*
 * The colour from moved toward to by frac (0-1), each byte apart:
 * from + frac * (to - from). Static, so that the compiler keeps frac in one
 * register across Sky_DrawBand's calls, as the game's code does. The green
 * and alpha bytes of from are shifted in place in shifted: written as
 * shifted = from >> 8 in one statement, the compiler carries the expression
 * forward and those channels get other registers.
 */
static u32 Color_Lerp(u32 from, u32 target, f32 frac)
{
    u8 red;
    u8 green;
    s32 alpha;
    u32 shifted;

    red = Lerp8(from >> 16, target >> 16, frac);
    shifted = from;
    shifted >>= 8;
    green = Lerp8(shifted, target >> 8, frac);
    shifted = from;
    shifted >>= 24;
    alpha = Lerp8(shifted, target >> 24, frac);
    return Lerp8(from, target, frac) | Color_PackARG(red, green, alpha);
}
#pragma optimization_level reset

/* The sky's day table (DAY_STEP_COUNT DayStep) of the current area: one for the houses
 * and AREA_ELDER_HALL, one for AREA_HUB_0x84 to AREA_TOWN_ARENA, gSkyDayTable elsewhere.
 * Returned as an integer: DayTable_GetTime computes with it as one. */
static s32 Sky_GetGradient(void)
{
    switch (gScene.area) {
    case AREA_HOUSE:
    case AREA_ELDER_HALL:
    case AREA_ONLINE_HOUSE_0:
    case AREA_ONLINE_HOUSE_1:
    case AREA_ONLINE_HOUSE_2:
        return (s32) gSkyDayTableHouses;
    case AREA_HUB_0x84:
    case AREA_HUB_0x85:
    case AREA_TAVERN:
    case AREA_TOWN_ARENA:
        return (s32) gSkyDayTableHubs;
    default:
        return (s32) gSkyDayTable;
    }
}

extern f32 gDayStepTimes[DAY_STEP_TIMES];

/*
 * The time of day (0-1) of a step of the area's day table (Sky_GetGradient):
 * a row of gDayStepTimes, or moment before the next step's time
 * (DAY_STEP_BEFORE_NEXT), or else moment after the previous step's; wrapped
 * into 0-1. For step 0 the previous step's type is read from word 7 of the
 * table, step 2's top colour, as the game's code does. The table is walked as
 * words (DayStep's three).
 */
static f32 DayTable_GetTime(s16 step, f32 moment)
{
    s32 base = Sky_GetGradient();
    s32 index = step;
    s32 type;
    s32 *entry = (s32 *)(base + index * sizeof(DayStep));
    f32 time;

    type = entry[0];

    if ((u32)type < DAY_STEP_TIMES) {
        time = gDayStepTimes[type];
    } else if (type == DAY_STEP_BEFORE_NEXT) {
        if (index >= DAY_STEP_COUNT - 1) {
            time = gDayStepTimes[*(s32 *)base] - moment;
        } else {
            time = gDayStepTimes[entry[3]] - moment;
        }
    } else if (index == 0) {
        time = moment + gDayStepTimes[*(s32 *)(base + 0x1C)];
    } else {
        time = moment + gDayStepTimes[entry[-3]];
    }
    if (time < 0.0f) {
        time += 1.0f;
    } else if (time > 1.0f) {
        time -= 1.0f;
    }
    return time;
}

f32 LobbyTime_GetDayLen30(void);
f32 LobbyTime_GetDayFrac(void);

#pragma optimization_level 3
/* A channel of colour c as a float (0-255): the byte at bit shift. */
#define CH(c, shift) ((f32)(((c) >> (shift)) & 0xFF))
/* x (a float channel) moved toward y by frac, as a byte. */
static inline u8 LerpF(f32 x, f32 y, f32 frac)
{
    return x + frac * (y - x);
}

/*
 * Draws the sky of the areas with a day cycle, in the town only: a full-screen
 * gradient whose top and bottom colours blend between the two steps of the
 * area's day table (Sky_GetGradient) around the lobby's time of day.
 * Built at optimization level 3, as the game's code was. DayTable_GetTime is
 * static, so that the compiler keeps this function's values in the same
 * registers across its calls. The top colour is computed channel by channel
 * into locals (red, green, blue, alpha), the bottom one in one expression
 * (alpha, red, green, blue): the game's code computes them in those orders.
 */
void Sky_DrawGradient(void)
{
    PrimGradRect rect;
    DayStep *steps;
    f32 now;
    f32 moment;
    f32 stepTime;
    f32 prevTime;
    f32 num;
    f32 den;
    f32 frac;
    s16 i;
    s16 next;
    s16 prev;
    f32 prevR;
    f32 prevG;
    f32 prevB;
    f32 prevA;
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;
    u32 packed;
    u8 *redPtr;
    u32 prevColor;
    u32 nextColor;
    u32 *prevBottom;
    DayStep *prevStep;
    DayStep *nextStep;
    u32 *nextBottom;

    Gs_SetState(RS_ALPHA_REF, 0);
    rect.x1 = 0x280;
    rect.x0 = 0;
    rect.y1 = 0x1C0;
    rect.y0 = 0;
    if (gScene.inLobby != 0) {
        /* nextColor holds the day's length first: with a variable of its own, the
         * compiler gives the function's values other registers than the game's code. */
        nextColor = LobbyTime_GetDayLen30();
        now = LobbyTime_GetDayFrac();
    } else {
        return;
    }
    steps = (DayStep *)Sky_GetGradient();
    moment = 30.0f / nextColor;
    prev = DAY_STEP_COUNT - 1;
    next = DAY_STEP_COUNT;
    for (i = 0; i < DAY_STEP_COUNT; i++) {
        stepTime = DayTable_GetTime(i, moment);
        if (stepTime > now) {
            next = i;
            break;
        } else {
            prev = i;
            prevTime = stepTime;
        }
    }
    if (next == 0) {
        prevTime = DayTable_GetTime(prev, moment);
    }
    if (next >= DAY_STEP_COUNT) {
        next = 0;
        stepTime = DayTable_GetTime(0, moment);
    }
    prevStep = &steps[prev];
    nextStep = &steps[next];
    if (next <= 0) {
        num = now - prevTime;
        den = 1.0f - prevTime + stepTime;
        if (num < 0.0f) {
            num += 1.0f;
        }
    } else {
        den = stepTime - prevTime;
        num = now - prevTime;
    }
    frac = num / den;
    prevColor = prevStep->top;

    prevR = CH(prevColor, 16);
    prevG = CH(prevColor, 8);
    prevB = CH(prevColor, 0);
    prevA = CH(prevColor, 24);
    nextColor = nextStep->top;
    red = LerpF(prevR, CH(nextColor, 16), frac);
    green = LerpF(prevG, CH(nextColor, 8), frac);
    blue = LerpF(prevB, CH(nextColor, 0), frac);
    alpha = LerpF(prevA, CH(nextColor, 24), frac);
    /* FAKEMATCH: red is read through a pointer. Taking cr's address changes the order the
     * compiler numbers the locals in, so that ps, pn and next get the game's registers v1, v0
     * and a0 (a0, v1 and a1 without it); it emits no code. */
    redPtr = &red;
    packed = blue | ((green << 8) | ((alpha << 24) | (*redPtr << 16)));
    rect.color[0] = packed;
    rect.color[1] = packed;
    /* FAKEMATCH: each walker stepped by two ++ (the bottom colour, 8 bytes on): the game
     * steps the pointers in their own registers (addiu 8) and reads at 0(); one += 2,
     * &ps->bottom or any single step folds the 8 into the loads. */
    nextBottom = (u32 *)nextStep;
    nextBottom++;
    nextBottom++;
    prevBottom = (u32 *)prevStep;
    prevBottom++;
    prevBottom++;
    prevColor = *prevBottom;
    prevR = CH(prevColor, 16);
    prevG = CH(prevColor, 8);
    prevB = CH(prevColor, 0);
    prevA = CH(prevColor, 24);
    nextColor = *nextBottom;
    packed = LerpF(prevB, CH(nextColor, 0), frac) | ((LerpF(prevG, CH(nextColor, 8), frac) << 8) |
        ((LerpF(prevA, CH(nextColor, 24), frac) << 24) | (LerpF(prevR, CH(nextColor, 16), frac) << 16)));
    rect.color[2] = packed;
    rect.color[3] = packed;
    Prim_FillGradRect(&rect);
}
#pragma optimization_level reset

/* Draws a sky band as a gradient rectangle: its corner colours blend between the band's four
 * headings by the camera's heading relative to dir. */
void Sky_DrawBand(SkyBand *band, f32 *dir)
{
    PrimGradRect rect;
    u16 camYaw;
    u16 yaw;
    f32 frac;

    rect.x0 = 0;
    rect.y0 = band->y;
    rect.x1 = 0x280;
    rect.y1 = band->h;
    camYaw = (s32)(0.5f + 65536.0f * Math_Atan2(-gCamDir[0], -gCamDir[2]) / 6.2831855f);
    yaw = (u16)(s32)(0.5f + 65536.0f * Math_Atan2(dir[0], dir[2]) / 6.2831855f) - camYaw;
    if (yaw < 0x4000) {
        frac = yaw / 16384.0f;
        rect.color[0] = Color_Lerp(band->c[0], band->c[2], frac);
        rect.color[1] = Color_Lerp(band->c[0], band->c[3], frac);
        rect.color[2] = Color_Lerp(band->c[4], band->c[6], frac);
        rect.color[3] = Color_Lerp(band->c[4], band->c[7], frac);
    } else if (yaw < 0x8000) {
        frac = (yaw - 0x4000) / 16384.0f;
        rect.color[0] = Color_Lerp(band->c[2], band->c[1], frac);
        rect.color[1] = Color_Lerp(band->c[3], band->c[1], frac);
        rect.color[2] = Color_Lerp(band->c[6], band->c[5], frac);
        rect.color[3] = Color_Lerp(band->c[7], band->c[5], frac);
    } else if (yaw < 0xC000) {
        frac = (yaw - 0x8000) / 16384.0f;
        rect.color[0] = Color_Lerp(band->c[1], band->c[3], frac);
        rect.color[1] = Color_Lerp(band->c[1], band->c[2], frac);
        rect.color[2] = Color_Lerp(band->c[5], band->c[7], frac);
        rect.color[3] = Color_Lerp(band->c[5], band->c[6], frac);
    } else {
        frac = (yaw - 0xC000) / 16384.0f;
        rect.color[0] = Color_Lerp(band->c[3], band->c[0], frac);
        rect.color[1] = Color_Lerp(band->c[2], band->c[0], frac);
        rect.color[2] = Color_Lerp(band->c[7], band->c[4], frac);
        rect.color[3] = Color_Lerp(band->c[6], band->c[4], frac);
    }
    Prim_FillGradRect(&rect);
}

void Sky_DrawGradient(void);
void Sky_DrawBand(SkyBand *band, f32 *dir);
void Gfx_SetBlend(s32 a, s32 b);
void Gfx_SetBlendOp(s32 a);
s32 Prim_FillRect(PrimLine *rect);
f32 Math_Sin(f32 x);
extern s16 gAreaFade;
extern s16 gAreaFadeTimer;

/* Draws the area's sky: Sky_DrawGradient in AREA_HOUSE, AREA_HUB_0x57, AREA_HUB_0x83 to
 * AREA_HUB_0x89 and AREA_ONLINE_HOUSE_0 to _2, else the sky band of the area state; then a
 * full-screen fade by gAreaFade (1: black, 2: fading with gAreaFadeTimer, else pulsing). */
void Sky_Draw(void)
{
    AreaState *state = &gAreaState;
    SkyBand band;
    PrimLine rect;
    u8 alpha;

    if (state->active == 0 || state->drawSky == 0) {
        return;
    }
    if (gScene.area == 0) {
        return;
    }
    Gs_SetState(RS_ZWRITE, 0);
    Gs_SetState(RS_ALPHA_REF, 0);
    switch (state->area) {
    case AREA_HOUSE:
    case AREA_HUB_0x57:
    case AREA_HUB_0x83:
    case AREA_HUB_0x84:
    case AREA_HUB_0x85:
    case AREA_TAVERN:
    case AREA_TOWN_ARENA:
    case AREA_ELDER_HALL:
    case AREA_HUB_0x89:
    case AREA_ONLINE_HOUSE_0:
    case AREA_ONLINE_HOUSE_1:
    case AREA_ONLINE_HOUSE_2:
        Sky_DrawGradient();
        break;
    default:
        band = state->band;
        Sky_DrawBand(&band, state->skyPos);
        break;
    }
    if (gAreaFade != 0) {
        rect.x0 = 0;       /* the whole screen, 640 x 448 */
        rect.x1 = 0x280;
        rect.y1 = 0x1C0;
        rect.y0 = 0;
        if (gAreaFade == 1) {
            Gfx_SetBlend(4, 5);
            alpha = 0xFF;
            Gfx_SetBlend(1, 1);
            Gfx_SetBlendOp(1);
        } else if (gAreaFade == 2) {
            Gfx_SetBlend(4, 5);
            alpha = 0xFF - (u8)(127.5f * gAreaFadeTimer);
            rect.color = alpha << 24;
            Prim_FillRect(&rect);
            Gfx_SetBlend(1, 1);
            Gfx_SetBlendOp(1);
        } else {
            alpha = 255.0f * (1.0f + Math_Sin(6.2831855f * (gAreaFadeTimer * 0xB6 + 0xC000) / 65536.0f));
            Gfx_SetBlend(4, 5);
        }
        if (gAreaFade == 1 || gAreaFade == 2) {
            rect.color = -1;
        } else {
            rect.color = (alpha << 24) | 0xFFFFFF;
        }
        Prim_FillRect(&rect);
        if (gAreaFade == 1 || gAreaFade == 2) {
            Gfx_SetBlendOp(0);
        }
    }
    Gs_SetState(RS_ZWRITE, 1);
    Gs_SetState(RS_ALPHA_REF, 0x80);
}

/* Draws one entry of a model set with matrix mtx, with its lights from mats
 * (the set's materials) unless mats is NULL. An entry with id -1 is empty. */
void Area_DrawEntry(ModelMatSet *entry, f32 *mtx, ModelMat *mats)
{
    Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
    if ((entry != NULL) && (entry->id != -1)) {
        if (mats != NULL) {
            Gs_SetLightList(mats, entry);
        }
        Draw_ApplyStates(entry->flags);
        Model_Draw(entry->id, 0);
    }
}

extern f32 gCamMatrix[];
extern f32 gCamYawMtx[];
extern f32 gGlowPosHouse[][3];
extern s16 gGlowEntryHouse[];
extern f32 gGlowPosHub83[][3];
extern s16 gGlowEntryHub83[];
extern f32 gGlowPosHub84[][3];
extern s16 gGlowEntryHub84[];
extern f32 gGlowPosHub85[][3];
extern s16 gGlowEntryHub85[];
extern s16 gGlowEntryTavern[];
extern f32 gGlowPosTavern[][3];
extern f32 gGlowPosTownArena[][3];
extern f32 gGlowScaleTownArena[];
extern f32 gGlowPosElderHall[][3];
extern f32 gGlowScaleElderHall[];
extern s16 gGlowEntryElderHall[];
extern f32 gGlowPosHub89[][3];
extern f32 gGlowScaleHub89[];
extern s16 gGlowEntryHub89[];
extern s16 gGlowVillageEntries[];
extern s16 gGlowEntryHubAA[];
extern f32 gGlowPosHubAA[][3];
extern f32 gGlowRotYHubAA[];
extern f32 gGlowPosOnlineHouse1[][3];
extern s16 gGlowEntryOnlineHouse1[];
extern f32 gGlowPosOnlineHouse2[][3];
extern s16 gGlowEntryOnlineHouse2[];
void Mtx_Concat3x3(f32 *m, f32 *n);
s32 Overlay_IsLoaded(s32 a);
void Draw_SetDefaultState(void);
u8 func_7D96D0(void); /* the lobby's: LobbyWorld.unk110, 0 while the lobby world is inactive */
u32 LobbyWorld_GetFadeColor(s32 i);

/*
 * Draws the lights of the town's hubs and houses, which glow at night: per
 * area, a table of positions and model entries (gGlowPos*, gGlowEntry*, and
 * scales in some). Most are turned to face the camera (gCamMatrix, or
 * gCamYawMtx for its heading alone), with an animated texture that steps
 * through 4 x 4 cells by the frame counter, and an alpha of the night level
 * (gAreaState.look[0]) or of the night level with a pulse (pulseAlpha). The
 * entry numbers that pick each effect are the area models' own. In
 * AREA_TOWN_ARENA four of its lights take their alpha from the lobby's
 * func_7D96D0, and three more, drawn while the lobby overlay is loaded, their
 * colour from LobbyWorld_GetFadeColor.
 */
void Area_DrawGlowProps(void)
{
    AreaState *state = &gAreaState;
    Model *propSet;
    Model *models;
    ModelMatSet *entry;
    ModelMat *lights;
    s16 i;
    s16 k;
    u8 alpha;
    u8 pulseAlpha;
    u32 night;
    f32 pulse;
    f32 mtx[4][4];
    f32 uvMtx[4][4];

    if (gScene.inLobby == 0) {
        return;
    }
    if (state->drawSky == 0) {
        return;
    }
    switch (state->area) {
    case AREA_HOUSE:
    case AREA_HUB_0x57:
    case AREA_HUB_0x83:
    case AREA_HUB_0x84:
    case AREA_HUB_0x85:
    case AREA_TAVERN:
    case AREA_TOWN_ARENA:
    case AREA_ELDER_HALL:
    case AREA_HUB_0x89:
    case AREA_HUB_0xAA:
    case AREA_HUB_0xAB:
    case AREA_HUB_0xAC:
    case AREA_ONLINE_HOUSE_0:
    case AREA_ONLINE_HOUSE_1:
    case AREA_ONLINE_HOUSE_2:
        break;
    default:
        return;
    }
    Gs_SetState(RS_ZWRITE, 0);
    Gs_SetState(RS_ALPHA_REF, 0);
    pulse = 24.0f * (1.0f + Math_Sin(6.2831855f * (state->frame << 12) / 65536.0f));
    night = state->look[0];
    pulseAlpha = 0.8117647f * night;
    pulseAlpha += pulse * (night / 255.0f);
    propSet = gAreaSubModels[0];
    switch (state->area) {
    case AREA_HOUSE:
    case AREA_HUB_0x57:
        models = state->model;
        if (models != NULL && models->active != 0) {
            lights = models->mats;
            for (i = 0; i < 12; i++) {
                Mtx_Identity(mtx[0]);
                Mtx_SetTrans(mtx[0], gGlowPosHouse[i][0], gGlowPosHouse[i][1], gGlowPosHouse[i][2]);
                switch (gGlowEntryHouse[i]) {
                case 6:
                    Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.25f * ((state->frame >> 2) & 3), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                    Gs_SetState(RS_COLOR2, (state->look[0] << 24) | 0xFFFFFF);
                    Mtx_Concat3x3(mtx[0], (f32 *)gCamYawMtx);
                    break;
                case 7:
                    Gs_SetState(RS_COLOR2, (pulseAlpha << 24) | 0xFFFFFF);
                    Mtx_Concat3x3(mtx[0], (f32 *)gCamYawMtx);
                    break;
                case 8:
                    Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.25f * ((state->frame >> 2) & 3), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                    Gs_SetState(RS_COLOR2, (state->look[0] << 24) | 0xFFFFFF);
                    break;
                default:
                    continue;
                }
                Area_DrawEntry(&models->matSets[gGlowEntryHouse[i]], mtx[0], lights);
            }
        }
        break;
    case AREA_HUB_0x83:
        models = state->model;
        if (models != NULL && models->active != 0) {
            lights = models->mats;
            for (i = 0; i < 11; i++) {
                Mtx_Identity(mtx[0]);
                Mtx_SetTrans(mtx[0], gGlowPosHub83[i][0], gGlowPosHub83[i][1], gGlowPosHub83[i][2]);
                switch (gGlowEntryHub83[i]) {
                case 11:
                    Mtx_Concat3x3(mtx[0], gCamMatrix);
                case 10:
                    Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.25f * ((state->frame >> 2) & 3), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                    Gs_SetState(RS_COLOR2, (state->look[0] << 24) | 0xFFFFFF);
                    break;
                case 12:
                    Gs_SetState(RS_COLOR2, (pulseAlpha << 24) | 0xFFFFFF);
                    Mtx_Concat3x3(mtx[0], gCamMatrix);
                    break;
                default:
                    continue;
                }
                Area_DrawEntry(&models->matSets[gGlowEntryHub83[i]], mtx[0], lights);
            }
        }
        break;
    case AREA_HUB_0x84:
        models = state->model;
        if (models != NULL && models->active != 0) {
            lights = models->mats;
            for (i = 0; i < 14; i++) {
                Mtx_Identity(mtx[0]);
                Mtx_SetTrans(mtx[0], gGlowPosHub84[i][0], gGlowPosHub84[i][1], gGlowPosHub84[i][2]);
                switch (gGlowEntryHub84[i]) {
                case 13:
                case 15:
                    Mtx_Concat3x3(mtx[0], (f32 *)gCamYawMtx);
                case 12:
                    Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.25f * ((state->frame >> 2) & 3), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                    Gs_SetState(RS_COLOR2, (state->look[0] << 24) | 0xFFFFFF);
                    break;
                case 14:
                    Gs_SetState(RS_COLOR2, (pulseAlpha << 24) | 0xFFFFFF);
                    Mtx_Concat3x3(mtx[0], (f32 *)gCamYawMtx);
                    break;
                default:
                    continue;
                }
                Area_DrawEntry(&models->matSets[gGlowEntryHub84[i]], mtx[0], lights);
            }
        }
        break;
    case AREA_HUB_0x85:
        models = state->model;
        if (models != NULL && models->active != 0) {
            lights = models->mats;
            Mtx_Identity(mtx[0]);
            Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.25f * ((state->frame >> 2) & 3), 0.0f);
            Gs_SetState(RS_UV_MTX, (s32)uvMtx);
            Gs_SetState(RS_COLOR2, -1);
            Area_DrawEntry(&models->matSets[5], mtx[0], lights);
        }
        alpha = 207.0f + pulse;
        models = gAreaSubModels[0];
        if (models != NULL && models->active != 0) {
            for (i = 0; i < 10; i++) {
                Mtx_Identity(mtx[0]);
                Mtx_SetTrans(mtx[0], gGlowPosHub85[i][0], gGlowPosHub85[i][1], gGlowPosHub85[i][2]);
                switch (gGlowEntryHub85[i]) {
                case 0:
                    Mtx_Translation(uvMtx[0], 0.0f, (state->frame & 0xFF) / 256.0f, 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                    Gs_SetState(RS_COLOR2, -1);
                    break;
                case 1:
                    Mtx_Translation(uvMtx[0], 0.5f * (state->frame & 1), 0.25f * ((state->frame >> 1) & 3), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                    Gs_SetState(RS_COLOR2, -1);
                    break;
                case 2:
                    Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.25f * ((state->frame >> 2) & 3), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                    Gs_SetState(RS_COLOR2, -1);
                    Mtx_Concat3x3(mtx[0], gCamMatrix);
                    break;
                case 3:
                    Gs_SetState(RS_COLOR2, (alpha << 24) | 0xFFFFFF);
                    Mtx_Concat3x3(mtx[0], gCamMatrix);
                    break;
                default:
                    continue;
                }
                Area_DrawEntry(&models->matSets[gGlowEntryHub85[i]], mtx[0], 0);
            }
        }
        break;
    case AREA_TAVERN:
        if (propSet != NULL && propSet->active != 0) {
            for (i = 0; i < 9; i++) {
                Mtx_Translation(mtx[0], gGlowPosTavern[i][0], gGlowPosTavern[i][1], gGlowPosTavern[i][2]);
                k = gGlowEntryTavern[i];
                switch (k) {
                case 1:
                default:
                    alpha = pulseAlpha;
                    Mtx_Concat3x3(mtx[0], gCamMatrix);
                    break;
                case 2:
                    Mtx_Concat3x3(mtx[0], gCamMatrix);
                case 3:
                    alpha = state->look[0];
                    Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.25f * ((state->frame >> 2) & 3), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                    break;
                }
                Gs_SetState(RS_COLOR2, (alpha << 24) | 0xFFFFFF);
                Area_DrawEntry(&propSet->matSets[k], mtx[0], 0);
            }
        }
        break;
    case AREA_TOWN_ARENA:
        models = state->model;
        if (models != NULL && models->active != 0) {
            lights = models->mats;
            Mtx_Identity(mtx[0]);
            Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.25f * ((state->frame >> 2) & 3), 0.0f);
            Gs_SetState(RS_UV_MTX, (s32)uvMtx);
            Gs_SetState(RS_COLOR2, (state->look[0] << 24) | 0xFFFFFF);
            Area_DrawEntry(&models->matSets[12], mtx[0], lights);
        }
        models = gAreaSubModels[0];
        if (models != NULL && models->active != 0) {
            for (i = 0; i < 7; i++) {
                Mtx_Scale(mtx[0], gGlowScaleTownArena[i], gGlowScaleTownArena[i], gGlowScaleTownArena[i]);
                Mtx_SetTrans(mtx[0], gGlowPosTownArena[i][0], gGlowPosTownArena[i][1], gGlowPosTownArena[i][2]);
                Mtx_Concat3x3(mtx[0], gCamMatrix);
                Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.25f * ((state->frame >> 2) & 3), 0.0f);
                Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                if (i < 3) {
                    Gs_SetState(RS_COLOR2, (state->look[0] << 24) | 0xFFFFFF);
                } else {
                    Gs_SetState(RS_COLOR2, ((u8)func_7D96D0() << 24) | 0xFFFFFF);
                }
                Area_DrawEntry(&models->matSets[0], mtx[0], 0);
            }
            if (Overlay_IsLoaded(OVERLAY_LOBBY) == 1) {
                for (i = 0; i < 3; i++) {
                    Mtx_Translation(mtx[0], 6600.0f, 170.0f, 8260.0f);
                    switch (i) {
                    case 1:
                    case 2:
                        Mtx_Concat3x3(mtx[0], (f32 *)gCamYawMtx);
                        break;
                    }
                    Gs_SetState(RS_COLOR2, LobbyWorld_GetFadeColor(i));
                    Area_DrawEntry(&models->matSets[2] + i, mtx[0], 0);
                }
            }
        }
        break;
    case AREA_ELDER_HALL:
        models = state->model;
        if (models != NULL && models->active != 0) {
            lights = models->mats;
            Mtx_Identity(mtx[0]);
            Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.25f * ((state->frame >> 2) & 3), 0.0f);
            Gs_SetState(RS_UV_MTX, (s32)uvMtx);
            Gs_SetState(RS_COLOR2, (state->look[0] << 24) | 0xFFFFFF);
            Area_DrawEntry(&models->matSets[11], mtx[0], lights);
        }
        models = gAreaSubModels[0];
        if (models != NULL && models->active != 0) {
            for (i = 0; i < 16; i++) {
                Mtx_Scale(mtx[0], gGlowScaleElderHall[i], gGlowScaleElderHall[i], gGlowScaleElderHall[i]);
                Mtx_SetTrans(mtx[0], gGlowPosElderHall[i][0], gGlowPosElderHall[i][1], gGlowPosElderHall[i][2]);
                if (gGlowEntryElderHall[i] == 10) {
                    Gs_SetState(RS_COLOR2, (pulseAlpha << 24) | 0xFFFFFF);
                } else {
                    Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.25f * ((state->frame >> 2) & 3), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                    Gs_SetState(RS_COLOR2, (state->look[0] << 24) | 0xFFFFFF);
                }
                Area_DrawEntry(&models->matSets[gGlowEntryElderHall[i]], mtx[0], 0);
            }
        }
        break;
    case AREA_HUB_0x89:
        if (propSet != NULL && propSet->active != 0) {
            for (i = 0; i < 12; i++) {
                Mtx_Scale(mtx[0], gGlowScaleHub89[i], gGlowScaleHub89[i], gGlowScaleHub89[i]);
                Mtx_SetTrans(mtx[0], gGlowPosHub89[i][0], gGlowPosHub89[i][1], gGlowPosHub89[i][2]);
                Mtx_Concat3x3(mtx[0], (f32 *)gCamYawMtx);
                entry = &propSet->matSets[gGlowEntryHub89[i]];
                if (gGlowEntryHub89[i] == 3) {
                    Gs_SetState(RS_COLOR2, (pulseAlpha << 24) | 0xFFFFFF);
                } else {
                    Gs_SetState(RS_COLOR2, (state->look[0] << 24) | 0xFFFFFF);
                    Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.25f * ((state->frame >> 2) & 3), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                }
                Area_DrawEntry(entry, mtx[0], 0);
            }
            entry = &propSet->matSets[7];
            Mtx_Identity(mtx[0]);
            Gs_SetState(RS_COLOR2, (state->look[0] << 24) | 0xFFFFFF);
            Area_DrawEntry(entry, mtx[0], 0);
        }
        {
        u8 level = Village_FacilityLevel(2);
        Model *villageSet;
        switch (level) {
        case 0:
        case 1:
        case 2:
            villageSet = gAreaSubModels[2];
            if (villageSet != NULL && villageSet->active != 0) {
                Mtx_Identity(mtx[0]);
                entry = &villageSet->matSets[gGlowVillageEntries[level]];
                Mtx_Translation(uvMtx[0], 0.0f, (state->frame & 0x1F) / 32.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                Area_DrawEntry(entry, mtx[0], 0);
            }
            break;
        }
        }
        break;
    case AREA_HUB_0xAA:
    case AREA_HUB_0xAB:
    case AREA_HUB_0xAC:
        for (i = 0; i < 4; i++) {
            entry = &propSet->matSets[gGlowEntryHubAA[i]];
            Mtx_Identity(mtx[0]);
            Mtx_SetTrans(mtx[0], gGlowPosHubAA[i][0], gGlowPosHubAA[i][1], gGlowPosHubAA[i][2]);
            Mtx_RotateY(mtx[0], gGlowRotYHubAA[i]);
            if (gGlowEntryHubAA[i] == 1) {
                Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.25f * ((state->frame & 0xC) >> 2), 0.0f);
                Gs_SetState(RS_UV_MTX, (s32)uvMtx);
            }
            Gs_SetState(RS_COLOR2, -1);
            Area_DrawEntry(entry, mtx[0], 0);
            Draw_SetDefaultState();
        }
        break;
    case AREA_ONLINE_HOUSE_0:
        if (propSet != NULL && propSet->active != 0) {
            Mtx_Translation(mtx[0], 2576.0f, 298.0f, 2452.0f);
            Mtx_Concat3x3(mtx[0], gCamMatrix);
            entry = &propSet->matSets[1];
            Gs_SetState(RS_COLOR2, (pulseAlpha << 24) | 0xFFFFFF);
            Area_DrawEntry(entry, mtx[0], 0);
            entry = &propSet->matSets[2];
            Gs_SetState(RS_COLOR2, (state->look[0] << 24) | 0xFFFFFF);
            Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.25f * ((state->frame >> 2) & 3), 0.0f);
            Gs_SetState(RS_UV_MTX, (s32)uvMtx);
            Area_DrawEntry(entry, mtx[0], 0);
        }
        break;
    case AREA_ONLINE_HOUSE_1:
        if (propSet != NULL && propSet->active != 0) {
            for (i = 0; i < 12; i++) {
                Mtx_Translation(mtx[0], gGlowPosOnlineHouse1[i][0], gGlowPosOnlineHouse1[i][1], gGlowPosOnlineHouse1[i][2]);
                Mtx_Concat3x3(mtx[0], (f32 *)gCamYawMtx);
                entry = &propSet->matSets[gGlowEntryOnlineHouse1[i]];
                if (gGlowEntryOnlineHouse1[i] == 1) {
                    Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
                    Gs_SetState(RS_COLOR2, (pulseAlpha << 24) | 0xFFFFFF);
                } else {
                    Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
                    Gs_SetState(RS_COLOR2, (state->look[0] << 24) | 0xFFFFFF);
                    Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.25f * ((state->frame >> 2) & 3), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                }
                Area_DrawEntry(entry, mtx[0], 0);
            }
        }
        break;
    case AREA_ONLINE_HOUSE_2:
        if (propSet != NULL && propSet->active != 0) {
            for (i = 0; i < 20; i++) {
                Mtx_Translation(mtx[0], gGlowPosOnlineHouse2[i][0], gGlowPosOnlineHouse2[i][1], gGlowPosOnlineHouse2[i][2]);
                Mtx_Concat3x3(mtx[0], (f32 *)gCamYawMtx);
                entry = &propSet->matSets[gGlowEntryOnlineHouse2[i]];
                if (gGlowEntryOnlineHouse2[i] == 1) {
                    Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
                    Gs_SetState(RS_COLOR2, (pulseAlpha << 24) | 0xFFFFFF);
                } else {
                    Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
                    Gs_SetState(RS_COLOR2, (state->look[0] << 24) | 0xFFFFFF);
                    Mtx_Translation(uvMtx[0], 0.25f * (state->frame & 3), 0.25f * ((state->frame >> 2) & 3), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32)uvMtx);
                }
                Area_DrawEntry(entry, mtx[0], 0);
            }
        }
        break;
    }
    Gs_SetState(RS_ZWRITE, 1);
    Gs_SetState(RS_ALPHA_REF, 0x80);
}

extern s8 gNoticeCount;           /* the notices queued */
extern s8 gNoticeHead;           /* the first one */
extern s8 gNoticeWait;           /* frames until the next notice */
extern Notice gNoticeQueue[NOTICE_QUEUE_LEN];

void *memset(void *dst, s32 c, u32 n);
/* Empties the notice queue (gNoticeQueue) and clears the banner shown and the notice ids. */
void Notice_ResetQueue(void)
{
    gNoticeCount = 0;
    gNoticeHead = 0;
    gScene.bannerNotice = 0xFF;
    gNoticeWait = 0;
    gScene.noticeId = 0;
    memset(gNoticeQueue, 0, sizeof(gNoticeQueue));
}

/* 1 when a queued notice has id k. */
s32 Notice_IsQueued(u8 k)
{
    s16 i;

    if (gNoticeCount == 0) {
        return 0;
    }
    for (i = 0; i < gNoticeCount; i++) {
        if (gNoticeQueue[(s16)((gNoticeHead + i) % NOTICE_QUEUE_LEN)].id == k) {
            return 1;
        }
    }
    return 0;
}

void NoticeBanner_Spawn(u8 kind, s16 textNo, s16 subject, u8 noticeId);
void NoticeBanner_SpawnString(char *msg, u8 noticeId);

/* Shows the first queued notice (60 frames apart, not while gScene.eventRunning is set, nor while
 * gQuest.inputLock is positive outside the town) once its delay has run out. */
void Notice_Update(void)
{
    Notice *notice;

    if (gScene.eventRunning != 0) {
        return;
    }
    if (gQuest.inputLock > 0 && gScene.inLobby == 0) {
        return;
    }
    if (gNoticeWait > 0) {
        gNoticeWait--;
        return;
    }
    if (gNoticeCount <= 0) {
        return;
    }
    notice = &gNoticeQueue[gNoticeHead];
    if (notice->delay > 0) {
        notice->delay--;
        if (notice->delay < 0) {
            notice->delay = 0;
        }
        return;
    }
    if (notice->kind == NOTICE_QUEUED_STRING) {
        NoticeBanner_SpawnString(notice->string, notice->id);
    } else {
        NoticeBanner_Spawn(notice->kind, notice->text, notice->subject, notice->id);
    }
    if (++gNoticeHead >= NOTICE_QUEUE_LEN) {
        gNoticeHead = 0;
    }
    gNoticeWait = 60;
    gNoticeCount--;
}

Obj *Obj_Alloc(s32 n);
void NoticeBanner_Task(Obj *o);

/* Starts a banner (an object of kind 1) of notice kind `kind` (NOTICE_KIND_*) with
 * Notice_Push's text and subject, and marks notice noticeId as the one shown
 * (gScene.bannerNotice). */
void NoticeBanner_Spawn(u8 kind, s16 textNo, s16 subject, u8 noticeId)
{
    Obj *obj = Obj_Alloc(1);

    if (obj != NULL) {
        obj->kind = 1;
        obj->keep = 1;
        obj->update = NoticeBanner_Task;
        obj->type = kind;
        obj->var5 = textNo;
        obj->timer = subject;
        gScene.bannerNotice = noticeId;
    }
}

/* Queues a notice of the given kind (NOTICE_KIND_*), with its text (a message or
 * phrase) and its subject (an item, a player or a count); returns its id, or 0xFF
 * when the queue is full. */
u8 Notice_Push(u8 kind, s16 text, s16 subject)
{
    s8 i;

    if (gNoticeCount >= NOTICE_QUEUE_LEN - 1) {
        return 0xFF;
    }
    gScene.noticeId = (gScene.noticeId + 1) & 0x7F;
    i = (gNoticeHead + gNoticeCount) % NOTICE_QUEUE_LEN;
    gNoticeQueue[i].kind = kind;
    gNoticeQueue[i].text = text;
    gNoticeQueue[i].id = gScene.noticeId;
    gNoticeQueue[i].delay = 0;
    gNoticeQueue[i].subject = subject;
    gNoticeCount++;
    return gScene.noticeId;
}

Obj *Obj_Alloc(s32 n);
void NoticeBanner_Task(Obj *o);

/* Starts a NOTICE_KIND_STRING banner showing msg, and marks notice noticeId as the one shown. */
void NoticeBanner_SpawnString(char *msg, u8 noticeId)
{
    Obj *obj = Obj_Alloc(1);

    if (obj != NULL) {
        NoticeText *text;

        obj->kind = 1;
        obj->keep = 1;
        obj->update = NoticeBanner_Task;
        obj->type = NOTICE_KIND_STRING;
        text = obj->buf;
        text->msg = msg;
        gScene.bannerNotice = noticeId;
    }
}

/* Queues an item notice; its id, or 0xFF when the queue is full. */
u8 Notice_PushItem(char *item)
{
    s8 i;

    if (gNoticeCount >= NOTICE_QUEUE_LEN - 1) {
        return 0xFF;
    }
    gScene.noticeId = (gScene.noticeId + 1) & 0x7F;
    i = (gNoticeHead + gNoticeCount) % NOTICE_QUEUE_LEN;
    gNoticeQueue[i].id = gScene.noticeId;
    gNoticeQueue[i].kind = NOTICE_QUEUED_STRING;
    gNoticeQueue[i].string = item;
    gNoticeQueue[i].delay = 0;
    gNoticeCount++;
    return gScene.noticeId;
}

char *strcpy(char *dst, const char *src);
u32 strlen(const char *s);
/* Queues a text notice (a copy, NOTICE_TEXT_LEN characters at most); returns its id,
 * or 0xFF when the queue is full. */
u8 Notice_PushText(char *text)
{
    s32 i;
    s32 k;

    if (gNoticeCount >= NOTICE_QUEUE_LEN - 1) {
        return 0xFF;
    }
    gScene.noticeId = (gScene.noticeId + 1) & 0x7F;
    i = (s8)((gNoticeHead + gNoticeCount) % NOTICE_QUEUE_LEN);
    gNoticeQueue[i].id = gScene.noticeId;
    gNoticeQueue[i].kind = NOTICE_QUEUED_STRING;
    gNoticeQueue[i].string = gNoticeQueue[i].buf;
    gNoticeQueue[i].delay = 0;
    if ((u32)strlen(text) < NOTICE_TEXT_LEN) {
        strcpy(gNoticeQueue[i].buf, text);
        gNoticeQueue[i].buf[NOTICE_TEXT_LEN] = 0;
        gNoticeQueue[i].buf[NOTICE_TEXT_LEN + 1] = 0;
    } else {
        for (k = 0; k < NOTICE_TEXT_LEN; k++) {
            gNoticeQueue[i].buf[k] = text[k];
        }
        gNoticeQueue[i].buf[NOTICE_TEXT_LEN] = 0;
        gNoticeQueue[i].buf[NOTICE_TEXT_LEN + 1] = 0;
    }
    gNoticeCount++;
    return gScene.noticeId;
}

/* The update of a notice banner: runs its state (Obj.state): 0 NoticeBanner_Start, 1 NoticeBanner_Run, 2 NoticeBanner_End, 3 NoticeBanner_Free. */
void NoticeBanner_Task(Obj *obj)
{
    u8 step;

    step = obj->state;
    switch (step) {
    case 0:
        NoticeBanner_Start(obj);
        break;
    case 1:
        NoticeBanner_Run(obj);
        break;
    case 2:
        NoticeBanner_End(obj);
        break;
    case 3:
        NoticeBanner_Free(obj);
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

/* Sets up a notice banner (NoticeBanner_Spawn, NoticeBanner_SpawnString): plays its
 * sound, builds its text by its kind (NOTICE_KIND_*: an item's name from gItemNames, a
 * player's name, a message of gNoticeBannerTexts ...), measures each line, centres the
 * banner above the screen, and gives it a draw slot drawn by TextNotice_Draw. The object
 * keeps the line count in var1, the widest line in pos[2], its slide speed in fvar and
 * its time on screen in var3. */
void NoticeBanner_Start(Obj *obj)
{
    NoticeText *banner = obj->buf;
    char line[0x80];
    s16 w;
    s32 i;
    s32 n;

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->keep = 1;
    Sound_PlaySeFlat(0, 0x10, 0);
    switch (obj->type) {
    case NOTICE_KIND_MESSAGE:
        sprintf(banner->text, "~C00%s", gNoticeBannerTexts[obj->var5]);
        break;
    case NOTICE_KIND_ITEM:
        sprintf(banner->text, "~C02%s~C00%s", gItemNames[(u16)obj->timer], gNoticeBannerItemTexts[obj->var5]);
        break;
    case NOTICE_KIND_TEXT_ITEM:
        sprintf(banner->text, "~C00%s~C02%s", gNoticeBannerTextsItem[obj->var5], gItemNames[(u16)obj->timer]);
        break;
    case NOTICE_KIND_PLAYER:
        sprintf(banner->text, "~C02%s~C00%s", gPlayers[obj->timer].name, gNoticeBannerPlayerTexts[obj->var5]);
        break;
    case NOTICE_KIND_STRING:
        sprintf(banner->text, "~C00%s", banner->msg);
        break;
    case NOTICE_KIND_ITEM_MADE:
        sprintf(banner->text, "~C02%s~C00%s~C05%d~C00%s", gItemNames[(u16)obj->var5], gNoticeMadeTexts[0], obj->timer, gNoticeMadeCountText[0]);
        break;
    }
    obj->var1 = 1;
    w = 0;
    for (i = 0, n = 0; i < 0x80; i++) {
        if (banner->text[i] == '\n') {
            line[n] = 0;
            banner->width[obj->var1 - 1] = Text_DisplayLength(line);
            if (w < banner->width[obj->var1 - 1]) {
                w = banner->width[obj->var1 - 1];
            }
            obj->var1++;
            n = 0;
        } else {
            line[n] = banner->text[i];
            if (line[n] == 0) {
                break;
            }
            n++;
        }
    }
    banner->width[obj->var1 - 1] = Text_DisplayLength(line);
    if (w < banner->width[obj->var1 - 1]) {
        w = banner->width[obj->var1 - 1];
    }
    obj->pos[0] = 320.0f - 10.0f * (w / 2.0f);
    obj->pos[1] = -20.0f;
    obj->pos[2] = w;
    obj->fvar = 40.0f;
    obj->var3 = 60;
    obj->slot = DrawSlotB_Alloc();
    if (obj->slot != -1) {
        obj->ds = DrawSlotB_Get(obj->slot);
        obj->ds->owner = obj;
        obj->ds->draw = TextNotice_Draw;
        NoticeBanner_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);

/* A notice banner's run state: it slides down to y 224, stays for its time (shorter once more
 * notices wait), then slides up out of the screen and ends. */
u32 NoticeBanner_Run(Obj *obj)
{
    switch (obj->var0) {
    case 0:
        if ((obj->pos[1] += obj->fvar) >= 224.0f) {
            obj->var0++;
            obj->pos[1] = 224.0f;
        }
        break;
    case 1:
        obj->var3--;
        if (obj->var3 <= 0 || (obj->var3 <= 30 && gNoticeCount > 0)) {
            obj->fvar *= -1.0f;
            gScene.bannerNotice = 0xFF;
            obj->var0++;
        }
        break;
    case 2:
        if ((obj->pos[1] += obj->fvar) <= -20.0f) {
            obj->state++;
        }
        break;
    }
    return OT_AddByDepth(gDrawOtText3, obj->ds, 0x10, 1);
}

/* Hides the banner and frees its draw slot; the next state frees the object. */
void NoticeBanner_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
    DrawSlotB_Free(obj->slot);
}

/* Frees the banner object. */
void NoticeBanner_Free(Obj *obj)
{
    Obj_Free(obj);
}

struct Frame;
void WindowFrame_Draw(struct Frame *b, s16 *off);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Text_SetSize(s32 size);
void Text_Printf(const char *fmt, ...);

/* Draws a notice banner (its draw slot's update): a window frame sized to its lines
 * (Obj.var1) and its widest line (pos[2]), then each line of its text, centred by its
 * width (NoticeText.width). */
void TextNotice_Draw(DrawSlot *slot)
{
    Obj *obj = slot->owner;
    NoticeText *banner = obj->buf;
    Frame box;
    char line[0x80];
    s32 n;
    s32 pos;
    s32 i;

    Gs_SetState(RS_ALPHA_REF, 0);
    box.x = obj->pos[0] - 4.0f;
    box.y = obj->pos[1] - 2.0f;
    box.w = (s16)obj->pos[2] * 10 + 8;
    box.h = obj->var1 * 22 + 4;
    box.style = 0x80000001;
    box.color = 0xFF7F7052;
    box.color2 = 0xBD101010;
    WindowFrame_Draw(&box, 0);
    Font_SetSpacing(0x14, 0x14);
    Text_SetSize(3);
    pos = 0;
    for (n = 0; n < obj->var1; n++) {
        for (i = 0; pos < 0x80; pos++, i++) {
            if (banner->text[pos] == '\n') {
                line[i] = 0;
                pos++;
                break;
            }
            line[i] = banner->text[pos];
            if (line[i] == 0) {
                break;
            }
        }
        Font_SetPos(320.0f - 10.0f * (banner->width[n] / 2.0f), (s16)obj->pos[1] + n * 20);
        Text_Printf("%s", line);
    }
    Gs_SetState(RS_ALPHA_REF, 0x80);
}

void AreaProp_Task(Obj *o);

/* Starts an area prop (an object of kind 6, AreaProp_Task) of type k: the type picks
 * its place and model in the area's tables. */
void AreaProp_Spawn(s16 k)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 6;
        obj->type = k;
        obj->update = AreaProp_Task;
    }
}

/* The update of an area prop: runs its state (Obj.state): 0 AreaProp_Start, 1 AreaProp_Run, 2 AreaProp_End, 3 AreaProp_Free. */
void AreaProp_Task(Obj *obj)
{
    u8 step;

    step = obj->state;
    switch (step) {
    case 0:
        AreaProp_Start(obj);
        break;
    case 1:
        AreaProp_Run(obj);
        break;
    case 2:
        AreaProp_End(obj);
        break;
    case 3:
        AreaProp_Free(obj);
        break;
    }
}

extern f32 gAreaPropPosHouse[];
extern f32 gAreaPropPosHub83[][3];
extern f32 gAreaPropPosHub84[][3];
extern f32 gAreaPropPosHub85[][3];
extern f32 gAreaPropPosElderHall[][3];
extern f32 gAreaPropPosHub89[][3];
extern f32 gAreaPropPosHubAA[];
extern f32 gAreaPropPosOnlineHouse1[];
extern f32 gAreaPropPosOnlineHouse2[];
u16 Rand_Next(s32 a);
void Vec_Copy(f32 *dst, f32 *src);
void AreaProp_Draw(DrawSlot *slot);

/* Sets up an area prop (AreaProp_Spawn): placed by the area and its type from the
 * area's table (gAreaPropPos*), with its turn time in var3 in some areas (200 to 327
 * frames at random), and a draw slot drawn by AreaProp_Draw. In other areas it is
 * freed. */
void AreaProp_Start(Obj *obj)
{
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->var3 = 0;
    obj->timer = 0;
    obj->fvar = 1.0f;
    switch (gScene.area) {
    case AREA_HOUSE:
    case AREA_HUB_0x57:
        obj->pos[0] = gAreaPropPosHouse[0];
        obj->pos[1] = gAreaPropPosHouse[1];
        obj->pos[2] = gAreaPropPosHouse[2];
        break;
    case AREA_HUB_0x83:
        Vec_Copy(obj->pos, gAreaPropPosHub83[obj->type]);
        switch (obj->type) {
        case 6:
            obj->var3 = 800;
            break;
        default:
            obj->var3 = (Rand_Next(1) & 0x7F) + 200;
            break;
        }
        break;
    case AREA_HUB_0x84:
        Vec_Copy(obj->pos, gAreaPropPosHub84[obj->type]);
        obj->var3 = (Rand_Next(1) & 0x7F) + 200;
        break;
    case AREA_HUB_0x85:
        obj->fvar = 0.99f;
        Vec_Copy(obj->pos, gAreaPropPosHub85[obj->type]);
        break;
    case AREA_TOWN_ARENA:
        obj->fvar = 0.0f;
        obj->var0 = 0;
        break;
    case AREA_ELDER_HALL:
        Vec_Copy(obj->pos, gAreaPropPosElderHall[obj->type]);
        obj->var3 = (Rand_Next(1) & 0x7F) + 200;
        break;
    case AREA_HUB_0x89:
        Vec_Copy(obj->pos, gAreaPropPosHub89[obj->type]);
        break;
    case AREA_HUB_0xAA:
    case AREA_HUB_0xAB:
    case AREA_HUB_0xAC:
        Vec_Copy(obj->pos, &gAreaPropPosHubAA[obj->type]);
        break;
    case AREA_ONLINE_HOUSE_1:
        Vec_Copy(obj->pos, &gAreaPropPosOnlineHouse1[obj->type]);
        break;
    case AREA_ONLINE_HOUSE_2:
        Vec_Copy(obj->pos, &gAreaPropPosOnlineHouse2[obj->type]);
        break;
    default:
        AreaProp_Free(obj);
        return;
    }
    obj->slot = DrawSlot_Alloc();
    obj->ds = DrawSlot_Get(obj->slot);
    if (obj->ds != NULL) {
        obj->ds->owner = obj;
        obj->ds->draw = AreaProp_Draw;
        AreaProp_Run(obj);
    } else {
        AreaProp_Free(obj);
    }
}

s32 Motion_PassedFrame(void *obj, s32 layer, f32 frame);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void Vec_Scale(f32 *out, f32 *in, f32 s);
void Vec_Copy(f32 *dst, f32 *src);
void Mtx_GetTrans(f32 *out, f32 *m);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
extern f32 gCamMatrix[];

/* The run state of an area prop: by area, it hides at a frame of the local player's
 * motion, turns its timers, or (AREA_TOWN_ARENA) fades in and out in front of the camera;
 * then its draw slot is added to the ordering table var4 picks (0 sorted, 1 the 32-deep,
 * 2 the 8-deep, 3 the stage list). */
void AreaProp_Run(Obj *obj)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    f32 offset[3];
    f32 camPos[3];

    obj->timer++;
    obj->var4 = 1;
    switch (gScene.area) {
    case AREA_HOUSE:
    case AREA_HUB_0x57:
        if (pl->motionId[0] == 0x287 && Motion_PassedFrame(pl, 0, 232.0f)) {
            obj->visible = 0;
        }
        break;
    case AREA_HUB_0x83:
        switch (obj->type) {
        case 6:
            obj->var4 = 2;
        default:
            if (obj->timer >= obj->var3) {
                obj->timer = 0;
            }
            break;
        }
        break;
    case AREA_HUB_0x84:
        switch (obj->type) {
        case 5:
            obj->var4 = 2;
            if (++obj->var3 >= 800) {
                obj->var3 = 0;
            }
            break;
        default:
            if (obj->timer >= obj->var3) {
                obj->timer = 0;
            }
            break;
        }
        break;
    case AREA_HUB_0x85:
        obj->var4 = 0;
        break;
    case AREA_TOWN_ARENA:
        switch (obj->var0) {
        case 0:
            if ((obj->fvar += 0.022222223f) >= 1.0f) {
                obj->var0++;
                obj->fvar = 1.0f;
                obj->timer = 0;
            }
            break;
        case 1:
            if (++obj->timer > 6) {
                obj->var0++;
            }
            break;
        case 2:
            if ((obj->fvar -= 0.022222223f) <= 0.0f) {
                obj->state++;
                obj->fvar = 0.0f;
                obj->timer = 0;
            }
            break;
        }
        if (obj->fvar == 0.0f) {
            return;
        }
        Vec_Copy(offset, gCamDir);
        Vec_Scale(offset, offset, -30.0f);
        Mtx_GetTrans(camPos, gCamMatrix);
        Vec_Add(obj->pos, offset, camPos);
        obj->var4 = 3;
        break;
    case AREA_ELDER_HALL:
        if (obj->timer >= obj->var3) {
            obj->timer = 0;
        }
        break;
    case AREA_HUB_0x89:
        obj->var4 = 2;
        break;
    case AREA_HUB_0xAC:
        if (pl->motionId[0] == 0xFA && Motion_PassedFrame(pl, 0, 154.0f)) {
            obj->visible = 0;
        }
        break;
    }
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    switch (obj->var4) {
    default:
        OT_AddByDepth(gDrawOtSorted, obj->ds, 0x40, 0);
        break;
    case 1:
        OT_AddByDepth(gDrawOt32, obj->ds, 0x20, 0);
        break;
    case 2:
        OT_AddByDepth(gDrawOt8, obj->ds, 8, 1);
        break;
    case 3:
        OT_AddByDepth(gDrawListStage, obj->ds, 1, 0);
        break;
    }
}

/* Hides the prop and frees its draw slot; the next state frees the object. */
void AreaProp_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
    DrawSlot_Free(obj->slot);
}

/* Frees the prop object. */
void AreaProp_Free(Obj *obj)
{
    Obj_Free(obj);
}

/* The colour of the props darkened by night: white by day, grey 0x30 at full night
 * (gAreaState.look[0] 0xFF), else 255 - 207 * night / 255 in each channel. */
u32 Area_NightTint(void)
{
    AreaState *state = &gAreaState;
    u8 r, g, b;
    f32 shade;

    if (gAreaState.look[0] == 0) {
        b = g = r = 0xFF;
    } else if (state->look[0] == 0xFF) {
        b = g = r = 0x30;
    } else {
        shade = 255.0f - 207.0f * (state->look[0] / 255.0f);
        r = shade;
        g = shade;
        b = shade;
    }
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

extern f32 gAreaPropRotHouse[];
extern f32 gAreaPropScaleHub83[];
extern s16 gAreaPropEntryHub83[];
extern f32 gAreaPropScaleHub84[];
extern s16 gAreaPropEntryHub84[];
extern s16 gAreaPropEntryHub85[];
extern f32 gAreaPropScaleElderHall[];
extern s16 gAreaPropEntryElderHall[];
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

/* Draws an area prop (AreaProp_Start's draw slot): its models by the area and its
 * type, at its position. The turning props spin once per var3 frames; some scroll their
 * texture by their timer. */
void AreaProp_Draw(DrawSlot *slot)
{
    f32 mtx[16];
    f32 uvMtx[16];
    AreaState *state;
    Model *set;
    s16 i;
    ModelMatSet *entry;
    ModelMat *lights;
    s16 k;
    Obj *obj;

    obj = slot->owner;
    set = gAreaSubModels[0];
    state = &gAreaState;
    if (obj->visible == 0) {
        return;
    }
    Mtx_Identity(mtx);
    switch (gScene.area) {
    case AREA_HOUSE:
    case AREA_HUB_0x57:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
        entry = &set->matSets[1];
        Gs_SetState(RS_ALPHA_REF, 0x80);
        Gs_SetState(RS_COLOR2, -1);
        if (entry != NULL && entry->id != -1) {
            Draw_ApplyStates(entry->flags);
            Model_Draw(entry->id, 0);
        }
        Mtx_RotateXYZ(mtx, gAreaPropRotHouse[0], gAreaPropRotHouse[1], gAreaPropRotHouse[2]);
        entry = &set->matSets[obj->type];
        break;
    case AREA_HUB_0x83:
        set = state->model;
        if (set == NULL || set->active == 0) {
            goto end;
        }
        lights = set->mats;
        entry = &set->matSets[gAreaPropEntryHub83[obj->type]];
        Gs_SetLightList(lights, entry);
        Mtx_Scale(mtx, gAreaPropScaleHub83[obj->type], gAreaPropScaleHub83[obj->type], gAreaPropScaleHub83[obj->type]);
        switch (obj->type) {
        case 6:
            Mtx_SetTrans(mtx, obj->pos[0], obj->pos[1], obj->pos[2]);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            Gs_SetState(RS_COLOR2, Area_NightTint());
            Gs_SetState(RS_ALPHA_REF, 0);
            Mtx_Translation(uvMtx, (f32)obj->timer / (f32)obj->var3, 0.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32)uvMtx);
            if (entry != NULL && entry->id != -1) {
                Draw_ApplyStates(entry->flags);
                Model_Draw(entry->id, 0);
            }
            entry = &set->matSets[gAreaPropEntryHub83[obj->type + 1]];
            Gs_SetLightList(lights, entry);
            Gs_SetState(RS_COLOR2, -1);
            break;
        default:
            Mtx_RotateY(mtx, DEG2RAD(360.0f * (1.0f - (f32)obj->timer / (f32)obj->var3)));
            break;
        }
        break;
    case AREA_HUB_0x84:
        set = state->model;
        if (set == NULL || set->active == 0) {
            goto end;
        }
        lights = set->mats;
        entry = &set->matSets[gAreaPropEntryHub84[obj->type]];
        Gs_SetLightList(lights, entry);
        Mtx_Scale(mtx, gAreaPropScaleHub84[obj->type], gAreaPropScaleHub84[obj->type], gAreaPropScaleHub84[obj->type]);
        switch (obj->type) {
        case 5:
            Mtx_SetTrans(mtx, obj->pos[0], obj->pos[1], obj->pos[2]);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            Gs_SetState(RS_COLOR2, Area_NightTint());
            Gs_SetState(RS_ALPHA_REF, 0);
            Mtx_Translation(uvMtx, (f32)obj->var3 / 800.0f, 0.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32)uvMtx);
            if (entry != NULL && entry->id != -1) {
                Draw_ApplyStates(entry->flags);
                Model_Draw(entry->id, 0);
            }
            entry = &set->matSets[gAreaPropEntryHub84[obj->type + 1]];
            Gs_SetLightList(lights, entry);
            Mtx_Translation(uvMtx, (f32)(obj->timer & 0x3FF) / 1024.0f, 0.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32)uvMtx);
            break;
        default:
            Mtx_RotateY(mtx, DEG2RAD(360.0f * (1.0f - (f32)obj->timer / (f32)obj->var3)));
            break;
        }
        break;
    case AREA_HUB_0x85:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        entry = &set->matSets[gAreaPropEntryHub85[obj->type]];
        if (obj->type == 0) {
            Mtx_Translation(uvMtx, 0.0f, (f32)(obj->timer & 0x7F) / 128.0f, 0.0f);
        } else {
            Mtx_Translation(uvMtx, 0.5f * (f32)(obj->timer & 1), 0.25f * (f32)((obj->timer & 7) >> 1), 0.0f);
        }
        Gs_SetState(RS_UV_MTX, (s32)uvMtx);
        Gs_SetState(RS_ZWRITE, 0);
        break;
    case AREA_TOWN_ARENA:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        entry = &set->matSets[1];
        Mtx_Copy(mtx, gCamMatrix);
        Gs_SetState(RS_ZWRITE, 0);
        Gs_SetState(RS_ZTEST, RS_CMP_ALWAYS);
        Gs_SetState(RS_COLOR2, ((u8)(255.0f * obj->fvar) << 24) | 0xFFFFFF);
        break;
    case AREA_ELDER_HALL:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        entry = &set->matSets[gAreaPropEntryElderHall[obj->type]];
        Mtx_Scale(mtx, gAreaPropScaleElderHall[obj->type], gAreaPropScaleElderHall[obj->type], gAreaPropScaleElderHall[obj->type]);
        Mtx_RotateY(mtx, DEG2RAD(360.0f * (1.0f - (f32)obj->timer / (f32)obj->var3)));
        Gs_SetState(RS_COLOR2, Area_NightTint());
        break;
    case AREA_HUB_0x89:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        Mtx_Translation(uvMtx, 0.0f, (f32)(s32)*(u8 *)&obj->timer / 256.0f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32)uvMtx);
        Mtx_SetTrans(mtx, obj->pos[0], obj->pos[1], obj->pos[2]);
        Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
        Gs_SetState(RS_ALPHA_REF, 0);
        for (i = 0; i < 2; i++) {
            entry = &set->matSets[i];
            if (entry != NULL && entry->id != -1) {
                Draw_ApplyStates(entry->flags);
                Model_Draw(entry->id, 0);
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
        entry = &gAreaSubModels[1]->matSets[k];
        Mtx_Translation(uvMtx, 0.0f, (f32)(obj->timer & 0x3F) / 64.0f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32)uvMtx);
        if (entry != NULL && entry->id != -1) {
            Draw_ApplyStates(entry->flags);
            Model_Draw(entry->id, 0);
        }
        Draw_SetDefaultState();
        goto end;
    case AREA_HUB_0xAA:
    case AREA_HUB_0xAB:
    case AREA_HUB_0xAC:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        entry = &set->matSets[5];
        Mtx_RotateX(mtx, -0.40142572f);
        Mtx_RotateY(mtx, 2.8274333f);
        Mtx_RotateZ(mtx, -1.2217306f);
        break;
    case AREA_ONLINE_HOUSE_1:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        entry = &set->matSets[18];
        Mtx_RotateX(mtx, -1.2566371f);
        Mtx_RotateZ(mtx, 1.5707964f);
        break;
    case AREA_ONLINE_HOUSE_2:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        entry = &set->matSets[21];
        Mtx_RotateX(mtx, -1.2566371f);
        Mtx_RotateZ(mtx, 1.5707964f);
        break;
    default:
        if (set == NULL || set->active == 0) {
            goto end;
        }
        Gs_SetState(RS_COLOR2, -1);
        entry = &set->matSets[obj->type];
        break;
    }
    Mtx_SetTrans(mtx, obj->pos[0], obj->pos[1], obj->pos[2]);
    Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
    switch (obj->var4) {
    case 0:
    case 2:
    case 3:
    default:
        Gs_SetState(RS_ALPHA_REF, 0);
        break;
    case 1:
        Gs_SetState(RS_ALPHA_REF, 0x80);
        break;
    }
    if (entry != NULL && entry->id != -1) {
        Draw_ApplyStates(entry->flags);
        Model_Draw(entry->id, 0);
    }
end:
    Draw_SetDefaultState();
    Gs_SetState(RS_ZWRITE, 1);
    Gs_SetState(RS_ZTEST, RS_CMP_LEQUAL);
}

s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void SoundObj_Task(Obj *o);

/* Starts a sound object (an object of kind 0xC, SoundObj_Task) at pos, playing sound
 * seId of bank with volume curve range (Sound_PlaySeAtPosChecked). mode is its
 * SoundObj_Run case, kept in var0: mode 0 plays the sound at once and lasts frames
 * frames (0xFFFF: until freed); in the other modes frames is the time between plays. */
void SoundObj_Start(s32 bank, s32 seId, s32 range, f32 *pos, s16 frames, u8 mode)
{
    Obj *obj;

    if (mode == 0) {
        Sound_PlaySeAtPosChecked(bank, seId, 0, pos, range, 0);
    }
    obj = Obj_Alloc(0);
    if (obj != NULL) {
        obj->kind = 0xC;
        obj->update = SoundObj_Task;
        obj->type = range;
        obj->var0 = mode;
        obj->var1 = bank;
        obj->var2 = seId;
        obj->pos[0] = pos[0];
        obj->pos[1] = pos[1];
        obj->pos[2] = pos[2];
        obj->timer = frames;
    }
}

/* The update of a sound object: runs its state (Obj.state): 0 SoundObj_Init, 1 SoundObj_Run, 2 SoundObj_End, 3 SoundObj_Free. */
void SoundObj_Task(Obj *obj)
{
    u8 step;

    step = obj->state;
    switch (step) {
    case 0:
        SoundObj_Init(obj);
        break;
    case 1:
        SoundObj_Run(obj);
        break;
    case 2:
        SoundObj_End(obj);
        break;
    case 3:
        SoundObj_Free(obj);
        break;
    }
}

/* The sound object's first state: shows it, with no draw function; its counter var3
 * starts at 0 in mode 0, else at the time between plays. */
void SoundObj_Init(Obj *obj)
{
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    if (obj->var0 == 0) {
        obj->var3 = 0;
        return;
    }
    obj->var3 = obj->timer;
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


/* The run state of a sound object (SoundObj_Start; its mode in var0, its bank and
 * sound in var1 and var2, its frames in timer). Mode 0 moves the playing sound's pan
 * every 5 frames until its frames run out. Modes 1 and 3 replay it every timer frames
 * (mode 1 give or take 16 at random) at the point of the area's river nearest to the
 * camera: the river is a line of three points per area (gRiverPtsDay*), and the object
 * ends in an area without one. Mode 2 replays it every timer frames where it is. */
void SoundObj_Run(Obj *obj)
{
    f32 cam[3];
    f32 dir[3];
    f32 hit[3];
    f32 toCam[3];
    f32 nearPts[2][3];
    HitCapsule cap;
    f32 *river;
    f32 *nearSecond;
    f32 distFirst;
    f32 distSecond;
    s32 segment;
    u8 nearSeg;

    switch (obj->var0) {
    case 0:
        if ((u16)obj->timer != 0xFFFF && --obj->timer <= 0) {
            obj->state++;
            break;
        }
        obj->var3++;
        if (obj->var3 >= 5) {
            obj->var3 = 0;
            Sound_PlaySeAtPosChecked(obj->var1, obj->var2, 0, obj->pos, obj->type, 1);
        }
        break;
    case 1:
    case 3:
        if (--obj->var3 <= 0) {
            if (obj->var0 == 1) {
                obj->var3 = obj->timer + (s16)((Rand_Next(1) & 0x1F) - 16);
            } else {
                obj->var3 = obj->timer;
            }
            switch (Area_DayId(gScene.area)) {
            case AREA_SWAMP_NIGHT_7:
            case AREA_SWAMP_7:
                river = gRiverPtsDay48;
                break;
            case AREA_SWAMP_NIGHT_9:
            case AREA_SWAMP_9:
                river = gRiverPtsDay4B;
                break;
            case AREA_JUNGLE_0:
                river = gRiverPtsDay6E;
                break;
            case AREA_JUNGLE_3:
                river = gRiverPtsDay71;
                break;
            case AREA_JUNGLE_4:
                river = gRiverPtsDay72;
                break;
            case AREA_JUNGLE_10:
                river = gRiverPtsDay78;
                break;
            case AREA_VOLCANO_0:
                river = gRiverPtsDayA1;
                break;
            default:
                obj->state++;
                return;
            }
            Mtx_GetTrans(cam, gCamMatrix);
            distFirst = Seg_NearestPointDist(nearPts[0], river, river + 3, cam);
            nearSecond = nearPts[1];
            distSecond = Seg_NearestPointDist(nearSecond, river + 3, river + 6, cam);
            Vec_Diff(dir, nearPts[0], nearSecond);
            if (Math_Absf(dir[0]) < 0.001f && Math_Absf(dir[1]) < 0.001f && Math_Absf(dir[2]) < 0.001f) {
                Vec_Diff(dir, river, river + 6);
                Vec_Diff(toCam, cam, river + 3);
                if (Vec_Dot(dir, toCam) >= 0.0f) {
                    nearSeg = 0;
                } else {
                    nearSeg = 1;
                }
            } else if (distFirst <= distSecond) {
                nearSeg = 0;
            } else {
                nearSeg = 1;
            }
            segment = nearSeg;
            cap.radius = 0.0f;
            if (!segment) {
                Vec_Copy(cap.a, river);
                Vec_Copy(cap.b, river + 3);
            } else {
                Vec_Copy(cap.a, river + 3);
                Vec_Copy(cap.b, river + 6);
            }
            if (Seg_ClipPlane(cap.a, cam, dir, hit)) {
                Vec_Copy(obj->pos, hit);
            } else if (!segment) {
                Vec_Copy(obj->pos, river);
            } else {
                Vec_Copy(obj->pos, river + 6);
            }
            Sound_PlaySeAtPosChecked(obj->var1, obj->var2, 0, obj->pos, obj->type, 0);
        }
        break;
    case 2:
        obj->var3++;
        if (obj->var3 >= obj->timer) {
            obj->var3 = 0;
            Sound_PlaySeAtPosChecked(obj->var1, obj->var2, 0, obj->pos, obj->type, 0);
        }
        break;
    }
}

/* Hides the sound object; the next state frees it. */
void SoundObj_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Frees the sound object. */
void SoundObj_Free(Obj *obj)
{
    Obj_Free(obj);
}

/* An object for an area atmosphere of the given type, from the pool: with one block of
 * buffer for the types with layers (8, 10, 11, 13 and 14), none otherwise. */
Obj *AreaAtmos_Alloc(s32 type)
{
    switch ((s16) type) {
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

Obj *AreaAtmos_Alloc(s32 k);
void AreaAtmos_Task(Obj *);

/* Starts an area atmosphere object (kind 0xD, AreaAtmos_Task) of the given type:
 * Area_StartObjects starts the area's own (gAreaInfo). */
void AreaAtmos_Spawn(s32 type)
{
    Obj *obj = AreaAtmos_Alloc(type);

    if (obj != NULL) {
        obj->kind = 0xD;
        obj->type = type;
        obj->update = AreaAtmos_Task;
    }
}

/* The update of an area atmosphere object: runs its state (Obj.state): 0 AreaAtmos_Setup, 1 AreaAtmos_Update, 2 AreaAtmos_End, 3 AreaAtmos_Free. */
void AreaAtmos_Task(Obj *obj)
{
    u8 step;

    step = obj->state;
    switch (step) {
    case 0:
        AreaAtmos_Setup(obj);
        break;
    case 1:
        AreaAtmos_Update(obj);
        break;
    case 2:
        AreaAtmos_End(obj);
        break;
    case 3:
        AreaAtmos_Free(obj);
        break;
    }
}

s16 Area_MonsterState(u16 area);
f32 AreaAtmos_LayerSpeed(u8 i, s16 n);
f32 AreaAtmos_LayerAlpha(u8 i, s16 n);
s16 Area_BoundaryZone(Player *p);
void AreaAtmos_Draw(DrawSlot *slot);
extern s16 gAreaAtmosLayerCounts[];

/* Sets up an area atmosphere object (state 0): its alpha (fvar) by the area and the
 * local player's position, its layers by its type (gAreaAtmosLayerCounts: their count;
 * their speeds and alphas from AreaAtmos_LayerSpeed and AreaAtmos_LayerAlpha by the
 * area's monster state), and a draw slot that runs AreaAtmos_Draw. The zone switch keeps
 * `case 0:` beside `default:` for the match: without the label the compiler emits one
 * instruction fewer than the game's code. */
void AreaAtmos_Setup(Obj *obj)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    s16 i;
    s16 n;
    AtmosLayer *atmosLayer;
    AtmosScroll *atmosScroll;
    AtmosUv *layerUv;
    s16 count;

    obj->state++;
    obj->visible = 1;
    obj->timer = 0;
    obj->var3 = 0;
    switch (gScene.area) {
    case AREA_DESERT_NIGHT_2:
    case AREA_DESERT_NIGHT_3:
    case AREA_SWAMP_NIGHT_8:
    case AREA_SWAMP_NIGHT_9:
    case AREA_SNOWY_MOUNTAINS_8:
    case AREA_SNOWY_MOUNTAINS_NIGHT_8:
    case AREA_JUNGLE_9:
    case AREA_SWAMP_8:
    case AREA_VOLCANO_7:
        obj->fvar = 1.0f;
        break;
    case AREA_FOREST_HILLS_10:
    case AREA_FOREST_HILLS_NIGHT_10:
        obj->fvar = 0.1f;
        break;
    case AREA_FOREST_HILLS_8:
    case AREA_FOREST_HILLS_NIGHT_8:
        obj->fvar = 0.3f;
        break;
    case AREA_DESERT_NIGHT_6:
        if (pl->pos[0] > 3000.0f && pl->pos[0] < 15000.0f && pl->pos[2] > 3000.0f && pl->pos[2] < 8000.0f) {
            obj->fvar = 0.0f;
        } else {
            obj->fvar = 1.0f;
        }
        break;
    case AREA_DESERT_NIGHT_9:
        if (pl->pos[0] > 0.0f && pl->pos[0] < 6200.0f && pl->pos[2] > 0.0f && pl->pos[2] < 9300.0f) {
            obj->fvar = 0.0f;
        } else {
            obj->fvar = 1.0f;
        }
        break;
    case AREA_GREAT_ARENA_NIGHT_1:
        if (pl->pos[2] > 100.0f && pl->pos[2] < 10400.0f) {
            obj->fvar = 1.0f;
        } else {
            obj->fvar = 0.0f;
        }
        break;
    case AREA_SWAMP_NIGHT_10:
    case AREA_DESERT_NIGHT_1:
    case AREA_DESERT_NIGHT_5:
    case AREA_DESERT_1:
    case AREA_DESERT_5:
        if (gScene.eventRunning == 1) {
            obj->var1 = 2;
            obj->fvar = 1.0f;
        } else {
            switch (Area_BoundaryZone(pl)) {
            default:
            case 0:
                obj->var1 = 2;
                obj->fvar = 1.0f;
                break;
            case 2:
                obj->var1 = 3;
                obj->fvar = 0.0f;
                break;
            case 1:
                obj->var1 = 0;
                obj->fvar = 1.0f;
                break;
            }
            if (gScene.area == AREA_DESERT_5 && pl->pos[2] < 2200.0f) {
                obj->fvar = 0.0f;
            }
        }
        break;
    case AREA_ARENAS_0:
        if (pl->pos[0] < 8832.0f || pl->pos[2] > 10675.0f) {
            obj->fvar = 0.0f;
        } else {
            obj->fvar = 1.0f;
        }
        break;
    case AREA_GREAT_ARENA_1:
        if (pl->pos[2] > 11000.0f) {
            obj->fvar = 0.0f;
        } else {
            obj->fvar = 1.0f;
        }
        break;
    case AREA_ARENA_1:
        if (pl->pos[0] < 8437.0f || pl->pos[2] > 11400.0f) {
            obj->fvar = 0.0f;
        } else {
            obj->fvar = 1.0f;
        }
        break;
    case AREA_DESERT_NIGHT_8:
    case AREA_DESERT_8:
        if (pl->pos[0] > 6916.0f || pl->pos[2] < 5555.0f) {
            obj->fvar = 0.0f;
        } else {
            obj->fvar = 1.0f;
        }
        break;
    default:
        obj->fvar = 0.4f;
        break;
    }
    count = gAreaAtmosLayerCounts[obj->type];
    switch (obj->type) {
    case 8:
    case 14:
        atmosLayer = obj->buf;
        n = Area_MonsterState(gScene.area);
        for (i = 0; i < count; i++) {
            atmosLayer->scroll[0] = 0.0f;
            atmosLayer->scroll[1] = 0.0f;
            atmosLayer->scroll[2] = 0.0f;
            atmosLayer->speed = AreaAtmos_LayerSpeed(i, n);
            atmosLayer->alpha = AreaAtmos_LayerAlpha(i, n);
            atmosLayer++;
        }
        break;
    case 10:
    case 11:
    case 13:
        atmosScroll = obj->buf;
        layerUv = atmosScroll->layer;
        Mtx_GetTrans(atmosScroll->camPos, gCamMatrix);
        n = Area_MonsterState(gScene.area);
        for (i = 0; i < count; i++) {
            layerUv->u = 0.0f;
            layerUv->v = 0.0f;
            layerUv->alpha = AreaAtmos_LayerAlpha(i, n);
            layerUv++;
        }
        break;
    case 12:
        obj->fvar = 0.0f;
        break;
    default:
        obj->var3 = Rand_Next(1) & 0x7FF;
        break;
    }
    obj->draw = NULL;
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->ds = DrawSlot_Get(obj->slot);
        obj->ds->owner = obj;
        obj->ds->draw = AreaAtmos_Draw;
        AreaAtmos_Update(obj);
    } else {
        Obj_Free(obj);
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
void AtmosScroll_Update(AtmosScroll *scroll, s16 n, s16 *speed, u16 mode);
extern f32 gCamPos[3];
extern s16 gAtmosScrollSpeeds[];
extern s16 *gAtmosScrollSpeedsByState[];

/*
 * Updates an area atmosphere object (state 1): its alpha (fvar) and timers by its
 * type, the area and the local player's position, then places its draw slot
 * (around the camera, or toward the area's sky point gAreaState.skyPos) and adds it
 * to the stage list. By type:
 * - 0: faces the sky point, fading in while the line to it is clear and out while
 *   it is blocked (Area_LineBlocked, and the areas' own zones);
 * - 1: in front of the camera, fading by the camera's distance and angle to a point
 *   of AREA_FOREST_HILLS_10;
 * - 2, 3, 6: in front of the camera, scrolling with area periods (3: two layers);
 * - 4: alpha by the camera's angle to the sky point, dimmed in a cylinder of the area;
 * - 5: at its own position, ending after var2 frames and fading in the last 4;
 * - 7: by the player's boundary zone (Area_BoundaryZone): facing the sky point, or
 *   falling past the camera;
 * - 8 and 14: layers scrolling in v, their speeds and alphas eased to the area's;
 * - 9, 15: at the camera;
 * - 10, 11, 13: layers whose textures follow the camera's turns (AtmosScroll_Update);
 * - 12: shown for 256 frames after a random wait of 1200 to 2223 frames.
 * Type 1 reads its distances and limits uninitialised outside AREA_FOREST_HILLS_10 and
 * AREA_FOREST_HILLS_NIGHT_10, as the game's code does. The zone switches keep
 * `case 0:` beside `default:` for the match, as in AreaAtmos_Setup.
 */
void AreaAtmos_Update(Obj *obj)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    s16 count;
    s16 lim;
    f32 skyPt[3];
    f32 drawPos[3];
    f32 dir[3];
    u8 blocked;
    f32 a, b, r, rr, mx, mn, dx, dz, d2, d, c, f, tg;
    AtmosLayer *atmosLayer;
    s16 i;
    s16 n;
    s16 k;
    AtmosScroll *atmosScroll;
    AtmosUv *layerUv;
    s16 *speeds;
    u16 ang;
    u16 absAngle;

    obj->timer++;
    if (obj->type != 5) {
        skyPt[0] = gAreaState.skyPos[0];
        skyPt[1] = gAreaState.skyPos[1];
        skyPt[2] = gAreaState.skyPos[2];
    }
    count = gAreaAtmosLayerCounts[obj->type];
    switch (obj->type) {
    case 0:
        dir[0] = skyPt[0] - gCamPos[0];
        dir[1] = skyPt[1] - gCamPos[1];
        dir[2] = skyPt[2] - gCamPos[2];
        blocked = Area_LineBlocked(skyPt);
        Vec_Normalize(dir);
        switch (gScene.area) {
        case AREA_DESERT_NIGHT_6:
            if (pl->pos[0] > 3000.0f && pl->pos[0] < 15000.0f && pl->pos[2] > 3000.0f && pl->pos[2] < 8000.0f) {
                blocked = 1;
            }
            break;
        case AREA_DESERT_NIGHT_9:
            if (pl->pos[0] > 0.0f && pl->pos[0] < 6200.0f && pl->pos[2] > 0.0f && pl->pos[2] < 9300.0f) {
                blocked = 1;
            }
            break;
        case AREA_GREAT_ARENA_NIGHT_1:
            if (pl->pos[2] <= 100.0f || pl->pos[2] >= 10400.0f) {
                blocked = 1;
            }
            break;
        case AREA_ARENAS_0:
            if (pl->pos[0] < 8832.0f || pl->pos[2] > 10675.0f) {
                blocked = 1;
            }
            break;
        case AREA_GREAT_ARENA_1:
            if (pl->pos[2] > 11000.0f) {
                blocked = 1;
            }
            break;
        case AREA_ARENA_1:
            if (pl->pos[0] < 8437.0f || pl->pos[2] > 11400.0f) {
                blocked = 1;
            }
            break;
        case AREA_DESERT_NIGHT_8:
        case AREA_DESERT_8:
            if (pl->pos[0] > 6916.0f || pl->pos[2] < 5555.0f) {
                blocked = 1;
            }
            break;
        }
        if (blocked) {
            if ((obj->fvar -= 0.07f) < 0.0f) {
                obj->fvar = 0.0f;
            }
        } else {
            if ((obj->fvar += 0.07f) > 1.0f) {
                obj->fvar = 1.0f;
            }
        }
        Vec_FromCamera(drawPos, dir, 100.0f);
        switch (gScene.area) {
        case AREA_DESERT_NIGHT_2:
        case AREA_DESERT_NIGHT_3:
            if (obj->timer >= 400) {
                obj->timer = 0;
            }
            /* fallthrough */
        case AREA_JUNGLE_9:
            if (obj->timer >= 200) {
                obj->timer = 0;
            }
            break;
        case AREA_DESERT_NIGHT_7:
        case AREA_DESERT_7:
            if (obj->timer >= 100) {
                obj->timer = 0;
            }
            break;
        case AREA_DESERT_NIGHT_10:
        case AREA_DESERT_10:
            if (obj->timer >= 100) {
                obj->timer = 0;
            }
            break;
        }
        break;
    case 1:
        Vec_Copy(dir, gCamDir);
        Vec_FromCamera(drawPos, dir, -50.0f);
        switch (gScene.area) {
        case AREA_FOREST_HILLS_10:
        case AREA_FOREST_HILLS_NIGHT_10:
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
        if (obj->timer >= lim) {
            obj->timer = 0;
        }
        /* dx, dz, rr, r, mx and mn are set only in AREA_FOREST_HILLS_10 and
         * AREA_FOREST_HILLS_NIGHT_10: elsewhere the game's code reads them unset. */
        d2 = dx * dx + dz * dz;
        if (d2 <= rr) {
            obj->var0 = 0;
        } else {
            d = Math_Sqrtf(d2);
            a = Math_Asin(r / d);
            c = (-dir[0] * dx + -dir[2] * dz) / (d * Math_Sqrtf(dir[0] * dir[0] + dir[2] * dir[2]));
            if (c <= -1.0f) {
                c = -1.0f;
            } else if (c >= 1.0f) {
                c = 1.0f;
            }
            b = Math_Acos(c);
            obj->var0 = 1;
        }
        if (obj->var0 == 0) {
            if ((obj->fvar += 0.02f) > mx) {
                obj->fvar = mx;
            }
        } else {
            f = mx + mn - mx * (b - a) / 3.1415927f;
            if (f > mx) {
                f = mx;
            }
            if (obj->fvar > f) {
                if ((obj->fvar -= 0.02f) < f) {
                    obj->fvar = f;
                }
            } else {
                obj->fvar = f;
            }
        }
        break;
    case 2:
        if (obj->timer >= 400) {
            obj->timer = 0;
        }
        Vec_Copy(dir, gCamDir);
        Vec_FromCamera(drawPos, dir, -30.0f);
        break;
    case 3:
        obj->var3++;
        switch (gScene.area) {
        case AREA_DESERT_NIGHT_8:
            lim = 1600;
            k = 700;
            break;
        default:
            lim = 800;
            k = 300;
            break;
        }
        if (obj->timer >= lim) {
            obj->timer = 0;
        }
        if (obj->var3 >= k) {
            obj->var3 = 0;
        }
        Vec_Copy(dir, gCamDir);
        Vec_FromCamera(drawPos, dir, -30.0f);
        break;
    case 4:
        drawPos[0] = 13960.0f;
        drawPos[1] = 41.0f;
        drawPos[2] = 8560.0f;
        if (Cyl_Contains(pl->pos, drawPos, 4000.0f, -1.0f, -1.0f)) {
            if ((obj->fvar -= 0.015f) < 0.125f) {
                obj->fvar = 0.125f;
            }
        } else {
            dir[0] = skyPt[0] - gCamPos[0];
            dir[1] = skyPt[1] - gCamPos[1];
            dir[2] = skyPt[2] - gCamPos[2];
            d = Vec_Length(dir);
            if (d < 0.001f) {
                ang = 0;
            } else {
                ang = (s32)(0.5f + 65536.0f * Math_Acos(-Vec_Dot(dir, gCamDir) / d) / 6.2831855f);
            }
            absAngle = Math_Absf((s16)ang);
            tg = 0.75f - 0.5f * (absAngle / 32768.0f);
            if (tg < obj->fvar) {
                if (tg > (obj->fvar -= 0.015f)) {
                    obj->fvar = tg;
                }
            } else {
                if (tg < (obj->fvar += 0.015f)) {
                    obj->fvar = tg;
                }
            }
        }
        Vec_Copy(dir, gCamDir);
        Vec_FromCamera(drawPos, dir, -30.0f);
        break;
    case 5:
        if ((s8)--obj->var2 <= 0) {
            obj->state++;
            return;
        }
        if (obj->var2 < 4) {
            obj->fvar = obj->var2 / 4.0f;
        }
        dir[0] = obj->pos[0] - gCamPos[0];
        dir[1] = obj->pos[1] - gCamPos[1];
        dir[2] = obj->pos[2] - gCamPos[2];
        Vec_Normalize(dir);
        Vec_FromCamera(drawPos, dir, 200.0f);
        break;
    case 6:
        Vec_Copy(dir, gCamDir);
        Vec_FromCamera(drawPos, dir, -50.0f);
        switch (gScene.area) {
        case AREA_FOREST_HILLS_8:
        case AREA_FOREST_HILLS_NIGHT_8:
            lim = 400;
            break;
        case AREA_SWAMP_NIGHT_8:
            lim = 200;
            break;
        case AREA_SWAMP_NIGHT_9:
            lim = 200;
            break;
        case AREA_SWAMP_8:
            lim = 200;
            break;
        default:
            lim = 0x400;
            break;
        }
        if (obj->timer >= lim) {
            obj->timer = 0;
        }
        break;
    case 7:
        f = 1.0f;
        obj->var3++;
        lim = 60;
        if (obj->timer >= lim) {
            obj->timer = 0;
        }
        switch (obj->var1) {
        case 0:
            if (gScene.eventRunning == 1 || Area_BoundaryZone(pl) != 1) {
                obj->var1 = 1;
            }
            break;
        case 3:
            if (gScene.eventRunning == 1) {
                obj->var1 = 2;
            } else {
                switch (Area_BoundaryZone(pl)) {
                default:
                case 0:
                    obj->var1 = 2;
                    break;
                case 2:
                    break;
                case 1:
                    obj->var1 = 0;
                    break;
                }
            }
            break;
        case 1:
            if (obj->fvar == 0.0f) {
                obj->fvar = 0.0f;
                obj->var1 = 3;
            }
            if (gScene.eventRunning == 1) {
                obj->var1 = 2;
            } else {
                switch (Area_BoundaryZone(pl)) {
                default:
                case 0:
                    obj->var1 = 2;
                    break;
                case 2:
                    break;
                case 1:
                    obj->var1 = 0;
                    break;
                }
            }
            break;
        case 2:
            if (gScene.eventRunning == 1) {
                obj->var1 = 2;
            } else if (Area_BoundaryZone(pl) != 0) {
                obj->var1 = 3;
            }
            break;
        }
        switch (obj->var1) {
        case 3:
            dir[0] = skyPt[0] - gCamPos[0];
            dir[1] = skyPt[1] - gCamPos[1];
            dir[2] = skyPt[2] - gCamPos[2];
            Vec_Normalize(dir);
            Vec_FromCamera(drawPos, dir, 100.0f);
            f = -0.08f;
            break;
        case 2:
            dir[0] = skyPt[0] - gCamPos[0];
            dir[1] = skyPt[1] - gCamPos[1];
            dir[2] = skyPt[2] - gCamPos[2];
            blocked = Area_LineBlocked(skyPt);
            if (gScene.area == AREA_DESERT_5 && pl->pos[2] < 2200.0f) {
                blocked = 1;
            }
            Vec_Normalize(dir);
            Vec_FromCamera(drawPos, dir, 100.0f);
            if (blocked) {
                f = -0.07f;
            } else {
                f = 0.07f;
            }
            break;
        case 0:
            f = 0.08f;
            Mtx_GetTrans(drawPos, gCamMatrix);
            break;
        case 1:
            f = -0.08f;
            Mtx_GetTrans(drawPos, gCamMatrix);
            break;
        }
        if (Math_Absf(f) < 1.0f) {
            tg = obj->fvar + f;
            obj->fvar = tg;
            if (tg < 0.0f) {
                obj->fvar = 0.0f;
            } else if (tg > 1.0f) {
                obj->fvar = 1.0f;
            }
        }
        break;
    case 8:
    case 14:
        atmosLayer = obj->buf;
        n = Area_MonsterState(gScene.area);
        for (i = 0; i < count; i++) {
            f = AreaAtmos_LayerSpeed(i, n);
            if (atmosLayer->speed < f) {
                if ((atmosLayer->speed += 0.001f) > f) {
                    atmosLayer->speed = f;
                }
            } else if (atmosLayer->speed > f) {
                if ((atmosLayer->speed -= 0.001f) < f) {
                    atmosLayer->speed = f;
                }
            }
            atmosLayer->scroll[1] += atmosLayer->speed;
            atmosLayer->scroll[1] -= Math_Floor(atmosLayer->scroll[1]);
            f = AreaAtmos_LayerAlpha(i, n);
            if (atmosLayer->alpha != f) {
                if (atmosLayer->alpha < f) {
                    if ((atmosLayer->alpha += 0.025f) > f) {
                        atmosLayer->alpha = f;
                    }
                } else {
                    if ((atmosLayer->alpha -= 0.025f) < f) {
                        atmosLayer->alpha = f;
                    }
                }
            }
            atmosLayer++;
        }
        Mtx_GetTrans(drawPos, gCamMatrix);
        break;
    case 9:
        Mtx_GetTrans(drawPos, gCamMatrix);
        break;
    case 10:
        atmosScroll = obj->buf;
        layerUv = atmosScroll->layer;
        n = Area_MonsterState(gScene.area);
        speeds = gAtmosScrollSpeeds;
        i = 0;
        do {
            f = AreaAtmos_LayerAlpha(i, n);
            if (layerUv->alpha != f) {
                if (layerUv->alpha < f) {
                    if ((layerUv->alpha += 0.025f) > f) {
                        layerUv->alpha = f;
                    }
                } else {
                    if ((layerUv->alpha -= 0.025f) < f) {
                        layerUv->alpha = f;
                    }
                }
            }
            i++;
            layerUv++;
        } while (i < 2);
        AtmosScroll_Update(atmosScroll, 2, speeds, 2);
        Mtx_GetTrans(drawPos, gCamMatrix);
        break;
    case 11:
        atmosScroll = obj->buf;
        switch (gScene.area) {
        case AREA_SWAMP_3:
            lim = 200;
            break;
        case AREA_SWAMP_4:
        default:
            lim = 200;
            break;
        }
        AtmosScroll_Update(atmosScroll, 2, &lim, 1);
        Vec_Copy(dir, gCamDir);
        Vec_FromCamera(drawPos, dir, -50.0f);
        break;
    case 12:
        switch (obj->var0) {
        case 0:
            if (++obj->timer >= obj->var3) {
                obj->timer = 0;
                obj->var0++;
            }
            return;
        case 1:
            if (++obj->timer >= 0x100) {
                obj->timer = 0;
                obj->var3 = (Rand_Next(1) & 0x3FF) + 1200;
                obj->var0 = 0;
                return;
            }
            obj->fvar = 1.0f;
            Vec_Copy(dir, gCamDir);
            Vec_FromCamera(drawPos, dir, -30.0f);
            break;
        }
        break;
    case 13:
        atmosScroll = obj->buf;
        layerUv = atmosScroll->layer;
        n = Area_MonsterState(gScene.area);
        speeds = gAtmosScrollSpeedsByState[n];
        i = 0;
        do {
            f = AreaAtmos_LayerAlpha(i, n);
            if (layerUv->alpha != f) {
                if (layerUv->alpha < f) {
                    if ((layerUv->alpha += 0.025f) > f) {
                        layerUv->alpha = f;
                    }
                } else {
                    if ((layerUv->alpha -= 0.025f) < f) {
                        layerUv->alpha = f;
                    }
                }
            }
            i++;
            layerUv++;
        } while (i < 3);
        AtmosScroll_Update(atmosScroll, 3, speeds, 2);
        Mtx_GetTrans(drawPos, gCamMatrix);
        break;
    case 15:
        Vec_Copy(drawPos, gCamPos);
        lim = 0x80;
        if (obj->timer >= lim) {
            obj->timer = 0;
        }
        break;
    }
    if (obj->ds != NULL) {
        obj->ds->pos[0] = drawPos[0];
        obj->ds->pos[1] = drawPos[1];
        obj->ds->pos[2] = drawPos[2];
        OT_AddByDepth(gDrawListStage, obj->ds, 1, 0);
    }
}

/* Hides the area atmosphere object; the next state frees it. */
void AreaAtmos_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Frees the area atmosphere object and its draw slot, if it has one. */
void AreaAtmos_Free(Obj *obj)
{
    if (obj->ds != NULL) {
        DrawSlot_Free(obj->slot);
    }
    Obj_Free(obj);
}

/* Narrows the camera's field of view to 50 degrees and sets the flag *fov. */
void CamFov_Narrow(s8 *fov)
{
    Camera_SetFov(0.87266463f);
    *fov = 1;
}

/* Restores the camera's field of view if *fov says it was narrowed, and clears the flag. */
void CamFov_Restore(u8 *fov)
{
    if (*fov != 0) {
        Camera_RestoreFov();
        *fov = 0;
    }
}

extern s16 gAreaAtmosLayerCounts[];
f32 Math_Sqrtf(f32 x);
f32 Math_Acos(f32 x);
void Mtx_Concat3x3(f32 *m, f32 *n);

/*
 * Draws an area atmosphere object (its draw slot's update): its model, entry
 * gAreaFxModels[area][1] of gAreaSubModels[0] and the entries after it for the
 * types with layers, at the draw slot's position. Most types are drawn with the
 * camera's field of view narrowed (CamFov_Narrow), turned to face the camera, with
 * a texture offset and an alpha from the object (see AreaAtmos_Update). Nothing is
 * drawn while the area fades to black (gAreaFade 1 or 2).
 */
void AreaAtmos_Draw(DrawSlot *slot)
{
    AtmosLayer *layer;
    Obj *obj = slot->owner;
    ModelMatSet *entry;
    s16 idx;
    s16 cnt;
    s16 i;
    s32 div;
    s32 div2nd;
    f32 uScroll;
    f32 dx, dz, cx, cz;
    f32 len;
    f32 c, a;
    f32 lx, lz;
    AtmosUv *layerUv;
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
    idx = gAreaFxModels[gScene.area][1];
    if (idx < 0) {
        return;
    }
    entry = &set->matSets[idx];
    if (obj->type != 5) {
        lx = gAreaState.skyPos[0];
        lz = gAreaState.skyPos[2];
    }
    Gs_SetState(RS_ALPHA_REF, 0);
    Gs_SetState(RS_ZWRITE, 0);
    Gs_SetState(RS_ZTEST, RS_CMP_ALWAYS);
    if ((u32)(obj->type - 7) <= 2 || obj->type == 14) {
        Mtx_Identity(mtx[0]);
    } else {
        Mtx_Copy(mtx[0], gCamMatrix);
    }
    Mtx_SetTrans(mtx[0], slot->pos[0], slot->pos[1], slot->pos[2]);
    cnt = gAreaAtmosLayerCounts[obj->type];
    switch (obj->type) {
    case 0:
        Mtx_Identity(mtx[0]);
        uScroll = 0.0f;
        switch (gScene.area) {
        case AREA_DESERT_NIGHT_2:
        case AREA_DESERT_NIGHT_3:
            uScroll = 0.0025f * obj->timer;
            break;
        case AREA_FOREST_HILLS_5:
        case AREA_FOREST_HILLS_NIGHT_5:
            uScroll = 0.0078125f * (obj->timer & 0x7F);
            break;
        case AREA_JUNGLE_9:
            uScroll = 0.005f * obj->timer;
            break;
        case AREA_DESERT_NIGHT_7:
        case AREA_DESERT_7:
            uScroll = 0.01f * obj->timer;
            break;
        case AREA_DESERT_NIGHT_10:
        case AREA_DESERT_10:
            uScroll = 0.01f * obj->timer;
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
        Mtx_Translation(tm[0], uScroll, 0.0f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32)tm);
        Mtx_Concat3x3(mtx[0], gCamMatrix);
        Mtx_SetTrans(mtx[0], slot->pos[0], slot->pos[1], slot->pos[2]);
        {
            u8 grey = 255.0f * obj->fvar;
            Gs_SetState(RS_COLOR2, (grey << 16) | 0xFF000000 | (grey << 8) | grey);
        }
        break;
    case 1:
        CamFov_Narrow(&fov);
        switch (gScene.area) {
        case AREA_DESERT_NIGHT_11:
        case AREA_FOREST_HILLS_10:
        case AREA_FOREST_HILLS_NIGHT_10:
            div = 500;
            break;
        default:
            div = 0x400;
            break;
        }
        Mtx_Translation(tm[0], (1.0f / div) * (obj->timer % (s16)div), 0.0f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32)tm);
        Gs_SetState(RS_COLOR2, ((u8)(255.0f * obj->fvar) << 24) | 0xFFFFFF);
        break;
    case 2:
        CamFov_Narrow(&fov);
        Mtx_Translation(tm[0], 0.0025f * obj->timer, 0.0f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32)tm);
        Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
        if (entry != NULL && entry->id != -1) {
            Draw_ApplyStates(entry->flags);
            Model_Draw(entry->id, 0);
        }
        break;
    case 3:
        CamFov_Narrow(&fov);
        switch (gScene.area) {
        case AREA_DESERT_NIGHT_8:
            div = 1600;
            div2nd = 700;
            break;
        default:
            div = 800;
            div2nd = 300;
            break;
        }
        Mtx_Translation(tm[0], (1.0f / div) * obj->timer, 0.0f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32)tm);
        Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
        Gs_SetState(RS_ZTEST, RS_CMP_ALWAYS);
        if (entry != NULL && entry->id != -1) {
            Draw_ApplyStates(entry->flags);
            Model_Draw(entry->id, 0);
        }
        entry = &set->matSets[idx] + 1;
        Mtx_Translation(tm[0], (1.0f / div2nd) * obj->var3, 0.0f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32)tm);
        break;
    case 4:
        CamFov_Narrow(&fov);
        Mtx_Translation(tm[0], 0.0009765625f * (obj->timer & 0x3FF), 0.0f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32)tm);
        Gs_SetState(RS_COLOR2, ((u8)(255.0f * obj->fvar) << 24) | 0xFFFFFF);
        break;
    case 5:
        CamFov_Narrow(&fov);
        {
            u8 grey = 255.0f * obj->fvar;
            Gs_SetState(RS_COLOR2, (grey << 16) | 0xFF000000 | (grey << 8) | grey);
        }
        break;
    case 6:
        CamFov_Narrow(&fov);
        switch (gScene.area) {
        case AREA_FOREST_HILLS_8:
        case AREA_FOREST_HILLS_NIGHT_8:
            div = 400;
            break;
        case AREA_SWAMP_NIGHT_8:
            div = 200;
            break;
        case AREA_SWAMP_NIGHT_9:
            div = 200;
            break;
        case AREA_SWAMP_8:
            div = 200;
            break;
        default:
            div = 0x400;
            break;
        }
        Mtx_Translation(tm[0], (1.0f / div) * (obj->timer % (s16)div), 0.0f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32)tm);
        Gs_SetState(RS_COLOR2, ((u8)(255.0f * obj->fvar) << 24) | 0xFFFFFF);
        break;
    case 7:
        switch (obj->var1) {
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
                u8 grey = 255.0f * obj->fvar;
                Gs_SetState(RS_COLOR2, (grey << 16) | 0xFF000000 | (grey << 8) | grey);
            }
            break;
        case 0:
        case 1:
            CamFov_Narrow(&fov);
            entry = &set->matSets[idx] + 1;
            Gs_SetState(RS_COLOR2, ((u8)(128.0f * obj->fvar) << 24) | 0xFFFFFF);
            Mtx_Translation(tm[0], 0.0f, 1.0f - 0.016666668f * obj->timer, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32)tm);
            break;
        }
        break;
    case 8:
    case 14:
        layer = obj->buf;
        Mtx_Identity(m2[0]);
        for (i = 0; i < cnt; i++, layer++) {
            if (i == 2) {
                Gs_SetState(RS_MTX_SLOT(0), (s32)m2);
            } else {
                CamFov_Narrow(&fov);
                Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            }
            entry = &set->matSets[idx] + i;
            if (i == 0) {
                Mtx_Translation(tm[0], 0.0f, 0.0f, 0.0f);
            } else {
                Mtx_Translation(tm[0], 0.0f, layer->scroll[1], 0.0f);
            }
            Gs_SetState(RS_UV_MTX, (s32)tm);
            Gs_SetState(RS_COLOR2, ((u8)(255.0f * layer->alpha) << 24) | 0xFFFFFF);
            if (entry != NULL && entry->id != -1) {
                Draw_ApplyStates(entry->flags);
                Model_Draw(entry->id, 0);
            }
            CamFov_Restore((u8 *)&fov);
        }
        goto done;
    case 9:
        Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
        entry = &set->matSets[idx];
        break;
    case 10:
    case 13:
        CamFov_Narrow(&fov);
        layerUv = ((AtmosScroll *)obj->buf)->layer;
        Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
        for (i = 0; i < cnt; i++, layerUv++) {
            entry = &set->matSets[idx] + i;
            Mtx_Translation(tm[0], layerUv->u, layerUv->v, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32)tm);
            Gs_SetState(RS_COLOR2, ((u8)(255.0f * layerUv->alpha) << 24) | 0xFFFFFF);
            if (entry != NULL && entry->id != -1) {
                Draw_ApplyStates(entry->flags);
                Model_Draw(entry->id, 0);
            }
        }
        goto done;
    case 11:
        CamFov_Narrow(&fov);
        Mtx_Translation(tm[0], ((AtmosScroll *)obj->buf)->layer[0].u, 0.0f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32)tm);
        break;
    case 12:
        CamFov_Narrow(&fov);
        Mtx_Translation(tm[0], 0.0f, 1.0f - obj->timer / 256.0f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32)tm);
        Gs_SetState(RS_COLOR2, ((u8)(255.0f * obj->fvar) << 24) | 0xFFFFFF);
        Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
        if (entry != NULL && entry->id != -1) {
            Draw_ApplyStates(entry->flags);
            Model_Draw(entry->id, 0);
        }
        if (obj->timer > 0x80) {
            goto done;
        }
        Mtx_Translation(tm[0], 0.0f, 1.0f - obj->timer / 128.0f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32)tm);
        break;
    case 15:
        Mtx_Identity(mtx[0]);
        Mtx_Translation(tm[0], 0.0f, obj->timer / 128.0f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32)tm);
        Gs_SetState(RS_COLOR2, -1);
        break;
    }
    Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
    if (entry != NULL && entry->id != -1) {
        Draw_ApplyStates(entry->flags);
        Model_Draw(entry->id, 0);
    }
done:
    Draw_SetDefaultState();
    Gs_SetState(RS_ZWRITE, 1);
    Gs_SetState(RS_ZTEST, RS_CMP_LEQUAL);
    CamFov_Restore((u8 *)&fov);
}

/* The file's data. */
extern u32 gDefaultStageHeader_3[];
extern u32 D_006AE9A0[];
extern u32 D_006AE9A8[];
extern u32 D_006AE9B0[];
extern char *gDefaultStageHeader_12[];
extern AreaAnim gAreaAnimTbls_128[];
s16 gAreaFxModels_0[16] = {
#include "sub_main/00441150.inc"
};
/* gm_sub (loaded with game): its 119 areas' rows of gAreaFxModels (model.h), 16 set
 * indices each (config/ld/gm_sub_aliases.ld). */
extern s16 D_0092AD60[119][16];
s16 *gAreaFxModels[200] = {
    gAreaFxModels_0, D_0092AD60[0], D_0092AD60[1], D_0092AD60[2],
    D_0092AD60[3], D_0092AD60[4], D_0092AD60[5], D_0092AD60[6],
    gAreaFxModels_0, D_0092AD60[7], gAreaFxModels_0, D_0092AD60[8],
    D_0092AD60[9], gAreaFxModels_0, D_0092AD60[10], D_0092AD60[11],
    D_0092AD60[99], gAreaFxModels_0, D_0092AD60[12], D_0092AD60[13],
    gAreaFxModels_0, D_0092AD60[14], D_0092AD60[78], D_0092AD60[79],
    D_0092AD60[15], D_0092AD60[16], D_0092AD60[80], D_0092AD60[58],
    D_0092AD60[17], D_0092AD60[18], D_0092AD60[19], D_0092AD60[20],
    D_0092AD60[21], D_0092AD60[22], D_0092AD60[23], D_0092AD60[24],
    D_0092AD60[25], D_0092AD60[26], D_0092AD60[27], D_0092AD60[28],
    D_0092AD60[29], D_0092AD60[30], D_0092AD60[31], D_0092AD60[32],
    D_0092AD60[33], D_0092AD60[34], D_0092AD60[35], D_0092AD60[36],
    D_0092AD60[37], D_0092AD60[38], D_0092AD60[39], D_0092AD60[40],
    D_0092AD60[41], D_0092AD60[95], D_0092AD60[42], D_0092AD60[43],
    D_0092AD60[98], D_0092AD60[44], gAreaFxModels_0, D_0092AD60[45],
    D_0092AD60[46], D_0092AD60[47], D_0092AD60[48], D_0092AD60[49],
    D_0092AD60[50], D_0092AD60[51], D_0092AD60[52], D_0092AD60[53],
    D_0092AD60[102], D_0092AD60[103], D_0092AD60[54], D_0092AD60[55],
    D_0092AD60[56], D_0092AD60[57], D_0092AD60[58], D_0092AD60[59],
    gAreaFxModels_0, D_0092AD60[8], D_0092AD60[60], D_0092AD60[10],
    D_0092AD60[61], D_0092AD60[19], D_0092AD60[20], gAreaFxModels_0,
    gAreaFxModels_0, gAreaFxModels_0, gAreaFxModels_0, gAreaFxModels_0,
    gAreaFxModels_0, D_0092AD60[62], D_0092AD60[63], D_0092AD60[64],
    gAreaFxModels_0, gAreaFxModels_0, D_0092AD60[65], D_0092AD60[66],
    D_0092AD60[67], D_0092AD60[68], gAreaFxModels_0, D_0092AD60[69],
    D_0092AD60[70], gAreaFxModels_0, gAreaFxModels_0, D_0092AD60[65],
    D_0092AD60[66], D_0092AD60[67], D_0092AD60[68], gAreaFxModels_0,
    D_0092AD60[69], D_0092AD60[70], D_0092AD60[71], D_0092AD60[72],
    D_0092AD60[73], D_0092AD60[74], D_0092AD60[75], D_0092AD60[76],
    D_0092AD60[77], D_0092AD60[78], D_0092AD60[79], D_0092AD60[80],
    D_0092AD60[81], gAreaFxModels_0, D_0092AD60[82], D_0092AD60[83],
    D_0092AD60[84], gAreaFxModels_0, gAreaFxModels_0, gAreaFxModels_0,
    D_0092AD60[85], D_0092AD60[86], D_0092AD60[87], gAreaFxModels_0,
    gAreaFxModels_0, gAreaFxModels_0, gAreaFxModels_0, gAreaFxModels_0,
    gAreaFxModels_0, gAreaFxModels_0, gAreaFxModels_0, gAreaFxModels_0,
    D_0092AD60[88], D_0092AD60[89], D_0092AD60[90], D_0092AD60[91],
    D_0092AD60[92], D_0092AD60[93], D_0092AD60[94], D_0092AD60[95],
    D_0092AD60[96], D_0092AD60[97], D_0092AD60[98], D_0092AD60[99],
    D_0092AD60[100], D_0092AD60[101], D_0092AD60[102], D_0092AD60[103],
    D_0092AD60[104], D_0092AD60[105], D_0092AD60[106], D_0092AD60[107],
    D_0092AD60[108], D_0092AD60[109], D_0092AD60[110], D_0092AD60[111],
    D_0092AD60[112], D_0092AD60[113], D_0092AD60[114], D_0092AD60[115],
    D_0092AD60[116], D_0092AD60[117], gAreaFxModels_0, gAreaFxModels_0,
    gAreaFxModels_0, gAreaFxModels_0, gAreaFxModels_0, gAreaFxModels_0,
    gAreaFxModels_0, gAreaFxModels_0, gAreaFxModels_0, gAreaFxModels_0,
    gAreaFxModels_0, gAreaFxModels_0, gAreaFxModels_0, gAreaFxModels_0,
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
AreaPropChoices gVillagePropChoices[2] = {
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
char *gMsgEventTexts[8] = {
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
char *gSharpnessFullText[2] = {
    D_004971B0, NULL,
};
char *gLoadingCaptions[2] = {
    D_004971D0, D_004971F0,
};
char *gLoadingCaptionsAlt[4] = {
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
char gAmbSePosHub84[0x10] = {
#include "sub_main/00448420.inc"
};
u32 gAmbSeIdsHub84[2] = {
#include "sub_main/00448430.inc"
};
u32 gAmbSePeriodsHub84[2] = {
#include "sub_main/00448438.inc"
};
u32 gAmbSeKindsHub84[2] = {
#include "sub_main/00448440.inc"
};
u32 gAmbSeSlotsHub84[2] = {
#include "sub_main/00448448.inc"
};
u32 gAmbSePosElderHall[6] = {
#include "sub_main/00448450.inc"
};
char gAmbSeIdsElderHall[0x8] = {
#include "sub_main/00448468.inc"
};
char gAmbSePeriodsElderHall[0x8] = {
#include "sub_main/00448470.inc"
};
u32 gAmbSeKindsElderHall[2] = {
#include "sub_main/00448478.inc"
};
u32 gAmbSeSlotsElderHall[4] = {
#include "sub_main/00448480.inc"
};
u32 gAmbSePosHub89[12] = {
#include "sub_main/00448490.inc"
};
u32 gAmbSeIdsHub89[4] = {
#include "sub_main/004484C0.inc"
};
u32 gAmbSePeriodsHub89[2] = {
#include "sub_main/004484D0.inc"
};
u32 gAmbSeKindsHub89[2] = {
#include "sub_main/004484D8.inc"
};
u32 gAmbSeSlotsHub89[2] = {
#include "sub_main/004484E0.inc"
};
u32 gAmbSeFlagsHub89[2] = {
#include "sub_main/004484E8.inc"
};
u32 gHub85EffectPos[48] = {
#include "sub_main/004484F0.inc"
};
FogParam gFogDefault = {
#include "sub_main/004485B0.inc"
};
FogParam gFogHouse = {
#include "sub_main/004485C0.inc"
};
u32 gAreaInfo[200] = {
#include "sub_main/004485D0.inc"
};
u8 gSkyDayTable[96] = {
#include "sub_main/004488F0.inc"
};
u8 gSkyDayTableHubs[96] = {
#include "sub_main/00448950.inc"
};
u8 gSkyDayTableHouses[96] = {
#include "sub_main/004489B0.inc"
};
f32 gGlowPosHouse[12][3] = {
#include "sub_main/00448A10.inc"
};
s16 gGlowEntryHouse[16] = {
#include "sub_main/00448AA0.inc"
};
f32 gGlowPosHub83[12][3] = {
#include "sub_main/00448AC0.inc"
};
s16 gGlowEntryHub83[16] = {
#include "sub_main/00448B50.inc"
};
f32 gGlowPosHub84[14][3] = {
#include "sub_main/00448B70.inc"
};
s16 gGlowEntryHub84[16] = {
#include "sub_main/00448C20.inc"
};
f32 gGlowPosHub85[10][3] = {
#include "sub_main/00448C40.inc"
};
s16 gGlowEntryHub85[16] = {
#include "sub_main/00448CC0.inc"
};
s16 gGlowEntryTavern[16] = {
#include "sub_main/00448CE0.inc"
};
f32 gGlowPosTavern[9][3] = {
#include "sub_main/00448D00.inc"
};
f32 gGlowPosTownArena[8][3] = {
#include "sub_main/00448D70.inc"
};
f32 gGlowScaleTownArena[8] = {
#include "sub_main/00448DD0.inc"
};
f32 gGlowPosElderHall[16][3] = {
#include "sub_main/00448DF0.inc"
};
f32 gGlowScaleElderHall[16] = {
#include "sub_main/00448EB0.inc"
};
s16 gGlowEntryElderHall[16] = {
#include "sub_main/00448EF0.inc"
};
f32 gGlowPosHub89[12][3] = {
#include "sub_main/00448F10.inc"
};
f32 gGlowScaleHub89[12] = {
#include "sub_main/00448FA0.inc"
};
s16 gGlowEntryHub89[12] = {
#include "sub_main/00448FD0.inc"
};
s16 gGlowVillageEntries[4] = {
#include "sub_main/00448FE8.inc"
};
s16 gGlowEntryHubAA[8] = {
#include "sub_main/00448FF0.inc"
};
f32 gGlowPosHubAA[4][3] = {
#include "sub_main/00449000.inc"
};
f32 gGlowRotYHubAA[4] = {
#include "sub_main/00449030.inc"
};
f32 gGlowPosOnlineHouse1[12][3] = {
#include "sub_main/00449040.inc"
};
s16 gGlowEntryOnlineHouse1[16] = {
#include "sub_main/004490D0.inc"
};
f32 gGlowPosOnlineHouse2[20][3] = {
#include "sub_main/004490F0.inc"
};
s16 gGlowEntryOnlineHouse2[24] = {
#include "sub_main/004491E0.inc"
};
f32 gAreaPropPosHouse[8] = {
#include "sub_main/00449210.inc"
};
f32 gAreaPropRotHouse[8] = {
#include "sub_main/00449230.inc"
};
f32 gAreaPropPosHub83[8][3] = {
#include "sub_main/00449250.inc"
};
f32 gAreaPropScaleHub83[8] = {
#include "sub_main/004492B0.inc"
};
s16 gAreaPropEntryHub83[8] = {
#include "sub_main/004492D0.inc"
};
f32 gAreaPropPosHub84[6][3] = {
#include "sub_main/004492E0.inc"
};
f32 gAreaPropScaleHub84[12] = {
#include "sub_main/00449330.inc"
};
f32 gAreaPropPosHub85[2][3] = {
#include "sub_main/00449360.inc"
};
s16 gAreaPropEntryHub85[4] = {
#include "sub_main/00449378.inc"
};
f32 gAreaPropPosElderHall[2][3] = {
#include "sub_main/00449380.inc"
};
f32 gAreaPropScaleElderHall[2] = {
#include "sub_main/00449398.inc"
};
s16 gAreaPropEntryElderHall[4] = {
#include "sub_main/004493A0.inc"
};
f32 gAreaPropPosHubAA[3] = {
#include "sub_main/004493A8.inc"
};
f32 gAreaPropPosOnlineHouse1[3] = {
#include "sub_main/004493B8.inc"
};
f32 gAreaPropPosOnlineHouse2[3] = {
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
