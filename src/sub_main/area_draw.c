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
 *   objects (OBJ_KIND_SOUND, SoundObj_*: a positioned sound, replayed or following a
 *   river) and the area atmosphere objects (OBJ_KIND_AREA_ATMOS, AreaAtmos_*: scrolled
 *   overlays drawn around the camera).
 * - At the end of the code, the atmosphere layers' helpers: the camera's line to the sky
 *   (Vec_FromCamera, and Area_LineBlocked against the spheres of gAreaBlockSpheres;
 *   Area_BoundaryZone for the two desert areas whose sky changes with the player's place),
 *   and the layers' scroll (AtmosScroll_Update), alpha and speed (AreaAtmos_LayerAlpha,
 *   AreaAtmos_LayerSpeed). Their tables end the file's data.
 *
 * One file: the data interleaves through it (the tables of the notices, the props, the sound
 * objects and the atmosphere objects lie among one another and among the tables other files
 * read), so no cut inside it keeps each side's data together.
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
#include "light.h"
#include "sound.h"
#include "text.h"
#include "camera.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "font.h"
#include "gs_setup.h"
#include "lobby_state.h"
#include "lobby_world.h"
#include "main.h"
#include "rand.h"
#include "vec_math.h"
#include "gm_em.h"

/* gm_sub's default stage data, which gDefaultStageHeader points to (its
 * fields hold a stage file's offsets, relocated on load: they stay u32) */
extern QuestInfo gDefaultQuestInfo[1];
extern StartArea gDefaultStartAreas[4];
extern ItemStack gDefaultSupply[4];
extern DefaultStageEntry *gDefaultEmSpawns[4];
extern DefaultStageEntry gDefaultStageEntry[2];
extern u32 gDefaultStageInfo[2];

/* The game's code of this file was built at optimization level 4; the colour helpers and
 * Sky_DrawGradient at level 3. */
#pragma optimization_level 4

/* prototypes */
extern u8 gSkyDayTable[];
extern u8 gSkyDayTableHubs[];
extern u8 gSkyDayTableHouses[];
extern f32 gCamDir[3];    /* the camera direction */
/* end of prototypes */

u8 Village_FacilityLevel(s32 k);

/* The file's read-only data. */
#pragma rodata on
char gDefaultStageHeader_12_0[16] = {
#include "sub_main/00490B70.inc"
};
char gDefaultStageHeader_12_1[32] = {
#include "sub_main/00490B80.inc"
};
char gDefaultStageHeader_12_2[16] = {
#include "sub_main/00490BA0.inc"
};
char gDefaultStageHeader_12_3[16] = {
#include "sub_main/00490BB0.inc"
};
char gDefaultStageHeader_12_4[16] = {
#include "sub_main/00490BC0.inc"
};
#pragma explicit_zero_data on
char gEmClassNames_0[16] = {
#include "sub_main/00490BD0.inc"
};
#pragma explicit_zero_data reset
char gReturn20sTownText_0[32] = {
#include "sub_main/00490BE0.inc"
};
char gReturn20sTownText_1[32] = {
#include "sub_main/00490C00.inc"
};
char gReturn1mTownText_0[32] = {
#include "sub_main/00490C20.inc"
};
char gReturn1mTownText_1[32] = {
#include "sub_main/00490C40.inc"
};
char gQuestClearText_0[32] = {
#include "sub_main/00490C60.inc"
};
char gQuestClearText_1[32] = {
#include "sub_main/00490C80.inc"
};
char gTimeLeftMinFmt_0[24] = {
#include "sub_main/00490CA0.inc"
};
char gTimeLeftMinFmt_1[15] = {
#include "sub_main/00490CB8.inc"
};
char gQuestNoticeTexts_0[32] = {
#include "sub_main/00490CD0.inc"
};
char gQuestNoticeTexts_1[24] = {
#include "sub_main/00490CF0.inc"
};
char gQuestNoticeTexts_2[13] = {
#include "sub_main/00490D08.inc"
};
char gQuestNoticeTexts_3[32] = {
#include "sub_main/00490D20.inc"
};
char gQuestNoticeTexts_4[32] = {
#include "sub_main/00490D40.inc"
};
char gQuestNoticeTexts_5[32] = {
#include "sub_main/00490D60.inc"
};
char gQuestNoticeTexts_6[32] = {
#include "sub_main/00490D80.inc"
};
char gQuestNoticeTexts_7[32] = {
#include "sub_main/00490DA0.inc"
};
char gSubBSupplyText_0[32] = {
#include "sub_main/00490DC0.inc"
};
char gSubBSupplyText_1[32] = {
#include "sub_main/00490DE0.inc"
};
char gSubBSupplyText_2[32] = {
#include "sub_main/00490E00.inc"
};
char gSubBSupplyText_3[32] = {
#include "sub_main/00490E20.inc"
};
char gMustOrderAllText_0[48] = {
#include "sub_main/00490E40.inc"
};
char gMustOrderAllText_1[48] = {
#include "sub_main/00490E70.inc"
};
char gReturn10sTownText_0[32] = {
#include "sub_main/00490EA0.inc"
};
char gReturn10sTownText_1[32] = {
#include "sub_main/00490EC0.inc"
};
char D_00490EE0[16] = {
#include "sub_main/00490EE0.inc"
};
char gDrillScoreTexts_1[48] = {
#include "sub_main/00490EF0.inc"
};
char gDrillScoreTexts_2[32] = {
#include "sub_main/00490F20.inc"
};
char gDrillScoreTexts_3[32] = {
#include "sub_main/00490F40.inc"
};
char gDrillScoreTexts_11[48] = {
#include "sub_main/00490F60.inc"
};
char gDrillScoreTexts_13[32] = {
#include "sub_main/00490F90.inc"
};
char gDrillScoreTexts_14[32] = {
#include "sub_main/00490FB0.inc"
};
char gDrillScoreTexts_15[32] = {
#include "sub_main/00490FD0.inc"
};
char gElderFledText_0[32] = {
#include "sub_main/00490FF0.inc"
};
char gEmClassNames_1[8] = {
#include "sub_main/00491010.inc"
};
char gEmClassNames_2[8] = {
#include "sub_main/00491018.inc"
};
char gEmClassNames_3[8] = {
#include "sub_main/00491020.inc"
};
char gEmClassNames_4[8] = {
#include "sub_main/00491028.inc"
};
char gEmClassNames_5[8] = {
#include "sub_main/00491030.inc"
};
char gEmClassNames_6[8] = {
#include "sub_main/00491038.inc"
};
char gEmClassNames_7[8] = {
#include "sub_main/00491040.inc"
};
char gEmClassNames_8[8] = {
#include "sub_main/00491048.inc"
};
char gEmClassNames_9[8] = {
#include "sub_main/00491050.inc"
};
char gMonsterNames_0[15] = {
#include "sub_main/00491058.inc"
};
char gMonsterNames_1[12] = {
#include "sub_main/00491068.inc"
};
char D_00491078[13] = {
#include "sub_main/00491078.inc"
};
char gMonsterNames_3[8] = {
#include "sub_main/00491088.inc"
};
char gMonsterNames_4[8] = {
#include "sub_main/00491090.inc"
};
char gMonsterNames_5[13] = {
#include "sub_main/00491098.inc"
};
char gMonsterNames_6[13] = {
#include "sub_main/004910A8.inc"
};
char D_004910B8[15] = {
#include "sub_main/004910B8.inc"
};
char gMonsterNames_8[13] = {
#include "sub_main/004910C8.inc"
};
char gMonsterNames_9[12] = {
#include "sub_main/004910D8.inc"
};
char gMonsterNames_10[8] = {
#include "sub_main/004910E8.inc"
};
char gMonsterNames_11[16] = {
#include "sub_main/004910F0.inc"
};
char gMonsterNames_12[16] = {
#include "sub_main/00491100.inc"
};
char gMonsterNames_13[16] = {
#include "sub_main/00491110.inc"
};
char gMonsterNames_14[16] = {
#include "sub_main/00491120.inc"
};
char gMonsterNames_15[16] = {
#include "sub_main/00491130.inc"
};
char D_00491140[16] = {
#include "sub_main/00491140.inc"
};
char gMonsterNames_17[16] = {
#include "sub_main/00491150.inc"
};
char gMonsterNames_18[16] = {
#include "sub_main/00491160.inc"
};
char gMonsterNames_19[16] = {
#include "sub_main/00491170.inc"
};
char gMonsterNames_20[16] = {
#include "sub_main/00491180.inc"
};
char gMonsterNames_21[16] = {
#include "sub_main/00491190.inc"
};
char gMonsterNames_22[16] = {
#include "sub_main/004911A0.inc"
};
char gMonsterNames_23[16] = {
#include "sub_main/004911B0.inc"
};
char gMonsterNames_24[16] = {
#include "sub_main/004911C0.inc"
};
char gMonsterNames_25[16] = {
#include "sub_main/004911D0.inc"
};
char gMonsterNames_26[16] = {
#include "sub_main/004911E0.inc"
};
char gMonsterNames_27[16] = {
#include "sub_main/004911F0.inc"
};
char gMonsterNames_28[16] = {
#include "sub_main/00491200.inc"
};
char gMonsterNames_29[8] = {
#include "sub_main/00491210.inc"
};
char gMonsterNames_30[12] = {
#include "sub_main/00491218.inc"
};
char gMonsterNames_31[13] = {
#include "sub_main/00491228.inc"
};
char gMonsterNames_32[8] = {
#include "sub_main/00491238.inc"
};
char gMonsterNames_33[8] = {
#include "sub_main/00491240.inc"
};
char gMonsterNames_34[12] = {
#include "sub_main/00491248.inc"
};
char D_00491260[32] = {
#include "sub_main/00491260.inc"
};
char gMonsterNames_38[32] = {
#include "sub_main/00491280.inc"
};
char gMonsterNames_39[24] = {
#include "sub_main/004912A0.inc"
};
char gMonsterNames_40[15] = {
#include "sub_main/004912B8.inc"
};
char D_004912D0[32] = {
#include "sub_main/004912D0.inc"
};
char gMonsterNames_43[32] = {
#include "sub_main/004912F0.inc"
};
char gMonsterNames_44[32] = {
#include "sub_main/00491310.inc"
};
char gMonsterNames_45[32] = {
#include "sub_main/00491330.inc"
};
char gMonsterNames_46[32] = {
#include "sub_main/00491350.inc"
};
char gMonsterNames_47[32] = {
#include "sub_main/00491370.inc"
};
char gMonsterNames_48[24] = {
#include "sub_main/00491390.inc"
};
char gMonsterNames_51[13] = {
#include "sub_main/004913A8.inc"
};
char gMonsterNames_52[12] = {
#include "sub_main/004913B8.inc"
};
char gMonsterNames_53[12] = {
#include "sub_main/004913C8.inc"
};
char D_004913D8[15] = {
#include "sub_main/004913D8.inc"
};
char gMonsterNames_55[15] = {
#include "sub_main/004913E8.inc"
};
char gMonsterNames_56[12] = {
#include "sub_main/004913F8.inc"
};
char gMonsterNames_57[13] = {
#include "sub_main/00491408.inc"
};
char D_00491418[12] = {
#include "sub_main/00491418.inc"
};
char gMonsterNames_59[12] = {
#include "sub_main/00491428.inc"
};
char gMonsterNames_61[12] = {
#include "sub_main/00491438.inc"
};
char gMonsterNames_62[8] = {
#include "sub_main/00491448.inc"
};
char gMonsterNames_63[16] = {
#include "sub_main/00491450.inc"
};
char gMonsterNames_64[32] = {
#include "sub_main/00491460.inc"
};
char gMonsterNames_65[24] = {
#include "sub_main/00491480.inc"
};
char gMonsterNames_66[12] = {
#include "sub_main/00491498.inc"
};
char gMonsterNames_67[24] = {
#include "sub_main/004914B0.inc"
};
char gMonsterNames_68[13] = {
#include "sub_main/004914C8.inc"
};
char gMonsterNames_69[12] = {
#include "sub_main/004914D8.inc"
};
char gMonsterNames_70[8] = {
#include "sub_main/004914E8.inc"
};
char gMonsterNames_73[16] = {
#include "sub_main/004914F0.inc"
};
char D_00491500[16] = {
#include "sub_main/00491500.inc"
};
char D_00491510[8] = {
#include "sub_main/00491510.inc"
};
char D_00491518[12] = {
#include "sub_main/00491518.inc"
};
char D_00491528[13] = {
#include "sub_main/00491528.inc"
};
char D_00491538[12] = {
#include "sub_main/00491538.inc"
};
char D_00491548[12] = {
#include "sub_main/00491548.inc"
};
char D_00491558[12] = {
#include "sub_main/00491558.inc"
};
char gFieldMenuItemsOnline_8[13] = {
#include "sub_main/00491568.inc"
};
char D_00491578[12] = {
#include "sub_main/00491578.inc"
};
char D_00491588[12] = {
#include "sub_main/00491588.inc"
};
char D_00491598[12] = {
#include "sub_main/00491598.inc"
};
char D_004915A8[12] = {
#include "sub_main/004915A8.inc"
};
char D_004915B8[15] = {
#include "sub_main/004915B8.inc"
};
char D_004915C8[13] = {
#include "sub_main/004915C8.inc"
};
char gFieldMenuItemsOffline_36[13] = {
#include "sub_main/004915D8.inc"
};
char gFieldMenuItemsOffline_43[12] = {
#include "sub_main/004915E8.inc"
};
char gPouchMenuPanel2_6_0[8] = {
#include "sub_main/004915F8.inc"
};
char gPouchMenuPanel2_6_1[8] = {
#include "sub_main/00491600.inc"
};
char gDeliveryPanel_7_0[8] = {
#include "sub_main/00491608.inc"
};
char D_00491610[16] = {
#include "sub_main/00491610.inc"
};
char D_00491620[32] = {
#include "sub_main/00491620.inc"
};
char gYesNoTexts_1[32] = {
#include "sub_main/00491640.inc"
};
char D_00491660[8] = {
#include "sub_main/00491660.inc"
};
char D_00491668[8] = {
#include "sub_main/00491668.inc"
};
char D_00491670[16] = {
#include "sub_main/00491670.inc"
};
char gItemActionLabels_4[16] = {
#include "sub_main/00491680.inc"
};
char gItemActionLabels_5[16] = {
#include "sub_main/00491690.inc"
};
char gItemActionLabels_6[16] = {
#include "sub_main/004916A0.inc"
};
char D_004916B0[16] = {
#include "sub_main/004916B0.inc"
};
char D_004916C0[16] = {
#include "sub_main/004916C0.inc"
};
char D_004916D0[16] = {
#include "sub_main/004916D0.inc"
};
char gStatusMenuColors_1_2[16] = {
#include "sub_main/004916E0.inc"
};
char gStatusMenuColors_1_3[32] = {
#include "sub_main/004916F0.inc"
};
char gStatusMenuColors_1_4[24] = {
#include "sub_main/00491710.inc"
};
char D_00491728[12] = {
#include "sub_main/00491728.inc"
};
char D_00491740[24] = {
#include "sub_main/00491740.inc"
};
char gInfoMenuLabels_0[12] = {
#include "sub_main/00491758.inc"
};
char gInfoMenuLabels_3[13] = {
#include "sub_main/00491768.inc"
};
char gCockpitToggleTexts_0[32] = {
#include "sub_main/00491780.inc"
};
char gCockpitToggleTexts_1[16] = {
#include "sub_main/004917A0.inc"
};
char gQuestStatusTitle_0[32] = {
#include "sub_main/004917B0.inc"
};
char gQuestStatusTitle_1[32] = {
#include "sub_main/004917D0.inc"
};
char gQuestMenuRecordTitles_0[48] = {
#include "sub_main/004917F0.inc"
};
char gQuestMenuRecordTitles_1[48] = {
#include "sub_main/00491820.inc"
};
char gQuestMenuTitles_0[48] = {
#include "sub_main/00491850.inc"
};
char gQuestMenuTitles_1[40] = {
#include "sub_main/00491880.inc"
};
char gQuestMenuStatusTexts_0[8] = {
#include "sub_main/004918A8.inc"
};
char gQuestMenuStatusTexts_1[16] = {
#include "sub_main/004918B0.inc"
};
char gQuestMenuTargetTexts_0[16] = {
#include "sub_main/004918C0.inc"
};
char gQuestMenuTargetTexts_1[16] = {
#include "sub_main/004918D0.inc"
};
char gDojoNoRecordText_0[16] = {
#include "sub_main/004918E0.inc"
};
char gDojoNoRecordText_1[16] = {
#include "sub_main/004918F0.inc"
};
char D_00491900[8] = {
#include "sub_main/00491900.inc"
};
char gQuestInfoFormats_1[12] = {
#include "sub_main/00491908.inc"
};
char gQuestInfoFormats_2[12] = {
#include "sub_main/00491918.inc"
};
char gQuestInfoFormats_3[32] = {
#include "sub_main/00491930.inc"
};
char D_00491950[32] = {
#include "sub_main/00491950.inc"
};
char gQuestInfoFormats_5[24] = {
#include "sub_main/00491970.inc"
};
char D_00491988[15] = {
#include "sub_main/00491988.inc"
};
char gQuestMenuItemLists_16[12] = {
#include "sub_main/00491998.inc"
};
char gQuestMenuItemLists_21[24] = {
#include "sub_main/004919B0.inc"
};
char D_004919C8[12] = {
#include "sub_main/004919C8.inc"
};
char gQuestMenuItemLists_25[32] = {
#include "sub_main/004919E0.inc"
};
char gQuestMenuItemLists_28[24] = {
#include "sub_main/00491A00.inc"
};
char D_00491A18[12] = {
#include "sub_main/00491A18.inc"
};
char D_00491A28[12] = {
#include "sub_main/00491A28.inc"
};
char D_00491A38[12] = {
#include "sub_main/00491A38.inc"
};
char gQuestMenuItemLists_40[15] = {
#include "sub_main/00491A48.inc"
};
char gQuestMenuItemLists_41[15] = {
#include "sub_main/00491A58.inc"
};
char gQuestMenuItemLists_42[15] = {
#include "sub_main/00491A68.inc"
};
char D_00491A78[12] = {
#include "sub_main/00491A78.inc"
};
char D_00491A88[12] = {
#include "sub_main/00491A88.inc"
};
char D_00491A98[12] = {
#include "sub_main/00491A98.inc"
};
char D_00491AA8[12] = {
#include "sub_main/00491AA8.inc"
};
char D_00491AB8[12] = {
#include "sub_main/00491AB8.inc"
};
char gQuestMenuItemLists_72[32] = {
#include "sub_main/00491AD0.inc"
};
char gQuestMenuItemLists_73[24] = {
#include "sub_main/00491AF0.inc"
};
char gQuestMenuItemLists_74[13] = {
#include "sub_main/00491B08.inc"
};
char gQuestMenuItemLists_75[13] = {
#include "sub_main/00491B18.inc"
};
char gQuestMenuItemLists_76[13] = {
#include "sub_main/00491B28.inc"
};
char gQuestMenuItemLists_90[15] = {
#include "sub_main/00491B38.inc"
};
char gQuestMenuItemLists_91[15] = {
#include "sub_main/00491B48.inc"
};
char gQuestMenuItemLists_105[32] = {
#include "sub_main/00491B60.inc"
};
char gQuestMenuItemLists_140[24] = {
#include "sub_main/00491B80.inc"
};
char gQuestMenuItemLists_159[12] = {
#include "sub_main/00491B98.inc"
};
char gQuestTargetLabels_0[96] = {
#include "sub_main/00491BB0.inc"
};
char gQuestTargetLabels_2[32] = {
#include "sub_main/00491C10.inc"
};
char gChatTypeTexts_0[16] = {
#include "sub_main/00491C30.inc"
};
char gCombineSelectFmt_0[32] = {
#include "sub_main/00491C40.inc"
};
char gCombineInfoLabels_0[40] = {
#include "sub_main/00491C60.inc"
};
char gCombineInfoLabels_1[8] = {
#include "sub_main/00491C88.inc"
};
char gCombineRateFmt_0[8] = {
#include "sub_main/00491C90.inc"
};
char gCombineRateFmt_1[8] = {
#include "sub_main/00491C98.inc"
};
char gCombineSelectingText_0[16] = {
#include "sub_main/00491CA0.inc"
};
char gCombineSelectingText_1[48] = {
#include "sub_main/00491CB0.inc"
};
char gCombineFailLabels_0[40] = {
#include "sub_main/00491CE0.inc"
};
char gCombine2Text_0[12] = {
#include "sub_main/00491D08.inc"
};
char gCombine2Text_1[12] = {
#include "sub_main/00491D18.inc"
};
char D_00491D28[12] = {
#include "sub_main/00491D28.inc"
};
char D_00491D38[12] = {
#include "sub_main/00491D38.inc"
};
char gCombine3MaterialLabels_2[12] = {
#include "sub_main/00491D48.inc"
};
char gCombineDoText_0[13] = {
#include "sub_main/00491D58.inc"
};
char D_00491D68[8] = {
#include "sub_main/00491D68.inc"
};
char gAmmoTensFmt_0[16] = {
#include "sub_main/00491D70.inc"
};
char gItemFullText_0[32] = {
#include "sub_main/00491D80.inc"
};
char gItemFullText_1[32] = {
#include "sub_main/00491DA0.inc"
};
char gFieldMenuTitle_0[24] = {
#include "sub_main/00491DC0.inc"
};
char gPausedText_0[15] = {
#include "sub_main/00491DD8.inc"
};
char gPhraseEditFmt_0[24] = {
#include "sub_main/00491DF0.inc"
};
char gChatMenuPanel_7_0[13] = {
#include "sub_main/00491E08.inc"
};
char gSupplySelectText_0[32] = {
#include "sub_main/00491E20.inc"
};
char gSupplySelectText_1[32] = {
#include "sub_main/00491E40.inc"
};
char gSupplyCannotCarryText_0[40] = {
#include "sub_main/00491E60.inc"
};
char gSupplyCountFmt_0[8] = {
#include "sub_main/00491E88.inc"
};
char D_00491E90[32] = {
#include "sub_main/00491E90.inc"
};
char D_00491EB0[32] = {
#include "sub_main/00491EB0.inc"
};
char D_00491ED0[32] = {
#include "sub_main/00491ED0.inc"
};
char D_00491EF0[32] = {
#include "sub_main/00491EF0.inc"
};
char D_00491F10[32] = {
#include "sub_main/00491F10.inc"
};
char gSubTargetHelpText_0[192] = {
#include "sub_main/00491F30.inc"
};
char gSubTargetHelpText_1[16] = {
#include "sub_main/00491FF0.inc"
};
char D_00492000[144] = {
#include "sub_main/00492000.inc"
};
char gQuestMenuHelpTexts_0[208] = {
#include "sub_main/00492090.inc"
};
char gQuestMenuHelpTexts_1[200] = {
#include "sub_main/00492160.inc"
};
char gDeliveryMenuTexts_0[13] = {
#include "sub_main/00492228.inc"
};
char gDeliveryMenuTexts_1[12] = {
#include "sub_main/00492238.inc"
};
char gDeliveryMenuTexts_2[12] = {
#include "sub_main/00492248.inc"
};
char gDeliveryMenuTexts_3[12] = {
#include "sub_main/00492258.inc"
};
char gDeliveryDoneText_0[8] = {
#include "sub_main/00492268.inc"
};
char gDeliveryDoneText_1[32] = {
#include "sub_main/00492270.inc"
};
char gDeliveryHelpText_0[176] = {
#include "sub_main/00492290.inc"
};
char gCombineListFmt_0[40] = {
#include "sub_main/00492340.inc"
};
char gCombineListFmt_1[8] = {
#include "sub_main/00492368.inc"
};
char gItemListLineTexts_0[32] = {
#include "sub_main/00492370.inc"
};
char gItemListLineTexts_1[32] = {
#include "sub_main/00492390.inc"
};
char gRecipeMaterialsFmt_0[32] = {
#include "sub_main/004923B0.inc"
};
char gRecipeMaterialsFmt_1[24] = {
#include "sub_main/004923D0.inc"
};
char gRecipeSwitchHint_0[15] = {
#include "sub_main/004923E8.inc"
};
char gRecipeMaterialFmts_0[15] = {
#include "sub_main/004923F8.inc"
};
char gRecipeMaterialFmts_1[15] = {
#include "sub_main/00492408.inc"
};
char gRecipeMaterialFmts_2[15] = {
#include "sub_main/00492418.inc"
};
u32 D_00492428[3] = {
#include "sub_main/00492428.inc"
};
char D_00492438[12] = {
#include "sub_main/00492438.inc"
};
char gRecipeRateLabels_0[8] = {
#include "sub_main/00492448.inc"
};
char gRecipeRateLabels_1[8] = {
#include "sub_main/00492450.inc"
};
char gRecipeRateLabels_2[8] = {
#include "sub_main/00492458.inc"
};
char gRecipeRateLabels_3[8] = {
#include "sub_main/00492460.inc"
};
char gRecipeRateLabels_4[8] = {
#include "sub_main/00492468.inc"
};
char gRecipeRateLabels_5[8] = {
#include "sub_main/00492470.inc"
};
char gRecipeYieldLabels_0[8] = {
#include "sub_main/00492478.inc"
};
char gRecipeYieldLabels_1[8] = {
#include "sub_main/00492480.inc"
};
char D_00492488[8] = {
#include "sub_main/00492488.inc"
};
char gRecipeYieldLabels_4[8] = {
#include "sub_main/00492490.inc"
};
char D_00492498[8] = {
#include "sub_main/00492498.inc"
};
char gRecipeYieldLabels_6[8] = {
#include "sub_main/004924A0.inc"
};
char gRecipeYieldLabels_8[8] = {
#include "sub_main/004924A8.inc"
};
char gMonsterListTitle_0[24] = {
#include "sub_main/004924B0.inc"
};
char gMonsterListTitle_1[8] = {
#include "sub_main/004924C8.inc"
};
char gMonsterListTitle_2[24] = {
#include "sub_main/004924D0.inc"
};
char gMonsterListTitle_3[8] = {
#include "sub_main/004924E8.inc"
};
char gMonsterNoSizeText_0[16] = {
#include "sub_main/004924F0.inc"
};
char gMonsterNoSizeText_1[32] = {
#include "sub_main/00492500.inc"
};
char gArenaBaitNoBookText_0[32] = {
#include "sub_main/00492520.inc"
};
char gArenaBaitNoBookText_1[32] = {
#include "sub_main/00492540.inc"
};
char gMonsterNoteLabels_0[32] = {
#include "sub_main/00492560.inc"
};
char gMonsterNoteLabels_1[32] = {
#include "sub_main/00492580.inc"
};
char gArenaBaitHint_0[32] = {
#include "sub_main/004925A0.inc"
};
char gArenaBaitHint_1[32] = {
#include "sub_main/004925C0.inc"
};
char gArenaBaitLabels_0[64] = {
#include "sub_main/004925E0.inc"
};
char D_00492620[8] = {
#include "sub_main/00492620.inc"
};
char gMonsterNoteValueFmt_0[8] = {
#include "sub_main/00492628.inc"
};
char gEmDescTexts_0[224] = {
#include "sub_main/00492630.inc"
};
char gEmDescTexts_1[272] = {
#include "sub_main/00492710.inc"
};
char gEmDescTexts_2[256] = {
#include "sub_main/00492820.inc"
};
char gEmDescTexts_3[208] = {
#include "sub_main/00492920.inc"
};
char gEmDescTexts_4[240] = {
#include "sub_main/004929F0.inc"
};
char gEmDescTexts_5[224] = {
#include "sub_main/00492AE0.inc"
};
char gEmDescTexts_6[240] = {
#include "sub_main/00492BC0.inc"
};
char gEmDescTexts_7[272] = {
#include "sub_main/00492CB0.inc"
};
char gEmDescTexts_8[240] = {
#include "sub_main/00492DC0.inc"
};
char gEmDescTexts_9[192] = {
#include "sub_main/00492EB0.inc"
};
char gEmDescTexts_10[208] = {
#include "sub_main/00492F70.inc"
};
char gEmDescTexts_11[208] = {
#include "sub_main/00493040.inc"
};
char gEmDescTexts_12[224] = {
#include "sub_main/00493110.inc"
};
char gEmDescTexts_13[224] = {
#include "sub_main/004931F0.inc"
};
char gEmDescTexts_14[224] = {
#include "sub_main/004932D0.inc"
};
char gEmDescTexts_15[176] = {
#include "sub_main/004933B0.inc"
};
char gEmDescTexts_16[160] = {
#include "sub_main/00493460.inc"
};
char gEmDescTexts_17[192] = {
#include "sub_main/00493500.inc"
};
char gEmDescTexts_18[160] = {
#include "sub_main/004935C0.inc"
};
char gEmDescTexts_19[208] = {
#include "sub_main/00493660.inc"
};
char gEmDescTexts_20[256] = {
#include "sub_main/00493730.inc"
};
char gEmDescTexts_21[224] = {
#include "sub_main/00493830.inc"
};
char gEmDescTexts_22[224] = {
#include "sub_main/00493910.inc"
};
char gEmDescTexts_23[240] = {
#include "sub_main/004939F0.inc"
};
char gEmDescTexts_24[256] = {
#include "sub_main/00493AE0.inc"
};
char gEmDescTexts_25[208] = {
#include "sub_main/00493BE0.inc"
};
char gEmDescTexts_26[192] = {
#include "sub_main/00493CB0.inc"
};
char gEmDescTexts_27[192] = {
#include "sub_main/00493D70.inc"
};
char gEmDescTexts_28[224] = {
#include "sub_main/00493E30.inc"
};
char gEmDescTexts_29[192] = {
#include "sub_main/00493F10.inc"
};
char gEmDescTexts_30[192] = {
#include "sub_main/00493FD0.inc"
};
char gEmDescTexts_31[192] = {
#include "sub_main/00494090.inc"
};
char gEmDescTexts_32[208] = {
#include "sub_main/00494150.inc"
};
char gEmDescTexts_33[240] = {
#include "sub_main/00494220.inc"
};
char gEmDescTexts_34[176] = {
#include "sub_main/00494310.inc"
};
char gEmDescTexts_35[240] = {
#include "sub_main/004943C0.inc"
};
char gEmDescTexts_36[256] = {
#include "sub_main/004944B0.inc"
};
char gEmDescTexts_37[240] = {
#include "sub_main/004945B0.inc"
};
char gEmDescTexts_38[224] = {
#include "sub_main/004946A0.inc"
};
char gEmDescTexts_39[208] = {
#include "sub_main/00494780.inc"
};
char gEmDescTexts_40[320] = {
#include "sub_main/00494850.inc"
};
char gEmDescTexts_41[208] = {
#include "sub_main/00494990.inc"
};
char gEmDescTexts_42[240] = {
#include "sub_main/00494A60.inc"
};
char gEmDescTexts_43[208] = {
#include "sub_main/00494B50.inc"
};
char gEmDescTexts_44[224] = {
#include "sub_main/00494C20.inc"
};
char gEmDescTexts_45[240] = {
#include "sub_main/00494D00.inc"
};
char gEmDescTexts_46[224] = {
#include "sub_main/00494DF0.inc"
};
char gEmDescTexts_47[256] = {
#include "sub_main/00494ED0.inc"
};
char gEmDescTexts_48[256] = {
#include "sub_main/00494FD0.inc"
};
char gEmDescTexts_49[176] = {
#include "sub_main/004950D0.inc"
};
char gEmDescTexts_50[240] = {
#include "sub_main/00495180.inc"
};
char gEmDescTexts_51[128] = {
#include "sub_main/00495270.inc"
};
char gHunterTitles_4[16] = {
#include "sub_main/004952F0.inc"
};
char gHunterTitles_5[16] = {
#include "sub_main/00495300.inc"
};
char gHunterTitles_6[16] = {
#include "sub_main/00495310.inc"
};
char gHunterTitles_7[24] = {
#include "sub_main/00495320.inc"
};
char gHunterTitles_8[12] = {
#include "sub_main/00495338.inc"
};
char gHunterTitles_9[12] = {
#include "sub_main/00495348.inc"
};
char gHunterTitles_125[13] = {
#include "sub_main/00495358.inc"
};
char gHunterTitles_10[24] = {
#include "sub_main/00495370.inc"
};
char gHunterTitles_11[15] = {
#include "sub_main/00495388.inc"
};
char gHunterTitles_12[13] = {
#include "sub_main/00495398.inc"
};
char gHunterTitles_13[15] = {
#include "sub_main/004953A8.inc"
};
char gHunterTitles_14[13] = {
#include "sub_main/004953B8.inc"
};
char gHunterTitles_15[13] = {
#include "sub_main/004953C8.inc"
};
char gHunterTitles_16[32] = {
#include "sub_main/004953E0.inc"
};
char gHunterTitles_17[32] = {
#include "sub_main/00495400.inc"
};
char gHunterTitles_18[24] = {
#include "sub_main/00495420.inc"
};
char gHunterTitles_19[12] = {
#include "sub_main/00495438.inc"
};
char gHunterTitles_20[24] = {
#include "sub_main/00495450.inc"
};
char gHunterTitles_21[13] = {
#include "sub_main/00495468.inc"
};
char gHunterTitles_22[32] = {
#include "sub_main/00495480.inc"
};
char gHunterTitles_120[32] = {
#include "sub_main/004954A0.inc"
};
char gHunterTitles_23[32] = {
#include "sub_main/004954C0.inc"
};
char gHunterTitles_24[32] = {
#include "sub_main/004954E0.inc"
};
char gHunterTitles_25[24] = {
#include "sub_main/00495500.inc"
};
char gHunterTitles_26[13] = {
#include "sub_main/00495518.inc"
};
char gHunterTitles_28[32] = {
#include "sub_main/00495530.inc"
};
char gEquipStatusTitle_0[32] = {
#include "sub_main/00495550.inc"
};
char gOptionMenuTitle_0[24] = {
#include "sub_main/00495570.inc"
};
char D_00495588[15] = {
#include "sub_main/00495588.inc"
};
char gFieldOptionLabels_1[12] = {
#include "sub_main/00495598.inc"
};
char D_004955B0[24] = {
#include "sub_main/004955B0.inc"
};
char D_004955C8[13] = {
#include "sub_main/004955C8.inc"
};
char D_004955D8[12] = {
#include "sub_main/004955D8.inc"
};
char D_004955E8[12] = {
#include "sub_main/004955E8.inc"
};
char D_004955F8[13] = {
#include "sub_main/004955F8.inc"
};
char D_00495608[15] = {
#include "sub_main/00495608.inc"
};
char D_00495620[32] = {
#include "sub_main/00495620.inc"
};
char gFieldOptionLabels_10[24] = {
#include "sub_main/00495640.inc"
};
char D_00495658[8] = {
#include "sub_main/00495658.inc"
};
char D_00495660[8] = {
#include "sub_main/00495660.inc"
};
char D_00495668[8] = {
#include "sub_main/00495668.inc"
};
char D_00495670[16] = {
#include "sub_main/00495670.inc"
};
char D_00495680[16] = {
#include "sub_main/00495680.inc"
};
char gFieldOptionValues_9[8] = {
#include "sub_main/00495690.inc"
};
char gFieldOptionValues_10[8] = {
#include "sub_main/00495698.inc"
};
char gFieldOptionValues_18[8] = {
#include "sub_main/004956A0.inc"
};
char gFieldOptionValues_19[8] = {
#include "sub_main/004956A8.inc"
};
char gFieldOptionValues_20[8] = {
#include "sub_main/004956B0.inc"
};
char gFieldOptionValues_21[8] = {
#include "sub_main/004956B8.inc"
};
char gFieldOptionValues_22[8] = {
#include "sub_main/004956C0.inc"
};
char gRarityFmt_0[8] = {
#include "sub_main/004956C8.inc"
};
char D_004956D0[24] = {
#include "sub_main/004956D0.inc"
};
char D_004956E8[12] = {
#include "sub_main/004956E8.inc"
};
char D_004956F8[12] = {
#include "sub_main/004956F8.inc"
};
char gChatRecvText_0[12] = {
#include "sub_main/00495708.inc"
};
char gChatRecvText_1[13] = {
#include "sub_main/00495718.inc"
};
char gTalkEventTitle_0[13] = {
#include "sub_main/00495728.inc"
};
char gNoticeNoSelText_0[32] = {
#include "sub_main/00495740.inc"
};
char gNoticeNoSelText_1[8] = {
#include "sub_main/00495760.inc"
};
char gItemListTitles_1[12] = {
#include "sub_main/00495768.inc"
};
char gItemListTitleFmt_0[32] = {
#include "sub_main/00495780.inc"
};
char D_004957A0[24] = {
#include "sub_main/004957A0.inc"
};
char gInfinityText_0[8] = {
#include "sub_main/004957B8.inc"
};
char gStatusHeadLabels_0[48] = {
#include "sub_main/004957C0.inc"
};
char gStatusHeadLabels_1[48] = {
#include "sub_main/004957F0.inc"
};
char gStatusHeadLabels_2[48] = {
#include "sub_main/00495820.inc"
};
char gStatusHeadLabels_3[64] = {
#include "sub_main/00495850.inc"
};
char gSymbolColorLabel_0[32] = {
#include "sub_main/00495890.inc"
};
char gSymbolColorLabel_1[32] = {
#include "sub_main/004958B0.inc"
};
char gArenaRecordLabels_0[80] = {
#include "sub_main/004958D0.inc"
};
char gArenaRecordLabels_1[32] = {
#include "sub_main/00495920.inc"
};
char gArenaRecordFmt_0[16] = {
#include "sub_main/00495940.inc"
};
char gArenaRecordFmt_1[16] = {
#include "sub_main/00495950.inc"
};
char D_00495960[32] = {
#include "sub_main/00495960.inc"
};
char gStatusStatLabels_0[32] = {
#include "sub_main/00495980.inc"
};
char gStatusStatLabels_1[32] = {
#include "sub_main/004959A0.inc"
};
char gStatusStatLabels_2[32] = {
#include "sub_main/004959C0.inc"
};
char gStatusStatLabels_3[32] = {
#include "sub_main/004959E0.inc"
};
char gWeaponTypeNames_0[8] = {
#include "sub_main/00495A00.inc"
};
char gWeaponTypeNames_1[15] = {
#include "sub_main/00495A08.inc"
};
char gWeaponTypeNames_2[12] = {
#include "sub_main/00495A18.inc"
};
char gWeaponTypeNames_3[8] = {
#include "sub_main/00495A28.inc"
};
char gWeaponTypeNames_4[8] = {
#include "sub_main/00495A30.inc"
};
char gWeaponTypeNames_5[15] = {
#include "sub_main/00495A38.inc"
};
char gWeaponTypeNames_6[8] = {
#include "sub_main/00495A48.inc"
};
char gWeaponTypeNames_7[8] = {
#include "sub_main/00495A50.inc"
};
char gWeaponTypeNames_8[8] = {
#include "sub_main/00495A58.inc"
};
char gWeaponTypeNames_9[16] = {
#include "sub_main/00495A60.inc"
};
char gWeaponTypeNames_10[8] = {
#include "sub_main/00495A70.inc"
};
char gStatusBoxTitles_0[13] = {
#include "sub_main/00495A78.inc"
};
char D_00495A88[15] = {
#include "sub_main/00495A88.inc"
};
char gStatusPageFmt_0[48] = {
#include "sub_main/00495AA0.inc"
};
char D_00495AD0[8] = {
#include "sub_main/00495AD0.inc"
};
char gRankPointsFmt_0[13] = {
#include "sub_main/00495AD8.inc"
};
char gRankPointsFmt_1[8] = {
#include "sub_main/00495AE8.inc"
};
char gSkillNameFmt_0[16] = {
#include "sub_main/00495AF0.inc"
};
char gSkillNameFmt_1[16] = {
#include "sub_main/00495B00.inc"
};
char gSkillPartPtsFmt_0[32] = {
#include "sub_main/00495B10.inc"
};
char gSkillPartPtsFmt_1[32] = {
#include "sub_main/00495B30.inc"
};
char gSkillRowFmt_0[32] = {
#include "sub_main/00495B50.inc"
};
char gSkillRowFmt_1[16] = {
#include "sub_main/00495B70.inc"
};
char gSkillRowFmt_2[40] = {
#include "sub_main/00495B80.inc"
};
char D_00495BA8[12] = {
#include "sub_main/00495BA8.inc"
};
char D_00495BC0[16] = {
#include "sub_main/00495BC0.inc"
};
char D_00495BD0[48] = {
#include "sub_main/00495BD0.inc"
};
char gSkillListHeader_0[40] = {
#include "sub_main/00495C00.inc"
};
char gSkillPartMarks_0[8] = {
#include "sub_main/00495C28.inc"
};
char gSkillPartMarks_1[8] = {
#include "sub_main/00495C30.inc"
};
char D_00495C38[13] = {
#include "sub_main/00495C38.inc"
};
char gUnknownEquipText_0[12] = {
#include "sub_main/00495C48.inc"
};
char gUnknownEquipText_1[12] = {
#include "sub_main/00495C58.inc"
};
char D_00495C70[24] = {
#include "sub_main/00495C70.inc"
};
char D_00495C88[12] = {
#include "sub_main/00495C88.inc"
};
char D_00495C98[12] = {
#include "sub_main/00495C98.inc"
};
char D_00495CB0[24] = {
#include "sub_main/00495CB0.inc"
};
char D_00495CC8[12] = {
#include "sub_main/00495CC8.inc"
};
u32 D_00495CD8[3] = {
#include "sub_main/00495CD8.inc"
};
char D_00495CE8[12] = {
#include "sub_main/00495CE8.inc"
};
char D_00495CF8[12] = {
#include "sub_main/00495CF8.inc"
};
u32 D_00495D08[3] = {
#include "sub_main/00495D08.inc"
};
char D_00495D20[32] = {
#include "sub_main/00495D20.inc"
};
char D_00495D40[32] = {
#include "sub_main/00495D40.inc"
};
char D_00495D60[32] = {
#include "sub_main/00495D60.inc"
};
char gArmorStatLabels_0[16] = {
#include "sub_main/00495D80.inc"
};
char gGunReloadTexts_0[8] = {
#include "sub_main/00495D90.inc"
};
char gGunReloadTexts_1[12] = {
#include "sub_main/00495D98.inc"
};
char D_00495DA8[8] = {
#include "sub_main/00495DA8.inc"
};
char gGunReloadTexts_3[16] = {
#include "sub_main/00495DB0.inc"
};
char gGunReloadTexts_4[8] = {
#include "sub_main/00495DC0.inc"
};
char gGunRecoilTexts_0[8] = {
#include "sub_main/00495DC8.inc"
};
char gGunRecoilTexts_1[8] = {
#include "sub_main/00495DD0.inc"
};
char gGunRecoilTexts_2[8] = {
#include "sub_main/00495DD8.inc"
};
char gGunRecoilTexts_3[8] = {
#include "sub_main/00495DE0.inc"
};
char gGunRecoilTexts_4[8] = {
#include "sub_main/00495DE8.inc"
};
char gGunRecoilTexts_5[8] = {
#include "sub_main/00495DF0.inc"
};
char gGunRecoilTexts_6[8] = {
#include "sub_main/00495DF8.inc"
};
char gHunterClassTexts_0[8] = {
#include "sub_main/00495E00.inc"
};
char gHunterClassTexts_1[12] = {
#include "sub_main/00495E08.inc"
};
char gHunterClassTexts_2[15] = {
#include "sub_main/00495E18.inc"
};
char gHunterClassTexts_3[12] = {
#include "sub_main/00495E28.inc"
};
char gEquipNameFmt_0[15] = {
#include "sub_main/00495E38.inc"
};
char gEquipNameFmt_1[32] = {
#include "sub_main/00495E50.inc"
};
char gScopeFixedText_0[24] = {
#include "sub_main/00495E70.inc"
};
char gScopeFixedText_1[13] = {
#include "sub_main/00495E88.inc"
};
char gPowerBarrelText_0[13] = {
#include "sub_main/00495E98.inc"
};
char gPowerBarrelText_1[13] = {
#include "sub_main/00495EA8.inc"
};
char gShieldText_0[13] = {
#include "sub_main/00495EB8.inc"
};
char gShieldText_1[8] = {
#include "sub_main/00495EC8.inc"
};
char gAmmoHeaderText_0[40] = {
#include "sub_main/00495ED0.inc"
};
char D_00495EF8[8] = {
#include "sub_main/00495EF8.inc"
};
char gAmmoHeaderText_2[16] = {
#include "sub_main/00495F00.inc"
};
char gAmmoHeaderText_3[24] = {
#include "sub_main/00495F10.inc"
};
char gAmmoHeaderText_4[13] = {
#include "sub_main/00495F28.inc"
};
char gAmmoHeaderText_5[32] = {
#include "sub_main/00495F40.inc"
};
char gAmmoNameFmt_0[24] = {
#include "sub_main/00495F60.inc"
};
char gAmmoNameFmt_1[12] = {
#include "sub_main/00495F78.inc"
};
char gAmmoLineFormats_0[32] = {
#include "sub_main/00495F90.inc"
};
char gAmmoLineFormats_1[48] = {
#include "sub_main/00495FB0.inc"
};
char gAmmoLineFormats_2[24] = {
#include "sub_main/00495FE0.inc"
};
char gAmmoLineFormats_3[12] = {
#include "sub_main/00495FF8.inc"
};
char gAmmoLineFormats_4[32] = {
#include "sub_main/00496010.inc"
};
char gAmmoLineFormats_5[48] = {
#include "sub_main/00496030.inc"
};
char gDecoDetailTitle_0[32] = {
#include "sub_main/00496060.inc"
};
char gDecoDetailTitle_1[24] = {
#include "sub_main/00496080.inc"
};
char gAffinityLabel_0[12] = {
#include "sub_main/00496098.inc"
};
char gAffinityLabel_1[8] = {
#include "sub_main/004960A8.inc"
};
char gAmmoNames_0[16] = {
#include "sub_main/004960B0.inc"
};
char gAmmoNames_1[16] = {
#include "sub_main/004960C0.inc"
};
char gAmmoNames_2[16] = {
#include "sub_main/004960D0.inc"
};
char gAmmoNames_3[16] = {
#include "sub_main/004960E0.inc"
};
char gAmmoNames_4[16] = {
#include "sub_main/004960F0.inc"
};
char gAmmoNames_5[8] = {
#include "sub_main/00496100.inc"
};
char gAmmoNames_6[8] = {
#include "sub_main/00496108.inc"
};
char gAmmoNames_7[8] = {
#include "sub_main/00496110.inc"
};
char gAmmoNames_8[8] = {
#include "sub_main/00496118.inc"
};
char gAmmoNames_9[8] = {
#include "sub_main/00496120.inc"
};
char gAmmoNames_10[8] = {
#include "sub_main/00496128.inc"
};
char gAmmoNames_11[8] = {
#include "sub_main/00496130.inc"
};
char gAmmoNames_12[8] = {
#include "sub_main/00496138.inc"
};
char gAmmoNames_13[8] = {
#include "sub_main/00496140.inc"
};
char gAmmoNames_14[13] = {
#include "sub_main/00496148.inc"
};
char gAmmoNames_15[13] = {
#include "sub_main/00496158.inc"
};
char gAmmoNames_16[13] = {
#include "sub_main/00496168.inc"
};
char gAmmoNames_17[13] = {
#include "sub_main/00496178.inc"
};
char gElementNames_0[8] = {
#include "sub_main/00496188.inc"
};
char gElementNames_1[8] = {
#include "sub_main/00496190.inc"
};
char gElementNames_2[8] = {
#include "sub_main/00496198.inc"
};
char gElementNames_3[8] = {
#include "sub_main/004961A0.inc"
};
char gElementNames_4[8] = {
#include "sub_main/004961A8.inc"
};
char D_004961B0[8] = {
#include "sub_main/004961B0.inc"
};
char D_004961B8[8] = {
#include "sub_main/004961B8.inc"
};
char D_004961C0[16] = {
#include "sub_main/004961C0.inc"
};
char gSkillPointsTitle_0[32] = {
#include "sub_main/004961D0.inc"
};
char gSkillPointsTitle_1[24] = {
#include "sub_main/004961F0.inc"
};
char gNoSkillText_0[15] = {
#include "sub_main/00496208.inc"
};
char gDecoDetailLabels_0[15] = {
#include "sub_main/00496218.inc"
};
char gDecoDetailLabels_1[15] = {
#include "sub_main/00496228.inc"
};
char gDecoDetailLabels_2[15] = {
#include "sub_main/00496238.inc"
};
char gRangeClassTexts_0[8] = {
#include "sub_main/00496248.inc"
};
char gRangeClassTexts_1[8] = {
#include "sub_main/00496250.inc"
};
char gRangeClassTexts_2[13] = {
#include "sub_main/00496258.inc"
};
char gRangeClassTexts_3[12] = {
#include "sub_main/00496268.inc"
};
char gDecoSkillPtsFmt_0[8] = {
#include "sub_main/00496278.inc"
};
char gDecoSkillPtsFmt_1[8] = {
#include "sub_main/00496280.inc"
};
char gDecoSkillFmt_0[12] = {
#include "sub_main/00496288.inc"
};
char gShellTypeTexts_0[12] = {
#include "sub_main/00496298.inc"
};
char gShellTypeTexts_1[12] = {
#include "sub_main/004962A8.inc"
};
char gShellTypeTexts_2[12] = {
#include "sub_main/004962B8.inc"
};
char gBowShotTexts_0[8] = {
#include "sub_main/004962C8.inc"
};
char gBowShotTexts_1[8] = {
#include "sub_main/004962D0.inc"
};
char gBowShotTexts_2[8] = {
#include "sub_main/004962D8.inc"
};
char gBowShotTexts_3[8] = {
#include "sub_main/004962E0.inc"
};
char gBowShotTexts_4[8] = {
#include "sub_main/004962E8.inc"
};
char gBowShotTexts_5[8] = {
#include "sub_main/004962F0.inc"
};
char gBowShotTexts_6[8] = {
#include "sub_main/004962F8.inc"
};
char gBowShotTexts_7[8] = {
#include "sub_main/00496300.inc"
};
char gBowShotTexts_8[8] = {
#include "sub_main/00496308.inc"
};
char gBowShotTexts_9[8] = {
#include "sub_main/00496310.inc"
};
char gBowShotTexts_10[8] = {
#include "sub_main/00496318.inc"
};
char gBowShotTexts_11[8] = {
#include "sub_main/00496320.inc"
};
char D_00496328[8] = {
#include "sub_main/00496328.inc"
};
char gTabPageFmt_1[8] = {
#include "sub_main/00496330.inc"
};
char gSortHintText_0[8] = {
#include "sub_main/00496338.inc"
};
char D_00496340[8] = {
#include "sub_main/00496340.inc"
};
char gSelectText_0[8] = {
#include "sub_main/00496348.inc"
};
char gDecoModeFmt_0[16] = {
#include "sub_main/00496350.inc"
};
char gCommentEditTitle_0[24] = {
#include "sub_main/00496360.inc"
};
char D_00496378[12] = {
#include "sub_main/00496378.inc"
};
char gChatTargetTexts_0[15] = {
#include "sub_main/00496388.inc"
};
char gChatTargetTexts_1[8] = {
#include "sub_main/00496398.inc"
};
char gChatTargetTexts_2[48] = {
#include "sub_main/004963A0.inc"
};
char gChatTargetTexts_3[32] = {
#include "sub_main/004963D0.inc"
};
char gChatNgWordsA_0[16] = {
#include "sub_main/004963F0.inc"
};
char gChatNgWordsA_1[16] = {
#include "sub_main/00496400.inc"
};
char gChatNgWordsA_2[16] = {
#include "sub_main/00496410.inc"
};
char gChatNgWordsA_3[16] = {
#include "sub_main/00496420.inc"
};
char gChatNgWordsA_4[16] = {
#include "sub_main/00496430.inc"
};
char gChatNgWordsA_5[16] = {
#include "sub_main/00496440.inc"
};
char gChatNgWordsA_6[16] = {
#include "sub_main/00496450.inc"
};
char gChatNgWordsA_7[16] = {
#include "sub_main/00496460.inc"
};
char gChatNgWordsA_8[16] = {
#include "sub_main/00496470.inc"
};
char gChatNgWordsA_9[16] = {
#include "sub_main/00496480.inc"
};
char gChatNgWordsA_10[16] = {
#include "sub_main/00496490.inc"
};
char gChatNgWordsA_11[16] = {
#include "sub_main/004964A0.inc"
};
char gChatNgWordsA_12[16] = {
#include "sub_main/004964B0.inc"
};
char gChatNgWordsA_13[16] = {
#include "sub_main/004964C0.inc"
};
char gChatNgWordsA_14[16] = {
#include "sub_main/004964D0.inc"
};
char gChatNgWordsA_15[16] = {
#include "sub_main/004964E0.inc"
};
char gChatNgWordsA_16[8] = {
#include "sub_main/004964F0.inc"
};
char gChatNgWordsA_17[8] = {
#include "sub_main/004964F8.inc"
};
char gChatNgWordsA_18[8] = {
#include "sub_main/00496500.inc"
};
char gChatNgWordsA_19[8] = {
#include "sub_main/00496508.inc"
};
char gChatNgWordsA_20[8] = {
#include "sub_main/00496510.inc"
};
char gChatNgWordsA_21[8] = {
#include "sub_main/00496518.inc"
};
char gChatNgWordsA_22[8] = {
#include "sub_main/00496520.inc"
};
char gChatNgWordsA_23[8] = {
#include "sub_main/00496528.inc"
};
char gChatNgWordsA_24[8] = {
#include "sub_main/00496530.inc"
};
char gChatNgWordsA_25[8] = {
#include "sub_main/00496538.inc"
};
char gChatNgWordsA_26[8] = {
#include "sub_main/00496540.inc"
};
char gChatNgWordsA_27[8] = {
#include "sub_main/00496548.inc"
};
char gChatNgWordsA_28[8] = {
#include "sub_main/00496550.inc"
};
char gChatNgWordsA_29[12] = {
#include "sub_main/00496558.inc"
};
char gChatNgWordsA_30[8] = {
#include "sub_main/00496568.inc"
};
char gChatNgWordsA_31[8] = {
#include "sub_main/00496570.inc"
};
char gChatNgWordsA_32[8] = {
#include "sub_main/00496578.inc"
};
char gChatNgWordsA_33[8] = {
#include "sub_main/00496580.inc"
};
char gChatNgWordsA_34[8] = {
#include "sub_main/00496588.inc"
};
char gChatNgWordsA_35[8] = {
#include "sub_main/00496590.inc"
};
char gChatNgWordsA_36[12] = {
#include "sub_main/00496598.inc"
};
char gChatNgWordsA_37[8] = {
#include "sub_main/004965A8.inc"
};
char gChatNgWordsA_38[8] = {
#include "sub_main/004965B0.inc"
};
char gChatNgWordsA_39[8] = {
#include "sub_main/004965B8.inc"
};
char gChatNgWordsA_40[8] = {
#include "sub_main/004965C0.inc"
};
char gChatNgWordsA_41[8] = {
#include "sub_main/004965C8.inc"
};
char gChatNgWordsA_42[8] = {
#include "sub_main/004965D0.inc"
};
char gChatNgWordsB_0[8] = {
#include "sub_main/004965D8.inc"
};
char gChatNgWordsB_1[8] = {
#include "sub_main/004965E0.inc"
};
char gChatNgWordsB_2[8] = {
#include "sub_main/004965E8.inc"
};
char gChatNgWordsB_3[8] = {
#include "sub_main/004965F0.inc"
};
char gChatNgWordsB_4[8] = {
#include "sub_main/004965F8.inc"
};
char gChatNgWordsB_5[8] = {
#include "sub_main/00496600.inc"
};
char gChatNgWordsB_6[8] = {
#include "sub_main/00496608.inc"
};
char gChatNgWordsB_7[8] = {
#include "sub_main/00496610.inc"
};
char gChatNgWordsB_8[8] = {
#include "sub_main/00496618.inc"
};
char gChatNgWordsB_9[8] = {
#include "sub_main/00496620.inc"
};
char gChatNgWordsB_10[12] = {
#include "sub_main/00496628.inc"
};
char gChatNgWordsB_11[12] = {
#include "sub_main/00496638.inc"
};
char gChatNgWordsB_12[8] = {
#include "sub_main/00496648.inc"
};
char gChatNgWordsB_13[8] = {
#include "sub_main/00496650.inc"
};
char gChatNgWordsB_14[8] = {
#include "sub_main/00496658.inc"
};
char gChatNgWordsB_15[8] = {
#include "sub_main/00496660.inc"
};
char gChatNgWordsB_16[8] = {
#include "sub_main/00496668.inc"
};
char gChatNgWordsB_17[8] = {
#include "sub_main/00496670.inc"
};
char gChatNgWordsB_18[8] = {
#include "sub_main/00496678.inc"
};
char gChatNgWordsB_19[8] = {
#include "sub_main/00496680.inc"
};
char gChatNgWordsB_20[8] = {
#include "sub_main/00496688.inc"
};
char gChatNgWordsB_21[8] = {
#include "sub_main/00496690.inc"
};
char gChatNgWordsB_22[12] = {
#include "sub_main/00496698.inc"
};
char gChatNgWordsB_23[12] = {
#include "sub_main/004966A8.inc"
};
char gChatNgWordsB_24[8] = {
#include "sub_main/004966B8.inc"
};
char gChatNgWordsB_25[8] = {
#include "sub_main/004966C0.inc"
};
char gChatNgWordsB_26[8] = {
#include "sub_main/004966C8.inc"
};
char gChatNgWordsB_27[8] = {
#include "sub_main/004966D0.inc"
};
char gChatNgWordsB_28[8] = {
#include "sub_main/004966D8.inc"
};
char gChatNgWordsB_29[8] = {
#include "sub_main/004966E0.inc"
};
char gChatNgWordsB_30[8] = {
#include "sub_main/004966E8.inc"
};
char gChatNgWordsB_31[8] = {
#include "sub_main/004966F0.inc"
};
char gChatNgWordsB_32[8] = {
#include "sub_main/004966F8.inc"
};
char gChatNgWordsB_33[16] = {
#include "sub_main/00496700.inc"
};
char gChatNgWordsB_34[16] = {
#include "sub_main/00496710.inc"
};
char gChatNgWordsB_35[8] = {
#include "sub_main/00496720.inc"
};
char gChatNgWordsB_36[8] = {
#include "sub_main/00496728.inc"
};
char gChatNgWordsB_37[8] = {
#include "sub_main/00496730.inc"
};
char gChatNgWordsB_38[8] = {
#include "sub_main/00496738.inc"
};
char gChatNgWordsB_39[8] = {
#include "sub_main/00496740.inc"
};
char gChatNgWordsB_40[12] = {
#include "sub_main/00496748.inc"
};
char gChatNgWordsB_41[12] = {
#include "sub_main/00496758.inc"
};
char gChatNgWordsB_42[8] = {
#include "sub_main/00496768.inc"
};
char gChatNgWordsB_43[8] = {
#include "sub_main/00496770.inc"
};
char gChatNgWordsB_44[12] = {
#include "sub_main/00496778.inc"
};
char gChatNgWordsB_45[12] = {
#include "sub_main/00496788.inc"
};
char gChatNgWordsB_46[8] = {
#include "sub_main/00496798.inc"
};
char gChatNgWordsB_47[8] = {
#include "sub_main/004967A0.inc"
};
char gChatNgWordsB_48[8] = {
#include "sub_main/004967A8.inc"
};
char gChatNgWordsB_49[16] = {
#include "sub_main/004967B0.inc"
};
char gChatNgWordsB_50[16] = {
#include "sub_main/004967C0.inc"
};
char gChatNgWordsB_51[16] = {
#include "sub_main/004967D0.inc"
};
char gChatNgWordsB_52[16] = {
#include "sub_main/004967E0.inc"
};
char gChatNgWordsB_53[8] = {
#include "sub_main/004967F0.inc"
};
char gChatNgWordsB_54[8] = {
#include "sub_main/004967F8.inc"
};
char gChatNgWordsB_55[8] = {
#include "sub_main/00496800.inc"
};
char gChatNgWordsB_56[8] = {
#include "sub_main/00496808.inc"
};
char gChatNgWordsB_57[8] = {
#include "sub_main/00496810.inc"
};
char gChatNgWordsB_58[8] = {
#include "sub_main/00496818.inc"
};
char gChatNgWordsB_59[16] = {
#include "sub_main/00496820.inc"
};
char gChatNgWordsB_60[16] = {
#include "sub_main/00496830.inc"
};
char gChatNgWordsB_61[16] = {
#include "sub_main/00496840.inc"
};
char gChatNgWordsB_62[16] = {
#include "sub_main/00496850.inc"
};
char gChatNgWordsB_63[16] = {
#include "sub_main/00496860.inc"
};
char gChatNgWordsB_64[16] = {
#include "sub_main/00496870.inc"
};
char gChatNgWordsB_65[16] = {
#include "sub_main/00496880.inc"
};
char gChatNgWordsB_66[16] = {
#include "sub_main/00496890.inc"
};
char gChatNgWordsB_67[16] = {
#include "sub_main/004968A0.inc"
};
char gChatNgWordsB_68[8] = {
#include "sub_main/004968B0.inc"
};
char gChatNgWordsB_69[12] = {
#include "sub_main/004968B8.inc"
};
char gChatNgWordsB_70[12] = {
#include "sub_main/004968C8.inc"
};
char gChatNgWordsB_71[12] = {
#include "sub_main/004968D8.inc"
};
char gChatNgWordsB_72[12] = {
#include "sub_main/004968E8.inc"
};
char gChatNgWordsB_73[8] = {
#include "sub_main/004968F8.inc"
};
char gChatNgWordsB_74[8] = {
#include "sub_main/00496900.inc"
};
char gChatNgWordsB_75[12] = {
#include "sub_main/00496908.inc"
};
char gChatNgWordsB_76[12] = {
#include "sub_main/00496918.inc"
};
char gChatNgWordsB_77[12] = {
#include "sub_main/00496928.inc"
};
char gChatNgWordsB_78[12] = {
#include "sub_main/00496938.inc"
};
char gChatNgWordsB_79[12] = {
#include "sub_main/00496948.inc"
};
char gChatNgWordsB_80[12] = {
#include "sub_main/00496958.inc"
};
char gChatNgWordsB_81[12] = {
#include "sub_main/00496968.inc"
};
char gChatNgWordsB_82[12] = {
#include "sub_main/00496978.inc"
};
char gChatNgWordsB_83[13] = {
#include "sub_main/00496988.inc"
};
char gChatNgWordsB_84[13] = {
#include "sub_main/00496998.inc"
};
char gChatNgWordsB_85[12] = {
#include "sub_main/004969A8.inc"
};
char gChatNgWordsB_86[12] = {
#include "sub_main/004969B8.inc"
};
char gChatNgWordsB_87[13] = {
#include "sub_main/004969C8.inc"
};
char gChatNgWordsB_88[13] = {
#include "sub_main/004969D8.inc"
};
char D_004969E8[13] = {
#include "sub_main/004969E8.inc"
};
char gMsgEventTexts_0[32] = {
#include "sub_main/00496A00.inc"
};
char gMsgEventTexts_1[24] = {
#include "sub_main/00496A20.inc"
};
char gMsgEventTexts_2[13] = {
#include "sub_main/00496A38.inc"
};
char gNoticeBannerTexts_0[32] = {
#include "sub_main/00496A50.inc"
};
char gNoticeBannerTexts_1[24] = {
#include "sub_main/00496A70.inc"
};
char gNoticeBannerTexts_2[13] = {
#include "sub_main/00496A88.inc"
};
char gNoticeBannerTexts_3[32] = {
#include "sub_main/00496AA0.inc"
};
char gNoticeBannerTexts_4[32] = {
#include "sub_main/00496AC0.inc"
};
char gNoticeBannerTexts_5[32] = {
#include "sub_main/00496AE0.inc"
};
char gNoticeBannerTexts_6[32] = {
#include "sub_main/00496B00.inc"
};
char gNoticeBannerTexts_7[32] = {
#include "sub_main/00496B20.inc"
};
char gNoticeBannerTexts_8[32] = {
#include "sub_main/00496B40.inc"
};
char gNoticeBannerTexts_9[32] = {
#include "sub_main/00496B60.inc"
};
char gNoticeBannerTexts_10[32] = {
#include "sub_main/00496B80.inc"
};
char gNoticeBannerTexts_11[32] = {
#include "sub_main/00496BA0.inc"
};
char gNoticeBannerTexts_12[32] = {
#include "sub_main/00496BC0.inc"
};
char gNoticeBannerTexts_13[32] = {
#include "sub_main/00496BE0.inc"
};
char gNoticeBannerTexts_14[32] = {
#include "sub_main/00496C00.inc"
};
char gNoticeBannerTexts_15[48] = {
#include "sub_main/00496C20.inc"
};
char gNoticeBannerTexts_16[32] = {
#include "sub_main/00496C50.inc"
};
char gNoticeBannerTexts_17[32] = {
#include "sub_main/00496C70.inc"
};
char gNoticeBannerTexts_18[32] = {
#include "sub_main/00496C90.inc"
};
char gNoticeBannerTexts_19[32] = {
#include "sub_main/00496CB0.inc"
};
char gNoticeBannerTexts_20[48] = {
#include "sub_main/00496CD0.inc"
};
char gNoticeBannerTexts_21[32] = {
#include "sub_main/00496D00.inc"
};
char gNoticeBannerTexts_22[32] = {
#include "sub_main/00496D20.inc"
};
char gNoticeBannerTexts_23[32] = {
#include "sub_main/00496D40.inc"
};
char gNoticeBannerTexts_24[32] = {
#include "sub_main/00496D60.inc"
};
char gNoticeAwardText_0[32] = {
#include "sub_main/00496D80.inc"
};
char gNoticeRateFlagText_0[48] = {
#include "sub_main/00496DA0.inc"
};
char D_00496DD0[32] = {
#include "sub_main/00496DD0.inc"
};
char D_00496DF0[32] = {
#include "sub_main/00496DF0.inc"
};
char D_00496E10[32] = {
#include "sub_main/00496E10.inc"
};
char D_00496E30[24] = {
#include "sub_main/00496E30.inc"
};
char D_00496E48[15] = {
#include "sub_main/00496E48.inc"
};
char gNoticeBannerItemTexts_0[15] = {
#include "sub_main/00496E58.inc"
};
char gNoticeBannerItemTexts_1[15] = {
#include "sub_main/00496E68.inc"
};
char gNoticeBannerItemTexts_2[12] = {
#include "sub_main/00496E78.inc"
};
char D_00496E90[24] = {
#include "sub_main/00496E90.inc"
};
char gNoticeBannerItemTexts_4[15] = {
#include "sub_main/00496EA8.inc"
};
char gNoticeBannerItemTexts_5[8] = {
#include "sub_main/00496EB8.inc"
};
char gNoticeBannerItemTexts_6[16] = {
#include "sub_main/00496EC0.inc"
};
char gNoticeBannerItemTexts_7[24] = {
#include "sub_main/00496ED0.inc"
};
char gNoticeBannerItemTexts_8[15] = {
#include "sub_main/00496EE8.inc"
};
char D_00496EF8[12] = {
#include "sub_main/00496EF8.inc"
};
char D_00496F10[24] = {
#include "sub_main/00496F10.inc"
};
char D_00496F28[13] = {
#include "sub_main/00496F28.inc"
};
char D_00496F38[13] = {
#include "sub_main/00496F38.inc"
};
char D_00496F48[12] = {
#include "sub_main/00496F48.inc"
};
char gNoticeBannerItemTexts_14[32] = {
#include "sub_main/00496F60.inc"
};
char D_00496F80[40] = {
#include "sub_main/00496F80.inc"
};
char D_00496FA8[12] = {
#include "sub_main/00496FA8.inc"
};
char gNoticeBannerItemTexts_17[13] = {
#include "sub_main/00496FB8.inc"
};
char gNoticeBannerItemTexts_18[32] = {
#include "sub_main/00496FD0.inc"
};
char gNoticeBannerItemTexts_19[32] = {
#include "sub_main/00496FF0.inc"
};
char gNoticeBannerItemTexts_20[32] = {
#include "sub_main/00497010.inc"
};
char gNoticeBannerTextsItem_0[24] = {
#include "sub_main/00497030.inc"
};
char gNoticeBannerPlayerTexts_0[12] = {
#include "sub_main/00497048.inc"
};
char gNoticeBannerPlayerTexts_1[32] = {
#include "sub_main/00497060.inc"
};
char gNoticeBannerPlayerTexts_2[32] = {
#include "sub_main/00497080.inc"
};
char gNoticeBannerPlayerTexts_3[16] = {
#include "sub_main/004970A0.inc"
};
char gNoticeMadeTexts_0[8] = {
#include "sub_main/004970B0.inc"
};
char gNoticeMadeTexts_1[12] = {
#include "sub_main/004970B8.inc"
};
char gNoticeMadeTexts_4[48] = {
#include "sub_main/004970D0.inc"
};
char gNothingMoreText_0[32] = {
#include "sub_main/00497100.inc"
};
char D_00497120[24] = {
#include "sub_main/00497120.inc"
};
char D_00497138[13] = {
#include "sub_main/00497138.inc"
};
char D_00497150[32] = {
#include "sub_main/00497150.inc"
};
char gSharpnessDownText_0[32] = {
#include "sub_main/00497170.inc"
};
char gSharpnessDownText_1[32] = {
#include "sub_main/00497190.inc"
};
char gSharpnessFullText_0[32] = {
#include "sub_main/004971B0.inc"
};
char gLoadingCaptions_0[32] = {
#include "sub_main/004971D0.inc"
};
char gLoadingCaptions_1[32] = {
#include "sub_main/004971F0.inc"
};
char gLoadingCaptionsAlt_0[32] = {
#include "sub_main/00497210.inc"
};
char gLoadingCaptionsAlt_1[32] = {
#include "sub_main/00497230.inc"
};
char gEmTraderMsgs_0[80] = {
#include "sub_main/00497250.inc"
};
char gEmTraderMsgs_1[96] = {
#include "sub_main/004972A0.inc"
};
char gEmTraderMsgs_2[64] = {
#include "sub_main/00497300.inc"
};
char gEmTraderMsgs_3[64] = {
#include "sub_main/00497340.inc"
};
char gEmTraderMsgs_4[96] = {
#include "sub_main/00497380.inc"
};
char gEmTraderMsgs_5[96] = {
#include "sub_main/004973E0.inc"
};
char gEmTraderMsgs_6[112] = {
#include "sub_main/00497440.inc"
};
char gEmTraderMsgs_7[48] = {
#include "sub_main/004974B0.inc"
};
char gEmTraderMsgs_8[96] = {
#include "sub_main/004974E0.inc"
};
char gEmTraderMsgs_9[80] = {
#include "sub_main/00497540.inc"
};
char gEmTraderMsgs_10[64] = {
#include "sub_main/00497590.inc"
};
char gEmTraderMsgs_11[64] = {
#include "sub_main/004975D0.inc"
};
char gEmTraderMsgs_12[112] = {
#include "sub_main/00497610.inc"
};
char gEmTraderMsgs_13[96] = {
#include "sub_main/00497680.inc"
};
char gEmTraderMsgs_14[112] = {
#include "sub_main/004976E0.inc"
};
char gEmTraderMsgs_15[96] = {
#include "sub_main/00497750.inc"
};
char gEmTraderMsgs_16[96] = {
#include "sub_main/004977B0.inc"
};
char gEmTraderMsgs_17[112] = {
#include "sub_main/00497810.inc"
};
char gEmTraderMsgs_18[64] = {
#include "sub_main/00497880.inc"
};
char gEmTraderMsgs_19[64] = {
#include "sub_main/004978C0.inc"
};
char gEmTraderMsgs_20[112] = {
#include "sub_main/00497900.inc"
};
char gEmTraderMsgs_21[96] = {
#include "sub_main/00497970.inc"
};
char gEmTraderMsgs_23[80] = {
#include "sub_main/004979D0.inc"
};
char gEmTraderMsgs_24[64] = {
#include "sub_main/00497A20.inc"
};
char gEmTraderMsgs_25[64] = {
#include "sub_main/00497A60.inc"
};
char gEmTraderMsgs_26[112] = {
#include "sub_main/00497AA0.inc"
};
char gEmTraderMsgs_27[96] = {
#include "sub_main/00497B10.inc"
};
char gEmTraderMsgs_28[80] = {
#include "sub_main/00497B70.inc"
};
char gEmTraderMsgs_29[112] = {
#include "sub_main/00497BC0.inc"
};
char gEmTraderMsgs_30[96] = {
#include "sub_main/00497C30.inc"
};
char gEmTraderMsgs_31[80] = {
#include "sub_main/00497C90.inc"
};
char gEmTraderMsgs_32[80] = {
#include "sub_main/00497CE0.inc"
};
char gEmTraderMsgs_33[96] = {
#include "sub_main/00497D30.inc"
};
char gEmTraderMsgs_34[80] = {
#include "sub_main/00497D90.inc"
};
char gEmTraderMsgs_35[96] = {
#include "sub_main/00497DE0.inc"
};
char gEmTraderMsgs_36[80] = {
#include "sub_main/00497E40.inc"
};
char gEmTraderMsgs_37[112] = {
#include "sub_main/00497E90.inc"
};
char gEmTraderMsgs_38[128] = {
#include "sub_main/00497F00.inc"
};
char gEmTraderMsgs_39[128] = {
#include "sub_main/00497F80.inc"
};
char gEmTraderMsgs_40[144] = {
#include "sub_main/00498000.inc"
};
char gEmTraderMsgs_41[144] = {
#include "sub_main/00498090.inc"
};
char gEmTraderMsgs_42[128] = {
#include "sub_main/00498120.inc"
};
char gEmTraderMsgs_43[128] = {
#include "sub_main/004981A0.inc"
};
char gEmTraderMsgs_44[176] = {
#include "sub_main/00498220.inc"
};
char gEmTraderMsgs_45[112] = {
#include "sub_main/004982D0.inc"
};
char gEmTraderMsgs_46[128] = {
#include "sub_main/00498340.inc"
};
char gEmTraderMsgs_47[112] = {
#include "sub_main/004983C0.inc"
};
char gEmTraderMsgs_48[112] = {
#include "sub_main/00498430.inc"
};
char gEmTraderMsgs_49[160] = {
#include "sub_main/004984A0.inc"
};
char gEmTraderMsgs_50[128] = {
#include "sub_main/00498540.inc"
};
char gEmTraderMsgs_51[112] = {
#include "sub_main/004985C0.inc"
};
char gEmTraderMsgs_52[160] = {
#include "sub_main/00498630.inc"
};
char gEmTraderMsgs_53[128] = {
#include "sub_main/004986D0.inc"
};
char gEmTraderMsgs_54[112] = {
#include "sub_main/00498750.inc"
};
char gEmTraderMsgs_55[144] = {
#include "sub_main/004987C0.inc"
};
char gEmTraderMsgs_56[112] = {
#include "sub_main/00498850.inc"
};
char gEmTraderMsgs_57[112] = {
#include "sub_main/004988C0.inc"
};
char gEmTraderMsgs_58[112] = {
#include "sub_main/00498930.inc"
};
char gEmTraderMsgs_59[96] = {
#include "sub_main/004989A0.inc"
};
char gEmTraderMsgs_60[112] = {
#include "sub_main/00498A00.inc"
};
char gEmTraderMsgs_61[112] = {
#include "sub_main/00498A70.inc"
};
char gEmTraderMsgs_62[128] = {
#include "sub_main/00498AE0.inc"
};
char gEmTraderMsgs_63[112] = {
#include "sub_main/00498B60.inc"
};
char gEmTraderMsgs_64[96] = {
#include "sub_main/00498BD0.inc"
};
char gEmTraderMsgs_65[96] = {
#include "sub_main/00498C30.inc"
};
char gEmTraderMsgs_66[96] = {
#include "sub_main/00498C90.inc"
};
char gEmTraderMsgs_67[128] = {
#include "sub_main/00498CF0.inc"
};
char gEmTraderMsgs_68[112] = {
#include "sub_main/00498D70.inc"
};
char gEmTraderMsgs_69[128] = {
#include "sub_main/00498DE0.inc"
};
char gEmTraderMsgs_70[144] = {
#include "sub_main/00498E60.inc"
};
char gEmTraderMsgs_71[96] = {
#include "sub_main/00498EF0.inc"
};
char gEmTraderMsgs_72[96] = {
#include "sub_main/00498F50.inc"
};
char gEmTraderMsgs_73[128] = {
#include "sub_main/00498FB0.inc"
};
char gEmTraderMsgs_74[112] = {
#include "sub_main/00499030.inc"
};
char gEmTraderMsgs_75[144] = {
#include "sub_main/004990A0.inc"
};
char gEmTraderMsgs_76[112] = {
#include "sub_main/00499130.inc"
};
char gEmTraderMsgs_77[112] = {
#include "sub_main/004991A0.inc"
};
char gEmTraderMsgs_78[144] = {
#include "sub_main/00499210.inc"
};
char gEmTraderMsgs_79[112] = {
#include "sub_main/004992A0.inc"
};
char gEmTraderMsgs_80[128] = {
#include "sub_main/00499310.inc"
};
char gEmTraderMsgs_81[112] = {
#include "sub_main/00499390.inc"
};
char gEmTraderMsgs_82[128] = {
#include "sub_main/00499400.inc"
};
char gEmTraderMsgs_83[96] = {
#include "sub_main/00499480.inc"
};
char gEmTraderMsgs_84[112] = {
#include "sub_main/004994E0.inc"
};
char gEmTraderMsgs_85[112] = {
#include "sub_main/00499550.inc"
};
char gEmTraderMsgs_86[112] = {
#include "sub_main/004995C0.inc"
};
char gEmTraderMsgs_87[112] = {
#include "sub_main/00499630.inc"
};
char gEmTraderMsgs_88[112] = {
#include "sub_main/004996A0.inc"
};
char gEmTraderMsgs_89[128] = {
#include "sub_main/00499710.inc"
};
char gEmTraderMsgs_90[80] = {
#include "sub_main/00499790.inc"
};
char gEmTraderMsgs_91[144] = {
#include "sub_main/004997E0.inc"
};
char gEmTraderMsgs_92[112] = {
#include "sub_main/00499870.inc"
};
char gEmTraderMsgs_93[96] = {
#include "sub_main/004998E0.inc"
};
char gEmTraderMsgs_94[32] = {
#include "sub_main/00499940.inc"
};
char gEmTraderMsgs_95[96] = {
#include "sub_main/00499960.inc"
};
char gEmTraderMsgs_96[32] = {
#include "sub_main/004999C0.inc"
};
char gEmTraderMsgs_97[112] = {
#include "sub_main/004999E0.inc"
};
char gEmTraderMsgs_98[48] = {
#include "sub_main/00499A50.inc"
};
char gEmTraderMsgs_99[64] = {
#include "sub_main/00499A80.inc"
};
char D_00499AC0[64] = {
#include "sub_main/00499AC0.inc"
};
char gEmTraderMsgs_102[96] = {
#include "sub_main/00499B00.inc"
};
char gEmTraderMsgs_103[112] = {
#include "sub_main/00499B60.inc"
};
char gEmTraderMsgs_104[128] = {
#include "sub_main/00499BD0.inc"
};
char gEmTraderMsgs_105[120] = {
#include "sub_main/00499C50.inc"
};
char gStageNames_0[8] = {
#include "sub_main/00499CC8.inc"
};
char D_00499CD0[8] = {
#include "sub_main/00499CD0.inc"
};
char D_00499CD8[8] = {
#include "sub_main/00499CD8.inc"
};
char D_00499CE0[8] = {
#include "sub_main/00499CE0.inc"
};
char D_00499CE8[8] = {
#include "sub_main/00499CE8.inc"
};
char D_00499CF0[8] = {
#include "sub_main/00499CF0.inc"
};
char D_00499CF8[8] = {
#include "sub_main/00499CF8.inc"
};
char gStageNames_7[16] = {
#include "sub_main/00499D00.inc"
};
char gStageNames_8[8] = {
#include "sub_main/00499D10.inc"
};
char D_00499D18[12] = {
#include "sub_main/00499D18.inc"
};
char D_00499D28[8] = {
#include "sub_main/00499D28.inc"
};
char D_00499D30[8] = {
#include "sub_main/00499D30.inc"
};
char D_00499D38[8] = {
#include "sub_main/00499D38.inc"
};
char D_00499D40[8] = {
#include "sub_main/00499D40.inc"
};
char D_00499D48[8] = {
#include "sub_main/00499D48.inc"
};
char D_00499D50[48] = {
#include "sub_main/00499D50.inc"
};
char gPlselHelpOnline_0[48] = {
#include "sub_main/00499D80.inc"
};
char D_00499DB0[8] = {
#include "sub_main/00499DB0.inc"
};
char D_00499DB8[8] = {
#include "sub_main/00499DB8.inc"
};
char D_00499DC0[16] = {
#include "sub_main/00499DC0.inc"
};
char D_00499DD0[8] = {
#include "sub_main/00499DD0.inc"
};
char D_00499DD8[8] = {
#include "sub_main/00499DD8.inc"
};
char D_00499DE0[8] = {
#include "sub_main/00499DE0.inc"
};
char D_00499DE8[8] = {
#include "sub_main/00499DE8.inc"
};
char D_00499DF0[8] = {
#include "sub_main/00499DF0.inc"
};
char D_00499DF8[8] = {
#include "sub_main/00499DF8.inc"
};
char D_00499E00[8] = {
#include "sub_main/00499E00.inc"
};
char D_00499E08[8] = {
#include "sub_main/00499E08.inc"
};
char D_00499E10[16] = {
#include "sub_main/00499E10.inc"
};
char D_00499E20[32] = {
#include "sub_main/00499E20.inc"
};
char D_00499E40[32] = {
#include "sub_main/00499E40.inc"
};
char D_00499E60[32] = {
#include "sub_main/00499E60.inc"
};
char gPlselStartPrompt_0[32] = {
#include "sub_main/00499E80.inc"
};
char D_00499EA0[32] = {
#include "sub_main/00499EA0.inc"
};
char D_00499EC0[48] = {
#include "sub_main/00499EC0.inc"
};
char gPlselPointRateFormat_0[88] = {
#include "sub_main/00499EF0.inc"
};
char D_00499F48[13] = {
#include "sub_main/00499F48.inc"
};
char D_00499F60[24] = {
#include "sub_main/00499F60.inc"
};
char D_00499F78[8] = {
#include "sub_main/00499F78.inc"
};
char gPlselWaitLine1_0[32] = {
#include "sub_main/00499F80.inc"
};
char gPlselWaitLine2_0[32] = {
#include "sub_main/00499FA0.inc"
};
char D_00499FC0[16] = {
#include "sub_main/00499FC0.inc"
};
char D_00499FD0[16] = {
#include "sub_main/00499FD0.inc"
};
char D_00499FE0[64] = {
#include "sub_main/00499FE0.inc"
};
char D_0049A020[64] = {
#include "sub_main/0049A020.inc"
};
char D_0049A060[56] = {
#include "sub_main/0049A060.inc"
};
char D_0049A098[8] = {
#include "sub_main/0049A098.inc"
};
char gQuestKindNames_1[32] = {
#include "sub_main/0049A0A0.inc"
};
char gQuestKindNames_2[32] = {
#include "sub_main/0049A0C0.inc"
};
char gQuestKindNames_3[32] = {
#include "sub_main/0049A0E0.inc"
};
char gQuestKindNames_4[32] = {
#include "sub_main/0049A100.inc"
};
char gQuestKindNames_5[32] = {
#include "sub_main/0049A120.inc"
};
char gQuestKindNames_6[32] = {
#include "sub_main/0049A140.inc"
};
char gQuestKindNames_7[32] = {
#include "sub_main/0049A160.inc"
};
char gQuestKindNames_8[32] = {
#include "sub_main/0049A180.inc"
};
char gQuestKindNames_9[32] = {
#include "sub_main/0049A1A0.inc"
};
char gQuestKindNames_10[32] = {
#include "sub_main/0049A1C0.inc"
};
char gQuestKindNames_11[32] = {
#include "sub_main/0049A1E0.inc"
};
char gQuestKindNames_12[32] = {
#include "sub_main/0049A200.inc"
};
char gQuestKindNames_13[32] = {
#include "sub_main/0049A220.inc"
};
char gQuestKindNames_14[32] = {
#include "sub_main/0049A240.inc"
};
char gQuestKindNames_15[32] = {
#include "sub_main/0049A260.inc"
};
char gQuestKindNames_16[32] = {
#include "sub_main/0049A280.inc"
};
char gQuestKindNames_17[32] = {
#include "sub_main/0049A2A0.inc"
};
char gQuestKindNames_18[32] = {
#include "sub_main/0049A2C0.inc"
};
char gQuestKindNames_19[32] = {
#include "sub_main/0049A2E0.inc"
};
char gQuestKindNames_20[32] = {
#include "sub_main/0049A300.inc"
};
char gQuestKindNames_21[32] = {
#include "sub_main/0049A320.inc"
};
char gQuestKindNames_22[32] = {
#include "sub_main/0049A340.inc"
};
char gQuestKindNames_23[32] = {
#include "sub_main/0049A360.inc"
};
char gQuestKindNames_24[32] = {
#include "sub_main/0049A380.inc"
};
char gQuestKindNames_25[32] = {
#include "sub_main/0049A3A0.inc"
};
char gQuestKindNames_26[32] = {
#include "sub_main/0049A3C0.inc"
};
char gQuestKindNames_27[32] = {
#include "sub_main/0049A3E0.inc"
};
char gQuestKindNames_28[32] = {
#include "sub_main/0049A400.inc"
};
char gQuestKindNames_29[48] = {
#include "sub_main/0049A420.inc"
};
char gQuestKindNames_30[64] = {
#include "sub_main/0049A450.inc"
};
char gQuestKindNames_31[64] = {
#include "sub_main/0049A490.inc"
};
char gQuestKindNames_32[48] = {
#include "sub_main/0049A4D0.inc"
};
char gQuestKindNames_33[48] = {
#include "sub_main/0049A500.inc"
};
char gQuestKindNames_34[48] = {
#include "sub_main/0049A530.inc"
};
char gQuestKindNames_35[64] = {
#include "sub_main/0049A560.inc"
};
char gQuestKindNames_36[48] = {
#include "sub_main/0049A5A0.inc"
};
char gQuestKindNames_37[64] = {
#include "sub_main/0049A5D0.inc"
};
char gQuestKindNames_38[48] = {
#include "sub_main/0049A610.inc"
};
char gQuestKindNames_39[64] = {
#include "sub_main/0049A640.inc"
};
char gQuestKindNames_40[32] = {
#include "sub_main/0049A680.inc"
};
char gQuestKindNames_41[32] = {
#include "sub_main/0049A6A0.inc"
};
char gQuestKindNames_42[32] = {
#include "sub_main/0049A6C0.inc"
};
char gQuestKindNames_43[32] = {
#include "sub_main/0049A6E0.inc"
};
char gQuestKindNames_44[32] = {
#include "sub_main/0049A700.inc"
};
char gQuestKindNames_45[32] = {
#include "sub_main/0049A720.inc"
};
char gQuestKindNames_46[32] = {
#include "sub_main/0049A740.inc"
};
char gQuestKindNames_47[32] = {
#include "sub_main/0049A760.inc"
};
char gQuestKindNames_48[32] = {
#include "sub_main/0049A780.inc"
};
char gQuestKindNames_49[32] = {
#include "sub_main/0049A7A0.inc"
};
char gQuestKindNames_50[32] = {
#include "sub_main/0049A7C0.inc"
};
char gQuestKindNames_51[16] = {
#include "sub_main/0049A7E0.inc"
};
char gQuestKindNames_52[32] = {
#include "sub_main/0049A7F0.inc"
};
char gQuestKindNames_53[32] = {
#include "sub_main/0049A810.inc"
};
char gQuestKindNames_54[32] = {
#include "sub_main/0049A830.inc"
};
char gQuestKindNames_55[32] = {
#include "sub_main/0049A850.inc"
};
char gQuestKindNames_56[32] = {
#include "sub_main/0049A870.inc"
};
char gQuestKindNames_57[32] = {
#include "sub_main/0049A890.inc"
};
char gQuestKindNames_58[32] = {
#include "sub_main/0049A8B0.inc"
};
char gQuestKindNames_59[64] = {
#include "sub_main/0049A8D0.inc"
};
char gQuestKindNames_60[32] = {
#include "sub_main/0049A910.inc"
};
char gQuestKindNames_61[32] = {
#include "sub_main/0049A930.inc"
};
char gQuestKindNames_62[32] = {
#include "sub_main/0049A950.inc"
};
char gQuestKindNames_63[48] = {
#include "sub_main/0049A970.inc"
};
char gQuestKindNames_64[64] = {
#include "sub_main/0049A9A0.inc"
};
char gQuestKindNames_65[32] = {
#include "sub_main/0049A9E0.inc"
};
char gQuestKindNames_66[32] = {
#include "sub_main/0049AA00.inc"
};
char gQuestKindNames_67[32] = {
#include "sub_main/0049AA20.inc"
};
char gQuestKindNames_68[32] = {
#include "sub_main/0049AA40.inc"
};
char gQuestKindNames_69[32] = {
#include "sub_main/0049AA60.inc"
};
char gQuestKindNames_70[32] = {
#include "sub_main/0049AA80.inc"
};
char gQuestKindNames_71[32] = {
#include "sub_main/0049AAA0.inc"
};
char gQuestKindNames_72[32] = {
#include "sub_main/0049AAC0.inc"
};
char gQuestKindNames_73[32] = {
#include "sub_main/0049AAE0.inc"
};
char gQuestKindNames_74[32] = {
#include "sub_main/0049AB00.inc"
};
char gQuestKindNames_75[32] = {
#include "sub_main/0049AB20.inc"
};
char gQuestKindNames_76[32] = {
#include "sub_main/0049AB40.inc"
};
char gQuestKindNames_77[32] = {
#include "sub_main/0049AB60.inc"
};
char gQuestKindNames_78[32] = {
#include "sub_main/0049AB80.inc"
};
char gQuestKindNames_79[48] = {
#include "sub_main/0049ABA0.inc"
};
char gQuestKindNames_80[48] = {
#include "sub_main/0049ABD0.inc"
};
char gQuestKindNames_81[48] = {
#include "sub_main/0049AC00.inc"
};
char gQuestKindNames_82[32] = {
#include "sub_main/0049AC30.inc"
};
char gQuestKindNames_83[32] = {
#include "sub_main/0049AC50.inc"
};
char gQuestKindNames_84[32] = {
#include "sub_main/0049AC70.inc"
};
char gQuestKindNames_85[32] = {
#include "sub_main/0049AC90.inc"
};
char gQuestKindNames_86[32] = {
#include "sub_main/0049ACB0.inc"
};
char gQuestKindNames_87[32] = {
#include "sub_main/0049ACD0.inc"
};
char gQuestKindNames_88[32] = {
#include "sub_main/0049ACF0.inc"
};
char gQuestKindNames_89[32] = {
#include "sub_main/0049AD10.inc"
};
char gQuestKindNames_90[32] = {
#include "sub_main/0049AD30.inc"
};
char gQuestKindNames_91[32] = {
#include "sub_main/0049AD50.inc"
};
char gQuestKindNames_92[32] = {
#include "sub_main/0049AD70.inc"
};
char gQuestKindNames_93[48] = {
#include "sub_main/0049AD90.inc"
};
char gQuestKindNames_94[48] = {
#include "sub_main/0049ADC0.inc"
};
char gSkillEffectNames_0[16] = {
#include "sub_main/0049ADF0.inc"
};
char gSkillEffectNames_1[16] = {
#include "sub_main/0049AE00.inc"
};
char gSkillEffectNames_2[16] = {
#include "sub_main/0049AE10.inc"
};
char gSkillEffectNames_3[16] = {
#include "sub_main/0049AE20.inc"
};
char gSkillEffectNames_4[16] = {
#include "sub_main/0049AE30.inc"
};
char gSkillEffectNames_5[16] = {
#include "sub_main/0049AE40.inc"
};
char gSkillEffectNames_6[16] = {
#include "sub_main/0049AE50.inc"
};
char gSkillEffectNames_7[16] = {
#include "sub_main/0049AE60.inc"
};
char gSkillEffectNames_8[16] = {
#include "sub_main/0049AE70.inc"
};
char gSkillEffectNames_9[16] = {
#include "sub_main/0049AE80.inc"
};
char gSkillEffectNames_10[8] = {
#include "sub_main/0049AE90.inc"
};
char gSkillEffectNames_11[8] = {
#include "sub_main/0049AE98.inc"
};
char gSkillEffectNames_12[8] = {
#include "sub_main/0049AEA0.inc"
};
char gSkillEffectNames_13[8] = {
#include "sub_main/0049AEA8.inc"
};
char gSkillEffectNames_14[8] = {
#include "sub_main/0049AEB0.inc"
};
char gSkillEffectNames_15[12] = {
#include "sub_main/0049AEB8.inc"
};
char gSkillEffectNames_16[8] = {
#include "sub_main/0049AEC8.inc"
};
char gSkillEffectNames_17[8] = {
#include "sub_main/0049AED0.inc"
};
char gSkillEffectNames_18[8] = {
#include "sub_main/0049AED8.inc"
};
char gSkillEffectNames_19[8] = {
#include "sub_main/0049AEE0.inc"
};
char gSkillEffectNames_20[8] = {
#include "sub_main/0049AEE8.inc"
};
char gSkillEffectNames_21[8] = {
#include "sub_main/0049AEF0.inc"
};
char gSkillEffectNames_22[8] = {
#include "sub_main/0049AEF8.inc"
};
char gSkillEffectNames_23[8] = {
#include "sub_main/0049AF00.inc"
};
char gSkillEffectNames_24[8] = {
#include "sub_main/0049AF08.inc"
};
char gSkillEffectNames_25[8] = {
#include "sub_main/0049AF10.inc"
};
char gSkillEffectNames_26[8] = {
#include "sub_main/0049AF18.inc"
};
char gSkillEffectNames_27[32] = {
#include "sub_main/0049AF20.inc"
};
char gSkillEffectNames_28[32] = {
#include "sub_main/0049AF40.inc"
};
char gSkillEffectNames_29[32] = {
#include "sub_main/0049AF60.inc"
};
char gSkillEffectNames_30[24] = {
#include "sub_main/0049AF80.inc"
};
char gSkillEffectNames_31[8] = {
#include "sub_main/0049AF98.inc"
};
char gSkillEffectNames_32[16] = {
#include "sub_main/0049AFA0.inc"
};
char gSkillEffectNames_33[16] = {
#include "sub_main/0049AFB0.inc"
};
char gSkillEffectNames_34[16] = {
#include "sub_main/0049AFC0.inc"
};
char gSkillEffectNames_35[8] = {
#include "sub_main/0049AFD0.inc"
};
char gSkillEffectNames_36[12] = {
#include "sub_main/0049AFD8.inc"
};
char gSkillEffectNames_37[12] = {
#include "sub_main/0049AFE8.inc"
};
char gSkillEffectNames_38[15] = {
#include "sub_main/0049AFF8.inc"
};
char gSkillEffectNames_39[13] = {
#include "sub_main/0049B008.inc"
};
char gSkillEffectNames_40[13] = {
#include "sub_main/0049B018.inc"
};
char gSkillEffectNames_41[13] = {
#include "sub_main/0049B028.inc"
};
char gSkillEffectNames_42[13] = {
#include "sub_main/0049B038.inc"
};
char gSkillEffectNames_43[13] = {
#include "sub_main/0049B048.inc"
};
char gSkillEffectNames_44[13] = {
#include "sub_main/0049B058.inc"
};
char gSkillEffectNames_45[12] = {
#include "sub_main/0049B068.inc"
};
char gSkillEffectNames_46[12] = {
#include "sub_main/0049B078.inc"
};
char gSkillEffectNames_47[12] = {
#include "sub_main/0049B088.inc"
};
char gSkillEffectNames_48[12] = {
#include "sub_main/0049B098.inc"
};
char gSkillEffectNames_49[12] = {
#include "sub_main/0049B0A8.inc"
};
char gSkillEffectNames_50[12] = {
#include "sub_main/0049B0B8.inc"
};
char D_0049B0C8[8] = {
#include "sub_main/0049B0C8.inc"
};
char gSkillEffectNames_52[16] = {
#include "sub_main/0049B0D0.inc"
};
char gSkillEffectNames_53[16] = {
#include "sub_main/0049B0E0.inc"
};
char gSkillEffectNames_54[16] = {
#include "sub_main/0049B0F0.inc"
};
char gSkillEffectNames_55[16] = {
#include "sub_main/0049B100.inc"
};
char gSkillEffectNames_56[16] = {
#include "sub_main/0049B110.inc"
};
char gSkillEffectNames_57[32] = {
#include "sub_main/0049B120.inc"
};
char gSkillEffectNames_58[32] = {
#include "sub_main/0049B140.inc"
};
char gSkillEffectNames_59[32] = {
#include "sub_main/0049B160.inc"
};
char gSkillEffectNames_60[32] = {
#include "sub_main/0049B180.inc"
};
char gSkillEffectNames_61[24] = {
#include "sub_main/0049B1A0.inc"
};
char gSkillEffectNames_62[15] = {
#include "sub_main/0049B1B8.inc"
};
char gSkillEffectNames_63[16] = {
#include "sub_main/0049B1D0.inc"
};
char gSkillEffectNames_64[32] = {
#include "sub_main/0049B1E0.inc"
};
char gSkillEffectNames_65[32] = {
#include "sub_main/0049B200.inc"
};
char gSkillEffectNames_66[32] = {
#include "sub_main/0049B220.inc"
};
char gSkillEffectNames_67[32] = {
#include "sub_main/0049B240.inc"
};
char gSkillEffectNames_68[32] = {
#include "sub_main/0049B260.inc"
};
char gSkillEffectNames_69[32] = {
#include "sub_main/0049B280.inc"
};
char gSkillEffectNames_70[32] = {
#include "sub_main/0049B2A0.inc"
};
char gSkillEffectNames_71[24] = {
#include "sub_main/0049B2C0.inc"
};
char gSkillEffectNames_72[8] = {
#include "sub_main/0049B2D8.inc"
};
char gSkillEffectNames_73[16] = {
#include "sub_main/0049B2E0.inc"
};
char gSkillEffectNames_74[16] = {
#include "sub_main/0049B2F0.inc"
};
char gSkillEffectNames_75[32] = {
#include "sub_main/0049B300.inc"
};
char gSkillEffectNames_76[24] = {
#include "sub_main/0049B320.inc"
};
char gSkillEffectNames_77[12] = {
#include "sub_main/0049B338.inc"
};
char gSkillEffectNames_78[12] = {
#include "sub_main/0049B348.inc"
};
char gSkillEffectNames_79[15] = {
#include "sub_main/0049B358.inc"
};
char gSkillEffectNames_80[15] = {
#include "sub_main/0049B368.inc"
};
char gSkillEffectNames_81[15] = {
#include "sub_main/0049B378.inc"
};
char gSkillEffectNames_82[8] = {
#include "sub_main/0049B388.inc"
};
char gSkillEffectNames_83[8] = {
#include "sub_main/0049B390.inc"
};
char gSkillEffectNames_84[8] = {
#include "sub_main/0049B398.inc"
};
char gSkillEffectNames_85[8] = {
#include "sub_main/0049B3A0.inc"
};
char gSkillEffectNames_86[8] = {
#include "sub_main/0049B3A8.inc"
};
char gSkillEffectNames_87[8] = {
#include "sub_main/0049B3B0.inc"
};
char gSkillEffectNames_88[12] = {
#include "sub_main/0049B3B8.inc"
};
char gSkillEffectNames_89[12] = {
#include "sub_main/0049B3C8.inc"
};
char gSkillEffectNames_90[8] = {
#include "sub_main/0049B3D8.inc"
};
char gSkillEffectNames_91[16] = {
#include "sub_main/0049B3E0.inc"
};
char D_0049B3F0[16] = {
#include "sub_main/0049B3F0.inc"
};
char gSkillEffectNames_93[16] = {
#include "sub_main/0049B400.inc"
};
char gSkillEffectNames_94[16] = {
#include "sub_main/0049B410.inc"
};
char gSkillEffectNames_95[16] = {
#include "sub_main/0049B420.inc"
};
char gSkillEffectNames_96[16] = {
#include "sub_main/0049B430.inc"
};
char gSkillEffectNames_97[16] = {
#include "sub_main/0049B440.inc"
};
char gSkillEffectNames_98[16] = {
#include "sub_main/0049B450.inc"
};
char gSkillEffectNames_99[16] = {
#include "sub_main/0049B460.inc"
};
char gSkillEffectNames_100[16] = {
#include "sub_main/0049B470.inc"
};
char gSkillEffectNames_101[16] = {
#include "sub_main/0049B480.inc"
};
char gSkillEffectNames_102[16] = {
#include "sub_main/0049B490.inc"
};
char gSkillEffectNames_103[16] = {
#include "sub_main/0049B4A0.inc"
};
char gSkillEffectNames_104[16] = {
#include "sub_main/0049B4B0.inc"
};
char gSkillEffectNames_105[16] = {
#include "sub_main/0049B4C0.inc"
};
char gSkillEffectNames_106[16] = {
#include "sub_main/0049B4D0.inc"
};
char gSkillEffectNames_107[16] = {
#include "sub_main/0049B4E0.inc"
};
char gSkillEffectNames_108[16] = {
#include "sub_main/0049B4F0.inc"
};
char gSkillEffectNames_109[16] = {
#include "sub_main/0049B500.inc"
};
char gSkillEffectNames_110[16] = {
#include "sub_main/0049B510.inc"
};
char gSkillEffectNames_111[16] = {
#include "sub_main/0049B520.inc"
};
char gSkillEffectNames_112[16] = {
#include "sub_main/0049B530.inc"
};
char gSkillEffectNames_113[16] = {
#include "sub_main/0049B540.inc"
};
char gSkillEffectNames_114[16] = {
#include "sub_main/0049B550.inc"
};
char gSkillEffectNames_115[16] = {
#include "sub_main/0049B560.inc"
};
char gSkillEffectNames_116[16] = {
#include "sub_main/0049B570.inc"
};
char gSkillEffectNames_117[16] = {
#include "sub_main/0049B580.inc"
};
char gSkillEffectNames_118[16] = {
#include "sub_main/0049B590.inc"
};
char gSkillEffectNames_119[16] = {
#include "sub_main/0049B5A0.inc"
};
char gSkillEffectNames_120[16] = {
#include "sub_main/0049B5B0.inc"
};
char gSkillEffectNames_121[16] = {
#include "sub_main/0049B5C0.inc"
};
char gSkillEffectNames_122[16] = {
#include "sub_main/0049B5D0.inc"
};
char gSkillEffectNames_123[16] = {
#include "sub_main/0049B5E0.inc"
};
char gSkillEffectNames_124[16] = {
#include "sub_main/0049B5F0.inc"
};
char gSkillEffectNames_125[16] = {
#include "sub_main/0049B600.inc"
};
char gSkillEffectNames_126[16] = {
#include "sub_main/0049B610.inc"
};
char gSkillEffectNames_127[16] = {
#include "sub_main/0049B620.inc"
};
char gSkillEffectNames_128[16] = {
#include "sub_main/0049B630.inc"
};
char gSkillEffectNames_129[16] = {
#include "sub_main/0049B640.inc"
};
char gSkillEffectNames_130[16] = {
#include "sub_main/0049B650.inc"
};
char gSkillEffectNames_131[16] = {
#include "sub_main/0049B660.inc"
};
char gSkillEffectNames_132[16] = {
#include "sub_main/0049B670.inc"
};
char gSkillEffectNames_133[16] = {
#include "sub_main/0049B680.inc"
};
char gSkillEffectNames_134[16] = {
#include "sub_main/0049B690.inc"
};
char gSkillEffectNames_135[16] = {
#include "sub_main/0049B6A0.inc"
};
char gSkillEffectNames_136[16] = {
#include "sub_main/0049B6B0.inc"
};
char gSkillEffectNames_137[16] = {
#include "sub_main/0049B6C0.inc"
};
char gSkillEffectNames_138[16] = {
#include "sub_main/0049B6D0.inc"
};
char gSkillEffectNames_139[16] = {
#include "sub_main/0049B6E0.inc"
};
char gSkillEffectNames_140[16] = {
#include "sub_main/0049B6F0.inc"
};
char gSkillEffectNames_141[16] = {
#include "sub_main/0049B700.inc"
};
char gSkillEffectNames_142[16] = {
#include "sub_main/0049B710.inc"
};
char gSkillEffectNames_143[16] = {
#include "sub_main/0049B720.inc"
};
char gSkillEffectNames_144[16] = {
#include "sub_main/0049B730.inc"
};
char gSkillEffectNames_145[16] = {
#include "sub_main/0049B740.inc"
};
char gSkillEffectNames_146[16] = {
#include "sub_main/0049B750.inc"
};
char gSkillEffectNames_147[8] = {
#include "sub_main/0049B760.inc"
};
char gSkillEffectNames_148[8] = {
#include "sub_main/0049B768.inc"
};
char gSkillEffectNames_149[8] = {
#include "sub_main/0049B770.inc"
};
char gSkillEffectNames_150[8] = {
#include "sub_main/0049B778.inc"
};
char gSkillEffectNames_151[24] = {
#include "sub_main/0049B780.inc"
};
char gSkillEffectNames_152[15] = {
#include "sub_main/0049B798.inc"
};
char gSkillEffectNames_153[13] = {
#include "sub_main/0049B7A8.inc"
};
char gSkillEffectNames_154[15] = {
#include "sub_main/0049B7B8.inc"
};
char gSkillEffectNames_155[15] = {
#include "sub_main/0049B7C8.inc"
};
char gSkillEffectNames_156[8] = {
#include "sub_main/0049B7D8.inc"
};
char gSkillEffectNames_157[8] = {
#include "sub_main/0049B7E0.inc"
};
char gSkillEffectNames_158[8] = {
#include "sub_main/0049B7E8.inc"
};
char gSkillEffectNames_159[8] = {
#include "sub_main/0049B7F0.inc"
};
char gSkillEffectNames_160[12] = {
#include "sub_main/0049B7F8.inc"
};
char gSkillEffectNames_161[8] = {
#include "sub_main/0049B808.inc"
};
char gSkillEffectNames_162[16] = {
#include "sub_main/0049B810.inc"
};
char gSkillEffectNames_163[32] = {
#include "sub_main/0049B820.inc"
};
char gSkillEffectNames_164[24] = {
#include "sub_main/0049B840.inc"
};
char gSkillEffectNames_165[14] = {
#include "sub_main/0049B858.inc"
};
char gSkillEffectNames_166[15] = {
#include "sub_main/0049B868.inc"
};
char gSkillEffectNames_167[15] = {
#include "sub_main/0049B878.inc"
};
char gSkillEffectNames_168[14] = {
#include "sub_main/0049B888.inc"
};
char gSkillEffectNames_169[15] = {
#include "sub_main/0049B898.inc"
};
char gSkillEffectNames_170[13] = {
#include "sub_main/0049B8A8.inc"
};
char D_0049B8B8[8] = {
#include "sub_main/0049B8B8.inc"
};
char D_0049B8C0[16] = {
#include "sub_main/0049B8C0.inc"
};
char gSkillEffectNames_173[16] = {
#include "sub_main/0049B8D0.inc"
};
char gSkillEffectNames_174[16] = {
#include "sub_main/0049B8E0.inc"
};
char gSkillEffectNames_175[16] = {
#include "sub_main/0049B8F0.inc"
};
char gSkillEffectNames_176[16] = {
#include "sub_main/0049B900.inc"
};
char gSkillEffectNames_177[16] = {
#include "sub_main/0049B910.inc"
};
char gSkillEffectNames_178[16] = {
#include "sub_main/0049B920.inc"
};
char gSkillEffectNames_179[16] = {
#include "sub_main/0049B930.inc"
};
char gSkillEffectNames_180[32] = {
#include "sub_main/0049B940.inc"
};
char gSkillEffectNames_181[24] = {
#include "sub_main/0049B960.inc"
};
char gSkillEffectNames_182[12] = {
#include "sub_main/0049B978.inc"
};
char gSkillEffectNames_183[8] = {
#include "sub_main/0049B988.inc"
};
char gSkillEffectNames_184[16] = {
#include "sub_main/0049B990.inc"
};
char gSkillEffectNames_185[16] = {
#include "sub_main/0049B9A0.inc"
};
char gSkillEffectNames_186[16] = {
#include "sub_main/0049B9B0.inc"
};
char gSkillEffectNames_187[16] = {
#include "sub_main/0049B9C0.inc"
};
char gSkillEffectNames_188[8] = {
#include "sub_main/0049B9D0.inc"
};
char gSkillEffectNames_189[13] = {
#include "sub_main/0049B9D8.inc"
};
char gSkillEffectNames_190[13] = {
#include "sub_main/0049B9E8.inc"
};
char gSkillEffectNames_191[32] = {
#include "sub_main/0049BA00.inc"
};
char gSkillEffectNames_192[32] = {
#include "sub_main/0049BA20.inc"
};
char gSkillEffectNames_193[32] = {
#include "sub_main/0049BA40.inc"
};
char gSkillEffectNames_194[24] = {
#include "sub_main/0049BA60.inc"
};
char D_0049BA78[8] = {
#include "sub_main/0049BA78.inc"
};
char gSkillEffectNames_196[16] = {
#include "sub_main/0049BA80.inc"
};
char gSkillEffectNames_197[16] = {
#include "sub_main/0049BA90.inc"
};
char gSkillEffectNames_198[16] = {
#include "sub_main/0049BAA0.inc"
};
char gSkillEffectNames_199[16] = {
#include "sub_main/0049BAB0.inc"
};
char gSkillEffectNames_200[16] = {
#include "sub_main/0049BAC0.inc"
};
char D_0049BAD0[8] = {
#include "sub_main/0049BAD0.inc"
};
char gSkillNames_1[8] = {
#include "sub_main/0049BAD8.inc"
};
char gSkillNames_2[8] = {
#include "sub_main/0049BAE0.inc"
};
char gSkillNames_3[8] = {
#include "sub_main/0049BAE8.inc"
};
char gSkillNames_4[16] = {
#include "sub_main/0049BAF0.inc"
};
char gSkillNames_5[16] = {
#include "sub_main/0049BB00.inc"
};
char gSkillNames_6[8] = {
#include "sub_main/0049BB10.inc"
};
char gSkillNames_7[12] = {
#include "sub_main/0049BB18.inc"
};
char gSkillNames_8[12] = {
#include "sub_main/0049BB28.inc"
};
char gSkillNames_9[8] = {
#include "sub_main/0049BB38.inc"
};
char gSkillNames_10[8] = {
#include "sub_main/0049BB40.inc"
};
char gSkillNames_11[12] = {
#include "sub_main/0049BB48.inc"
};
char gSkillNames_12[12] = {
#include "sub_main/0049BB58.inc"
};
char gSkillNames_13[8] = {
#include "sub_main/0049BB68.inc"
};
char gSkillNames_14[16] = {
#include "sub_main/0049BB70.inc"
};
char gSkillNames_15[8] = {
#include "sub_main/0049BB80.inc"
};
char D_0049BB88[12] = {
#include "sub_main/0049BB88.inc"
};
char gSkillNames_17[8] = {
#include "sub_main/0049BB98.inc"
};
char gSkillNames_18[8] = {
#include "sub_main/0049BBA0.inc"
};
char gSkillNames_19[12] = {
#include "sub_main/0049BBA8.inc"
};
char gSkillNames_20[12] = {
#include "sub_main/0049BBB8.inc"
};
char gSkillNames_21[8] = {
#include "sub_main/0049BBC8.inc"
};
char gSkillNames_22[8] = {
#include "sub_main/0049BBD0.inc"
};
char gSkillNames_23[12] = {
#include "sub_main/0049BBD8.inc"
};
char gSkillNames_24[12] = {
#include "sub_main/0049BBE8.inc"
};
char gSkillNames_26[12] = {
#include "sub_main/0049BBF8.inc"
};
char gSkillNames_27[8] = {
#include "sub_main/0049BC08.inc"
};
char gSkillNames_28[8] = {
#include "sub_main/0049BC10.inc"
};
char gSkillNames_29[8] = {
#include "sub_main/0049BC18.inc"
};
char gSkillNames_30[8] = {
#include "sub_main/0049BC20.inc"
};
char gSkillNames_31[8] = {
#include "sub_main/0049BC28.inc"
};
char gSkillNames_32[8] = {
#include "sub_main/0049BC30.inc"
};
char gSkillNames_33[12] = {
#include "sub_main/0049BC38.inc"
};
char gSkillNames_34[8] = {
#include "sub_main/0049BC48.inc"
};
char gSkillNames_35[8] = {
#include "sub_main/0049BC50.inc"
};
char gSkillNames_36[12] = {
#include "sub_main/0049BC58.inc"
};
char gSkillNames_37[12] = {
#include "sub_main/0049BC68.inc"
};
char gSkillNames_38[12] = {
#include "sub_main/0049BC78.inc"
};
char gSkillNames_39[12] = {
#include "sub_main/0049BC88.inc"
};
char gSkillNames_40[8] = {
#include "sub_main/0049BC98.inc"
};
char gSkillNames_41[16] = {
#include "sub_main/0049BCA0.inc"
};
char gSkillNames_42[16] = {
#include "sub_main/0049BCB0.inc"
};
char gSkillNames_44[8] = {
#include "sub_main/0049BCC0.inc"
};
char gSkillNames_45[8] = {
#include "sub_main/0049BCC8.inc"
};
char gSkillNames_47[16] = {
#include "sub_main/0049BCD0.inc"
};
char gSkillNames_48[8] = {
#include "sub_main/0049BCE0.inc"
};
char gSkillNames_50[12] = {
#include "sub_main/0049BCE8.inc"
};
char gSkillNames_51[12] = {
#include "sub_main/0049BCF8.inc"
};
char gSkillNames_52[8] = {
#include "sub_main/0049BD08.inc"
};
char gSkillNames_53[8] = {
#include "sub_main/0049BD10.inc"
};
char gSkillNames_54[8] = {
#include "sub_main/0049BD18.inc"
};
char gSkillNames_55[8] = {
#include "sub_main/0049BD20.inc"
};
char gSkillNames_56[8] = {
#include "sub_main/0049BD28.inc"
};
char gSkillNames_58[8] = {
#include "sub_main/0049BD30.inc"
};
char gSkillNames_59[8] = {
#include "sub_main/0049BD38.inc"
};
char gSkillNames_60[16] = {
#include "sub_main/0049BD40.inc"
};
char gSkillNames_63[16] = {
#include "sub_main/0049BD50.inc"
};
char gSkillNames_64[16] = {
#include "sub_main/0049BD60.inc"
};
char gSkillNames_65[8] = {
#include "sub_main/0049BD70.inc"
};
char gSkillNames_66[12] = {
#include "sub_main/0049BD78.inc"
};
char gSkillNames_67[12] = {
#include "sub_main/0049BD88.inc"
};
char gSkillNames_69[12] = {
#include "sub_main/0049BD98.inc"
};
char gSkillNames_70[12] = {
#include "sub_main/0049BDA8.inc"
};
char gSkillNames_71[8] = {
#include "sub_main/0049BDB8.inc"
};
char gSkillNames_72[16] = {
#include "sub_main/0049BDC0.inc"
};
char gSkillNames_73[8] = {
#include "sub_main/0049BDD0.inc"
};
char gSkillNames_74[12] = {
#include "sub_main/0049BDD8.inc"
};
char gSkillNames_75[8] = {
#include "sub_main/0049BDE8.inc"
};
char gSkillNames_76[8] = {
#include "sub_main/0049BDF0.inc"
};
char gSkillNames_78[8] = {
#include "sub_main/0049BDF8.inc"
};
char gSkillNames_79[8] = {
#include "sub_main/0049BE00.inc"
};
char gSkillNames_80[8] = {
#include "sub_main/0049BE08.inc"
};
char gSkillNames_82[8] = {
#include "sub_main/0049BE10.inc"
};
char gSkillNames_83[8] = {
#include "sub_main/0049BE18.inc"
};
char D_0049BE20[16] = {
#include "sub_main/0049BE20.inc"
};
char D_0049BE30[64] = {
#include "sub_main/0049BE30.inc"
};
char D_0049BE70[32] = {
#include "sub_main/0049BE70.inc"
};
char D_0049BE90[48] = {
#include "sub_main/0049BE90.inc"
};
char D_0049BEC0[48] = {
#include "sub_main/0049BEC0.inc"
};
char gHelpTexts_0_0[32] = {
#include "sub_main/0049BEF0.inc"
};
char gHelpTexts_0_1[32] = {
#include "sub_main/0049BF10.inc"
};
char gHelpTexts_0_2[32] = {
#include "sub_main/0049BF30.inc"
};
char gHelpTexts_0_3[32] = {
#include "sub_main/0049BF50.inc"
};
char gHelpTexts_0_4[32] = {
#include "sub_main/0049BF70.inc"
};
char gHelpTexts_0_5[48] = {
#include "sub_main/0049BF90.inc"
};
char gHelpTexts_0_6[48] = {
#include "sub_main/0049BFC0.inc"
};
char D_0049BFF0[48] = {
#include "sub_main/0049BFF0.inc"
};
char gHelpTexts_0_8[48] = {
#include "sub_main/0049C020.inc"
};
char gHelpTexts_0_9[48] = {
#include "sub_main/0049C050.inc"
};
char gHelpTexts_0_10[48] = {
#include "sub_main/0049C080.inc"
};
char D_0049C0B0[32] = {
#include "sub_main/0049C0B0.inc"
};
char D_0049C0D0[32] = {
#include "sub_main/0049C0D0.inc"
};
char gHelpTexts_3_0[32] = {
#include "sub_main/0049C0F0.inc"
};
char gHelpTexts_3_1[48] = {
#include "sub_main/0049C110.inc"
};
char D_0049C140[48] = {
#include "sub_main/0049C140.inc"
};
char D_0049C170[32] = {
#include "sub_main/0049C170.inc"
};
char D_0049C190[48] = {
#include "sub_main/0049C190.inc"
};
char D_0049C1C0[32] = {
#include "sub_main/0049C1C0.inc"
};
char D_0049C1E0[64] = {
#include "sub_main/0049C1E0.inc"
};
char D_0049C220[64] = {
#include "sub_main/0049C220.inc"
};
char D_0049C260[48] = {
#include "sub_main/0049C260.inc"
};
char D_0049C290[64] = {
#include "sub_main/0049C290.inc"
};
char D_0049C2D0[48] = {
#include "sub_main/0049C2D0.inc"
};
char gHelpTexts_9_0[48] = {
#include "sub_main/0049C300.inc"
};
char gHelpTexts_9_2[48] = {
#include "sub_main/0049C330.inc"
};
char gHelpTexts_9_3[64] = {
#include "sub_main/0049C360.inc"
};
char gHelpTexts_9_4[64] = {
#include "sub_main/0049C3A0.inc"
};
char gHelpTexts_9_5[64] = {
#include "sub_main/0049C3E0.inc"
};
char gHelpTexts_9_6[64] = {
#include "sub_main/0049C420.inc"
};
char gHelpTexts_9_7[48] = {
#include "sub_main/0049C460.inc"
};
char gHelpTexts_9_8[48] = {
#include "sub_main/0049C490.inc"
};
char gHelpTexts_9_9[48] = {
#include "sub_main/0049C4C0.inc"
};
char gHelpTexts_9_10[48] = {
#include "sub_main/0049C4F0.inc"
};
char gHelpTexts_9_11[32] = {
#include "sub_main/0049C520.inc"
};
char gHelpTexts_9_12[48] = {
#include "sub_main/0049C540.inc"
};
char gHelpTexts_9_13[112] = {
#include "sub_main/0049C570.inc"
};
char gHelpTexts_9_14[32] = {
#include "sub_main/0049C5E0.inc"
};
char gHelpTexts_9_15[112] = {
#include "sub_main/0049C600.inc"
};
char D_0049C670[32] = {
#include "sub_main/0049C670.inc"
};
char D_0049C690[16] = {
#include "sub_main/0049C690.inc"
};
char gHelpTexts_10_0[48] = {
#include "sub_main/0049C6A0.inc"
};
char gHelpTexts_10_1[64] = {
#include "sub_main/0049C6D0.inc"
};
char gHelpTexts_11_0[48] = {
#include "sub_main/0049C710.inc"
};
char gHelpTexts_11_1[32] = {
#include "sub_main/0049C740.inc"
};
char gHelpTexts_11_2[64] = {
#include "sub_main/0049C760.inc"
};
char gHelpTexts_11_3[48] = {
#include "sub_main/0049C7A0.inc"
};
char gHelpTexts_11_4[32] = {
#include "sub_main/0049C7D0.inc"
};
char gHelpTexts_11_5[64] = {
#include "sub_main/0049C7F0.inc"
};
char gHelpTexts_11_6[48] = {
#include "sub_main/0049C830.inc"
};
char D_0049C860[48] = {
#include "sub_main/0049C860.inc"
};
char D_0049C890[48] = {
#include "sub_main/0049C890.inc"
};
char D_0049C8C0[48] = {
#include "sub_main/0049C8C0.inc"
};
char gHelpTexts_12_0[80] = {
#include "sub_main/0049C8F0.inc"
};
char gHelpTexts_12_1[48] = {
#include "sub_main/0049C940.inc"
};
char gHelpTexts_12_2[48] = {
#include "sub_main/0049C970.inc"
};
char gHelpTexts_12_3[64] = {
#include "sub_main/0049C9A0.inc"
};
char gHelpTexts_12_4[48] = {
#include "sub_main/0049C9E0.inc"
};
char gHelpTexts_12_5[96] = {
#include "sub_main/0049CA10.inc"
};
char D_0049CA70[24] = {
#include "sub_main/0049CA70.inc"
};
char D_0049CA88[15] = {
#include "sub_main/0049CA88.inc"
};
char gSymbolColorNames_0[12] = {
#include "sub_main/0049CA98.inc"
};
char gSymbolColorNames_1[12] = {
#include "sub_main/0049CAA8.inc"
};
char gSymbolColorNames_2[12] = {
#include "sub_main/0049CAB8.inc"
};
char gSymbolColorNames_3[12] = {
#include "sub_main/0049CAC8.inc"
};
char gSymbolColorNames_4[12] = {
#include "sub_main/0049CAD8.inc"
};
char gSymbolColorNames_5[12] = {
#include "sub_main/0049CAE8.inc"
};
char gSymbolColorNames_6[12] = {
#include "sub_main/0049CAF8.inc"
};
char gSymbolColorNames_7[12] = {
#include "sub_main/0049CB08.inc"
};
char gSymbolColorNames_8[12] = {
#include "sub_main/0049CB18.inc"
};
char gSymbolColorNames_9[12] = {
#include "sub_main/0049CB28.inc"
};
char gSymbolColorNames_10[12] = {
#include "sub_main/0049CB38.inc"
};
char gSymbolColorNames_11[12] = {
#include "sub_main/0049CB48.inc"
};
char gSeasonNames_0[8] = {
#include "sub_main/0049CB58.inc"
};
char gSeasonNames_1[8] = {
#include "sub_main/0049CB60.inc"
};
char gSeasonNames_2[8] = {
#include "sub_main/0049CB68.inc"
};
char gDaytimeNames_0[8] = {
#include "sub_main/0049CB70.inc"
};
char gDaytimeNames_1[8] = {
#include "sub_main/0049CB78.inc"
};
char gHunterTitles_1[8] = {
#include "sub_main/0049CB80.inc"
};
char gHunterTitles_2[8] = {
#include "sub_main/0049CB88.inc"
};
char gHunterTitles_3[24] = {
#include "sub_main/0049CB90.inc"
};
char gHunterTitles_27[12] = {
#include "sub_main/0049CBA8.inc"
};
char gHunterTitles_29[12] = {
#include "sub_main/0049CBB8.inc"
};
char gHunterTitles_30[32] = {
#include "sub_main/0049CBD0.inc"
};
char gHunterTitles_31[24] = {
#include "sub_main/0049CBF0.inc"
};
char gHunterTitles_32[12] = {
#include "sub_main/0049CC08.inc"
};
char gHunterTitles_34[24] = {
#include "sub_main/0049CC20.inc"
};
char gHunterTitles_35[8] = {
#include "sub_main/0049CC38.inc"
};
char gHunterTitles_36[16] = {
#include "sub_main/0049CC40.inc"
};
char gHunterTitles_37[8] = {
#include "sub_main/0049CC50.inc"
};
char gHunterTitles_38[12] = {
#include "sub_main/0049CC58.inc"
};
char gHunterTitles_39[24] = {
#include "sub_main/0049CC70.inc"
};
char gHunterTitles_40[12] = {
#include "sub_main/0049CC88.inc"
};
char gHunterTitles_41[32] = {
#include "sub_main/0049CCA0.inc"
};
char gHunterTitles_42[32] = {
#include "sub_main/0049CCC0.inc"
};
char gHunterTitles_43[32] = {
#include "sub_main/0049CCE0.inc"
};
char gHunterTitles_44[24] = {
#include "sub_main/0049CD00.inc"
};
char gHunterTitles_45[15] = {
#include "sub_main/0049CD18.inc"
};
char gHunterTitles_46[32] = {
#include "sub_main/0049CD30.inc"
};
char gHunterTitles_47[24] = {
#include "sub_main/0049CD50.inc"
};
char gHunterTitles_48[15] = {
#include "sub_main/0049CD68.inc"
};
char gHunterTitles_49[24] = {
#include "sub_main/0049CD80.inc"
};
char gHunterTitles_50[15] = {
#include "sub_main/0049CD98.inc"
};
char gHunterTitles_51[32] = {
#include "sub_main/0049CDB0.inc"
};
char gHunterTitles_52[32] = {
#include "sub_main/0049CDD0.inc"
};
char gHunterTitles_53[32] = {
#include "sub_main/0049CDF0.inc"
};
char gHunterTitles_54[24] = {
#include "sub_main/0049CE10.inc"
};
char gHunterTitles_55[15] = {
#include "sub_main/0049CE28.inc"
};
char gHunterTitles_56[15] = {
#include "sub_main/0049CE38.inc"
};
char gHunterTitles_57[24] = {
#include "sub_main/0049CE50.inc"
};
char gHunterTitles_58[15] = {
#include "sub_main/0049CE68.inc"
};
char gHunterTitles_59[15] = {
#include "sub_main/0049CE78.inc"
};
char gHunterTitles_60[32] = {
#include "sub_main/0049CE90.inc"
};
char gHunterTitles_61[32] = {
#include "sub_main/0049CEB0.inc"
};
char gHunterTitles_62[32] = {
#include "sub_main/0049CED0.inc"
};
char gHunterTitles_63[24] = {
#include "sub_main/0049CEF0.inc"
};
char gHunterTitles_64[13] = {
#include "sub_main/0049CF08.inc"
};
char gHunterTitles_65[13] = {
#include "sub_main/0049CF18.inc"
};
char gHunterTitles_66[12] = {
#include "sub_main/0049CF28.inc"
};
char gHunterTitles_67[12] = {
#include "sub_main/0049CF38.inc"
};
char gHunterTitles_68[12] = {
#include "sub_main/0049CF48.inc"
};
char gHunterTitles_69[12] = {
#include "sub_main/0049CF58.inc"
};
char gHunterTitles_70[12] = {
#include "sub_main/0049CF68.inc"
};
char gHunterTitles_71[12] = {
#include "sub_main/0049CF78.inc"
};
char gHunterTitles_72[12] = {
#include "sub_main/0049CF88.inc"
};
char gHunterTitles_73[12] = {
#include "sub_main/0049CF98.inc"
};
char gHunterTitles_74[12] = {
#include "sub_main/0049CFA8.inc"
};
char gHunterTitles_75[12] = {
#include "sub_main/0049CFB8.inc"
};
char gHunterTitles_76[12] = {
#include "sub_main/0049CFC8.inc"
};
char gHunterTitles_77[12] = {
#include "sub_main/0049CFD8.inc"
};
char gHunterTitles_78[12] = {
#include "sub_main/0049CFE8.inc"
};
char gHunterTitles_79[12] = {
#include "sub_main/0049CFF8.inc"
};
char gHunterTitles_80[12] = {
#include "sub_main/0049D008.inc"
};
char gHunterTitles_81[12] = {
#include "sub_main/0049D018.inc"
};
char gHunterTitles_82[12] = {
#include "sub_main/0049D028.inc"
};
char gHunterTitles_83[12] = {
#include "sub_main/0049D038.inc"
};
char gHunterTitles_84[12] = {
#include "sub_main/0049D048.inc"
};
char gHunterTitles_85[12] = {
#include "sub_main/0049D058.inc"
};
char gHunterTitles_86[12] = {
#include "sub_main/0049D068.inc"
};
char gHunterTitles_87[12] = {
#include "sub_main/0049D078.inc"
};
char gHunterTitles_88[12] = {
#include "sub_main/0049D088.inc"
};
char gHunterTitles_89[12] = {
#include "sub_main/0049D098.inc"
};
char gHunterTitles_90[12] = {
#include "sub_main/0049D0A8.inc"
};
char gHunterTitles_91[12] = {
#include "sub_main/0049D0B8.inc"
};
char gHunterTitles_92[12] = {
#include "sub_main/0049D0C8.inc"
};
char gHunterTitles_93[12] = {
#include "sub_main/0049D0D8.inc"
};
char gHunterTitles_94[12] = {
#include "sub_main/0049D0E8.inc"
};
char gHunterTitles_95[12] = {
#include "sub_main/0049D0F8.inc"
};
char gHunterTitles_96[12] = {
#include "sub_main/0049D108.inc"
};
char gHunterTitles_97[12] = {
#include "sub_main/0049D118.inc"
};
char gHunterTitles_98[12] = {
#include "sub_main/0049D128.inc"
};
char gHunterTitles_99[13] = {
#include "sub_main/0049D138.inc"
};
char gHunterTitles_100[13] = {
#include "sub_main/0049D148.inc"
};
char gHunterTitles_101[15] = {
#include "sub_main/0049D158.inc"
};
char gHunterTitles_102[24] = {
#include "sub_main/0049D170.inc"
};
char gHunterTitles_103[15] = {
#include "sub_main/0049D188.inc"
};
char gHunterTitles_104[24] = {
#include "sub_main/0049D1A0.inc"
};
char gHunterTitles_105[12] = {
#include "sub_main/0049D1B8.inc"
};
char gHunterTitles_106[8] = {
#include "sub_main/0049D1C8.inc"
};
char gHunterTitles_107[16] = {
#include "sub_main/0049D1D0.inc"
};
char gHunterTitles_108[24] = {
#include "sub_main/0049D1E0.inc"
};
char gHunterTitles_109[13] = {
#include "sub_main/0049D1F8.inc"
};
char gHunterTitles_110[12] = {
#include "sub_main/0049D208.inc"
};
char gHunterTitles_111[13] = {
#include "sub_main/0049D218.inc"
};
char gHunterTitles_112[15] = {
#include "sub_main/0049D228.inc"
};
char gHunterTitles_113[12] = {
#include "sub_main/0049D238.inc"
};
char gHunterTitles_114[32] = {
#include "sub_main/0049D250.inc"
};
char gHunterTitles_115[24] = {
#include "sub_main/0049D270.inc"
};
char gHunterTitles_116[12] = {
#include "sub_main/0049D288.inc"
};
char gHunterTitles_117[12] = {
#include "sub_main/0049D298.inc"
};
char gHunterTitles_118[24] = {
#include "sub_main/0049D2B0.inc"
};
char gHunterTitles_119[13] = {
#include "sub_main/0049D2C8.inc"
};
char gHunterTitles_121[32] = {
#include "sub_main/0049D2E0.inc"
};
char gHunterTitles_122[24] = {
#include "sub_main/0049D300.inc"
};
char gHunterTitles_123[12] = {
#include "sub_main/0049D318.inc"
};
char gHunterTitles_124[15] = {
#include "sub_main/0049D328.inc"
};
char gHunterTitles_126[32] = {
#include "sub_main/0049D340.inc"
};
char gHunterTitles_127[24] = {
#include "sub_main/0049D360.inc"
};
char gHunterTitles_128[13] = {
#include "sub_main/0049D378.inc"
};
char gHunterTitles_129[8] = {
#include "sub_main/0049D388.inc"
};
char gHunterTitles_130[16] = {
#include "sub_main/0049D390.inc"
};
char gHunterTitles_131[16] = {
#include "sub_main/0049D3A0.inc"
};
char gHunterTitles_132[16] = {
#include "sub_main/0049D3B0.inc"
};
char gHunterTitles_133[16] = {
#include "sub_main/0049D3C0.inc"
};
char gHunterTitles_134[16] = {
#include "sub_main/0049D3D0.inc"
};
char gHunterTitles_135[16] = {
#include "sub_main/0049D3E0.inc"
};
char gHunterTitles_136[32] = {
#include "sub_main/0049D3F0.inc"
};
char gHunterTitles_137[24] = {
#include "sub_main/0049D410.inc"
};
char gHunterTitles_138[12] = {
#include "sub_main/0049D428.inc"
};
char gHunterTitles_139[12] = {
#include "sub_main/0049D438.inc"
};
char gHunterTitles_140[12] = {
#include "sub_main/0049D448.inc"
};
char gHunterTitles_141[32] = {
#include "sub_main/0049D460.inc"
};
char gHunterTitles_142[16] = {
#include "sub_main/0049D480.inc"
};
char gHunterTitles_143[16] = {
#include "sub_main/0049D490.inc"
};
char gHunterTitles_144[24] = {
#include "sub_main/0049D4A0.inc"
};
char gHunterTitles_145[15] = {
#include "sub_main/0049D4B8.inc"
};
char gHunterTitles_146[12] = {
#include "sub_main/0049D4C8.inc"
};
char gHunterTitles_147[32] = {
#include "sub_main/0049D4E0.inc"
};
char gPriceLegendTexts_0[32] = {
#include "sub_main/0049D500.inc"
};
char gPriceLegendTexts_1[32] = {
#include "sub_main/0049D520.inc"
};
char gPriceLegendTexts_2[32] = {
#include "sub_main/0049D540.inc"
};
char D_0049D560[40] = {
#include "sub_main/0049D560.inc"
};
char D_0049D588[12] = {
#include "sub_main/0049D588.inc"
};
char D_0049D5A0[32] = {
#include "sub_main/0049D5A0.inc"
};
char D_0049D5C0[32] = {
#include "sub_main/0049D5C0.inc"
};
char D_0049D5E0[24] = {
#include "sub_main/0049D5E0.inc"
};
char D_0049D5F8[12] = {
#include "sub_main/0049D5F8.inc"
};
char D_0049D610[32] = {
#include "sub_main/0049D610.inc"
};
char D_0049D630[32] = {
#include "sub_main/0049D630.inc"
};
char D_0049D650[32] = {
#include "sub_main/0049D650.inc"
};
char D_0049D670[32] = {
#include "sub_main/0049D670.inc"
};
char D_0049D690[32] = {
#include "sub_main/0049D690.inc"
};
char D_0049D6B0[32] = {
#include "sub_main/0049D6B0.inc"
};
char D_0049D6D0[32] = {
#include "sub_main/0049D6D0.inc"
};
char D_0049D6F0[24] = {
#include "sub_main/0049D6F0.inc"
};
char D_0049D708[13] = {
#include "sub_main/0049D708.inc"
};
char D_0049D718[15] = {
#include "sub_main/0049D718.inc"
};
char D_0049D730[32] = {
#include "sub_main/0049D730.inc"
};
char D_0049D750[24] = {
#include "sub_main/0049D750.inc"
};
char D_0049D768[12] = {
#include "sub_main/0049D768.inc"
};
char D_0049D778[13] = {
#include "sub_main/0049D778.inc"
};
char D_0049D788[15] = {
#include "sub_main/0049D788.inc"
};
char gAwardGalleryTitles_0[24] = {
#include "sub_main/0049D7A0.inc"
};
char gAwardGalleryTitles_1[13] = {
#include "sub_main/0049D7B8.inc"
};
char gAwardGalleryTitles_2[24] = {
#include "sub_main/0049D7D0.inc"
};
char gAwardGalleryTitles_3[12] = {
#include "sub_main/0049D7E8.inc"
};
char gAwardGalleryTitles_4[15] = {
#include "sub_main/0049D7F8.inc"
};
char gAwardGalleryTitles_5[24] = {
#include "sub_main/0049D810.inc"
};
char gAwardGalleryTitles_6[13] = {
#include "sub_main/0049D828.inc"
};
char gAwardGalleryTitles_7[15] = {
#include "sub_main/0049D838.inc"
};
char gAwardGalleryTitles_8[12] = {
#include "sub_main/0049D848.inc"
};
char gAwardGalleryTitles_9[32] = {
#include "sub_main/0049D860.inc"
};
char gAwardGalleryTitles_10[24] = {
#include "sub_main/0049D880.inc"
};
char gAwardGalleryTitles_11[12] = {
#include "sub_main/0049D898.inc"
};
char gAwardGalleryTitles_12[12] = {
#include "sub_main/0049D8A8.inc"
};
char gAwardGalleryTitles_13[12] = {
#include "sub_main/0049D8B8.inc"
};
char gAwardGalleryTitles_14[24] = {
#include "sub_main/0049D8D0.inc"
};
char gAwardGalleryTitles_15[12] = {
#include "sub_main/0049D8E8.inc"
};
char gAwardGalleryTitles_16[13] = {
#include "sub_main/0049D8F8.inc"
};
char gAwardGalleryTitles_17[12] = {
#include "sub_main/0049D908.inc"
};
char gAwardGalleryTitles_18[13] = {
#include "sub_main/0049D918.inc"
};
char gAwardGalleryTitles_19[12] = {
#include "sub_main/0049D928.inc"
};
char gAwardGalleryTitles_20[8] = {
#include "sub_main/0049D938.inc"
};
char gAwardGalleryTitles_21[16] = {
#include "sub_main/0049D940.inc"
};
char gAwardGalleryTitles_22[16] = {
#include "sub_main/0049D950.inc"
};
char gAwardGalleryTitles_23[16] = {
#include "sub_main/0049D960.inc"
};
char gAwardGalleryTitles_24[16] = {
#include "sub_main/0049D970.inc"
};
char gAwardGalleryTitles_25[16] = {
#include "sub_main/0049D980.inc"
};
char gAwardGalleryTitles_26[16] = {
#include "sub_main/0049D990.inc"
};
char gAwardGalleryTitles_27[16] = {
#include "sub_main/0049D9A0.inc"
};
char gAwardGalleryTitles_28[16] = {
#include "sub_main/0049D9B0.inc"
};
char gAwardGalleryTitles_29[16] = {
#include "sub_main/0049D9C0.inc"
};
char gAwardGalleryTitles_30[16] = {
#include "sub_main/0049D9D0.inc"
};
char gAwardGalleryTitles_31[16] = {
#include "sub_main/0049D9E0.inc"
};
char gAwardGalleryTitles_32[16] = {
#include "sub_main/0049D9F0.inc"
};
char gAwardDojoTitles_0[32] = {
#include "sub_main/0049DA00.inc"
};
char D_0049DA20[32] = {
#include "sub_main/0049DA20.inc"
};
char D_0049DA40[32] = {
#include "sub_main/0049DA40.inc"
};
char gAwardDojoTitles_3[32] = {
#include "sub_main/0049DA60.inc"
};
char gAwardDojoTitles_4[32] = {
#include "sub_main/0049DA80.inc"
};
char gAwardDojoTitles_7[32] = {
#include "sub_main/0049DAA0.inc"
};
char D_0049DAC0[32] = {
#include "sub_main/0049DAC0.inc"
};
char D_0049DAE0[32] = {
#include "sub_main/0049DAE0.inc"
};
char gArenaEmReleasedText_0[32] = {
#include "sub_main/0049DB00.inc"
};
char D_0049DB20[16] = {
#include "sub_main/0049DB20.inc"
};
char D_0049DB30[16] = {
#include "sub_main/0049DB30.inc"
};
char D_0049DB40[16] = {
#include "sub_main/0049DB40.inc"
};
char D_0049DB50[16] = {
#include "sub_main/0049DB50.inc"
};
char D_0049DB60[32] = {
#include "sub_main/0049DB60.inc"
};
char gArenaEmPenHelp_1[48] = {
#include "sub_main/0049DB80.inc"
};
char gArenaEmPenHelp_2[64] = {
#include "sub_main/0049DBB0.inc"
};
char gArenaEmPenHelp_3[48] = {
#include "sub_main/0049DBF0.inc"
};
char gArenaEmPenHelp_4[32] = {
#include "sub_main/0049DC20.inc"
};
char gArenaEmPenHelp_5[32] = {
#include "sub_main/0049DC40.inc"
};
char gArenaEmPenHelp_6[32] = {
#include "sub_main/0049DC60.inc"
};
char gArenaEmTameNames_0[16] = {
#include "sub_main/0049DC80.inc"
};
char gArenaEmTameNames_1[16] = {
#include "sub_main/0049DC90.inc"
};
char gArenaEmTameNames_2[16] = {
#include "sub_main/0049DCA0.inc"
};
char gArenaEmTameNames_3[16] = {
#include "sub_main/0049DCB0.inc"
};
char gArenaEmTameNames_4[16] = {
#include "sub_main/0049DCC0.inc"
};
char D_0049DCD0[8] = {
#include "sub_main/0049DCD0.inc"
};
char D_0049DCD8[8] = {
#include "sub_main/0049DCD8.inc"
};
char D_0049DCE0[8] = {
#include "sub_main/0049DCE0.inc"
};
char D_0049DCE8[8] = {
#include "sub_main/0049DCE8.inc"
};
char gArenaEmRankNames_0[8] = {
#include "sub_main/0049DCF0.inc"
};
char gArenaEmRankNames_1[12] = {
#include "sub_main/0049DCF8.inc"
};
char gArenaEmRankNames_2[12] = {
#include "sub_main/0049DD08.inc"
};
char gArenaEmRankNames_3[8] = {
#include "sub_main/0049DD18.inc"
};
char gArenaEmRankNames_5[16] = {
#include "sub_main/0049DD20.inc"
};
char gArenaEmRankNames_6[8] = {
#include "sub_main/0049DD30.inc"
};
char gArenaEmRankNames_7[12] = {
#include "sub_main/0049DD38.inc"
};
char gArenaEmRankNames_8[12] = {
#include "sub_main/0049DD48.inc"
};
char gArenaEmRankNames_9[8] = {
#include "sub_main/0049DD58.inc"
};
char D_0049DD60[16] = {
#include "sub_main/0049DD60.inc"
};
char D_0049DD70[32] = {
#include "sub_main/0049DD70.inc"
};
char D_0049DD90[32] = {
#include "sub_main/0049DD90.inc"
};
char D_0049DDB0[32] = {
#include "sub_main/0049DDB0.inc"
};
char gHornSongTexts_1[32] = {
#include "sub_main/0049DDD0.inc"
};
char gHornSongTexts_0[32] = {
#include "sub_main/0049DDF0.inc"
};
char gHornSongTexts_2[32] = {
#include "sub_main/0049DE10.inc"
};
char D_0049DE30[32] = {
#include "sub_main/0049DE30.inc"
};
char gHornSongTexts_4[24] = {
#include "sub_main/0049DE50.inc"
};
char D_0049DE68[15] = {
#include "sub_main/0049DE68.inc"
};
char gHornSongTexts_8[15] = {
#include "sub_main/0049DE78.inc"
};
char D_0049DE88[15] = {
#include "sub_main/0049DE88.inc"
};
char gHornSongTexts_12[12] = {
#include "sub_main/0049DE98.inc"
};
char D_0049DEA8[12] = {
#include "sub_main/0049DEA8.inc"
};
char D_0049DEB8[15] = {
#include "sub_main/0049DEB8.inc"
};
char D_0049DED0[32] = {
#include "sub_main/0049DED0.inc"
};
char D_0049DEF0[24] = {
#include "sub_main/0049DEF0.inc"
};
char D_0049DF08[15] = {
#include "sub_main/0049DF08.inc"
};
char D_0049DF18[15] = {
#include "sub_main/0049DF18.inc"
};
char gHornSongTexts_26[32] = {
#include "sub_main/0049DF30.inc"
};
char D_0049DF50[32] = {
#include "sub_main/0049DF50.inc"
};
char gHornSongTexts_28[32] = {
#include "sub_main/0049DF70.inc"
};
char gHornSongTexts_30[32] = {
#include "sub_main/0049DF90.inc"
};
char D_0049DFB0[32] = {
#include "sub_main/0049DFB0.inc"
};
char gHornSongTexts_32[32] = {
#include "sub_main/0049DFD0.inc"
};
char gHornSongTexts_34[32] = {
#include "sub_main/0049DFF0.inc"
};
char D_0049E010[32] = {
#include "sub_main/0049E010.inc"
};
char gHornSongTexts_36[32] = {
#include "sub_main/0049E030.inc"
};
char gHornSongTexts_38[32] = {
#include "sub_main/0049E050.inc"
};
char D_0049E070[32] = {
#include "sub_main/0049E070.inc"
};
char gHornSongTexts_40[32] = {
#include "sub_main/0049E090.inc"
};
char gHornSongTexts_42[32] = {
#include "sub_main/0049E0B0.inc"
};
char D_0049E0D0[32] = {
#include "sub_main/0049E0D0.inc"
};
char gHornSongTexts_44[24] = {
#include "sub_main/0049E0F0.inc"
};
char gHornSongTexts_46[12] = {
#include "sub_main/0049E108.inc"
};
char gHornSongTexts_47[32] = {
#include "sub_main/0049E120.inc"
};
char D_0049E140[32] = {
#include "sub_main/0049E140.inc"
};
char D_0049E160[48] = {
#include "sub_main/0049E160.inc"
};
char D_0049E190[48] = {
#include "sub_main/0049E190.inc"
};
char D_0049E1C0[48] = {
#include "sub_main/0049E1C0.inc"
};
char gHunterTitleDescs_0[32] = {
#include "sub_main/0049E1F0.inc"
};
char gHunterTitleDescs_1[48] = {
#include "sub_main/0049E210.inc"
};
char gHunterTitleDescs_2[48] = {
#include "sub_main/0049E240.inc"
};
char gHunterTitleDescs_3[48] = {
#include "sub_main/0049E270.inc"
};
char gHunterTitleDescs_4[48] = {
#include "sub_main/0049E2A0.inc"
};
char gHunterTitleDescs_5[80] = {
#include "sub_main/0049E2D0.inc"
};
char gHunterTitleDescs_6[48] = {
#include "sub_main/0049E320.inc"
};
char gHunterTitleDescs_7[48] = {
#include "sub_main/0049E350.inc"
};
char gHunterTitleDescs_8[80] = {
#include "sub_main/0049E380.inc"
};
char gHunterTitleDescs_9[48] = {
#include "sub_main/0049E3D0.inc"
};
char gHunterTitleDescs_10[48] = {
#include "sub_main/0049E400.inc"
};
char gHunterTitleDescs_11[112] = {
#include "sub_main/0049E430.inc"
};
char gHunterTitleDescs_12[48] = {
#include "sub_main/0049E4A0.inc"
};
char gHunterTitleDescs_13[48] = {
#include "sub_main/0049E4D0.inc"
};
char gHunterTitleDescs_14[48] = {
#include "sub_main/0049E500.inc"
};
char gHunterTitleDescs_15[80] = {
#include "sub_main/0049E530.inc"
};
char gHunterTitleDescs_16[48] = {
#include "sub_main/0049E580.inc"
};
char gHunterTitleDescs_17[48] = {
#include "sub_main/0049E5B0.inc"
};
char gHunterTitleDescs_18[48] = {
#include "sub_main/0049E5E0.inc"
};
char gHunterTitleDescs_19[48] = {
#include "sub_main/0049E610.inc"
};
char gHunterTitleDescs_20[80] = {
#include "sub_main/0049E640.inc"
};
char gHunterTitleDescs_21[48] = {
#include "sub_main/0049E690.inc"
};
char gHunterTitleDescs_22[48] = {
#include "sub_main/0049E6C0.inc"
};
char gHunterTitleDescs_23[48] = {
#include "sub_main/0049E6F0.inc"
};
char gHunterTitleDescs_24[48] = {
#include "sub_main/0049E720.inc"
};
char gHunterTitleDescs_25[80] = {
#include "sub_main/0049E750.inc"
};
char gHunterTitleDescs_26[80] = {
#include "sub_main/0049E7A0.inc"
};
char gHunterTitleDescs_27[64] = {
#include "sub_main/0049E7F0.inc"
};
char gHunterTitleDescs_28[64] = {
#include "sub_main/0049E830.inc"
};
char gHunterTitleDescs_29[64] = {
#include "sub_main/0049E870.inc"
};
char gHunterTitleDescs_30[64] = {
#include "sub_main/0049E8B0.inc"
};
char gHunterTitleDescs_31[32] = {
#include "sub_main/0049E8F0.inc"
};
char gHunterTitleDescs_32[48] = {
#include "sub_main/0049E910.inc"
};
char gHunterTitleDescs_33[48] = {
#include "sub_main/0049E940.inc"
};
char gHunterTitleDescs_34[64] = {
#include "sub_main/0049E970.inc"
};
char gHunterTitleDescs_35[64] = {
#include "sub_main/0049E9B0.inc"
};
char gHunterTitleDescs_36[48] = {
#include "sub_main/0049E9F0.inc"
};
char gHunterTitleDescs_37[32] = {
#include "sub_main/0049EA20.inc"
};
char gHunterTitleDescs_38[32] = {
#include "sub_main/0049EA40.inc"
};
char gHunterTitleDescs_39[32] = {
#include "sub_main/0049EA60.inc"
};
char gHunterTitleDescs_40[32] = {
#include "sub_main/0049EA80.inc"
};
char gHunterTitleDescs_41[32] = {
#include "sub_main/0049EAA0.inc"
};
char gHunterTitleDescs_42[32] = {
#include "sub_main/0049EAC0.inc"
};
char gHunterTitleDescs_43[32] = {
#include "sub_main/0049EAE0.inc"
};
char gHunterTitleDescs_44[32] = {
#include "sub_main/0049EB00.inc"
};
char gHunterTitleDescs_45[32] = {
#include "sub_main/0049EB20.inc"
};
char gHunterTitleDescs_46[32] = {
#include "sub_main/0049EB40.inc"
};
char gHunterTitleDescs_47[32] = {
#include "sub_main/0049EB60.inc"
};
char gHunterTitleDescs_48[32] = {
#include "sub_main/0049EB80.inc"
};
char gHunterTitleDescs_49[32] = {
#include "sub_main/0049EBA0.inc"
};
char gHunterTitleDescs_50[32] = {
#include "sub_main/0049EBC0.inc"
};
char gHunterTitleDescs_51[32] = {
#include "sub_main/0049EBE0.inc"
};
char gHunterTitleDescs_52[32] = {
#include "sub_main/0049EC00.inc"
};
char gHunterTitleDescs_53[32] = {
#include "sub_main/0049EC20.inc"
};
char gHunterTitleDescs_54[32] = {
#include "sub_main/0049EC40.inc"
};
char gHunterTitleDescs_55[32] = {
#include "sub_main/0049EC60.inc"
};
char gHunterTitleDescs_56[32] = {
#include "sub_main/0049EC80.inc"
};
char gHunterTitleDescs_57[32] = {
#include "sub_main/0049ECA0.inc"
};
char gHunterTitleDescs_58[32] = {
#include "sub_main/0049ECC0.inc"
};
char gHunterTitleDescs_59[32] = {
#include "sub_main/0049ECE0.inc"
};
char gHunterTitleDescs_60[32] = {
#include "sub_main/0049ED00.inc"
};
char gHunterTitleDescs_61[32] = {
#include "sub_main/0049ED20.inc"
};
char gHunterTitleDescs_62[32] = {
#include "sub_main/0049ED40.inc"
};
char gHunterTitleDescs_63[32] = {
#include "sub_main/0049ED60.inc"
};
char gHunterTitleDescs_64[32] = {
#include "sub_main/0049ED80.inc"
};
char gHunterTitleDescs_65[32] = {
#include "sub_main/0049EDA0.inc"
};
char gHunterTitleDescs_66[32] = {
#include "sub_main/0049EDC0.inc"
};
char gHunterTitleDescs_67[32] = {
#include "sub_main/0049EDE0.inc"
};
char gHunterTitleDescs_68[32] = {
#include "sub_main/0049EE00.inc"
};
char gHunterTitleDescs_69[32] = {
#include "sub_main/0049EE20.inc"
};
char gHunterTitleDescs_70[32] = {
#include "sub_main/0049EE40.inc"
};
char gHunterTitleDescs_71[32] = {
#include "sub_main/0049EE60.inc"
};
char gHunterTitleDescs_72[32] = {
#include "sub_main/0049EE80.inc"
};
char gHunterTitleDescs_73[32] = {
#include "sub_main/0049EEA0.inc"
};
char gHunterTitleDescs_74[32] = {
#include "sub_main/0049EEC0.inc"
};
char gHunterTitleDescs_75[32] = {
#include "sub_main/0049EEE0.inc"
};
char gHunterTitleDescs_76[32] = {
#include "sub_main/0049EF00.inc"
};
char gHunterTitleDescs_77[32] = {
#include "sub_main/0049EF20.inc"
};
char gHunterTitleDescs_78[32] = {
#include "sub_main/0049EF40.inc"
};
char gHunterTitleDescs_79[32] = {
#include "sub_main/0049EF60.inc"
};
char gHunterTitleDescs_80[32] = {
#include "sub_main/0049EF80.inc"
};
char gHunterTitleDescs_81[32] = {
#include "sub_main/0049EFA0.inc"
};
char gHunterTitleDescs_82[32] = {
#include "sub_main/0049EFC0.inc"
};
char gHunterTitleDescs_83[32] = {
#include "sub_main/0049EFE0.inc"
};
char gHunterTitleDescs_84[32] = {
#include "sub_main/0049F000.inc"
};
char gHunterTitleDescs_85[32] = {
#include "sub_main/0049F020.inc"
};
char gHunterTitleDescs_86[32] = {
#include "sub_main/0049F040.inc"
};
char gHunterTitleDescs_87[32] = {
#include "sub_main/0049F060.inc"
};
char gHunterTitleDescs_88[32] = {
#include "sub_main/0049F080.inc"
};
char gHunterTitleDescs_89[32] = {
#include "sub_main/0049F0A0.inc"
};
char gHunterTitleDescs_90[32] = {
#include "sub_main/0049F0C0.inc"
};
char gHunterTitleDescs_91[32] = {
#include "sub_main/0049F0E0.inc"
};
char gHunterTitleDescs_92[32] = {
#include "sub_main/0049F100.inc"
};
char gHunterTitleDescs_93[32] = {
#include "sub_main/0049F120.inc"
};
char gHunterTitleDescs_94[32] = {
#include "sub_main/0049F140.inc"
};
char gHunterTitleDescs_95[64] = {
#include "sub_main/0049F160.inc"
};
char gHunterTitleDescs_96[48] = {
#include "sub_main/0049F1A0.inc"
};
char gHunterTitleDescs_97[48] = {
#include "sub_main/0049F1D0.inc"
};
char gHunterTitleDescs_98[48] = {
#include "sub_main/0049F200.inc"
};
char gHunterTitleDescs_99[48] = {
#include "sub_main/0049F230.inc"
};
char gHunterTitleDescs_100[48] = {
#include "sub_main/0049F260.inc"
};
char gHunterTitleDescs_101[48] = {
#include "sub_main/0049F290.inc"
};
char gHunterTitleDescs_102[64] = {
#include "sub_main/0049F2C0.inc"
};
char gHunterTitleDescs_103[64] = {
#include "sub_main/0049F300.inc"
};
char gHunterTitleDescs_104[64] = {
#include "sub_main/0049F340.inc"
};
char gHunterTitleDescs_105[64] = {
#include "sub_main/0049F380.inc"
};
char gHunterTitleDescs_106[48] = {
#include "sub_main/0049F3C0.inc"
};
char gHunterTitleDescs_107[64] = {
#include "sub_main/0049F3F0.inc"
};
char gHunterTitleDescs_108[64] = {
#include "sub_main/0049F430.inc"
};
char gHunterTitleDescs_109[48] = {
#include "sub_main/0049F470.inc"
};
char gHunterTitleDescs_110[80] = {
#include "sub_main/0049F4A0.inc"
};
char gHunterTitleDescs_111[80] = {
#include "sub_main/0049F4F0.inc"
};
char gHunterTitleDescs_112[48] = {
#include "sub_main/0049F540.inc"
};
char gHunterTitleDescs_113[48] = {
#include "sub_main/0049F570.inc"
};
char gHunterTitleDescs_114[64] = {
#include "sub_main/0049F5A0.inc"
};
char gHunterTitleDescs_115[64] = {
#include "sub_main/0049F5E0.inc"
};
char gHunterTitleDescs_116[96] = {
#include "sub_main/0049F620.inc"
};
char gHunterTitleDescs_117[64] = {
#include "sub_main/0049F680.inc"
};
char gHunterTitleDescs_118[64] = {
#include "sub_main/0049F6C0.inc"
};
char gHunterTitleDescs_119[48] = {
#include "sub_main/0049F700.inc"
};
char gHunterTitleDescs_120[48] = {
#include "sub_main/0049F730.inc"
};
char gHunterTitleDescs_121[48] = {
#include "sub_main/0049F760.inc"
};
char gHunterTitleDescs_122[48] = {
#include "sub_main/0049F790.inc"
};
char gHunterTitleDescs_123[48] = {
#include "sub_main/0049F7C0.inc"
};
char gHunterTitleDescs_124[48] = {
#include "sub_main/0049F7F0.inc"
};
char gHunterTitleDescs_125[32] = {
#include "sub_main/0049F820.inc"
};
char gHunterTitleDescs_126[48] = {
#include "sub_main/0049F840.inc"
};
char gHunterTitleDescs_127[48] = {
#include "sub_main/0049F870.inc"
};
char gHunterTitleDescs_128[48] = {
#include "sub_main/0049F8A0.inc"
};
char gHunterTitleDescs_129[48] = {
#include "sub_main/0049F8D0.inc"
};
char gHunterTitleDescs_130[48] = {
#include "sub_main/0049F900.inc"
};
char gHunterTitleDescs_131[48] = {
#include "sub_main/0049F930.inc"
};
char gHunterTitleDescs_132[48] = {
#include "sub_main/0049F960.inc"
};
char gHunterTitleDescs_133[48] = {
#include "sub_main/0049F990.inc"
};
char gHunterTitleDescs_134[48] = {
#include "sub_main/0049F9C0.inc"
};
char gHunterTitleDescs_135[48] = {
#include "sub_main/0049F9F0.inc"
};
char gHunterTitleDescs_136[64] = {
#include "sub_main/0049FA20.inc"
};
char gHunterTitleDescs_137[32] = {
#include "sub_main/0049FA60.inc"
};
char gHunterTitleDescs_138[32] = {
#include "sub_main/0049FA80.inc"
};
char gHunterTitleDescs_139[48] = {
#include "sub_main/0049FAA0.inc"
};
char gHunterTitleDescs_140[48] = {
#include "sub_main/0049FAD0.inc"
};
char gHunterTitleDescs_141[48] = {
#include "sub_main/0049FB00.inc"
};
char gHunterTitleDescs_142[64] = {
#include "sub_main/0049FB30.inc"
};
char gHunterTitleDescs_143[40] = {
#include "sub_main/0049FB70.inc"
};
#pragma rodata off
extern f32 *gAreaBlockSpheres[];       /* per area: the spheres that block the camera's line, -1.0f ends */
extern f32 gSnowLayerSpeedsState0[];
extern f32 gSnowLayerSpeedsState1[];
extern f32 gSnowLayerSpeedsState2[];
extern s16 *gSwampLayerPeriodsByState[];

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
        if (state->look[0] >= 255) {
            return;
        }
        Gs_SetState(RS_COLOR2, ((255 - state->look[0]) << 24) | 0xFFFFFF);
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
                if (state->propTimer > 30) {
                    Mtx_RotateX(mtx[0], 6.2831855f * (-2.0f * Math_Sin(6.2831855f * ((state->propTimer - 30) << 8) / 65536.0f)) / 360.0f);
                }
                break;
            case 2:
                if (state->propTimer < 256) {
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
                blue = green = red = 255;
            } else if (state->look[0] == 255) {
                blue = green = red = 48;
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

            Gs_SetState(RS_COLOR2, ((u8)(alpha + 160) << 24) | 0xFFFFFF);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            break;
        }
        case PROP_ANIM_SLIDE:
            Vec_Copy(pos, param);
            if (!(state->frame & 0x100)) {
                if ((state->frame & 0xFF) < 128) {
                    pos[2] += 2600.0f * ((state->frame & 0x7F) / 128.0f);
                } else {
                    pos[2] += 2600.0f;
                }
            } else if ((state->frame & 0xFF) < 128) {
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
            Gs_SetState(RS_COLOR2, ((255 - (state->look[0] >> 1)) << 24) | 0xFFFFFF);
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
            Tex_CountLoaded(264);
            Tex_Prepare(16, 264);
            Gs_SetState(RS_ALPHA_REF, 128);
            alphaRef = 128;
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
                                Gs_SetState(RS_COLOR2, ((255 - state->look[0]) << 24) | 0xFFFFFF);
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
                                        blue = green = red = 255;
                                    } else if (state->look[0] == 255) {
                                        blue = green = red = 48;
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
                                    Mtx_RotateY(mtx[0], (6.2831855f * (u32)*(u16 *)(arg + 16)) / 65536.0f);
                                    Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
                                    break;
                                }
                            case AREA_MODEL_ANIM_PULSE: {
                                    f32 pulse = 0.5f * (1.0f + Math_Sin((6.2831855f * (state->frame << 10)) / 65536.0f));
                                    u8 green = 32.0f * pulse;
                                    u8 blue = 64.0f * pulse;

                                    Gs_SetState(RS_COLOR2, (((u8)(green + 192)) << 8) | 0xFFFF0000 | (u8)(blue + 160));
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
                    if (alphaRef != 128) {
                        Gs_SetState(RS_ALPHA_REF, 128);
                        alphaRef = 128;
                    }
                    Model_Draw(entry->id, 0);
                }
                if (k == 0) {
                    Gs_SetState(RS_ZWRITE, 1);
                    Gs_ClearColor(0, 0);
                    Gs_SetState(RS_ZTEST, RS_CMP_LEQUAL);
                }
            }
            alphaRef = 128;
            Gs_SetState(RS_ALPHA_REF, 128);
            set = gAreaSubModels[0];
            if (set == NULL || set->active == 0) {
                goto end;
            }
            Tex_Prepare(16, 296);
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
        time = moment + gDayStepTimes[*(s32 *)(base + 28)];
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
    rect.x1 = 640;
    rect.x0 = 0;
    rect.y1 = 448;
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
    rect.x1 = 640;
    rect.y1 = band->h;
    camYaw = (s32)(0.5f + 65536.0f * Math_Atan2(-gCamDir[0], -gCamDir[2]) / 6.2831855f);
    yaw = (u16)(s32)(0.5f + 65536.0f * Math_Atan2(dir[0], dir[2]) / 6.2831855f) - camYaw;
    if (yaw < (ANGLE_TURN / 4)) {
        frac = yaw / 16384.0f;
        rect.color[0] = Color_Lerp(band->c[0], band->c[2], frac);
        rect.color[1] = Color_Lerp(band->c[0], band->c[3], frac);
        rect.color[2] = Color_Lerp(band->c[4], band->c[6], frac);
        rect.color[3] = Color_Lerp(band->c[4], band->c[7], frac);
    } else if (yaw < (ANGLE_TURN / 2)) {
        frac = (yaw - ANGLE_TURN / 4) / 16384.0f;
        rect.color[0] = Color_Lerp(band->c[2], band->c[1], frac);
        rect.color[1] = Color_Lerp(band->c[3], band->c[1], frac);
        rect.color[2] = Color_Lerp(band->c[6], band->c[5], frac);
        rect.color[3] = Color_Lerp(band->c[7], band->c[5], frac);
    } else if (yaw < (ANGLE_TURN * 3 / 4)) {
        frac = (yaw - ANGLE_TURN / 2) / 16384.0f;
        rect.color[0] = Color_Lerp(band->c[1], band->c[3], frac);
        rect.color[1] = Color_Lerp(band->c[1], band->c[2], frac);
        rect.color[2] = Color_Lerp(band->c[5], band->c[7], frac);
        rect.color[3] = Color_Lerp(band->c[5], band->c[6], frac);
    } else {
        frac = (yaw - ANGLE_TURN * 3 / 4) / 16384.0f;
        rect.color[0] = Color_Lerp(band->c[3], band->c[0], frac);
        rect.color[1] = Color_Lerp(band->c[2], band->c[0], frac);
        rect.color[2] = Color_Lerp(band->c[7], band->c[4], frac);
        rect.color[3] = Color_Lerp(band->c[6], band->c[4], frac);
    }
    Prim_FillGradRect(&rect);
}

extern s16 gAreaFade;
extern s16 gAreaFadeTimer;

/* Draws the area's sky: Sky_DrawGradient in AREA_HOUSE, AREA_HUB_0x57, AREA_ONLINE_VILLAGE to
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
    case AREA_ONLINE_VILLAGE:
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
        rect.x1 = 640;
        rect.y1 = 448;
        rect.y0 = 0;
        if (gAreaFade == 1) {
            Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
            alpha = 255;
            Gfx_SetBlend(GFX_BLEND_ONE, GFX_BLEND_ONE);
            Gfx_SetBlendOp(GFX_BLEND_OP_SUB);
        } else if (gAreaFade == 2) {
            Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
            alpha = 255 - (u8)(127.5f * gAreaFadeTimer);
            rect.color = alpha << 24;
            Prim_FillRect(&rect);
            Gfx_SetBlend(GFX_BLEND_ONE, GFX_BLEND_ONE);
            Gfx_SetBlendOp(GFX_BLEND_OP_SUB);
        } else {
            alpha = 255.0f * (1.0f + Math_Sin(6.2831855f * (gAreaFadeTimer * 182 + 49152) / 65536.0f));
            Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
        }
        if (gAreaFade == 1 || gAreaFade == 2) {
            rect.color = -1;
        } else {
            rect.color = (alpha << 24) | 0xFFFFFF;
        }
        Prim_FillRect(&rect);
        if (gAreaFade == 1 || gAreaFade == 2) {
            Gfx_SetBlendOp(GFX_BLEND_OP_ADD);
        }
    }
    Gs_SetState(RS_ZWRITE, 1);
    Gs_SetState(RS_ALPHA_REF, 128);
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
extern f32 gGlowPosOnlineVillage[][3];
extern s16 gGlowEntryOnlineVillage[];
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
u8 func_7D96D0(void); /* the lobby's: LobbyWorld.unk110, 0 while the lobby world is inactive */

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
    case AREA_ONLINE_VILLAGE:
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
    case AREA_ONLINE_VILLAGE:
        models = state->model;
        if (models != NULL && models->active != 0) {
            lights = models->mats;
            for (i = 0; i < 11; i++) {
                Mtx_Identity(mtx[0]);
                Mtx_SetTrans(mtx[0], gGlowPosOnlineVillage[i][0], gGlowPosOnlineVillage[i][1], gGlowPosOnlineVillage[i][2]);
                switch (gGlowEntryOnlineVillage[i]) {
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
                Area_DrawEntry(&models->matSets[gGlowEntryOnlineVillage[i]], mtx[0], lights);
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
    Gs_SetState(RS_ALPHA_REF, 128);
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
    gScene.bannerNotice = 255;
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
 * phrase) and its subject (an item, a player or a count); returns its id, or 255
 * when the queue is full. */
u8 Notice_Push(u8 kind, s16 text, s16 subject)
{
    s8 i;

    if (gNoticeCount >= NOTICE_QUEUE_LEN - 1) {
        return 255;
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

/* Queues an item notice; its id, or 255 when the queue is full. */
u8 Notice_PushItem(char *item)
{
    s8 i;

    if (gNoticeCount >= NOTICE_QUEUE_LEN - 1) {
        return 255;
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
 * or 255 when the queue is full. */
u8 Notice_PushText(char *text)
{
    s32 i;
    s32 k;

    if (gNoticeCount >= NOTICE_QUEUE_LEN - 1) {
        return 255;
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
    char line[128];
    s16 w;
    s32 i;
    s32 n;

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->keep = 1;
    Sound_PlaySeFlat(0, 16, 0);
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
    for (i = 0, n = 0; i < 128; i++) {
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
            gScene.bannerNotice = 255;
            obj->var0++;
        }
        break;
    case 2:
        if ((obj->pos[1] += obj->fvar) <= -20.0f) {
            obj->state++;
        }
        break;
    }
    return OT_AddByDepth(gDrawOtText3, obj->ds, 16, 1);
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

/* Draws a notice banner (its draw slot's update): a window frame sized to its lines
 * (Obj.var1) and its widest line (pos[2]), then each line of its text, centred by its
 * width (NoticeText.width). */
void TextNotice_Draw(DrawSlot *slot)
{
    Obj *obj = slot->owner;
    NoticeText *banner = obj->buf;
    Frame box;
    char line[128];
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
    Font_SetSpacing(20, 20);
    Text_SetSize(3);
    pos = 0;
    for (n = 0; n < obj->var1; n++) {
        for (i = 0; pos < 128; pos++, i++) {
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
    Gs_SetState(RS_ALPHA_REF, 128);
}

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
extern f32 gAreaPropPosOnlineVillage[][3];
extern f32 gAreaPropPosHub84[][3];
extern f32 gAreaPropPosHub85[][3];
extern f32 gAreaPropPosElderHall[][3];
extern f32 gAreaPropPosHub89[][3];
extern f32 gAreaPropPosHubAA[];
extern f32 gAreaPropPosOnlineHouse1[];
extern f32 gAreaPropPosOnlineHouse2[];

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
    case AREA_ONLINE_VILLAGE:
        Vec_Copy(obj->pos, gAreaPropPosOnlineVillage[obj->type]);
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
        if (pl->motionId[0] == 647 && Motion_PassedFrame(pl, 0, 232.0f)) {
            obj->visible = 0;
        }
        break;
    case AREA_ONLINE_VILLAGE:
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
        if (pl->motionId[0] == 250 && Motion_PassedFrame(pl, 0, 154.0f)) {
            obj->visible = 0;
        }
        break;
    }
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    switch (obj->var4) {
    default:
        OT_AddByDepth(gDrawOtSorted, obj->ds, 64, 0);
        break;
    case 1:
        OT_AddByDepth(gDrawOt32, obj->ds, 32, 0);
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

/* The colour of the props darkened by night: white by day, grey 48 at full night
 * (gAreaState.look[0] 255), else 255 - 207 * night / 255 in each channel. */
u32 Area_NightTint(void)
{
    AreaState *state = &gAreaState;
    u8 r, g, b;
    f32 shade;

    if (gAreaState.look[0] == 0) {
        b = g = r = 255;
    } else if (state->look[0] == 255) {
        b = g = r = 48;
    } else {
        shade = 255.0f - 207.0f * (state->look[0] / 255.0f);
        r = shade;
        g = shade;
        b = shade;
    }
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

extern f32 gAreaPropRotHouse[];
extern f32 gAreaPropScaleOnlineVillage[];
extern s16 gAreaPropEntryOnlineVillage[];
extern f32 gAreaPropScaleHub84[];
extern s16 gAreaPropEntryHub84[];
extern s16 gAreaPropEntryHub85[];
extern f32 gAreaPropScaleElderHall[];
extern s16 gAreaPropEntryElderHall[];

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
        Gs_SetState(RS_ALPHA_REF, 128);
        Gs_SetState(RS_COLOR2, -1);
        if (entry != NULL && entry->id != -1) {
            Draw_ApplyStates(entry->flags);
            Model_Draw(entry->id, 0);
        }
        Mtx_RotateXYZ(mtx, gAreaPropRotHouse[0], gAreaPropRotHouse[1], gAreaPropRotHouse[2]);
        entry = &set->matSets[obj->type];
        break;
    case AREA_ONLINE_VILLAGE:
        set = state->model;
        if (set == NULL || set->active == 0) {
            goto end;
        }
        lights = set->mats;
        entry = &set->matSets[gAreaPropEntryOnlineVillage[obj->type]];
        Gs_SetLightList(lights, entry);
        Mtx_Scale(mtx, gAreaPropScaleOnlineVillage[obj->type], gAreaPropScaleOnlineVillage[obj->type], gAreaPropScaleOnlineVillage[obj->type]);
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
            entry = &set->matSets[gAreaPropEntryOnlineVillage[obj->type + 1]];
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
        Gs_SetState(RS_ALPHA_REF, 128);
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

/* Starts a sound object (an object of kind OBJ_KIND_SOUND, SoundObj_Task) at pos, playing sound
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
        obj->kind = 12;
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

extern f32 gRiverPtsDay48[];
extern f32 gRiverPtsDay4B[];
extern f32 gRiverPtsDay6E[];
extern f32 gRiverPtsDay71[];
extern f32 gRiverPtsDay72[];
extern f32 gRiverPtsDay78[];
extern f32 gRiverPtsDayA1[];


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
    case 10:
    case 11:
    case 13:
    case 14:
        return Obj_Alloc(1);
    }
}

/* Starts an area atmosphere object (OBJ_KIND_AREA_ATMOS, AreaAtmos_Task) of the given type:
 * Area_StartObjects starts the area's own (gAreaInfo). */
void AreaAtmos_Spawn(s32 type)
{
    Obj *obj = AreaAtmos_Alloc(type);

    if (obj != NULL) {
        obj->kind = 13;
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
            lim = 1024;
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
            lim = 1024;
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
            if (++obj->timer >= 256) {
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
        lim = 128;
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
            div = 1024;
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
            div = 1024;
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
        if (obj->timer > 128) {
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

/* The point at s times the vector v from the camera's position. */
void Vec_FromCamera(f32 *out, f32 *dir, f32 dist)
{
    f32 x, y, z;

    x = dist * dir[0];
    y = dist * dir[1];
    z = dist * dir[2];
    out[0] = gCamPos[0] + x;
    out[1] = gCamPos[1] + y;
    out[2] = gCamPos[2] + z;
}

/* 1 when the line from the camera to pos (a capsule of radius 5) meets one
 * of the area's spheres in gAreaBlockSpheres (radius, x, y, z; -1 ends). */
s32 Area_LineBlocked(f32 *pos)
{
    f32 center[3];
    f32 pushOut[4];
    HitCapsule cap;
    u8 capsule[64];
    f32 *sphere;
    f32 sphereRadius;

    Vec_Copy(cap.a, gCamPos);
    Vec_Copy(cap.b, pos);
    cap.radius = 5.0f;
    Capsule_Copy(&cap, (Capsule *) capsule);
    sphere = gAreaBlockSpheres[gScene.area];
    if (sphere != NULL) {
        for (sphereRadius = *sphere; sphereRadius != -1.0f; sphereRadius = *sphere) {
            sphere++;
            center[0] = *sphere++;
            center[1] = *sphere++;
            center[2] = *sphere++;
            if (Capsule_PushSphere((Capsule *) capsule, center, pushOut, sphereRadius)) {
                return 1;
            }
        }
    }
    return 0;
}

/*
 * Where the player stands in AREA_DESERT_1 and AREA_DESERT_5 (and their
 * night areas): 0 inside the inner limits (x between two bounds; in
 * AREA_DESERT_1, x below one bound and z above another), 1 beyond the
 * outer ones, 2 in between. 0 in every other area.
 */
s16 Area_BoundaryZone(Player *pl)
{
    f32 outerMinX, outerMinZ, innerMinZ, outerMaxX, innerMaxX, innerMinX;
    u8 shape;

    switch (gScene.area) {
    case AREA_DESERT_NIGHT_1:
    case AREA_DESERT_1:
        outerMaxX = 17800.0f;
        outerMinZ = 2200.0f;
        innerMaxX = 17200.0f;
        innerMinZ = 2800.0f;
        shape = 1;
        break;
    case AREA_DESERT_NIGHT_5:
    case AREA_DESERT_5:
        outerMinX = 4500.0f;
        outerMaxX = 18000.0f;
        innerMinX = 5100.0f;
        innerMaxX = 17400.0f;
        shape = 0;
        break;
    default:
        return 0;
    }
    switch (shape) {
    default:
        if (pl->pos[0] > innerMinX && pl->pos[0] < innerMaxX) {
            return 0;
        }
        if (pl->pos[0] < outerMinX || pl->pos[0] > outerMaxX) {
            return 1;
        }
        break;
    case 1:
        if (pl->pos[0] < innerMaxX && pl->pos[2] > innerMinZ) {
            return 0;
        }
        if (pl->pos[0] > outerMaxX || pl->pos[2] < outerMinZ) {
            return 1;
        }
        break;
    }
    return 2;
}

/*
 * Scrolls the n atmosphere layers of h with the camera: u by the camera's
 * turn about y since the last frame, v by its rise. A layer whose speed
 * is set also drifts by 1 / speed per frame, in u (mode 1) or v (mode 2).
 * The coordinates wrap to 0..1.
 */
void AtmosScroll_Update(AtmosScroll *scroll, s16 count, s16 *speed, u16 mode)
{
    f32 camNow[3];
    u16 camPitch;
    u16 camYaw;
    f32 uStep;
    f32 vStep;
    s16 i;

    Mtx_GetTrans(camNow, gCamMatrix);
    Vec_DirToAngles((f32 *) gCamDir, (s16 *) &camPitch, (s16 *) &camYaw);
    uStep = (s16)(camYaw - scroll->yaw);
    uStep *= 6.964391650399193e-05f;
    vStep = camNow[1] - scroll->camPos[1];
    vStep *= 0.001f;
    scroll->pitch = camPitch;
    scroll->yaw = camYaw;
    Vec_Copy(scroll->camPos, camNow);
    for (i = 0; i < count; i++) {
        scroll->layer[i].u += uStep;
        scroll->layer[i].v += vStep;
        if (speed != NULL && speed[i] != 0) {
            switch (mode) {
            case 0:
                break;
            case 1:
                scroll->layer[i].u += 1.0f / speed[i];
                break;
            case 2:
                scroll->layer[i].v += 1.0f / speed[i];
                break;
            }
        }
        if (scroll->layer[i].u > 1.0f || scroll->layer[i].u < 0.0f) {
            scroll->layer[i].u -= Math_Floor(scroll->layer[i].u);
        }
        if (scroll->layer[i].v > 1.0f || scroll->layer[i].v < 0.0f) {
            scroll->layer[i].v -= Math_Floor(scroll->layer[i].v);
        }
    }
}

/* The alpha of atmosphere layer a in the area's state b (Area_MonsterState)
 * for the jungle and fortress at night, the swamp, the snowy mountains and
 * the swamp at night; 1.0 elsewhere. */
f32 AreaAtmos_LayerAlpha(u8 layer, s16 state)
{
    switch (gScene.area) {
    case AREA_JUNGLE_NIGHT_0:
    case AREA_JUNGLE_NIGHT_1:
    case AREA_JUNGLE_NIGHT_2:
    case AREA_JUNGLE_NIGHT_3:
    case AREA_JUNGLE_NIGHT_4:
    case AREA_SWAMP_10:
    case AREA_JUNGLE_NIGHT_5:
    case AREA_JUNGLE_NIGHT_6:
    case AREA_FORTRESS_NIGHT_4:
    case AREA_FORTRESS_NIGHT_5:
    case AREA_FORTRESS_NIGHT_1:
    case AREA_FORTRESS_NIGHT_3:
    case AREA_FORTRESS_NIGHT_2:
    case AREA_FORTRESS_NIGHT_6:
    case AREA_SWAMP_0:
    case AREA_SWAMP_1:
    case AREA_SWAMP_2:
    case AREA_SWAMP_5:
    case AREA_SWAMP_6:
    case AREA_SWAMP_7:
    case AREA_SWAMP_9:
        switch (state) {
        case 0:
            switch (layer) {
            case 0:
            case 1:
                return 0.0f;
            }
            break;
        case 1:
            switch (layer) {
            case 0:
                return 0.0f;
            case 1:
                return 0.45f;
            }
            break;
        case 2:
            switch (layer) {
            case 1:
                return 0.9f;
            }
            break;
        }
        break;
    case AREA_SNOWY_MOUNTAINS_5:
    case AREA_SNOWY_MOUNTAINS_6:
    case AREA_SNOWY_MOUNTAINS_8:
    case AREA_SNOWY_MOUNTAINS_NIGHT_5:
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
    case AREA_SNOWY_MOUNTAINS_NIGHT_8:
        switch (state) {
        case 0:
            switch (layer) {
            case 0:
            case 1:
                return 0.0f;
            }
            break;
        case 1:
            switch (layer) {
            case 0:
                return 0.1f;
            case 1:
                return 0.2f;
            }
            break;
        case 2:
            switch (layer) {
            case 0:
                return 1.0f;
            case 1:
                return 0.45f;
            }
            break;
        }
        break;
    case AREA_SWAMP_NIGHT_0:
    case AREA_SWAMP_NIGHT_10:
    case AREA_SWAMP_NIGHT_1:
    case AREA_SWAMP_NIGHT_2:
    case AREA_SWAMP_NIGHT_5:
    case AREA_SWAMP_NIGHT_6:
    case AREA_SWAMP_NIGHT_7:
    case AREA_SWAMP_NIGHT_9:
        switch (state) {
        case 0:
            switch (layer) {
            case 0:
                return 0.0f;
            case 1:
                return 0.1f;
            }
            break;
        case 1:
            switch (layer) {
            case 0:
                return 0.1f;
            case 1:
                return 0.2f;
            }
            break;
        case 2:
            switch (layer) {
            case 0:
                return 0.5f;
            case 1:
                return 0.4f;
            }
            break;
        }
        break;
    default:
        return 1.0f;
    }
    return 1.0f;
}

/* The scroll speed of atmosphere layer a in the area's state b: the snowy
 * mountains' from gSnowLayerSpeedsState0 to 2, the swamp's at night 1 /
 * its period (gSwampLayerPeriodsByState; layer 0 doesn't move); 0
 * elsewhere. */
f32 AreaAtmos_LayerSpeed(u8 layer, s16 state)
{
    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_5:
    case AREA_SNOWY_MOUNTAINS_6:
    case AREA_SNOWY_MOUNTAINS_8:
    case AREA_SNOWY_MOUNTAINS_NIGHT_5:
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
    case AREA_SNOWY_MOUNTAINS_NIGHT_8:
        switch (state) {
        case 1:
            return gSnowLayerSpeedsState1[layer];
        default:
        case 0:
            return gSnowLayerSpeedsState0[layer];
        case 2:
            return gSnowLayerSpeedsState2[layer];
        }
    case AREA_SWAMP_NIGHT_0:
    case AREA_SWAMP_NIGHT_10:
    case AREA_SWAMP_NIGHT_1:
    case AREA_SWAMP_NIGHT_2:
    case AREA_SWAMP_NIGHT_5:
    case AREA_SWAMP_NIGHT_6:
    case AREA_SWAMP_NIGHT_7:
    case AREA_SWAMP_NIGHT_9:
        switch (layer) {
        case 0:
            return 0.0f;
        default:
        case 1:
            return 1.0f / gSwampLayerPeriodsByState[state][layer];
        }
    }
    return 0.0f;
}

/* The file's data. */
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
AreaRecord gDefaultAreaRecords[1] = {
#include "sub_main/00441490.inc"
};
StageHeader gDefaultStageHeader = {
#include "sub_main/004414B0.inc"
};
AnimSpin gAreaAnimArg_1_0[1] = {
#include "sub_main/00441530.inc"
};
AreaAnim gAreaAnimTbls_1[4] = {
#include "sub_main/00441540.inc"
};
AnimSpin gAreaAnimArg_2_0[1] = {
#include "sub_main/00441560.inc"
};
AreaAnim gAreaAnimTbls_2[2] = {
#include "sub_main/00441570.inc"
};
AnimSpin gAreaAnimArg_4_0[1] = {
#include "sub_main/00441580.inc"
};
AreaAnim gAreaAnimTbls_4[2] = {
#include "sub_main/00441590.inc"
};
AnimSpin gAreaAnimArg_5_0[1] = {
#include "sub_main/004415A0.inc"
};
AreaAnim gAreaAnimTbls_5[2] = {
#include "sub_main/004415B0.inc"
};
AreaAnim gAreaAnimTbls_7[4] = {
#include "sub_main/004415C0.inc"
};
AnimSpin gAreaAnimArg_8_0[1] = {
#include "sub_main/004415E0.inc"
};
UvScroll gAreaAnimArg_8_1[4] = {
#include "sub_main/004415F0.inc"
};
AreaAnim gAreaAnimTbls_8[4] = {
#include "sub_main/00441600.inc"
};
AreaAnim gAreaAnimTbls_9[2] = {
#include "sub_main/00441620.inc"
};
AnimSpin gAreaAnimArg_14_0[1] = {
#include "sub_main/00441630.inc"
};
AnimSpin gAreaAnimArg_14_1[1] = {
#include "sub_main/00441640.inc"
};
AreaAnim gAreaAnimTbls_14[4] = {
#include "sub_main/00441650.inc"
};
UvWave gAreaAnimArg_21_0[2] = {
#include "sub_main/00441670.inc"
};
AreaAnim gAreaAnimTbls_21[2] = {
#include "sub_main/00441690.inc"
};
AreaAnim gAreaAnimTbls_24[4] = {
#include "sub_main/004416A0.inc"
};
AnimSpin gAreaAnimArg_25_0[1] = {
#include "sub_main/004416C0.inc"
};
AnimSpin gAreaAnimArg_25_1_Spin[1] = {
#include "sub_main/004416D0.inc"
};
UvScroll gAreaAnimArg_25_1_Uv[2] = {
#include "sub_main/004416E0.inc"
};
AnimScrollSpin gAreaAnimArg_25_1[1] = {
    {gAreaAnimArg_25_1_Uv, gAreaAnimArg_25_1_Spin},
};
AreaAnim gAreaAnimTbls_25[4] = {
#include "sub_main/004416F0.inc"
};
UvScroll gAreaAnimArg_27_1[2] = {
#include "sub_main/00441710.inc"
};
UvScroll gAreaAnimArg_27_3[2] = {
#include "sub_main/00441718.inc"
};
AreaAnim gAreaAnimTbls_27[8] = {
#include "sub_main/00441720.inc"
};
AnimSpin gAreaAnimArg_31_0[1] = {
#include "sub_main/00441760.inc"
};
AnimSpin gAreaAnimArg_31_1[1] = {
#include "sub_main/00441770.inc"
};
AreaAnim gAreaAnimTbls_31[4] = {
#include "sub_main/00441780.inc"
};
AnimSpin gAreaAnimArg_32_0[1] = {
#include "sub_main/004417A0.inc"
};
AreaAnim gAreaAnimTbls_32[4] = {
#include "sub_main/004417B0.inc"
};
AnimSpin gAreaAnimArg_33_0[1] = {
#include "sub_main/004417D0.inc"
};
AreaAnim gAreaAnimTbls_33[2] = {
#include "sub_main/004417E0.inc"
};
UvWave gAreaAnimArg_34_0[2] = {
#include "sub_main/004417F0.inc"
};
AreaAnim gAreaAnimTbls_34[2] = {
#include "sub_main/00441810.inc"
};
UvWave gAreaAnimArg_35_0[2] = {
#include "sub_main/00441820.inc"
};
AreaAnim gAreaAnimTbls_35[2] = {
#include "sub_main/00441840.inc"
};
UvWave gAreaAnimArg_36_0[2] = {
#include "sub_main/00441850.inc"
};
AreaAnim gAreaAnimTbls_36[2] = {
#include "sub_main/00441870.inc"
};
AnimSpin gAreaAnimArg_37_0[1] = {
#include "sub_main/00441880.inc"
};
UvScroll gAreaAnimArg_37_1[4] = {
#include "sub_main/00441890.inc"
};
AreaAnim gAreaAnimTbls_37[4] = {
#include "sub_main/004418A0.inc"
};
AnimSpin gAreaAnimArg_38_0[1] = {
#include "sub_main/004418C0.inc"
};
UvScroll gAreaAnimArg_38_1[4] = {
#include "sub_main/004418D0.inc"
};
AreaAnim gAreaAnimTbls_38[4] = {
#include "sub_main/004418E0.inc"
};
AnimSpin gAreaAnimArg_39_0[1] = {
#include "sub_main/00441900.inc"
};
UvScroll gAreaAnimArg_39_1[4] = {
#include "sub_main/00441910.inc"
};
UvStep gAreaAnimArg_39_2[2] = {
#include "sub_main/00441920.inc"
};
AreaAnim gAreaAnimTbls_39[4] = {
#include "sub_main/00441930.inc"
};
AreaAnim gAreaAnimTbls_40[2] = {
#include "sub_main/00441950.inc"
};
UvWave gAreaAnimArg_41_0[2] = {
#include "sub_main/00441960.inc"
};
AreaAnim gAreaAnimTbls_41[2] = {
#include "sub_main/00441980.inc"
};
UvWave gAreaAnimArg_42_0[2] = {
#include "sub_main/00441990.inc"
};
AreaAnim gAreaAnimTbls_42[2] = {
#include "sub_main/004419B0.inc"
};
UvWave gAreaAnimArg_43_0[2] = {
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
UvScroll gAreaAnimArg_47_2[2] = {
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
UvScroll gAreaAnimArg_58_1[2] = {
#include "sub_main/00441B28.inc"
};
AreaAnim gAreaAnimTbls_58[4] = {
#include "sub_main/00441B30.inc"
};
UvScroll gAreaAnimArg_60_0[2] = {
#include "sub_main/00441B50.inc"
};
UvScroll gAreaAnimArg_60_4[2] = {
#include "sub_main/00441B58.inc"
};
AreaAnim gAreaAnimTbls_60[7] = {
#include "sub_main/00441B60.inc"
};
UvScroll gAreaAnimArg_61_1[2] = {
#include "sub_main/00441B98.inc"
};
AreaAnim gAreaAnimTbls_61[6] = {
#include "sub_main/00441BA0.inc"
};
UvScroll gAreaAnimArg_62_1[2] = {
#include "sub_main/00441BD0.inc"
};
UvScroll gAreaAnimArg_62_3[2] = {
#include "sub_main/00441BD8.inc"
};
AreaAnim gAreaAnimTbls_62[8] = {
#include "sub_main/00441BE0.inc"
};
UvScroll gAreaAnimArg_65_0[2] = {
#include "sub_main/00441C20.inc"
};
UvScroll gAreaAnimArg_65_2[2] = {
#include "sub_main/00441C28.inc"
};
AreaAnim gAreaAnimTbls_65[6] = {
#include "sub_main/00441C30.inc"
};
AreaAnim gAreaAnimTbls_66[4] = {
#include "sub_main/00441C60.inc"
};
AnimSpin gAreaAnimArg_74_0[1] = {
#include "sub_main/00441C80.inc"
};
UvScroll gAreaAnimArg_74_1[2] = {
#include "sub_main/00441C90.inc"
};
UvScroll gAreaAnimArg_74_3[2] = {
#include "sub_main/00441C98.inc"
};
AreaAnim gAreaAnimTbls_74[8] = {
#include "sub_main/00441CA0.inc"
};
AreaAnim gAreaAnimTbls_75[2] = {
#include "sub_main/00441CE0.inc"
};
f32 gAreaAnimArg_84_0[1][3] = {
#include "sub_main/00441CF0.inc"
};
AreaAnim gAreaAnimTbls_84[5] = {
#include "sub_main/00441D00.inc"
};
UvScroll gAreaAnimArg_85_0[2] = {
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
AnimSpin gAreaAnimArg_88_0[1] = {
#include "sub_main/00441DB0.inc"
};
UvScroll gAreaAnimArg_88_1[2] = {
#include "sub_main/00441DC0.inc"
};
UvScroll gAreaAnimArg_88_3[2] = {
#include "sub_main/00441DC8.inc"
};
AreaAnim gAreaAnimTbls_88[6] = {
#include "sub_main/00441DD0.inc"
};
AnimSpin gAreaAnimArg_89_0[1] = {
#include "sub_main/00441E00.inc"
};
AreaAnim gAreaAnimTbls_89[4] = {
#include "sub_main/00441E10.inc"
};
AnimSpin gAreaAnimArg_90_0[1] = {
#include "sub_main/00441E30.inc"
};
AreaAnim gAreaAnimTbls_90[4] = {
#include "sub_main/00441E40.inc"
};
AnimSpin gAreaAnimArg_91_0[1] = {
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
UvScroll gAreaAnimArg_96_0[4] = {
#include "sub_main/00441F20.inc"
};
AreaAnim gAreaAnimTbls_96[4] = {
#include "sub_main/00441F30.inc"
};
UvScroll gAreaAnimArg_97_0[4] = {
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
UvScroll gAreaAnimArg_100_0[2] = {
#include "sub_main/00441FB0.inc"
};
UvScroll gAreaAnimArg_100_2[2] = {
#include "sub_main/00441FB8.inc"
};
UvScroll gAreaAnimArg_100_4[4] = {
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
AnimSpin gAreaAnimArg_111_0[1] = {
#include "sub_main/00442050.inc"
};
AreaAnim gAreaAnimTbls_111[2] = {
#include "sub_main/00442060.inc"
};
AnimSpin gAreaAnimArg_3_0[1] = {
#include "sub_main/00442070.inc"
};
UvScroll gAreaAnimArg_3_1[4] = {
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
UvScroll gAreaAnimArg_22_0[4] = {
#include "sub_main/004420E0.inc"
};
AreaAnim gAreaAnimTbls_22[4] = {
#include "sub_main/004420F0.inc"
};
UvScroll gAreaAnimArg_23_0[4] = {
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
UvScroll gAreaAnimArg_122_0[4] = {
#include "sub_main/00442170.inc"
};
AreaAnim gAreaAnimTbls_122[6] = {
#include "sub_main/00442180.inc"
};
AreaAnim gAreaAnimTbls_123[6] = {
#include "sub_main/004421B0.inc"
};
UvScroll gAreaAnimArg_124_0[2] = {
#include "sub_main/004421E0.inc"
};
UvScroll gAreaAnimArg_124_2[2] = {
#include "sub_main/004421E8.inc"
};
UvScroll gAreaAnimArg_124_4[2] = {
#include "sub_main/004421F0.inc"
};
UvScroll gAreaAnimArg_124_6[2] = {
#include "sub_main/004421F8.inc"
};
AreaAnim gAreaAnimTbls_124[10] = {
#include "sub_main/00442200.inc"
};
UvScroll gAreaAnimArg_83_0[4] = {
#include "sub_main/00442250.inc"
};
AreaAnim gAreaAnimTbls_83[2] = {
#include "sub_main/00442260.inc"
};
AreaAnim gAreaAnimTbls_127[4] = {
#include "sub_main/00442270.inc"
};
UvScroll gAreaAnimArg_128_0[2] = {
#include "sub_main/00442290.inc"
};
UvScroll gAreaAnimArg_128_3[2] = {
#include "sub_main/00442298.inc"
};
AreaAnim gAreaAnimTbls_128[10] = {
#include "sub_main/004422A0.inc"
};
UvScroll gAreaAnimArg_129_1[2] = {
#include "sub_main/004422F0.inc"
};
UvScroll gAreaAnimArg_129_3[2] = {
#include "sub_main/004422F8.inc"
};
AreaAnim gAreaAnimTbls_129[8] = {
#include "sub_main/00442300.inc"
};
UvScroll gAreaAnimArg_131_0[4] = {
#include "sub_main/00442340.inc"
};
AreaAnim gAreaAnimTbls_131[10] = {
#include "sub_main/00442350.inc"
};
UvScroll gAreaAnimArg_132_0[4] = {
#include "sub_main/004423A0.inc"
};
AreaAnim gAreaAnimTbls_132[14] = {
#include "sub_main/004423B0.inc"
};
AnimSpinTurn gAreaAnimArg_133_2[4] = {
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
UvStep gAreaAnimArg_136_0[2] = {
#include "sub_main/00442510.inc"
};
AreaAnim gAreaAnimTbls_136[8] = {
#include "sub_main/00442520.inc"
};
AreaAnim gAreaAnimTbls_137[4] = {
#include "sub_main/00442560.inc"
};
UvScroll gAreaAnimArg_138_0[2] = {
#include "sub_main/00442580.inc"
};
UvScroll gAreaAnimArg_138_3[2] = {
#include "sub_main/00442588.inc"
};
UvScroll gAreaAnimArg_138_5[4] = {
#include "sub_main/00442590.inc"
};
AreaAnim gAreaAnimTbls_138[8] = {
#include "sub_main/004425A0.inc"
};
AreaAnim gAreaAnimTbls_140[2] = {
#include "sub_main/004425E0.inc"
};
UvScroll gAreaAnimArg_141_1[4] = {
#include "sub_main/004425F0.inc"
};
AreaAnim gAreaAnimTbls_141[6] = {
#include "sub_main/00442600.inc"
};
AnimSpin gAreaAnimArg_142_0[1] = {
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
f32 gAreaAnimArg_68_0[1][3] = {
#include "sub_main/00442778.inc"
};
AreaAnim gAreaAnimTbls_68[4] = {
#include "sub_main/00442790.inc"
};
f32 gAreaAnimArg_69_0[1][3] = {
#include "sub_main/004427B0.inc"
};
AreaAnim gAreaAnimTbls_69[6] = {
#include "sub_main/004427C0.inc"
};
AnimSpin gAreaAnimArg_70_0[1] = {
#include "sub_main/004427F0.inc"
};
AnimSpin gAreaAnimArg_70_1[1] = {
#include "sub_main/00442800.inc"
};
AreaAnim gAreaAnimTbls_70[3] = {
#include "sub_main/00442810.inc"
};
UvScroll gAreaAnimArg_71_0[2] = {
#include "sub_main/00442828.inc"
};
AreaAnim gAreaAnimTbls_71[4] = {
#include "sub_main/00442830.inc"
};
AreaAnim gAreaAnimTbls_158[2] = {
#include "sub_main/00442850.inc"
};
AnimSpin gAreaAnimArg_73_0[1] = {
#include "sub_main/00442860.inc"
};
AreaAnim gAreaAnimTbls_73[4] = {
#include "sub_main/00442870.inc"
};
AreaAnim gAreaAnimTbls_160[2] = {
#include "sub_main/00442890.inc"
};
AnimSpin gAreaAnimArg_161_0[1] = {
#include "sub_main/004428A0.inc"
};
UvScroll gAreaAnimArg_161_1[4] = {
#include "sub_main/004428B0.inc"
};
AreaAnim gAreaAnimTbls_161[4] = {
#include "sub_main/004428C0.inc"
};
UvScroll gAreaAnimArg_59_0[4] = {
#include "sub_main/004428E0.inc"
};
AreaAnim gAreaAnimTbls_59[5] = {
#include "sub_main/004428F0.inc"
};
UvScroll gAreaAnimArg_163_0[2] = {
#include "sub_main/00442918.inc"
};
UvScroll gAreaAnimArg_163_4[4] = {
#include "sub_main/00442920.inc"
};
AreaAnim gAreaAnimTbls_163[8] = {
#include "sub_main/00442930.inc"
};
AnimSpin gAreaAnimArg_164_0[1] = {
#include "sub_main/00442970.inc"
};
UvScroll gAreaAnimArg_164_2[4] = {
#include "sub_main/00442980.inc"
};
AreaAnim gAreaAnimTbls_164[4] = {
#include "sub_main/00442990.inc"
};
AnimSpin gAreaAnimArg_165_0[1] = {
#include "sub_main/004429B0.inc"
};
UvScroll gAreaAnimArg_165_1[2] = {
#include "sub_main/004429C0.inc"
};
UvScroll gAreaAnimArg_165_3[2] = {
#include "sub_main/004429C8.inc"
};
AreaAnim gAreaAnimTbls_165[7] = {
#include "sub_main/004429D0.inc"
};
UvScroll gAreaAnimArg_63_0[2] = {
#include "sub_main/00442A08.inc"
};
UvScroll gAreaAnimArg_63_4[4] = {
#include "sub_main/00442A10.inc"
};
AreaAnim gAreaAnimTbls_63[8] = {
#include "sub_main/00442A20.inc"
};
AnimSpin gAreaAnimArg_64_0[1] = {
#include "sub_main/00442A60.inc"
};
UvScroll gAreaAnimArg_64_1[2] = {
#include "sub_main/00442A70.inc"
};
UvScroll gAreaAnimArg_64_3[2] = {
#include "sub_main/00442A78.inc"
};
AreaAnim gAreaAnimTbls_64[8] = {
#include "sub_main/00442A80.inc"
};
UvScroll gAreaAnimArg_168_0[2] = {
#include "sub_main/00442AC0.inc"
};
UvScroll gAreaAnimArg_168_2[2] = {
#include "sub_main/00442AC8.inc"
};
UvScroll gAreaAnimArg_168_4[4] = {
#include "sub_main/00442AD0.inc"
};
AreaAnim gAreaAnimTbls_168[8] = {
#include "sub_main/00442AE0.inc"
};
AnimSpin gAreaAnimArg_169_0[1] = {
#include "sub_main/00442B20.inc"
};
UvScroll gAreaAnimArg_169_1_Uv[2] = {
#include "sub_main/00442B30.inc"
};
UvScroll gAreaAnimArg_169_3[2] = {
#include "sub_main/00442B38.inc"
};
f32 gAreaAnimArg_169_1_Pos[6] = {
#include "sub_main/00442B40.inc"
};
AnimScrollPos gAreaAnimArg_169_1[1] = {
    {gAreaAnimArg_169_1_Uv, gAreaAnimArg_169_1_Pos},
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
AnimSpin gAreaAnimArg_177_0[1] = {
#include "sub_main/00442C50.inc"
};
UvScroll gAreaAnimArg_177_1[4] = {
#include "sub_main/00442C60.inc"
};
AreaAnim gAreaAnimTbls_177[4] = {
#include "sub_main/00442C70.inc"
};
AnimSpin gAreaAnimArg_178_0[2] = {
#include "sub_main/00442C90.inc"
};
AreaAnim gAreaAnimTbls_178[3] = {
#include "sub_main/00442CB0.inc"
};
UvScroll gAreaAnimArg_179_0[2] = {
#include "sub_main/00442CC8.inc"
};
AreaAnim gAreaAnimTbls_179[14] = {
#include "sub_main/00442CD0.inc"
};
AreaAnim gAreaAnimTbls_180[2] = {
#include "sub_main/00442D40.inc"
};
AnimSpin gAreaAnimArg_181_1[1] = {
#include "sub_main/00442D50.inc"
};
UvScroll gAreaAnimArg_181_2[4] = {
#include "sub_main/00442D60.inc"
};
AreaAnim gAreaAnimTbls_181[6] = {
#include "sub_main/00442D70.inc"
};
AnimSpin gAreaAnimArg_182_1[2] = {
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
UvScroll gAreaAnimArg_191_1[4] = {
#include "sub_main/00442E20.inc"
};
AreaAnim gAreaAnimTbls_191[3] = {
#include "sub_main/00442E30.inc"
};
UvScroll gAreaAnimArg_192_1[2] = {
#include "sub_main/00442E48.inc"
};
UvStep gAreaAnimArg_192_2[2] = {
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
UvScroll gAreaPropData_1_0[2] = {
#include "sub_main/004431A0.inc"
};
UvScroll gAreaPropData_1_2[2] = {
#include "sub_main/004431A8.inc"
};
extern AnimRot D_0092A730[];
extern AnimRot D_0092A740[];
extern AnimRot D_0092A770[];
extern AnimRot D_0092A7B0[];
extern AnimRot D_0092A7D0[];
extern AnimRot D_0092A7F0[];
extern AnimRot D_0092A810[];
extern AnimRot D_0092A840[];
extern AnimRot D_0092A860[];
extern AnimRot D_0092A880[];
extern AnimRot D_0092A8B0[];
extern AnimRot D_0092A910[];
extern AnimRot D_0092A940[];
extern AnimRot D_0092A950[];
extern f32 D_0092A9A0[][2];
extern f32 D_0092A9C0[][2];
extern AnimRot D_0092A9E0[];
extern AnimRot D_0092AA30[];
extern f32 D_0092AA80[][2];
extern f32 D_0092AA90[][2];
extern AnimRot D_0092AAB0[];
extern AnimRot D_0092AAF0[];
extern AnimRot D_0092AB10[];
extern AnimRot D_0092AB30[];
extern AnimRot D_0092AB50[];
extern AnimRot D_0092AB90[];
extern AnimRot D_0092ABD0[];
extern AnimRot D_0092AC40[];
extern AnimRot D_0092ACA0[];
extern AnimRot D_0092ACF0[];
extern AnimRot D_0092AD30[];
extern AnimRot D_0092ABF0[];
extern AnimRot D_0092AC00[];
extern AnimRot D_0092AC10[];
extern AnimRot D_0092AC20[];
extern AnimRot D_0092AC30[];
PropRec gAreaProps_1[8] = {
#include "sub_main/004431B0.inc"
};
AnimSpin gAreaPropData_7_0[1] = {
#include "sub_main/00443210.inc"
};
AnimSpin gAreaPropData_7_1[1] = {
#include "sub_main/00443220.inc"
};
PropRec gAreaProps_7[3] = {
#include "sub_main/00443230.inc"
};
UvScroll gAreaPropData_18_0[2] = {
#include "sub_main/00443258.inc"
};
PropRec gAreaProps_18[4] = {
#include "sub_main/00443260.inc"
};
AnimSpin gAreaPropData_19_1[1] = {
#include "sub_main/00443290.inc"
};
PropRec gAreaProps_19[4] = {
#include "sub_main/004432A0.inc"
};
f32 gAreaPropData_20_0[2][3] = {
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
UvScroll gAreaPropData_24_0[2] = {
#include "sub_main/00443338.inc"
};
UvScroll gAreaPropData_24_2[4] = {
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
AnimSpin gAreaPropData_39_0[1] = {
#include "sub_main/00443460.inc"
};
PropRec gAreaProps_39[4] = {
#include "sub_main/00443470.inc"
};
AnimRotScale gAreaPropData_43_0[1] = {
#include "sub_main/004434A0.inc"
};
PropRec gAreaProps_43[4] = {
#include "sub_main/004434B0.inc"
};
UvScroll gAreaPropData_45_0[2] = {
#include "sub_main/004434E0.inc"
};
UvScroll gAreaPropData_45_2[2] = {
#include "sub_main/004434E8.inc"
};
PropRec gAreaProps_45[5] = {
#include "sub_main/004434F0.inc"
};
UvScroll gAreaPropData_46_0[2] = {
#include "sub_main/00443530.inc"
};
UvScroll gAreaPropData_46_2[2] = {
#include "sub_main/00443538.inc"
};
PropRec gAreaProps_46[5] = {
#include "sub_main/00443540.inc"
};
UvScroll gAreaPropData_47_0[2] = {
#include "sub_main/00443580.inc"
};
UvScroll gAreaPropData_47_2[2] = {
#include "sub_main/00443588.inc"
};
UvScroll gAreaPropData_47_4[4] = {
#include "sub_main/00443590.inc"
};
PropRec gAreaProps_47[8] = {
#include "sub_main/004435A0.inc"
};
UvScroll gAreaPropData_48_2[2] = {
#include "sub_main/00443600.inc"
};
UvScroll gAreaPropData_48_4[2] = {
#include "sub_main/00443608.inc"
};
PropRec gAreaProps_48[8] = {
#include "sub_main/00443610.inc"
};
PropRec gAreaProps_49[4] = {
#include "sub_main/00443670.inc"
};
UvScroll gAreaPropData_51_0[2] = {
#include "sub_main/004436A0.inc"
};
UvScroll gAreaPropData_51_2[2] = {
#include "sub_main/004436A8.inc"
};
UvScroll gAreaPropData_51_6[4] = {
#include "sub_main/004436B0.inc"
};
PropRec gAreaProps_51[9] = {
#include "sub_main/004436C0.inc"
};
f32 gAreaPropData_52_4[2][3] = {
#include "sub_main/00443730.inc"
};
UvScroll gAreaPropData_52_0[4] = {
#include "sub_main/00443750.inc"
};
AnimSpin2 gAreaPropData_52_6[1] = {
#include "sub_main/00443760.inc"
};
PropRec gAreaProps_52[12] = {
#include "sub_main/00443770.inc"
};
UvScroll gAreaPropData_54_1[2] = {
#include "sub_main/00443800.inc"
};
UvScroll gAreaPropData_54_3[2] = {
#include "sub_main/00443808.inc"
};
PropRec gAreaProps_54[6] = {
#include "sub_main/00443810.inc"
};
UvScroll gAreaPropData_55_1[4] = {
#include "sub_main/00443860.inc"
};
UvScroll gAreaPropData_55_5[4] = {
#include "sub_main/00443870.inc"
};
PropRec gAreaProps_55[8] = {
#include "sub_main/00443880.inc"
};
UvScroll gAreaPropData_57_0[2] = {
#include "sub_main/004438E0.inc"
};
UvScroll gAreaPropData_57_2[2] = {
#include "sub_main/004438E8.inc"
};
PropRec gAreaProps_57[5] = {
#include "sub_main/004438F0.inc"
};
UvScroll gAreaPropData_60_0[4] = {
#include "sub_main/00443930.inc"
};
PropRec gAreaProps_60[3] = {
#include "sub_main/00443940.inc"
};
UvScroll gAreaPropData_63_0[2] = {
#include "sub_main/00443968.inc"
};
PropRec gAreaProps_63[3] = {
#include "sub_main/00443970.inc"
};
UvScroll gAreaPropData_65_0[2] = {
#include "sub_main/00443998.inc"
};
PropRec gAreaProps_65[2] = {
#include "sub_main/004439A0.inc"
};
UvScroll gAreaPropData_66_0[4] = {
#include "sub_main/004439C0.inc"
};
PropRec gAreaProps_66[5] = {
#include "sub_main/004439D0.inc"
};
PropRec gAreaProps_75[2] = {
#include "sub_main/00443A10.inc"
};
UvScroll gAreaPropData_84_0_Uv[4] = {
#include "sub_main/00443A30.inc"
};
f32 gAreaPropData_84_0_Pos[8] = {
#include "sub_main/00443A40.inc"
};
AnimScrollPos gAreaPropData_84_0[1] = {
    {gAreaPropData_84_0_Uv, gAreaPropData_84_0_Pos},
};
AnimScrollPos gAreaPropData_84_1[1] = {
    {&gAreaPropData_84_0_Uv[1], &gAreaPropData_84_0_Pos[3]},
};
PropRec gAreaProps_84[5] = {
#include "sub_main/00443A70.inc"
};
UvScroll gAreaPropData_92_0[4] = {
#include "sub_main/00443AB0.inc"
};
PropRec gAreaProps_92[3] = {
#include "sub_main/00443AC0.inc"
};
UvScroll gAreaPropData_93_1[2] = {
#include "sub_main/00443AE8.inc"
};
PropRec gAreaProps_93[4] = {
#include "sub_main/00443AF0.inc"
};
UvScroll gAreaPropData_94_1[2] = {
#include "sub_main/00443B20.inc"
};
f32 gAreaPropData_94_2_Pos[3] = {
#include "sub_main/00443B28.inc"
};
AnimScrollPos gAreaPropData_94_2[1] = {
    {&gAreaPropData_94_1[1], gAreaPropData_94_2_Pos},
};
PropRec gAreaProps_94[4] = {
#include "sub_main/00443B40.inc"
};
UvScroll gAreaPropData_6_0[2] = {
#include "sub_main/00443B70.inc"
};
UvScroll gAreaPropData_6_2[2] = {
#include "sub_main/00443B78.inc"
};
UvScroll gAreaPropData_6_4[4] = {
#include "sub_main/00443B80.inc"
};
PropRec gAreaProps_6[6] = {
#include "sub_main/00443B90.inc"
};
UvScroll gAreaPropData_96_0[2] = {
#include "sub_main/00443BD8.inc"
};
PropRec gAreaProps_96[2] = {
#include "sub_main/00443BE0.inc"
};
PropRec gAreaProps_97[2] = {
#include "sub_main/00443C00.inc"
};
UvScroll gAreaPropData_98_0[2] = {
#include "sub_main/00443C18.inc"
};
UvScroll gAreaPropData_98_2[4] = {
#include "sub_main/00443C20.inc"
};
PropRec gAreaProps_98[4] = {
#include "sub_main/00443C30.inc"
};
AnimSpin gAreaPropData_99_3[1] = {
#include "sub_main/00443C60.inc"
};
UvScroll gAreaPropData_99_0[2] = {
#include "sub_main/00443C70.inc"
};
UvScroll gAreaPropData_99_2[2] = {
#include "sub_main/00443C78.inc"
};
PropRec gAreaProps_99[6] = {
#include "sub_main/00443C80.inc"
};
UvScroll gAreaPropData_15_1[4] = {
#include "sub_main/00443CD0.inc"
};
PropRec gAreaProps_15[6] = {
#include "sub_main/00443CE0.inc"
};
UvScroll gAreaPropData_107_0[4] = {
#include "sub_main/00443D30.inc"
};
PropRec gAreaProps_107[5] = {
#include "sub_main/00443D40.inc"
};
AnimSpin gAreaPropData_108_3[1] = {
#include "sub_main/00443D80.inc"
};
UvScroll gAreaPropData_108_0[2] = {
#include "sub_main/00443D90.inc"
};
UvScroll gAreaPropData_108_2[2] = {
#include "sub_main/00443D98.inc"
};
PropRec gAreaProps_108[6] = {
#include "sub_main/00443DA0.inc"
};
UvScroll gAreaPropData_110_2[4] = {
#include "sub_main/00443DF0.inc"
};
UvScroll gAreaPropData_110_6[4] = {
#include "sub_main/00443E00.inc"
};
AnimSpin gAreaPropData_110_1[1] = {
#include "sub_main/00443E10.inc"
};
AnimSpin gAreaPropData_110_0[1] = {
#include "sub_main/00443E20.inc"
};
PropRec gAreaProps_110[10] = {
#include "sub_main/00443E30.inc"
};
AnimSpin gAreaPropData_111_0[1] = {
#include "sub_main/00443EB0.inc"
};
PropRec gAreaProps_111[2] = {
#include "sub_main/00443EC0.inc"
};
PropRec gAreaProps_3[2] = {
#include "sub_main/00443EE0.inc"
};
AnimSpin gAreaPropData_4_0[1] = {
#include "sub_main/00443F00.inc"
};
PropRec gAreaProps_4[3] = {
#include "sub_main/00443F10.inc"
};
UvScroll gAreaPropData_5_1[2] = {
#include "sub_main/00443F38.inc"
};
AnimSpin gAreaPropData_5_0[1] = {
#include "sub_main/00443F40.inc"
};
PropRec gAreaProps_5[3] = {
#include "sub_main/00443F50.inc"
};
UvScroll gAreaPropData_115_0[2] = {
#include "sub_main/00443F78.inc"
};
AnimSpin gAreaPropData_115_2[1] = {
#include "sub_main/00443F80.inc"
};
PropRec gAreaProps_115[5] = {
#include "sub_main/00443F90.inc"
};
AnimSpin gAreaPropData_116_1[1] = {
#include "sub_main/00443FD0.inc"
};
PropRec gAreaProps_116[3] = {
#include "sub_main/00443FE0.inc"
};
UvScroll gAreaPropData_22_1[2] = {
#include "sub_main/00444008.inc"
};
PropRec gAreaProps_22[4] = {
#include "sub_main/00444010.inc"
};
AnimSpin gAreaPropData_118_0[1] = {
#include "sub_main/00444040.inc"
};
PropRec gAreaProps_118[2] = {
#include "sub_main/00444050.inc"
};
f32 gAreaPropData_26_0[1][3] = {
#include "sub_main/00444068.inc"
};
PropRec gAreaProps_26[2] = {
#include "sub_main/00444080.inc"
};
AnimSpin gAreaPropData_120_0[1] = {
#include "sub_main/004440A0.inc"
};
PropRec gAreaProps_120[4] = {
#include "sub_main/004440B0.inc"
};
AnimSpin gAreaPropData_121_2[1] = {
#include "sub_main/004440E0.inc"
};
UvScroll gAreaPropData_121_0[4] = {
#include "sub_main/004440F0.inc"
};
PropRec gAreaProps_121[4] = {
#include "sub_main/00444100.inc"
};
UvScroll gAreaPropData_122_0[4] = {
#include "sub_main/00444130.inc"
};
PropRec gAreaProps_122[2] = {
#include "sub_main/00444140.inc"
};
UvScroll gAreaPropData_123_0[2] = {
#include "sub_main/00444158.inc"
};
PropRec gAreaProps_123[3] = {
#include "sub_main/00444160.inc"
};
UvScroll gAreaPropData_126_0[2] = {
#include "sub_main/00444188.inc"
};
PropRec gAreaProps_126[4] = {
#include "sub_main/00444190.inc"
};
UvScroll gAreaPropData_127_0[2] = {
#include "sub_main/004441C0.inc"
};
UvScroll gAreaPropData_127_2[2] = {
#include "sub_main/004441C8.inc"
};
PropRec gAreaProps_127[5] = {
#include "sub_main/004441D0.inc"
};
PropRec gAreaProps_131[8] = {
#include "sub_main/00444210.inc"
};
UvScroll gAreaPropData_136_0_Uv[2] = {
#include "sub_main/00444270.inc"
};
f32 gAreaPropData_136_0_Pos[3] = {
#include "sub_main/00444278.inc"
};
PropPlaceY gAreaPropData_136_1[1] = {
#include "sub_main/00444290.inc"
};
PropPlaceY gAreaPropData_136_2[1] = {
#include "sub_main/004442A0.inc"
};
PropPlaceY gAreaPropData_136_3[1] = {
#include "sub_main/004442B0.inc"
};
PropPlaceY gAreaPropData_136_4[1] = {
#include "sub_main/004442C0.inc"
};
PropPlaceY gAreaPropData_136_5[2] = {
#include "sub_main/004442D0.inc"
};
PropPlaceY gAreaPropData_136_6[2] = {
#include "sub_main/004442F0.inc"
};
AnimScrollPos gAreaPropData_136_0[1] = {
    {gAreaPropData_136_0_Uv, gAreaPropData_136_0_Pos},
};
PropRec gAreaProps_136[8] = {
#include "sub_main/00444320.inc"
};
AnimSpin gAreaPropData_137_0[1] = {
#include "sub_main/00444380.inc"
};
AnimRot gAreaPropData_137_2[6] = {
#include "sub_main/00444390.inc"
};
PropPlaceYZ gAreaPropData_137_3[1] = {
#include "sub_main/004443F0.inc"
};
PropRec gAreaProps_137[6] = {
#include "sub_main/00444410.inc"
};
AnimSpin gAreaPropData_140_0[1] = {
#include "sub_main/00444460.inc"
};
AnimSpin gAreaPropData_140_1[1] = {
#include "sub_main/00444470.inc"
};
PropRec gAreaProps_140[4] = {
#include "sub_main/00444480.inc"
};
AnimSpin gAreaPropData_141_0[1] = {
#include "sub_main/004444B0.inc"
};
AnimSpin2 gAreaPropData_141_1[1] = {
#include "sub_main/004444C0.inc"
};
f32 gAreaPropData_141_2[1][3] = {
#include "sub_main/004444D0.inc"
};
PropRec gAreaProps_141[4] = {
#include "sub_main/004444E0.inc"
};
PropRec gAreaProps_142[4] = {
#include "sub_main/00444510.inc"
};
AnimSpin gAreaPropData_143_3[1] = {
#include "sub_main/00444540.inc"
};
PropRec gAreaProps_143[5] = {
#include "sub_main/00444550.inc"
};
AnimSpin gAreaPropData_145_0[1] = {
#include "sub_main/00444590.inc"
};
f32 gAreaPropData_145_2[6] = {
#include "sub_main/004445A0.inc"
};
UvScroll gAreaPropData_145_3_Uv[2] = {
#include "sub_main/004445B8.inc"
};
AnimScrollPos gAreaPropData_145_3[2] = {
    {gAreaPropData_145_3_Uv, &gAreaPropData_145_2[3]},
    {NULL, NULL},
};
AnimSpin2 gAreaPropData_145_1[1] = {
#include "sub_main/004445D0.inc"
};
PropRec gAreaProps_145[6] = {
#include "sub_main/004445E0.inc"
};
AnimSpin gAreaPropData_146_0[1] = {
#include "sub_main/00444630.inc"
};
f32 gAreaPropData_146_2[2][3] = {
#include "sub_main/00444640.inc"
};
AnimSpin2 gAreaPropData_146_1[1] = {
#include "sub_main/00444660.inc"
};
PropRec gAreaProps_146[6] = {
#include "sub_main/00444670.inc"
};
AnimSpin gAreaPropData_148_0[1] = {
#include "sub_main/004446C0.inc"
};
PropRec gAreaProps_148[2] = {
#include "sub_main/004446D0.inc"
};
UvScroll gAreaPropData_149_1[2] = {
#include "sub_main/004446E8.inc"
};
AnimSpin gAreaPropData_149_0[1] = {
#include "sub_main/004446F0.inc"
};
PropRec gAreaProps_149[3] = {
#include "sub_main/00444700.inc"
};
UvScroll gAreaPropData_158_0[2] = {
#include "sub_main/00444728.inc"
};
PropRec gAreaProps_158[4] = {
#include "sub_main/00444730.inc"
};
UvScroll gAreaPropData_159_0[4] = {
#include "sub_main/00444760.inc"
};
PropRec gAreaProps_159[2] = {
#include "sub_main/00444770.inc"
};
UvScroll gAreaPropData_160_0_Uv[2] = {
#include "sub_main/00444788.inc"
};
AnimScrollRot gAreaPropData_160_0[2] = {
    {gAreaPropData_160_0_Uv, D_0092ABF0},
    {gAreaPropData_160_0_Uv, D_0092AC00},
};
AnimScrollRot gAreaPropData_160_1[3] = {
    {&gAreaPropData_160_0_Uv[1], D_0092AC10},
    {&gAreaPropData_160_0_Uv[1], D_0092AC20},
    {&gAreaPropData_160_0_Uv[1], D_0092AC30},
};
PropRec gAreaProps_160[5] = {
#include "sub_main/004447C0.inc"
};
UvScroll gAreaPropData_59_0[2] = {
#include "sub_main/00444800.inc"
};
UvScroll gAreaPropData_59_2[2] = {
#include "sub_main/00444808.inc"
};
PropRec gAreaProps_59[4] = {
#include "sub_main/00444810.inc"
};
UvScroll gAreaPropData_163_0[4] = {
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
UvScroll gAreaPropData_166_0[2] = {
#include "sub_main/004448B8.inc"
};
PropRec gAreaProps_166[3] = {
#include "sub_main/004448C0.inc"
};
s16 gAreaPropData_170_0[4] = {
#include "sub_main/004448E8.inc"
};
PropRec gAreaProps_170[4] = {
#include "sub_main/004448F0.inc"
};
s16 gAreaPropData_171_0[8] = {
#include "sub_main/00444920.inc"
};
PropRec gAreaProps_171[4] = {
#include "sub_main/00444930.inc"
};
s16 gAreaPropData_172_0[8] = {
#include "sub_main/00444960.inc"
};
PropRec gAreaProps_172[4] = {
#include "sub_main/00444970.inc"
};
u8 gAreaPropData_173_2[16] = {
#include "sub_main/004449A0.inc"
};
AnimSpin gAreaPropData_173_0[1] = {
#include "sub_main/004449B0.inc"
};
PropRec gAreaProps_173[9] = {
#include "sub_main/004449C0.inc"
};
AnimSpin gAreaPropData_174_0[1] = {
#include "sub_main/00444A30.inc"
};
UvScroll gAreaPropData_174_6[4] = {
#include "sub_main/00444A40.inc"
};
PropRec gAreaProps_174[12] = {
#include "sub_main/00444A50.inc"
};
AnimSpin gAreaPropData_175_0[1] = {
#include "sub_main/00444AE0.inc"
};
PropRec gAreaProps_175[11] = {
#include "sub_main/00444AF0.inc"
};
UvScroll gAreaPropData_176_0[2] = {
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
    NULL, NULL, NULL, gAreaProps_15,
    NULL, NULL, gAreaProps_18, gAreaProps_19,
    gAreaProps_20, NULL, gAreaProps_22, gAreaProps_23,
    gAreaProps_24, NULL, gAreaProps_26, NULL,
    gAreaProps_28, NULL, NULL, NULL,
    NULL, gAreaProps_33, gAreaProps_34, NULL,
    gAreaProps_36, NULL, gAreaProps_38, gAreaProps_39,
    NULL, NULL, NULL, gAreaProps_43,
    NULL, gAreaProps_45, gAreaProps_46, gAreaProps_47,
    gAreaProps_48, gAreaProps_49, NULL, gAreaProps_51,
    gAreaProps_52, NULL, gAreaProps_54, gAreaProps_55,
    NULL, gAreaProps_57, NULL, gAreaProps_59,
    gAreaProps_60, gAreaProps_61, gAreaProps_62, gAreaProps_63,
    NULL, gAreaProps_65, gAreaProps_66, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, gAreaProps_75,
    NULL, NULL, NULL, NULL,
    gAreaProps_28, NULL, NULL, NULL,
    gAreaProps_84, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    gAreaProps_92, gAreaProps_93, gAreaProps_94, gAreaProps_6,
    gAreaProps_96, gAreaProps_97, gAreaProps_98, gAreaProps_99,
    NULL, gAreaProps_92, gAreaProps_93, gAreaProps_94,
    gAreaProps_15, NULL, NULL, gAreaProps_107,
    gAreaProps_108, NULL, gAreaProps_110, gAreaProps_111,
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
f32 gVillagePropData_0_6[5][3] = {
#include "sub_main/00444EE0.inc"
};
PropRec gVillagePropChoices_0_0[8] = {
#include "sub_main/00444F20.inc"
};
AnimSpin gVillagePropData_3_4[1] = {
#include "sub_main/00444F80.inc"
};
f32 gVillagePropData_3_5[4][3] = {
#include "sub_main/00444F90.inc"
};
PropRec gVillagePropChoices_0_3[8] = {
#include "sub_main/00444FC0.inc"
};
f32 gVillagePropData_6_4[4][3] = {
#include "sub_main/00445020.inc"
};
PropRec gVillagePropChoices_0_6[8] = {
#include "sub_main/00445050.inc"
};
UvScroll gVillagePropData_1_3[4] = {
#include "sub_main/004450B0.inc"
};
PropRec gVillagePropChoices_0_1[8] = {
#include "sub_main/004450C0.inc"
};
f32 gVillagePropData_4_6[2][3] = {
#include "sub_main/00445120.inc"
};
PropRec gVillagePropChoices_0_4[8] = {
#include "sub_main/00445140.inc"
};
PropRec gVillagePropChoices_0_7[5] = {
#include "sub_main/004451A0.inc"
};
PropPlaceY gVillagePropData_2_4[1] = {
#include "sub_main/004451E0.inc"
};
PropPlaceY gVillagePropData_5_6[1] = {
#include "sub_main/004451F0.inc"
};
PropPlaceY gVillagePropData_8_7[1] = {
#include "sub_main/00445200.inc"
};
AnimSpin gVillagePropData_5_10[1] = {
#include "sub_main/00445210.inc"
};
PropRec gVillagePropChoices_0_2[9] = {
#include "sub_main/00445220.inc"
};
PropPlaceXYZ gVillagePropData_5_9[3] = {
#include "sub_main/00445290.inc"
};
PropRec gVillagePropChoices_0_5[12] = {
#include "sub_main/004452E0.inc"
};
f32 gVillagePropData_8_3[10][3] = {
#include "sub_main/00445370.inc"
};
AnimRot gVillagePropData_8_10[3] = {
#include "sub_main/004453F0.inc"
};
PropRec gVillagePropChoices_0_8[13] = {
#include "sub_main/00445420.inc"
};
PropChoice gVillagePropChoices_0[10] = {
#include "sub_main/004454C0.inc"
};
AreaPropChoices gVillagePropChoices[2] = {
#include "sub_main/00445510.inc"
};
char *gDefaultStageHeader_12[8] = {
    gDefaultStageHeader_12_0, gDefaultStageHeader_12_1, gDefaultStageHeader_12_2, gDefaultStageHeader_12_3,
    gDefaultStageHeader_12_4, gEmClassNames_0, NULL, NULL,
};
char *gReturn20sTownText[2] = {
    gReturn20sTownText_0, gReturn20sTownText_1,
};
char *gReturn1mTownText[2] = {
    gReturn1mTownText_0, gReturn1mTownText_1,
};
char *gQuestClearText[2] = {
    gQuestClearText_0, gQuestClearText_1,
};
char *gTimeLeftMinFmt[2] = {
    gTimeLeftMinFmt_0, gTimeLeftMinFmt_1,
};
char *gQuestNoticeTexts[8] = {
    gQuestNoticeTexts_0, gQuestNoticeTexts_1, gQuestNoticeTexts_2, gQuestNoticeTexts_3,
    gQuestNoticeTexts_4, gQuestNoticeTexts_5, gQuestNoticeTexts_6, gQuestNoticeTexts_7,
};
char *gSubBSupplyText[4] = {
    gSubBSupplyText_0, gSubBSupplyText_1, gSubBSupplyText_2, gSubBSupplyText_3,
};
char *gMustOrderAllText[2] = {
    gMustOrderAllText_0, gMustOrderAllText_1,
};
char *gReturn10sTownText[2] = {
    gReturn10sTownText_0, gReturn10sTownText_1,
};
char *gDrillScoreTexts[16] = {
    D_00490EE0, gDrillScoreTexts_1, gDrillScoreTexts_2, gDrillScoreTexts_3,
    D_00490EE0, D_00490EE0, D_00490EE0, D_00490EE0,
    D_00490EE0, D_00490EE0, D_00490EE0, gDrillScoreTexts_11,
    D_00490EE0, gDrillScoreTexts_13, gDrillScoreTexts_14, gDrillScoreTexts_15,
};
char *gElderFledText[4] = {
    gElderFledText_0, NULL, NULL, NULL,
};
char *gEmClassNames[12] = {
    gEmClassNames_0, gEmClassNames_1, gEmClassNames_2, gEmClassNames_3,
    gEmClassNames_4, gEmClassNames_5, gEmClassNames_6, gEmClassNames_7,
    gEmClassNames_8, gEmClassNames_9, NULL, NULL,
};
char *gMonsterNames[76] = {
    gMonsterNames_0, gMonsterNames_1, D_00491078, gMonsterNames_3,
    gMonsterNames_4, gMonsterNames_5, gMonsterNames_6, D_004910B8,
    gMonsterNames_8, gMonsterNames_9, gMonsterNames_10, gMonsterNames_11,
    gMonsterNames_12, gMonsterNames_13, gMonsterNames_14, gMonsterNames_15,
    D_00491140, gMonsterNames_17, gMonsterNames_18, gMonsterNames_19,
    gMonsterNames_20, gMonsterNames_21, gMonsterNames_22, gMonsterNames_23,
    gMonsterNames_24, gMonsterNames_25, gMonsterNames_26, gMonsterNames_27,
    gMonsterNames_28, gMonsterNames_29, gMonsterNames_30, gMonsterNames_31,
    gMonsterNames_32, gMonsterNames_33, gMonsterNames_34, D_00491140,
    D_00491078, D_00491260, gMonsterNames_38, gMonsterNames_39,
    gMonsterNames_40, D_004912D0, D_00491260, gMonsterNames_43,
    gMonsterNames_44, gMonsterNames_45, gMonsterNames_46, gMonsterNames_47,
    gMonsterNames_48, D_004912D0, D_004910B8, gMonsterNames_51,
    gMonsterNames_52, gMonsterNames_53, D_004913D8, gMonsterNames_55,
    gMonsterNames_56, gMonsterNames_57, D_00491418, gMonsterNames_59,
    D_004913D8, gMonsterNames_61, gMonsterNames_62, gMonsterNames_63,
    gMonsterNames_64, gMonsterNames_65, gMonsterNames_66, gMonsterNames_67,
    gMonsterNames_68, gMonsterNames_69, gMonsterNames_70, D_00491078,
    D_00491418, gMonsterNames_73, NULL, NULL,
};
char *gFieldMenuItemsOnline[2][7] = {
    {D_00491500, D_00491510, D_00491518, D_00491528, D_00491538, D_00491548, D_00491558},
    {D_00491500, gFieldMenuItemsOnline_8, D_00491518, D_00491528, D_00491538, D_00491548, D_00491558},
};
char *gFieldMenuItemsOffline[5][9] = {
    {D_00491500, D_00491510, D_00491518, D_00491528, D_00491538, D_00491578, D_00491558, D_00491588, D_00491598},
    {D_00491500, D_004915A8, D_004915B8, D_00491528, D_00491538, D_00491578, D_00491558, D_00491588, D_00491598},
    {D_00491500, D_00491510, D_00491518, D_00491528, D_00491538, D_00491578, D_00491558, D_004915C8, D_00491598},
    {D_00491500, D_004915A8, D_004915B8, D_00491528, D_00491538, D_00491578, D_00491558, D_004915C8, D_00491598},
    {gFieldMenuItemsOffline_36, D_00491510, D_004915B8, D_00491528, D_00491538, D_00491578, D_00491558, gFieldMenuItemsOffline_43, D_00491598},
};
char *gPouchMenuPanel2_6[2] = {
    gPouchMenuPanel2_6_0, gPouchMenuPanel2_6_1,
};
char *gDeliveryPanel_7[2] = {
    gDeliveryPanel_7_0, D_00491610,
};
char *gYesNoTexts[2] = {
    D_00491620, gYesNoTexts_1,
};
char *gItemActionLabels[3][3] = {
    {D_00491660, D_00491668, D_00491670},
    {D_00491660, gItemActionLabels_4, gItemActionLabels_5},
    {gItemActionLabels_6, D_00491668, D_00491670},
};
char *D_00445888[2] = {
    D_004916B0, D_00491660,
};
char *gStatusMenuColors_1[6] = {
    D_004916C0, D_004916D0, gStatusMenuColors_1_2, gStatusMenuColors_1_3,
    gStatusMenuColors_1_4, NULL,
};
char *gInfoMenuLabelsOffline[2] = {
    D_00491728, D_00491740,
};
char *gInfoMenuLabels[4] = {
    gInfoMenuLabels_0, D_00491728, D_00491740, gInfoMenuLabels_3,
};
char *gCockpitToggleTexts[4] = {
    gCockpitToggleTexts_0, gCockpitToggleTexts_1, NULL, NULL,
};
char *gQuestStatusTitle[2] = {
    gQuestStatusTitle_0, gQuestStatusTitle_1,
};
char *gQuestMenuRecordTitles[2] = {
    gQuestMenuRecordTitles_0, gQuestMenuRecordTitles_1,
};
char *gQuestMenuTitles[2] = {
    gQuestMenuTitles_0, gQuestMenuTitles_1,
};
char *gQuestMenuStatusTexts[2] = {
    gQuestMenuStatusTexts_0, gQuestMenuStatusTexts_1,
};
char *gQuestMenuTargetTexts[2] = {
    gQuestMenuTargetTexts_0, gQuestMenuTargetTexts_1,
};
char *gDojoNoRecordText[2] = {
    gDojoNoRecordText_0, gDojoNoRecordText_1,
};
char *gQuestInfoFormats[8] = {
    D_00491900, gQuestInfoFormats_1, gQuestInfoFormats_2, gQuestInfoFormats_3,
    D_00491950, gQuestInfoFormats_5, NULL, NULL,
};
char *gQuestMenuItemLists[10][17] = {
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gQuestMenuItemLists_16},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gQuestMenuItemLists_21, gEmClassNames_0, D_004919C8, gEmClassNames_0, gQuestMenuItemLists_25, gEmClassNames_0, gEmClassNames_0, gQuestMenuItemLists_28, gEmClassNames_0, gEmClassNames_0, D_00491A18, NULL, NULL},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A28, D_00491A38, gQuestMenuItemLists_40, gQuestMenuItemLists_41, gQuestMenuItemLists_42, gEmClassNames_0, D_00491A78, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A88, NULL, NULL},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A98, D_00491AA8, gEmClassNames_0, D_00491AB8, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gQuestMenuItemLists_72, gQuestMenuItemLists_73, gQuestMenuItemLists_74, gQuestMenuItemLists_75, gQuestMenuItemLists_76, gEmClassNames_0, D_00491A78, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A88, NULL, NULL},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, D_00491A98, D_00491AA8, gQuestMenuItemLists_90, gQuestMenuItemLists_91, D_00491AB8, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gQuestMenuItemLists_105, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A28, D_00491A38, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A78, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A88, NULL, NULL},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gQuestMenuItemLists_140, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_004919C8, gEmClassNames_0, D_00491A18, NULL, NULL, NULL, NULL, NULL},
    {D_00491988, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A28, D_00491A38, gQuestMenuItemLists_159, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A78, gEmClassNames_0, gEmClassNames_0, gEmClassNames_0, D_00491A88, NULL, NULL},
};
char *gQuestTargetLabels[3] = {
    gQuestTargetLabels_0, D_00491950, gQuestTargetLabels_2,
};
#pragma explicit_zero_data on
u32 D_00445BD8[2] = {
    0, 0,
};
#pragma explicit_zero_data reset
char *gCombineSelectFmt[2] = {
    gCombineSelectFmt_0, gEmClassNames_0,
};
char *gCombineInfoLabels[2] = {
    gCombineInfoLabels_0, gCombineInfoLabels_1,
};
char *gCombineRateFmt[2] = {
    gCombineRateFmt_0, gCombineRateFmt_1,
};
char *gCombineSelectingText[2] = {
    gCombineSelectingText_0, gCombineSelectingText_1,
};
char *gCombineFailLabels[2] = {
    gCombineFailLabels_0, NULL,
};
char *gCombine2Text[2] = {
    gCombine2Text_0, gCombine2Text_1,
};
char *gCombine2MaterialLabels[4] = {
    D_00491D28, D_00491D38, NULL, NULL,
};
char *gCombine3MaterialLabels[4] = {
    D_00491D28, D_00491D38, gCombine3MaterialLabels_2, NULL,
};
char *gCombineDoText[2] = {
    gCombineDoText_0, D_00491610,
};
char *gPouchInfiniteFmt[2] = {
    D_00491D68, NULL,
};
char *gAmmoTensFmt[2] = {
    gAmmoTensFmt_0, NULL,
};
char *gItemFullText[2] = {
    gItemFullText_0, gItemFullText_1,
};
char *gFieldMenuTitle[2] = {
    gFieldMenuTitle_0, NULL,
};
char *gPausedText[2] = {
    gPausedText_0, NULL,
};
char *gPhraseEditFmt[2] = {
    gPhraseEditFmt_0, NULL,
};
char *gChatMenuPanel_7[2] = {
    gChatMenuPanel_7_0, D_00491578,
};
char *gSupplySelectText[2] = {
    gSupplySelectText_0, gSupplySelectText_1,
};
char *gSupplyCannotCarryText[2] = {
    gSupplyCannotCarryText_0, D_00491D68,
};
char *gSupplyCountFmt[4] = {
    gSupplyCountFmt_0, NULL, NULL, NULL,
};
char *gTentMenuLabels[18] = {
    D_00491E90, D_00491EB0, D_00491ED0, D_00491E90,
    D_00491EB0, D_00491ED0, D_00491E90, D_00491EF0,
    D_00491ED0, D_00491E90, D_00491EF0, D_00491ED0,
    D_00491E90, D_00491F10, D_00491ED0, D_00491E90,
    D_00491F10, D_00491ED0,
};
char *gSubTargetHelpText[2] = {
    gSubTargetHelpText_0, gSubTargetHelpText_1,
};
char *gQuestMenuHelpTexts[4] = {
    gQuestMenuHelpTexts_0, gQuestMenuHelpTexts_1, NULL, NULL,
};
char *gDeliveryMenuTexts[4] = {
    gDeliveryMenuTexts_0, gDeliveryMenuTexts_1, gDeliveryMenuTexts_2, gDeliveryMenuTexts_3,
};
char *gDeliveryDoneText[2] = {
    gDeliveryDoneText_0, gDeliveryDoneText_1,
};
char *gDeliveryHelpText[2] = {
    gDeliveryHelpText_0, NULL,
};
char *gCombineListFmt[2] = {
    gCombineListFmt_0, gCombineListFmt_1,
};
char *gItemListLineTexts[2] = {
    gItemListLineTexts_0, gItemListLineTexts_1,
};
char *gRecipeMaterialsFmt[2] = {
    gRecipeMaterialsFmt_0, gRecipeMaterialsFmt_1,
};
char *gRecipeSwitchHint[2] = {
    gRecipeSwitchHint_0, NULL,
};
char *gRecipeMaterialFmts[4] = {
    gRecipeMaterialFmts_0, gRecipeMaterialFmts_1, gRecipeMaterialFmts_2, NULL,
};
char gRecipeRateLines[16] = {
#include "sub_main/00445D40.inc"
};
char *gRecipeRateLabels[8] = {
    gRecipeRateLabels_0, gRecipeRateLabels_1, gRecipeRateLabels_2, gRecipeRateLabels_3,
    gRecipeRateLabels_4, gRecipeRateLabels_5, NULL, NULL,
};
char *gRecipeYieldLabels[12] = {
    gRecipeYieldLabels_0, gRecipeYieldLabels_1, D_00492488, D_00492488,
    gRecipeYieldLabels_4, D_00492498, gRecipeYieldLabels_6, D_00492498,
    gRecipeYieldLabels_8, NULL, NULL, NULL,
};
char *gMonsterListTitle[4] = {
    gMonsterListTitle_0, gMonsterListTitle_1, gMonsterListTitle_2, gMonsterListTitle_3,
};
char *gMonsterNoSizeText[2] = {
    gMonsterNoSizeText_0, gMonsterNoSizeText_1,
};
char *gArenaBaitNoBookText[2] = {
    gArenaBaitNoBookText_0, gArenaBaitNoBookText_1,
};
char *gMonsterNoteLabels[2] = {
    gMonsterNoteLabels_0, gMonsterNoteLabels_1,
};
char *gArenaBaitHint[2] = {
    gArenaBaitHint_0, gArenaBaitHint_1,
};
char *gArenaBaitLabels[2] = {
    gArenaBaitLabels_0, D_00492620,
};
char *gMonsterNoteValueFmt[2] = {
    gMonsterNoteValueFmt_0, NULL,
};
char *gEmDescTexts[52] = {
    gEmDescTexts_0, gEmDescTexts_1, gEmDescTexts_2, gEmDescTexts_3,
    gEmDescTexts_4, gEmDescTexts_5, gEmDescTexts_6, gEmDescTexts_7,
    gEmDescTexts_8, gEmDescTexts_9, gEmDescTexts_10, gEmDescTexts_11,
    gEmDescTexts_12, gEmDescTexts_13, gEmDescTexts_14, gEmDescTexts_15,
    gEmDescTexts_16, gEmDescTexts_17, gEmDescTexts_18, gEmDescTexts_19,
    gEmDescTexts_20, gEmDescTexts_21, gEmDescTexts_22, gEmDescTexts_23,
    gEmDescTexts_24, gEmDescTexts_25, gEmDescTexts_26, gEmDescTexts_27,
    gEmDescTexts_28, gEmDescTexts_29, gEmDescTexts_30, gEmDescTexts_31,
    gEmDescTexts_32, gEmDescTexts_33, gEmDescTexts_34, gEmDescTexts_35,
    gEmDescTexts_36, gEmDescTexts_37, gEmDescTexts_38, gEmDescTexts_39,
    gEmDescTexts_40, gEmDescTexts_41, gEmDescTexts_42, gEmDescTexts_43,
    gEmDescTexts_44, gEmDescTexts_45, gEmDescTexts_46, gEmDescTexts_47,
    gEmDescTexts_48, gEmDescTexts_49, gEmDescTexts_50, gEmDescTexts_51,
};
char *gEquipStatusTitle[2] = {
    gEquipStatusTitle_0, NULL,
};
char *gOptionMenuTitle[2] = {
    gOptionMenuTitle_0, NULL,
};
char *gFieldOptionLabels[2][9] = {
    {D_00495588, gFieldOptionLabels_1, D_004955B0, D_004955C8, D_004955D8, D_004955E8, D_004955F8, D_00495608, D_00495620},
    {D_00495588, gFieldOptionLabels_10, D_004955B0, D_004955C8, D_004955D8, D_004955E8, D_004955F8, D_00495608, D_00495620},
};
char *gFieldOptionValues[9][3] = {
    {D_00495658, D_00495660, gEmClassNames_0},
    {D_00495658, D_00495660, gEmClassNames_0},
    {D_00495668, D_00495670, D_00495680},
    {gFieldOptionValues_9, gFieldOptionValues_10, gEmClassNames_0},
    {D_00495668, D_00495670, D_00495680},
    {D_00495660, D_00495658, gEmClassNames_0},
    {gFieldOptionValues_18, gFieldOptionValues_19, gFieldOptionValues_20},
    {gFieldOptionValues_21, gFieldOptionValues_22, gEmClassNames_0},
    {D_00495658, D_00495660, gEmClassNames_0},
};
char *gRarityFmt[8] = {
    gRarityFmt_0, NULL, D_00491548, gChatTypeTexts_0,
    D_004956D0, D_004956E8, D_004956F8, NULL,
};
char *gChatRecvText[4] = {
    gChatRecvText_0, gChatRecvText_1, NULL, NULL,
};
char *gTalkEventTitle[2] = {
    gTalkEventTitle_0, D_00491620,
};
char *gNoticeNoSelText[2] = {
    gNoticeNoSelText_0, gNoticeNoSelText_1,
};
char *gItemListTitles[2] = {
    D_00491500, gItemListTitles_1,
};
char *gItemListTitleFmt[2] = {
    gItemListTitleFmt_0, D_004957A0,
};
char *gInfinityText[4] = {
    gInfinityText_0, NULL, NULL, NULL,
};
char *gStatusHeadLabels[4] = {
    gStatusHeadLabels_0, gStatusHeadLabels_1, gStatusHeadLabels_2, gStatusHeadLabels_3,
};
char *gSymbolColorLabel[2] = {
    gSymbolColorLabel_0, gSymbolColorLabel_1,
};
char *gArenaRecordLabels[2] = {
    gArenaRecordLabels_0, gArenaRecordLabels_1,
};
char *gArenaRecordFmt[4] = {
    gArenaRecordFmt_0, gArenaRecordFmt_1, NULL, NULL,
};
char *gStatusStatLabels[8] = {
    D_00495960, gStatusStatLabels_0, gStatusStatLabels_1, gStatusStatLabels_2,
    gStatusStatLabels_3, NULL, NULL, NULL,
};
char *gWeaponTypeNames[12] = {
    gWeaponTypeNames_0, gWeaponTypeNames_1, gWeaponTypeNames_2, gWeaponTypeNames_3,
    gWeaponTypeNames_4, gWeaponTypeNames_5, gWeaponTypeNames_6, gWeaponTypeNames_7,
    gWeaponTypeNames_8, gWeaponTypeNames_9, gWeaponTypeNames_10, NULL,
};
char *gStatusBoxTitles[8] = {
    gStatusBoxTitles_0, gEmClassNames_0, D_00495A88, D_00495A88,
    D_00495A88, D_00495A88, D_00495A88, D_00495A88,
};
char *gStatusPageFmt[2] = {
    gStatusPageFmt_0, D_00495AD0,
};
char *gRankPointsFmt[2] = {
    gRankPointsFmt_0, gRankPointsFmt_1,
};
char *gSkillNameFmt[2] = {
    gSkillNameFmt_0, gSkillNameFmt_1,
};
char *gSkillPartPtsFmt[2] = {
    gSkillPartPtsFmt_0, gSkillPartPtsFmt_1,
};
char *gSkillRowFmt[6] = {
    gSkillRowFmt_0, gSkillRowFmt_1, gSkillRowFmt_2, D_00495BA8,
    D_00495BC0, D_00495BD0,
};
char *gPointsFmt[2] = {
    D_00491900, NULL,
};
char *gSkillListHeader[2] = {
    gSkillListHeader_0, NULL,
};
char *gSkillPartMarks[2] = {
    gSkillPartMarks_0, gSkillPartMarks_1,
};
char *gEquipWeaponFmt[2] = {
    D_00495C38, D_00495C38,
};
char *gUnknownEquipText[2] = {
    gUnknownEquipText_0, gUnknownEquipText_1,
};
char *gHunterTypeLabels[4] = {
    D_00495C70, D_00495C88, D_00495C98, NULL,
};
char *gEquipTypeLabels[4] = {
    D_00495CB0, D_00495C88, D_00495C98, NULL,
};
char gBladeDetailLabels[16] = {
#include "sub_main/00446100.inc"
};
char *gBowgunDetailLabels[6] = {
#include "sub_main/00446110.inc"
};
char *gBowDetailLabels[2] = {
    D_00495CC8, NULL,
};
char *gArmorStatLabels[8] = {
    D_00495D20, D_00495D40, D_00495D60, gArmorStatLabels_0,
    NULL, NULL, NULL, NULL,
};
char *gGunReloadTexts[8] = {
    gGunReloadTexts_0, gGunReloadTexts_1, D_00495DA8, gGunReloadTexts_3,
    gGunReloadTexts_4, NULL, NULL, NULL,
};
char *gGunRecoilTexts[8] = {
    gGunRecoilTexts_0, gGunRecoilTexts_1, gGunRecoilTexts_2, gGunRecoilTexts_3,
    gGunRecoilTexts_4, gGunRecoilTexts_5, gGunRecoilTexts_6, NULL,
};
char *gHunterClassTexts[4] = {
    gHunterClassTexts_0, gHunterClassTexts_1, gHunterClassTexts_2, gHunterClassTexts_3,
};
char *gEquipNameFmt[2] = {
    gEquipNameFmt_0, gEquipNameFmt_1,
};
char *gScopeFixedText[2] = {
    gScopeFixedText_0, gScopeFixedText_1,
};
char *gPowerBarrelText[2] = {
    gPowerBarrelText_0, gPowerBarrelText_1,
};
char *gShieldText[2] = {
    gShieldText_0, gShieldText_1,
};
char *gAmmoHeaderText[6] = {
    gAmmoHeaderText_0, D_00495EF8, gAmmoHeaderText_2, gAmmoHeaderText_3,
    gAmmoHeaderText_4, gAmmoHeaderText_5,
};
char *gAmmoNameFmt[2] = {
    gAmmoNameFmt_0, gAmmoNameFmt_1,
};
char *gAmmoLineFormats[6] = {
    gAmmoLineFormats_0, gAmmoLineFormats_1, gAmmoLineFormats_2, gAmmoLineFormats_3,
    gAmmoLineFormats_4, gAmmoLineFormats_5,
};
char *gDecoDetailTitle[2] = {
    gDecoDetailTitle_0, gDecoDetailTitle_1,
};
char *gAffinityLabel[4] = {
    gAffinityLabel_0, gAffinityLabel_1, NULL, NULL,
};
char *gAmmoNames[20] = {
    gAmmoNames_0, gAmmoNames_1, gAmmoNames_2, gAmmoNames_3,
    gAmmoNames_4, gAmmoNames_5, gAmmoNames_6, gAmmoNames_7,
    gAmmoNames_8, gAmmoNames_9, gAmmoNames_10, gAmmoNames_11,
    gAmmoNames_12, gAmmoNames_13, gAmmoNames_14, gAmmoNames_15,
    gAmmoNames_16, gAmmoNames_17, NULL, NULL,
};
char *gElementNames[8] = {
    gElementNames_0, gElementNames_1, gElementNames_2, gElementNames_3,
    gElementNames_4, D_004961B0, D_004961B8, D_004961C0,
};
char *gSkillPointsTitle[2] = {
    gSkillPointsTitle_0, gSkillPointsTitle_1,
};
char *gNoSkillText[2] = {
    gNoSkillText_0, NULL,
};
char *gDecoDetailLabels[4] = {
    gDecoDetailLabels_0, gDecoDetailLabels_1, gDecoDetailLabels_2, NULL,
};
char *gRangeClassTexts[4] = {
    gRangeClassTexts_0, gRangeClassTexts_1, gRangeClassTexts_2, gRangeClassTexts_3,
};
char *gDecoSkillPtsFmt[2] = {
    gDecoSkillPtsFmt_0, gDecoSkillPtsFmt_1,
};
char *gDecoSkillFmt[2] = {
    gDecoSkillFmt_0, NULL,
};
char *gShellTypeTexts[4] = {
    gShellTypeTexts_0, gShellTypeTexts_1, gShellTypeTexts_2, NULL,
};
char *gBowShotTexts[12] = {
    gBowShotTexts_0, gBowShotTexts_1, gBowShotTexts_2, gBowShotTexts_3,
    gBowShotTexts_4, gBowShotTexts_5, gBowShotTexts_6, gBowShotTexts_7,
    gBowShotTexts_8, gBowShotTexts_9, gBowShotTexts_10, gBowShotTexts_11,
};
char *gTabPageFmt[2] = {
    D_00496328, gTabPageFmt_1,
};
char *gSortHintText[2] = {
    gSortHintText_0, D_00496340,
};
char *gSelectText[2] = {
    gSelectText_0, D_00492620,
};
char *gDecoModeFmt[2] = {
    gDecoModeFmt_0, NULL,
};
char *gCommentEditTitle[2] = {
    gCommentEditTitle_0, NULL,
};
char *gCommentEditText[2] = {
    D_00496378, NULL,
};
char *gChatTargetTexts[4] = {
    gChatTargetTexts_0, gChatTargetTexts_1, gChatTargetTexts_2, gChatTargetTexts_3,
};
char *gChatNgWordsA[44] = {
    gChatNgWordsA_0, gChatNgWordsA_1, gChatNgWordsA_2, gChatNgWordsA_3,
    gChatNgWordsA_4, gChatNgWordsA_5, gChatNgWordsA_6, gChatNgWordsA_7,
    gChatNgWordsA_8, gChatNgWordsA_9, gChatNgWordsA_10, gChatNgWordsA_11,
    gChatNgWordsA_12, gChatNgWordsA_13, gChatNgWordsA_14, gChatNgWordsA_15,
    gChatNgWordsA_16, gChatNgWordsA_17, gChatNgWordsA_18, gChatNgWordsA_19,
    gChatNgWordsA_20, gChatNgWordsA_21, gChatNgWordsA_22, gChatNgWordsA_23,
    gChatNgWordsA_24, gChatNgWordsA_25, gChatNgWordsA_26, gChatNgWordsA_27,
    gChatNgWordsA_28, gChatNgWordsA_29, gChatNgWordsA_30, gChatNgWordsA_31,
    gChatNgWordsA_32, gChatNgWordsA_33, gChatNgWordsA_34, gChatNgWordsA_35,
    gChatNgWordsA_36, gChatNgWordsA_37, gChatNgWordsA_38, gChatNgWordsA_39,
    gChatNgWordsA_40, gChatNgWordsA_41, gChatNgWordsA_42, NULL,
};
char *gChatNgWordsB[92] = {
    gChatNgWordsB_0, gChatNgWordsB_1, gChatNgWordsB_2, gChatNgWordsB_3,
    gChatNgWordsB_4, gChatNgWordsB_5, gChatNgWordsB_6, gChatNgWordsB_7,
    gChatNgWordsB_8, gChatNgWordsB_9, gChatNgWordsB_10, gChatNgWordsB_11,
    gChatNgWordsB_12, gChatNgWordsB_13, gChatNgWordsB_14, gChatNgWordsB_15,
    gChatNgWordsB_16, gChatNgWordsB_17, gChatNgWordsB_18, gChatNgWordsB_19,
    gChatNgWordsB_20, gChatNgWordsB_21, gChatNgWordsB_22, gChatNgWordsB_23,
    gChatNgWordsB_24, gChatNgWordsB_25, gChatNgWordsB_26, gChatNgWordsB_27,
    gChatNgWordsB_28, gChatNgWordsB_29, gChatNgWordsB_30, gChatNgWordsB_31,
    gChatNgWordsB_32, gChatNgWordsB_33, gChatNgWordsB_34, gChatNgWordsB_35,
    gChatNgWordsB_36, gChatNgWordsB_37, gChatNgWordsB_38, gChatNgWordsB_39,
    gChatNgWordsB_40, gChatNgWordsB_41, gChatNgWordsB_42, gChatNgWordsB_43,
    gChatNgWordsB_44, gChatNgWordsB_45, gChatNgWordsB_46, gChatNgWordsB_47,
    gChatNgWordsB_48, gChatNgWordsB_49, gChatNgWordsB_50, gChatNgWordsB_51,
    gChatNgWordsB_52, gChatNgWordsB_53, gChatNgWordsB_54, gChatNgWordsB_55,
    gChatNgWordsB_56, gChatNgWordsB_57, gChatNgWordsB_58, gChatNgWordsB_59,
    gChatNgWordsB_60, gChatNgWordsB_61, gChatNgWordsB_62, gChatNgWordsB_63,
    gChatNgWordsB_64, gChatNgWordsB_65, gChatNgWordsB_66, gChatNgWordsB_67,
    gChatNgWordsB_68, gChatNgWordsB_69, gChatNgWordsB_70, gChatNgWordsB_71,
    gChatNgWordsB_72, gChatNgWordsB_73, gChatNgWordsB_74, gChatNgWordsB_75,
    gChatNgWordsB_76, gChatNgWordsB_77, gChatNgWordsB_78, gChatNgWordsB_79,
    gChatNgWordsB_80, gChatNgWordsB_81, gChatNgWordsB_82, gChatNgWordsB_83,
    gChatNgWordsB_84, gChatNgWordsB_85, gChatNgWordsB_86, gChatNgWordsB_87,
    gChatNgWordsB_88, NULL, NULL, NULL,
};
char gDefaultPhrases[24][45] = {
#include "sub_main/00446560.inc"
};
char *gMsgEventTexts[8] = {
    D_004969E8, gMsgEventTexts_0, gMsgEventTexts_1, gMsgEventTexts_2,
    D_00492620, D_00496328, NULL, NULL,
};
char *gNoticeBannerTexts[32] = {
    gNoticeBannerTexts_0, gNoticeBannerTexts_1, gNoticeBannerTexts_2, gNoticeBannerTexts_3,
    gNoticeBannerTexts_4, gNoticeBannerTexts_5, gNoticeBannerTexts_6, gNoticeBannerTexts_7,
    gNoticeBannerTexts_8, gNoticeBannerTexts_9, gNoticeBannerTexts_10, gNoticeBannerTexts_11,
    gNoticeBannerTexts_12, gNoticeBannerTexts_13, gNoticeBannerTexts_14, gNoticeBannerTexts_15,
    gNoticeBannerTexts_16, gNoticeBannerTexts_17, gNoticeBannerTexts_18, gNoticeBannerTexts_19,
    gNoticeBannerTexts_20, gNoticeBannerTexts_21, gNoticeBannerTexts_22, gNoticeBannerTexts_23,
    gNoticeBannerTexts_24, gNoticeAwardText_0, gNoticeRateFlagText_0, D_00496DD0,
    D_00496DF0, D_00496E10, D_00496E30, D_00496E48,
};
char *gNoticeBannerItemTexts[24] = {
    gNoticeBannerItemTexts_0, gNoticeBannerItemTexts_1, gNoticeBannerItemTexts_2, D_00496E90,
    gNoticeBannerItemTexts_4, gNoticeBannerItemTexts_5, gNoticeBannerItemTexts_6, gNoticeBannerItemTexts_7,
    gNoticeBannerItemTexts_8, D_00496EF8, D_00496F10, D_00496F28,
    D_00496F38, D_00496F48, gNoticeBannerItemTexts_14, D_00496F80,
    D_00496FA8, gNoticeBannerItemTexts_17, gNoticeBannerItemTexts_18, gNoticeBannerItemTexts_19,
    gNoticeBannerItemTexts_20, NULL, NULL, NULL,
};
char *gNoticeBannerTextsItem[12] = {
    gNoticeBannerTextsItem_0, D_00496EF8, D_00496F10, D_00496F28,
    D_00496F48, D_00496F80, D_00496FA8, D_00496E90,
    D_00496F38, NULL, NULL, NULL,
};
char *gNoticeBannerPlayerTexts[4] = {
    gNoticeBannerPlayerTexts_0, gNoticeBannerPlayerTexts_1, gNoticeBannerPlayerTexts_2, gNoticeBannerPlayerTexts_3,
};
char *gNoticeMadeTexts[10] = {
    gNoticeMadeTexts_0, gNoticeMadeTexts_1, NULL, NULL,
    gNoticeMadeTexts_4, gNothingMoreText_0, D_00497120, D_00497138,
    D_00497120, D_00497150,
};
char *gSharpnessDownText[2] = {
    gSharpnessDownText_0, gSharpnessDownText_1,
};
char *gSharpnessFullText[2] = {
    gSharpnessFullText_0, NULL,
};
char *gLoadingCaptions[2] = {
    gLoadingCaptions_0, gLoadingCaptions_1,
};
char *gLoadingCaptionsAlt[4] = {
    gLoadingCaptionsAlt_0, gLoadingCaptionsAlt_1, NULL, NULL,
};
EmTraderMsg gEmTraderMsgs[106] = {
#include "sub_main/00446B30.inc"
};
char *gStageNames[80] = {
    gStageNames_0, D_00499CD0, D_00499CD8, D_00499CE0,
    D_00499CE8, D_00499CF0, D_00499CF8, gStageNames_7,
    gStageNames_8, D_00499D18, D_00499D28, D_00499D30,
    D_00499D38, D_00499D40, D_00499D40, D_00499D40,
    D_00499CD8, D_00499CE0, D_00499CE8, D_00499CF0,
    D_00499CF8, D_00499D30, D_00499D38, D_00499CD0,
    D_00499D18, D_00499D28, NULL, NULL,
    D_00499D48, D_00499D48, D_00499D50, gPlselHelpOnline_0,
    D_00499D48, D_00499D48, D_00499D48, D_00499DB0,
    D_00499DB8, D_00499DC0, D_00499DD0, D_00499DD8,
    D_00499DE0, D_00499DE8, D_00499DF0, D_00499DF8,
    D_00499E00, D_00499E08, D_00499E10, D_00499E20,
    D_00499E40, D_00499E60, gPlselStartPrompt_0, D_00499EA0,
    D_00499EC0, gPlselPointRateFormat_0, D_00499F48, D_00499F60,
    D_00491900, D_00499F78, gPlselWaitLine1_0, gPlselWaitLine2_0,
    D_00491500, D_004916C0, D_004916D0, D_00491528,
    D_00499FC0, NULL, NULL, NULL,
    D_00491500, D_004916C0, D_004916D0, D_00491528,
    D_00499FD0, NULL, NULL, NULL,
    D_00499FE0, D_0049A020, D_0049A060, NULL,
};
char *gQuestKindNames[96] = {
    D_0049A098, gQuestKindNames_1, gQuestKindNames_2, gQuestKindNames_3,
    gQuestKindNames_4, gQuestKindNames_5, gQuestKindNames_6, gQuestKindNames_7,
    gQuestKindNames_8, gQuestKindNames_9, gQuestKindNames_10, gQuestKindNames_11,
    gQuestKindNames_12, gQuestKindNames_13, gQuestKindNames_14, gQuestKindNames_15,
    gQuestKindNames_16, gQuestKindNames_17, gQuestKindNames_18, gQuestKindNames_19,
    gQuestKindNames_20, gQuestKindNames_21, gQuestKindNames_22, gQuestKindNames_23,
    gQuestKindNames_24, gQuestKindNames_25, gQuestKindNames_26, gQuestKindNames_27,
    gQuestKindNames_28, gQuestKindNames_29, gQuestKindNames_30, gQuestKindNames_31,
    gQuestKindNames_32, gQuestKindNames_33, gQuestKindNames_34, gQuestKindNames_35,
    gQuestKindNames_36, gQuestKindNames_37, gQuestKindNames_38, gQuestKindNames_39,
    gQuestKindNames_40, gQuestKindNames_41, gQuestKindNames_42, gQuestKindNames_43,
    gQuestKindNames_44, gQuestKindNames_45, gQuestKindNames_46, gQuestKindNames_47,
    gQuestKindNames_48, gQuestKindNames_49, gQuestKindNames_50, gQuestKindNames_51,
    gQuestKindNames_52, gQuestKindNames_53, gQuestKindNames_54, gQuestKindNames_55,
    gQuestKindNames_56, gQuestKindNames_57, gQuestKindNames_58, gQuestKindNames_59,
    gQuestKindNames_60, gQuestKindNames_61, gQuestKindNames_62, gQuestKindNames_63,
    gQuestKindNames_64, gQuestKindNames_65, gQuestKindNames_66, gQuestKindNames_67,
    gQuestKindNames_68, gQuestKindNames_69, gQuestKindNames_70, gQuestKindNames_71,
    gQuestKindNames_72, gQuestKindNames_73, gQuestKindNames_74, gQuestKindNames_75,
    gQuestKindNames_76, gQuestKindNames_77, gQuestKindNames_78, gQuestKindNames_79,
    gQuestKindNames_80, gQuestKindNames_81, gQuestKindNames_82, gQuestKindNames_83,
    gQuestKindNames_84, gQuestKindNames_85, gQuestKindNames_86, gQuestKindNames_87,
    gQuestKindNames_88, gQuestKindNames_89, gQuestKindNames_90, gQuestKindNames_91,
    gQuestKindNames_92, gQuestKindNames_93, gQuestKindNames_94, NULL,
};
char *gSkillEffectNames[204] = {
    gSkillEffectNames_0, gSkillEffectNames_1, gSkillEffectNames_2, gSkillEffectNames_3,
    gSkillEffectNames_4, gSkillEffectNames_5, gSkillEffectNames_6, gSkillEffectNames_7,
    gSkillEffectNames_8, gSkillEffectNames_9, gSkillEffectNames_10, gSkillEffectNames_11,
    gSkillEffectNames_12, gSkillEffectNames_13, gSkillEffectNames_14, gSkillEffectNames_15,
    gSkillEffectNames_16, gSkillEffectNames_17, gSkillEffectNames_18, gSkillEffectNames_19,
    gSkillEffectNames_20, gSkillEffectNames_21, gSkillEffectNames_22, gSkillEffectNames_23,
    gSkillEffectNames_24, gSkillEffectNames_25, gSkillEffectNames_26, gSkillEffectNames_27,
    gSkillEffectNames_28, gSkillEffectNames_29, gSkillEffectNames_30, gSkillEffectNames_31,
    gSkillEffectNames_32, gSkillEffectNames_33, gSkillEffectNames_34, gSkillEffectNames_35,
    gSkillEffectNames_36, gSkillEffectNames_37, gSkillEffectNames_38, gSkillEffectNames_39,
    gSkillEffectNames_40, gSkillEffectNames_41, gSkillEffectNames_42, gSkillEffectNames_43,
    gSkillEffectNames_44, gSkillEffectNames_45, gSkillEffectNames_46, gSkillEffectNames_47,
    gSkillEffectNames_48, gSkillEffectNames_49, gSkillEffectNames_50, D_0049B0C8,
    gSkillEffectNames_52, gSkillEffectNames_53, gSkillEffectNames_54, gSkillEffectNames_55,
    gSkillEffectNames_56, gSkillEffectNames_57, gSkillEffectNames_58, gSkillEffectNames_59,
    gSkillEffectNames_60, gSkillEffectNames_61, gSkillEffectNames_62, gSkillEffectNames_63,
    gSkillEffectNames_64, gSkillEffectNames_65, gSkillEffectNames_66, gSkillEffectNames_67,
    gSkillEffectNames_68, gSkillEffectNames_69, gSkillEffectNames_70, gSkillEffectNames_71,
    gSkillEffectNames_72, gSkillEffectNames_73, gSkillEffectNames_74, gSkillEffectNames_75,
    gSkillEffectNames_76, gSkillEffectNames_77, gSkillEffectNames_78, gSkillEffectNames_79,
    gSkillEffectNames_80, gSkillEffectNames_81, gSkillEffectNames_82, gSkillEffectNames_83,
    gSkillEffectNames_84, gSkillEffectNames_85, gSkillEffectNames_86, gSkillEffectNames_87,
    gSkillEffectNames_88, gSkillEffectNames_89, gSkillEffectNames_90, gSkillEffectNames_91,
    D_0049B3F0, gSkillEffectNames_93, gSkillEffectNames_94, gSkillEffectNames_95,
    gSkillEffectNames_96, gSkillEffectNames_97, gSkillEffectNames_98, gSkillEffectNames_99,
    gSkillEffectNames_100, gSkillEffectNames_101, gSkillEffectNames_102, gSkillEffectNames_103,
    gSkillEffectNames_104, gSkillEffectNames_105, gSkillEffectNames_106, gSkillEffectNames_107,
    gSkillEffectNames_108, gSkillEffectNames_109, gSkillEffectNames_110, gSkillEffectNames_111,
    gSkillEffectNames_112, gSkillEffectNames_113, gSkillEffectNames_114, gSkillEffectNames_115,
    gSkillEffectNames_116, gSkillEffectNames_117, gSkillEffectNames_118, gSkillEffectNames_119,
    gSkillEffectNames_120, gSkillEffectNames_121, gSkillEffectNames_122, gSkillEffectNames_123,
    gSkillEffectNames_124, gSkillEffectNames_125, gSkillEffectNames_126, gSkillEffectNames_127,
    gSkillEffectNames_128, gSkillEffectNames_129, gSkillEffectNames_130, gSkillEffectNames_131,
    gSkillEffectNames_132, gSkillEffectNames_133, gSkillEffectNames_134, gSkillEffectNames_135,
    gSkillEffectNames_136, gSkillEffectNames_137, gSkillEffectNames_138, gSkillEffectNames_139,
    gSkillEffectNames_140, gSkillEffectNames_141, gSkillEffectNames_142, gSkillEffectNames_143,
    gSkillEffectNames_144, gSkillEffectNames_145, gSkillEffectNames_146, gSkillEffectNames_147,
    gSkillEffectNames_148, gSkillEffectNames_149, gSkillEffectNames_150, gSkillEffectNames_151,
    gSkillEffectNames_152, gSkillEffectNames_153, gSkillEffectNames_154, gSkillEffectNames_155,
    gSkillEffectNames_156, gSkillEffectNames_157, gSkillEffectNames_158, gSkillEffectNames_159,
    gSkillEffectNames_160, gSkillEffectNames_161, gSkillEffectNames_162, gSkillEffectNames_163,
    gSkillEffectNames_164, gSkillEffectNames_165, gSkillEffectNames_166, gSkillEffectNames_167,
    gSkillEffectNames_168, gSkillEffectNames_169, gSkillEffectNames_170, D_0049B8B8,
    D_0049B8C0, gSkillEffectNames_173, gSkillEffectNames_174, gSkillEffectNames_175,
    gSkillEffectNames_176, gSkillEffectNames_177, gSkillEffectNames_178, gSkillEffectNames_179,
    gSkillEffectNames_180, gSkillEffectNames_181, gSkillEffectNames_182, gSkillEffectNames_183,
    gSkillEffectNames_184, gSkillEffectNames_185, gSkillEffectNames_186, gSkillEffectNames_187,
    gSkillEffectNames_188, gSkillEffectNames_189, gSkillEffectNames_190, gSkillEffectNames_191,
    gSkillEffectNames_192, gSkillEffectNames_193, gSkillEffectNames_194, D_0049BA78,
    gSkillEffectNames_196, gSkillEffectNames_197, gSkillEffectNames_198, gSkillEffectNames_199,
    gSkillEffectNames_200, D_0049BAD0, NULL, NULL,
};
char *gSkillNames[88] = {
    D_0049A098, gSkillNames_1, gSkillNames_2, gSkillNames_3,
    gSkillNames_4, gSkillNames_5, gSkillNames_6, gSkillNames_7,
    gSkillNames_8, gSkillNames_9, gSkillNames_10, gSkillNames_11,
    gSkillNames_12, gSkillNames_13, gSkillNames_14, gSkillNames_15,
    D_0049BB88, gSkillNames_17, gSkillNames_18, gSkillNames_19,
    gSkillNames_20, gSkillNames_21, gSkillNames_22, gSkillNames_23,
    gSkillNames_24, D_004961C0, gSkillNames_26, gSkillNames_27,
    gSkillNames_28, gSkillNames_29, gSkillNames_30, gSkillNames_31,
    gSkillNames_32, gSkillNames_33, gSkillNames_34, gSkillNames_35,
    gSkillNames_36, gSkillNames_37, gSkillNames_38, gSkillNames_39,
    gSkillNames_40, gSkillNames_41, gSkillNames_42, D_0049B8C0,
    gSkillNames_44, gSkillNames_45, D_004961B0, gSkillNames_47,
    gSkillNames_48, D_0049B3F0, gSkillNames_50, gSkillNames_51,
    gSkillNames_52, gSkillNames_53, gSkillNames_54, gSkillNames_55,
    gSkillNames_56, D_004961B8, gSkillNames_58, gSkillNames_59,
    gSkillNames_60, D_0049B8B8, D_0049B0C8, gSkillNames_63,
    gSkillNames_64, gSkillNames_65, gSkillNames_66, gSkillNames_67,
    D_00495EF8, gSkillNames_69, gSkillNames_70, gSkillNames_71,
    gSkillNames_72, gSkillNames_73, gSkillNames_74, gSkillNames_75,
    gSkillNames_76, D_0049BA78, gSkillNames_78, gSkillNames_79,
    gSkillNames_80, D_0049BAD0, gSkillNames_82, gSkillNames_83,
    D_0049BE20, NULL, NULL, NULL,
};
char *gHelpTexts_0[16] = {
    D_0049BE30, D_0049BE70, D_0049BE90, D_0049BEC0,
    gHelpTexts_0_0, gHelpTexts_0_1, gHelpTexts_0_2, gHelpTexts_0_3,
    gHelpTexts_0_4, gHelpTexts_0_5, gHelpTexts_0_6, D_0049BFF0,
    gHelpTexts_0_8, gHelpTexts_0_9, gHelpTexts_0_10, NULL,
};
char *D_00447610[2] = {D_0049C0B0, D_0049C0D0};
char *gHelpTexts_3[2] = {
    gHelpTexts_3_0, gHelpTexts_3_1,
};
char *D_00447620[4] = {D_0049C140, D_0049C170, D_0049C190, D_0049C1C0};
char *D_00447630[8] = {
    D_0049C1E0, D_0049C220, D_0049C260, D_0049C1E0,
    D_0049C290, D_0049C2D0, NULL, NULL,
};
char *gHelpTexts_9[16] = {
    gHelpTexts_9_0, D_0049BFF0, gHelpTexts_9_2, gHelpTexts_9_3,
    gHelpTexts_9_4, gHelpTexts_9_5, gHelpTexts_9_6, gHelpTexts_9_7,
    gHelpTexts_9_8, gHelpTexts_9_9, gHelpTexts_9_10, gHelpTexts_9_11,
    gHelpTexts_9_12, gHelpTexts_9_13, gHelpTexts_9_14, gHelpTexts_9_15,
};
char *gHelpTexts_10[8] = {
    D_0049C670, D_0049C0D0, D_0049C0B0, D_0049C690,
    gHelpTexts_10_0, gHelpTexts_10_1, NULL, NULL,
};
char *gHelpTexts_11[8] = {
    gHelpTexts_11_0, gHelpTexts_11_1, gHelpTexts_11_2, gHelpTexts_11_3,
    gHelpTexts_11_4, gHelpTexts_11_5, gHelpTexts_11_6, NULL,
};
char *gHelpTexts_12[12] = {
    D_0049C860, D_0049C890, D_0049C8C0, gHelpTexts_12_0,
    gHelpTexts_12_1, gHelpTexts_12_2, gHelpTexts_12_3, gHelpTexts_12_4,
    gHelpTexts_12_5, NULL, D_0049CA70, D_0049CA88,
};
char *gSymbolColorNames[12] = {
    gSymbolColorNames_0, gSymbolColorNames_1, gSymbolColorNames_2, gSymbolColorNames_3,
    gSymbolColorNames_4, gSymbolColorNames_5, gSymbolColorNames_6, gSymbolColorNames_7,
    gSymbolColorNames_8, gSymbolColorNames_9, gSymbolColorNames_10, gSymbolColorNames_11,
};
/* The seasons, then day and night (gDaytimeNames, an alias at + 0xC: as an object of its own
 * MWCC would 8-align it). */
char *gSeasonNames[5] = {
    gSeasonNames_0, gSeasonNames_1, gSeasonNames_2, gDaytimeNames_0, gDaytimeNames_1,
};
HunterTitle gHunterTitles[148] = {
#include "sub_main/00447750.inc"
};
char *gPriceLegendTexts[4] = {
    gPriceLegendTexts_0, gPriceLegendTexts_1, gPriceLegendTexts_2, NULL,
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
    gAwardGalleryTitles_0, gAwardGalleryTitles_1, gAwardGalleryTitles_2, gAwardGalleryTitles_3,
    gAwardGalleryTitles_4, gAwardGalleryTitles_5, gAwardGalleryTitles_6, gAwardGalleryTitles_7,
    gAwardGalleryTitles_8, gAwardGalleryTitles_9, gAwardGalleryTitles_10, gAwardGalleryTitles_11,
    gAwardGalleryTitles_12, gAwardGalleryTitles_13, gAwardGalleryTitles_14, gAwardGalleryTitles_15,
    gAwardGalleryTitles_16, gAwardGalleryTitles_17, gAwardGalleryTitles_18, gAwardGalleryTitles_19,
    gAwardGalleryTitles_20, gAwardGalleryTitles_21, gAwardGalleryTitles_22, gAwardGalleryTitles_23,
    gAwardGalleryTitles_24, gAwardGalleryTitles_25, gAwardGalleryTitles_26, gAwardGalleryTitles_27,
    gAwardGalleryTitles_28, gAwardGalleryTitles_29, gAwardGalleryTitles_30, gAwardGalleryTitles_31,
    gAwardGalleryTitles_32,
};
char *gAwardDojoTitles[8] = {
    gAwardDojoTitles_0, D_0049DA20, D_0049DA40, gAwardDojoTitles_3,
    gAwardDojoTitles_4, D_0049DA20, D_0049DA40, gAwardDojoTitles_7,
};
char *gArenaEmPenMenuTexts[2] = {
    D_0049DAC0, D_0049DAE0,
};
char *gArenaEmReleasedText = gArenaEmReleasedText_0;
char *gArenaEmLineFmts[4] = {
    D_0049DB20, D_0049DB30, D_0049DB40, D_0049DB50,
};
/* One object: gRecordDashesText (+ 0x4) and gArenaEmPenHelp (+ 0xC, PlselWork.help1 and help2,
 * [0] none) are not 8-aligned, so neither can start an object of its own. */
char *gArenaEmBlankLine[10] = {
    D_0049DB60, D_004957A0, NULL, NULL,
    gArenaEmPenHelp_1, gArenaEmPenHelp_2, gArenaEmPenHelp_3, gArenaEmPenHelp_4,
    gArenaEmPenHelp_5, gArenaEmPenHelp_6,
};
char *gArenaEmTameNames[5] = {
    gArenaEmTameNames_0, gArenaEmTameNames_1, gArenaEmTameNames_2, gArenaEmTameNames_3,
    gArenaEmTameNames_4,
};
char *gArenaEmCondNames[5] = {
    D_0049DCD0, D_0049DCD8, D_00495DA8, D_0049DCE0,
    D_0049DCE8,
};
char *gArenaEmRankNames[10] = {
    gArenaEmRankNames_0, gArenaEmRankNames_1, gArenaEmRankNames_2, gArenaEmRankNames_3,
    D_00495DA8, gArenaEmRankNames_5, gArenaEmRankNames_6, gArenaEmRankNames_7,
    gArenaEmRankNames_8, gArenaEmRankNames_9,
};
char *gHornSongTexts[53] = {
    D_0049DD60, D_0049DD70, D_0049DD90, D_0049DDB0,
    gHornSongTexts_1, gHornSongTexts_0, gHornSongTexts_1, gHornSongTexts_2,
    D_0049DE30, gHornSongTexts_4, D_0049DE30, D_0049DE68,
    D_0049DE68, gHornSongTexts_8, D_0049DE68, D_0049DE88,
    D_0049DE88, gHornSongTexts_12, D_0049DEA8, D_0049DEA8,
    D_0049DEB8, D_0049DEB8, D_0049DEB8, D_0049DED0,
    D_0049DED0, D_0049DEF0, D_0049DEF0, D_0049DF08,
    D_0049DF08, D_0049DF18, D_0049DF18, gHornSongTexts_26,
    D_0049DF50, gHornSongTexts_28, D_0049DF50, gHornSongTexts_30,
    D_0049DFB0, gHornSongTexts_32, D_0049DFB0, gHornSongTexts_34,
    D_0049E010, gHornSongTexts_36, D_0049E010, gHornSongTexts_38,
    D_0049E070, gHornSongTexts_40, D_0049E070, gHornSongTexts_42,
    D_0049E0D0, gHornSongTexts_44, D_0049E0D0, gHornSongTexts_46,
    gHornSongTexts_47,
};
char *gHunterTitleDescs[149] = {
    D_0049E140, D_0049E160, D_0049E190, D_0049E1C0,
    gHunterTitleDescs_0, gHunterTitleDescs_1, gHunterTitleDescs_2, gHunterTitleDescs_3,
    gHunterTitleDescs_4, gHunterTitleDescs_5, gHunterTitleDescs_6, gHunterTitleDescs_7,
    gHunterTitleDescs_8, gHunterTitleDescs_9, gHunterTitleDescs_10, gHunterTitleDescs_11,
    gHunterTitleDescs_12, gHunterTitleDescs_13, gHunterTitleDescs_14, gHunterTitleDescs_15,
    gHunterTitleDescs_16, gHunterTitleDescs_17, gHunterTitleDescs_18, gHunterTitleDescs_19,
    gHunterTitleDescs_20, gHunterTitleDescs_21, gHunterTitleDescs_22, gHunterTitleDescs_23,
    gHunterTitleDescs_24, gHunterTitleDescs_25, gHunterTitleDescs_26, gHunterTitleDescs_27,
    gHunterTitleDescs_28, gHunterTitleDescs_29, gHunterTitleDescs_30, gHunterTitleDescs_31,
    gHunterTitleDescs_32, gHunterTitleDescs_33, gHunterTitleDescs_34, gHunterTitleDescs_35,
    gHunterTitleDescs_36, gHunterTitleDescs_37, gHunterTitleDescs_38, gHunterTitleDescs_39,
    gHunterTitleDescs_40, gHunterTitleDescs_41, gHunterTitleDescs_42, gHunterTitleDescs_43,
    gHunterTitleDescs_44, gHunterTitleDescs_45, gHunterTitleDescs_46, gHunterTitleDescs_47,
    gHunterTitleDescs_48, gHunterTitleDescs_49, gHunterTitleDescs_50, gHunterTitleDescs_51,
    gHunterTitleDescs_52, gHunterTitleDescs_53, gHunterTitleDescs_54, gHunterTitleDescs_55,
    gHunterTitleDescs_56, gHunterTitleDescs_57, gHunterTitleDescs_58, gHunterTitleDescs_59,
    gHunterTitleDescs_60, gHunterTitleDescs_61, gHunterTitleDescs_62, gHunterTitleDescs_63,
    gHunterTitleDescs_64, gHunterTitleDescs_65, gHunterTitleDescs_66, gHunterTitleDescs_67,
    gHunterTitleDescs_68, gHunterTitleDescs_69, gHunterTitleDescs_70, gHunterTitleDescs_71,
    gHunterTitleDescs_72, gHunterTitleDescs_73, gHunterTitleDescs_74, gHunterTitleDescs_75,
    gHunterTitleDescs_76, gHunterTitleDescs_77, gHunterTitleDescs_78, gHunterTitleDescs_79,
    gHunterTitleDescs_80, gHunterTitleDescs_81, gHunterTitleDescs_82, gHunterTitleDescs_83,
    gHunterTitleDescs_84, gHunterTitleDescs_85, gHunterTitleDescs_86, gHunterTitleDescs_87,
    gHunterTitleDescs_88, gHunterTitleDescs_89, gHunterTitleDescs_90, gHunterTitleDescs_91,
    gHunterTitleDescs_92, gHunterTitleDescs_93, gHunterTitleDescs_94, gHunterTitleDescs_95,
    gHunterTitleDescs_96, gHunterTitleDescs_97, gHunterTitleDescs_98, gHunterTitleDescs_99,
    gHunterTitleDescs_100, gHunterTitleDescs_101, gHunterTitleDescs_102, gHunterTitleDescs_103,
    gHunterTitleDescs_104, gHunterTitleDescs_105, gHunterTitleDescs_106, gHunterTitleDescs_107,
    gHunterTitleDescs_108, gHunterTitleDescs_109, gHunterTitleDescs_110, gHunterTitleDescs_111,
    gHunterTitleDescs_112, gHunterTitleDescs_113, gHunterTitleDescs_114, gHunterTitleDescs_115,
    gHunterTitleDescs_116, gHunterTitleDescs_117, gHunterTitleDescs_118, gHunterTitleDescs_119,
    gHunterTitleDescs_120, gHunterTitleDescs_121, gHunterTitleDescs_122, gHunterTitleDescs_123,
    gHunterTitleDescs_124, gHunterTitleDescs_125, gHunterTitleDescs_126, gHunterTitleDescs_127,
    gHunterTitleDescs_128, gHunterTitleDescs_129, gHunterTitleDescs_130, gHunterTitleDescs_131,
    gHunterTitleDescs_132, gHunterTitleDescs_133, gHunterTitleDescs_134, gHunterTitleDescs_135,
    gHunterTitleDescs_136, gHunterTitleDescs_137, gHunterTitleDescs_138, gHunterTitleDescs_139,
    gHunterTitleDescs_140, gHunterTitleDescs_141, gHunterTitleDescs_142, gHunterTitleDescs_143,
    gEmClassNames_0,
};
char D_00448198[15] = {
#include "sub_main/00448198.inc"
};
char D_004481B0[48] = {
#include "sub_main/004481B0.inc"
};
char D_004481E0[32] = {
#include "sub_main/004481E0.inc"
};
char gHelpTexts_1_0[16] = {
#include "sub_main/00448200.inc"
};
char gHelpTexts_1_1[32] = {
#include "sub_main/00448210.inc"
};
char gHelpTexts_1_2[32] = {
#include "sub_main/00448230.inc"
};
char gHelpTexts_1_3[48] = {
#include "sub_main/00448250.inc"
};
char gHelpTexts_1_4[32] = {
#include "sub_main/00448280.inc"
};
char gHelpTexts_1_5[32] = {
#include "sub_main/004482A0.inc"
};
char D_004482C0[16] = {
#include "sub_main/004482C0.inc"
};
char gHelpTexts_1_9[32] = {
#include "sub_main/004482D0.inc"
};
char gHelpTexts_1_10[48] = {
#include "sub_main/004482F0.inc"
};
char gHelpTexts_1_11[32] = {
#include "sub_main/00448320.inc"
};
char gHelpTexts_1_12[32] = {
#include "sub_main/00448340.inc"
};
char gHelpTexts_1_13[32] = {
#include "sub_main/00448360.inc"
};
char gHelpTexts_1_18[64] = {
#include "sub_main/00448380.inc"
};
char gHelpTexts_1_19[32] = {
#include "sub_main/004483C0.inc"
};
char gHelpTexts_1_20[48] = {
#include "sub_main/004483E0.inc"
};
f32 gDayStepTimes[4] = {
#include "sub_main/00448410.inc"
};
f32 gAmbSePosHub84[1][3] = {
#include "sub_main/00448420.inc"
};
s32 gAmbSeIdsHub84[1] = {
#include "sub_main/00448430.inc"
};
s16 gAmbSePeriodsHub84[1] = {
#include "sub_main/00448438.inc"
};
s16 gAmbSeKindsHub84[1] = {
#include "sub_main/00448440.inc"
};
s16 gAmbSeSlotsHub84[1] = {
#include "sub_main/00448448.inc"
};
f32 gAmbSePosElderHall[2][3] = {
#include "sub_main/00448450.inc"
};
s32 gAmbSeIdsElderHall[2] = {
#include "sub_main/00448468.inc"
};
s16 gAmbSePeriodsElderHall[2] = {
#include "sub_main/00448470.inc"
};
s16 gAmbSeKindsElderHall[2] = {
#include "sub_main/00448478.inc"
};
s16 gAmbSeSlotsElderHall[2] = {
#include "sub_main/00448480.inc"
};
f32 gAmbSePosHub89[4][3] = {
#include "sub_main/00448490.inc"
};
s32 gAmbSeIdsHub89[4] = {
#include "sub_main/004484C0.inc"
};
s16 gAmbSePeriodsHub89[4] = {
#include "sub_main/004484D0.inc"
};
s16 gAmbSeKindsHub89[4] = {
#include "sub_main/004484D8.inc"
};
s16 gAmbSeSlotsHub89[4] = {
#include "sub_main/004484E0.inc"
};
u16 gAmbSeFlagsHub89[4] = {
#include "sub_main/004484E8.inc"
};
f32 gHub85EffectPos[16][3] = {
#include "sub_main/004484F0.inc"
};
FogParam gFogDefault = {
#include "sub_main/004485B0.inc"
};
FogParam gFogHouse = {
#include "sub_main/004485C0.inc"
};
AreaInfo gAreaInfo[200] = {
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
f32 gGlowPosOnlineVillage[12][3] = {
#include "sub_main/00448AC0.inc"
};
s16 gGlowEntryOnlineVillage[16] = {
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
f32 gAreaPropPosOnlineVillage[8][3] = {
#include "sub_main/00449250.inc"
};
f32 gAreaPropScaleOnlineVillage[8] = {
#include "sub_main/004492B0.inc"
};
s16 gAreaPropEntryOnlineVillage[8] = {
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

/* The camera's blocking spheres, and the atmosphere layers' speeds and periods. */
f32 gAreaBlockSpheres_33[36] = {
#include "sub_main/00449530.inc"
};
f32 gAreaBlockSpheres_38[28] = {
#include "sub_main/004495C0.inc"
};
f32 gAreaBlockSpheres_40[12] = {
#include "sub_main/00449630.inc"
};
f32 gAreaBlockSpheres_45[16] = {
#include "sub_main/00449660.inc"
};
f32 gAreaBlockSpheres_49[20] = {
#include "sub_main/004496A0.inc"
};
f32 gAreaBlockSpheres_57[8] = {
#include "sub_main/004496F0.inc"
};
f32 gAreaBlockSpheres_66[28] = {
#include "sub_main/00449710.inc"
};
f32 gAreaBlockSpheres_91[24] = {
#include "sub_main/00449780.inc"
};
f32 gAreaBlockSpheres_110[20] = {
#include "sub_main/004497E0.inc"
};
f32 gAreaBlockSpheres_113[20] = {
#include "sub_main/00449830.inc"
};
f32 gAreaBlockSpheres_114[16] = {
#include "sub_main/00449880.inc"
};
f32 gAreaBlockSpheres_26[8] = {
#include "sub_main/004498C0.inc"
};
f32 gAreaBlockSpheres_141[8] = {
#include "sub_main/004498E0.inc"
};
f32 gAreaBlockSpheres_142[12] = {
#include "sub_main/00449900.inc"
};
f32 gAreaBlockSpheres_143[20] = {
#include "sub_main/00449930.inc"
};
f32 gAreaBlockSpheres_145[24] = {
#include "sub_main/00449980.inc"
};
f32 gAreaBlockSpheres_185[52] = {
#include "sub_main/004499E0.inc"
};
f32 gAreaBlockSpheres_190[28] = {
#include "sub_main/00449AB0.inc"
};
f32 gAreaBlockSpheres_191[36] = {
#include "sub_main/00449B20.inc"
};
f32 *gAreaBlockSpheres[200] = {
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, gAreaBlockSpheres_26, NULL,
    NULL, NULL, NULL, NULL,
    NULL, gAreaBlockSpheres_33, NULL, NULL,
    NULL, NULL, gAreaBlockSpheres_38, NULL,
    gAreaBlockSpheres_40, NULL, NULL, NULL,
    NULL, gAreaBlockSpheres_45, NULL, NULL,
    NULL, gAreaBlockSpheres_49, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, gAreaBlockSpheres_57, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, gAreaBlockSpheres_66, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, gAreaBlockSpheres_91,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, gAreaBlockSpheres_110, NULL,
    NULL, gAreaBlockSpheres_113, gAreaBlockSpheres_114, NULL,
    NULL, NULL, NULL, gAreaBlockSpheres_26,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, gAreaBlockSpheres_141, gAreaBlockSpheres_142, gAreaBlockSpheres_143,
    NULL, gAreaBlockSpheres_145, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, gAreaBlockSpheres_185, NULL, NULL,
    NULL, NULL, gAreaBlockSpheres_190, gAreaBlockSpheres_191,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
f32 gSnowLayerSpeedsState0[4] = {
#include "sub_main/00449ED0.inc"
};
f32 gSnowLayerSpeedsState1[4] = {
#include "sub_main/00449EE0.inc"
};
f32 gSnowLayerSpeedsState2[4] = {
#include "sub_main/00449EF0.inc"
};
s16 gAtmosScrollSpeeds[4] = {
#include "sub_main/00449F00.inc"
};
s16 gAtmosScrollSpeedsState0[4] = {
#include "sub_main/00449F08.inc"
};
s16 gAtmosScrollSpeedsState1[4] = {
#include "sub_main/00449F10.inc"
};
s16 gAtmosScrollSpeedsState2[4] = {
#include "sub_main/00449F18.inc"
};
s16 *gAtmosScrollSpeedsByState[4] = {
    gAtmosScrollSpeedsState0, gAtmosScrollSpeedsState1, gAtmosScrollSpeedsState2, NULL,
};
s16 gSwampLayerPeriodsState0[4] = {
#include "sub_main/00449F30.inc"
};
s16 gSwampLayerPeriodsState1[4] = {
#include "sub_main/00449F38.inc"
};
s16 gSwampLayerPeriodsState2[4] = {
#include "sub_main/00449F40.inc"
};
s16 *gSwampLayerPeriodsByState[3] = {
    gSwampLayerPeriodsState0, gSwampLayerPeriodsState1, gSwampLayerPeriodsState2,
};
s16 gAreaAtmosLayerCounts[16] = {
#include "sub_main/00449F60.inc"
};
