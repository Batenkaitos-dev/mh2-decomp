/*
 * Drawing the hunters and what they hold.
 *
 * - The hunters: Player_Draw and Player_DrawLocal (the local one, faded near
 *   the camera) draw the skeleton's own model (the legs), the face and the
 *   four other armour pieces placed by their bones and lit per material
 *   (Player_SetLight, Player_PartFlags, Color_Cycle8), the weapon
 *   (Player_DrawWeapon) and the held props (Player_DrawHeldProp,
 *   Player_DrawHeldItem); CharMake_DrawHunter draws the hunter being made.
 * - The weapon: Player_MotionPhase says where it is in the current motion
 *   (in a hand or on the back, WP_PHASE_*), the WpPose tables place it on its
 *   bone, and keyframe tracks (Key1, Key2, Key3) move its moving bones in the
 *   weapon's own motions.
 * - The equipment preview of the menus (EquipPreview_*, gEquipPreview): a
 *   window that shows one piece of equipment, posed by its type, with its
 *   own lights (LightSet_SetupSet).
 * - The monsters' models (Em_DrawParts, their draw slots' function), the
 *   players' world matrices (Player_UpdateWorldMatrix) and the bone scaling
 *   the hunters' models use (Player_ScaleBones).
 * The data tables these read (the poses, keyframes, light sets and the
 * parts each object kind skips) are defined at the end of the file.
 */
#include "common.h"
#include "scene.h"
#include "em.h"
#include "camera.h"
#include "player.h"
#include "objhead.h"
#include "stageobj.h"
#include "sprite.h"
#include "chredit.h"
#include "area.h"
#include "pad.h"
#include "render_state.h"
#include "item.h"
#include "light.h"
#include "equip_preview.h"

/* Where the weapon is drawn (Player_MotionPhase, Player_DrawWeapon). */
#define WP_PHASE_OTHER_HAND 0   /* in the other hand (PL_BONE_OTHER_HAND, gWpPoseDrawnAlt) */
#define WP_PHASE_HAND 1         /* in the hand (PL_BONE_HAND, gWpPoseDrawn) */
#define WP_PHASE_SHEATHED 2     /* on the back (gWpPoseSheathed) */

/* The hunter's hand bones, which are also the indices of their part records (Player.partRecs,
 * Player_ScaleBones). */


u8 Player_PartFlags(Player *pl, u8 slot);
u32 Color_Cycle8(void);
s32 Gs_SetState(u8 reg, u32 v);
void Vec_AnglesToRad(s32 *in, f32 *out);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateYXZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Copy(f32 *dst, f32 *src);
struct EmBone;
void Bone_UpdateWorld(struct EmBone *b, f32 *m);
void Player_ScaleBones(Player *pl, EmBone *bones, f32 *parent);

s32 Motion_ReachedFrame(void *o, s32 layer, f32 frame);

/* Where the player's weapon is drawn in its current motion (motionId[0]): WP_PHASE_HAND,
 * WP_PHASE_OTHER_HAND or WP_PHASE_SHEATHED, by weapon type and the frame the motion has
 * reached (the weapon's own motions, 0x3E8 and up, draw and sheathe it), then by the shared
 * motions (the gestures, the sharpening 0x19D ...), which override it. secondPart picks the
 * dual swords' second blade's frames; otherPlayer (set when Player_Draw draws another player's
 * hunter) keeps motion 0x349's weapon on the back. The arms
 * that set WP_PHASE_SHEATHED where it already is are the game's own: the compiler drops the
 * store but keeps the branch. */
s16 Player_MotionPhase(Player *pl, s32 secondPart, s32 otherPlayer)
{
    s16 phase = WP_PHASE_SHEATHED;

    switch (pl->weaponType) {
    case WEAPON_HAMMER:
        switch (pl->motionId[0]) {
        case 0x3EA:
            if (Motion_ReachedFrame(pl, 0, 6.0f)) phase = WP_PHASE_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(pl, 0, 48.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 0x3F1:
            if (Motion_ReachedFrame(pl, 0, 50.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 0x3F2:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    case WEAPON_HUNTING_HORN:
        switch (pl->motionId[0]) {
        case 0x3EA:
            if (Motion_ReachedFrame(pl, 0, 68.0f)) phase = WP_PHASE_HAND;
            else if (Motion_ReachedFrame(pl, 0, 14.0f)) phase = WP_PHASE_OTHER_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(pl, 0, 50.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 0x3ED:
            if (Motion_ReachedFrame(pl, 0, 50.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 0x3EE:
            if (Motion_ReachedFrame(pl, 0, 16.0f)) phase = WP_PHASE_HAND;
            else if (Motion_ReachedFrame(pl, 0, 6.0f)) phase = WP_PHASE_OTHER_HAND; else phase = WP_PHASE_SHEATHED;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 0x57C:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_HAND;
            else if (Motion_ReachedFrame(pl, 0, 54.0f)) phase = WP_PHASE_HAND;
            else phase = WP_PHASE_OTHER_HAND;
            break;
        case 0x588:
            phase = WP_PHASE_OTHER_HAND;
            break;
        case 0x597:
            if (Motion_ReachedFrame(pl, 0, 10.0f)) phase = WP_PHASE_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    case WEAPON_LANCE:
    case WEAPON_GUNLANCE:
        switch (pl->motionId[0]) {
        case 0x3EA:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            else if (pl->weaponType == WEAPON_LANCE) phase = WP_PHASE_HAND;
            else if (Motion_ReachedFrame(pl, 0, 14.0f)) phase = WP_PHASE_HAND;
            break;
        case 0x3FA:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(pl, 0, 60.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 0x3F1:
            if (Motion_ReachedFrame(pl, 0, 66.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 0x3F2:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    case WEAPON_SWORD_SHIELD:
        switch (pl->motionId[0]) {
        case 0x3EA:
        case 0x3FA:
        case 0x3F2:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(pl, 0, 32.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 0x3F1:
            if (Motion_ReachedFrame(pl, 0, 30.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 0x3F8:
            if (Motion_ReachedFrame(pl, 0, 14.0f)) phase = WP_PHASE_OTHER_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 0x3F9:
            if (Motion_ReachedFrame(pl, 0, 44.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_OTHER_HAND;
            break;
        case 0x582:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_OTHER_HAND;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    case WEAPON_DUAL_SWORDS:
        switch (pl->motionId[0]) {
        case 0x3EA:
            if (Motion_ReachedFrame(pl, 0, secondPart == 0 ? 8.0f : 10.0f)) phase = WP_PHASE_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(pl, 0, secondPart == 0 ? 20.0f : 28.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 0x3F1:
            if (Motion_ReachedFrame(pl, 0, secondPart == 0 ? 18.0f : 30.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            break;
        case 0x3F9:
            if (Motion_ReachedFrame(pl, 0, 44.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_OTHER_HAND;
            break;
        case 0x585:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            else if (Motion_ReachedFrame(pl, 0, 4.0f)) phase = WP_PHASE_HAND; else phase = WP_PHASE_SHEATHED;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    default:
        switch (pl->motionId[0]) {
        case 0x3EA:
            if (Motion_ReachedFrame(pl, 0, 16.0f)) phase = WP_PHASE_OTHER_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(pl, 0, 38.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_OTHER_HAND;
            break;
        case 0x3F8:
            if (Motion_ReachedFrame(pl, 0, 14.0f)) phase = WP_PHASE_OTHER_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 0x3F9:
            if (Motion_ReachedFrame(pl, 0, 44.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_OTHER_HAND;
            break;
        case 0x584:
        case 0x585:
        case 0x586:
        case 0x587:
            phase = WP_PHASE_OTHER_HAND;
            break;
        case 0x582:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_OTHER_HAND;
            break;
        case 0x597:
            if (Motion_ReachedFrame(pl, 0, 20.0f)) phase = WP_PHASE_HAND;
            else if (Motion_ReachedFrame(pl, 0, 6.0f)) phase = WP_PHASE_OTHER_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    case WEAPON_LONG_SWORD:
        switch (pl->motionId[0]) {
        case 0x3EA:
            if (Motion_ReachedFrame(pl, 0, 56.0f)) phase = WP_PHASE_HAND;
            else if (Motion_ReachedFrame(pl, 0, 16.0f)) phase = WP_PHASE_OTHER_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(pl, 0, 52.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_OTHER_HAND;
            break;
        case 0x587:
            if (Motion_ReachedFrame(pl, 0, 28.0f)) phase = WP_PHASE_HAND;
            else if (Motion_ReachedFrame(pl, 0, 8.0f)) phase = WP_PHASE_OTHER_HAND; else phase = WP_PHASE_SHEATHED;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 0x592:
            if (Motion_ReachedFrame(pl, 0, 4.0f)) phase = WP_PHASE_OTHER_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 0x3ED:
            if (Motion_ReachedFrame(pl, 0, 44.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_OTHER_HAND;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    case WEAPON_BOW:
        switch (pl->motionId[0]) {
        case 0x3EA:
            if (Motion_ReachedFrame(pl, 0, 18.0f)) phase = WP_PHASE_HAND;
            break;
        case 0x3F2:
            if (Motion_ReachedFrame(pl, 0, 12.0f)) phase = WP_PHASE_HAND;
            break;
        case PL_MOT_BOW_CHARGE_FROM_DRAW:
            if (Motion_ReachedFrame(pl, 0, 6.0f)) phase = WP_PHASE_HAND;
            break;
        case 0x3F6:
            if (Motion_ReachedFrame(pl, 0, 4.0f)) phase = WP_PHASE_HAND;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(pl, 0, 64.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_HAND;
            break;
        case 0x3ED:
            if (Motion_ReachedFrame(pl, 0, 78.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_HAND;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    case WEAPON_HEAVY_BOWGUN:
        switch (pl->motionId[0]) {
        case 0x3EA:
            if (Motion_ReachedFrame(pl, 0, 2.0f)) phase = WP_PHASE_HAND;
            break;
        case 0x3EB:
            if (Motion_ReachedFrame(pl, 0, 110.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_HAND;
            break;
        case 0x3F1:
            if (Motion_ReachedFrame(pl, 0, 92.0f)) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_HAND;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    case WEAPON_LIGHT_BOWGUN:
        switch (pl->motionId[0]) {
        case 0x3EA:
            if (Motion_ReachedFrame(pl, 0, 28.0f)) phase = WP_PHASE_OTHER_HAND; else phase = WP_PHASE_SHEATHED;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 0x58E:
            if (Motion_ReachedFrame(pl, 0, 26.0f)) phase = WP_PHASE_HAND; else phase = WP_PHASE_SHEATHED;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 0x58F:
            if (Motion_ReachedFrame(pl, 0, 24.0f)) phase = WP_PHASE_HAND; else phase = WP_PHASE_SHEATHED;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 0x3F2:
            if (Motion_ReachedFrame(pl, 0, 22.0f)) phase = WP_PHASE_OTHER_HAND; else phase = WP_PHASE_SHEATHED;
            if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
            break;
        case 0x3EB:
        case 0x3F1:
            if (pl->motion[0].blend == 1) phase = WP_PHASE_HAND; else phase = WP_PHASE_SHEATHED;
            break;
        default:
            if (pl->weaponOut) phase = WP_PHASE_HAND;
            break;
        }
        break;
    }

    switch (pl->motionId[0]) {
    case 0xCA:
    case 0xCC:
        if (pl->weaponOut) phase = WP_PHASE_HAND;
        break;
    case 0xCF:
    case 0xD8:
        if (pl->weaponOut) {
            if (pl->stateGroup == PL_GROUP_GESTURE) phase = WP_PHASE_SHEATHED; else phase = WP_PHASE_HAND;
        }
        break;
    case 0x3C:
    case 0x3D:
    case 0x280:
    case 0x26C:
    case 0x26D:
    case 0x26E:
    case 0x26F:
    case 0x270:
    case 0x271:
    case 0x272:
    case 0x273:
    case 0x274:
    case 0x275:
    case 0x276:
    case 0x277:
    case 0x278:
    case 0x279:
        phase = WP_PHASE_SHEATHED;
        break;
    case 0xCD:
    case 0xCE:
    case 0xD0:
    case 0xD1:
    case 0xD3:
    case 0xD4:
    case 0xD5:
    case 0xD6:
    case 0xD7:
    case 0xDB:
    case 0xDC:
        phase = WP_PHASE_SHEATHED;
        break;
    case 0xD2:
        if (pl->stateGroup == PL_GROUP_GESTURE) phase = WP_PHASE_SHEATHED;
        break;
    case 0x19D:
        if (Motion_ReachedFrame(pl, 0, 328.0f)) phase = WP_PHASE_SHEATHED;
        else if (Motion_ReachedFrame(pl, 0, 22.0f)) phase = WP_PHASE_HAND;
        else phase = WP_PHASE_SHEATHED;
        if (pl->motion[0].blend == 1) phase = WP_PHASE_SHEATHED;
        break;
    case 0x349:
        if (otherPlayer == 1) phase = WP_PHASE_SHEATHED;
        break;
    }
    return phase;
}

void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void Hud_DrawTargetMarker(f32 *pos);
extern f32 gBoneMarkerDirs[][3];
void Mtx_Identity(f32 *m);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
extern f32 gObjMarkerOffset[];

/* Draws the target marker (Hud_DrawTargetMarker) at bone record 14 of the player, moved
 * along direction dirIndex of gBoneMarkerDirs turned by that bone. */
void Player_DrawBoneMarker(Player *pl, u8 dirIndex)
{
    f32 pos[3];
    f32 dir[3];

    Vec_Rotate(dir, gBoneMarkerDirs[dirIndex], pl->partRecs[14]->m1);
    Mtx_GetTrans(pos, pl->partRecs[14]->m1);
    Vec_Add(pos, pos, dir);
    Hud_DrawTargetMarker(pos);
}

/* Draws the target marker (Hud_DrawTargetMarker) in front of the player: its position plus
 * gObjMarkerOffset turned by its aim (-aimPitch and rot[1], 16-bit turns), 100 higher. */
void Hud_DrawObjMarker(Player *pl)
{
    f32 rotMtx[16];
    f32 markerPos[3];

    Mtx_Identity(rotMtx);
    Mtx_RotateXYZ(rotMtx, 6.2831855f * (f32)(-pl->aimPitch) / 65536.0f,
                  6.2831855f * (f32)pl->rot[1] / 65536.0f, 0.0f);
    Vec_Rotate(markerPos, gObjMarkerOffset, rotMtx);
    Vec_Add(markerPos, markerPos, pl->pos);
    markerPos[1] += 100.0f;
    Hud_DrawTargetMarker(markerPos);
}

extern u8 gFishColors[][3];
extern u8 gFoodMat0Colors[][3];
extern u8 gFoodMat1Colors[][3];
void Draw_ApplyStates(u32 a);
s32 Model_Draw(u32 h, s32 flags);

/* Draws one part of a held prop with the matrix mtx and its materials (lights: the model's
 * ModelMat array, read as bytes), each with the player's alpha. mode colours them: 1 and 2 the
 * first two materials by row colorRow of gFoodMat0Colors and gFoodMat1Colors (1: the spit
 * roast's cookResult; 2: the food eaten, useItem, or nothing drawn for another item); 3, 4
 * and 5 the first material dark, white or light grey; 6 the second colour (RS_COLOR2) set to
 * fish colour 3 (gFishColors); 0 as the model has them. */
void Player_DrawPartLit(Player *pl, ModelMatSet *part, f32 *mtx, u8 *lights, u8 mode)
{
    s16 colorRow;
    s32 i;
    u8 red;
    u8 green;
    u8 blue;
    f32 *light;

    if (part != NULL && part->id != -1) {
        switch (mode) {
        case 1:
            colorRow = pl->cookResult;
            break;
        case 2:
            switch (pl->useItem) {
            case ITEM_ID_FOOD_STAMINA_75:
            case ITEM_ID_FOOD_RESTORE:
                colorRow = 1;
                break;
            case ITEM_ID_FOOD_STAMINA_150:
            case ITEM_ID_FOOD_REGEN_SHORT:
            case ITEM_ID_FOOD_REGEN:
            case ITEM_ID_FOOD_REGEN_B:
            case ITEM_ID_FOOD_REGEN_LONG:
                colorRow = 2;
                break;
            case ITEM_ID_FOOD_STAMINA_GAMBLE:
            case ITEM_ID_FOOD_RESTORE_B:
                colorRow = 3;
                break;
            case ITEM_ID_FOOD_STAMINA_FULL:
                colorRow = 4;
                break;
            case ITEM_ID_FOOD_HOT:
            case ITEM_ID_FOOD_HOT_STAMINA:
            case ITEM_ID_FOOD_COOL:
            case ITEM_ID_FOOD_COOL_STAMINA:
            case ITEM_ID_FOOD_CURE:
            case ITEM_ID_FOOD_CURE_STAMINA:
                colorRow = 4;
                break;
            default:
                return;
            }
            break;
        case 3:
            red = green = blue = 0x3F;
            break;
        case 4:
            red = green = blue = 0xFF;
            break;
        case 5:
            red = green = blue = 0x9F;
            break;
        case 6:
            Gs_SetState(RS_COLOR2, 0xFF000000 | gFishColors[3][0] << 16 | gFishColors[3][1] << 8 | gFishColors[3][2]);
            break;
        }
        Gs_SetState(RS_MTX_SLOT(0), (u32)mtx);
        for (i = 0; i < part->count; i++) {
            light = (f32 *)(lights + part->mat[i] * 0x4C);
            light[4] = pl->alpha;
            switch (mode) {
            case 1:
            case 2:
                switch (i) {
                case 0:
                    light[1] = gFoodMat0Colors[colorRow][0] / 255.0f;
                    light[2] = gFoodMat0Colors[colorRow][1] / 255.0f;
                    light[3] = gFoodMat0Colors[colorRow][2] / 255.0f;
                    break;
                case 1:
                    light[1] = gFoodMat1Colors[colorRow][0] / 255.0f;
                    light[2] = gFoodMat1Colors[colorRow][1] / 255.0f;
                    light[3] = gFoodMat1Colors[colorRow][2] / 255.0f;
                    break;
                }
                break;
            case 3:
            case 4:
            case 5:
                switch (i) {
                case 0:
                    light[1] = red / 255.0f;
                    light[2] = green / 255.0f;
                    light[3] = blue / 255.0f;
                    break;
                }
                break;
            }
            Gs_SetState((u8)RS_MATERIAL(i), (u32)light);
        }
        Draw_ApplyStates(part->flags);
        Model_Draw(part->id, 0);
    }
}

s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame);
s16 Effect_MapTexId(s16 id);

/* Picks the sound or effect (Effect_MapTexId) of motions 0x327-0x32A by the frames reached, and returns
 * its (s16) result. */
s16 Player_HeldPartForMotion(void *obj)
{
    ObjHead *head = obj;
    s16 texId;

    switch (head->motionId[0]) {
    case 0x327:
        if (Motion_ReachedFrame(head, 0, 20.0f) == 0) {
            texId = 0x44;
        } else if (Motion_ReachedFrame(head, 0, 30.0f) == 0) {
            texId = 0x83;
        } else {
            texId = 0x82;
        }
        break;
    case 0x328:
        if (Motion_ReachedFrame(head, 0, 20.0f) == 0) {
            texId = 0x83;
        } else if (Motion_ReachedFrame(head, 0, 26.0f) == 0) {
            texId = 0x82;
        } else if (Motion_ReachedFrame(head, 0, 28.0f) == 0) {
            texId = 0x83;
        } else {
            texId = 0x44;
        }
        break;
    case 0x329:
        if (Motion_ReachedFrame(head, 0, 36.0f) == 0) {
            texId = 0x83;
        } else if (Motion_ReachedFrame(head, 0, 40.0f) == 0) {
            texId = 0x82;
        } else if (Motion_ReachedFrame(head, 0, 42.0f) == 0) {
            texId = 0x83;
        } else {
            texId = 0x44;
        }
        break;
    case 0x32A:
        if (Motion_ReachedFrame(head, 0, 20.0f) == 0) {
            texId = 0x83;
        } else if (Motion_ReachedFrame(head, 0, 22.0f) != 0) {
            texId = 0x82;
        } else if (Motion_ReachedFrame(head, 0, 24.0f) != 0) {
            texId = 0x83;
        } else {
            texId = 0x44;
        }
        break;
    default:
        texId = 0x44;
        break;
    }
    return Effect_MapTexId(texId);
}

s16 Player_IsState(void *obj, u8 state, u8 sub);
struct EmBone *Em_GetBone(void *obj, s16 bone);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Draw_SetDefaultState(void);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Vec_RotateY(f32 *v, f32 a);
f32 Math_Atan2(f32 y, f32 x);

/* Draws the prop the player holds, a part of the stage's model set (gStageModelSet) placed at
 * a bone: the carried item (Player.carry: its kind picks the part, size and colour), the mash
 * status's prop (PL_STATUS_ICON_MASH), or the prop of the motion playing: the item motions
 * 0x19F to 0x1A9 (0x1A0, eating: the food by useItem), the spit roast (0x320 to 0x323: the
 * meat, with the roaster standing on the ground in front of the player), motions 0x324 to
 * 0x32A (the part Player_HeldPartForMotion picks), and the bow's arrow in its motions. */
/* The player comes in as `void *obj`, read through `Player *pl = obj;`, as the other player
 * helpers take it: with a Player * parameter the compiler swaps the two registers that hold
 * the player and the model set.
 * FAKEMATCH: the spit roast's last motion (0x323) reaches off[1] and off[2] through the
 * pointers offY and offZ. Written as off[1] and off[2], the compiler gives that block its own
 * copies of their addresses, 4 instructions more than the game's
 * (docs/notes/nm/Player_DrawHeldProp.md). */
void Player_DrawHeldProp(void *obj)
{
    Model *set;
    Player *pl = obj;
    u8 *lights;
    ModelMatSet *part;
    ModelMatSet *roasterPart;
    s16 drawn;
    s32 onBone;
    s32 idx;
    s16 bone;
    s16 mode;
    s16 stand;
    f32 *offY;
    f32 *offZ;

    f32 pos[3];
    f32 off[3];
    f32 propMtx[16];
    f32 boneMtx[16];
    f32 uvMtx[16];
    f32 scaleX;
    f32 scaleY;
    f32 scaleZ;
    f32 rotX;
    f32 rotY;
    f32 rotZ;
    f32 reachFrame;
    f32 x;
    f32 y;
    f32 z;

    drawn = 0;
    mode = 0;
    set = gStageModelSet;
    if (set != NULL && set->active != 0) {
        lights = (u8 *)set->mats;
        scaleX = 1.0f;
        scaleY = scaleX;
        scaleZ = scaleX;
        if (pl->carry & PL_CARRY_HOLD_MASK) {
            if (Player_IsState((u8 *)pl, PL_GROUP_CARRY, PL_CARRY_PICKED_UP) != 0 && pl->motionId[0] == 0x33 && pl->motion[0].blend != 0) {
                return;
            }
            switch ((pl->carry & PL_CARRY_KIND_MASK) >> PL_CARRY_KIND_SHIFT) {
            case 0:
            case 1:
            case 6:
                bone = PL_BONE_HAND;
                part = &set->matSets[133];
                scaleY *= 1.3f;
                rotX = -0.9424778f;
                rotY = -3.2812192f;
                rotZ = 2.0594885f;
                off[0] = 5.5f;
                off[1] = -28.0f;
                off[2] = 12.0f;
                Mtx_Translation(uvMtx, 0.0f, 0.0625f, 0.0f);
                Gs_SetState(RS_UV_MTX, (u32)uvMtx);
                break;
            case 2:
                rotX = -0.9424778f;
                bone = PL_BONE_HAND;
                part = &set->matSets[133];
                rotY = -3.2812192f;
                rotZ = 2.0594885f;
                off[0] = 8.9f;
                off[1] = -25.0f;
                off[2] = 4.6f;
                Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (u32)uvMtx);
                break;
            case 3:
            case 4:
            case 5:
                part = &set->matSets[41];
                scaleX *= 2.5f;
                scaleY *= 3.0f;
                scaleZ *= 2.0f;
                bone = PL_BONE_HAND;
                rotX = -0.24434611f;
                rotY = 2.86234f;
                rotZ = 1.2566371f;
                off[0] = 3.3f;
                off[1] = -24.0f;
                off[2] = 4.0f;
                switch ((pl->carry & PL_CARRY_KIND_MASK) >> PL_CARRY_KIND_SHIFT) {
                case 5:
                    mode = 3;
                    break;
                case 3:
                    mode = 4;
                    break;
                case 4:
                    mode = 5;
                    break;
                }
                break;
            default:
                return;
            }
        } else if (pl->statusIcons & PL_STATUS_ICON_MASH) {
            rotZ = 0.0f;
            bone = 2;
            rotY = rotZ;
            rotX = rotZ;
            part = &set->matSets[145];
            off[2] = 0.0f;
            off[1] = 0.0f;
            off[0] = 0.0f;
        } else {
            switch (pl->motionId[0]) {
            case 0x19F:
                part = &set->matSets[Effect_MapTexId(0x46)];
                bone = PL_BONE_HAND;
                rotX = 0.36651915f;
                rotY = 0.17453294f;
                rotZ = 1.3613569f;
                off[0] = 17.0f;
                off[1] = 0.9f;
                off[2] = 22.0f;
                break;
            case 0x1A0:
                if ((pl->motion[0].blend == 0 || pl->stateVar1 == 0)
                    && (Motion_ReachedFrame(pl, 0, 16.0f) == 0 || Motion_ReachedFrame(pl, 0, 164.0f) != 0)) {
                    return;
                }
                switch (pl->useItem) {
                case ITEM_ID_FOOD_HEAL_20:
                case ITEM_ID_FOOD_PRIZE_A:
                case ITEM_ID_FOOD_PRIZE_B:
                    part = &set->matSets[114];
                    bone = PL_BONE_OTHER_HAND;
                    rotX = -1.2217306f;
                    rotY = 0.08726647f;
                    rotZ = -0.38397247f;
                    mode = 6;
                    off[0] = -8.0f;
                    off[1] = -3.0f;
                    off[2] = 0.0f;
                    break;
                case ITEM_ID_FOOD_RESTORE:
                case ITEM_ID_FOOD_REGEN_SHORT:
                case ITEM_ID_FOOD_REGEN:
                case ITEM_ID_FOOD_REGEN_B:
                case ITEM_ID_FOOD_REGEN_LONG:
                case ITEM_ID_FOOD_RESTORE_B:
                    scaleX = 0.75f;
                    scaleY = scaleX;
                    part = &set->matSets[128];
                    bone = PL_BONE_OTHER_HAND;
                    rotX = -0.38397247f;
                    rotY = 0.296706f;
                    mode = 2;
                    rotZ = -1.9722221f;
                    off[0] = -17.0f;
                    off[1] = -30.0f;
                    off[2] = 1.0f;
                    scaleZ = scaleX;
                    break;
                default:
                    part = &set->matSets[58];
                    bone = PL_BONE_OTHER_HAND;
                    rotX = -0.38397247f;
                    rotY = 0.296706f;
                    rotZ = -1.9722221f;
                    mode = 2;
                    off[0] = -17.0f;
                    off[1] = -30.0f;
                    off[2] = 1.0f;
                    break;
                }
                break;
            case 0x1A2:
                if (Motion_ReachedFrame(pl, 0, 118.0f) != 0) {
                    return;
                }
                rotX = 0.0f;
                bone = PL_BONE_OTHER_HAND;
                rotY = 3.1415927f;
                rotZ = rotY;
                part = &set->matSets[71];
                off[0] = -9.0f;
                off[1] = -1.0f;
                off[2] = 65.0f;
                break;
            case 0x1A3:
            case 0x1A6:
                switch (pl->motionId[0]) {
                case 0x1A3:
                    if (Motion_ReachedFrame(pl, 0, 26.0f) == 0 || Motion_ReachedFrame(pl, 0, 248.0f) != 0) {
                        return;
                    }
                    break;
                case 0x1A6:
                    if (Motion_ReachedFrame(pl, 0, 20.0f) == 0) {
                        return;
                    }
                    break;
                }
                rotX = 0.0f;
                bone = PL_BONE_HAND;
                rotY = rotX;
                rotZ = rotX;
                part = &set->matSets[120];
                off[0] = 8.0f;
                off[1] = 0.0f;
                off[2] = -6.0f;
                break;
            case 0x1A4:
            case 0x1A5:
                if (pl->motionId[0] == 0x1A4) {
                    if (Motion_ReachedFrame(pl, 0, 6.0f) == 0) {
                        return;
                    }
                } else if (pl->motionId[0] == 0x1A5) {
                    if (Motion_ReachedFrame(pl, 0, 34.0f) != 0) {
                        return;
                    }
                }
                bone = PL_BONE_OTHER_HAND;
                rotX = -1.0821042f;
                rotY = -2.9670596f;
                part = &set->matSets[19];
                rotZ = 0.69813174f;
                off[0] = -8.4f;
                off[1] = -3.8f;
                off[2] = -5.2f;
                break;
            case 0x1A8:
            case 0x1A9:
                if (pl->motionId[0] == 0x1A8) {
                    if (pl->motion[0].blend != 0) {
                        return;
                    }
                } else if (Motion_ReachedFrame(pl, 0, 58.0f) != 0) {
                    return;
                }
                bone = PL_BONE_OTHER_HAND;
                rotX = 2.86234f;
                rotY = -0.41887903f;
                part = &set->matSets[83];
                rotZ = -1.0646509f;
                off[0] = -12.0f;
                off[1] = -7.0f;
                off[2] = -3.0f;
                break;
            case 0x320:
            case 0x321:
            case 0x322:
            case 0x323:
                switch (pl->useItem) {
                case 0xA3:
                case 0xA4:
                case 0xA5:
                case 0xA6:
                case 0xA7:
                default:
                    idx = 0x3A;
                    break;
                case 0xA8:
                case 0xA9:
                    idx = 0x80;
                    break;
                }
                stand = 1;
                switch (pl->motionId[0]) {
                case 0x320:
                    if (Motion_ReachedFrame(pl, 0, 26.0f) == 0) {
                        return;
                    }
                    onBone = 0;
                    break;
                case 0x321:
                    if (Motion_ReachedFrame(pl, 0, 106.0f) != 0) {
                        return;
                    }
                    if (Motion_ReachedFrame(pl, 0, 96.0f) != 0) {
                        stand = 0;
                    }
                    onBone = 1;
                    break;
                case 0x322:
                    if (Motion_ReachedFrame(pl, 0, 100.0f) != 0) {
                        return;
                    }
                    if (Motion_ReachedFrame(pl, 0, 70.0f) != 0) {
                        stand = 2;
                    }
                    onBone = 1;
                    break;
                default:
                    onBone = 0;
                    break;
                }
                if (stand) {
                    roasterPart = &set->matSets[69];
                    Mtx_Translation(propMtx, pl->pos[0], pl->floorY, pl->pos[2]);
                    Mtx_RotateY(propMtx, (6.2831855f * pl->rot[1]) / 65536.0f);
                    Gs_SetState(RS_MTX_SLOT(0), (u32)propMtx);
                    Gs_SetState(RS_COLOR2, -1);
                    Player_DrawPartLit(pl, roasterPart, propMtx, lights, 0);
                    if (stand == 2) {
                        return;
                    }
                }
                if (onBone) {
                    part = &set->matSets[idx];
                    bone = PL_BONE_HAND;
                    rotX = 1.0471976f;
                    rotY = -0.7330383f;
                    rotZ = 0.41887903f;
                    off[0] = 37.0f;
                    off[1] = 14.0f;
                    off[2] = 37.0f;
                } else {
                    off[0] = 0.0f;
                    off[1] = 0.0f;
                    off[2] = 128.0f;
                    Vec_RotateY(off, (6.2831855f * pl->rot[1]) / 65536.0f);
                    pos[0] = pl->pos[0] + off[0];
                    pos[1] = 77.0f + pl->floorY;
                    pos[2] = pl->pos[2] + off[2];
                    off[0] = -52.0f;
                    off[1] = 0.0f;
                    off[2] = 0.0f;
                    rotY = (6.2831855f * (pl->rot[1] + 0x7FFF + 0x4001)) / 65536.0f;
                    Vec_RotateY(off, rotY);
                    x = pos[0] + off[0];
                    y = pos[1] + off[1];
                    z = pos[2] + off[2];
                    if (pl->motionId[0] == 0x323) {
                        offY = &off[1]; /* FAKEMATCH: see above */
                        offZ = &off[2];
                        off[0] = 12.0f;
                        *offY = 0.0f;
                        *offZ = 9.0f;
                        Mtx_Copy(propMtx, (void *)Em_GetBone(pl, PL_BONE_HAND));
                        Vec_RotateSelf(off, propMtx);
                        off[0] += propMtx[12];
                        *offY += propMtx[13];
                        *offZ += propMtx[14];
                        off[0] -= x;
                        *offY -= y;
                        *offZ -= z;
                        Vec_RotateY(off, -rotY);
                        rotX = Math_Atan2(*offZ, *offY);
                    } else {
                        rotX = 0.0f;
                    }
                    Mtx_Identity(propMtx);
                    Mtx_RotateX(propMtx, rotX);
                    Mtx_RotateY(propMtx, rotY);
                    roasterPart = &set->matSets[60];
                    Mtx_SetTrans(propMtx, x, y, z);
                    Gs_SetState(RS_COLOR2, 0xFF727162);
                    Player_DrawPartLit(pl, roasterPart, propMtx, lights, 0);
                    part = &set->matSets[idx];
                    rotZ = 0.0f;
                    drawn = 1;
                    Mtx_SetTrans(propMtx, pos[0], pos[1], pos[2]);
                }
                mode = 1;
                break;
            case 0x32A:
                if (Motion_ReachedFrame(pl, 0, 22.0f) != 0) {
                    return;
                }
            case 0x324:
            case 0x325:
            case 0x326:
            case 0x327:
            case 0x328:
            case 0x329:
                part = &set->matSets[Player_HeldPartForMotion(pl)];
                bone = PL_BONE_OTHER_HAND;
                rotZ = -1.5707964f;
                off[0] = -9.2f;
                rotX = 0.0f;
                off[1] = -1.7f;
                rotY = rotX;
                off[2] = 12.0f;
                break;
            case 0x3F1:
            case 0x3F4:
            case 0x3F5:
            case 0x57D:
            case 0x58A:
                if (pl->weaponType == WEAPON_BOW) {
                    switch (pl->motionId[0]) {
                    case PL_MOT_BOW_CHARGE_FROM_DRAW:
                        reachFrame = 6.0f;
                        if (Motion_ReachedFrame(pl, 0, reachFrame) == 0) {
                            return;
                        }
                        break;
                    case PL_MOT_BOW_CHARGE_FROM_STAB:
                        reachFrame = 4.0f;
                        if (Motion_ReachedFrame(pl, 0, reachFrame) == 0) {
                            return;
                        }
                        break;
                    default:
                        reachFrame = 0.0f;
                        break;
                    }
                    scaleX = scaleY = 1.0f;
                    if (((u32)(pl->motionId[0] - PL_MOT_BOW_CHARGE_FROM_DRAW) <= 1 || pl->motionId[0] == PL_MOT_BOW_CHARGE
                        || pl->motionId[0] == PL_MOT_BOW_CHARGE_FROM_SHOT)
                        && Motion_ReachedFrame(pl, 0, reachFrame) != 0 && Motion_ReachedFrame(pl, 0, 12.0f + reachFrame) == 0) {
                        scaleZ = (0.5f * (pl->motion[0].frame - reachFrame)) / 12.0f;
                    } else {
                        scaleZ = scaleX;
                    }
                    bone = PL_BONE_OTHER_HAND;
                    part = &set->matSets[149];
                    rotX = 1.5707964f;
                    rotY = 0.0f;
                    rotZ = -1.5707964f;
                    off[0] = -10.0f;
                    off[1] = 0.0f;
                    off[2] = 0.0f;
                } else {
                    return;
                }
                break;
            case 0x3F6:
            case 0x583:
            case 0x585:
            case 0x586:
                if (pl->weaponType == WEAPON_BOW) {
                    switch (pl->motionId[0]) {
                    case 0x3F6:
                        if (Motion_ReachedFrame(pl, 0, 12.0f) == 0) {
                            return;
                        }
                        break;
                    case 0x583:
                        if (Motion_ReachedFrame(pl, 0, 76.0f) != 0) {
                            return;
                        }
                        break;
                    case 0x585:
                        if (Motion_ReachedFrame(pl, 0, 12.0f) == 0 || Motion_ReachedFrame(pl, 0, 98.0f) != 0) {
                            return;
                        }
                        break;
                    case 0x586:
                        if (Motion_ReachedFrame(pl, 0, 100.0f) != 0) {
                            return;
                        }
                        break;
                    }
                    bone = PL_BONE_OTHER_HAND;
                    part = &set->matSets[149];
                    rotX = 3.1415927f;
                    rotY = 0.0f;
                    rotZ = -1.5707964f;
                    off[0] = -10.0f;
                    off[1] = 0.0f;
                    off[2] = 80.0f;
                    scaleZ = 0.9f;
                    scaleY = scaleZ;
                    scaleX = scaleZ;
                } else {
                    return;
                }
                break;
            default:
                return;
            }
        }
        if (drawn == 0) {
            Mtx_Copy(boneMtx, (void *)Em_GetBone(pl, bone));
            Mtx_GetTrans(pos, boneMtx);
            Vec_RotateSelf(off, boneMtx);
            pos[0] += off[0];
            pos[1] += off[1];
            pos[2] += off[2];
            Mtx_Scale(propMtx, scaleX, scaleY, scaleZ);
            Mtx_RotateXYZ(propMtx, rotX, rotY, rotZ);
            Mtx_Concat3x3(propMtx, boneMtx);
        }
        Mtx_SetTrans(propMtx, pos[0], pos[1], pos[2]);
        Gs_SetState(RS_COLOR2, -1);
        Player_DrawPartLit(pl, part, propMtx, lights, mode);
        Draw_SetDefaultState();
    }
}

/* Where a weapon model sits on its bone: an offset in the bone's frame and a rotation (radians). */
typedef struct WpPose {
    f32 off[3];
    f32 rot[3];
} WpPose;

void Bone_StoreMatrices(struct EmBone *b);
void Bone_UpdateWorldScaledRoot(struct EmBone *b, f32 *m);
s32 Model_Draw(u32 h, s32 flags);
void Vec_Set(f32 *v, f32 x, f32 y, f32 z);
void Mtx_SetRotXYZ(f32 *m, f32 x, f32 y, f32 z);
void Tex_Prepare(s32 n, s32 tex);
struct KeyClock;
f32 Key1_Eval(struct KeyClock *c, Key1 *k);
void Key2_Eval(struct KeyClock *c, Key2 *k, f32 *a, f32 *b);
void Key3_Eval(struct KeyClock *c, Key3 *k, f32 *out);
void Key3_EvalByTimer(Player *p, Key3 *k, f32 *out);
void Mtx_AddTrans(f32 *m, f32 *v);
s32 Bowgun_HasBarrel(Player *p);
s32 HeavyBowgun_HasShield(Player *p);
s32 LightBowgun_HasSilencer(Player *p);
extern WpPose gWpPoseDrawnAlt[];
extern WpPose gWpPoseDrawn[];
extern WpPose gWpPoseSheathed[];
extern WpPose gWpPoseSharpen[];
extern Key1 gLbgKey579ScaleZ[];
extern Key1 gHbgKey579ScaleZ[];
extern Key1 gLbgKey579ScaleY[];
extern Key1 gHbgKey579ScaleX[];
extern Key1 gLbgKey57CScaleZ[];
extern Key1 gHbgKey57CScaleZ[];
extern Key1 gLbgKey57CScaleY[];
extern Key1 gHbgKey57CScaleX[];
extern Key1 gLbgKey57DEmptyScaleZ[];
extern Key1 gHbgKey57DEmptyScaleZ[];
extern Key1 gLbgKey57DEmptyScaleY[];
extern Key1 gHbgKey57DEmptyScaleX[];
extern Key1 gLbgKey57DScaleZ[];
extern Key1 gHbgKey57DScaleZ[];
extern Key1 gLbgKey57DScaleY[];
extern Key1 gHbgKey57DScaleX[];
extern Key1 gBowgunKey580ScaleZ[];
extern Key1 gBowgunKey580Scale2[];
extern Key1 gHbgKey3EARotX[];
extern Key1 gHbgKey3EAScaleZ[];
extern Key1 gHbgKey3EAScaleX[];
extern Key1 gHbgKey3EBRotX[];
extern Key1 gHbgKey3F1ScaleZ[];
extern Key1 gHbgKey3EBScaleX[];
extern Key3 gLbgKey3EARot[];
extern Key3 gLbgKey3EAOff[];
extern Key3 gLbgKey3EBRot[];
extern Key3 gLbgKey3EBOff[];
extern Key3 gLbgKey3F1Rot[];
extern Key3 gLbgKey3F1Off[];
extern Key3 gLbgKey3F2Rot[];
extern Key3 gLbgKey3F2Off[];
extern Key1 gLanceKey19DScaleZ[];
extern Key1 gLanceKey3EAScaleZ[];
extern Key1 gLanceKey3EBScaleZ[];
extern Key1 gLanceKey3F1ScaleZ[];
extern Key1 gLanceKey3F2ScaleZ[];
extern Key1 gLanceKey3FAScaleZ[];
extern Key1 gHammerKey57EScale[];
extern Key1 gHammerKey57CScale[];
extern Key1 gGsKey19DScaleZ[];
extern Key1 gGsKey3EAScaleZ[];
extern Key1 gGsKey3EBScaleZ[];
extern Key1 gGsKey3F9ScaleZ[];
extern Key1 gGsKey582ScaleZ[];
extern Key2 gGsKey19DScaleXZ[];
extern Key2 gGsKey3EAScaleXZ[];
extern Key2 gGsKey3EBScaleXZ[];
extern Key2 gGsKey3F9ScaleXZ[];
extern Key2 gGsKey582ScaleXZ[];
extern Key2 gHornKey57DRot[];
extern Key3 gHornKey57DScale[];
extern Key1 gGlKey19DRotX[];
extern Key1 gGlKey3EARotX[];
extern Key1 gGlKey3EBRotX[];
extern Key1 gGlKey3F2RotX[];
extern Key1 gGlKey57CRotX[];
extern Key3 gGlKey580Rot[];
extern Key3 gGlKeyTimerRot[];
extern Key1 gBowKey3EARotX[];
extern Key1 gBowKey3EBRotX[];
extern Key1 gBowKey3EDRotX[];
extern Key1 gBowKey3F4RotX[];
extern Key1 gBowKey3F6RotX[];
extern Key1 gBowKey57DRotX[];
extern Key1 gBowKey580RotX[];
extern Key1 gBowKey587RotX[];
extern Key1 gBowKey58ARotX[];
extern Key1 gBowKey3EAScaleZ[];
extern Key1 gBowKey3EBScaleZ[];
extern Key1 gBowKey3EDScaleZ[];
extern Key1 gBowKey3F4ScaleZ[];
extern Key1 gBowKey3F6ScaleZ[];
extern Key1 gBowKey57DScaleZ[];
extern Key1 gBowKey580ScaleZ[];
extern Key1 gBowKey587ScaleZ[];
extern Key1 gBowKey58AScaleZ[];

#pragma optimization_level 4
/*
 * Draws the player's weapon (weaponModel) at the alpha given:
 * - places it by Player_MotionPhase: on the hand's bone (PL_BONE_HAND, PL_BONE_OTHER_HAND) with
 *   gWpPoseDrawn/DrawnAlt (gWpPoseSharpen while sharpening, motion 0x19D), or sheathed on part
 *   record 9 (the sword and shield, the dual swords) or 10 (the others) with gWpPoseSheathed,
 *   scaled per weapon;
 * - scales and turns its moving bones by weapon type and motion, from keyframe tracks of the
 *   weapon's own motions (Key1_Eval, Key2_Eval, Key3_Eval) or fixed values; a bowgun's
 *   materials show by its parts (Bowgun_HasBarrel, HeavyBowgun_HasShield,
 *   LightBowgun_HasSilencer);
 * - draws its material sets, the second one of a lance, a sword and shield or dual swords (the
 *   shield or the other blade) on bone 0x11, by the second part's phase;
 * - then, for the long sword, the bow and the gunlance, draws again the materials that stay on
 *   the back while the weapon is out, on part record 10 by gWpPoseSheathed.
 * otherPlayer is set when Player_Draw draws another player's hunter (Player_MotionPhase).
 */
void Player_DrawWeapon(Player *pl, s32 otherPlayer, f32 alpha)
{
    Model *wpModel;
    ModelMatSet *set;
    WpPose *pose;
    ModelMat *material;
    f32 *poseSrc;
    s32 i;
    s32 j;
    s16 phase;
    s16 secondPhase;
    f32 scale;
    EmBone *bones;
    ModelMat *materials;
    f32 identity[16];
    f32 mtx[16];
    f32 scaleMtx[16];
    f32 off[3];
    f32 pos[3];
    f32 offWorld[3];
    f32 boneScale[3];
    f32 boneScale2[3];
    f32 rot[3];

    if (pl->weaponModel == NULL) {
        return;
    }
    Gs_SetState(RS_COLOR2, -1);
    Tex_Prepare(4, pl->index + 0x21);
    phase = Player_MotionPhase(pl, 0, otherPlayer);
    scale = 1.0f;
    wpModel = pl->weaponModel;
    bones = wpModel->partBones[0];
    set = wpModel->matSets;
    if (phase != WP_PHASE_SHEATHED) {
        Mtx_Copy(mtx, pl->partRecs[phase != WP_PHASE_OTHER_HAND ? PL_BONE_HAND : PL_BONE_OTHER_HAND]->m1);
        if (phase == WP_PHASE_OTHER_HAND) {
            pose = &gWpPoseDrawnAlt[pl->weaponType];
        } else {
            pose = &gWpPoseDrawn[pl->weaponType];
        }
        if (pl->motionId[0] == 0x19D) {
            pose = &gWpPoseSharpen[pl->weaponType];
        }
        poseSrc = pose->off;
        off[0] = *poseSrc++;
        off[1] = *poseSrc++;
        off[2] = *poseSrc++;
        rot[0] = *poseSrc++;
        rot[1] = *poseSrc++;
        rot[2] = *poseSrc++;
        Vec_Rotate(offWorld, off, pl->partRecs[phase != WP_PHASE_OTHER_HAND ? PL_BONE_HAND : PL_BONE_OTHER_HAND]->m1);
        Mtx_AddTrans(mtx, offWorld);
        Mtx_Identity(identity);
        if (pl->weaponType == WEAPON_LANCE || pl->weaponType == WEAPON_LIGHT_BOWGUN) {
            Mtx_GetTrans(pos, bones[2].local);
            Mtx_Scale(bones[2].local, 1.0f, 1.0f, 1.0f);
            Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
        }
    } else {
        switch (pl->weaponType) {
        case WEAPON_SWORD_SHIELD:
            Mtx_Copy(mtx, pl->partRecs[9]->m1);
            break;
        case WEAPON_DUAL_SWORDS:
            Mtx_Copy(mtx, pl->partRecs[9]->m1);
            break;
        default:
            Mtx_Copy(mtx, pl->partRecs[10]->m1);
            break;
        }
        if ((s32)pl->weaponType == WEAPON_LIGHT_BOWGUN
            && (pl->motionId[0] == 0x58E || pl->motionId[0] == 0x58F || pl->motionId[0] == 0x3EA || pl->motionId[0] == 0x3EB
                || pl->motionId[0] == 0x3F1 || pl->motionId[0] == 0x3F2)
            && pl->motion[0].blend == 0) {
            switch (pl->motionId[0]) {
            case 0x3EA:
            case 0x58E:
            case 0x58F:
                Key3_Eval((struct KeyClock *)pl, gLbgKey3EAOff, off);
                Key3_Eval((struct KeyClock *)pl, gLbgKey3EARot, rot);
                break;
            case 0x3EB:
                Key3_Eval((struct KeyClock *)pl, gLbgKey3EBOff, off);
                Key3_Eval((struct KeyClock *)pl, gLbgKey3EBRot, rot);
                break;
            case 0x3F1:
                Key3_Eval((struct KeyClock *)pl, gLbgKey3F1Off, off);
                Key3_Eval((struct KeyClock *)pl, gLbgKey3F1Rot, rot);
                break;
            case 0x3F2:
                Key3_Eval((struct KeyClock *)pl, gLbgKey3F2Off, off);
                Key3_Eval((struct KeyClock *)pl, gLbgKey3F2Rot, rot);
                break;
            }
            rot[0] = 6.2831855f * rot[0] / 360.0f;
            rot[1] = 6.2831855f * rot[1] / 360.0f;
            rot[2] = 6.2831855f * rot[2] / 360.0f;
        } else {
            pose = &gWpPoseSheathed[(s32)pl->weaponType];
            poseSrc = pose->off;
            off[0] = *poseSrc++;
            off[1] = *poseSrc++;
            off[2] = *poseSrc++;
            rot[0] = *poseSrc++;
            rot[1] = *poseSrc++;
            rot[2] = *poseSrc++;
        }
        if (pl->weaponType == WEAPON_DUAL_SWORDS) {
            Vec_Rotate(offWorld, off, pl->partRecs[9]->m1);
        } else {
            Vec_Rotate(offWorld, off, pl->partRecs[10]->m1);
        }
        Mtx_AddTrans(mtx, offWorld);
        switch (pl->weaponType) {
        case WEAPON_HEAVY_BOWGUN:
        case WEAPON_BOW:
            Mtx_GetTrans(pos, bones[3].local);
            Mtx_Scale(bones[3].local, 1.0f, 0.0f, 3.0f);
            Mtx_SetTrans(bones[3].local, pos[0], pos[1], pos[2]);
            scale = 0.8f;
            break;
        case WEAPON_LIGHT_BOWGUN:
            Mtx_GetTrans(pos, bones[3].local);
            Mtx_Scale(bones[3].local, 1.0f, 0.0f, 3.0f);
            Mtx_SetTrans(bones[3].local, pos[0], pos[1], pos[2]);
            Mtx_GetTrans(pos, bones[2].local);
            Mtx_Scale(bones[2].local, 1.0f, 1.0f, 0.5f);
            Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
            scale = 1.0f;
            break;
        case WEAPON_LONG_SWORD:
        default:
            scale = 0.8f;
            break;
        case WEAPON_SWORD_SHIELD:
        case WEAPON_DUAL_SWORDS:
            scale = 1.0f;
            break;
        case WEAPON_LANCE:
            scale = 0.9f;
            Mtx_GetTrans(pos, bones[2].local);
            Mtx_Scale(bones[2].local, 1.0f, 1.0f, 0.45f);
            Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
            break;
        }
    }
    Mtx_Identity(bones[0].local);
    Mtx_SetRotXYZ(bones[0].local, rot[0], rot[1], rot[2]);
    Mtx_Scale(scaleMtx, scale, scale, scale);
    Mtx_Concat3x3(bones[0].local, scaleMtx);
    switch (pl->weaponType) {
    case WEAPON_HAMMER:
        Mtx_GetTrans(pos, bones[3].local);
        boneScale[0] = 1.0f;
        if (pl->weaponTimer > 0 && pl->weaponTimer / 30 >= 2) {
            boneScale[0] = 1.5f;
        }
        switch (pl->motionId[0]) {
        case 0x57C:
            if (pl->motion[0].blend == 0) {
                boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHammerKey57CScale);
            } else {
                boneScale[0] = 1.5f;
            }
            break;
        case 0x57E:
            if (pl->motion[0].blend == 0) {
                boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHammerKey57EScale);
            } else {
                boneScale[0] = 1.5f;
            }
            break;
        }
        Mtx_Scale(bones[3].local, boneScale[0], boneScale[0], boneScale[0]);
        Mtx_SetTrans(bones[3].local, pos[0], pos[1], pos[2]);
        break;
    case WEAPON_HUNTING_HORN:
        Mtx_GetTrans(off, bones[2].local);
        Mtx_GetTrans(pos, bones[3].local);
        Vec_Set(rot, 0.0f, 0.0f, 0.0f);
        Vec_Set(boneScale, 1.0f, 1.0f, 1.0f);
        switch (pl->motionId[0]) {
        case 0x57D:
        case 0x57E:
        case 0x57F:
            Key2_Eval((struct KeyClock *)pl, gHornKey57DRot, &rot[0], &rot[2]);
            Key3_Eval((struct KeyClock *)pl, gHornKey57DScale, boneScale);
            break;
        }
        rot[0] = 6.2831855f * rot[0] / 360.0f;
        rot[1] = 6.2831855f * rot[1] / 360.0f;
        rot[2] = 6.2831855f * rot[2] / 360.0f;
        Mtx_Scale(bones[2].local, 1.0f, 1.0f, 1.0f);
        Mtx_SetRotXYZ(bones[2].local, rot[0], rot[1], rot[2]);
        Mtx_SetTrans(bones[2].local, off[0], off[1], off[2]);
        Mtx_Scale(bones[3].local, boneScale[0], boneScale[1], boneScale[2]);
        Mtx_SetTrans(bones[3].local, pos[0], pos[1], pos[2]);
        break;
    case WEAPON_GREAT_SWORD:
    case WEAPON_LONG_SWORD:
        Mtx_GetTrans(pos, bones[2].local);
        Mtx_GetTrans(off, bones[3].local);
        if (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED) {
            Vec_Set(boneScale2, 1.0f, 1.0f, 1.0f);
        } else {
            Vec_Set(boneScale2, 2.6f, 1.0f, 2.5f);
        }
        switch (pl->motionId[0]) {
        case 0x19D:
            if (pl->motion[0].blend == 0) {
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gGsKey19DScaleZ);
                Key2_Eval((struct KeyClock *)pl, gGsKey19DScaleXZ, &boneScale2[0], &boneScale2[2]);
            } else {
                boneScale[2] = 0.73f;
            }
            break;
        case 0x3EA:
        case 0x3F8:
        case 0x582:
            if (pl->motion[0].blend == 0) {
                if (pl->motionId[0] != 0x582) {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gGsKey3EAScaleZ);
                    Key2_Eval((struct KeyClock *)pl, gGsKey3EAScaleXZ, &boneScale2[0], &boneScale2[2]);
                } else {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gGsKey582ScaleZ);
                    Key2_Eval((struct KeyClock *)pl, gGsKey582ScaleXZ, &boneScale2[0], &boneScale2[2]);
                }
            } else {
                boneScale[2] = 0.73f;
                Vec_Set(boneScale2, 1.0f, 1.0f, 1.0f);
            }
            break;
        case 0x3EB:
        case 0x3F9:
            if (pl->motion[0].blend == 0) {
                if (pl->motionId[0] == 0x3EB) {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gGsKey3EBScaleZ);
                    Key2_Eval((struct KeyClock *)pl, gGsKey3EBScaleXZ, &boneScale2[0], &boneScale2[2]);
                } else {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gGsKey3F9ScaleZ);
                    Key2_Eval((struct KeyClock *)pl, gGsKey3F9ScaleXZ, &boneScale2[0], &boneScale2[2]);
                }
            } else {
                boneScale[2] = 1.0f;
                Vec_Set(boneScale2, 2.6f, 1.0f, 2.5f);
            }
            break;
        default:
            if (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED) {
                boneScale[2] = 0.73f;
            } else {
                boneScale[2] = 1.0f;
            }
            break;
        }
        if (pl->weaponType == WEAPON_LONG_SWORD) {
            if (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED) {
                boneScale[2] = 0.73f;
            } else {
                boneScale[2] = 1.0f;
            }
        }
        Mtx_Scale(bones[2].local, 1.0f, 1.0f, boneScale[2]);
        Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
        Mtx_Scale(bones[3].local, boneScale2[0], boneScale2[1], boneScale2[2]);
        Mtx_SetTrans(bones[3].local, off[0], off[1], off[2]);
        break;
    case WEAPON_LANCE:
        Mtx_GetTrans(pos, bones[2].local);
        switch (pl->motionId[0]) {
        case 0x19D:
            if (pl->motion[0].blend == 0) {
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLanceKey19DScaleZ);
            } else {
                boneScale[2] = 0.45f;
            }
            break;
        case 0x3EA:
        case 0x3F2:
            if (pl->motion[0].blend == 0) {
                if (pl->motionId[0] != 0x3F2) {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLanceKey3EAScaleZ);
                } else {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLanceKey3F2ScaleZ);
                }
            } else {
                boneScale[2] = 0.45f;
            }
            break;
        case 0x3EB:
        case 0x3F1:
            if (pl->motion[0].blend == 0) {
                if (pl->motionId[0] == 0x3EB) {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLanceKey3EBScaleZ);
                } else {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLanceKey3F1ScaleZ);
                }
            } else {
                boneScale[2] = 1.0f;
            }
            break;
        case 0x3FA:
            if (pl->motion[0].blend == 0) {
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLanceKey3FAScaleZ);
            } else {
                boneScale[2] = 0.45f;
            }
            break;
        default:
            if (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED) {
                boneScale[2] = 0.45f;
            } else {
                boneScale[2] = 1.0f;
            }
            break;
        }
        Mtx_Scale(bones[2].local, 1.0f, 1.0f, boneScale[2]);
        Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
        break;
    case WEAPON_GUNLANCE:
        Mtx_GetTrans(pos, bones[2].local);
        Vec_Set(rot, 0.0f, 0.0f, 0.0f);
        Mtx_GetTrans(pos, bones[2].local);
        Mtx_GetTrans(off, bones[3].local);
        switch (pl->motionId[0]) {
        case 0x19D:
            if (pl->motion[0].blend == 0) {
                rot[0] = Key1_Eval((struct KeyClock *)pl, gGlKey19DRotX);
            } else {
                rot[0] = 180.0f;
            }
            break;
        case 0x3EA:
            if (pl->motion[0].blend == 0) {
                rot[0] = Key1_Eval((struct KeyClock *)pl, gGlKey3EARotX);
            } else {
                rot[0] = 180.0f;
            }
            break;
        case 0x3F2:
            if (pl->motion[0].blend == 0) {
                rot[0] = Key1_Eval((struct KeyClock *)pl, gGlKey3F2RotX);
            } else {
                rot[0] = 180.0f;
            }
            break;
        case 0x3EB:
        case 0x3F1:
            if (pl->motion[0].blend == 0) {
                rot[0] = Key1_Eval((struct KeyClock *)pl, gGlKey3EBRotX);
            } else {
                rot[0] = 0.0f;
            }
            break;
        case 0x57C:
            if (pl->motion[0].blend == 0) {
                rot[0] = Key1_Eval((struct KeyClock *)pl, gGlKey57CRotX);
            } else {
                rot[0] = 0.0f;
            }
            break;
        default:
            if (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED) {
                rot[0] = 180.0f;
            } else {
                rot[0] = 0.0f;
            }
            break;
        }
        Mtx_SetRotXYZ(bones[2].local, 6.2831855f * rot[0] / 360.0f, 0.0f, 0.0f);
        Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
        if (gScene.inLobby != 0) {
            Vec_Set(rot, 0.0f, 0.0f, 0.0f);
        } else if (pl->weaponTimer > 0) {
            if (pl->motionId[0] == 0x580) {
                Key3_Eval((struct KeyClock *)pl, gGlKey580Rot, rot);
            } else {
                Vec_Set(rot, 69.0f, -43.0f, -30.0f);
            }
        } else if (pl->swordGaugeTimer > 0) {
            Key3_EvalByTimer(pl, gGlKeyTimerRot, rot);
        } else {
            Vec_Set(rot, 0.0f, 0.0f, 0.0f);
        }
        Mtx_SetRotXYZ(bones[3].local, 6.2831855f * rot[0] / 360.0f, 6.2831855f * rot[1] / 360.0f,
                      6.2831855f * rot[2] / 360.0f);
        Mtx_SetTrans(bones[3].local, off[0], off[1], off[2]);
        break;
    case WEAPON_BOW:
        Mtx_GetTrans(pos, bones[2].local);
        Mtx_GetTrans(off, bones[3].local);
        Vec_Set(boneScale, 1.0f, 1.0f, 1.0f);
        Vec_Set(rot, 0.0f, 0.0f, 0.0f);
        if (pl->motion[0].blend == 0) {
            switch (pl->motionId[0]) {
            case 0x3EA:
            case 0x3F2:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey3EARotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey3EAScaleZ);
                break;
            case 0x3EB:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey3EBRotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey3EBScaleZ);
                break;
            case 0x3ED:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey3EDRotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey3EDScaleZ);
                break;
            case PL_MOT_BOW_CHARGE_WALK:
                rot[0] = 120.0f;
                boneScale[2] = 2.35f;
                break;
            case PL_MOT_BOW_CHARGE_FROM_DRAW:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey3F4RotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey3F4ScaleZ);
                break;
            case 0x3F6:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey3F6RotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey3F6ScaleZ);
                break;
            case PL_MOT_BOW_CHARGE:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey57DRotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey57DScaleZ);
                break;
            case 0x580:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey580RotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey580ScaleZ);
                break;
            case 0x587:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey587RotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey587ScaleZ);
                break;
            case PL_MOT_BOW_CHARGE_FROM_SHOT:
                rot[0] = Key1_Eval((struct KeyClock *)pl, gBowKey58ARotX);
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowKey58AScaleZ);
                break;
            default:
                if (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED) {
                    rot[0] = 0.0f;
                } else {
                    rot[0] = 155.0f;
                }
                break;
            }
        } else {
            if ((pl->motionId[0] != 0x3EB && pl->motionId[0] != 0x3ED && (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED))
                || pl->motionId[0] == 0x3EA) {
                rot[0] = 0.0f;
            } else {
                rot[0] = 155.0f;
            }
            if (pl->motionId[0] == 0x580) {
                rot[0] = 110.0f;
                boneScale[2] = 2.7f;
            }
        }
        Mtx_SetRotXYZ(bones[2].local, 6.2831855f * rot[0] / 360.0f, 0.0f, 0.0f);
        Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
        Mtx_Scale(bones[3].local, boneScale[0], boneScale[1], boneScale[2]);
        Mtx_SetTrans(bones[3].local, off[0], off[1], off[2]);
        break;
    case WEAPON_HEAVY_BOWGUN:
    case WEAPON_LIGHT_BOWGUN:
        if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
            Mtx_GetTrans(off, bones[3].local);
        } else {
            Mtx_GetTrans(off, bones[3].local);
            Mtx_SetRotXYZ(bones[2].local, 3.1415927f, 0.0f, 0.0f);
        }
        boneScale[0] = 1.0f;
        boneScale[1] = 1.0f;
        boneScale[2] = 1.0f;
        if (pl->motion[0].blend == 0) {
            switch (pl->motionId[0]) {
            case 0x579:
                if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLbgKey579ScaleZ);
                    boneScale[1] = Key1_Eval((struct KeyClock *)pl, gLbgKey579ScaleY);
                } else {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gHbgKey579ScaleZ);
                    boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHbgKey579ScaleX);
                }
                break;
            case 0x57D:
                if (pl->roundsLoaded == 0) {
                    if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                        boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLbgKey57DEmptyScaleZ);
                        boneScale[1] = Key1_Eval((struct KeyClock *)pl, gLbgKey57DEmptyScaleY);
                    } else {
                        boneScale[2] = Key1_Eval((struct KeyClock *)pl, gHbgKey57DEmptyScaleZ);
                        boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHbgKey57DEmptyScaleX);
                    }
                } else if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLbgKey57DScaleZ);
                    boneScale[1] = Key1_Eval((struct KeyClock *)pl, gLbgKey57DScaleY);
                } else {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gHbgKey57DScaleZ);
                    boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHbgKey57DScaleX);
                }
                break;
            case 0x580:
                boneScale[2] = Key1_Eval((struct KeyClock *)pl, gBowgunKey580ScaleZ);
                if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                    boneScale[1] = Key1_Eval((struct KeyClock *)pl, gBowgunKey580Scale2);
                } else {
                    boneScale[0] = Key1_Eval((struct KeyClock *)pl, gBowgunKey580Scale2);
                }
                break;
            case 0x57C:
                if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gLbgKey57CScaleZ);
                    boneScale[1] = Key1_Eval((struct KeyClock *)pl, gLbgKey57CScaleY);
                } else {
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gHbgKey57CScaleZ);
                    boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHbgKey57CScaleX);
                }
                break;
            case 0x3EA:
            case 0x58E:
            case 0x58F:
                if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
                    Mtx_SetRotXYZ(bones[2].local, 6.2831855f * Key1_Eval((struct KeyClock *)pl, gHbgKey3EARotX) / 360.0f, 0.0f, 0.0f);
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gHbgKey3EAScaleZ);
                    boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHbgKey3EAScaleX);
                }
                break;
            case 0x3EB:
                if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
                    Mtx_SetRotXYZ(bones[2].local, 6.2831855f * Key1_Eval((struct KeyClock *)pl, gHbgKey3EBRotX) / 360.0f, 0.0f, 0.0f);
                    boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHbgKey3EBScaleX);
                }
                break;
            case 0x3F1:
                if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
                    Mtx_SetRotXYZ(bones[2].local, 6.2831855f * Key1_Eval((struct KeyClock *)pl, gHbgKey3EBRotX) / 360.0f, 0.0f, 0.0f);
                    boneScale[2] = Key1_Eval((struct KeyClock *)pl, gHbgKey3F1ScaleZ);
                    boneScale[0] = Key1_Eval((struct KeyClock *)pl, gHbgKey3EBScaleX);
                }
                break;
            default:
                if (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED) {
                    Mtx_SetRotXYZ(bones[1].local, 0.0f, 0.0f, 0.0f);
                    if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
                        Mtx_SetRotXYZ(bones[2].local, 0.0f, 0.0f, 0.0f);
                    }
                    Mtx_SetRotXYZ(bones[3].local, 0.0f, 0.0f, 0.0f);
                    if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                        boneScale[1] = 0.4f;
                        boneScale[2] = 2.2f;
                    } else {
                        boneScale[0] = -0.5f;
                        boneScale[2] = 3.0f;
                    }
                } else {
                    if (pl->roundsLoaded != 0) {
                        if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                            boneScale[1] = 0.55f;
                            boneScale[2] = 2.0f;
                        } else {
                            boneScale[0] = 0.8f;
                            boneScale[2] = 3.0f;
                        }
                    }
                }
                break;
            }
        } else {
            if ((pl->roundsLoaded != 0 && pl->motionId[0] != 0x57C) || pl->motionId[0] == 0x579) {
                if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                    boneScale[1] = 0.55f;
                    boneScale[2] = 2.0f;
                } else {
                    boneScale[0] = 0.8f;
                    boneScale[2] = 3.0f;
                }
            }
            if ((pl->motionId[0] != 0x3EB && pl->motionId[0] != 0x3F1 && (pl->weaponOut == 0 || phase == WP_PHASE_SHEATHED))
                || pl->motionId[0] == 0x3EA || pl->motionId[0] == 0x58E || pl->motionId[0] == 0x58F) {
                Mtx_SetRotXYZ(bones[1].local, 0.0f, 0.0f, 0.0f);
                if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
                    Mtx_SetRotXYZ(bones[2].local, 0.0f, 0.0f, 0.0f);
                }
                Mtx_SetRotXYZ(bones[3].local, 0.0f, 0.0f, 0.0f);
                if (pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                    boneScale[1] = 0.4f;
                    boneScale[2] = 2.2f;
                } else {
                    boneScale[0] = -0.5f;
                    boneScale[2] = 3.0f;
                }
            }
        }
        Mtx_Scale(bones[3].local, boneScale[0], boneScale[1], boneScale[2]);
        Mtx_SetTrans(bones[3].local, off[0], off[1], off[2]);
        break;
    }
    Bone_UpdateWorldScaledRoot(wpModel->partBones[0], mtx);
    Bone_StoreMatrices(wpModel->partBones[0]);
    materials = pl->weaponModel->mats;
    secondPhase = Player_MotionPhase(pl, 1, otherPlayer);
    for (i = 0; i < wpModel->matSetCount; i++, set++) {
        if (i != 0 && (pl->weaponType == WEAPON_LANCE || pl->weaponType == WEAPON_SWORD_SHIELD || pl->weaponType == WEAPON_DUAL_SWORDS)) {
            bones = wpModel->partBones[1];
            Mtx_Copy(mtx, (void *)Em_GetBone(pl, 0x11));
            Vec_Set(rot, 0.0f, 0.0f, 0.0f);
            switch (pl->weaponType) {
            case WEAPON_SWORD_SHIELD:
                Mtx_SetRotXYZ(bones[0].local, -0.45378563f, -0.05235988f, 0.13962634f);
                off[0] = -21.0f;
                off[1] = 2.2f;
                off[2] = 5.0f;
                break;
            case WEAPON_LANCE:
                Mtx_SetRotXYZ(bones[0].local, 0.36651915f, -3.0019662f, -0.043633234f);
                off[0] = -18.0f;
                off[1] = 2.0f;
                off[2] = -14.0f;
                break;
            case WEAPON_DUAL_SWORDS:
                if (secondPhase != WP_PHASE_SHEATHED) {
                    Mtx_Copy(mtx, (void *)Em_GetBone(pl, PL_BONE_OTHER_HAND));
                    Mtx_SetRotXYZ(bones[0].local, -0.45378563f, -0.05235988f, 0.13962634f);
                    off[0] = -7.4f;
                    off[1] = -0.4f;
                    off[2] = -6.5f;
                    rot[0] = 0.0f;
                    rot[1] = 0.0f;
                    rot[2] = 3.1415927f;
                } else {
                    Mtx_Copy(mtx, pl->partRecs[9]->m1);
                    off[0] = 17.0f;
                    off[1] = 50.0f;
                    off[2] = -18.0f;
                    rot[0] = 1.6755161f;
                    rot[1] = 0.148353f;
                    rot[2] = -0.48869222f;
                }
                break;
            }
            Vec_Rotate(offWorld, off, mtx);
            Mtx_AddTrans(mtx, offWorld);
            if (pl->weaponType == WEAPON_DUAL_SWORDS) {
                Mtx_Identity(wpModel->partBones[1][0].local);
                Mtx_SetRotXYZ(wpModel->partBones[1][0].local, rot[0], rot[1], rot[2]);
            }
            Bone_UpdateWorldScaledRoot(wpModel->partBones[1], mtx);
            Bone_StoreMatrices(wpModel->partBones[1]);
        }
        if (set->id != -1) {
            for (j = 0; j < set->count; j++) {
                material = &materials[set->mat[j]];
                material->alpha = alpha;
                if (pl->weaponType == WEAPON_HEAVY_BOWGUN || pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                    if (j == 1 && Bowgun_HasBarrel(pl) == 0) {
                        material->alpha = 0.0f;
                    }
                    if (j == 2) {
                        if (pl->weaponType == WEAPON_HEAVY_BOWGUN && HeavyBowgun_HasShield(pl) == 0) {
                            material->alpha = 0.0f;
                        } else if (pl->weaponType == WEAPON_LIGHT_BOWGUN && LightBowgun_HasSilencer(pl) == 0) {
                            material->alpha = 0.0f;
                        }
                    }
                } else {
                    switch (pl->weaponType) {
                    case WEAPON_LONG_SWORD:
                        if (secondPhase != WP_PHASE_SHEATHED) {
                            switch (j) {
                            case 0:
                            case 1:
                            case 2:
                                material->alpha = alpha;
                                break;
                            default:
                                material->alpha = 0.0f;
                                break;
                            }
                        } else {
                            switch (j) {
                            case 0:
                            case 3:
                            case 4:
                                material->alpha = alpha;
                                break;
                            default:
                                material->alpha = 0.0f;
                                break;
                            }
                        }
                        break;
                    case WEAPON_BOW:
                        if (j > 0) {
                            material->alpha = alpha;
                        } else {
                            material->alpha = 0.0f;
                        }
                        break;
                    case WEAPON_GUNLANCE:
                        if (j > 0) {
                            material->alpha = alpha;
                        } else {
                            material->alpha = 0.0f;
                        }
                        break;
                    }
                }
                Gs_SetState((u8)RS_MATERIAL(j), (u32)material);
            }
        }
        Model_Draw(set->id, 0);
        if (pl->weaponType == WEAPON_LONG_SWORD && secondPhase != WP_PHASE_SHEATHED) {
            Mtx_Copy(mtx, pl->partRecs[10]->m1);
            pose = &gWpPoseSheathed[pl->weaponType];
            poseSrc = pose->off;
            off[0] = *poseSrc++;
            off[1] = *poseSrc++;
            off[2] = *poseSrc++;
            rot[0] = *poseSrc++;
            rot[1] = *poseSrc++;
            rot[2] = *poseSrc++;
            Vec_Rotate(offWorld, off, pl->partRecs[10]->m1);
            Mtx_AddTrans(mtx, offWorld);
            Mtx_Identity(bones[0].local);
            Mtx_SetRotXYZ(bones[0].local, rot[0], rot[1], rot[2]);
            Mtx_Scale(scaleMtx, 0.8f, 0.8f, 0.8f);
            Mtx_Concat3x3(bones[0].local, scaleMtx);
            Mtx_GetTrans(pos, bones[2].local);
            Mtx_GetTrans(off, bones[3].local);
            Vec_Set(boneScale2, 1.0f, 1.0f, 1.0f);
            boneScale[2] = 0.73f;
            Mtx_Scale(bones[2].local, 1.0f, 1.0f, boneScale[2]);
            Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
            Mtx_Scale(bones[3].local, boneScale2[0], boneScale2[1], boneScale2[2]);
            Mtx_SetTrans(bones[3].local, off[0], off[1], off[2]);
            Bone_UpdateWorldScaledRoot(wpModel->partBones[0], mtx);
            Bone_StoreMatrices(wpModel->partBones[0]);
            materials = pl->weaponModel->mats;
            if (set->id != -1) {
                for (j = 0; j < set->count; j++) {
                    material = &materials[set->mat[j]];
                    switch (j) {
                    case 3:
                    case 4:
                        material->alpha = alpha;
                        break;
                    default:
                        material->alpha = 0.0f;
                        break;
                    }
                    Gs_SetState((u8)RS_MATERIAL(j), (u32)material);
                }
            }
            Model_Draw(set->id, 0);
        }
        if (pl->weaponType == WEAPON_BOW) {
            Mtx_Copy(mtx, pl->partRecs[2]->m1);
            off[0] = 8.5f;
            off[1] = 0.5f;
            off[2] = -20.0f;
            rot[0] = -0.17453294f;
            rot[1] = 0.0f;
            rot[2] = -0.27925268f;
            Vec_Rotate(offWorld, off, pl->partRecs[2]->m1);
            Mtx_AddTrans(mtx, offWorld);
            Mtx_Identity(bones[0].local);
            Mtx_SetRotXYZ(bones[0].local, rot[0], rot[1], rot[2]);
            Mtx_Scale(scaleMtx, 1.0f, 1.0f, 1.0f);
            Mtx_Concat3x3(bones[0].local, scaleMtx);
            Mtx_GetTrans(pos, bones[2].local);
            Mtx_GetTrans(off, bones[3].local);
            Vec_Set(boneScale2, 1.0f, 1.0f, 1.0f);
            boneScale[2] = 1.0f;
            Mtx_Scale(bones[2].local, 1.0f, 1.0f, boneScale[2]);
            Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
            Mtx_Scale(bones[3].local, boneScale2[0], boneScale2[1], boneScale2[2]);
            Mtx_SetTrans(bones[3].local, off[0], off[1], off[2]);
            Bone_UpdateWorldScaledRoot(wpModel->partBones[0], mtx);
            Bone_StoreMatrices(wpModel->partBones[0]);
            materials = pl->weaponModel->mats;
            if (set->id != -1) {
                for (j = 0; j < set->count; j++) {
                    material = &materials[set->mat[j]];
                    switch (j) {
                    case 0:
                        material->alpha = alpha;
                        break;
                    default:
                        material->alpha = 0.0f;
                        break;
                    }
                    Gs_SetState((u8)RS_MATERIAL(j), (u32)material);
                }
            }
            Model_Draw(set->id, 0);
        }
        if (pl->weaponType == WEAPON_GUNLANCE) {
            Mtx_Copy(mtx, pl->partRecs[17]->m1);
            off[0] = -18.0f;
            off[1] = 2.0f;
            off[2] = -14.0f;
            rot[0] = 0.36651915f;
            rot[1] = 3.0019662f;
            rot[2] = -0.043633234f;
            Vec_Rotate(offWorld, off, pl->partRecs[17]->m1);
            Mtx_AddTrans(mtx, offWorld);
            Mtx_Identity(bones[0].local);
            Mtx_SetRotXYZ(bones[0].local, rot[0], rot[1], rot[2]);
            Mtx_Scale(scaleMtx, 1.0f, 1.0f, 1.0f);
            Mtx_Concat3x3(bones[0].local, scaleMtx);
            Mtx_GetTrans(pos, bones[2].local);
            Mtx_GetTrans(off, bones[3].local);
            Vec_Set(boneScale2, 1.0f, 1.0f, 1.0f);
            boneScale[2] = 1.0f;
            Mtx_Scale(bones[2].local, 1.0f, 1.0f, boneScale[2]);
            Mtx_SetTrans(bones[2].local, pos[0], pos[1], pos[2]);
            Mtx_Scale(bones[3].local, boneScale2[0], boneScale2[1], boneScale2[2]);
            Mtx_SetTrans(bones[3].local, off[0], off[1], off[2]);
            Bone_UpdateWorldScaledRoot(wpModel->partBones[0], mtx);
            Bone_StoreMatrices(wpModel->partBones[0]);
            materials = pl->weaponModel->mats;
            if (set->id != -1) {
                for (j = 0; j < set->count; j++) {
                    material = &materials[set->mat[j]];
                    switch (j) {
                    case 0:
                        material->alpha = alpha;
                        break;
                    default:
                        material->alpha = 0.0f;
                        break;
                    }
                    Gs_SetState((u8)RS_MATERIAL(j), (u32)material);
                }
            }
            Model_Draw(set->id, 0);
        }
    }
}
#pragma optimization_level reset

extern u8 *gPlayerPartFlagTbls[];

/* An entry of the per-gender (6 slots each) table of slot's lists, by pl's armourKind for the slot. */
u8 Player_PartFlags(Player *pl, u8 slot)
{
    return gPlayerPartFlagTbls[pl->gender * 6 + slot][pl->armourKind[slot]];
}

extern u32 gCycleColors[];

/* A colour cycling through the 8 of gCycleColors, 16 frames each (blended by the scene frame counter). */
u32 Color_Cycle8(void)
{
    u8 fromPart;
    u8 channel;
    u16 tick;
    f32 frac;
    f32 invFrac;
    s32 k;
    u32 fromColor;
    u32 toColor;
    u8 toPart;
    u32 color;

    color = 0xFF000000;
    tick = gScene.frame;
    k = (tick >> 4) & 7;
    fromColor = gCycleColors[k];
    invFrac = 1.0f - (frac = (tick & 0xF) / 16.0f);
    k = (k + 1) & 7;
    toColor = gCycleColors[k];
    fromPart = fromColor >> 16;
    channel = toColor >> 16;
    fromPart = fromPart * invFrac;
    toPart = channel * frac;
    channel = fromPart + toPart;
    color |= channel << 16;
    fromPart = fromColor >> 8;
    channel = toColor >> 8;
    fromPart = fromPart * invFrac;
    toPart = channel * frac;
    channel = fromPart + toPart;
    color |= channel << 8;
    fromPart = fromColor;
    channel = toColor;
    fromPart = fromPart * invFrac;
    toPart = channel * frac;
    channel = fromPart + toPart;
    return color | channel;
}

/*
 * Sets material matIndex of material set setIndex of one of the player's parts and hands it to
 * Gs_SetState as RS_MATERIAL(matIndex), with the alpha matAlpha. The part slot is an EquipType
 * (EQUIP_LEGS is the skeleton's own model) or EQUIP_SLOT_FACE.
 * - The head's first material takes the hair colour.
 * - The second material of the first set of a part whose Player_PartFlags has bit 0 takes the
 *   style colour (the cycling Color_Cycle8 for style 0xB).
 * - The face is hidden (alpha 0) by the head armour's flags: bit 2 hides its first material,
 *   bit 4 all of it. With headAltModel set, neither the face nor the head's style colour is
 *   touched.
 */
void Player_SetLight(Player *pl, u8 slot, s32 setIndex, s32 matIndex, ModelMat *mat, f32 matAlpha)
{
    u32 color;
    u8 headFlags;

    if (pl->style == 0xB) {
        color = Color_Cycle8();
    } else {
        color = pl->styleColor;
    }
    mat->alpha = matAlpha;
    switch (slot) {
    case EQUIP_HEAD:
        if (setIndex == 0 && matIndex == 0) {
            mat->rgb[0] = (pl->hairColor >> 16 & 0xFF) / 255.0f;
            mat->rgb[1] = (pl->hairColor >> 8 & 0xFF) / 255.0f;
            mat->rgb[2] = (pl->hairColor & 0xFF) / 255.0f;
        }
        if (pl->headAltModel != 0) {
            break;
        }
    case EQUIP_LEGS:
    case EQUIP_BODY:
    case EQUIP_ARMS:
    case EQUIP_WAIST:
    default:
        if ((Player_PartFlags(pl, slot) & 1) && setIndex == 0 && matIndex == 1) {
            mat->rgb[0] = (color >> 16 & 0xFF) / 255.0f;
            mat->rgb[1] = (color >> 8 & 0xFF) / 255.0f;
            mat->rgb[2] = (color & 0xFF) / 255.0f;
        }
        break;
    case EQUIP_SLOT_FACE:
        if (pl->headAltModel == 0) {
            headFlags = Player_PartFlags(pl, EQUIP_HEAD);
            if (headFlags & 2) {
                if (setIndex == 0 && matIndex == 0) {
                    mat->alpha = 0.0f;
                }
            } else if (headFlags & 4) {
                mat->alpha = 0.0f;
            }
        }
        break;
    }
    Gs_SetState((u8)RS_MATERIAL(matIndex), (u32)mat);
}

s32 Player_IsLocal(void *obj);
s32 CamCtl_IsAiming(Player *pl);
s32 Scene_InField(void);
s32 CamCtl_GetPlayerFade(f32 *out);
f32 Math_Absf(f32 x);
void Player_DrawHeldProp(void *obj);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void LightSet_SetColors(void *obj, s32 n);
void LightSet_FadeColors(void *obj);
void Bone_StoreMatrices(struct EmBone *b);
void Draw_ApplyStates(u32 a);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
void LightSet_ResetColors(s32 a);
void LightSet_Apply(s32 a);
void Gfx_SetTexFilter(s32 a);
void Tex_Prepare(s32 n, s32 tex);
s32 Debug_IsOn(s32 unused0);
void Player_DrawWeapon(Player *pl, s32 otherPlayer, f32 alpha);
void Player_DrawHeldItem(void *obj);
void Bone_UpdateWorldScaledRoot(struct EmBone *b, f32 *m);
void Mtx_Identity(f32 *m);
struct BoneRoot;
void Bone_SetMatricesRel(struct EmBone *b, struct BoneRoot *r, s16 tbl, s32 a3, f32 *parent);
void Bone_SetMatrices(struct EmBone *b, struct BoneRoot *r, s16 tbl, s32 a3);
void Player_SetLight(Player *pl, u8 slot, s32 a, s32 b, ModelMat *light, f32 f);
extern u16 gPlPartRootBone[];

/*
 * Draws the local player's hunter (Player_Draw draws the others), unless it is aiming in the
 * first person (CamCtl_IsAiming):
 * - the skeleton's own model (the legs), at alpha 0.2 while nearCamera, else faded toward the
 *   alpha CamCtl_GetPlayerFade asks for in the field (a tenth of the way each frame, kept in
 *   fadeAlpha);
 * - the weapon (Player_DrawWeapon);
 * - the face and the four other armour pieces (partModels), each placed by its root bone
 *   (gPlPartRootBone) and lit per material (Player_SetLight);
 * - the held prop (Player_DrawHeldProp);
 * - the aiming markers: Hud_DrawObjMarker on the mounted gun with L1 held, Player_DrawBoneMarker
 *   for a loaded bowgun aimed with L1.
 * The body's textures take 2 slots per player from 0xD, the parts' 12 per player from 0x2B.
 */
void Player_DrawLocal(Player *pl, s32 unused /* a1: DrawCb_LocalPlayer passes 0 */)
{
    s32 partSetIndex;
    ModelMatSet *partSet;
    ModelMat *partMats;
    s32 i;
    s32 *partMatWords;
    u16 *rootBone;
    s32 slot;
    s32 j;
    s32 partMatIndex;
    ModelMat *bodyMats;
    s32 setCount;
    f32 alpha;
    f32 world[16];
    s32 variant;
    EmBone *bones;
    Model *partModel;
    Model *body;
    ModelMatSet *bodySet;
    f32 targetAlpha;
    f32 identity[16];

    body = pl->model;
    if (Player_IsLocal(pl) == 1 && CamCtl_IsAiming(pl) != 0) {
        return;
    }
    LightSet_SetColors(pl, 1);
    LightSet_FadeColors(pl);
    Gs_SetState(RS_COLOR2, -1);
    Gfx_SetTexFilter(1);
    if (pl->nearCamera != 0) {
        Gs_SetState(RS_ALPHA_REF, 0);
        alpha = 0.2f;
    } else {
        Gs_SetState(RS_ALPHA_REF, 0xC0);
        alpha = pl->alpha;
    }
    if (Player_IsLocal(pl) == 1 && Scene_InField() == 1) {
        if (CamCtl_GetPlayerFade(&targetAlpha) == 1) {
            alpha = pl->fadeAlpha + (targetAlpha - pl->fadeAlpha) / 10.0f;
            if (Math_Absf(alpha - targetAlpha) <= 0.1f) {
                alpha = targetAlpha;
            }
            Gs_SetState(RS_ALPHA_REF, alpha < 1.0f ? 0 : 0xC0);
            pl->fadeAlpha = alpha;
        } else {
            pl->fadeAlpha = 1.0f;
            Gs_SetState(RS_ALPHA_REF, 0xC0);
            alpha = pl->alpha;
        }
    }
    Tex_Prepare(2, pl->index * 2 + 0xD);
    bodySet = body->matSets;
    Mtx_Copy(world, pl->worldMtx[0]);
    Bone_StoreMatrices(body->bones);
    bodyMats = pl->model->mats;
    for (i = 0; i < body->matSetCount; i++, bodySet++) {
        if (pl->meshShown[i] != 1) {
            continue;
        }
        if (bodySet->id != -1) {
            for (j = 0; j < bodySet->count; j++) {
                Player_SetLight(pl, EQUIP_LEGS, i, j, &bodyMats[bodySet->mat[j]], alpha);
            }
        }
        Draw_ApplyStates(bodySet->flags);
        Model_Draw(bodySet->id, 0);
    }
    Draw_SetDefaultState();
    Gfx_SetTexFilter(1);
    if (pl->weaponShown != 0) {
        Player_DrawWeapon(pl, 0, alpha);
    }
    Tex_Prepare(0xC, pl->index * 12 + 0x2B);
    for (slot = EQUIP_SLOT_FACE, rootBone = &gPlPartRootBone[EQUIP_SLOT_FACE]; slot < EQUIP_SLOT_WEAPON; slot++, rootBone++) {
        variant = pl->armourKind[slot];
        partModel = pl->partModels[slot];
        if (partModel == NULL) {
            continue;
        }
        partSet = partModel->matSets;
        setCount = partModel->matSetCount;
        Bone_UpdateWorldScaledRoot(partModel->bones, pl->partRecs[*rootBone]->m1);
        if (slot == EQUIP_BODY || slot == EQUIP_WAIST) {
            Mtx_Identity(identity);
            Bone_SetMatricesRel(partModel->bones, (struct BoneRoot *) pl, slot, variant - 1, identity);
        } else if (slot == EQUIP_HEAD) {
            bones = partModel->bones;
            if (bones->count > 4) {
                Bone_UpdateWorldScaledRoot(&bones[4], pl->partRecs[20]->m1);
            }
            Mtx_Identity(identity);
            Bone_SetMatricesRel(partModel->bones, (struct BoneRoot *) pl, slot, variant - 1, identity);
        } else {
            Bone_SetMatrices(partModel->bones, (struct BoneRoot *) pl, slot, variant - 1);
        }
        partMats = pl->partModels[slot]->mats;
        for (partSetIndex = 0; partSetIndex < setCount; partSetIndex++, partSet++) {
            if (partSet->id != -1) {
                if (0 < partSet->count) { /* if + do-while: j2 = 0 in the test slot, the walker after it */
                    partMatIndex = 0;
                    partMatWords = (s32 *)partSet; /* the set as words: e2[2] is mat[j2] (a walker from parts->mat is computed first) */
                    do {
                        Player_SetLight(pl, slot, partSetIndex, partMatIndex, &partMats[partMatWords[2]], alpha);
                        partMatIndex++;
                        partMatWords++;
                    } while (partMatIndex < partSet->count);
                }
            }
            Draw_ApplyStates(partSet->flags);
            Model_Draw(partSet->id, 0);
        }
        Draw_SetDefaultState();
    }
    Player_DrawHeldProp(pl);
    if (pl->nearCamera != 0) {
        Gs_SetState(RS_ZWRITE, 1);
    }
    Gs_SetState(RS_ALPHA_REF, 0);
    LightSet_ResetColors(1);
    LightSet_Apply(1);
    if (Player_IsLocal(pl) != 0 && pl->aimMode == 0) {
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_AIM) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_REAIM) != 0) {
            if (pl->input.held & PAD_L1) {
                Hud_DrawObjMarker(pl);
            }
        } else if (pl->weaponOut != 0 && pl->stateGroup != PL_GROUP_REACT && pl->stateGroup != PL_GROUP_GESTURE) {
            switch (pl->weaponType) {
            case WEAPON_HEAVY_BOWGUN:
            case WEAPON_LIGHT_BOWGUN:
                if ((pl->input.held & PAD_L1) && pl->aimLockTimer == 0 && pl->roundsLoaded != 0) {
                    Player_DrawBoneMarker(pl, 0);
                }
                break;
            case WEAPON_BOW:
                break;
            }
        }
    }
}

s16 Player_IsState(void *obj, u8 state, u8 sub);
s32 Motion_InFrameRange(void *obj, s32 layer, f32 from, f32 to);
struct EmBone *Em_GetBone(void *obj, s16 bone);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void Draw_ApplyStates(u32 a);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);

/*
 * Draws the prop the player holds in some motions: a material set of gCommonModels[0] (lit by its
 * own materials) or of gAreaSubModels[0], placed at a bone with an offset and a turn.
 * - Crouched in AREA_TAVERN to AREA_ELDER_HALL or in AREA_HUB_0x89 (not in motions 0x260 and
 *   0x296, nor in the town's action state 0x80): common set 0x28 at bone 0x12.
 * - Motion 0xFA in AREA_HUB_0xAC, frames 152 to 406: area set 5.
 * - Motion 0x19F: the common set of Effect_MapTexId(0x46) at bone 0xE.
 * - Motion 0x287 in AREA_HUB_0x57, frames 232 to 578: area set 0.
 * - Motions 0x324 to 0x329: the common set Player_HeldPartForMotion picks.
 * - Motion 0x349: common set 0x22 at bone 0xE, posed as the drawn heavy bowgun (gWpPoseDrawn),
 *   10 lower.
 * The misses of motions 0xFA and 0x287 jump to the end instead of returning: only that way does
 * the compiler lay the function out as the game's.
 */
void Player_DrawHeldItem(void *obj)
{
    Player *pl = obj;
    f32 boneMtx[16];
    f32 mtx[16];
    f32 pos[3];
    f32 off[3];
    Model *model;
    ModelMatSet *part;
    u8 fromCommon;
    s16 setIndex;
    s16 bone;
    f32 rotX;
    f32 rotY;
    f32 rotZ;
    f32 *pose;

    fromCommon = 0;
    if (pl->action == PL_POSTURE_CROUCHED && pl->motionId[0] != 0x260 && pl->motionId[0] != 0x296
        && Player_IsState(pl, PL_GROUP_ACTION, 0x80) == 0
        && ((u32)(gScene.area - AREA_TAVERN) <= AREA_ELDER_HALL - AREA_TAVERN || gScene.area == AREA_HUB_0x89)) {
        fromCommon = 1;
        setIndex = 0x28;
        bone = PL_BONE_OTHER_HAND;
        rotX = 1.5707964f;
        off[0] = -23.5f;
        off[1] = 0.0f;
        off[2] = -13.5f;
        rotY = 0.0f;
        rotZ = 0.0f;
    } else {
        switch (pl->motionId[0]) {
        case 0xFA:
            if (gScene.area == AREA_HUB_0xAC && Motion_InFrameRange(pl, 0, 152.0f, 406.0f) != 0) {
                setIndex = 5;
                bone = PL_BONE_OTHER_HAND;
                rotX = 0.0f;
                off[0] = -8.5f;
                rotY = 0.0f;
                rotZ = 0.0f;
                off[1] = -0.4f;
                off[2] = 1.2f;
            } else {
                goto end;
            }
            break;
        case 0x19F:
            fromCommon = 1;
            setIndex = Effect_MapTexId(0x46);
            bone = PL_BONE_HAND;
            rotX = 0.36651915f;
            rotY = 0.17453294f;
            rotZ = 1.3613569f;
            off[0] = 17.0f;
            off[1] = 0.9f;
            off[2] = 22.0f;
            break;
        case 0x287:
            if (gScene.area == AREA_HUB_0x57 && Motion_InFrameRange(pl, 0, 232.0f, 578.0f) != 0) {
                setIndex = 0;
                bone = PL_BONE_OTHER_HAND;
                rotX = 0.0f;
                off[0] = 2.08f;
                off[1] = 3.02f;
                rotY = 0.0f;
                off[2] = -2.0f;
                rotZ = 0.0f;
            } else {
                goto end;
            }
            break;
        case 0x324:
        case 0x325:
        case 0x326:
        case 0x327:
        case 0x328:
        case 0x329:
            fromCommon = 1;
            setIndex = Player_HeldPartForMotion(pl);
            bone = PL_BONE_OTHER_HAND;
            rotX = 0.0f;
            rotY = 0.0f;
            rotZ = -1.5707964f;
            off[0] = -9.2f;
            off[1] = -1.7f;
            off[2] = 12.0f;
            break;
        case 0x349:
            bone = PL_BONE_HAND;
            fromCommon = 1;
            setIndex = 0x22;
            pose = (f32 *)&gWpPoseDrawn[WEAPON_HEAVY_BOWGUN];
            off[0] = *pose++;
            off[1] = *pose++;
            off[1] -= 10.0f;
            off[2] = *pose++;
            rotX = *pose++;
            rotY = *pose++;
            rotZ = *pose++;
            break;
        default:
            return;
        }
    }
    if (fromCommon) {
        model = gCommonModels[0];
    } else {
        model = gAreaSubModels[0];
    }
    if (model != NULL && model->active != 0) {
        part = &model->matSets[setIndex];
        Mtx_Copy(boneMtx, (void *)Em_GetBone(pl, bone));
        Mtx_GetTrans(pos, boneMtx);
        Vec_RotateSelf(off, boneMtx);
        pos[0] += off[0];
        pos[1] += off[1];
        pos[2] += off[2];
        Mtx_Identity(mtx);
        Mtx_RotateXYZ(mtx, rotX, rotY, rotZ);
        Mtx_Concat3x3(mtx, boneMtx);
        Mtx_SetTrans(mtx, pos[0], pos[1], pos[2]);
        if (part != NULL && part->id != -1) {
            Gs_SetState(RS_MTX_SLOT(0), (u32)mtx);
            if (fromCommon) {
                Gs_SetLightList(model->mats, part);
            }
            Draw_ApplyStates(part->flags);
            Model_Draw(part->id, 0);
        }
        Draw_SetDefaultState();
    }
end:
    ;
}


/* Draws another player's hunter as Player_DrawLocal draws the local one, without the camera's fade
 * and the markers: the skeleton's own model (alpha 0.2 while nearCamera, unless debug switch 0x7B
 * is on), the weapon (Player_DrawWeapon, not in motion 0x349, where Player_DrawHeldItem draws it),
 * the face and the four other armour pieces, then the held item (Player_DrawHeldItem). */
void Player_Draw(Player *pl, s32 unused /* a1: DrawCb_Player passes 0 */)
{
    s32 partSetIndex;
    ModelMatSet *partSet;
    s32 i;
    Model *body;
    ModelMatSet *bodySet;
    s32 setCount;
    f32 world[16];
    s32 variant;
    f32 identity[16];
    s32 *partMatWords;
    s32 slot;
    s32 j;
    ModelMat *partMats;
    EmBone *bones;
    Model *partModel;
    ModelMat *material;
    ModelMat *bodyMats;
    f32 alpha;
    s32 partMatIndex;
    u16 *rootBone;

    body = pl->model;
    LightSet_SetColors(pl, 1);
    LightSet_FadeColors(pl);
    Gs_SetState(RS_COLOR2, -1);
    Gfx_SetTexFilter(1);
    if (pl->nearCamera != 0 && Debug_IsOn(0x7B) == 0) {
        Gs_SetState(RS_ALPHA_REF, 0);
        alpha = 0.2f;
    } else {
        Gs_SetState(RS_ALPHA_REF, 0xC0);
        alpha = pl->alpha;
    }
    Tex_Prepare(2, pl->index * 2 + 0xD);
    bodySet = body->matSets;
    Mtx_Copy(world, pl->worldMtx[0]);
    Bone_StoreMatrices(body->bones);
    bodyMats = pl->model->mats;
    for (i = 0; i < body->matSetCount; i++, bodySet++) {
        if (pl->meshShown[i] != 1) {
            continue;
        }
        if (bodySet->id != -1) {
            for (j = 0; j < bodySet->count; j++) {
                material = &bodyMats[bodySet->mat[j]];
                material->alpha = alpha;
                Player_SetLight(pl, EQUIP_LEGS, i, j, material, alpha);
            }
        }
        Draw_ApplyStates(bodySet->flags);
        Model_Draw(bodySet->id, 0);
    }
    Draw_SetDefaultState();
    Gfx_SetTexFilter(1);
    if (pl->weaponShown != 0 && pl->motionId[0] != 0x349) {
        Player_DrawWeapon(pl, 1, alpha);
    }
    Tex_Prepare(0xC, pl->index * 12 + 0x2B);
    for (slot = EQUIP_SLOT_FACE, rootBone = &gPlPartRootBone[EQUIP_SLOT_FACE]; slot < EQUIP_SLOT_WEAPON; slot++, rootBone++) {
        variant = pl->armourKind[slot];
        partModel = pl->partModels[slot];
        if (partModel == NULL) {
            continue;
        }
        partSet = partModel->matSets;
        setCount = partModel->matSetCount;
        Bone_UpdateWorldScaledRoot(partModel->bones, pl->partRecs[*rootBone]->m1);
        if (slot == EQUIP_BODY || slot == EQUIP_WAIST) {
            Mtx_Identity(identity);
            Bone_SetMatricesRel(partModel->bones, (struct BoneRoot *) pl, slot, variant - 1, identity);
        } else if (slot == EQUIP_HEAD) {
            bones = partModel->bones;
            if (bones->count > 4) {
                Bone_UpdateWorldScaledRoot(&bones[4], pl->partRecs[20]->m1);
            }
            Mtx_Identity(identity);
            Bone_SetMatricesRel(partModel->bones, (struct BoneRoot *) pl, slot, variant - 1, identity);
        } else {
            Bone_SetMatrices(partModel->bones, (struct BoneRoot *) pl, slot, variant - 1);
        }
        partMats = pl->partModels[slot]->mats;
        for (partSetIndex = 0; partSetIndex < setCount; partSetIndex++, partSet++) {
            if (partSet->id != -1) {
                if (0 < partSet->count) {
                    partMatIndex = 0;
                    partMatWords = (s32 *)partSet; /* the set as words: e[2] is mat[j2] (a walker from parts->mat is computed first) */
                    do {
                        Player_SetLight(pl, slot, partSetIndex, partMatIndex, &partMats[partMatWords[2]], alpha);
                        partMatIndex++;
                        partMatWords++;
                    } while (partMatIndex < partSet->count);
                }
            }
            Draw_ApplyStates(partSet->flags);
            Model_Draw(partSet->id, 0);
        }
        Draw_SetDefaultState();
    }
    Player_DrawHeldItem(pl);
    if (pl->nearCamera != 0) {
        Gs_SetState(RS_ZWRITE, 1);
    }
    Gs_SetState(RS_ALPHA_REF, 0);
    LightSet_ResetColors(1);
    LightSet_Apply(1);
}

extern Model *gAreaSubModel;
extern u8 gFaceBodyTypeMale[];
void LightSet_SetColors(void *obj, s32 n);
void LightSet_FadeColors(void *obj);
void Bone_StoreMatrices(struct EmBone *b);
void Draw_ApplyStates(u32 a);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
void LightSet_ResetColors(s32 a);
void LightSet_Apply(s32 a);
void Gfx_SetTexFilter(s32 a);

/* The character maker's parts model (gAreaSubModel) holds one material set per part: the bodies
 * (four per type, by gFaceBodyTypeMale's variant), then the faces, then the hair styles. */
#define CHARMAKE_SET_FACES 16   /* the first face's set */
#define CHARMAKE_SET_HAIRS 43   /* the first hair style's set */

/* Draws the hunter being made (gChrEdit) on the player's skeleton: the body of its type (the
 * variant gFaceBodyTypeMale gives for its face), its face and its hair, the hair's first material
 * in the chosen colour. */
void CharMake_DrawHunter(Player *pl, s32 unused /* a1: DrawCb_CharMake passes 0 */)
{
    u8 bodySet;
    f32 world[16];
    Model *skeleton;
    ChrEdit *edit;
    u8 typeBase;
    ModelMat *mat;
    s32 i;
    Model *parts;
    ModelMatSet *set;
    ModelMat *mats;

    skeleton = pl->model;
    edit = &gChrEdit;
    LightSet_SetColors(pl, 1);
    LightSet_FadeColors(pl);
    Gs_SetState(RS_COLOR2, -1);
    Gs_SetState(RS_ALPHA_REF, 0xC0);
    Mtx_Copy(world, pl->worldMtx[0]);
    Bone_StoreMatrices(skeleton->bones);
    Gfx_SetTexFilter(1);
    bodySet = 3 - gFaceBodyTypeMale[edit->face];
    typeBase = edit->type * 4;
    bodySet = bodySet + typeBase;
    parts = gAreaSubModel;
    set = parts->matSets + bodySet;
    mats = parts->mats;
    if (set->id != -1) {
        for (i = 0; i < set->count; i++) {
            Gs_SetState((u8)RS_MATERIAL(i), (u32)&mats[set->mat[i]]);
        }
    }
    Draw_ApplyStates(set->flags);
    Model_Draw(set->id, 0);
    set = parts->matSets + edit->face + CHARMAKE_SET_FACES;
    if (set->id != -1) {
        for (i = 0; i < set->count; i++) {
            Gs_SetState((u8)RS_MATERIAL(i), (u32)&mats[set->mat[i]]);
        }
    }
    Draw_ApplyStates(set->flags);
    Model_Draw(set->id, 0);
    set = parts->matSets + edit->hair + CHARMAKE_SET_HAIRS;
    if (set->id != -1) {
        for (i = 0; i < set->count; i++) {
            mat = &mats[set->mat[i]];
            if (i == 0) {
                mat->rgb[0] = ((edit->color >> 16) & 0xFF) / 255.0f;
                mat->rgb[1] = ((edit->color >> 8) & 0xFF) / 255.0f;
                mat->rgb[2] = (u8)edit->color / 255.0f;
                Gs_SetState((u8)RS_MATERIAL(i), (u32)mat);
            }
            Gs_SetState((u8)RS_MATERIAL(i), (u32)mat);
        }
    }
    Draw_ApplyStates(set->flags);
    Model_Draw(set->id, 0);
    Draw_SetDefaultState();
    LightSet_ResetColors(1);
    LightSet_Apply(1);
}

void *memset(void *p, s32 c, u32 n);
extern f32 gLightSetDirs[][3];
extern f32 gLightSetColors[][3];
extern f32 gLightSetAmbients[][3];
extern f32 gCamMatrix[];

/* Sets up the three lights of set k: directions gLightSetDirs turned by the camera gCamMatrix, colours
 * gLightSetColors and gLightSetAmbients. */
void LightSet_SetupSet(s32 setIndex)
{
    Light lights[3];
    f32 lightDir[3];
    f32 viewDir[3];
    s16 i;
    s32 row;
    Light *light;

    for (i = 0, light = lights; i < 3; i++, light++) {
        row = setIndex * 3 + i;
        memset(light, 0, sizeof(Light));
        lightDir[0] = gLightSetDirs[row][0];
        lightDir[1] = gLightSetDirs[row][1];
        lightDir[2] = gLightSetDirs[row][2];
        Vec_Rotate(viewDir, lightDir, gCamMatrix);
        light->color[0] = gLightSetColors[row][0];
        light->color[1] = gLightSetColors[row][1];
        light->color[2] = gLightSetColors[row][2];
        light->ambient[0] = gLightSetAmbients[row][0];
        light->ambient[1] = gLightSetAmbients[row][1];
        light->ambient[2] = gLightSetAmbients[row][2];
        light->unk14[0] = 1.0f;
        light->unk14[1] = 1.0f;
        light->unk14[2] = 1.0f;
        light->dir[0] = viewDir[0];
        light->dir[1] = viewDir[1];
        light->dir[2] = viewDir[2];
    }
    Gs_SetState(RS_LIGHT(0), (u32)&lights[0]);
    Gs_SetState(RS_LIGHT(1), (u32)&lights[1]);
    Gs_SetState(RS_LIGHT(2), (u32)&lights[2]);
}

/* Puts the equipment preview's view back to the start: no pan, no zoom. */
void EquipPreview_Reset(void)
{
    gEquipPreview.zoom = 0.0f;
    gEquipPreview.panX = 0.0f;
    gEquipPreview.panY = 0.0f;
    gEquipPreview.unk20 = 0.0f;
}

void Camera_SetPerspective(s32 i, f32 n, f32 f, f32 fov, f32 aspect);

/* The lobby's menu offsets (lobby .bss), which place the preview window. */
extern s16 gMenuOffsetY;
extern s16 gMenuOffsetX;

/* Sets the view and the projection: the camera's own (mode 0), or the preview's window: the fixed
 * one for EQUIP_PREVIEW_FIXED, else one placed by the lobby's menu offsets with the field of view
 * widened by the preview's zoom. The camera's own near and far values stay. */
void EquipPreview_SetViewport(s16 mode)
{
    GsViewport view;
    EquipPreview *preview = &gEquipPreview;

    if (mode == 0) {
        Gs_SetState(RS_ORTHO, (u32)gCamera->viewport);
        Camera_SetPerspective(0x20, gCamera->farZ, gCamera->nearZ, gCamera->fov, 1.4285715f);
        return;
    }
    view.nearZ = 0;
    view.farZ = 1.0f;
    if (preview->equipType == EQUIP_PREVIEW_FIXED) {
        gCamera->fov = 0.08726647f;
        view.x = 0x108;
        view.y = 0x3A;
        view.width = 0xE8;
        view.height = 0x118;
        Gs_SetState(RS_ORTHO, (u32)&view);
        Camera_SetPerspective(0x20, gCamera->farZ, gCamera->nearZ, gCamera->fov, 1.0f);
    } else {
        gCamera->fov = 6.2831855f * (5.0f * (1.0f + preview->zoom / 100.0f)) / 360.0f;
        view.x = 0.8f * (f32)(gMenuOffsetX + 0x136);
        view.y = gMenuOffsetY + 0x36;
        view.width = 0xF4;
        view.height = 0x96;
        Gs_SetState(RS_ORTHO, (u32)&view);
        Camera_SetPerspective(0x20, gCamera->farZ, gCamera->nearZ, gCamera->fov, 2.0f);
    }
}

/* unit's own declaration: s32 here, where the definition returns s8: with s8 the compiler adds a sign extension after the call that the game's code doesn't have */
s32 Kbd_IsOpen(void);
s32 Gs_ClearColor(u8 mode, u32 color);
void Gfx_SetBlend(s32 a, s32 b);
void Vec_Set(f32 *v, f32 x, f32 y, f32 z);
void Mtx_LookAt(f32 *m, f32 *eye, f32 *at, f32 *up);
struct MtxSlot;
void MtxSlot_Load(struct MtxSlot *m, s32 i);
void Mtx_Inverse(f32 *dst, f32 *src);
void Mtx_SetRotXYZ(f32 *m, f32 x, f32 y, f32 z);
void Effect_RunList(EquipFxSwing *a, struct EmBone *b3, struct EmBone *b1, struct EmBone *b0, u16 key, u8 alt, u8 kind);
void Camera_RestoreFov(void);
extern s32 gPreviewHideTex;
extern u16 gFrameTick;
extern f32 gViewMatrix[];
extern u8 gWeaponKindClass[];
extern EquipPreviewEye gEquipPreviewEyes[];
extern f32 gEquipPreviewWpEyeZ[];
extern f32 gEquipPreviewWpPos[][3];
extern f32 gEquipPreviewSubPos[][3];
extern f32 gEquipPreviewWpRot[][3];
extern f32 gEquipPreviewSubRot[][3];
extern f32 gEquipPreviewWaistBones[][3];
extern f32 gEquipPreviewArmsBones[][3];
extern f32 gEquipPreviewBodyBones[][3];

#pragma optimization_level 4
/*
 * Draws the equipment preview (gEquipPreview) in its window, when a model is loaded:
 * - the pad pans it (the d-pad) and zooms it (L1, L2) unless the keyboard is open or the view is
 *   the fixed one, and L3 resets the view;
 * - the eye looks straight along z from the piece's eye point (gEquipPreviewEyes; a weapon's
 *   distance by its WeaponType), lit by light set 1 in the fixed window, else set 0;
 * - the model turns with the frame counter in the fixed window, an armour piece is turned 30
 *   degrees (the arms also tilted 45); its bones are posed by piece (the waist, the arms and the
 *   body from their tables; the head's swinging parts settle over ten passes the first time) or
 *   by weapon (gEquipPreviewWpRot/WpPos);
 * - each material is drawn opaque, apart from the texture gPreviewHideTex and, per weapon, the
 *   materials of its second part, which the second pass draws (a bowgun's materials 1 and 2 are
 *   not drawn at all); the head takes the player's hair colour, the fixed model the preview's;
 * - a weapon of two parts then draws its second part on partBones[1] (a lance, a sword and
 *   shield, dual swords) or again on partBones[0] (a long sword, a gunlance, a bow), placed by
 *   gEquipPreviewSubRot/SubPos;
 * - the camera, its view and its field of view are put back.
 * subType is unsigned: with a signed one the compiler moves the conversion of the parts loop's
 * test out of the loop into a saved register, where the game's code redoes it on each pass.
 */
void EquipPreview_Draw(void)
{
    EquipPreview *preview;
    u8 weaponType;
    Model *pieceModel;
    EmBone *bones;
    ModelMatSet *set;
    ModelMat *materials;
    ModelMat *material;
    s32 i;
    s32 j;
    Player *pl;
    u32 subType;
    f32 rotX;
    f32 rotY;
    f32 rotZ;
    f32 *trans;

    f32 eye[3];
    f32 target[3];
    f32 upDir[3];
    f32 pos[3];
    f32 mtx[16];
    f32 view[16];

    pl = &gPlayers[gScene.localPlayer];
    preview = &gEquipPreview;
    if (preview->active == 0) {
        return;
    }
    Gs_SetState(RS_COLOR2, -1);
    if (Kbd_IsOpen() == 0 && preview->equipType != EQUIP_PREVIEW_FIXED) {
        if (gPadState[0].dir & PAD_L2) {
            if ((preview->zoom -= 2.0f) <= -30.0f) {
                preview->zoom = -30.0f;
            }
        } else if (gPadState[0].dir & PAD_L1) {
            if ((preview->zoom += 2.0f) >= 30.0f) {
                preview->zoom = 30.0f;
            }
        }
        if (gPadState[0].dir & PAD_RIGHT) {
            if ((preview->panX -= 4.0f) <= -60.0f) {
                preview->panX = -60.0f;
            }
        } else if (gPadState[0].dir & PAD_LEFT) {
            if ((preview->panX += 4.0f) >= 60.0f) {
                preview->panX = 60.0f;
            }
        }
        if (gPadState[0].dir & PAD_UP) {
            if ((preview->panY -= 2.0f) <= -30.0f) {
                preview->panY = -30.0f;
            }
        } else if (gPadState[0].dir & PAD_DOWN) {
            if ((preview->panY += 2.0f) >= 30.0f) {
                preview->panY = 30.0f;
            }
        }
        if (gPadState[0].pressed & PAD_L3) {
            EquipPreview_Reset();
        }
    }
    pieceModel = preview->model;
    EquipPreview_SetViewport(1);
    Gs_SetState(RS_ZWRITE, 1);
    Gs_ClearColor(0, 0);
    Gs_SetState(RS_COLOR2, -1);
    Gfx_SetBlend(4, 5);
    Gs_SetState(RS_ZTEST, RS_CMP_LEQUAL);
    Gs_SetState(RS_ALPHA_TEST, RS_CMP_GREATER);
    Vec_Set(upDir, 0.0f, 1.0f, 0.0f);
    switch (preview->equipType) {
    case EQUIP_PREVIEW_FIXED:
        *(EquipPreviewEye *)eye = gEquipPreviewEyes[1];
        break;
    case EQUIP_BLADE:
    case EQUIP_GUN:
        weaponType = gWeaponKindClass[preview->equipId];
        eye[0] = 0.0f;
        eye[1] = 0.0f;
        eye[2] = gEquipPreviewWpEyeZ[weaponType];
        target[0] = 0.0f;
        target[1] = 0.0f;
        break;
    default:
        *(EquipPreviewEye *)eye = gEquipPreviewEyes[preview->equipType];
        break;
    }
    if (preview->equipType != EQUIP_PREVIEW_FIXED) {
        eye[0] += preview->panX;
        eye[1] += preview->panY;
        eye[2] *= 6.0f;
    }
    target[0] = eye[0];
    target[1] = eye[1];
    target[2] = 0.0f;
    Mtx_LookAt(mtx, eye, target, upDir);
    Gs_SetState(RS_VIEW_MTX, (u32)mtx);
    MtxSlot_Load((struct MtxSlot *)view, 0x21);
    Mtx_Inverse(gCamMatrix, view);
    if (preview->equipType == EQUIP_PREVIEW_FIXED) {
        LightSet_SetupSet(1);
    } else {
        LightSet_SetupSet(0);
    }
    Mtx_Identity(mtx);
    if (preview->equipType != EQUIP_BLADE && preview->equipType != EQUIP_GUN) {
        if (preview->equipType == EQUIP_PREVIEW_FIXED) {
            Mtx_RotateXYZ(mtx, 0.0f, 6.2831855f * (f32)((u8)gFrameTick << 8) / 65536.0f, 0.0f);
        } else if (preview->equipType == EQUIP_ARMS) {
            Mtx_RotateXYZ(mtx, -0.7853982f, -0.5235988f, 0.0f);
        } else {
            Mtx_RotateXYZ(mtx, 0.0f, -0.5235988f, 0.0f);
        }
    } else {
        Mtx_RotateXYZ(mtx, 0.0f, 0.0f, 0.0f);
    }
    bones = pieceModel->partBones[0];
    Mtx_SetTrans(bones[0].local, 0.0f, 0.0f, 0.0f);
    switch (preview->equipType) {
    case EQUIP_PREVIEW_FIXED:
    case EQUIP_LEGS:
        break;
    case EQUIP_WAIST:
        trans = gEquipPreviewWaistBones[0];
        for (i = 1; i < 7; i++) {
            Mtx_SetTrans(bones[i].local, trans[0], trans[1], trans[2]);
            trans += 3;
        }
        break;
    case EQUIP_ARMS:
        Mtx_SetRotXYZ(bones[1].local, 0.0f, 0.0f, 0.0f);
        Mtx_SetRotXYZ(bones[2].local, 0.0f, 0.0f, -0.34906587f);
        Mtx_SetRotXYZ(bones[3].local, 1.5707964f, 0.0f, -1.2217306f);
        Mtx_SetRotXYZ(bones[6].local, 0.0f, 0.0f, 0.34906587f);
        Mtx_SetRotXYZ(bones[7].local, 1.5707964f, 0.0f, 1.2217306f);
        for (i = 1, trans = gEquipPreviewArmsBones[0]; i < 10; i++) {
            Mtx_SetTrans(bones[i].local, trans[0], trans[1], trans[2]);
            trans += 3;
        }
        break;
    case EQUIP_BODY:
        Mtx_SetRotXYZ(bones[4].local, 0.0f, 0.0f, -0.34906587f);
        Mtx_SetRotXYZ(bones[5].local, 0.0f, 0.0f, -1.2217306f);
        Mtx_SetRotXYZ(bones[6].local, 0.0f, 0.0f, 0.34906587f);
        Mtx_SetRotXYZ(bones[7].local, 0.0f, 0.0f, 1.2217306f);
        for (i = 1, trans = gEquipPreviewBodyBones[0]; i < 10; i++) {
            Mtx_SetTrans(bones[i].local, trans[0], trans[1], trans[2]);
            trans += 3;
        }
        break;
    case EQUIP_HEAD:
        Mtx_SetTrans(bones[1].local, 0.0f, 0.0f, 0.0f);
        Mtx_SetTrans(bones[2].local, 0.0f, 22.699997f, -5.499998f);
        Mtx_SetTrans(bones[3].local, 0.0f, 12.300014f, 3.499999f);
        Bone_UpdateWorldScaledRoot(pieceModel->partBones[0], mtx);
        if (preview->settled == 0) {
            preview->settled = 1;
            for (i = 0; i < 10; i++) {
                Effect_RunList(preview->equipFx, &bones[3], &bones[1], bones, preview->equipId, 1, preview->female);
                Bone_UpdateWorldScaledRoot(pieceModel->partBones[0], mtx);
            }
        } else {
            Effect_RunList(preview->equipFx, &bones[3], &bones[1], bones, preview->equipId, 1, preview->female);
            Bone_UpdateWorldScaledRoot(pieceModel->partBones[0], mtx);
        }
        break;
    case EQUIP_BLADE:
    case EQUIP_GUN:
        rotX = gEquipPreviewWpRot[weaponType][0];
        rotY = gEquipPreviewWpRot[weaponType][1];
        rotZ = gEquipPreviewWpRot[weaponType][2];
        eye[0] = gEquipPreviewWpPos[weaponType][0];
        eye[1] = gEquipPreviewWpPos[weaponType][1];
        eye[2] = gEquipPreviewWpPos[weaponType][2];
        Mtx_Identity(mtx);
        Mtx_RotateXYZ(mtx, 6.2831855f * rotX / 360.0f, 6.2831855f * rotY / 360.0f, 6.2831855f * rotZ / 360.0f);
        Mtx_SetTrans(mtx, eye[0], eye[1], eye[2]);
        switch (weaponType) {
        case WEAPON_GREAT_SWORD:
            Mtx_GetTrans(pos, bones[3].local);
            Mtx_Scale(bones[3].local, 2.6f, 1.0f, 2.5f);
            Mtx_SetTrans(bones[3].local, pos[0], pos[1], pos[2]);
            break;
        case WEAPON_HEAVY_BOWGUN:
            Mtx_SetRotXYZ(bones[2].local, 3.1415927f, 0.0f, 0.0f);
            break;
        case WEAPON_BOW:
            Mtx_SetRotXYZ(bones[2].local, 2.7052603f, 0.0f, 0.0f);
            break;
        }
        break;
    }
    set = pieceModel->matSets;
    Bone_UpdateWorldScaledRoot(pieceModel->partBones[0], mtx);
    Bone_StoreMatrices(pieceModel->partBones[0]);
    materials = pieceModel->mats;
    for (i = 0; i < pieceModel->matSetCount; i++, set++) {
        if (preview->equipType == EQUIP_BLADE) {
            switch (weaponType) {
            case WEAPON_SWORD_SHIELD:
            case WEAPON_LANCE:
            case WEAPON_DUAL_SWORDS:
                if (i == 1) {
                    continue;
                }
            }
        }
        if (set == NULL || set->id == -1) {
            continue;
        }
        for (j = 0; j < set->count; j++) {
            material = &materials[set->mat[j]];
            material->alpha = 1.0f;
            switch (preview->equipType) {
            default:
                if (gPreviewHideTex != 0 && gPreviewHideTex == material->tex) {
                    material->alpha = 0.0f;
                }
                break;
            case EQUIP_HEAD:
                if (gPreviewHideTex != 0 && gPreviewHideTex == material->tex) {
                    material->alpha = 0.0f;
                }
                if (i == 0 && j == 0) {
                    material->rgb[0] = (pl->hairColor >> 16 & 0xFF) / 255.0f;
                    material->rgb[1] = (pl->hairColor >> 8 & 0xFF) / 255.0f;
                    material->rgb[2] = (pl->hairColor & 0xFF) / 255.0f;
                }
                break;
            case EQUIP_PREVIEW_FIXED:
                if (i == 0) {
                    switch (j) {
                    case 1:
                    case 3:
                    case 4:
                        material->rgb[0] = (preview->color >> 16 & 0xFF) / 255.0f;
                        material->rgb[1] = (preview->color >> 8 & 0xFF) / 255.0f;
                        material->rgb[2] = (preview->color & 0xFF) / 255.0f;
                        break;
                    }
                }
                break;
            case EQUIP_BLADE:
            case EQUIP_GUN:
                switch (weaponType) {
                case WEAPON_LONG_SWORD:
                    if (j >= 3) {
                        material->alpha = 0.0f;
                    }
                    break;
                case WEAPON_BOW:
                    if (j == 0) {
                        material->alpha = 0.0f;
                    }
                    break;
                case WEAPON_GUNLANCE:
                    if (j == 0) {
                        material->alpha = 0.0f;
                    }
                    break;
                case WEAPON_HEAVY_BOWGUN:
                case WEAPON_LIGHT_BOWGUN:
                    if (j == 1 || j == 2) {
                        material->alpha = 0.0f;
                    }
                    break;
                }
                break;
            }
            Gs_SetState((u8)RS_MATERIAL(j), (u32)material);
        }
        Draw_ApplyStates(set->flags);
        Model_Draw(set->id, 0);
    }
    switch (preview->equipType) {
    case EQUIP_BLADE:
    case EQUIP_GUN:
        subType = weaponType;
        rotX = gEquipPreviewSubRot[subType][0];
        rotY = gEquipPreviewSubRot[subType][1];
        rotZ = gEquipPreviewSubRot[subType][2];
        eye[0] = gEquipPreviewSubPos[subType][0] + gEquipPreviewWpPos[subType][0];
        eye[1] = gEquipPreviewSubPos[subType][1] + gEquipPreviewWpPos[subType][1];
        eye[2] = gEquipPreviewSubPos[subType][2] + gEquipPreviewWpPos[subType][2];
        Mtx_Identity(mtx);
        Mtx_RotateXYZ(mtx, 6.2831855f * rotX / 360.0f, 6.2831855f * rotY / 360.0f, 6.2831855f * rotZ / 360.0f);
        Mtx_SetTrans(mtx, eye[0], eye[1], eye[2]);
        switch (subType) {
        case WEAPON_LANCE:
        case WEAPON_SWORD_SHIELD:
        case WEAPON_DUAL_SWORDS:
            Bone_UpdateWorldScaledRoot(pieceModel->partBones[1], mtx);
            Bone_StoreMatrices(pieceModel->partBones[1]);
            break;
        case WEAPON_LONG_SWORD:
        case WEAPON_GUNLANCE:
        case WEAPON_BOW:
            Bone_UpdateWorldScaledRoot(pieceModel->partBones[0], mtx);
            Bone_StoreMatrices(pieceModel->partBones[0]);
            break;
        default:
            goto done; /* no second part */
        }
        set = pieceModel->matSets;
        materials = pieceModel->mats;
        for (i = 0; i < pieceModel->matSetCount; i++) {
            if ((subType == WEAPON_LANCE || subType == WEAPON_SWORD_SHIELD || subType == WEAPON_DUAL_SWORDS) && i == 0) {
                set++;
                continue;
            }
            if (set != NULL) {
                if (set->id != -1) {
                    for (j = 0; j < set->count; j++) {
                        material = &materials[set->mat[j]];
                        material->alpha = 1.0f;
                        switch (subType) {
                        case WEAPON_LONG_SWORD:
                            if (j < 3) {
                                material->alpha = 0.0f;
                            }
                            break;
                        case WEAPON_BOW:
                            if (j != 0) {
                                material->alpha = 0.0f;
                            }
                            break;
                        case WEAPON_GUNLANCE:
                            if (j != 0) {
                                material->alpha = 0.0f;
                            }
                            break;
                        }
                        Gs_SetState((u8)RS_MATERIAL(j), (u32)material);
                    }
                }
                Draw_ApplyStates(set->flags);
                Model_Draw(set->id, 0);
            }
            set++;
        }
        break;
    }
done:
    EquipPreview_SetViewport(0);
    Mtx_LookAt(mtx, gCamera->eye, gCamera->at, gCamera->up);
    Gs_SetState(RS_VIEW_MTX, (u32)mtx);
    MtxSlot_Load((struct MtxSlot *)gViewMatrix, 0x21);
    Mtx_Inverse(gCamMatrix, gViewMatrix);
    Camera_RestoreFov();
    LightSet_ResetColors(1);
}
#pragma optimization_level reset

void LightSet_SetColors(void *obj, s32 n);
void LightSet_FadeColors(void *obj);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Bone_StoreMatrices(struct EmBone *b);
struct MtxSlot;
void Bone_StoreMatricesTo(struct MtxSlot *tbl, struct EmBone *b);
s32 EmBlangonga_RageTimerFull(Em *em);
s32 GmEm_SetMeshSetUv(Em *em, s32 i);
struct EmMelynxMeshSet;    /* the Melynx's mesh sets, in the game overlay */
struct EmKutkuMeshSet;     /* the Yian Kut-Ku's mesh sets, in the game overlay */
void EmMelynx_SetMeshSetAlpha(Em *em, s32 kind, struct EmMelynxMeshSet *sets);
void EmKutku_SetMeshSetAlpha(Em *em, s32 kind, struct EmKutkuMeshSet *sets);
void GmEm_PrepareMeshSet(Em *em, s32 mode, ModelMatSet *sets);
void Draw_ApplyStates(u32 a);
void MtxSlot_LoadList(s16 *list, struct MtxSlot *tbl);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
void LightSet_ResetColors(s32 a);
void LightSet_Apply(s32 a);
void Tex_Prepare(s32 n, s32 tex);
void Gfx_SetTexFilter(s32 a);
void Mtx_Identity(f32 *m);
extern s16 *gEmNoAlphaTestLists[];
extern u8 gBoneMatrixBuf[];
extern u8 gModelPartLists[][0x420];

/*
 * Draws the monster of a draw slot (Em.drawSlot's draw function): its world matrix from its scale,
 * angles and position (kept in worldMtx), its bones, then each material set it shows
 * (meshShown). The Kelbi and the Anteka draw only the set of their gender. Each set is lit by
 * the monster family's own function (EmMelynx_SetMeshSetAlpha, EmKutku_SetMeshSetAlpha, else
 * GmEm_PrepareMeshSet) and drawn without the alpha test when the monster fades, when its kind's
 * gEmNoAlphaTestLists entry lists it, or for set 5 of a calm EM_KIND_RAJANG
 * (EmBlangonga_RageTimerFull). A model of more than 32 bones keeps its matrices in gBoneMatrixBuf
 * and loads each set's list of them (gModelPartLists) before drawing it.
 */
void Em_DrawParts(DrawSlot *drawSlot)
{
    Em *em;
    ModelMatSet *set;
    f32 world[16];
    Model *emModel;
    s32 i;
    ModelMatSet *sets;
    s32 angles[3];
    s16 *skipList;
    s32 setCount;
    s32 manyBones;
    f32 uvMtx[16];
    EmBone *bones;
    s16 nextSkip;
    f32 scaleMtx[16];

    em = drawSlot->owner;
    emModel = em->model;
    if (em->active == 0 || em->visible == 0) {
        return;
    }
    LightSet_SetColors(em, 1);
    LightSet_FadeColors(em);
    Vec_AnglesToRad(&em->rotX, (f32 *)angles);
    Mtx_Scale(scaleMtx, em->scaleX, em->scaleY, em->scaleZ);
    Mtx_FromAnglesYXZ(&em->rotX, world);
    Mtx_SetTrans(world, em->posX, em->posY, em->posZ);
    Mtx_Concat3x3(world, scaleMtx);
    Mtx_Copy(&em->worldMtx[0][0], world);
    Gfx_SetTexFilter(1);
    nextSkip = *(skipList = gEmNoAlphaTestLists[em->type]);
    if (em->type == EM_KIND_KELBI || em->type == EM_KIND_ANTEKA) {
        setCount = 1;
        sets = emModel->matSets + em->gender;
    } else {
        sets = emModel->matSets;
        setCount = emModel->matSetCount;
    }
    bones = emModel->bones;
    manyBones = bones->count >= 0x21;
    if (manyBones == 0) {
        Bone_StoreMatrices(bones);
    } else {
        Bone_StoreMatricesTo((struct MtxSlot *) gBoneMatrixBuf, bones);
    }
    Mtx_Identity(uvMtx);
    Gs_SetState(RS_UV_MTX, (u32)uvMtx);
    Tex_Prepare(0xA, em->modelSlot * 20 + 0xA3);
    for (i = 0; i < setCount; i++) {
        set = sets + i;
        if ((s8)em->meshShown[i] == 0) {
            continue;
        }
        while (nextSkip != -1) {
            if (!(i > nextSkip)) {
                break;
            }
            nextSkip = *++skipList;
        }
        if (nextSkip == i || em->alpha != 1.0f || (em->type == EM_KIND_RAJANG && i == 5 && EmBlangonga_RageTimerFull(em) == 0)) {
            Gs_SetState(RS_ALPHA_REF, 0);
            nextSkip = *++skipList;
        } else {
            Gs_SetState(RS_ALPHA_REF, 0xC0);
        }
        if (GmEm_SetMeshSetUv(em, i) == 0) {
            continue;
        }
        Gs_SetState(RS_COLOR2, -1);
        if (set->id == -1) {
            continue;
        }
        switch (em->type) {
        case 9:
        case EM_KIND_MELYNX:
        case EM_KIND_FELYNE_CART:
            EmMelynx_SetMeshSetAlpha(em, i, (struct EmMelynxMeshSet *)sets);
            break;
        case EM_KIND_GYPCEROS:
        case EM_KIND_PURPLE_GYPCEROS:
            EmKutku_SetMeshSetAlpha(em, i, (struct EmKutkuMeshSet *)sets);
            break;
        default:
            GmEm_PrepareMeshSet(em, i, sets);
            break;
        }
        Draw_ApplyStates(set->flags);
        if (manyBones == 1) {
            MtxSlot_LoadList((s16 *) (gModelPartLists[em->modelSlot] + i * 0x42), (struct MtxSlot *) gBoneMatrixBuf);
        }
        Model_Draw(set->id, 0);
    }
    Draw_SetDefaultState();
    Gs_SetState(RS_ALPHA_REF, 0);
    LightSet_ResetColors(1);
    LightSet_Apply(1);
}

void Player_TwistBoneToLook(Player *p, s16 kind);
void Mtx_Mul(f32 *dst, f32 *a, f32 *b);
void Bone_UpdateWorldScaledRoot(struct EmBone *b, f32 *m);
void Bone_StoreMatrices(struct EmBone *b);

/* Scales the player's first 21 bones each by its part's scale (partRecs[i]->scale), after
 * Player_TwistBoneToLook turns it, updates them under the matrix parent, and copies each bone's
 * world matrix back into its part's record (m1). */
void Player_ScaleBones(Player *pl, EmBone *bones, f32 *parent)
{
    f32 scaleMtx[16];
    f32 scaled[16];
    s32 i;
    EmBone *bone;

    for (i = 0, bone = bones; i < 21; i++, bone++) {
        Player_TwistBoneToLook(pl, bone->index);
        Mtx_Scale(scaleMtx, pl->partRecs[i]->scale[0], pl->partRecs[i]->scale[1], pl->partRecs[i]->scale[2]);
        Mtx_Mul(scaled, bone->local, scaleMtx);
        bone->local[12] = scaled[12];
        bone->local[13] = scaled[13];
        bone->local[14] = scaled[14];
    }
    Bone_UpdateWorldScaledRoot(bones, parent);
    Bone_StoreMatrices(bones);
    for (i = 0; i < 21; i++, bones++) {
        Mtx_Copy(pl->partRecs[bones->index]->m1, bones->world);
    }
}

/* For a player being drawn (visible): its world matrix from its scale, angles and position, kept
 * in worldMtx[0], then its bones placed under it (Player_ScaleBones). */
void Player_UpdateWorldMatrix(Player *pl)
{
    f32 world[16];
    f32 angles[3];
    Model *model;

    if (pl->visible == 0) {
        return;
    }
    model = pl->model;
    Vec_AnglesToRad(pl->rot, angles);
    Mtx_Scale(world, pl->scale[0], pl->scale[1], pl->scale[2]);
    Mtx_RotateYXZ(world, angles[0], angles[1], angles[2]);
    Mtx_SetTrans(world, pl->pos[0], pl->pos[1], pl->pos[2]);
    Mtx_Copy(pl->worldMtx[0], world);
    Bone_UpdateWorld(model->partBones[0], world);
    Player_ScaleBones(pl, model->partBones[0], world);
}

/* The file's data: the weapon poses and keyframe tracks, the part bones, the light sets, the
 * equipment preview's tables and the monsters' alpha-test lists. */
WpPose gWpPoseDrawnAlt[11] = {
#include "sub_main/0043CA30.inc"
};
WpPose gWpPoseDrawn[11] = {
#include "sub_main/0043CB40.inc"
};
WpPose gWpPoseSheathed[11] = {
#include "sub_main/0043CC50.inc"
};
WpPose gWpPoseSharpen[11] = {
#include "sub_main/0043CD60.inc"
};
Key1 gLbgKey579ScaleZ[8] = {
#include "sub_main/0043CE70.inc"
};
Key1 gHbgKey579ScaleZ[8] = {
#include "sub_main/0043CEB0.inc"
};
Key1 gLbgKey579ScaleY[4] = {
#include "sub_main/0043CEF0.inc"
};
Key1 gHbgKey579ScaleX[4] = {
#include "sub_main/0043CF10.inc"
};
Key1 gLbgKey57CScaleZ[8] = {
#include "sub_main/0043CF30.inc"
};
Key1 gHbgKey57CScaleZ[8] = {
#include "sub_main/0043CF70.inc"
};
Key1 gLbgKey57CScaleY[4] = {
#include "sub_main/0043CFB0.inc"
};
Key1 gHbgKey57CScaleX[4] = {
#include "sub_main/0043CFD0.inc"
};
Key1 gLbgKey57DEmptyScaleZ[8] = {
#include "sub_main/0043CFF0.inc"
};
Key1 gHbgKey57DEmptyScaleZ[8] = {
#include "sub_main/0043D030.inc"
};
Key1 gLbgKey57DEmptyScaleY[4] = {
#include "sub_main/0043D070.inc"
};
Key1 gHbgKey57DEmptyScaleX[4] = {
#include "sub_main/0043D090.inc"
};
Key1 gLbgKey57DScaleZ[8] = {
#include "sub_main/0043D0B0.inc"
};
Key1 gHbgKey57DScaleZ[8] = {
#include "sub_main/0043D0F0.inc"
};
Key1 gLbgKey57DScaleY[6] = {
#include "sub_main/0043D130.inc"
};
Key1 gHbgKey57DScaleX[6] = {
#include "sub_main/0043D160.inc"
};
Key1 gBowgunKey580ScaleZ[8] = {
#include "sub_main/0043D190.inc"
};
Key1 gBowgunKey580Scale2[4] = {
#include "sub_main/0043D1D0.inc"
};
Key1 gHbgKey3EARotX[6] = {
#include "sub_main/0043D1F0.inc"
};
Key1 gHbgKey3EAScaleZ[8] = {
#include "sub_main/0043D220.inc"
};
Key1 gHbgKey3EAScaleX[4] = {
#include "sub_main/0043D260.inc"
};
Key1 gHbgKey3EBRotX[4] = {
#include "sub_main/0043D280.inc"
};
Key1 gHbgKey3F1ScaleZ[6] = {
#include "sub_main/0043D2A0.inc"
};
Key1 gHbgKey3EBScaleX[6] = {
#include "sub_main/0043D2D0.inc"
};
Key3 gLbgKey3EARot[9] = {
#include "sub_main/0043D300.inc"
};
Key3 gLbgKey3EAOff[9] = {
#include "sub_main/0043D390.inc"
};
Key3 gLbgKey3EBRot[12] = {
#include "sub_main/0043D420.inc"
};
Key3 gLbgKey3EBOff[12] = {
#include "sub_main/0043D4E0.inc"
};
Key3 gLbgKey3F1Rot[10] = {
#include "sub_main/0043D5A0.inc"
};
Key3 gLbgKey3F1Off[10] = {
#include "sub_main/0043D640.inc"
};
Key3 gLbgKey3F2Rot[10] = {
#include "sub_main/0043D6E0.inc"
};
Key3 gLbgKey3F2Off[10] = {
#include "sub_main/0043D780.inc"
};
Key1 gLanceKey19DScaleZ[6] = {
#include "sub_main/0043D820.inc"
};
Key1 gLanceKey3EAScaleZ[6] = {
#include "sub_main/0043D850.inc"
};
Key1 gLanceKey3EBScaleZ[6] = {
#include "sub_main/0043D880.inc"
};
Key1 gLanceKey3F1ScaleZ[6] = {
#include "sub_main/0043D8B0.inc"
};
Key1 gLanceKey3F2ScaleZ[6] = {
#include "sub_main/0043D8E0.inc"
};
Key1 gLanceKey3FAScaleZ[6] = {
#include "sub_main/0043D910.inc"
};
Key1 gHammerKey57EScale[6] = {
#include "sub_main/0043D940.inc"
};
Key1 gHammerKey57CScale[6] = {
#include "sub_main/0043D970.inc"
};
Key1 gGsKey19DScaleZ[6] = {
#include "sub_main/0043D9A0.inc"
};
Key1 gGsKey3EAScaleZ[8] = {
#include "sub_main/0043D9D0.inc"
};
Key1 gGsKey3EBScaleZ[8] = {
#include "sub_main/0043DA10.inc"
};
Key1 gGsKey3F9ScaleZ[8] = {
#include "sub_main/0043DA50.inc"
};
Key1 gGsKey582ScaleZ[6] = {
#include "sub_main/0043DA90.inc"
};
Key2 gGsKey19DScaleXZ[10] = {
#include "sub_main/0043DAC0.inc"
};
Key2 gGsKey3EAScaleXZ[6] = {
#include "sub_main/0043DB40.inc"
};
Key2 gGsKey3EBScaleXZ[6] = {
#include "sub_main/0043DB90.inc"
};
Key2 gGsKey3F9ScaleXZ[6] = {
#include "sub_main/0043DBE0.inc"
};
Key2 gGsKey582ScaleXZ[6] = {
#include "sub_main/0043DC30.inc"
};
Key2 gHornKey57DRot[18] = {
#include "sub_main/0043DC80.inc"
};
Key3 gHornKey57DScale[24] = {
#include "sub_main/0043DD60.inc"
};
Key1 gGlKey19DRotX[10] = {
#include "sub_main/0043DEE0.inc"
};
Key1 gGlKey3EARotX[6] = {
#include "sub_main/0043DF30.inc"
};
Key1 gGlKey3EBRotX[8] = {
#include "sub_main/0043DF60.inc"
};
Key1 gGlKey3F2RotX[4] = {
#include "sub_main/0043DFA0.inc"
};
Key1 gGlKey57CRotX[16] = {
#include "sub_main/0043DFC0.inc"
};
Key3 gGlKey580Rot[6] = {
#include "sub_main/0043E040.inc"
};
Key3 gGlKeyTimerRot[7] = {
#include "sub_main/0043E0A0.inc"
};
Key1 gBowKey3EARotX[8] = {
#include "sub_main/0043E110.inc"
};
Key1 gBowKey3EBRotX[12] = {
#include "sub_main/0043E150.inc"
};
Key1 gBowKey3EDRotX[12] = {
#include "sub_main/0043E1B0.inc"
};
Key1 gBowKey3F4RotX[8] = {
#include "sub_main/0043E210.inc"
};
Key1 gBowKey3F6RotX[8] = {
#include "sub_main/0043E250.inc"
};
Key1 gBowKey57DRotX[6] = {
#include "sub_main/0043E290.inc"
};
Key1 gBowKey580RotX[8] = {
#include "sub_main/0043E2C0.inc"
};
Key1 gBowKey587RotX[8] = {
#include "sub_main/0043E300.inc"
};
Key1 gBowKey58ARotX[4] = {
#include "sub_main/0043E340.inc"
};
Key1 gBowKey3EAScaleZ[6] = {
#include "sub_main/0043E360.inc"
};
Key1 gBowKey3EBScaleZ[6] = {
#include "sub_main/0043E390.inc"
};
Key1 gBowKey3EDScaleZ[6] = {
#include "sub_main/0043E3C0.inc"
};
Key1 gBowKey3F4ScaleZ[6] = {
#include "sub_main/0043E3F0.inc"
};
Key1 gBowKey3F6ScaleZ[6] = {
#include "sub_main/0043E420.inc"
};
Key1 gBowKey57DScaleZ[8] = {
#include "sub_main/0043E450.inc"
};
Key1 gBowKey580ScaleZ[6] = {
#include "sub_main/0043E490.inc"
};
Key1 gBowKey587ScaleZ[6] = {
#include "sub_main/0043E4C0.inc"
};
Key1 gBowKey58AScaleZ[5] = {
#include "sub_main/0043E4F0.inc"
};
u32 gBoneMtxSlotMaps_0[2] = {
#include "sub_main/0043E518.inc"
};
u32 gBoneMtxSlotMaps_1[4] = {
#include "sub_main/0043E520.inc"
};
u32 gBoneMtxSlotMaps_2[8] = {
#include "sub_main/0043E530.inc"
};
u32 gBoneMtxSlotMaps_3[8] = {
#include "sub_main/0043E550.inc"
};
u32 gBoneMtxSlotMaps_4[8] = {
#include "sub_main/0043E570.inc"
};
u32 gBoneMtxSlotMaps_5[8] = {
#include "sub_main/0043E590.inc"
};
u32 gBoneMtxSlotMaps[8] = {
    (u32)gBoneMtxSlotMaps_0, (u32)gBoneMtxSlotMaps_1, (u32)gBoneMtxSlotMaps_2, (u32)gBoneMtxSlotMaps_3,
    (u32)gBoneMtxSlotMaps_4, (u32)gBoneMtxSlotMaps_5, 0, 0,
};
f32 gBoneMarkerDirs[6][3] = {
#include "sub_main/0043E5D0.inc"
};
u32 gCycleColors[8] = {
#include "sub_main/0043E620.inc"
};
/* The bone each part slot (EQUIP_SLOT_FACE to EQUIP_WAIST) hangs from (Player_DrawLocal). */
u16 gPlPartRootBone[8] = {
#include "sub_main/0043E640.inc"
};
f32 gLightSetDirs[6][3] = {
#include "sub_main/0043E650.inc"
};
f32 gLightSetColors[6][3] = {
#include "sub_main/0043E6A0.inc"
};
f32 gLightSetAmbients[6][3] = {
#include "sub_main/0043E6F0.inc"
};
EquipPreviewEye gEquipPreviewEyes[6] = {
#include "sub_main/0043E740.inc"
};
f32 gEquipPreviewWpEyeZ[12] = {
#include "sub_main/0043E790.inc"
};
f32 gEquipPreviewWpPos[22][3] = {
#include "sub_main/0043E7C0.inc"
};
f32 gEquipPreviewPoses[154] = {
#include "sub_main/0043E8D0.inc"
};
s16 gEmNoAlphaTestLists_0[4] = {
#include "sub_main/0043EB38.inc"
};
s16 gEmNoAlphaTestLists_14[4] = {
#include "sub_main/0043EB40.inc"
};
s16 gEmNoAlphaTestLists_15[6] = {
#include "sub_main/0043EB48.inc"
};
s16 gEmNoAlphaTestLists_17[4] = {
#include "sub_main/0043EB58.inc"
};
s16 gEmNoAlphaTestLists_20[4] = {
#include "sub_main/0043EB60.inc"
};
s16 gEmNoAlphaTestLists_19[4] = {
#include "sub_main/0043EB68.inc"
};
s16 gEmNoAlphaTestLists_21[4] = {
#include "sub_main/0043EB70.inc"
};
s16 gEmNoAlphaTestLists_22[4] = {
#include "sub_main/0043EB78.inc"
};
s16 gEmNoAlphaTestLists_24[4] = {
#include "sub_main/0043EB80.inc"
};
s16 gEmNoAlphaTestLists_26[4] = {
#include "sub_main/0043EB88.inc"
};
s16 gEmNoAlphaTestLists_33[4] = {
#include "sub_main/0043EB90.inc"
};
s16 gEmNoAlphaTestLists_53[4] = {
#include "sub_main/0043EB98.inc"
};
s16 gEmNoAlphaTestLists_55[4] = {
#include "sub_main/0043EBA0.inc"
};
s16 gEmNoAlphaTestLists_59[4] = {
#include "sub_main/0043EBA8.inc"
};
s16 gEmNoAlphaTestLists_71[8] = {
#include "sub_main/0043EBB0.inc"
};
s16 *gEmNoAlphaTestLists[76] = {
    gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0,
    gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0,
    gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0,
    gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_14, gEmNoAlphaTestLists_15,
    gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_17, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_19,
    gEmNoAlphaTestLists_20, gEmNoAlphaTestLists_21, gEmNoAlphaTestLists_22, gEmNoAlphaTestLists_0,
    gEmNoAlphaTestLists_24, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_26, gEmNoAlphaTestLists_0,
    gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0,
    gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_33, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0,
    gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_20,
    gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_14,
    gEmNoAlphaTestLists_26, gEmNoAlphaTestLists_15, gEmNoAlphaTestLists_21, gEmNoAlphaTestLists_17,
    gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0,
    gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_53, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_55,
    gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_59,
    gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0,
    gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0,
    gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_71,
    gEmNoAlphaTestLists_0, gEmNoAlphaTestLists_0, NULL, NULL,
};
