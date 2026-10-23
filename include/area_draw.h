#ifndef AREA_DRAW_H
#define AREA_DRAW_H

/*
 * The drawing of an area's models (sub_main): the area model with its
 * animated parts (Area_DrawModels, gAreaAnimTbls), its props (Area_DrawProp,
 * gAreaProps, and the village's gVillagePropChoices), the sky's gradient
 * (Sky_DrawGradient) and the buffers of the area atmosphere objects
 * (AreaAtmos_*). The animations' parameter blocks are area_anim.h's.
 *
 * Both lists carry an alpha test reference and flags per entry: the flags
 * pick when the entry shows (AREA_DRAW_*) and how its animation runs
 * (area_anim.h's AREA_ANIM_REVERSE, AREA_ANIM_ROT_X, AREA_ANIM_ROT_Z).
 */

#include "common.h"
#include "obj.h"
#include "area.h"
#include "area_anim.h"

/* AreaAnim.flags and PropRec.flags. */
#define AREA_DRAW_NOT_IN_EVENT  0x01    /* AreaAnim: skipped while an event scene runs */
#define AREA_DRAW_NO_ZWRITE     0x02    /* drawn without writing the depth buffer */
#define AREA_DRAW_DAY           0x08    /* fades out as night falls (alpha 255 less the night level) */
#define AREA_DRAW_NIGHT         0x10    /* fades in as night falls (alpha the night level) */

/* An animated part of the area model (gAreaAnimTbls, one list per area):
 * 8 bytes, sorted by model; the entries of one model follow each other. */
typedef struct AreaAnim {
    u8 model;                   /* 0x00: its entry in the area model's set */
    u8 alphaRef;                /* 0x01: the alpha test's reference (RS_ALPHA_REF) */
    u8 type;                    /* 0x02: AREA_MODEL_ANIM_* */
    u8 flags;                   /* 0x03: AREA_DRAW_*, and area_anim.h's AREA_ANIM_* */
    void *arg;                  /* 0x04: the animation's parameters (area_anim.h's blocks) */
} AreaAnim;

/* AreaAnim.type: how Area_DrawModels animates the part. */
#define AREA_MODEL_ANIM_NONE            0x00    /* drawn in place */
#define AREA_MODEL_ANIM_SPIN            0x01    /* AreaAnim_TranslateSpin */
#define AREA_MODEL_ANIM_TRANSLATE       0x02    /* AreaAnim_Translate */
#define AREA_MODEL_ANIM_HIDDEN          0x03    /* not drawn */
#define AREA_MODEL_ANIM_UV_SCROLL       0x04    /* AreaAnim_UvScroll */
#define AREA_MODEL_ANIM_UV_WAVE         0x05    /* AreaAnim_UvWave */
#define AREA_MODEL_ANIM_UV_SCROLL_SPIN  0x06    /* AreaAnim_UvScrollSpin */
#define AREA_MODEL_ANIM_UV_STEP         0x07    /* AreaAnim_UvStep */
#define AREA_MODEL_ANIM_UV_SCROLL_MOVE  0x08    /* AreaAnim_UvScrollTranslate */
#define AREA_MODEL_ANIM_SPIN_SCROLL     0x09    /* AreaAnim_SpinScroll */
#define AREA_MODEL_ANIM_MAT_ALPHAS      0x64    /* its first material opaque, its second clear */
#define AREA_MODEL_ANIM_UV_SCROLL_BLINK 0x65    /* scrolled, shown for 64 frames of each 256 */
#define AREA_MODEL_ANIM_NIGHT_SCROLL    0x66    /* scrolled, darkened by night (also AREA_MODEL_ANIM_NIGHT_SCROLL_2) */
#define AREA_MODEL_ANIM_NIGHT_SCROLL_2  0x67
#define AREA_MODEL_ANIM_SPIN_TURN       0x68    /* spun, then turned by the angle after its AnimSpin (to AREA_MODEL_ANIM_SPIN_TURN_3) */
#define AREA_MODEL_ANIM_SPIN_TURN_BIG   0x69    /* the same, scaled by 1.25 */
#define AREA_MODEL_ANIM_SPIN_TURN_2     0x6A
#define AREA_MODEL_ANIM_SPIN_TURN_3     0x6B
#define AREA_MODEL_ANIM_PULSE           0x6C    /* its colour pulses (green and blue) */

/* A prop of an area (gAreaProps, one list per area; the village's from
 * gVillagePropChoices): 12 bytes, the list ended by model PROP_END. */
typedef struct PropRec {
    u8 model;                   /* 0x00: its entry in the props' model set; PROP_END ends the list */
    u8 alphaRef;                /* 0x01: the alpha test's reference (RS_ALPHA_REF) */
    u8 type;                    /* 0x02: its animation: area_anim.h's AREA_ANIM_CMD_*, or PROP_ANIM_* */
    u8 flags;                   /* 0x03: AREA_DRAW_*, and area_anim.h's AREA_ANIM_* */
    void *data;                 /* 0x04: the animation's parameters, one block per copy */
    s16 count;                  /* 0x08: the copies drawn */
    u16 late;                   /* 0x0A: drawn after the others (two at most; more are drawn in turn) */
} PropRec;

#define PROP_END 0xFF

/* PropRec.type, besides area_anim.h's AREA_ANIM_CMD_* (1 to 12). The parameters
 * named are the f32 words of PropRec.data, per copy. */
#define PROP_ANIM_NONE          0x00    /* drawn in place */
#define PROP_ANIM_MOVING        0x0D    /* at pos[3] plus gAreaState.propOfs, rocking while propState runs */
#define PROP_ANIM_FLIPBOOK      0x64    /* its texture steps through 4 x 8 cells, forwards then back */
#define PROP_ANIM_PLACE_YZ      0x65    /* at pos[3], turned about y and z by two angles */
#define PROP_ANIM_NIGHT_GLOW    0x66    /* alpha 0.875 of the night level, plus a fast pulse */
#define PROP_ANIM_NIGHT_SPIN    0x67    /* darkened by night (Area_NightTint's colour), and spun (AnimSpin) */
#define PROP_ANIM_PULSE         0x68    /* alpha pulses between 160 and 255 */
#define PROP_ANIM_SLIDE         0x69    /* at pos[3], sliding 2600 along z and back, holding between */
#define PROP_ANIM_SWAY          0x6A    /* at pos[3], swaying about z, turned about y */
#define PROP_ANIM_WHEEL         0x6B    /* copies spaced around y, turning with the frame counter; those
                                         * between 100 and 250 degrees are not drawn */
#define PROP_ANIM_PLACE_XYZ     0x6C    /* at pos[3], turned about x, y and z */
#define PROP_ANIM_FLICKER       0x6D    /* at pos[3], stretched along z by a sum of sines, swaying about y */
#define PROP_ANIM_FLICKER_2     0x6E    /* the same, with other frequencies */
#define PROP_ANIM_DAY_SCROLL    0x6F    /* AreaAnim_UvScrollTranslate, half faded by night */
#define PROP_ANIM_NIGHT_GLOW_2  0x70    /* this to PROP_ANIM_NIGHT_GLOW_4: as PROP_ANIM_NIGHT_GLOW */
#define PROP_ANIM_NIGHT_GLOW_3  0x71
#define PROP_ANIM_NIGHT_GLOW_4  0x72
#define PROP_ANIM_NIGHT_GLOW_5  0x73    /* this and PROP_ANIM_NIGHT_GLOW_6: alpha 0.81 of the night level, plus a slower pulse */
#define PROP_ANIM_NIGHT_GLOW_6  0x74
#define PROP_ANIM_FACILITY      0x75    /* drawn when the village's facility level 0 is the s16 in data */
#define PROP_ANIM_VARIANT       0x76    /* drawn when the area's variant (gAreaState.look[1]) is the byte in data */
#define PROP_ANIM_KUSHALA_WIND  0x77    /* AreaAnim_UvScroll, drawn only while a Kushala Daora is in the field */

/* The parameters of PROP_ANIM_SWAY, PROP_ANIM_FLICKER and PROP_ANIM_FLICKER_2,
 * one per copy (PropRec.data). */
typedef struct PropPlaceY {
    f32 pos[3];                 /* 0x00 */
    f32 rotY;                   /* 0x0C: the turn about y (radians) */
} PropPlaceY;

/* The parameters of PROP_ANIM_PLACE_YZ, one per copy. */
typedef struct PropPlaceYZ {
    f32 pos[3];                 /* 0x00 */
    f32 rotY;                   /* 0x0C: the turn about y (radians) */
    f32 rotZ;                   /* 0x10: then about z */
} PropPlaceYZ;

/* The parameters of PROP_ANIM_PLACE_XYZ, one per copy. */
typedef struct PropPlaceXYZ {
    f32 pos[3];                 /* 0x00 */
    f32 rot[3];                 /* 0x0C: the turns about x, y and z (radians, Mtx_RotateXYZ) */
} PropPlaceXYZ;

/* The parameters of AREA_MODEL_ANIM_SPIN_TURN to AREA_MODEL_ANIM_SPIN_TURN_3
 * (AreaAnim.arg). */
typedef struct AnimSpinTurn {
    AnimSpin spin;              /* 0x00 */
    u16 turn;                   /* 0x10: then the turn about y (65536 a full turn) */
} AnimSpinTurn;

/* The village's props by its facilities (gVillagePropChoices's lists). */
typedef struct PropChoice {
    u16 mask;                   /* 0x00: bit n: for model set n; bit 8 + l: for facility level l */
    u8 pad02[2];
    PropRec *props;             /* 0x04 */
} PropChoice;

/* An area's prop choices (gVillagePropChoices, ended by area 65535). */
typedef struct AreaPropChoices {
    u16 area;                   /* 0x00: the area id; 65535 ends the table */
    s16 count;                  /* 0x02: the choices */
    PropChoice *choices;        /* 0x04 */
} AreaPropChoices;

/* The props held back to be drawn last (Area_DrawModels). */
typedef struct PropDefer {
    PropRec *prop[2];
    u8 set[2];                  /* their model sets (gAreaSubModels) */
} PropDefer;

/* A step of an area's day table (Sky_GetGradient): 8 steps. */
typedef struct DayStep {
    s32 type;                   /* 0x0: a row of gDayStepTimes (below DAY_STEP_TIMES), or DAY_STEP_BEFORE_NEXT */
    u32 top;                    /* 0x4: the sky's top colour */
    u32 bottom;                 /* 0x8: its bottom colour */
} DayStep;

#define DAY_STEP_COUNT      8
#define DAY_STEP_TIMES      4   /* the rows of gDayStepTimes: types 0-3 */
#define DAY_STEP_BEFORE_NEXT 5  /* a moment before the next step's time; other types: a moment after
                                 * the previous step's (DayTable_GetTime) */

/* A scrolled layer of an area atmosphere object (types 8 and 14). */
typedef struct AtmosLayer {
    f32 scroll[3];              /* 0x00: [1] the texture's v offset, kept in 0-1; [0] and [2] stay 0 */
    f32 speed;                  /* 0x0C: its v per frame, eased to AreaAtmos_LayerSpeed */
    f32 alpha;                  /* 0x10: eased to AreaAtmos_LayerAlpha */
} AtmosLayer;

/* A texture offset of an area atmosphere object's layer (AtmosScroll). */
typedef struct AtmosUv {
    f32 u;                      /* 0x0 */
    f32 v;                      /* 0x4 */
    f32 alpha;                  /* 0x8: eased to AreaAtmos_LayerAlpha (types 10 and 13) */
} AtmosUv;

/* The buffer of the area atmosphere objects of types 10, 11 and 13: their
 * layers' textures follow the camera's turns and climbs (AtmosScroll_Update). */
typedef struct AtmosScroll {
    u16 pitch;                  /* 0x00: the camera's last pitch (Vec_DirToAngles) */
    u16 yaw;                    /* 0x02: and its last yaw */
    f32 camPos[3];              /* 0x04: and its last position */
    AtmosUv layer[1];           /* 0x10: one per layer (gAreaAtmosLayerCounts) */
} AtmosScroll;

void Area_DrawProp(PropRec *prop, Model *set, u8 *alphaRef);
void Area_DrawModels(void);
void Sky_DrawGradient(void);
void Sky_DrawBand(SkyBand *band, f32 *dir);
void Sky_Draw(void);
void Area_DrawGlowProps(void);
void Notice_ResetQueue(void);
s32 Notice_IsQueued(u8 k);
void Notice_Update(void);
void NoticeBanner_Spawn(u8 kind, s16 textNo, s16 subject, u8 noticeId);
u8 Notice_Push(u8 kind, s16 text, s16 subject);
void NoticeBanner_SpawnString(char *msg, u8 noticeId);
u8 Notice_PushItem(char *item);
u8 Notice_PushText(char *text);
void NoticeBanner_Task(Obj *obj);
void NoticeBanner_Start(Obj *obj);
u32 NoticeBanner_Run(Obj *obj);
void NoticeBanner_End(Obj *obj);
void NoticeBanner_Free(Obj *obj);
void TextNotice_Draw(DrawSlot *slot);
void AreaProp_Spawn(s16 k);
void AreaProp_Task(Obj *obj);
void AreaProp_Start(Obj *obj);
void AreaProp_Run(Obj *obj);
void AreaProp_End(Obj *obj);
void AreaProp_Free(Obj *obj);
void AreaProp_Draw(DrawSlot *slot);
void SoundObj_Start(s32 bank, s32 seId, s32 range, f32 *pos, s16 frames, u8 mode);
void SoundObj_Task(Obj *obj);
void SoundObj_Init(Obj *obj);
void SoundObj_Run(Obj *obj);
void SoundObj_End(Obj *obj);
void SoundObj_Free(Obj *obj);
Obj *AreaAtmos_Alloc(s32 type);
void AreaAtmos_Spawn(s32 type);
void AreaAtmos_Task(Obj *obj);
void AreaAtmos_Setup(Obj *obj);
void AreaAtmos_Update(Obj *obj);
void AreaAtmos_End(Obj *obj);
void AreaAtmos_Free(Obj *obj);
void AreaAtmos_Draw(DrawSlot *slot);
void Vec_FromCamera(f32 *out, f32 *dir, f32 dist);
s32 Area_LineBlocked(f32 *pos);
s16 Area_BoundaryZone(Player *pl);
void AtmosScroll_Update(AtmosScroll *scroll, s16 count, s16 *speed, u16 mode);
f32 AreaAtmos_LayerAlpha(u8 layer, s16 state);
f32 AreaAtmos_LayerSpeed(u8 layer, s16 state);
/* Per area (gScene.area), 16 material set indices into gAreaSubModels[0]'s
 * matSets, -1: none (most rows are a table of gm_sub's): [0] the set
 * Player_GetAreaPitModel checks and PitfallTrap draws (three from it), [1] AreaAtmos_Draw's,
 * [2..9] the draws of AmbientLife's types 1 to 8 (AmbientLife_DrawFallingLeaf ...). */
extern s16 *gAreaFxModels[200];
/* Per area id (4 bytes): four setup ids for Area_StartObjects (255: none). atmosType
 * is also a kind: 8, 13 and 14 get Area_MonsterState's state. */
typedef struct AreaInfo {
    u8 atmosType;       /* AreaAtmos_Spawn's type; Area_MonsterState switches on it */
    u8 lifeSet;         /* AmbientLife_Create's set */
    u8 scenerySet;      /* AnimatedScenery_Create's set */
    u8 flyerPoint;      /* AreaFlyer_Create's point; 255 clears AreaState.flyer */
} AreaInfo;
extern AreaInfo gAreaInfo[200];
extern char *gReturn20sTownText[2];
extern char *gReturn1mTownText[2];
extern char *gQuestClearText[2];
extern char *gTimeLeftMinFmt[2];
extern char *gQuestNoticeTexts[8];
extern char *gSubBSupplyText[4];
extern char *gMustOrderAllText[2];
extern char *gReturn10sTownText[2];
extern char *gDrillScoreTexts[16];
extern char *gElderFledText[4];
extern char *gEmClassNames[12];
extern char *gMonsterNames[76];
extern char *gPouchMenuPanel2_6[2]; /* sub_main: the YES/NO strings */
extern char *gDeliveryPanel_7[2];
extern char *gYesNoTexts[2];
extern char *gStatusMenuColors_1[6];
extern char *gInfoMenuLabelsOffline[2];
extern char *gInfoMenuLabels[4];
extern char *gCockpitToggleTexts[4];
extern char *gQuestStatusTitle[2];
extern char *gQuestMenuRecordTitles[2];
extern char *gQuestMenuTitles[2];
extern char *gQuestMenuStatusTexts[2];
extern char *gQuestMenuTargetTexts[2];
extern char *gDojoNoRecordText[2];
extern char *gQuestInfoFormats[8];
extern char *gQuestTargetLabels[3];
extern char *gCombineSelectFmt[2];
extern char *gCombineInfoLabels[2];
extern char *gCombineRateFmt[2];
extern char *gCombineSelectingText[2];
extern char *gCombineFailLabels[2];
extern char *gCombine2Text[2];
extern char *gCombine2MaterialLabels[4];
extern char *gCombine3MaterialLabels[4];
extern char *gCombineDoText[2];
extern char *gPouchInfiniteFmt[2];
extern char *gAmmoTensFmt[2];
extern char *gItemFullText[2];
extern char *gFieldMenuTitle[2];
extern char *gPausedText[2];
extern char *gPhraseEditFmt[2];
extern char *gChatMenuPanel_7[2];
extern char *gSupplySelectText[2];
extern char *gSupplyCannotCarryText[2];
extern char *gSupplyCountFmt[4];
extern char *gTentMenuLabels[18];
extern char *gSubTargetHelpText[2];
extern char *gQuestMenuHelpTexts[4];
extern char *gDeliveryMenuTexts[4];
extern char *gDeliveryDoneText[2];
extern char *gDeliveryHelpText[2];
extern char *gCombineListFmt[2];
extern char *gItemListLineTexts[2];
extern char *gRecipeMaterialsFmt[2];
extern char *gRecipeSwitchHint[2];
extern char *gRecipeMaterialFmts[4];
extern char *gRecipeRateLabels[8];
extern char *gRecipeYieldLabels[12];
extern char *gMonsterListTitle[4];
extern char *gMonsterNoSizeText[2];
extern char *gArenaBaitNoBookText[2];
extern char *gMonsterNoteLabels[2];
extern char *gArenaBaitHint[2];
extern char *gArenaBaitLabels[2];
extern char *gMonsterNoteValueFmt[2];
extern char *gEmDescTexts[52];
extern char *gEquipStatusTitle[2];
extern char *gOptionMenuTitle[2];
extern char *gRarityFmt[8];
extern s32 gChatTitleText;
extern char *gChatRecvText[4];
extern char *gNoticeNoSelText[2];
extern char *gItemListTitles[2];
extern char *gItemListTitleFmt[2];
extern char *gInfinityText[4];
extern char *gStatusHeadLabels[4];
extern char *gSymbolColorLabel[2];
extern char *gArenaRecordLabels[2];
extern char *gArenaRecordFmt[4];
extern char *gStatusStatLabels[8];
extern char *gWeaponTypeNames[12];
extern char *gStatusBoxTitles[8];
extern char *gStatusPageFmt[2];
extern char *gMemberCardNumFmt;
extern char *gRankPointsFmt[2];
extern char *gSkillNameFmt[2];
extern char *gSkillPartPtsFmt[2];
extern char *gSkillRowFmt[6];
extern char *gSkillDetailTitle;
extern char *gSkillDetailTitleDim;
extern char *gPointsFmt[2];
extern char *gSkillListHeader[2];
extern char *gSkillPartMarks[2];
extern char *gEquipWeaponFmt[2];
extern char *gUnknownEquipText[2];
extern char *gHunterTypeLabels[4];
extern char *gEquipTypeLabels[4];
extern char *gBowDetailLabels[2];
extern char *gArmorStatLabels[8];
extern char *gGunReloadTexts[8];
extern char *gGunRecoilTexts[8];
extern char *gHunterClassTexts[4];
extern char *gEquipNameFmt[2];
extern char *gScopeFixedText[2];
extern char *gPowerBarrelText[2];
extern char *gShieldText[2];
extern char *gAmmoHeaderText[6];
extern char *gAmmoNameFmt[2];
extern char *gAmmoLineFormats[6];
extern char *gDecoDetailTitle[2];
extern char *gAffinityLabel[4];
extern char *gAmmoNames[20];
extern char *gElementNames[8];
extern char *gSkillPointsTitle[2];
extern char *gNoSkillText[2];
extern char *gDecoDetailLabels[4];
extern char *gRangeClassTexts[4];
extern char *gDecoSkillPtsFmt[2];
extern char *gDecoSkillFmt[2];
extern char *gShellTypeTexts[4];
extern char *gBowShotTexts[12];
extern char *gTabPageFmt[2];
extern char *gSortHintText[2];
extern s8 *gStartText;
extern char *gSelectText[2];
extern char *gPageFmt;
extern char *gDecoModeFmt[2];
extern char *gCommentEditTitle[2];
extern char *gCommentEditText[2];
extern char *gChatTargetTexts[4]; /* the "everyone" target text */
extern char *gChatNgWordsA[44];
extern char *gChatNgWordsB[92];
extern char *gMsgEventTexts[8];
extern char *gNoticeBannerTexts[32]; /* sub_main: the notices' texts, by NOTICE_MSG_* */
extern s32 gNothingMoreText;
extern s32 gRewardZeroText;
extern s32 gFaintedText;
extern s32 gRewardZeroText2;
extern s32 gNoMoreReviveText;
extern char *gSharpnessDownText[2];
extern char *gSharpnessFullText[2];
extern char *gLoadingCaptions[2];
extern char *gLoadingCaptionsAlt[4];
extern char *gStageNames[80];
extern char *gPlselHelpOffline;
extern char *gPlselHelpOnline;
extern char *gPlselEquipPrompt;
extern char *gPlselYesNoTexts[];
extern char *gPlselStartPrompt;
extern char *gPlselNoRechooseText;
extern char *gPlselPointRateFormat;
extern char *gPlselEquipTypeLabel;
extern char *gPlayerNumFormat;
extern char *gPlselReadyText;
extern char *gPlselWaitLine1;
extern char *gPlselWaitLine2;
extern char *D_00446F70[];
extern char *gPlselStageTitleOffline;
extern char *gPlselMenuOnlineItems[];
extern char *gPlselStageTitleOnline;
extern char *gQuestKindNames[96];
extern char *gSkillEffectNames[204];
extern char *gSkillNames[88]; /* sub_main */
extern char *gEquipSetNameFormat;
extern char *gHelpTexts_0[16];
extern char *gHelpTexts_3[2];
extern char *gHelpTexts_9[16];
extern char *gHelpTexts_10[8];
extern char *gHelpTexts_11[8];
extern char *gHelpTexts_12[12];
extern char *gQNetErrorText;
extern char *gSymbolColorNames[12];
extern char *gSeasonNames[5];
extern char *gDaytimeNames[];
extern char *gPriceLegendTexts[];
extern char **gAwardNoticeLines[10];
extern char *gAwardGalleryTitles[37];
extern char *gAwardDojoTitles[8];
extern char *gArenaEmPenMenuTexts[2];
extern char *gArenaEmReleasedText;
extern char *gArenaEmLineFmts[4];
extern char *gArenaEmBlankLine[10];
extern char *gRecordDashesText;
extern char *gArenaEmPenHelp[];
extern char *gArenaEmTameNames[5];
extern char *gArenaEmCondNames[5];
extern char *gArenaEmRankNames[10];
extern char *gHornSongTexts[53];
extern char *gHunterTitleDescs[149];
extern FogParam gFogDefault; /* the default fog */
extern FogParam gFogHouse; /* the fog of AREA_HOUSE */
extern char *D_00445888[2];
extern char D_00448198[15];
extern char D_004481E0[32];
extern char D_004481B0[48];
extern char gHelpTexts_1_20[48];
extern char gHelpTexts_1_19[32];
extern char gHelpTexts_1_18[64];
extern char gHelpTexts_1_13[32];
extern char gHelpTexts_1_12[32];
extern char gHelpTexts_1_11[32];
extern char gHelpTexts_1_10[48];
extern char gHelpTexts_1_9[32];
extern char D_004482C0[16];
extern char gHelpTexts_1_5[32];
extern char gHelpTexts_1_4[32];
extern char gHelpTexts_1_3[48];
extern char gHelpTexts_1_2[32];
extern char gHelpTexts_1_1[32];
extern char gHelpTexts_1_0[16];
extern char *D_00447630[8];
extern char *D_00447620[4];
extern char *D_00447610[2];

#endif /* AREA_DRAW_H */
