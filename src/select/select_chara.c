/*
 * select: character creation and character selection, the overlay's
 * first file (with its .data and .rodata).
 *
 * gm_sub's main menu starts one of the two tasks:
 * - Chara_CreateTask makes a new character. Its state is sub_main's
 *   gChrEdit (ChrEdit, chredit.h): the gender, then a list of panels
 *   (CharaPanel) whose pages edit the name, type, face, hair, colour and
 *   voice. The face, hair and voice pages filter their parts by the
 *   conditions the player sets (Chara_FilterParts, the condition tables
 *   below). The finished character becomes a save record (Chara_MakeSave)
 *   and sub_main's card menu saves it.
 * - Chara_SelectTask loads the memory card's characters and picks one; as
 *   the main menu's character screen it runs select_title.c's character
 *   selection (CharaSel_*) beside it.
 * select_parts.c draws the creation's panels; select.h holds the steps,
 *   panels and views (CHARA_*).
 */
#include "common.h"
#include "card.h"
#include "window.h"
#include "kbd.h"
#include "mine_point.h"
#include "screen_fade.h"
extern u8 gOldGameKind;
#include "syssettings.h"
#include "camera.h"
#include "scene.h"
#include "pad.h"
#include "m2c_macros.h"
#include "select.h"
#include "player.h"
#include "sprite.h"
#include "model.h"
#include "em.h"
#include "syssave.h"
#include "qsync.h"
#include "render_state.h"
#include "bgm.h"
#include "chat_box.h"
#include "light.h"
#include "movie.h"
#include "sound.h"
#include "texture.h"
#include "area.h"
#include "draw_helpers.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "file_load.h"
#include "frame.h"
#include "game_util.h"
#include "gm_main.h"
#include "mem_util.h"
#include "rand.h"
#include "tex_files.h"
#include "libc.h"
#include "font.h"
#include "quest.h"
#include "area_draw.h"
extern SaveData *gCardSlots; /* sub_main: the save slots read from the memory card */

void ColorEdit_Draw(u8 mode, s16 x, s16 y, u8 r, u8 g, u8 b, u8 sel, u8 pal);
/* The caller passes an s16 slot unextended: the slot is an s16. */
u32 Equip_Color(u8 idx, u8 alpha, s32 alt);      /* a colour from an alpha */
struct Frame;
void Text_SetColor(s16 color);
void Text_SetSize(s32 size);
s32 Text_DisplayLength(char *);            /* a message's width */
void Text_Printf(const char *fmt, ...);
extern char *gNoMatchTexts[];          /* select_psp.c's .data, as the other texts */
extern char *gRotateTexts[];
extern char *gCharaPartNames[];
extern char *gCharaListHelpTexts[];
extern char *gCharaPartHelpTexts[];
extern char *gCharaEditHelpTexts[];
extern char *gCharaSelectTexts[];
/* unit's own declaration: an s32 colour as Chara_DrawEquipment passes it; with the definition's s16, the calls compile differently */
void Text_PrintfAt(s16, s16, s32, const char *, ...);
s32 Str_Length(char *);                /* a message's width */
extern char *gCardSavingTexts[];
void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);

/* .data */
/* Per panel (CharaPanel), the camera view (CharaCamera) of its page. */
s16 gCharaCameraModes[8] = {
#include "select/006C7D80.inc"
};
/* The help window under the screen, and the no-match message's window. */
Frame gCharaHelpFrame = {
#include "select/006C7D90.inc"
};
Frame gCharaNoMatchFrame = {
#include "select/006C7DB0.inc"
};
/* Per hair style, face and voice (by gender, 0 then 1), its values for the
 * page's conditions: 0 fits any, CHARA_COND_NONE none (Chara_FilterParts). */
u8 gCharaHairConds0[18][3] = {
#include "select/006C7DD0.inc"
};
u8 gCharaHairConds1[18][3] = {
#include "select/006C7E10.inc"
};
u8 gCharaFaceConds0[27][3] = {
#include "select/006C7E50.inc"
};
u8 gCharaFaceConds1[27][3] = {
#include "select/006C7EB0.inc"
};
u8 gCharaVoiceConds0[15][2] = {
#include "select/006C7F10.inc"
};
u8 gCharaVoiceConds1[15][2] = {
#include "select/006C7F30.inc"
};
/* Per gender and voice, four speaking motions (Chara_SetVoiceMotion). */
s16 gCharaVoiceMotions[2][15][4] = {
#include "select/006C7F50.inc"
};
/* Per condition (ChrEdit.cond) and gender, the count of its values. */
u8 gCharaStepConds[8][2] = {
#include "select/006C8040.inc"
};
/* The armour icons' texture coordinates and cells (Chara_DrawArmorIcon,
 * Chara_DrawEquipIcon). */
s16 gArmorIconUV[8][2] = {
#include "select/006C8050.inc"
};
s16 gArmorIconUVAlt[8][2] = {
#include "select/006C8070.inc"
};
u8 gArmorIconCells[16] = {
#include "select/006C8090.inc"
};

/*
 * The overlay's entry: resets gChrEdit for a new character. keep == 0 also
 * resets the name, the colour (preset 0), the slot, the panel and the loaded
 * body model; keep != 0 (a new gender) keeps them.
 */
void select_entry(s16 keep)
{
    ChrEdit *edit;
    s16 i;

    edit = &gChrEdit;
    if (keep == 0) {
        edit->unk00 = 0;
        edit->slot = 0;
        edit->panel = CHARA_PANEL_NAME;
        edit->choice = CHARA_CHOICE_YES;
        edit->gender = 0;
        edit->loadedGender = CHARA_GENDER_NONE;
        for (i = 0; i < CHARA_NAME_LEN; i++) {
            edit->name[i] = 0;
        }
        edit->presetColor = 0;
        edit->color = gPresetColors[edit->presetColor];
    }
    edit->type = 0;
    edit->face = 0;
    edit->voice = 0;
    edit->hair = 0;
    edit->timer = 0;
    edit->savedToCard = 0;
    edit->sound = 0;
    edit->soundWait = 0;
    edit->unk44[1] = 0;
    edit->unk44[2] = 0;
    edit->inputDelay = 20;
    edit->cond[0] = 0;
    edit->cond[1] = 0;
    edit->cond[2] = 0;
    edit->cond[3] = 0;
    edit->cond[4] = 0;
    edit->cond[5] = 0;
    edit->cond[6] = 0;
    edit->cond[7] = 0;
}

/*
 * Whether the selection shows the saved characters' equipment lists instead
 * of their models (ChrEdit.equipView, the option copied at its start).
 */
s32 Chara_EquipViewOn(void)
{
    return gChrEdit.equipView != 0;
}

u16 Box_AddItem(SaveData *save, u8 type, u16 id, u16 level);

/*
 * Fills a new save record from the finished character (slot CHARA_SLOT_NONE:
 * gSave itself, otherwise gSysSave.save): its looks and name, the starting
 * weapon and armour, the starting items and money when gOldGameKind is set,
 * its rank, its random seeds, and ten empty equipment sets with numbered
 * names. The sets are walked by moving a SaveData pointer one EquipSet at a
 * time and writing its equipSets[0]: indexing equipSets[j] swaps two saved
 * registers.
 */
void Chara_MakeSave(ChrEdit *edit, u8 slot)
{
    SaveData *newSave;
    s16 i;
    SaveData *walk;
    s16 j;

    if (slot == CHARA_SLOT_NONE) {
        newSave = &gSave;
    } else {
        newSave = &gSysSave.save;
    }
    Mem_Fill(newSave, 0, sizeof(SaveData));
    newSave->used = 1;
    newSave->gender = edit->gender;
    newSave->face = edit->face;
    newSave->voice = edit->voice;
    if (edit->hair == CHARA_HAIR_SPECIAL) {
        newSave->hairStyle = 19;
    } else {
        newSave->hairStyle = edit->hair;
    }
    newSave->hairColor = edit->color;
    for (i = 0; i < CHARA_NAME_LEN; i++) {
        newSave->name[i] = edit->name[i];
    }
    newSave->charType = edit->type;
    newSave->money = 0;
    newSave->equipIdx[1] = 65535;
    newSave->equipIdx[2] = 65535;
    newSave->equipIdx[3] = 65535;
    newSave->equipIdx[4] = 65535;
    newSave->equipIdx[5] = 65535;
    newSave->modeSelChoice = GMSUB_MODE_SEL_OFFLINE;
    newSave->hrPoints = 0;
    newSave->hr = 0;
    Equip_EquipSlot(newSave, (s16)Box_AddItem(newSave, 6, 156, 0));
    Equip_Refresh(newSave);
    newSave->equip[1].type = 0;
    newSave->equip[2].type = 2;
    newSave->equip[3].type = 3;
    newSave->equip[4].type = 4;
    newSave->equip[5].type = 5;
    if (gOldGameKind != 0) {
        ItemBox_AddTo(newSave, 7, 10);
        ItemBox_AddTo(newSave, 172, 20);
        ItemBox_AddTo(newSave, 48, 10);
        ItemBox_AddTo(newSave, 71, 10);
        ItemBox_AddTo(newSave, 142, 5);
        ItemBox_AddTo(newSave, 151, 3);
        ItemBox_AddTo(newSave, 154, 3);
        newSave->money += 3000;
        Save_SetEventFlag(119);
    }
    newSave->hr = Save_RankFromPoints(newSave);
    for (i = 0; i < 32; i++) {
        newSave->randSaved[i] = Rand_Next(1);
    }
    Rate_InitSave(newSave);
    /* equipSets[j] (idx, then name at +0xC) through a byte walk: the indexed form
     * swaps s1 and s2 (the loop counter and the walk). */
    for (j = 0, walk = newSave; j < 10; j++, walk = (SaveData *)((u8 *)walk + sizeof(EquipSet))) {
        walk->equipSets[0].idx[0] = 65535;
        walk->equipSets[0].idx[1] = 65535;
        walk->equipSets[0].idx[2] = 65535;
        walk->equipSets[0].idx[3] = 65535;
        walk->equipSets[0].idx[4] = 65535;
        walk->equipSets[0].idx[5] = 65535;
        sprintf((char *)walk->equipSets[0].name, gEquipSetNameFormat, j + 1);
    }
    Save_SetTitleIn(newSave, 0);
    Save_SetTitleIn(newSave, 1);
    Save_SetTitleIn(newSave, 2);
    Save_SetTitleIn(newSave, 3);
}

/* A record of sub_main's gStartPos: an area's start position (no shared
 * type for it yet). */
typedef struct SelVec3 {
    f32 x, y, z;
} SelVec3;

extern SelVec3 gStartPos[];     /* sub_main */
extern s32 gAreaSubModelId[];
void Player_LoadPartModels(Player *p);

/*
 * Sets up player object slot as the character being created: at the area's
 * start position, the gender's body model (gAreaSubModelId) drawn by
 * DrawCb_CharMake. fillMode 0 also takes the gender and colour from gChrEdit
 * and gives it the default equipment.
 */
void Chara_SetupEditPlayer(Player *pl, s32 mode, s32 slot)
{
    ChrEdit *edit;
    s16 i;
    s16 fillMode;

    fillMode = mode;
    edit = &gChrEdit;
    pl->active = 1;
    pl->visible = 1;
    pl->index = slot;
    pl->isEm = 0;
    pl->weaponShown = 0;
    pl->alpha = 1.0f;
    pl->motionLayerCount = 2;
    pl->scale[0] = 1.0f;
    pl->scale[1] = 1.0f;
    pl->scale[2] = 1.0f;
    pl->motion[0].step = 2.0f;
    pl->motion[1].step = 2.0f;
    pl->motionSpeed = 1.0f;
    pl->weaponOut = 0;
    pl->pos[0] = gStartPos[gScene.area].x;
    pl->pos[1] = gStartPos[gScene.area].y;
    pl->pos[2] = gStartPos[gScene.area].z;
    pl->rot[1] = 0.0f;
    pl->rot[0] = 0.0f;
    pl->rot[2] = 0.0f;
    pl->weaponType = 0;
    pl->isGunner = 0;
    pl->bladeSpecial = 0;
    if (fillMode == 0) {
        pl->gender = edit->gender;
        pl->hairColor = edit->color;
        Player_SetColorWord(pl, 2);
        for (i = 0; i < 6; i++) {
            pl->equip[i].used = 1;
            pl->equip[i].type = i;
            pl->equip[i].id = 1;
            pl->equip[i].level = 0;
            pl->equip[i].deco[0] = 0;
            pl->equip[i].deco[1] = 0;
            pl->equip[i].deco[2] = 0;
        }
        pl->lookVariant = 0;
    }
    Player_InitPartTable(pl->index);
    pl->model = Model_Get(gAreaSubModelId[(s16)slot]);
    Player_InitRecords(pl);
    pl->drawSlotId = DrawSlot_Alloc();
    if (pl->drawSlotId != -1) {
        pl->drawSlot = DrawSlot_Get(pl->drawSlotId);
        pl->drawSlot->owner = (void *) pl->index;
        pl->drawSlot->draw = DrawCb_CharMake;
    }
}

/*
 * Starts the speaking motion of a voice: one of four per gender and voice
 * (gCharaVoiceMotions), picked by the left stick's direction, or at random
 * when it is centred. The flat index is the game's: as [gender][voice][dir]
 * the code differs.
 */
void Chara_SetVoiceMotion(Player *pl, u8 gender, u8 voice)
{
    s16 dir;
    u16 pad;

    pad = gPadState[0].dir;
    if (pad & 0x3C00) {
        if (pad & 0x2000) {
            dir = 0;
        } else if (pad & 0x1000) {
            dir = 1;
        } else if (pad & 0x800) {
            dir = 2;
        } else {
            dir = 3;
        }
    } else {
        dir = Rand_Next(1) & 3;
    }
    Player_SetMotionPair(pl, ((s16 *)gCharaVoiceMotions)[dir + gender * 60 + voice * 4], 4, 0);
}

/*
 * Sets up player object slot as a saved character for the selection: its
 * part and weapon models, drawn as the local player (DrawCb_LocalPlayer).
 * fillMode 0 also takes the gender and colour from gChrEdit and gives it
 * the default equipment (the selection passes 1: Player_SetupFromSave has
 * filled them).
 */
void Chara_SetupCardPlayer(Player *pl, s32 mode, s16 slot)
{
    ChrEdit *edit;
    s16 i;
    s16 fillMode;

    fillMode = mode;
    edit = &gChrEdit;
    pl->active = 1;
    pl->visible = 1;
    pl->index = slot;
    pl->isEm = 0;
    pl->weaponShown = 1;
    pl->alpha = 1.0f;
    pl->motionLayerCount = 2;
    pl->scale[0] = 1.0f;
    pl->scale[1] = 1.0f;
    pl->scale[2] = 1.0f;
    pl->motion[0].step = 2.0f;
    pl->motion[1].step = 2.0f;
    pl->motionSpeed = 1.0f;
    pl->weaponOut = 0;
    pl->pos[0] = gStartPos[gScene.area].x;
    pl->pos[1] = gStartPos[gScene.area].y;
    pl->pos[2] = gStartPos[gScene.area].z;
    pl->rot[1] = 0.0f;
    pl->rot[0] = 0.0f;
    pl->rot[2] = 0.0f;
    if (fillMode == 0) {
        pl->gender = edit->gender;
        pl->hairColor = edit->color;
        Player_SetColorWord(pl, 2);
        for (i = 0; i < 6; i++) {
            pl->equip[i].used = 1;
            pl->equip[i].type = i;
            pl->equip[i].id = 1;
            pl->equip[i].level = 0;
            pl->equip[i].deco[0] = 0;
            pl->equip[i].deco[1] = 0;
            pl->equip[i].deco[2] = 0;
        }
        pl->lookVariant = 0;
    }
    Player_InitPartTable(pl->index);
    Player_LoadPartModels(pl);
    Player_LoadWeaponModel(pl->weaponKind, pl->index);
    Player_ClearFxBlock(pl);
    Player_InitRecords(pl);
    pl->drawSlotId = DrawSlot_Alloc();
    if (pl->drawSlotId != -1) {
        pl->drawSlot = DrawSlot_Get(pl->drawSlotId);
        pl->drawSlot->owner = (void *) pl->index;
        pl->drawSlot->draw = DrawCb_LocalPlayer;
    }
}

/* Draws the creation's backdrop, texture 163 over the whole screen. */
void Chara_DrawBackground(void)
{
    Sprite spr;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 163);
    Tex_Bind(163);
    Gs_SetState(RS_ZWRITE, 0);
    spr.x = 0;
    spr.y = 0;
    spr.w = 640;
    spr.h = 448;
    spr.color = -1;
    spr.u0 = 0;
    spr.v0 = 0;
    spr.u1 = 511;
    spr.v1 = 447;
    Sprite_Draw(&spr);
    Gs_SetState(RS_ZWRITE, 1);
}

/*
 * Draws the creation's header and the rotate hint at the top right (the
 * parameters are not used).
 */
void Chara_DrawPartsHeader(TaskSlot *task, ChrEdit *edit)
{
    Gs_SetState(RS_ZWRITE, 0);
    Sel_DrawHeader(4);
    Font_SetSpacing(20, 20);
    Hud_DrawButtonIconWhite(18, 518, 96, 8, 1.0f);
    Text_PrintfAt(544, 96, 0, gRotateTexts[0]);
    Gs_SetState(RS_ZWRITE, 1);
}

/* Plays the decide sound. */
void Chara_PlayDecideSe(void)
{
    Sound_PlaySeFlat(0, 19, 0);
}

/* Plays the cancel sound. */
void Chara_PlayCancelSe(void)
{
    Sound_PlaySeFlat(0, 20, 0);
}

const char gCharaStrFormat[3] = {
#include "select/006CDD80.inc"
};

/*
 * Draws the help window: the name of a part (kind 0: the current panel's;
 * gCharaPartNames) and under it the list's button help (showPartHelp 0) or
 * the part's own help line. The default is a ternary: an `if` gives an
 * aligned join and another schedule after it.
 */
void Chara_DrawPartHelp(ChrEdit *edit, u8 kind, u8 showPartHelp)
{
    char *helpLine;

    WindowFrame_Draw(&gCharaHelpFrame, 0);
    Font_SetSpacing(20, 20);
    Text_SetColor(0);
    kind = (kind == 0) ? edit->panel : kind;
    Font_SetPos((640 - Text_DisplayLength(gCharaPartNames[kind]) * 10) / 2, 360);
    Text_Printf(gCharaStrFormat, gCharaPartNames[kind]);
    if (showPartHelp == 0) {
        helpLine = gCharaListHelpTexts[0];
    } else {
        helpLine = gCharaPartHelpTexts[kind];
    }
    Font_SetPos((640 - Text_DisplayLength(helpLine) * 10) / 2, 384);
    Text_Printf(gCharaStrFormat, helpLine);
}

/*
 * Draws the message that no part fits the chosen conditions, in its own
 * window at double size.
 */
void Chara_DrawNoMatch(ChrEdit *edit)
{
    char *msg;
    s16 x;

    WindowFrame_Draw(&gCharaNoMatchFrame, 0);
    Font_SetSpacing(20, 20);
    Text_SetColor(0);
    Text_SetSize(2);
    msg = gNoMatchTexts[0];
    x = (640 - Text_DisplayLength(msg) * 10) / 2;
    Font_SetPos(x, 162);
    Text_Printf(gCharaStrFormat, msg);
    msg = gNoMatchTexts[1];
    Font_SetPos(x, 186);
    Text_Printf(gCharaStrFormat, msg);
    Text_SetSize(0);
}

/*
 * Whether the name holds a character other than a space before its end
 * (the first 16 characters).
 */
s32 Chara_NameHasText(ChrEdit *edit)
{
    s16 i;
    u8 found;

    found = 0;
    for (i = 0; i < 16; i++) {
        if (edit->name[i] == 0) {
            break;
        }
        if (edit->name[i] != 0 && edit->name[i] != 32) {
            found = 1;
        }
    }
    if (found == 1) {
        return 1;
    }
    return 0;
}

s32 Str_Length(char *);
extern char *gNameInvalidTexts[];
extern char *gCharaConfirmTexts[];
extern char *gCharaQuestionTexts[];
extern char *gCharaYesNoTexts[];
extern char *gCharaYesNoTexts2[];
extern char *gGenderBackTexts[];

/*
 * Draws a yes/no question (CharaQuestion) with its two answers, the one
 * under the cursor highlighted. Every question but CHARA_ASK_GENDER checks
 * the name first: when it is empty or refused (Chara_CheckName),
 * CHARA_ASK_CREATE draws that error instead.
 */
void Chara_DrawQuestion(ChrEdit *edit, s16 mode)
{
    u8 err;
    char *msg;
    s16 y;
    s16 x;

    Font_SetSpacing(20, 20);
    WindowFrame_Draw(&gCharaHelpFrame, 0);
    err = 0;
    if (mode != CHARA_ASK_GENDER) {
        if (Chara_NameHasText(edit) == 0) {
            err = 1;
        } else if (Chara_CheckName(edit->name) == 0) {
            err = 2;
        }
    }
    if (err != 0 && mode == CHARA_ASK_CREATE) {
        if (err == 1) {
            x = (640 - Str_Length(gRotateTexts[1]) * 10) / 2;
            Text_PrintfAt(x, 360, 0, gRotateTexts[1]);
        } else {
            x = (640 - Str_Length(gNameInvalidTexts[0]) * 10) / 2;
            Text_PrintfAt(x, 360, 0, gNameInvalidTexts[0]);
        }
        Font_SetPos((640 - Text_DisplayLength(gNameInvalidTexts[1]) * 10) / 2, 384);
        Text_Printf(gNameInvalidTexts[1]);
        return;
    }
    switch (mode) {
    case CHARA_ASK_CREATE:
    default:
        msg = gCharaConfirmTexts[0];
        break;
    case CHARA_ASK_SAVE:
        msg = gCharaConfirmTexts[1];
        break;
    case CHARA_ASK_SELECT:
        msg = gCharaQuestionTexts[0];
        break;
    case CHARA_ASK_QUIT:
        msg = gCharaQuestionTexts[1];
        break;
    case CHARA_ASK_GENDER:
        msg = gGenderBackTexts[0];
        break;
    case CHARA_ASK_NET_MESSAGE:
        NetMenu_DrawMessage(320, 352, 48);
        break;
    }
    if (mode != CHARA_ASK_NET_MESSAGE) {
        Text_PrintfAt((640 - Str_Length(msg) * 10) / 2, 360, 0, msg);
        y = 388;
    } else {
        y = 384;
    }
    if (edit->choice == CHARA_CHOICE_YES) {
        Menu_DrawGlowBar(270, y - 2, 100, 24, 0xFF20C0C0);
        Text_PrintfAt(250, y, 5, gCharaYesNoTexts[0]);
        Text_PrintfAt(350, y, 0, gCharaYesNoTexts[1]);
    } else {
        Menu_DrawGlowBar(380, y - 2, 100, 24, 0xFF20C0C0);
        Text_PrintfAt(250, y, 0, gCharaYesNoTexts2[0]);
        Text_PrintfAt(350, y, 5, gCharaYesNoTexts2[1]);
    }
}

/*
 * Draws the help window of the selection screens: mode 0 its two help
 * lines, otherwise the character-select prompt and its button help.
 */
void Chara_DrawEditHelp(ChrEdit *edit, s32 mode)
{
    WindowFrame_Draw(&gCharaHelpFrame, 0);
    Font_SetSpacing(20, 20);
    Text_SetColor(0);
    if ((u16)mode == 0) {
        Text_PrintfAt(160, 360, 0, gCharaEditHelpTexts[0]);
        Text_PrintfAt(160, 384, 0, gCharaEditHelpTexts[1]);
    } else {
        Font_SetPos((640 - Text_DisplayLength(gCharaSelectTexts[0]) * 10) / 2, 360);
        Text_Printf(gCharaSelectTexts[0]);
        Font_SetPos((640 - Text_DisplayLength(gCharaSelectTexts[1]) * 10) / 2, 384);
        Text_Printf(gCharaSelectTexts[1]);
    }
}

/*
 * Draws the window of the creation's save: showResult 0, the lines shown
 * while saving; otherwise the message for the save menu's result
 * (ChrEdit.savedToCard: 1 saved, 0 not). Those two are gCardSavingTexts' last
 * entries, which the game reads as gRotateTexts[18] and [19].
 */
void Chara_DrawSaveMessage(ChrEdit *edit, s32 showResult)
{
    u8 msgIndex;
    s16 x;

    WindowFrame_Draw(&gCharaHelpFrame, 0);
    Font_SetSpacing(20, 20);
    if ((s16)showResult == 0) {
        x = (640 - Str_Length(gCardSavingTexts[0]) * 10) / 2;
        Text_PrintfAt(x, 360, 0, gCardSavingTexts[0]);
        x = (640 - Str_Length(gCardSavingTexts[1]) * 10) / 2;
        Text_PrintfAt(x, 384, 2, gCardSavingTexts[1]);
    } else {
        switch (edit->savedToCard) {
        case 0:
            msgIndex = 19;
            break;
        case 1:
            msgIndex = 18;
            break;
        }
        x = (640 - Str_Length(gRotateTexts[msgIndex]) * 10) / 2;
        Text_PrintfAt(x, 360, 0, gRotateTexts[msgIndex]);
    }
}

/*
 * Draws the colour editor at the left with gChrEdit's colour, its row and
 * its preset.
 */
void Chara_DrawColorEdit(ChrEdit *edit)
{
    ColorEdit_Draw(0, 0, 120, (edit->color >> 16) & 0xFF, (edit->color >> 8) & 0xFF, edit->color & 0xFF, edit->choice, edit->presetColor);
}

/*
 * Sets the camera's target and eye for a view of the player (CharaCamera)
 * in gChrEdit; with hold == 0 the camera jumps there at once, otherwise the
 * tasks ease it there.
 */
void Chara_SetCamera(Player *pl, s32 mode, s32 hold)
{
    ChrEdit *edit;

    edit = &gChrEdit;
    switch ((s16)mode) {
    case CHARA_CAM_BODY:
        Vec_Copy(edit->camTarget, pl->pos);
        edit->camTarget[0] -= 100.0f;
        edit->camTarget[1] += 90.0f;
        Vec_Copy(edit->camPos, edit->camTarget);
        edit->camPos[2] += 300.0f;
        break;
    case CHARA_CAM_FACE:
        Em_GetBonePos(pl, 20, edit->camTarget);
        edit->camTarget[0] -= 20.0f;
        Vec_Copy(edit->camPos, edit->camTarget);
        edit->camPos[1] += 15.0f;
        edit->camPos[2] += 80.0f;
        break;
    case CHARA_CAM_FRONT:
        Vec_Copy(edit->camTarget, pl->pos);
        edit->camTarget[1] += 100.0f;
        Vec_Copy(edit->camPos, edit->camTarget);
        edit->camPos[2] += 260.0f;
        break;
    }
    if ((s16)hold == 0) {
        Vec_Copy(gCamera->at, edit->camTarget);
        Vec_Copy(gCamera->eye, edit->camPos);
    }
}

/*
 * Moves *cursor through count values with the pad's left and right (horiz
 * 0) or up and down (horiz 1), wrapping at the ends, and plays sound on a
 * move.
 */
void Chara_MoveCursorWrap(ChrEdit *edit, u16 pad, u8 *cursor, u16 count, u16 sound, s32 horiz)
{
    u16 prev;
    u16 next;

    if (horiz == 0) {
        prev = 2048;
        next = 1024;
    } else {
        prev = 8192;
        next = 4096;
    }
    if (pad & prev) {
        if (*cursor == 0) {
            *cursor = count - 1;
        } else {
            *cursor -= 1;
        }
        Sound_PlaySeFlat(0, sound, 0);
    }
    if (pad & next) {
        if (*cursor >= count - 1) {
            *cursor = 0;
        } else {
            *cursor += 1;
        }
        Sound_PlaySeFlat(0, sound, 0);
    }
}

/*
 * Steps *value through count values with the pad's left and right,
 * stopping at the ends, and plays sound on a step.
 */
void Chara_MoveCursorClamp(ChrEdit *edit, u16 pad, u8 *value, u16 count, u16 sound)
{
    if (pad & 0x800) {
        if (*value > 0) {
            *value -= 1;
            Sound_PlaySeFlat(0, sound, 0);
        } else {
            *value = 0;
        }
    }
    if (pad & 0x400) {
        if (*value < count - 1) {
            *value += 1;
            Sound_PlaySeFlat(0, sound, 0);
        } else {
            *value = count - 1;
        }
    }
}

/*
 * Moves the page's row cursor (ChrEdit.cursor) through rowCount rows with
 * up and down, wrapping.
 */
void Chara_MoveRowCursor(ChrEdit *edit, u8 rowCount, u16 pad)
{
    if (pad & 0x1000) {
        if (edit->cursor >= rowCount - 1) {
            edit->cursor = 0;
        } else {
            edit->cursor++;
        }
        Sound_PlaySeFlat(0, 18, 0);
    } else if (pad & 0x2000) {
        if (edit->cursor == 0) {
            edit->cursor = rowCount - 1;
        } else {
            edit->cursor--;
        }
        Sound_PlaySeFlat(0, 18, 0);
    }
}

/*
 * On the cancel button, moves the row cursor to the last row (the one that
 * confirms the page).
 */
void Chara_CancelToLast(ChrEdit *edit, u8 rowCount)
{
    if (edit->padPressed & 0x40) {
        if (edit->cursor != rowCount - 1) {
            edit->cursor = rowCount - 1;
            Chara_PlayCancelSe();
        }
    }
}

/*
 * Steps condition condIndex with left and right through its values
 * (gCharaStepConds, by gender), wrapping, and bounces the marker of list
 * entry bounceEntry (+1 for right). Returns 1 when it changed.
 */
s32 Chara_StepCondition(ChrEdit *edit, u16 pad, u8 condIndex, u8 bounceEntry)
{
    u8 valueCount;

    valueCount = (gCharaStepConds[0] + edit->gender)[condIndex * 2];
    if (pad & 0x400) {
        if (edit->cond[condIndex] >= valueCount - 1) {
            edit->cond[condIndex] = 0;
        } else {
            edit->cond[condIndex]++;
        }
        Parts_StartBounce(bounceEntry + 1);
        return 1;
    }
    if (pad & 0x800) {
        if (edit->cond[condIndex] == 0) {
            edit->cond[condIndex] = valueCount - 1;
        } else {
            edit->cond[condIndex]--;
        }
        Parts_StartBounce(bounceEntry);
        Sound_PlaySeFlat(0, 22, 0);
        return 1;
    }
    return 0;
}

/*
 * Finds the parts of a kind (CharaPartKind) that fit the conditions chosen
 * on its page: a part fits when each condition is CHARA_COND_ANY, equal to
 * the part's value, or the part has none (0). Sets ChrEdit.mask (a bit per
 * part) and matchCount, and returns the count. The last hair style fits
 * only when gSysSave.bonusUnlocked is set. Static: its callers keep their first
 * argument's register across the call.
 */
static s8 Chara_FilterParts(ChrEdit *edit, u8 kind)
{
    s16 i;
    u8 *conds;
    u8 firstValue;
    u8 secondValue;
    u8 thirdValue;
    u8 misses;

    edit->matchCount = 0;
    edit->mask = 0;
    switch (kind) {
    case CHARA_PARTS_FACE:
        if (edit->gender == 0) {
            conds = (u8 *)gCharaFaceConds0;
        } else {
            conds = (u8 *)gCharaFaceConds1;
        }
        for (i = 0; i < CHARA_FACE_COUNT; i++) {
            firstValue = *conds++;
            secondValue = *conds++;
            thirdValue = *conds++;
            misses = 3;
            if (edit->cond[0] == 0 || edit->cond[0] == firstValue || firstValue == 0) {
                misses--;
            }
            if (edit->cond[1] == 0 || edit->cond[1] == secondValue || secondValue == 0) {
                misses--;
            }
            if (edit->cond[2] == 0 || edit->cond[2] == thirdValue || thirdValue == 0) {
                misses--;
            }
            if (misses == 0) {
                edit->mask |= 1 << i;
                edit->matchCount++;
            }
        }
        break;
    case CHARA_PARTS_HAIR:
        if (edit->gender == 0) {
            conds = (u8 *)gCharaHairConds0;
        } else {
            conds = (u8 *)gCharaHairConds1;
        }
        for (i = 0; i < CHARA_HAIR_COUNT; i++) {
            firstValue = *conds++;
            secondValue = *conds++;
            thirdValue = *conds++;
            misses = 3;
            if (edit->cond[3] == 0 || edit->cond[3] == firstValue || firstValue == 0) {
                misses--;
            }
            if (edit->cond[4] == 0 || edit->cond[4] == secondValue || secondValue == 0) {
                misses--;
            }
            if (edit->cond[5] == 0 || edit->cond[5] == thirdValue || thirdValue == 0) {
                misses--;
            }
            if (i == CHARA_HAIR_SPECIAL && gSysSave.bonusUnlocked == 0) {
                misses = 99;
            }
            if (misses == 0) {
                edit->mask |= 1 << i;
                edit->matchCount++;
            }
        }
        break;
    case CHARA_PARTS_VOICE:
        if (edit->gender == 0) {
            conds = (u8 *)gCharaVoiceConds0;
        } else {
            conds = (u8 *)gCharaVoiceConds1;
        }
        for (i = 0; i < CHARA_VOICE_COUNT; i++) {
            firstValue = *conds++;
            secondValue = *conds++;
            misses = 2;
            if (edit->cond[6] == 0 || edit->cond[6] == firstValue || firstValue == 0) {
                misses--;
            }
            if (edit->cond[7] == 0 || edit->cond[7] == secondValue || secondValue == 0) {
                misses--;
            }
            if (misses == 0) {
                edit->mask |= 1 << i;
                edit->matchCount++;
            }
        }
        break;
    }
    return edit->matchCount;
}

/*
 * Shows or hides the no-match message, ignores the pad for 10 frames and
 * plays sound 21.
 */
void Chara_ShowNoMatch(ChrEdit *edit, s8 show)
{
    edit->noMatch = show;
    edit->inputDelay = 10;
    Sound_PlaySeFlat(0, 21, 0);
}

/*
 * When the face no longer fits the conditions, moves it to the first that
 * does.
 */
void Chara_FixFace(ChrEdit *edit)
{
    s16 i;

    if (edit->matchCount != 0 && (edit->mask & (1 << edit->face)) == 0) {
        for (i = 0; i < CHARA_FACE_COUNT; i++) {
            if (!(edit->mask & (1 << i)) == 0) {
                edit->face = i;
                return;
            }
        }
    }
}

/*
 * The face page, a frame: row 0 steps through the faces that fit, rows 1-3
 * the conditions, row 4 confirms; the button 0x200 clears the conditions.
 * Returns 1 when confirmed with a face that fits.
 */
s32 Chara_FacePage(ChrEdit *edit)
{
    u16 pad;

    pad = edit->padPressed | edit->padRepeat;
    if (edit->padPressed & 0x20) {
        if (edit->cursor == 4) {
            if (edit->matchCount > 0) {
                return 1;
            }
            Chara_ShowNoMatch(edit, 1);
            return 0;
        }
        edit->cursor = 4;
    }
    Chara_MoveRowCursor(edit, 5, pad);
    Chara_CancelToLast(edit, 5);
    if (edit->padPressed & 0x200) {
        edit->cond[0] = 0;
        edit->cond[1] = 0;
        edit->cond[2] = 0;
        Chara_FilterParts(edit, CHARA_PARTS_FACE);
        Chara_FixFace(edit);
        Sound_PlayCancel();
    }
    switch (edit->cursor) {
    case 0:
        if (edit->matchCount != 0) {
            if (pad & 0x400) {
                do {
                    if (edit->face >= CHARA_FACE_COUNT - 1) {
                        edit->face = 0;
                    } else {
                        edit->face++;
                    }
                } while ((edit->mask & (1 << edit->face)) == 0);
                Parts_StartBounce(1);
            } else if (pad & 0x800) {
                do {
                    if (edit->face == 0) {
                        edit->face = CHARA_FACE_COUNT - 1;
                    } else {
                        edit->face--;
                    }
                } while ((edit->mask & (1 << edit->face)) == 0);
                Parts_StartBounce(0);
            }
        }
        break;
    case 1:
        if (Chara_StepCondition(edit, pad, 0, 2) == 1) {
            if (Chara_FilterParts(edit, CHARA_PARTS_FACE) == 0) {
                Chara_ShowNoMatch(edit, 1);
            } else {
                Chara_FixFace(edit);
            }
        }
        break;
    case 2:
        if (Chara_StepCondition(edit, pad, 1, 4) == 1) {
            if (Chara_FilterParts(edit, CHARA_PARTS_FACE) == 0) {
                Chara_ShowNoMatch(edit, 1);
            } else {
                Chara_FixFace(edit);
            }
        }
        break;
    case 3:
        if (Chara_StepCondition(edit, pad, 2, 6) == 1) {
            if (Chara_FilterParts(edit, CHARA_PARTS_FACE) == 0) {
                Chara_ShowNoMatch(edit, 1);
            } else {
                Chara_FixFace(edit);
            }
        }
        break;
    case 4:
        break;
    }
    return 0;
}

/*
 * When the hair style no longer fits the conditions, moves it to the first
 * that does.
 */
void Chara_FixHair(ChrEdit *edit)
{
    s16 i;

    if (edit->matchCount != 0 && (edit->mask & (1 << edit->hair)) == 0) {
        for (i = 0; i < CHARA_HAIR_COUNT; i++) {
            if (!(edit->mask & (1 << i)) == 0) {
                edit->hair = i;
                return;
            }
        }
    }
}

/*
 * The hair page, a frame: as Chara_FacePage, for the hair styles. Returns 1
 * when confirmed.
 */
s32 Chara_HairPage(ChrEdit *edit)
{
    u16 pad;

    pad = edit->padPressed | edit->padRepeat;
    if (edit->padPressed & 0x20) {
        if (edit->cursor == 4) {
            if (edit->matchCount > 0) {
                return 1;
            }
            Chara_ShowNoMatch(edit, 1);
            return 0;
        }
        edit->cursor = 4;
    }
    Chara_MoveRowCursor(edit, 5, pad);
    Chara_CancelToLast(edit, 5);
    if (edit->padPressed & 0x200) {
        edit->cond[3] = 0;
        edit->cond[4] = 0;
        edit->cond[5] = 0;
        Chara_FilterParts(edit, CHARA_PARTS_HAIR);
        Chara_FixHair(edit);
        Sound_PlayCancel();
    }
    switch (edit->cursor) {
    case 0:
        if (edit->matchCount != 0) {
            if (pad & 0x400) {
                do {
                    if (edit->hair >= CHARA_HAIR_COUNT - 1) {
                        edit->hair = 0;
                    } else {
                        edit->hair++;
                    }
                } while ((edit->mask & (1 << edit->hair)) == 0);
                Parts_StartBounce(1);
            } else if (pad & 0x800) {
                do {
                    if (edit->hair == 0) {
                        edit->hair = CHARA_HAIR_COUNT - 1;
                    } else {
                        edit->hair--;
                    }
                } while ((edit->mask & (1 << edit->hair)) == 0);
                Parts_StartBounce(0);
            }
        }
        break;
    case 1:
        if (Chara_StepCondition(edit, pad, 3, 2) == 1) {
            if (Chara_FilterParts(edit, CHARA_PARTS_HAIR) == 0) {
                Chara_ShowNoMatch(edit, 1);
            } else {
                Chara_FixHair(edit);
            }
        }
        break;
    case 2:
        if (Chara_StepCondition(edit, pad, 4, 4) == 1) {
            if (Chara_FilterParts(edit, CHARA_PARTS_HAIR) == 0) {
                Chara_ShowNoMatch(edit, 1);
            } else {
                Chara_FixHair(edit);
            }
        }
        break;
    case 3:
        if (Chara_StepCondition(edit, pad, 5, 6) == 1) {
            if (Chara_FilterParts(edit, CHARA_PARTS_HAIR) == 0) {
                Chara_ShowNoMatch(edit, 1);
            } else {
                Chara_FixHair(edit);
            }
        }
        break;
    case 4:
        break;
    }
    return 0;
}

/*
 * When the voice no longer fits the conditions, moves it to the first that
 * does.
 */
void Chara_FixVoice(ChrEdit *edit)
{
    s16 i;

    if (edit->matchCount != 0 && (edit->mask & (1 << edit->voice)) == 0) {
        for (i = 0; i < CHARA_VOICE_COUNT; i++) {
            if (!(edit->mask & (1 << i)) == 0) {
                edit->voice = i;
                return;
            }
        }
    }
}

/*
 * The voice page, a frame: row 0 steps through the voices that fit, rows
 * 1-2 the conditions, row 3 confirms. A new voice loads its sound bank.
 * Returns 1 when confirmed.
 */
s32 Chara_VoicePage(ChrEdit *edit)
{
    u16 pad;
    u8 oldVoice;

    pad = edit->padPressed | edit->padRepeat;
    if (edit->padPressed & 0x20) {
        if (edit->cursor == 3) {
            if (edit->matchCount > 0) {
                return 1;
            }
            Chara_ShowNoMatch(edit, 1);
            return 0;
        }
        edit->cursor = 3;
    }
    Chara_MoveRowCursor(edit, 4, pad);
    Chara_CancelToLast(edit, 4);
    if (edit->padPressed & 0x200) {
        edit->cond[6] = 0;
        edit->cond[7] = 0;
        Chara_FilterParts(edit, CHARA_PARTS_VOICE);
        Chara_FixVoice(edit);
        Sound_PlayCancel();
    }
    oldVoice = edit->voice;
    switch (edit->cursor) {
    case 0:
        if (edit->matchCount != 0) {
            if (pad & 0x400) {
                do {
                    if (edit->voice >= 14) {
                        edit->voice = 0;
                    } else {
                        edit->voice++;
                    }
                } while ((edit->mask & (1 << edit->voice)) == 0);
                Parts_StartBounce(1);
            } else if (pad & 0x800) {
                do {
                    if (edit->voice == 0) {
                        edit->voice = 14;
                    } else {
                        edit->voice--;
                    }
                } while ((edit->mask & (1 << edit->voice)) == 0);
                Parts_StartBounce(0);
            }
        }
        break;
    case 1:
        if (Chara_StepCondition(edit, pad, 6, 2) == 1) {
            if (Chara_FilterParts(edit, CHARA_PARTS_VOICE) == 0) {
                Chara_ShowNoMatch(edit, 1);
            } else {
                Chara_FixVoice(edit);
            }
        }
        break;
    case 2:
        if (Chara_StepCondition(edit, pad, 7, 4) == 1) {
            if (Chara_FilterParts(edit, CHARA_PARTS_VOICE) == 0) {
                Chara_ShowNoMatch(edit, 1);
            } else {
                Chara_FixVoice(edit);
            }
        }
        break;
    case 3:
        break;
    }
    if (oldVoice != edit->voice) {
        edit->sound = 0;
        Sound_LoadVoiceBank(edit->gender, edit->voice);
    }
    return 0;
}

/*
 * Sets up player object pl from saved character slot (gCardSlots) when the
 * slot is used, with its gender's idle motion. The first parameter is not
 * used. The record's address is loaded, then the slot's offset added: one
 * expression `gCardSlots + slot` computes the offset first.
 */
void Chara_LoadSlot(s32 unused, Player *pl, s16 slot)
{
    SaveData *save;

    save = gCardSlots;
    save += slot;
    if (save->used != 0) {
        Player_SetupFromSave(pl, save);
        if (pl->gender == 0) {
            Player_SetMotionPair(pl, 1, 0, 0);
        } else {
            Player_SetMotionPair(pl, 811, 0, 0);
        }
    }
}

/*
 * Turns the character with the right stick's left and right (0x400 a
 * frame); the other two player objects follow.
 */
void Chara_RotateByPad(void)
{
    u16 pad;
    s32 rot;

    pad = gPadState[0].dir;
    if (pad & 8) {
        gPlayers[0].rot[1] -= 0x400;
    }
    if (pad & 4) {
        gPlayers[0].rot[1] += 0x400;
    }
    rot = gPlayers[0].rot[1];
    gPlayers[1].rot[1] = rot;
    gPlayers[2].rot[1] = rot;
}


/*
 * The colour page, a frame: the red, green and blue rows step their value
 * (held buttons repeat after 10 frames), the presets row picks one of 16
 * colours, and the last row confirms. Copies the colour to the player's
 * hair. Returns 1 when confirmed.
 */
s32 Chara_EditColor(ChrEdit *edit, Player *pl, u16 pad)
{
    u8 rgb[3];

    if (edit->choice == CHARA_COLOR_ROW_DONE && (edit->padPressed & 0x20)) {
        return 1;
    }
    if ((edit->padPressed & 0x40) && edit->choice != CHARA_COLOR_ROW_DONE) {
        edit->choice = CHARA_COLOR_ROW_DONE;
        Chara_PlayCancelSe();
    }
    rgb[0] = edit->color >> 16;
    rgb[1] = edit->color >> 8;
    rgb[2] = edit->color;
    if (pad & 0x2000) {
        if (edit->choice == CHARA_COLOR_ROW_RED) {
            edit->choice = CHARA_COLOR_ROW_DONE;
        } else {
            edit->choice--;
        }
        Sound_PlaySeFlat(0, 18, 0);
    }
    if (pad & 0x1000) {
        if (edit->choice >= CHARA_COLOR_ROW_DONE) {
            edit->choice = CHARA_COLOR_ROW_RED;
        } else {
            edit->choice++;
        }
        Sound_PlaySeFlat(0, 18, 0);
    }
    if (edit->choice < CHARA_COLOR_ROW_PRESETS) {
        Chara_MoveCursorClamp(edit, edit->padHoldRepeat, &rgb[edit->choice], 256, 23);
        edit->color = (rgb[0] << 16) | 0xFF000000 | (rgb[1] << 8) | rgb[2];
    } else if (edit->choice == CHARA_COLOR_ROW_PRESETS) {
        Chara_MoveCursorWrap(edit, pad, &edit->presetColor, CHARA_PRESET_COLOR_COUNT, 22, 0);
        if (edit->padPressed & 0x20) {
            edit->color = gPresetColors[edit->presetColor];
            Chara_PlayDecideSe();
        }
    }
    pl->hairColor = edit->color;
    return 0;
}

extern char *gCharaPageTexts[];

/*
 * The creation's draw hook (gSelectDrawHook): the header and help of the
 * current step and the open panel.
 */
void Chara_DrawPages(DrawSlot *hook)
{
    TaskSlot *task;
    ChrEdit *edit;

    task = hook->owner;
    edit = &gChrEdit;
    Gs_SetState(RS_ALPHA_REF, 0);
    if (task->mode >= CHARA_CREATE_PANELS) {
        if (task->mode < CHARA_CREATE_SAVE) {
            Chara_DrawPartsHeader(task, edit);
        } else if (task->mode <= CHARA_CREATE_SAVE || task->mode >= CHARA_CREATE_ASK_GENDER) {
            Hud_DrawButtonIconWhite(18, 518, 96, 8, 1.0f);
            Font_SetSpacing(20, 20);
            Text_PrintfAt(544, 96, 0, gCharaPageTexts[0]);
        }
    }
    switch (task->mode) {
    case CHARA_CREATE_GENDER:
        Gs_SetState(RS_ZWRITE, 0);
        Sel_DrawHeader(4);
        Gs_SetState(RS_ZWRITE, 1);
        if (Parts_DrawSides(task) == 1) {
            Chara_DrawPartHelp(edit, 7, 1);
        }
        break;
    case CHARA_CREATE_PANELS:
        Chara_DrawPartHelp(edit, 0, 0);
        break;
    case CHARA_CREATE_EDIT:
        Chara_DrawPartHelp(edit, 0, 1);
        switch (edit->panel) {
        case CHARA_PANEL_NAME:
            Kbd_SetPos(240, 75.0f);
            Chat_DrawKbdBox(0xBD6C6CC5);
            break;
        case CHARA_PANEL_TYPE:
            Parts_DrawTypes(0, 96, edit->type);
            break;
        case CHARA_PANEL_FACE:
            Parts_DrawFacePanel(0, 96, 0);
            break;
        case CHARA_PANEL_COLOR:
            Chara_DrawColorEdit(edit);
            break;
        case CHARA_PANEL_VOICE:
            Parts_DrawVoicePanel(0, 96, 0);
            break;
        case CHARA_PANEL_HAIR:
            Parts_DrawHairPanel(0, 96, 0);
            break;
        }
        break;
    case CHARA_CREATE_CONFIRM:
        Chara_DrawQuestion(edit, CHARA_ASK_CREATE);
        break;
    case CHARA_CREATE_DONE:
    case CHARA_CREATE_FADE_OUT:
    case CHARA_CREATE_END:
        Chara_DrawSaveMessage(edit, 1);
        break;
    case CHARA_CREATE_ASK_QUIT:
    case CHARA_CREATE_QUIT:
        Sel_DrawHeader(4);
        Chara_DrawQuestion(edit, CHARA_ASK_QUIT);
        break;
    case CHARA_CREATE_ASK_GENDER:
        Sel_DrawHeader(4);
        Chara_DrawQuestion(edit, CHARA_ASK_GENDER);
        break;
    case CHARA_CREATE_INIT:
    case CHARA_CREATE_LOAD:
    case CHARA_CREATE_FADE_IN:
    case CHARA_CREATE_SAVE:
        break;
    }
}

/*
 * The creation's text draw hook (gCharaTextDrawHook): the no-match message
 * while it is shown.
 */
void Chara_DrawNoMatchLayer(DrawSlot *hook)
{
    ChrEdit *edit = &gChrEdit;

    if (edit->noMatch != 0) {
        Chara_DrawNoMatch(edit);
    }
}

/*
 * Leaves a panel's page: back to the list, the panels slide in, the camera
 * returns to the whole body.
 */
void Chara_ClosePage(TaskSlot *task, ChrEdit *edit)
{
    task->mode = CHARA_CREATE_PANELS;
    Parts_SlideInPanels(edit->panel);
    Chara_SetCamera(gPlayers, gCharaCameraModes[0], 1);
    edit->inputDelay = 20;
    Parts_StartPanel(1);
}

void Gfx_SetBlend(s32, s32);
void Hud_LoadTextures(void);
s8 Kbd_Run(s8 *, s16, s16);
u8 NetMenu_Update(void);
extern DrawSlot gSelectDrawHook;
extern DrawSlot gCharaTextDrawHook;


/*
 * Character creation, the task (CharaCreateStep): the gender, then the
 * panels (the list picks one, its page edits it; the buttons 0x4000 and
 * 0x8000 together randomise the look), the name check and confirmation, the memory card's
 * save menu, and the character's first words before the game starts. Each
 * frame it copies the pad into gChrEdit (nothing while inputDelay counts or
 * the no-match message is shown), then runs the players, eases the camera
 * an eighth of the way to its target and draws.
 */
void Chara_CreateTask(TaskSlot *task)
{
    ChrEdit *edit;
    Player *player;
    u16 trig;
    s16 i;
    Player *pl;
    u8 saveResult;
    s32 movieStatus;

    edit = &gChrEdit;
    player = gPlayers;
    if (gChrEdit.soundWait != 0) {
        edit->soundWait--;
    }
    if (edit->inputDelay != 0) {
        edit->inputDelay--;
        edit->padPressed = 0;
        edit->padPrevHeld = 0;
        edit->padHeld = 0;
        edit->padRepeat = 0;
    } else {
        edit->padPressed = gPadState[0].pressed;
        edit->padPrevHeld = gPadState[0].prevHeld;
        edit->padHeld = gPadState[0].held;
        edit->padRepeat = gPadState[0].repeat;
    }
    edit->padHoldRepeat = 0;
    if (edit->padPrevHeld == edit->padHeld) {
        edit->holdFrames++;
        if (edit->holdFrames > 10) {
            edit->holdFrames = 10;
            edit->padHoldRepeat = edit->padHeld;
        }
    } else {
        edit->holdFrames = 0;
    }
    edit->padHoldRepeat |= edit->padPressed;
    trig = edit->padPressed | edit->padRepeat;
    if (edit->noMatch != 0) {
        if (edit->padPressed & 0x60) {
            edit->noMatch = 0;
            Chara_PlayDecideSe();
        }
        edit->padPressed = 0;
        edit->padPrevHeld = 0;
        edit->padHeld = 0;
        edit->padRepeat = 0;
    }
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Text_SetSize(0);
    switch (task->mode) {
    case CHARA_CREATE_INIT:
        task->mode++;
        Game_ResetSubsystems();
        Tex_LoadSystemPack(2);
        Sel_LoadCommonTextures();
        Tex_LoadFile(12, 363, 0);
        FadeA_Reset();
        select_entry(0);
        Camera_Reset();
        CamCtl_Init();
        CamCtl_Clear();
        gSelectDrawHook.owner = task;
        gSelectDrawHook.draw = Chara_DrawPages;
        gCharaTextDrawHook.draw = Chara_DrawNoMatchLayer;
        gCharaTextDrawHook.part = (s32)task;
    case CHARA_CREATE_LOAD:
        if (Sound_LoadSelectBanks(task) != 0) {
            task->mode++;
            FadeA_Start(2);
        }
        break;
    case CHARA_CREATE_FADE_IN:
        if (FadeA_GetState() != 1) {
            task->mode++;
            edit->gender = 0;
            edit->voice = 0;
            Parts_Init();
            edit->inputDelay = 30;
        }
        break;
    case CHARA_CREATE_GENDER:
        if ((edit->padPressed & 0x800) && edit->gender == 1) {
            edit->gender = 0;
            edit->inputDelay = 3;
            Sound_PlaySeFlatPan(0, 18, 0, edit->gender * 100 - 50);
        } else if ((edit->padPressed & 0x400) && edit->gender == 0) {
            edit->gender = 1;
            edit->inputDelay = 3;
            Sound_PlaySeFlatPan(0, 18, 0, edit->gender * 100 - 50);
        } else if (edit->padPressed & 0x20) {
            task->mode++;
            if (edit->loadedGender != edit->gender) {
                select_entry(1);
                Sound_LoadVoiceBank(edit->gender, edit->voice);
                gScene.area = 0;
                gScene.questId = 0;
                Quest_Clear();
                Light_InitAll(gScene.area);
                Area_Enter(gScene.area);
                gSysSettings.soundLoading = 1;
                Sound_PlaySeFlat(1, 9, 0);
                Loading_Start();
                if (edit->loadedGender == CHARA_GENDER_NONE) {
                    Hud_LoadTextures();
                    Anim_LoadCommon(4);
                    CharMake_LoadPartsModel(edit->gender);
                    Chara_SetupEditPlayer(player, 0, 0);
                } else {
                    CharMake_FreePartsModel();
                    Mem_Compact();
                    CharMake_LoadPartsModel(edit->gender);
                }
                gSysSettings.soundLoading = 0;
                Loading_Stop();
                edit->loadedGender = edit->gender;
                gPlayers[0].visible = 1;
                gPlayers[0].gender = edit->gender;
                pl = gPlayers;
                if (edit->gender == 0) {
                    Player_SetMotionPair(pl, 1, 0, 0);
                } else {
                    Player_SetMotionPair(pl, 811, 0, 0);
                }
                Chara_SetCamera(gPlayers, CHARA_CAM_BODY, 0);
                gCamera->fov = 0.87266463f;
                gCamera->roll = 0.0f;
            }
            Sound_PlaySeMusicVol(6, 0, 0);
            Sound_PlaySeMusicVol(6, 1, 0);
            Parts_SlideInPanels(255);
        } else if (edit->padPressed & 0x40) {
            Sound_PlayCancel();
            task->mode = CHARA_CREATE_ASK_QUIT;
            edit->choice = CHARA_CHOICE_NO;
        }
        break;
    case CHARA_CREATE_PANELS:
        Chara_RotateByPad();
        if (edit->padPressed & 0x20) {
            if (edit->panel == CHARA_PANEL_FINISH) {
                Chara_PlayDecideSe();
                task->mode = CHARA_CREATE_CONFIRM;
                edit->choice = CHARA_CHOICE_NO;
                Chara_SetCamera(gPlayers, gCharaCameraModes[0], 1);
            } else {
                task->mode++;
                Chara_PlayDecideSe();
                Chara_SetCamera(gPlayers, gCharaCameraModes[edit->panel], 1);
                Parts_SlideOutPanels(edit->panel);
                edit->inputDelay = 20;
                edit->cursor = 0;
                Parts_ResetBounces();
                Parts_StartPanel(0);
                switch (edit->panel) {
                case CHARA_PANEL_NAME:
                    Chat_OpenKbd(3, 15, 8, edit->name);
                    break;
                case CHARA_PANEL_COLOR:
                    break;
                case CHARA_PANEL_FACE:
                    Chara_FilterParts(edit, CHARA_PARTS_FACE);
                    break;
                case CHARA_PANEL_HAIR:
                    Chara_FilterParts(edit, CHARA_PARTS_HAIR);
                    break;
                case CHARA_PANEL_VOICE:
                    Chara_FilterParts(edit, CHARA_PARTS_VOICE);
                    break;
                }
            }
        } else if (edit->padPressed & 0x40) {
            Sound_PlayCancel();
            task->mode = CHARA_CREATE_ASK_GENDER;
            edit->choice = CHARA_CHOICE_NO;
        } else if (edit->padPressed & 0x2000) {
            if (edit->panel == CHARA_PANEL_NAME) {
                edit->panel = CHARA_PANEL_FINISH;
            } else {
                edit->panel--;
            }
            Sound_PlaySeFlat(1, 12, 0);
        } else if (edit->padPressed & 0x1000) {
            if (edit->panel >= CHARA_PANEL_FINISH) {
                edit->panel = CHARA_PANEL_NAME;
            } else {
                edit->panel++;
            }
            Sound_PlaySeFlat(1, 12, 0);
        } else if (((edit->padHeld & 0x4000) && (edit->padPressed & 0x8000)) ||
                   ((edit->padPressed & 0x4000) && (edit->padHeld & 0x8000))) {
            edit->type = Rand_Next(1) & 3;
            edit->face = Rand_Next(1) % CHARA_FACE_COUNT;
            edit->color = gPresetColors[Rand_Next(1) & (CHARA_PRESET_COLOR_COUNT - 1)];
            edit->hair = Rand_Next(1) % (CHARA_HAIR_COUNT - 1);
            edit->voice = Rand_Next(1) % CHARA_VOICE_COUNT;
            Sound_LoadVoiceBank(edit->gender, edit->voice);
            if (edit->gender == 0) {
                edit->cond[0] = gCharaFaceConds0[edit->face][0];
                edit->cond[1] = gCharaFaceConds0[edit->face][1];
                edit->cond[2] = gCharaFaceConds0[edit->face][2];
                edit->cond[3] = gCharaHairConds0[edit->hair][0];
                edit->cond[4] = gCharaHairConds0[edit->hair][1];
                edit->cond[5] = gCharaHairConds0[edit->hair][2];
                edit->cond[6] = gCharaVoiceConds0[edit->voice][0];
                edit->cond[7] = gCharaVoiceConds0[edit->voice][1];
            } else {
                edit->cond[0] = gCharaFaceConds1[edit->face][0];
                edit->cond[1] = gCharaFaceConds1[edit->face][1];
                edit->cond[2] = gCharaFaceConds1[edit->face][2];
                edit->cond[3] = gCharaHairConds1[edit->hair][0];
                edit->cond[4] = gCharaHairConds1[edit->hair][1];
                edit->cond[5] = gCharaHairConds1[edit->hair][2];
                edit->cond[6] = gCharaVoiceConds1[edit->voice][0];
                edit->cond[7] = gCharaVoiceConds1[edit->voice][1];
            }
            for (i = 0; i < 8; i++) {
                if (edit->cond[i] == CHARA_COND_NONE) {
                    edit->cond[i] = 0;
                }
            }
        }
        break;
    case CHARA_CREATE_EDIT:
        Chara_RotateByPad();
        switch (edit->panel) {
        case CHARA_PANEL_NAME:
            if (Kbd_Run(edit->name, edit->padHeld, edit->padPressed) != 0) {
                Kbd_Cancel();
                Chara_ClosePage(task, edit);
            }
            break;
        case CHARA_PANEL_TYPE:
            if (edit->padPressed & 0x20) {
                Chara_PlayDecideSe();
                Chara_ClosePage(task, edit);
            } else {
                Chara_MoveCursorWrap(edit, trig, &edit->type, 4, 18, 1);
            }
            break;
        case CHARA_PANEL_FACE:
            if (Chara_FacePage(edit) == 1) {
                Chara_PlayDecideSe();
                Chara_ClosePage(task, edit);
            }
            break;
        case CHARA_PANEL_HAIR:
            if (Chara_HairPage(edit) == 1) {
                Chara_PlayDecideSe();
                Chara_ClosePage(task, edit);
            }
            break;
        case CHARA_PANEL_COLOR:
            if (Chara_EditColor(edit, gPlayers, trig) == 1) {
                Chara_PlayDecideSe();
                Chara_ClosePage(task, edit);
            }
            break;
        case CHARA_PANEL_VOICE:
            if (Chara_VoicePage(edit) == 1) {
                Chara_PlayDecideSe();
                Chara_ClosePage(task, edit);
            } else if ((edit->padPressed & 0x100) && edit->soundWait == 0) {
                edit->soundWait = 5;
                Sound_PlaySeFlat(2, edit->sound, 0);
                edit->sound++;
                if (edit->sound >= 12) {
                    edit->sound = 0;
                }
            }
            break;
        }
        break;
    case CHARA_CREATE_CONFIRM:
        Chara_RotateByPad();
        if (Chara_NameHasText(edit) == 0 || Chara_CheckName(edit->name) == 0) {
            if (edit->padPressed & 0x60) {
                task->mode = CHARA_CREATE_PANELS;
                Chara_PlayCancelSe();
            }
        } else if ((edit->padPressed & 0x20) && edit->choice == CHARA_CHOICE_YES) {
            Sound_PlaySeFlat(1, 6, 0);
            edit->slot = 0;
            task->mode++;
            NetMenu_Open(NETMENU_MODE_SAVE_NEW);
        } else if (((edit->padPressed & 0x20) && edit->choice == CHARA_CHOICE_NO) || (edit->padPressed & 0x40)) {
            task->mode = CHARA_CREATE_PANELS;
            Chara_PlayCancelSe();
        } else {
            if ((trig & 0x800) && edit->choice != CHARA_CHOICE_YES) {
                Sound_PlaySeFlat(0, 18, 0);
                edit->choice = CHARA_CHOICE_YES;
            }
            if ((trig & 0x400) && edit->choice == CHARA_CHOICE_YES) {
                Sound_PlaySeFlat(0, 18, 0);
                edit->choice = CHARA_CHOICE_NO;
            }
        }
        break;
    case CHARA_CREATE_SAVE:
        Chara_RotateByPad();
        saveResult = NetMenu_Update();
        if (saveResult != 0) {
            if (saveResult == 2) {
                edit->savedToCard = 0;
                Sound_PlaySeFlat(1, 4, 0);
            } else {
                edit->savedToCard = 1;
            }
            Chara_MakeSave(edit, CHARA_SLOT_NONE);
            task->mode++;
            gQuestSync.charIndex = edit->slot;
            edit->timer = 0;
            Chara_SetCamera(gPlayers, CHARA_CAM_FRONT, 1);
            for (i = 0, player = gPlayers; i < 2; i++, player++) {
                player->rot[1] = 0;
                Chara_SetVoiceMotion(player, edit->gender, edit->voice);
            }
            Sound_PlaySeMusicVol(1, 2, 0);
            Sound_PlaySeMusicVol(1, 3, 0);
        }
        break;
    case CHARA_CREATE_DONE:
        if (++edit->timer >= 60) {
            task->mode++;
            FadeA_Start(5);
        }
        break;
    case CHARA_CREATE_FADE_OUT:
        if (FadeA_GetState() != 1) {
            task->mode++;
            task->step = 0;
            edit->timer = 10;
        }
        break;
    case CHARA_CREATE_END:
        /* Step 0 ends the task after 10 frames. Steps 1 and 2 play the
         * opening movie, but nothing in select sets them. The labels share
         * the task's end between the steps, as the game's code jumps. */
        switch (task->step) {
        case 0:
            if (--edit->timer <= 0) {
                Game_ResetSubsystems();
                FadeA_Start(2);
                goto end;
            }
            break;
        case 1:
            Movie_Update();
            movieStatus = Movie_GetStatus();
            if (movieStatus != 3) {
                edit->timer++;
                if (movieStatus != 2 || !(gPadState[0].pressed & 0x8000)) {
                    break;
                }
            }
            FadeA_Start(1);
            task->step++;
            edit->timer = 150;
            Movie_RequestStop();
            break;
        case 2:
            Movie_Update();
            if (FadeA_GetState() != 1) {
                if (Movie_GetStatus() != 0) {
                    goto check;
                }
            stop:
                {
                    Movie_Stop();
                end:
                    TaskSlot_End(task);
                    gSysSettings.online = 0;
                    gSysSettings.unk03 = 1;
                    gSysSettings.unk02 = 0;
                    gSysSettings.unk05 = 0;
                    gSysSettings.useSavedEquip = 0;
                    gSysSettings.galleryPlay = 0;
                    gSysSettings.unk45 = 0;
                    Task_EndGmSubForNetScene();
                    FadeA_Start(2);
                    return;
                }
            check:
                if (edit->timer == 0) {
                    goto stop;
                }
                edit->timer--;
            }
            break;
        }
        break;
    case CHARA_CREATE_ASK_QUIT:
        if ((edit->padPressed & 0x20) && edit->choice == CHARA_CHOICE_YES) {
            task->mode++;
            Chara_PlayCancelSe();
            Sound_PlaySeMusicVol(1, 2, 0);
            Sound_PlaySeMusicVol(1, 3, 0);
            FadeA_Start(1);
        } else if (((edit->padPressed & 0x20) && edit->choice == CHARA_CHOICE_NO) || (edit->padPressed & 0x40)) {
            task->mode = CHARA_CREATE_GENDER;
            Chara_PlayCancelSe();
        } else {
            if ((trig & 0x800) && edit->choice != CHARA_CHOICE_YES) {
                Sound_PlaySeFlat(0, 18, 0);
                edit->choice = CHARA_CHOICE_YES;
            }
            if ((trig & 0x400) && edit->choice == CHARA_CHOICE_YES) {
                Sound_PlaySeFlat(0, 18, 0);
                edit->choice = CHARA_CHOICE_NO;
            }
        }
        break;
    case CHARA_CREATE_QUIT:
        if (FadeA_GetState() != 1) {
            TaskSlot_End(task);
            Task_StartGmSub();
            FadeA_Start(2);
            return;
        }
        break;
    case CHARA_CREATE_ASK_GENDER:
        Chara_RotateByPad();
        if ((edit->padPressed & 0x20) && edit->choice == CHARA_CHOICE_YES) {
            task->mode = CHARA_CREATE_GENDER;
            Sound_Silence();
            Chara_PlayCancelSe();
        } else if (((edit->padPressed & 0x20) && edit->choice == CHARA_CHOICE_NO) || (edit->padPressed & 0x40)) {
            task->mode = CHARA_CREATE_PANELS;
            Chara_PlayCancelSe();
        } else {
            if ((trig & 0x800) && edit->choice != CHARA_CHOICE_YES) {
                Sound_PlaySeFlat(0, 18, 0);
                edit->choice = CHARA_CHOICE_YES;
            }
            if ((trig & 0x400) && edit->choice == CHARA_CHOICE_YES) {
                Sound_PlaySeFlat(0, 18, 0);
                edit->choice = CHARA_CHOICE_NO;
            }
        }
        break;
    }
    pl = gPlayers;
    if (task->mode >= CHARA_CREATE_PANELS && pl->active != 0) {
        Player_TickTimers(pl);
        Player_UpdateHitStop(pl);
        Player_UpdateMotionStep(pl);
    }
    Players_UpdateMatrices();
    LightSet_UpdateAll();
    edit->frameCount++;
    gCamera->at[0] += (edit->camTarget[0] - gCamera->at[0]) / 8.0f;
    gCamera->at[1] += (edit->camTarget[1] - gCamera->at[1]) / 8.0f;
    gCamera->at[2] += (edit->camTarget[2] - gCamera->at[2]) / 8.0f;
    gCamera->eye[0] += (edit->camPos[0] - gCamera->eye[0]) / 8.0f;
    gCamera->eye[1] += (edit->camPos[1] - gCamera->eye[1]) / 8.0f;
    gCamera->eye[2] += (edit->camPos[2] - gCamera->eye[2]) / 8.0f;
    Camera_Update();
    if (task->mode != CHARA_CREATE_END) {
        if (task->mode >= CHARA_CREATE_PANELS && task->mode != CHARA_CREATE_ASK_QUIT && task->mode != CHARA_CREATE_QUIT) {
            pl = gPlayers;
            if (gPlayers[0].active != 0) {
            pl->drawSlot->pos[0] = pl->pos[0];
            pl->drawSlot->pos[1] = pl->pos[1];
            pl->drawSlot->pos[2] = pl->pos[2];
            OT_AddByDepth(gDrawOt32, pl->drawSlot, 32, 0);
            }
        }
        Parts_Animate();
        OT_AddAt(gDrawOtSorted, &gSelectDrawHook, 0, 64);
        OT_AddAt(gDrawListText2, &gCharaTextDrawHook, 0, 1);
        Chara_DrawBackground();
        if (task->mode >= CHARA_CREATE_PANELS && task->mode < CHARA_CREATE_SAVE) {
            Parts_DrawPanels(task);
        }
        Frame_Draw();
    }
}

/* Draws the selection's backdrop and header (the parameter is not used). */
void Chara_DrawHeader(ChrEdit *unused)
{
    Gs_SetState(RS_ZWRITE, 0);
    Sel_DrawBackground(255);
    Gs_SetState(RS_ZWRITE, 1);
    Sel_DrawHeader(5);
}

/*
 * Draws the icon of an armour piece (idx, gArmorIconUV) from system texture
 * 8, tinted by its rarity.
 */
void Chara_DrawArmorIcon(s16 x, s16 y, s16 kind, s16 idx, s32 rarity)
{
    Sprite spr;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 8);
    Tex_Bind(8);
    spr.x = x - 38;
    spr.y = y - 7;
    spr.w = 32;
    spr.h = 32;
    spr.color = Equip_Color(rarity, 255, 0);
    if (kind == 0) {
        spr.u0 = gArmorIconUV[idx][0];
        spr.v0 = gArmorIconUV[idx][1];
    } else {
        spr.u0 = gArmorIconUVAlt[idx][0];
        spr.v0 = gArmorIconUVAlt[idx][1];
    }
    spr.u1 = spr.u0 + 24;
    spr.v1 = spr.v0 + 24;
    Sprite_Draw(&spr);
}

/*
 * Draws an equipment icon from the 32x32 icon sheet (kind 0: a weapon
 * class; otherwise an armour piece, gArmorIconCells), tinted by rarity.
 * Computing u, u0, then v, v0 gives the game's schedule of the texture
 * coordinates.
 */
void Chara_DrawEquipIcon(s16 x, s16 y, s16 kind, s16 idx, s32 rarity)
{
    Sprite spr;
    s32 cell;
    s32 texU;
    s32 texV;

    if (kind == 0) {
        cell = idx + 63;
    } else {
        cell = gArmorIconCells[idx] + 74;
    }
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 324);
    Tex_Bind(324);
    spr.x = x - 38;
    spr.y = y - 7;
    spr.w = 32;
    spr.h = 32;
    texU = (cell & 7) * 32;
    spr.u0 = texU + 1;
    texV = (cell >> 3) * 32;
    spr.v0 = texV + 1;
    spr.u1 = texU + 31;
    spr.v1 = texV + 31;
    spr.color = Equip_Color(rarity, 255, 0);
    Sprite_Draw(&spr);
}


u8 Equip_GetRarity(u8 type, u16 id);      /* an item's rarity */
s32 Rarity_Color(u8 rarity);             /* the rarity's colour; unit's own declaration: s32 as Chara_DrawEquipment uses it; with the definition's s16 return, the call compiles differently */
u8 Weapon_GetClass(u8 type, u16 id);

/*
 * Draws the selected character's equipment: the weapon and the five armour
 * pieces, each name in its rarity's colour beside its icon. The slot is
 * read from gChrEdit (the parameter is not used).
 */
void Chara_DrawEquipment(ChrEdit *unused, s32 mode)
{
    u8 rarity;
    SaveData *save;
    Frame win;

    save = gCardSlots;
    save += gChrEdit.slot;
    win.x = 350;
    win.w = 240;
    win.y = 88;
    win.h = 240;
    win.style = 1;
    win.color = -1;
    win.color2 = 0xC0402010;
    WindowFrame_Draw(&win, 0);
    Font_SetSpacing(18, 18);
    if (save->equip[0].type == 7) {
        rarity = Equip_GetRarity(7, save->equip[0].id);
        Text_PrintfAt(390, 110, Rarity_Color(rarity), gCharaStrFormat, gGunNames[save->equip[0].id]);
    } else {
        rarity = Equip_GetRarity(6, save->equip[0].id);
        Text_PrintfAt(390, 110, Rarity_Color(rarity), gCharaStrFormat, gBladeNames[save->equip[0].id]);
    }
    Chara_DrawEquipIcon(390, 110, 0, Weapon_GetClass(save->equip[0].type, save->equip[0].id), rarity);
    rarity = Equip_GetRarity(2, save->equip[2].id);
    Text_PrintfAt(390, 145, Rarity_Color(rarity), gCharaStrFormat, gArmorHeadNames[save->equip[2].id]);
    Chara_DrawArmorIcon(390, 145, 0, 0, rarity);
    rarity = Equip_GetRarity(3, save->equip[3].id);
    Text_PrintfAt(390, 180, Rarity_Color(rarity), gCharaStrFormat, gArmorBodyNames[save->equip[3].id]);
    Chara_DrawArmorIcon(390, 180, 0, 1, rarity);
    rarity = Equip_GetRarity(4, save->equip[4].id);
    Text_PrintfAt(390, 215, Rarity_Color(rarity), gCharaStrFormat, gArmorArmsNames[save->equip[4].id]);
    Chara_DrawArmorIcon(390, 215, 0, 2, rarity);
    rarity = Equip_GetRarity(5, save->equip[5].id);
    Text_PrintfAt(390, 250, Rarity_Color(rarity), gCharaStrFormat, gArmorWaistNames[save->equip[5].id]);
    Chara_DrawArmorIcon(390, 250, 0, 3, rarity);
    rarity = Equip_GetRarity(0, save->equip[1].id);
    Text_PrintfAt(390, 285, Rarity_Color(rarity), gCharaStrFormat, gArmorLegsNames[save->equip[1].id]);
    Chara_DrawArmorIcon(390, 285, 0, 4, rarity);
}

struct NetMenu;

/*
 * The selection's draw hook (gSelectDrawHook): the yes/no question, the
 * help, the card slots with each character's equipment or the rotate hint,
 * and the card menu's message with its blinking button.
 */
void Chara_DrawPrompts(DrawSlot *hook)
{
    TaskSlot *task;
    ChrEdit *edit;

    task = hook->owner;
    edit = &gChrEdit;
    Gs_SetState(RS_ALPHA_REF, 0);
    if (task->mode < CHARA_SELECT_MESSAGE) {
        if (task->mode == CHARA_SELECT_CONFIRM) {
            Chara_DrawQuestion(edit, CHARA_ASK_SELECT);
        }
        if (task->mode == CHARA_SELECT_CHOOSE) {
            Chara_DrawEditHelp(edit, 1);
        }
        if (task->mode >= CHARA_SELECT_LOAD) {
            if (GmSub_IsCharaSelectMenu() == 1) {
                if (CharaSel_IsIdle() != 0) {
                    CardMenu_DrawSlots((struct NetMenu *)edit, 1, task->mode, 0, 1);
                    if (Chara_EquipViewOn() == 0) {
                        Hud_DrawButtonIconWhite(18, 518, 96, 8, 1.0f);
                        Font_SetSpacing(20, 20);
                        Text_PrintfAt(544, 96, 0, gCharaPageTexts[0]);
                    } else {
                        Chara_DrawEquipment(edit, 1);
                    }
                }
            } else {
                CardMenu_DrawSlots((struct NetMenu *)edit, 1, task->mode, 0, 1);
                if (Chara_EquipViewOn() == 0) {
                    Hud_DrawButtonIconWhite(18, 518, 96, 8, 1.0f);
                    Font_SetSpacing(20, 20);
                    Text_PrintfAt(544, 96, 0, gCharaPageTexts[0]);
                } else {
                    Chara_DrawEquipment(edit, 1);
                }
            }
        }
    }
    if (task->mode == CHARA_SELECT_MESSAGE) {
        NetMenu_DrawMessageY(356, 40);
        WindowFrame_Draw(&gCharaHelpFrame, 0);
        if (!((gChrEdit.timer / 15) & 1)) {
            Hud_DrawButtonIconWhite(20, 562, 388, 8, 1.0f);
        }
    }
}

/*
 * Character selection, the task (CharaSelectStep): the card's load menu,
 * then the saved characters (left and right skip the empty slots), the
 * yes/no question, and the character's greeting before the game starts.
 * Cancelling returns to gm_sub's menu, or to the PSP link when it waits
 * for a character. One u16 local holds the pad's pressed-or-repeat bits
 * and, in the first step, the area id: the game's saved registers come
 * from that reuse. The slot's record address is read, then advanced.
 */
void Chara_SelectTask(TaskSlot *task)
{
    ChrEdit *edit;
    Player *player;
    s32 trig;
    SaveData *save;
    s16 i;
    Player *pl;
    s16 menuResult;
    u8 firstSlot;
    u8 oldSlot;
    u16 pressedOrRepeat;

    edit = &gChrEdit;
    player = gPlayers;
    trig = gPadState[0].pressed;
    pressedOrRepeat = trig | gPadState[0].repeat;
    (gScene.frame)++;
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    switch (task->mode) {
    case CHARA_SELECT_INIT:
        task->mode++;
        Game_ResetSubsystems();
        Tex_LoadSystemPack(0);
        if (GmSub_IsCharaSelectMenu() == 1) {
            Sel_LoadCommonTextures();
            Tex_LoadPack(gSystemTexFiles[1], 324, 0);
            CharaSel_Start();
        }
        FadeA_Start(2);
        select_entry(0);
        gSelectDrawHook.owner = task;
        gSelectDrawHook.draw = Chara_DrawPrompts;
        Camera_Reset();
        CamCtl_Clear();
        gScene.area = 0;
        gScene.questId = 0;
        Quest_Clear();
        Light_InitAll(gScene.area);
        pressedOrRepeat = gScene.area;
        gSysSettings.soundLoading = 1;
        Area_Enter(pressedOrRepeat);
        gSysSettings.soundLoading = 0;
        gCamera->fov = 0.87266463f;
        gCamera->roll = 0.0f;
        NetMenu_Open(NETMENU_MODE_LOAD);
        break;
    case CHARA_SELECT_CARD:
        menuResult = NetMenu_Update();
        if (menuResult == 0) {
            break;
        }
        if (menuResult == 2) {
            Sound_PlaySeMusicVol(1, 2, 0);
            Sound_PlaySeMusicVol(1, 3, 0);
            if (gSysSettings.pspCharaWait != 0) {
                gSysSettings.pspCharaWait = 0;
                TaskSlot_End(TaskSlot_Get(5));
                Task_StartBootToGmSub();
            } else {
                TaskSlot_End(task);
                Task_StartGmSub();
            }
            if (GmSub_IsCharaSelectMenu() == 1) {
                Sel_FreeTextures();
                Tex_FreeRange(324, 1);
            }
            return;
        }
        edit->equipView = gSysSave.options.equipView;
        edit->slot = CHARA_SLOT_NONE;
        if (Card_ReadyCount() != 0 && Chara_EquipViewOn() == 0) {
            Loading_Start();
        }
        save = gCardSlots;
        for (i = 0; i < 3; i++) {
            player->rot[1] = 0;
            if (save->used == 0) {
                player->active = 0;
                player->pos[0] = gStartPos[gScene.area].x;
                player->pos[1] = gStartPos[gScene.area].y;
                player->pos[2] = gStartPos[gScene.area].z;
            } else {
                if (edit->slot == CHARA_SLOT_NONE) {
                    edit->slot = i;
                }
                if (Card_SlotReady(i) == 1) {
                    Chara_LoadSlot((s32)edit, player, i);
                    if (Chara_EquipViewOn() == 0) {
                        Chara_SetupCardPlayer(player, 1, i);
                        player->visible = 0;
                    }
                } else {
                    player->pos[0] = gStartPos[gScene.area].x;
                    player->pos[1] = gStartPos[gScene.area].y;
                    player->pos[2] = gStartPos[gScene.area].z;
                }
            }
            player++;
            save++;
        }
        firstSlot = gSysSave.lastSlot;
        i = firstSlot;
        while (1) {
            if (gCardSlots[i].used == 0) {
                i++;
                if (i >= 3) {
                    i = 0;
                }
                if (i == firstSlot) {
                    break;
                }
            } else {
                gSysSave.lastSlot = i;
                break;
            }
        }
        edit->slot = gSysSave.lastSlot;
        Chara_SetCamera(&gPlayers[edit->slot], CHARA_CAM_BODY, 0);
        if (Chara_EquipViewOn() == 0) {
            Anim_LoadCommon(4);
        }
        if (GmSub_IsCharaSelectMenu() == 1) {
            CharaSel_Start();
        }
        task->mode++;
    case CHARA_SELECT_LOAD:
        if (Sound_LoadSelectBanks(task) == 0) {
            break;
        }
        task->mode++;
        task->step = 0;
        save = gCardSlots;
        save += edit->slot;
        if (Card_SlotReady(edit->slot) == 1 && save->used != 0) {
            Sound_LoadVoiceBank(edit->gender, edit->voice);
            edit->panel = edit->slot;
            if (Chara_EquipViewOn() == 0) {
                player->visible = 1;
            }
        }
        Loading_Stop();
        Sound_PlaySeMusicVol(6, 0, 0);
        Sound_PlaySeMusicVol(6, 1, 0);
        Sound_PlaySeFlat(0, 25, 0);
        Chara_EquipViewOn();
        edit->timer = 1;
    case CHARA_SELECT_CHOOSE:
        /* goto check: past the input, to the step's end (the game's jumps) */
        if (edit->timer > 0) {
            edit->timer--;
            goto check;
        }
        if (GmSub_IsCharaSelectMenu() == 1 && CharaSel_IsIdle() == 0) {
            goto check;
        }
        save = gCardSlots;
        save += edit->slot;
        if (Card_SlotReady(edit->slot) == 1 && save->used != 0 && Chara_EquipViewOn() == 0) {
            Chara_RotateByPad();
        }
        if ((trig & 0x20) && Card_SlotReady(edit->slot) == 1) {
            task->mode++;
            edit->choice = CHARA_CHOICE_YES;
            Chara_PlayDecideSe();
            break;
        }
        if (trig & 0x40) {
            Chara_PlayCancelSe();
            Sound_PlaySeMusicVol(1, 2, 0);
            Sound_PlaySeMusicVol(1, 3, 0);
            TaskSlot_End(task);
            if (gSysSettings.pspCharaWait != 0) {
                gSysSettings.pspCharaWait = 0;
                TaskSlot_End(TaskSlot_Get(5));
                Task_StartBootToGmSub();
            } else {
                Task_StartGmSub();
                if (GmSub_IsCharaSelectMenu() == 1) {
                    Sel_FreeTextures();
                    Tex_FreeRange(324, 1);
                }
            }
            return;
        }
        if (trig & 0x2000) {
            oldSlot = edit->slot;
            do {
                if (edit->slot == 0) {
                    edit->slot = 2;
                } else {
                    edit->slot--;
                }
                save = gCardSlots;
        save += edit->slot;
            } while (Card_SlotReady(edit->slot) != 0 && save->used == 0);
            if (oldSlot != edit->slot) {
                Sound_PlaySlotChange();
                if (Chara_EquipViewOn() == 0) {
                    *(s32 *)((u8 *)&gPlayers[0].lightFadeColors[0] + edit->slot * 2416) = 0x01000000;
                    *(s32 *)((u8 *)&gPlayers[0].lightFadeColors[1] + edit->slot * 2416) = 0x01000000;
                    *(s32 *)((u8 *)&gPlayers[0].lightFadeColors[2] + edit->slot * 2416) = 0x01000000;
                }
            }
        } else if (trig & 0x1000) {
            oldSlot = edit->slot;
            do {
                if (edit->slot >= 2) {
                    edit->slot = 0;
                } else {
                    edit->slot++;
                }
                save = gCardSlots;
        save += edit->slot;
            } while (Card_SlotReady(edit->slot) != 0 && save->used == 0);
            if (oldSlot != edit->slot) {
                Sound_PlaySlotChange();
                if (edit->slot != CHARA_SLOT_NONE && Chara_EquipViewOn() == 0) {
                    *(s32 *)((u8 *)&gPlayers[0].lightFadeColors[0] + edit->slot * 2416) = 0x01000000;
                    *(s32 *)((u8 *)&gPlayers[0].lightFadeColors[1] + edit->slot * 2416) = 0x01000000;
                    *(s32 *)((u8 *)&gPlayers[0].lightFadeColors[2] + edit->slot * 2416) = 0x01000000;
                }
            }
        }
    check:
        if (edit->panel != edit->slot) {
            edit->panel = edit->slot;
        }
        break;
    case CHARA_SELECT_CONFIRM:
        if (Chara_EquipViewOn() == 0) {
            Chara_RotateByPad();
        }
        if ((trig & 0x20) && edit->choice == CHARA_CHOICE_YES) {
            if (GmSub_IsCharaSelectMenu() == 1) {
                CharaSel_Leave();
            }
            task->mode++;
            Chara_PlayDecideSe();
            gSysSave.lastSlot = edit->slot;
            pl = &gPlayers[edit->slot];
            Chara_SetCamera(pl, CHARA_CAM_FRONT, 1);
            pl->rot[1] = 0;
            edit->timer = 0;
            gQuestSync.charIndex = edit->slot;
            Save_LoadSlot(edit->slot);
        } else if (((trig & 0x20) && edit->choice == CHARA_CHOICE_NO) || (trig & 0x40)) {
            Chara_PlayCancelSe();
            task->mode = CHARA_SELECT_CHOOSE;
        } else if ((pressedOrRepeat & 0x800) && edit->choice != CHARA_CHOICE_YES) {
            Sound_PlaySeFlat(0, 18, 0);
            edit->choice = CHARA_CHOICE_YES;
        } else if ((pressedOrRepeat & 0x400) && edit->choice == CHARA_CHOICE_YES) {
            Sound_PlaySeFlat(0, 18, 0);
            edit->choice = CHARA_CHOICE_NO;
        }
        break;
    case CHARA_SELECT_MESSAGE:
        edit->timer++;
        if (trig & 0x20) {
            gSysSettings.systemSaved = 1;
            task->mode++;
            Sound_PlaySeFlat(1, 4, 0);
            Sound_PlaySeMusicVol(1, 2, 0);
            Sound_PlaySeMusicVol(1, 3, 0);
            if (Chara_EquipViewOn() == 0) {
                pl = &gPlayers[edit->slot];
                Chara_SetVoiceMotion(pl, pl->gender, pl->voice);
                edit->timer = 0;
            } else {
                edit->timer = 65;
            }
        }
        break;
    case CHARA_SELECT_DONE:
        if (GmSub_IsCharaSelectMenu() == 1) {
            if (CharaSel_IsDone() != 0) {
                if (++edit->timer >= 65) {
                    task->mode++;
                    FadeA_Start(5);
                }
            }
        } else {
            if (++edit->timer >= 65) {
                task->mode++;
                FadeA_Start(5);
            }
        }
        break;
    case CHARA_SELECT_FADE_OUT:
        if (FadeA_GetState() != 1) {
            task->mode++;
            edit->timer = 60;
        }
        break;
    case CHARA_SELECT_END:
        if (--edit->timer <= 0) {
            TaskSlot_End(task);
            if (gSysSettings.pspCharaWait != 0) {
                gSysSettings.pspCharaWait = 0;
                FadeA_Start(2);
            } else {
                TaskSlot_End(TaskSlot_Get(2));
                Task_StartBootToGmSubKeep();
                FadeA_Start(2);
            }
            return;
        }
        break;
    }
    edit->frameCount++;
    Chara_Frame(task);
}


/*
 * The selection's frame after its step: the character selection screen
 * (when it runs as the menu's), the shown character's motion, the camera
 * easing a tenth of the way to its target, and the drawing.
 */
void Chara_Frame(TaskSlot *task)
{
    ChrEdit *edit;
    Player *pl;
    s32 i;

    edit = &gChrEdit;
    if (GmSub_IsCharaSelectMenu() == 1) {
        if (task->mode >= CHARA_SELECT_CHOOSE && task->mode < CHARA_SELECT_FADE_OUT) {
            CharaSel_Update();
        }
        if (CharaSel_IsShown() == 0) {
            /* the selection screen is not up yet: only the drawing */
            goto draw;
        }
    }
    if (Chara_EquipViewOn() != 1 && Card_SlotReady(edit->slot) != 0) {
        if (task->mode >= CHARA_SELECT_LOAD) {
            for (i = 0, pl = gPlayers; i < 3; i++, pl++) {
                if (pl->active != 0) {
                    if (i == edit->slot) {
                        pl->visible = 1;
                    } else {
                        pl->visible = 0;
                    }
                    Player_TickTimers(pl);
                    Player_UpdateHitStop(pl);
                    Player_UpdateMotionStep(pl);
                }
            }
            Players_RunEquipEffects();
            Players_UpdateMatrices();
            LightSet_UpdateAll();
        }
        gCamera->at[0] += (edit->camTarget[0] - gCamera->at[0]) / 10.0f;
        gCamera->at[1] += (edit->camTarget[1] - gCamera->at[1]) / 10.0f;
        gCamera->at[2] += (edit->camTarget[2] - gCamera->at[2]) / 10.0f;
        gCamera->eye[0] += (edit->camPos[0] - gCamera->eye[0]) / 10.0f;
        gCamera->eye[1] += (edit->camPos[1] - gCamera->eye[1]) / 10.0f;
        gCamera->eye[2] += (edit->camPos[2] - gCamera->eye[2]) / 10.0f;
        Camera_Update();
    }
draw:
    if (task->mode < CHARA_SELECT_END) {
        if (Card_SlotReady(edit->slot) == 1 && task->mode >= CHARA_SELECT_LOAD && Chara_EquipViewOn() == 0) {
            pl = &gPlayers[edit->slot];
            if (task->mode >= CHARA_SELECT_LOAD && pl->drawSlot != 0) {
                pl->drawSlot->pos[0] = pl->pos[0];
                pl->drawSlot->pos[1] = pl->pos[1];
                pl->drawSlot->pos[2] = pl->pos[2];
                OT_AddByDepth(gDrawOt32, pl->drawSlot, 32, 0);
            }
        }
        if (GmSub_IsCharaSelectMenu() == 0) {
            OT_AddAt(gDrawOtSorted, &gSelectDrawHook, 0, 64);
            Chara_DrawHeader(edit);
        } else {
            CharaSel_Draw();
            OT_AddAt(gDrawOtSorted, &gSelectDrawHook, 0, 64);
        }
        Frame_Draw();
    }
}

/* Whether the name is allowed (NgWord_Check). */
s32 Chara_CheckName(s8 *name)
{
    return NgWord_Check(name);
}
