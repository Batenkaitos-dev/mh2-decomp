/*
 * The menus' equipment preview (sub_main): a window that shows one piece
 * of equipment, posed by its type and lit by its own three lights
 * (LightSet_SetupSet, from the light sets gLightSetDirs, gLightSetColors
 * and gLightSetAmbients). EquipPreview_Reset puts the view back,
 * EquipPreview_SetViewport sets the view and the projection, and
 * EquipPreview_Draw draws the piece (gEquipPreview, equip_preview.h).
 *
 * Last comes Em_DrawParts, the monsters' draw slot function, which draws a
 * monster's model set by set. It shares this file because its tables
 * (gEmNoAlphaTestLists) follow the preview's in the game's data with no
 * 16-byte boundary between them: in a file of its own, its data would
 * start 16-aligned, past where the game has it.
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "camera.h"
#include "objhead.h"
#include "sprite.h"
#include "area.h"
#include "pad.h"
#include "render_state.h"
#include "light.h"
#include "equip_preview.h"
#include "save.h"
#include "text.h"
#include "window.h"
#include "engine_math.h"
#include "gm_em.h"
#include "gs_setup.h"
#include "libc.h"
#include "vec_math.h"
#include "em_blangonga.h"

struct EmBone;
/* end of prototypes */

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
        Camera_SetPerspective(32, gCamera->farZ, gCamera->nearZ, gCamera->fov, 1.4285715f);
        return;
    }
    view.nearZ = 0;
    view.farZ = 1.0f;
    if (preview->equipType == EQUIP_PREVIEW_FIXED) {
        gCamera->fov = 0.08726647f;
        view.x = 264;
        view.y = 58;
        view.width = 232;
        view.height = 280;
        Gs_SetState(RS_ORTHO, (u32)&view);
        Camera_SetPerspective(32, gCamera->farZ, gCamera->nearZ, gCamera->fov, 1.0f);
    } else {
        gCamera->fov = 6.2831855f * (5.0f * (1.0f + preview->zoom / 100.0f)) / 360.0f;
        view.x = 0.8f * (f32)(gMenuOffsetX + 310);
        view.y = gMenuOffsetY + 54;
        view.width = 244;
        view.height = 150;
        Gs_SetState(RS_ORTHO, (u32)&view);
        Camera_SetPerspective(32, gCamera->farZ, gCamera->nearZ, gCamera->fov, 2.0f);
    }
}

/* unit's own declaration: s32 here, where the definition returns s8: with s8 the compiler adds a sign extension after the call that the game's code doesn't have */
s32 Kbd_IsOpen(void);
struct MtxSlot;
void Effect_RunList(EquipFxSwing *swing, struct EmBone *baseBone, struct EmBone *pushBone, struct EmBone *bones, u16 key, u8 alt, u8 kind);
extern s32 gPreviewHideTex;
extern u16 gFrameTick;
extern f32 gViewMatrix[];
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
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
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
    MtxSlot_Load((struct MtxSlot *)view, 33);
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
    MtxSlot_Load((struct MtxSlot *)gViewMatrix, 33);
    Mtx_Inverse(gCamMatrix, gViewMatrix);
    Camera_RestoreFov();
    LightSet_ResetColors(1);
}
#pragma optimization_level reset

struct EmMelynxMeshSet;    /* the Melynx's mesh sets, in the game overlay */
struct EmKutkuMeshSet;     /* the Yian Kut-Ku's mesh sets, in the game overlay */
void EmMelynx_SetMeshSetAlpha(Em *em, s32 kind, struct EmMelynxMeshSet *sets);
void EmKutku_SetMeshSetAlpha(Em *em, s32 kind, struct EmKutkuMeshSet *sets);
extern s16 *gEmNoAlphaTestLists[];
extern u8 gBoneMatrixBuf[];
extern u8 gModelPartLists[][1056];

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
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    nextSkip = *(skipList = gEmNoAlphaTestLists[em->type]);
    if (em->type == EM_KIND_KELBI || em->type == EM_KIND_ANTEKA) {
        setCount = 1;
        sets = emModel->matSets + em->gender;
    } else {
        sets = emModel->matSets;
        setCount = emModel->matSetCount;
    }
    bones = emModel->bones;
    manyBones = bones->count >= 33;
    if (manyBones == 0) {
        Bone_StoreMatrices(bones);
    } else {
        Bone_StoreMatricesTo((struct MtxSlot *) gBoneMatrixBuf, bones);
    }
    Mtx_Identity(uvMtx);
    Gs_SetState(RS_UV_MTX, (u32)uvMtx);
    Tex_Prepare(10, em->modelSlot * 20 + 163);
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
            Gs_SetState(RS_ALPHA_REF, 192);
        }
        if (GmEm_SetMeshSetUv(em, i) == 0) {
            continue;
        }
        Gs_SetState(RS_COLOR2, -1);
        if (set->id == -1) {
            continue;
        }
        switch (em->type) {
        case EM_KIND_FELYNE:
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
            MtxSlot_LoadList((s16 *) (gModelPartLists[em->modelSlot] + i * 66), (struct MtxSlot *) gBoneMatrixBuf);
        }
        Model_Draw(set->id, 0);
    }
    Draw_SetDefaultState();
    Gs_SetState(RS_ALPHA_REF, 0);
    LightSet_ResetColors(1);
    LightSet_Apply(1);
}

/* The file's data: the light sets, the equipment preview's tables and the monsters'
 * alpha-test lists. */
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
