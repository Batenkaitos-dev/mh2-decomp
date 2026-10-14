/*
 * select: character creation and selection (0x6AF280-0x6B4B30), the first
 * unit of the overlay. The file owns its .data (0x6C7D80-0x6C80A0) and
 * .rodata (0x6CDD80-0x6CDEA0).
 */
#include "common.h"
#include "window.h"
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
extern SaveData *gCardSlots; /* sub_main: the save slots read from the memory card */

void ColorEdit_Draw(u8 mode, s16 x, s16 y, u8 r, u8 g, u8 b, u8 sel, u8 pal);
void Chara_SetupSavedPlayer(Player *p, s32 mode, s32 slot);
/* The caller passes an s16 slot unextended: the slot is an s16. */
void Chara_SetupNewPlayer(Player *p, s32 mode, s16 slot);
void CharaSel_Update(void);
void Chara_PlayCancelSe(void);
u32 Equip_Color(u8 idx, u8 alpha, s32 alt);      /* a colour from an alpha */
struct Frame;
void WindowFrame_Draw(struct Frame *, s16 *off);       /* draws a window */
void Text_SetColor(s16 color);
void Text_SetSize(s32 size);
s32 Text_DisplayLength(char *);            /* a message's width */
void Text_Printf(const char *fmt, ...);     /* prints */
u16 Rand_Next(s32);
extern char *gNoMatchTexts[];          /* messages (select_psp.c) */
void Parts_StartBounce(u8);
s32 Sound_LoadVoiceBank(s32 sex, s32 voice);
void Sound_PlayCancel(void);
extern char *gRotateTexts[];  /* messages (select_psp.c) */
extern char *gCharaPartNames[];
extern char *gNameEntryHelpTexts[];
extern char *gCharaPartHelpTexts[];
extern char *gCharaEditHelpTexts[];
extern char *gCharaSelectTexts[];
void Chara_DrawNoMatch(ChrEdit *c);
void Hud_DrawButtonIconWhite(s32 a, s32 b, s32 c, s32 d, f32 scale);
/* unit's own declaration: with the definition's void (s16 x, s16 y, s16 color, const char *fmt, ...) (sub_main/unk_368F10.c), Chara_DrawEquipment: 159 of 217 words differ */
void Text_PrintfAt(s16, s16, s32, const char *, ...);
s32 Str_Length(char *);                /* a message's width */
extern char *gCardSavingTexts[];
void Player_SetupFromSave(struct Player *, struct SaveData *);
void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);


void Chara_SetCamera(void *obj, s32 mode, s32 hold);
void Chara_StartTabAnim(u8 sel);
void Parts_StartPanel(s16 flag);


void Sel_DrawBackground(u8 c);
void Sel_DrawHeader(s16 kind);
s32 NgWord_Check(s8 *name);

/* .data */
s16 gCharaCameraModes[8] = {
#include "select/006C7D80.inc"
};
Frame gCharaHelpFrame = {
#include "select/006C7D90.inc"
};
Frame gCharaNoMatchFrame = {
#include "select/006C7DB0.inc"
};
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
s16 gCharaVoiceMotions[30][4] = {
#include "select/006C7F50.inc"
};
u8 gCharaStepConds[8][2] = {
#include "select/006C8040.inc"
};
s16 D_006C8050[8][2] = {
#include "select/006C8050.inc"
};
s16 D_006C8070[8][2] = {
#include "select/006C8070.inc"
};
u32 gCharaIconUV[4] = {
#include "select/006C8090.inc"
};


extern u32 gPresetColors[];      /* sub_main: the preset colours */

/* The overlay's entry: resets the character being created (keep != 0
 * keeps its name, colour and the first bytes). */
void select_entry(s16 keep) {
    ChrEdit *c;
    s16 i;

    c = &gChrEdit;
    if (keep == 0) {
        c->unk00 = 0;
        c->slot = 0;
        c->panel = 0;
        c->unk3 = 0;
        c->gender = 0;
        c->unk5C = 0xFF;
        for (i = 0; i < 0x12; i++) {
            c->name[i] = 0;
        }
        c->unk3A = 0;
        c->color = gPresetColors[c->unk3A];
    }
    c->type = 0;
    c->face = 0;
    c->voice = 0;
    c->hair = 0;
    c->timer = 0;
    c->sex = 0;
    c->sound = 0;
    c->soundWait = 0;
    c->unk44[1] = 0;
    c->unk44[2] = 0;
    c->unk43 = 0x14;
    c->cond[0] = 0;
    c->cond[1] = 0;
    c->cond[2] = 0;
    c->cond[3] = 0;
    c->cond[4] = 0;
    c->cond[5] = 0;
    c->cond[6] = 0;
    c->cond[7] = 0;
}

/* Returns ChrEdit.unk42 (copied from the option byte gSysSave.options.equipView): 0 shows the rotation hint instead of the equipment. */
s32 Chara_EquipViewOn(void) {
    return gChrEdit.unk42 != 0;
}

void Mem_Fill(void *dst, u8 c, s32 n);
u16 Box_AddItem(SaveData *save, u8 type, u16 id, u16 level);
s32 Equip_EquipSlot(SaveData *save, u16 idx);
void Equip_Refresh(SaveData *save);
u32 ItemBox_AddTo(SaveData *s, u16 item, u32 count);
void Save_SetEventFlag(s32);
u16 Save_RankFromPoints(SaveData *s);
void Rate_InitSave(SaveData *save);
s32 sprintf(char *, const char *, ...);    /* sprintf */
void SaveFlag_SetIn(SaveData *save, s32 n);

extern char *D_004475C0;
/* Makes the new character's save record from the character being created
 * (slot 0xFF: gSave itself). */
void Chara_MakeSave(ChrEdit *c, u8 slot) {
    SaveData *s;
    s16 i;
    u8 *q;
    s16 j;

    if (slot == 0xFF) {
        s = &gSave;
    } else {
        s = &gSysSave.save;
    }
    Mem_Fill(s, 0, 0x37F8);
    s->used = 1;
    s->gender = c->gender;
    s->face = c->face;
    s->voice = c->voice;
    if (c->hair == 0x11) {
        s->hairStyle = 0x13;
    } else {
        s->hairStyle = c->hair;
    }
    s->hairColor = c->color;
    for (i = 0; i < 0x12; i++) {
        s->name[i] = c->name[i];
    }
    s->charType = c->type;
    s->money = 0;
    s->equipIdx[1] = 0xFFFF;
    s->equipIdx[2] = 0xFFFF;
    s->equipIdx[3] = 0xFFFF;
    s->equipIdx[4] = 0xFFFF;
    s->equipIdx[5] = 0xFFFF;
    s->unk20D9 = 1;
    s->hrPoints = 0;
    s->hr = 0;
    Equip_EquipSlot(s, (s16)Box_AddItem(s, 6, 0x9C, 0));
    Equip_Refresh(s);
    s->equip[1].type = 0;
    s->equip[2].type = 2;
    s->equip[3].type = 3;
    s->equip[4].type = 4;
    s->equip[5].type = 5;
    if (gOldGameKind != 0) {
        ItemBox_AddTo(s, 7, 0xA);
        ItemBox_AddTo(s, 0xAC, 0x14);
        ItemBox_AddTo(s, 0x30, 0xA);
        ItemBox_AddTo(s, 0x47, 0xA);
        ItemBox_AddTo(s, 0x8E, 5);
        ItemBox_AddTo(s, 0x97, 3);
        ItemBox_AddTo(s, 0x9A, 3);
        s->money += 0xBB8;
        Save_SetEventFlag(0x77);
    }
    s->hr = Save_RankFromPoints(s);
    for (i = 0; i < 0x20; i++) {
        s->randSaved[i] = Rand_Next(1);
    }
    Rate_InitSave(s);
    /* equipSets[j] (idx, then name at +0xC) through a byte walk: the indexed form
     * swaps s1 and s2 (the loop counter and the walk). */
    for (j = 0, q = (u8 *)s; j < 10; j++, q += 0x20) {
        *(u16 *)(q + 0x25E4) = 0xFFFF;
        *(u16 *)(q + 0x25E6) = 0xFFFF;
        *(u16 *)(q + 0x25E8) = 0xFFFF;
        *(u16 *)(q + 0x25EA) = 0xFFFF;
        *(u16 *)(q + 0x25EC) = 0xFFFF;
        *(u16 *)(q + 0x25EE) = 0xFFFF;
        sprintf((char *)(q + 0x25F0), D_004475C0, j + 1);
    }
    SaveFlag_SetIn(s, 0);
    SaveFlag_SetIn(s, 1);
    SaveFlag_SetIn(s, 2);
    SaveFlag_SetIn(s, 3);
}

typedef struct SelVec3 {
    f32 x, y, z;
} SelVec3;

extern SelVec3 gStartPos[];     /* sub_main */
extern s32 gAreaSubModelId[];
extern s32 gSystemTexFiles[];
void DrawCb_LocalPlayer(DrawSlot *r);
void DrawCb_CharMake(DrawSlot *r);
void Player_SetColorWord(Player *p, u8 c);
void Player_InitPartTable(s32 idx);
void Player_LoadPartModels(Player *p);
void Player_ClearFxBlock(Player *p);
void Player_InitRecords(Player *p);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
Model *Model_Get(s32 i);
void Player_LoadWeaponModel(s32 kind, s32 pl);

/* Sets up the player shown on the selection screens (the saved one). */
void Chara_SetupSavedPlayer(Player *p, s32 mode, s32 slot) {
    ChrEdit *c;
    s16 i;
    s16 m;

    m = mode;
    c = &gChrEdit;
    p->active = 1;
    p->visible = 1;
    p->index = slot;
    p->isEm = 0;
    p->weaponShown = 0;
    p->alpha = 1.0f;
    p->motionLayerCount = 2;
    p->scale[0] = 1.0f;
    p->scale[1] = 1.0f;
    p->scale[2] = 1.0f;
    p->motion[0].step = 2.0f;
    p->motion[1].step = 2.0f;
    p->motionSpeed = 1.0f;
    p->weaponOut = 0;
    p->pos[0] = gStartPos[gScene.area].x;
    p->pos[1] = gStartPos[gScene.area].y;
    p->pos[2] = gStartPos[gScene.area].z;
    p->rot[1] = 0.0f;
    p->rot[0] = 0.0f;
    p->rot[2] = 0.0f;
    p->weaponType = 0;
    p->isGunner = 0;
    p->bladeSpecial = 0;
    if (m == 0) {
        p->gender = c->gender;
        p->hairColor = c->color;
        Player_SetColorWord(p, 2);
        for (i = 0; i < 6; i++) {
            p->equip[i].used = 1;
            p->equip[i].type = i;
            p->equip[i].id = 1;
            p->equip[i].level = 0;
            p->equip[i].deco[0] = 0;
            p->equip[i].deco[1] = 0;
            p->equip[i].deco[2] = 0;
        }
        p->lookVariant = 0;
    }
    Player_InitPartTable(p->index);
    p->model = Model_Get(gAreaSubModelId[(s16)slot]);
    Player_InitRecords(p);
    p->drawSlotId = DrawSlot_Alloc();
    if (p->drawSlotId != -1) {
        p->drawSlot = DrawSlot_Get(p->drawSlotId);
        p->drawSlot->owner = (void *) p->index;
        p->drawSlot->draw = DrawCb_CharMake;
    }
}

void Chara_SetVoiceMotion(void *obj, u8 a, u8 b) {
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
    Player_SetMotionPair(obj, ((s16 *)gCharaVoiceMotions)[dir + a * 60 + b * 4], 4, 0);
}

/* ... and the one being created. */
void Chara_SetupNewPlayer(Player *p, s32 mode, s16 slot) {
    ChrEdit *c;
    s16 i;
    s16 m;

    m = mode;
    c = &gChrEdit;
    p->active = 1;
    p->visible = 1;
    p->index = slot;
    p->isEm = 0;
    p->weaponShown = 1;
    p->alpha = 1.0f;
    p->motionLayerCount = 2;
    p->scale[0] = 1.0f;
    p->scale[1] = 1.0f;
    p->scale[2] = 1.0f;
    p->motion[0].step = 2.0f;
    p->motion[1].step = 2.0f;
    p->motionSpeed = 1.0f;
    p->weaponOut = 0;
    p->pos[0] = gStartPos[gScene.area].x;
    p->pos[1] = gStartPos[gScene.area].y;
    p->pos[2] = gStartPos[gScene.area].z;
    p->rot[1] = 0.0f;
    p->rot[0] = 0.0f;
    p->rot[2] = 0.0f;
    if (m == 0) {
        p->gender = c->gender;
        p->hairColor = c->color;
        Player_SetColorWord(p, 2);
        for (i = 0; i < 6; i++) {
            p->equip[i].used = 1;
            p->equip[i].type = i;
            p->equip[i].id = 1;
            p->equip[i].level = 0;
            p->equip[i].deco[0] = 0;
            p->equip[i].deco[1] = 0;
            p->equip[i].deco[2] = 0;
        }
        p->lookVariant = 0;
    }
    Player_InitPartTable(p->index);
    Player_LoadPartModels(p);
    Player_LoadWeaponModel(p->weaponKind, p->index);
    Player_ClearFxBlock(p);
    Player_InitRecords(p);
    p->drawSlotId = DrawSlot_Alloc();
    if (p->drawSlotId != -1) {
        p->drawSlot = DrawSlot_Get(p->drawSlotId);
        p->drawSlot->owner = (void *) p->index;
        p->drawSlot->draw = DrawCb_LocalPlayer;
    }
}

void Chara_DrawBackground(void) {
    Sprite spr;

    Gfx_SetState63(1);
    Tex_Prepare(1, 0xA3);
    Tex_Bind(0xA3);
    Gs_SetState(0x6C, 0);
    spr.x = 0;
    spr.y = 0;
    spr.w = 0x280;
    spr.h = 0x1C0;
    spr.color = -1;
    spr.u0 = 0;
    spr.v0 = 0;
    spr.u1 = 0x1FF;
    spr.v1 = 0x1BF;
    Sprite_Draw(&spr);
    Gs_SetState(0x6C, 1);
}

/* The parts pages' header and help line (the arguments are not used). */
void Chara_DrawPartsHeader(TaskSlot *p, ChrEdit *c) {
    Gs_SetState(0x6C, 0);
    Sel_DrawHeader(4);
    Font_SetSpacing(0x14, 0x14);
    Hud_DrawButtonIconWhite(0x12, 0x206, 0x60, 8, 1.0f);
    Text_PrintfAt(0x220, 0x60, 0, gRotateTexts[0]);
    Gs_SetState(0x6C, 1);
}

void Chara_PlayDecideSe(void) {
    Sound_PlaySeFlat(0, 0x13, 0);
}

void Chara_PlayCancelSe(void) {
    Sound_PlaySeFlat(0, 0x14, 0);
}

const char D_006CDD80[3] = {
#include "select/006CDD80.inc"
};

/* The help text of a part (kind 0: the current one) and its second line.
 * The default is a ternary: an `if` gives an aligned join and other
 * scheduling after it. */
void Chara_DrawPartHelp(ChrEdit *c, u8 kind, u8 alt) {
    char *s;

    WindowFrame_Draw(&gCharaHelpFrame, 0);
    Font_SetSpacing(0x14, 0x14);
    Text_SetColor(0);
    kind = (kind == 0) ? c->panel : kind;
    Font_SetPos((0x280 - Text_DisplayLength(gCharaPartNames[kind]) * 10) / 2, 0x168);
    Text_Printf(D_006CDD80, gCharaPartNames[kind]);
    if (alt == 0) {
        s = gNameEntryHelpTexts[0];
    } else {
        s = gCharaPartHelpTexts[kind];
    }
    Font_SetPos((0x280 - Text_DisplayLength(s) * 10) / 2, 0x180);
    Text_Printf(D_006CDD80, s);
}

void Chara_DrawNoMatch(ChrEdit *c) {
    char *msg;
    s16 x;

    WindowFrame_Draw(&gCharaNoMatchFrame, 0);
    Font_SetSpacing(0x14, 0x14);
    Text_SetColor(0);
    Text_SetSize(2);
    msg = gNoMatchTexts[0];
    x = (0x280 - Text_DisplayLength(msg) * 10) / 2;
    Font_SetPos(x, 0xA2);
    Text_Printf(D_006CDD80, msg);
    msg = gNoMatchTexts[1];
    Font_SetPos(x, 0xBA);
    Text_Printf(D_006CDD80, msg);
    Text_SetSize(0);
}

s32 Chara_NameHasText(ChrEdit *c) {
    s16 i;
    u8 found;

    found = 0;
    for (i = 0; i < 0x10; i++) {
        if (c->name[i] == 0) {
            break;
        }
        if (c->name[i] != 0 && c->name[i] != 0x20) {
            found = 1;
        }
    }
    if (found == 1) {
        return 1;
    }
    return 0;
}

s32 Chara_CheckName(s8 *name);
s16 NetMenu_DrawMessage(s32 x, s16 y, s32 id);
void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 color);
s32 Str_Length(char *);
extern char *gNameInvalidTexts[];  /* messages (select_psp.c) */
extern char *gCharaConfirmTexts[];
extern char *gCharaQuestionTexts[];
extern char *gCharaYesNoTexts[];
extern char *gCharaYesNoTexts2[];
extern char *gGenderBackTexts[];

/* A yes/no question of character creation (mode: which one), with the
 * name's errors (empty, or a listed word) instead for mode 0. */
void Chara_DrawQuestion(ChrEdit *c, s16 mode) {
    u8 err;
    char *msg;
    s16 y;
    s16 x;

    Font_SetSpacing(0x14, 0x14);
    WindowFrame_Draw(&gCharaHelpFrame, 0);
    err = 0;
    if (mode != 4) {
        if (Chara_NameHasText(c) == 0) {
            err = 1;
        } else if (Chara_CheckName(c->name) == 0) {
            err = 2;
        }
    }
    if (err != 0 && mode == 0) {
        if (err == 1) {
            x = (0x280 - Str_Length(gRotateTexts[1]) * 10) / 2;
            Text_PrintfAt(x, 0x168, 0, gRotateTexts[1]);
        } else {
            x = (0x280 - Str_Length(gNameInvalidTexts[0]) * 10) / 2;
            Text_PrintfAt(x, 0x168, 0, gNameInvalidTexts[0]);
        }
        Font_SetPos((0x280 - Text_DisplayLength(gNameInvalidTexts[1]) * 10) / 2, 0x180);
        Text_Printf(gNameInvalidTexts[1]);
        return;
    }
    switch (mode) {
    case 0:
    default:
        msg = gCharaConfirmTexts[0];
        break;
    case 1:
        msg = gCharaConfirmTexts[1];
        break;
    case 2:
        msg = gCharaQuestionTexts[0];
        break;
    case 3:
        msg = gCharaQuestionTexts[1];
        break;
    case 4:
        msg = gGenderBackTexts[0];
        break;
    case 5:
        NetMenu_DrawMessage(0x140, 0x160, 0x30);
        break;
    }
    if (mode != 5) {
        Text_PrintfAt((0x280 - Str_Length(msg) * 10) / 2, 0x168, 0, msg);
        y = 0x184;
    } else {
        y = 0x180;
    }
    if (c->unk3 == 0) {
        Menu_DrawGlowBar(0x10E, y - 2, 0x64, 0x18, 0xFF20C0C0);
        Text_PrintfAt(0xFA, y, 5, gCharaYesNoTexts[0]);
        Text_PrintfAt(0x15E, y, 0, gCharaYesNoTexts[1]);
    } else {
        Menu_DrawGlowBar(0x17C, y - 2, 0x64, 0x18, 0xFF20C0C0);
        Text_PrintfAt(0xFA, y, 0, gCharaYesNoTexts2[0]);
        Text_PrintfAt(0x15E, y, 5, gCharaYesNoTexts2[1]);
    }
}

void Chara_DrawEditHelp(ChrEdit *c, s32 mode) {
    WindowFrame_Draw(&gCharaHelpFrame, 0);
    Font_SetSpacing(0x14, 0x14);
    Text_SetColor(0);
    if ((u16)mode == 0) {
        Text_PrintfAt(0xA0, 0x168, 0, gCharaEditHelpTexts[0]);
        Text_PrintfAt(0xA0, 0x180, 0, gCharaEditHelpTexts[1]);
    } else {
        Font_SetPos((0x280 - Text_DisplayLength(gCharaSelectTexts[0]) * 10) / 2, 0x168);
        Text_Printf(gCharaSelectTexts[0]);
        Font_SetPos((0x280 - Text_DisplayLength(gCharaSelectTexts[1]) * 10) / 2, 0x180);
        Text_Printf(gCharaSelectTexts[1]);
    }
}

void Chara_DrawSexHelp(ChrEdit *c, s32 mode) {
    u8 k;
    s16 x;

    WindowFrame_Draw(&gCharaHelpFrame, 0);
    Font_SetSpacing(0x14, 0x14);
    if ((s16)mode == 0) {
        x = (0x280 - Str_Length(gCardSavingTexts[0]) * 10) / 2;
        Text_PrintfAt(x, 0x168, 0, gCardSavingTexts[0]);
        x = (0x280 - Str_Length(gCardSavingTexts[1]) * 10) / 2;
        Text_PrintfAt(x, 0x180, 2, gCardSavingTexts[1]);
    } else {
        switch (c->sex) {
        case 0:
            k = 0x13;
            break;
        case 1:
            k = 0x12;
            break;
        }
        x = (0x280 - Str_Length(gRotateTexts[k]) * 10) / 2;
        Text_PrintfAt(x, 0x168, 0, gRotateTexts[k]);
    }
}

void Chara_DrawColorEdit(ChrEdit *c) {
    ColorEdit_Draw(0, 0, 0x78, (c->color >> 16) & 0xFF, (c->color >> 8) & 0xFF, c->color & 0xFF, c->unk3, c->unk3A);
}

void Vec_Copy(f32 *dst, f32 *src);          /* copies a vector */
void Em_GetBonePos(void *obj, s16 bone, f32 *out);

/* Points the camera at the character (0: whole body, 1: face, 2: close). */
void Chara_SetCamera(void *obj, s32 mode, s32 hold) {
    ChrEdit *c;

    c = &gChrEdit;
    switch ((s16)mode) {
    case 0:
        Vec_Copy(c->camTarget, (f32 *)((u8 *)obj + 0xAC));
        c->camTarget[0] -= 100.0f;
        c->camTarget[1] += 90.0f;
        Vec_Copy(c->camPos, c->camTarget);
        c->camPos[2] += 300.0f;
        break;
    case 1:
        Em_GetBonePos(obj, 0x14, c->camTarget);
        c->camTarget[0] -= 20.0f;
        Vec_Copy(c->camPos, c->camTarget);
        c->camPos[1] += 15.0f;
        c->camPos[2] += 80.0f;
        break;
    case 2:
        Vec_Copy(c->camTarget, (f32 *)((u8 *)obj + 0xAC));
        c->camTarget[1] += 100.0f;
        Vec_Copy(c->camPos, c->camTarget);
        c->camPos[2] += 260.0f;
        break;
    }
    if ((s16)hold == 0) {
        Vec_Copy(gCamera->at, c->camTarget);
        Vec_Copy(gCamera->eye, c->camPos);
    }
}

void Chara_MoveCursorWrap(ChrEdit *c, u16 pad, u8 *cur, u16 n, u16 se, s32 horiz) {
    u16 prev;
    u16 next;

    if (horiz == 0) {
        prev = 0x800;
        next = 0x400;
    } else {
        prev = 0x2000;
        next = 0x1000;
    }
    if (pad & prev) {
        if (*cur == 0) {
            *cur = n - 1;
        } else {
            *cur -= 1;
        }
        Sound_PlaySeFlat(0, se, 0);
    }
    if (pad & next) {
        if (*cur >= n - 1) {
            *cur = 0;
        } else {
            *cur += 1;
        }
        Sound_PlaySeFlat(0, se, 0);
    }
}

void Chara_MoveCursorClamp(ChrEdit *c, u16 pad, u8 *cur, u16 n, u16 se) {
    if (pad & 0x800) {
        if (*cur > 0) {
            *cur -= 1;
            Sound_PlaySeFlat(0, se, 0);
        } else {
            *cur = 0;
        }
    }
    if (pad & 0x400) {
        if (*cur < n - 1) {
            *cur += 1;
            Sound_PlaySeFlat(0, se, 0);
        } else {
            *cur = n - 1;
        }
    }
}

void Chara_MoveCursorV(ChrEdit *c, u8 n, u16 pad) {
    if (pad & 0x1000) {
        if (c->cursor >= n - 1) {
            c->cursor = 0;
        } else {
            c->cursor++;
        }
        Sound_PlaySeFlat(0, 0x12, 0);
    } else if (pad & 0x2000) {
        if (c->cursor == 0) {
            c->cursor = n - 1;
        } else {
            c->cursor--;
        }
        Sound_PlaySeFlat(0, 0x12, 0);
    }
}

void Chara_CancelToLast(ChrEdit *c, u8 n) {
    if (c->flags & 0x40) {
        if (c->cursor != n - 1) {
            c->cursor = n - 1;
            Chara_PlayCancelSe();
        }
    }
}

s32 Chara_StepCondition(ChrEdit *c, u16 pad, u8 idx, u8 k) {
    u8 n;

    n = (gCharaStepConds[0] + c->gender)[idx * 2];
    if (pad & 0x400) {
        if (c->cond[idx] >= n - 1) {
            c->cond[idx] = 0;
        } else {
            c->cond[idx]++;
        }
        Parts_StartBounce(k + 1);
        return 1;
    }
    if (pad & 0x800) {
        if (c->cond[idx] == 0) {
            c->cond[idx] = n - 1;
        } else {
            c->cond[idx]--;
        }
        Parts_StartBounce(k);
        Sound_PlaySeFlat(0, 0x16, 0);
        return 1;
    }
    return 0;
}

/* The parts of a kind (0 faces, 1 hair styles, 2 voices) that fit the
 * conditions chosen on its page (unk50: three for faces and hair, two for
 * voices; 0 = any): their bits in mask, their count in unk5E. Hair style
 * 0x11 needs the flag at 0x6AA2A8. Static: its callers keep a0 across it. */
static s8 Chara_FilterParts(ChrEdit *c, u8 kind) {
    s16 i;
    u8 *p;
    u8 a;
    u8 b;
    u8 d;
    u8 n;

    c->matchCount = 0;
    c->mask = 0;
    switch (kind) {
    case 0:
        if (c->gender == 0) {
            p = (u8 *)gCharaFaceConds0;
        } else {
            p = (u8 *)gCharaFaceConds1;
        }
        for (i = 0; i < 0x1B; i++) {
            a = *p++;
            b = *p++;
            d = *p++;
            n = 3;
            if (c->cond[0] == 0 || c->cond[0] == a || a == 0) {
                n--;
            }
            if (c->cond[1] == 0 || c->cond[1] == b || b == 0) {
                n--;
            }
            if (c->cond[2] == 0 || c->cond[2] == d || d == 0) {
                n--;
            }
            if (n == 0) {
                c->mask |= 1 << i;
                c->matchCount++;
            }
        }
        break;
    case 1:
        if (c->gender == 0) {
            p = (u8 *)gCharaHairConds0;
        } else {
            p = (u8 *)gCharaHairConds1;
        }
        for (i = 0; i < 0x12; i++) {
            a = *p++;
            b = *p++;
            d = *p++;
            n = 3;
            if (c->cond[3] == 0 || c->cond[3] == a || a == 0) {
                n--;
            }
            if (c->cond[4] == 0 || c->cond[4] == b || b == 0) {
                n--;
            }
            if (c->cond[5] == 0 || c->cond[5] == d || d == 0) {
                n--;
            }
            if (i == 0x11 && gSysSave.unk458 == 0) {
                n = 0x63;
            }
            if (n == 0) {
                c->mask |= 1 << i;
                c->matchCount++;
            }
        }
        break;
    case 2:
        if (c->gender == 0) {
            p = (u8 *)gCharaVoiceConds0;
        } else {
            p = (u8 *)gCharaVoiceConds1;
        }
        for (i = 0; i < 0xF; i++) {
            a = *p++;
            b = *p++;
            n = 2;
            if (c->cond[6] == 0 || c->cond[6] == a || a == 0) {
                n--;
            }
            if (c->cond[7] == 0 || c->cond[7] == b || b == 0) {
                n--;
            }
            if (n == 0) {
                c->mask |= 1 << i;
                c->matchCount++;
            }
        }
        break;
    }
    return c->matchCount;
}

void Chara_ShowNoMatch(ChrEdit *c, s8 v) {
    c->noMatch = v;
    c->unk43 = 0xA;
    Sound_PlaySeFlat(0, 0x15, 0);
}

/* Moves the face to the first one that fits, if it no longer does. */
void Chara_FixFace(ChrEdit *c) {
    s16 i;

    if (c->matchCount != 0 && (c->mask & (1 << c->face)) == 0) {
        for (i = 0; i < 0x1B; i++) {
            if (!(c->mask & (1 << i)) == 0) {
                c->face = i;
                return;
            }
        }
    }
}

/* FACE: the face (cursor 0) and its three conditions; 1 when confirmed. */
s32 Chara_FacePage(ChrEdit *c) {
    u16 pad;

    pad = c->flags | c->unk4E;
    if (c->flags & 0x20) {
        if (c->cursor == 4) {
            if (c->matchCount > 0) {
                return 1;
            }
            Chara_ShowNoMatch(c, 1);
            return 0;
        }
        c->cursor = 4;
    }
    Chara_MoveCursorV(c, 5, pad);
    Chara_CancelToLast(c, 5);
    if (c->flags & 0x200) {
        c->cond[0] = 0;
        c->cond[1] = 0;
        c->cond[2] = 0;
        Chara_FilterParts(c, 0);
        Chara_FixFace(c);
        Sound_PlayCancel();
    }
    switch (c->cursor) {
    case 0:
        if (c->matchCount != 0) {
            if (pad & 0x400) {
                do {
                    if (c->face >= 0x1A) {
                        c->face = 0;
                    } else {
                        c->face++;
                    }
                } while ((c->mask & (1 << c->face)) == 0);
                Parts_StartBounce(1);
            } else if (pad & 0x800) {
                do {
                    if (c->face == 0) {
                        c->face = 0x1A;
                    } else {
                        c->face--;
                    }
                } while ((c->mask & (1 << c->face)) == 0);
                Parts_StartBounce(0);
            }
        }
        break;
    case 1:
        if (Chara_StepCondition(c, pad, 0, 2) == 1) {
            if (Chara_FilterParts(c, 0) == 0) {
                Chara_ShowNoMatch(c, 1);
            } else {
                Chara_FixFace(c);
            }
        }
        break;
    case 2:
        if (Chara_StepCondition(c, pad, 1, 4) == 1) {
            if (Chara_FilterParts(c, 0) == 0) {
                Chara_ShowNoMatch(c, 1);
            } else {
                Chara_FixFace(c);
            }
        }
        break;
    case 3:
        if (Chara_StepCondition(c, pad, 2, 6) == 1) {
            if (Chara_FilterParts(c, 0) == 0) {
                Chara_ShowNoMatch(c, 1);
            } else {
                Chara_FixFace(c);
            }
        }
        break;
    case 4:
        break;
    }
    return 0;
}

/* Moves the hair style to the first one that fits, if it no longer does. */
void Chara_FixHair(ChrEdit *c) {
    s16 i;

    if (c->matchCount != 0 && (c->mask & (1 << c->hair)) == 0) {
        for (i = 0; i < 0x12; i++) {
            if (!(c->mask & (1 << i)) == 0) {
                c->hair = i;
                return;
            }
        }
    }
}

/* HAIR: the hair style (cursor 0) and its three conditions. */
s32 Chara_HairPage(ChrEdit *c) {
    u16 pad;

    pad = c->flags | c->unk4E;
    if (c->flags & 0x20) {
        if (c->cursor == 4) {
            if (c->matchCount > 0) {
                return 1;
            }
            Chara_ShowNoMatch(c, 1);
            return 0;
        }
        c->cursor = 4;
    }
    Chara_MoveCursorV(c, 5, pad);
    Chara_CancelToLast(c, 5);
    if (c->flags & 0x200) {
        c->cond[3] = 0;
        c->cond[4] = 0;
        c->cond[5] = 0;
        Chara_FilterParts(c, 1);
        Chara_FixHair(c);
        Sound_PlayCancel();
    }
    switch (c->cursor) {
    case 0:
        if (c->matchCount != 0) {
            if (pad & 0x400) {
                do {
                    if (c->hair >= 0x11) {
                        c->hair = 0;
                    } else {
                        c->hair++;
                    }
                } while ((c->mask & (1 << c->hair)) == 0);
                Parts_StartBounce(1);
            } else if (pad & 0x800) {
                do {
                    if (c->hair == 0) {
                        c->hair = 0x11;
                    } else {
                        c->hair--;
                    }
                } while ((c->mask & (1 << c->hair)) == 0);
                Parts_StartBounce(0);
            }
        }
        break;
    case 1:
        if (Chara_StepCondition(c, pad, 3, 2) == 1) {
            if (Chara_FilterParts(c, 1) == 0) {
                Chara_ShowNoMatch(c, 1);
            } else {
                Chara_FixHair(c);
            }
        }
        break;
    case 2:
        if (Chara_StepCondition(c, pad, 4, 4) == 1) {
            if (Chara_FilterParts(c, 1) == 0) {
                Chara_ShowNoMatch(c, 1);
            } else {
                Chara_FixHair(c);
            }
        }
        break;
    case 3:
        if (Chara_StepCondition(c, pad, 5, 6) == 1) {
            if (Chara_FilterParts(c, 1) == 0) {
                Chara_ShowNoMatch(c, 1);
            } else {
                Chara_FixHair(c);
            }
        }
        break;
    case 4:
        break;
    }
    return 0;
}

/* Moves the voice to the first one that fits, if it no longer does. */
void Chara_FixVoice(ChrEdit *c) {
    s16 i;

    if (c->matchCount != 0 && (c->mask & (1 << c->voice)) == 0) {
        for (i = 0; i < 0xF; i++) {
            if (!(c->mask & (1 << i)) == 0) {
                c->voice = i;
                return;
            }
        }
    }
}

/* VOICE: the voice (cursor 0, plays it when it changes) and its two
 * conditions. */
s32 Chara_VoicePage(ChrEdit *c) {
    u16 pad;
    u8 old;

    pad = c->flags | c->unk4E;
    if (c->flags & 0x20) {
        if (c->cursor == 3) {
            if (c->matchCount > 0) {
                return 1;
            }
            Chara_ShowNoMatch(c, 1);
            return 0;
        }
        c->cursor = 3;
    }
    Chara_MoveCursorV(c, 4, pad);
    Chara_CancelToLast(c, 4);
    if (c->flags & 0x200) {
        c->cond[6] = 0;
        c->cond[7] = 0;
        Chara_FilterParts(c, 2);
        Chara_FixVoice(c);
        Sound_PlayCancel();
    }
    old = c->voice;
    switch (c->cursor) {
    case 0:
        if (c->matchCount != 0) {
            if (pad & 0x400) {
                do {
                    if (c->voice >= 0xE) {
                        c->voice = 0;
                    } else {
                        c->voice++;
                    }
                } while ((c->mask & (1 << c->voice)) == 0);
                Parts_StartBounce(1);
            } else if (pad & 0x800) {
                do {
                    if (c->voice == 0) {
                        c->voice = 0xE;
                    } else {
                        c->voice--;
                    }
                } while ((c->mask & (1 << c->voice)) == 0);
                Parts_StartBounce(0);
            }
        }
        break;
    case 1:
        if (Chara_StepCondition(c, pad, 6, 2) == 1) {
            if (Chara_FilterParts(c, 2) == 0) {
                Chara_ShowNoMatch(c, 1);
            } else {
                Chara_FixVoice(c);
            }
        }
        break;
    case 2:
        if (Chara_StepCondition(c, pad, 7, 4) == 1) {
            if (Chara_FilterParts(c, 2) == 0) {
                Chara_ShowNoMatch(c, 1);
            } else {
                Chara_FixVoice(c);
            }
        }
        break;
    case 3:
        break;
    }
    if (old != c->voice) {
        c->sound = 0;
        Sound_LoadVoiceBank(c->gender, c->voice);
    }
    return 0;
}

/* Sets up the player object from save slot idx (0x37F8-byte records at
 * gCardSlots) when the slot is used, with the idle motion of its sex.
 * The caller also passes its own record (unused). The pointer is loaded
 * first, then the offset added: `base + i * 0x37F8` in one expression
 * computes the offset first. */
void Chara_LoadSlot(s32 unused, void *obj, s16 i) {
    SaveData *p;

    p = gCardSlots;
    p += i;
    if (p->used != 0) {
        Player_SetupFromSave((struct Player *)obj, p);
        if (((Player *) obj)->gender == 0) {
            Player_SetMotionPair(obj, 1, 0, 0);
        } else {
            Player_SetMotionPair(obj, 0x32B, 0, 0);
        }
    }
}

void Chara_RotateByPad(void) {
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

extern u32 gPresetColors[];      /* sub_main: the preset colours */

/* The colour editor: R, G, B sliders, then the presets. */
s32 Chara_EditColor(ChrEdit *c, u8 *pl, u16 pad) {
    u8 rgb[3];

    if (c->unk3 == 4 && (c->flags & 0x20)) {
        return 1;
    }
    if ((c->flags & 0x40) && c->unk3 != 4) {
        c->unk3 = 4;
        Chara_PlayCancelSe();
    }
    rgb[0] = c->color >> 16;
    rgb[1] = c->color >> 8;
    rgb[2] = c->color;
    if (pad & 0x2000) {
        if (c->unk3 == 0) {
            c->unk3 = 4;
        } else {
            c->unk3--;
        }
        Sound_PlaySeFlat(0, 0x12, 0);
    }
    if (pad & 0x1000) {
        if (c->unk3 >= 4) {
            c->unk3 = 0;
        } else {
            c->unk3++;
        }
        Sound_PlaySeFlat(0, 0x12, 0);
    }
    if (c->unk3 < 3) {
        Chara_MoveCursorClamp(c, c->unk3E, &rgb[c->unk3], 0x100, 0x17);
        c->color = (rgb[0] << 16) | 0xFF000000 | (rgb[1] << 8) | rgb[2];
    } else if (c->unk3 == 3) {
        Chara_MoveCursorWrap(c, pad, &c->unk3A, 0x10, 0x16, 0);
        if (c->flags & 0x20) {
            c->color = gPresetColors[c->unk3A];
            Chara_PlayDecideSe();
        }
    }
    *(u32 *)(pl + 0x580) = c->color;
    return 0;
}

s32 Parts_DrawSides(TaskSlot *p);
void Parts_DrawTypes(s16 x, s16 y, s32 sel);
void Parts_DrawFacePanel(s16 x, s16 y, s32 sel);
void Parts_DrawHairPanel(s16 x, s16 y, s32 sel);
void Parts_DrawVoicePanel(s16 x, s16 y, s32 sel);
void Kbd_SetPos(s32, f32);
void Chat_DrawKbdBox(s32 text);
void Chara_DrawQuestion(ChrEdit *c, s16 mode);
extern char *gCharaPageTexts[];  /* messages (select_psp.c) */

/* Character creation's pages, by the parent task's step. */
void Chara_DrawPages(DrawSlot *ds) {
    TaskSlot *p;
    ChrEdit *c;

    p = ds->owner;
    c = &gChrEdit;
    Gs_SetState(0x60, 0);
    if (p->mode >= 4) {
        if (p->mode < 7) {
            Chara_DrawPartsHeader(p, c);
        } else if (p->mode <= 7 || p->mode >= 13) {
            Hud_DrawButtonIconWhite(0x12, 0x206, 0x60, 8, 1.0f);
            Font_SetSpacing(0x14, 0x14);
            Text_PrintfAt(0x220, 0x60, 0, gCharaPageTexts[0]);
        }
    }
    switch (p->mode) {
    case 3:
        Gs_SetState(0x6C, 0);
        Sel_DrawHeader(4);
        Gs_SetState(0x6C, 1);
        if (Parts_DrawSides(p) == 1) {
            Chara_DrawPartHelp(c, 7, 1);
        }
        break;
    case 4:
        Chara_DrawPartHelp(c, 0, 0);
        break;
    case 5:
        Chara_DrawPartHelp(c, 0, 1);
        switch (c->panel) {
        case 0:
            Kbd_SetPos(0xF0, 75.0f);
            Chat_DrawKbdBox(0xBD6C6CC5);
            break;
        case 1:
            Parts_DrawTypes(0, 0x60, c->type);
            break;
        case 2:
            Parts_DrawFacePanel(0, 0x60, 0);
            break;
        case 4:
            Chara_DrawColorEdit(c);
            break;
        case 5:
            Parts_DrawVoicePanel(0, 0x60, 0);
            break;
        case 3:
            Parts_DrawHairPanel(0, 0x60, 0);
            break;
        }
        break;
    case 6:
        Chara_DrawQuestion(c, 0);
        break;
    case 8:
    case 9:
    case 10:
        Chara_DrawSexHelp(c, 1);
        break;
    case 11:
    case 12:
        Sel_DrawHeader(4);
        Chara_DrawQuestion(c, 3);
        break;
    case 13:
        Sel_DrawHeader(4);
        Chara_DrawQuestion(c, 4);
        break;
    case 0:
    case 1:
    case 2:
    case 7:
        break;
    }
}

void Chara_DrawNoMatchLayer(void) {
    ChrEdit *c = &gChrEdit;

    if (c->noMatch != 0) {
        Chara_DrawNoMatch(c);
    }
}

/* Leaves an edit page: back to the page list (step 4). */
void Chara_ClosePage(TaskSlot *t, ChrEdit *c) {
    t->mode = 4;
    Chara_StartTabAnim(c->panel);
    Chara_SetCamera(gPlayers, gCharaCameraModes[0], 1);
    c->unk43 = 0x14;
    Parts_StartPanel(1);
}

void Gfx_SetBlend(s32, s32);
s32 Sound_LoadSelectBanks(void *task);
u8 SeqA_GetState(void);
void Game_ResetSubsystems(void);
void Tex_LoadSystemPack(s32);
void Sel_LoadCommonTextures(void);
void Tex_LoadFile(s32, s32, s32);
void SeqA_Reset(void);
void Camera_Reset(void);
void CamCtl_Init(void);
void CamCtl_Clear(void);
void Quest_Clear(void);
void Light_InitAll(s32 unused0);
void Area_Enter(u16);
void Loading_Start(void);
void Loading_Stop(void);
void Hud_LoadTextures(void);
void Anim_LoadCommon(s32);
void Area_LoadSubModel(s32 arg0);
void Area_FreeSubModel(void);
void Mem_Compact(void);
void Sound_PlaySeFlatPan(u32 slot, u32 id, s32 prog, s32 pan);
void Sound_PlaySeMusicVol(u32, u32, s32);
void Chara_StartTabAnim2(u8);
void Parts_ResetBounces(void);
void Parts_Init(void);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
s8 Kbd_Run(s8 *, s16, s16);
void Kbd_Cancel(void);
void NetMenu_Open(u8);
u8 NetMenu_Update(void);
s32 Movie_Update(void);
s32 Movie_GetStatus(void);
void Movie_RequestStop(void);
void Movie_Stop(void);
void TaskSlot2_EndCode1(void);
void Task_SetGmSub(void);
void Sound_Silence(void);
void Player_TickTimers(Player *p);
void Player_UpdateHitStop(void *obj);
void Player_UpdateMotionStep(Player *p);
void Players_UpdateMatrices(void);
void LightSet_UpdateAll(void);
void Camera_Update(void);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
s32 OT_AddAt(u32 *table, void *node, s32 idx, s32 n);
void Parts_Animate(void);
s32 Parts_DrawPanels(TaskSlot *t);
void Frame_Draw(void);
extern u8 gSelectDrawHook[];
extern u8 gCharaTextDrawHook[];

extern TaskSlot *D_006A39E8;
extern TaskSlot *D_006A3A0C;
extern void *D_006A39E4;
extern void *D_006A3A04;
/* Character creation, the task: picks the sex (step 3), then the pages
 * (step 4 chooses one, step 5 edits it; L1+R1 randomises the look), the
 * name check and confirmation (6), the save (7), and the opening movie
 * (8-10) before the game starts. Steps 11 and 13 ask to quit or go back. */
void Chara_CreateTask(TaskSlot *t) {
    ChrEdit *c;
    Player *p;
    u16 trig;
    s16 i;
    Player *pl;
    u8 r;
    s32 st;

    c = &gChrEdit;
    p = gPlayers;
    if (gChrEdit.soundWait != 0) {
        c->soundWait--;
    }
    if (c->unk43 != 0) {
        c->unk43--;
        c->flags = 0;
        c->unk4A = 0;
        c->unk4C = 0;
        c->unk4E = 0;
    } else {
        c->flags = gPadState[0].pressed;
        c->unk4A = gPadState[0].prevHeld;
        c->unk4C = gPadState[0].held;
        c->unk4E = gPadState[0].repeat;
    }
    c->unk3E = 0;
    if (c->unk4A == c->unk4C) {
        c->unk40++;
        if (c->unk40 > 10) {
            c->unk40 = 10;
            c->unk3E = c->unk4C;
        }
    } else {
        c->unk40 = 0;
    }
    c->unk3E |= c->flags;
    trig = c->flags | c->unk4E;
    if (c->noMatch != 0) {
        if (c->flags & 0x60) {
            c->noMatch = 0;
            Chara_PlayDecideSe();
        }
        c->flags = 0;
        c->unk4A = 0;
        c->unk4C = 0;
        c->unk4E = 0;
    }
    Gfx_SetBlend(4, 5);
    Text_SetSize(0);
    switch (t->mode) {
    case 0:
        t->mode++;
        Game_ResetSubsystems();
        Tex_LoadSystemPack(2);
        Sel_LoadCommonTextures();
        Tex_LoadFile(0xC, 0x16B, 0);
        SeqA_Reset();
        select_entry(0);
        Camera_Reset();
        CamCtl_Init();
        CamCtl_Clear();
        D_006A39E8 = t;
        D_006A39E4 = Chara_DrawPages;
        D_006A3A04 = Chara_DrawNoMatchLayer;
        D_006A3A0C = t;
    case 1:
        if (Sound_LoadSelectBanks(t) != 0) {
            t->mode++;
            SeqA_Start(2);
        }
        break;
    case 2:
        if (SeqA_GetState() != 1) {
            t->mode++;
            c->gender = 0;
            c->voice = 0;
            Parts_Init();
            c->unk43 = 0x1E;
        }
        break;
    case 3:
        if ((c->flags & 0x800) && c->gender == 1) {
            c->gender = 0;
            c->unk43 = 3;
            Sound_PlaySeFlatPan(0, 0x12, 0, c->gender * 100 - 50);
        } else if ((c->flags & 0x400) && c->gender == 0) {
            c->gender = 1;
            c->unk43 = 3;
            Sound_PlaySeFlatPan(0, 0x12, 0, c->gender * 100 - 50);
        } else if (c->flags & 0x20) {
            t->mode++;
            if (c->unk5C != c->gender) {
                select_entry(1);
                Sound_LoadVoiceBank(c->gender, c->voice);
                gScene.area = 0;
                gScene.questId = 0;
                Quest_Clear();
                Light_InitAll(gScene.area);
                Area_Enter(gScene.area);
                gSysSettings.soundLoading = 1;
                Sound_PlaySeFlat(1, 9, 0);
                Loading_Start();
                if (c->unk5C == 0xFF) {
                    Hud_LoadTextures();
                    Anim_LoadCommon(4);
                    Area_LoadSubModel(c->gender);
                    Chara_SetupSavedPlayer(p, 0, 0);
                } else {
                    Area_FreeSubModel();
                    Mem_Compact();
                    Area_LoadSubModel(c->gender);
                }
                gSysSettings.soundLoading = 0;
                Loading_Stop();
                c->unk5C = c->gender;
                gPlayers[0].visible = 1;
                gPlayers[0].gender = c->gender;
                pl = gPlayers;
                if (c->gender == 0) {
                    Player_SetMotionPair(pl, 1, 0, 0);
                } else {
                    Player_SetMotionPair(pl, 0x32B, 0, 0);
                }
                Chara_SetCamera(gPlayers, 0, 0);
                gCamera->fov = 0.87266463f;
                gCamera->roll = 0.0f;
            }
            Sound_PlaySeMusicVol(6, 0, 0);
            Sound_PlaySeMusicVol(6, 1, 0);
            Chara_StartTabAnim(0xFF);
        } else if (c->flags & 0x40) {
            Sound_PlayCancel();
            t->mode = 11;
            c->unk3 = 1;
        }
        break;
    case 4:
        Chara_RotateByPad();
        if (c->flags & 0x20) {
            if (c->panel == 6) {
                Chara_PlayDecideSe();
                t->mode = 6;
                c->unk3 = 1;
                Chara_SetCamera(gPlayers, gCharaCameraModes[0], 1);
            } else {
                t->mode++;
                Chara_PlayDecideSe();
                Chara_SetCamera(gPlayers, gCharaCameraModes[c->panel], 1);
                Chara_StartTabAnim2(c->panel);
                c->unk43 = 0x14;
                c->cursor = 0;
                Parts_ResetBounces();
                Parts_StartPanel(0);
                switch (c->panel) {
                case 0:
                    Chat_OpenKbd(3, 0xF, 8, c->name);
                    break;
                case 4:
                    break;
                case 2:
                    Chara_FilterParts(c, 0);
                    break;
                case 3:
                    Chara_FilterParts(c, 1);
                    break;
                case 5:
                    Chara_FilterParts(c, 2);
                    break;
                }
            }
        } else if (c->flags & 0x40) {
            Sound_PlayCancel();
            t->mode = 13;
            c->unk3 = 1;
        } else if (c->flags & 0x2000) {
            if (c->panel == 0) {
                c->panel = 6;
            } else {
                c->panel--;
            }
            Sound_PlaySeFlat(1, 0xC, 0);
        } else if (c->flags & 0x1000) {
            if (c->panel >= 6) {
                c->panel = 0;
            } else {
                c->panel++;
            }
            Sound_PlaySeFlat(1, 0xC, 0);
        } else if (((c->unk4C & 0x4000) && (c->flags & 0x8000)) ||
                   ((c->flags & 0x4000) && (c->unk4C & 0x8000))) {
            c->type = Rand_Next(1) & 3;
            c->face = Rand_Next(1) % 27;
            c->color = gPresetColors[Rand_Next(1) & 0xF];
            c->hair = Rand_Next(1) % 17;
            c->voice = Rand_Next(1) % 15;
            Sound_LoadVoiceBank(c->gender, c->voice);
            if (c->gender == 0) {
                c->cond[0] = gCharaFaceConds0[c->face][0];
                c->cond[1] = gCharaFaceConds0[c->face][1];
                c->cond[2] = gCharaFaceConds0[c->face][2];
                c->cond[3] = gCharaHairConds0[c->hair][0];
                c->cond[4] = gCharaHairConds0[c->hair][1];
                c->cond[5] = gCharaHairConds0[c->hair][2];
                c->cond[6] = gCharaVoiceConds0[c->voice][0];
                c->cond[7] = gCharaVoiceConds0[c->voice][1];
            } else {
                c->cond[0] = gCharaFaceConds1[c->face][0];
                c->cond[1] = gCharaFaceConds1[c->face][1];
                c->cond[2] = gCharaFaceConds1[c->face][2];
                c->cond[3] = gCharaHairConds1[c->hair][0];
                c->cond[4] = gCharaHairConds1[c->hair][1];
                c->cond[5] = gCharaHairConds1[c->hair][2];
                c->cond[6] = gCharaVoiceConds1[c->voice][0];
                c->cond[7] = gCharaVoiceConds1[c->voice][1];
            }
            for (i = 0; i < 8; i++) {
                if (c->cond[i] == 99) {
                    c->cond[i] = 0;
                }
            }
        }
        break;
    case 5:
        Chara_RotateByPad();
        switch (c->panel) {
        case 0:
            if (Kbd_Run(c->name, c->unk4C, c->flags) != 0) {
                Kbd_Cancel();
                Chara_ClosePage(t, c);
            }
            break;
        case 1:
            if (c->flags & 0x20) {
                Chara_PlayDecideSe();
                Chara_ClosePage(t, c);
            } else {
                Chara_MoveCursorWrap(c, trig, &c->type, 4, 0x12, 1);
            }
            break;
        case 2:
            if (Chara_FacePage(c) == 1) {
                Chara_PlayDecideSe();
                Chara_ClosePage(t, c);
            }
            break;
        case 3:
            if (Chara_HairPage(c) == 1) {
                Chara_PlayDecideSe();
                Chara_ClosePage(t, c);
            }
            break;
        case 4:
            if (Chara_EditColor(c, (u8 *)gPlayers, trig) == 1) {
                Chara_PlayDecideSe();
                Chara_ClosePage(t, c);
            }
            break;
        case 5:
            if (Chara_VoicePage(c) == 1) {
                Chara_PlayDecideSe();
                Chara_ClosePage(t, c);
            } else if ((c->flags & 0x100) && c->soundWait == 0) {
                c->soundWait = 5;
                Sound_PlaySeFlat(2, c->sound, 0);
                c->sound++;
                if (c->sound >= 12) {
                    c->sound = 0;
                }
            }
            break;
        }
        break;
    case 6:
        Chara_RotateByPad();
        if (Chara_NameHasText(c) == 0 || Chara_CheckName(c->name) == 0) {
            if (c->flags & 0x60) {
                t->mode = 4;
                Chara_PlayCancelSe();
            }
        } else if ((c->flags & 0x20) && c->unk3 == 0) {
            Sound_PlaySeFlat(1, 6, 0);
            c->slot = 0;
            t->mode++;
            NetMenu_Open(3);
        } else if (((c->flags & 0x20) && c->unk3 == 1) || (c->flags & 0x40)) {
            t->mode = 4;
            Chara_PlayCancelSe();
        } else {
            if ((trig & 0x800) && c->unk3 != 0) {
                Sound_PlaySeFlat(0, 0x12, 0);
                c->unk3 = 0;
            }
            if ((trig & 0x400) && c->unk3 == 0) {
                Sound_PlaySeFlat(0, 0x12, 0);
                c->unk3 = 1;
            }
        }
        break;
    case 7:
        Chara_RotateByPad();
        r = NetMenu_Update();
        if (r != 0) {
            if (r == 2) {
                c->sex = 0;
                Sound_PlaySeFlat(1, 4, 0);
            } else {
                c->sex = 1;
            }
            Chara_MakeSave(c, 0xFF);
            t->mode++;
            gQuestSync.charIndex = c->slot;
            c->timer = 0;
            Chara_SetCamera(gPlayers, 2, 1);
            for (i = 0, p = gPlayers; i < 2; i++, p++) {
                p->rot[1] = 0;
                Chara_SetVoiceMotion(p, c->gender, c->voice);
            }
            Sound_PlaySeMusicVol(1, 2, 0);
            Sound_PlaySeMusicVol(1, 3, 0);
        }
        break;
    case 8:
        if (++c->timer >= 0x3C) {
            t->mode++;
            SeqA_Start(5);
        }
        break;
    case 9:
        if (SeqA_GetState() != 1) {
            t->mode++;
            t->step = 0;
            c->timer = 10;
        }
        break;
    case 10:
        switch (t->step) {
        case 0:
            if (--c->timer <= 0) {
                Game_ResetSubsystems();
                SeqA_Start(2);
                goto end;
            }
            break;
        case 1:
            Movie_Update();
            st = Movie_GetStatus();
            if (st != 3) {
                c->timer++;
                if (st != 2 || !(gPadState[0].pressed & 0x8000)) {
                    break;
                }
            }
            SeqA_Start(1);
            t->step++;
            c->timer = 150;
            Movie_RequestStop();
            break;
        case 2:
            Movie_Update();
            if (SeqA_GetState() != 1) {
                if (Movie_GetStatus() != 0) {
                    goto check;
                }
            stop:
                {
                    Movie_Stop();
                end:
                    TaskSlot_End(t);
                    gSysSettings.online = 0;
                    gSysSettings.unk03 = 1;
                    gSysSettings.unk02 = 0;
                    gSysSettings.unk05 = 0;
                    gSysSettings.useSavedEquip = 0;
                    gSysSettings.galleryPlay = 0;
                    gSysSettings.unk45 = 0;
                    TaskSlot2_EndCode1();
                    SeqA_Start(2);
                    return;
                }
            check:
                if (c->timer == 0) {
                    goto stop;
                }
                c->timer--;
            }
            break;
        }
        break;
    case 11:
        if ((c->flags & 0x20) && c->unk3 == 0) {
            t->mode++;
            Chara_PlayCancelSe();
            Sound_PlaySeMusicVol(1, 2, 0);
            Sound_PlaySeMusicVol(1, 3, 0);
            SeqA_Start(1);
        } else if (((c->flags & 0x20) && c->unk3 == 1) || (c->flags & 0x40)) {
            t->mode = 3;
            Chara_PlayCancelSe();
        } else {
            if ((trig & 0x800) && c->unk3 != 0) {
                Sound_PlaySeFlat(0, 0x12, 0);
                c->unk3 = 0;
            }
            if ((trig & 0x400) && c->unk3 == 0) {
                Sound_PlaySeFlat(0, 0x12, 0);
                c->unk3 = 1;
            }
        }
        break;
    case 12:
        if (SeqA_GetState() != 1) {
            TaskSlot_End(t);
            Task_SetGmSub();
            SeqA_Start(2);
            return;
        }
        break;
    case 13:
        Chara_RotateByPad();
        if ((c->flags & 0x20) && c->unk3 == 0) {
            t->mode = 3;
            Sound_Silence();
            Chara_PlayCancelSe();
        } else if (((c->flags & 0x20) && c->unk3 == 1) || (c->flags & 0x40)) {
            t->mode = 4;
            Chara_PlayCancelSe();
        } else {
            if ((trig & 0x800) && c->unk3 != 0) {
                Sound_PlaySeFlat(0, 0x12, 0);
                c->unk3 = 0;
            }
            if ((trig & 0x400) && c->unk3 == 0) {
                Sound_PlaySeFlat(0, 0x12, 0);
                c->unk3 = 1;
            }
        }
        break;
    }
    pl = gPlayers;
    if (t->mode >= 4 && pl->active != 0) {
        Player_TickTimers(pl);
        Player_UpdateHitStop(pl);
        Player_UpdateMotionStep(pl);
    }
    Players_UpdateMatrices();
    LightSet_UpdateAll();
    c->unk36++;
    gCamera->at[0] += (c->camTarget[0] - gCamera->at[0]) / 8.0f;
    gCamera->at[1] += (c->camTarget[1] - gCamera->at[1]) / 8.0f;
    gCamera->at[2] += (c->camTarget[2] - gCamera->at[2]) / 8.0f;
    gCamera->eye[0] += (c->camPos[0] - gCamera->eye[0]) / 8.0f;
    gCamera->eye[1] += (c->camPos[1] - gCamera->eye[1]) / 8.0f;
    gCamera->eye[2] += (c->camPos[2] - gCamera->eye[2]) / 8.0f;
    Camera_Update();
    if (t->mode != 10) {
        if (t->mode >= 4 && t->mode != 11 && t->mode != 12) {
            pl = gPlayers;
            if (gPlayers[0].active != 0) {
            pl->drawSlot->pos[0] = pl->pos[0];
            pl->drawSlot->pos[1] = pl->pos[1];
            pl->drawSlot->pos[2] = pl->pos[2];
            OT_AddByDepth(gDrawOt32, pl->drawSlot, 0x20, 0);
            }
        }
        Parts_Animate();
        OT_AddAt(gDrawOtSorted, gSelectDrawHook, 0, 0x40);
        OT_AddAt(gDrawListText2, gCharaTextDrawHook, 0, 1);
        Chara_DrawBackground();
        if (t->mode >= 4 && t->mode < 7) {
            Parts_DrawPanels(t);
        }
        Frame_Draw();
    }
}

/* The header over character creation (c is not used). */
void Chara_DrawHeader(ChrEdit *c) {
    Gs_SetState(0x6C, 0);
    Sel_DrawBackground(0xFF);
    Gs_SetState(0x6C, 1);
    Sel_DrawHeader(5);
}

void Chara_DrawIcon(s16 x, s16 y, s16 kind, s16 idx, s32 alpha) {
    Sprite spr;

    Gfx_SetState63(1);
    Tex_Prepare(1, 8);
    Tex_Bind(8);
    spr.x = x - 0x26;
    spr.y = y - 7;
    spr.w = 0x20;
    spr.h = 0x20;
    spr.color = Equip_Color(alpha, 0xFF, 0);
    if (kind == 0) {
        spr.u0 = D_006C8050[idx][0];
        spr.v0 = D_006C8050[idx][1];
    } else {
        spr.u0 = D_006C8070[idx][0];
        spr.v0 = D_006C8070[idx][1];
    }
    spr.u1 = spr.u0 + 0x18;
    spr.v1 = spr.v0 + 0x18;
    Sprite_Draw(&spr);
}

/* An equipment icon (kind 0: a weapon class; otherwise an armour piece,
 * through gCharaIconUV) from the 32x32 icon sheet. u, u0, then v, v0: this
 * order gives the game's pairwise schedule of the texture coordinates. */
void Chara_DrawEquipIcon(s16 x, s16 y, s16 kind, s16 idx, s32 alpha) {
    Sprite spr;
    s32 cell;
    s32 u;
    s32 v;

    if (kind == 0) {
        cell = idx + 0x3F;
    } else {
        cell = ((u8 *)gCharaIconUV)[idx] + 0x4A;
    }
    Gfx_SetState63(1);
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    spr.x = x - 0x26;
    spr.y = y - 7;
    spr.w = 0x20;
    spr.h = 0x20;
    u = (cell & 7) * 32;
    spr.u0 = u + 1;
    v = (cell >> 3) * 32;
    spr.v0 = v + 1;
    spr.u1 = u + 0x1F;
    spr.v1 = v + 0x1F;
    spr.color = Equip_Color(alpha, 0xFF, 0);
    Sprite_Draw(&spr);
}


u8 Equip_GetRarity(u8 type, u16 id);      /* an item's rarity */
s32 Rarity_Color(u8 rarity);             /* ... and its colour; unit's own declaration: the definition (sub_main unk_3549F0.c) returns s16; with it Chara_DrawEquipment differs (159 of 217 words) */
u8 Weapon_GetClass(u8 type, u16 id);
extern char *gGunNames[];    /* item names by kind */
extern char *gBladeNames[];
extern char *gArmorHeadNames[];
extern char *gArmorBodyNames[];
extern char *gArmorArmsNames[];
extern char *gArmorWaistNames[];
extern char *gArmorLegsNames[];

/* The saved character's equipment: the weapon and the five armour pieces,
 * each with its rarity colour and icon. The slot index is read from the
 * global gChrEdit (the parameter is not used), before the window setup. */
void Chara_DrawEquipment(ChrEdit *unused, s32 mode) {
    u8 r;
    SaveData *save;
    Frame win;

    save = gCardSlots;
    save += gChrEdit.slot;
    win.x = 0x15E;
    win.w = 0xF0;
    win.y = 0x58;
    win.h = 0xF0;
    win.style = 1;
    win.color = -1;
    win.color2 = 0xC0402010;
    WindowFrame_Draw(&win, 0);
    Font_SetSpacing(0x12, 0x12);
    if (save->equip[0].type == 7) {
        r = Equip_GetRarity(7, save->equip[0].id);
        Text_PrintfAt(0x186, 0x6E, Rarity_Color(r), D_006CDD80, gGunNames[save->equip[0].id]);
    } else {
        r = Equip_GetRarity(6, save->equip[0].id);
        Text_PrintfAt(0x186, 0x6E, Rarity_Color(r), D_006CDD80, gBladeNames[save->equip[0].id]);
    }
    Chara_DrawEquipIcon(0x186, 0x6E, 0, Weapon_GetClass(save->equip[0].type, save->equip[0].id), r);
    r = Equip_GetRarity(2, save->equip[2].id);
    Text_PrintfAt(0x186, 0x91, Rarity_Color(r), D_006CDD80, gArmorHeadNames[save->equip[2].id]);
    Chara_DrawIcon(0x186, 0x91, 0, 0, r);
    r = Equip_GetRarity(3, save->equip[3].id);
    Text_PrintfAt(0x186, 0xB4, Rarity_Color(r), D_006CDD80, gArmorBodyNames[save->equip[3].id]);
    Chara_DrawIcon(0x186, 0xB4, 0, 1, r);
    r = Equip_GetRarity(4, save->equip[4].id);
    Text_PrintfAt(0x186, 0xD7, Rarity_Color(r), D_006CDD80, gArmorArmsNames[save->equip[4].id]);
    Chara_DrawIcon(0x186, 0xD7, 0, 2, r);
    r = Equip_GetRarity(5, save->equip[5].id);
    Text_PrintfAt(0x186, 0xFA, Rarity_Color(r), D_006CDD80, gArmorWaistNames[save->equip[5].id]);
    Chara_DrawIcon(0x186, 0xFA, 0, 3, r);
    r = Equip_GetRarity(0, save->equip[1].id);
    Text_PrintfAt(0x186, 0x11D, Rarity_Color(r), D_006CDD80, gArmorLegsNames[save->equip[1].id]);
    Chara_DrawIcon(0x186, 0x11D, 0, 4, r);
}

void Chara_DrawQuestion(ChrEdit *c, s16 mode);
void Chara_DrawEquipment(ChrEdit *c, s32 mode);
s32 func_8FC080(void);
s32 CharaSel_IsIdle(void);
struct NetMenu;
void CardMenu_DrawSlots(struct NetMenu *c, s32 mode, u8 n, s32 unused, s32 flag);
s16 NetMenu_DrawMessageY(s16 y, s32 id);
extern char *gCharaPageTexts[];  /* messages (select_psp.c) */

/* Character creation's help and prompts, by the parent task's step. */
void Chara_DrawPrompts(DrawSlot *ds) {
    TaskSlot *p;
    ChrEdit *c;

    p = ds->owner;
    c = &gChrEdit;
    Gs_SetState(0x60, 0);
    if (p->mode < 5) {
        if (p->mode == 4) {
            Chara_DrawQuestion(c, 2);
        }
        if (p->mode == 3) {
            Chara_DrawEditHelp(c, 1);
        }
        if (p->mode >= 2) {
            if (func_8FC080() == 1) {
                if (CharaSel_IsIdle() != 0) {
                    CardMenu_DrawSlots((struct NetMenu *)c, 1, p->mode, 0, 1);
                    if (Chara_EquipViewOn() == 0) {
                        Hud_DrawButtonIconWhite(0x12, 0x206, 0x60, 8, 1.0f);
                        Font_SetSpacing(0x14, 0x14);
                        Text_PrintfAt(0x220, 0x60, 0, gCharaPageTexts[0]);
                    } else {
                        Chara_DrawEquipment(c, 1);
                    }
                }
            } else {
                CardMenu_DrawSlots((struct NetMenu *)c, 1, p->mode, 0, 1);
                if (Chara_EquipViewOn() == 0) {
                    Hud_DrawButtonIconWhite(0x12, 0x206, 0x60, 8, 1.0f);
                    Font_SetSpacing(0x14, 0x14);
                    Text_PrintfAt(0x220, 0x60, 0, gCharaPageTexts[0]);
                } else {
                    Chara_DrawEquipment(c, 1);
                }
            }
        }
    }
    if (p->mode == 5) {
        NetMenu_DrawMessageY(0x164, 0x28);
        WindowFrame_Draw(&gCharaHelpFrame, 0);
        if (!((gChrEdit.timer / 15) & 1)) {
            Hud_DrawButtonIconWhite(0x14, 0x232, 0x184, 8, 1.0f);
        }
    }
}

void Game_ResetSubsystems(void);
void Tex_LoadSystemPack(s32);
void Sel_LoadCommonTextures(void);
s32 Tex_LoadPack(s32 id, s32 slot, s32 kind);
void CharaSel_Start(void);
void Camera_Reset(void);
void CamCtl_Clear(void);
void Quest_Clear(void);
void Light_InitAll(s32 unused0);
void Area_Enter(u16);
void NetMenu_Open(u8);
u8 NetMenu_Update(void);
void Sound_PlaySeMusicVol(u32, u32, s32);
void Task_SetBootMode1(void);
void Task_SetGmSub(void);
void Sel_FreeTextures(void);
void Tex_FreeRange(s32, s32);
s32 Card_ReadyCount(void);
void Loading_Start(void);
void Anim_LoadCommon(s32);
s32 Sound_LoadSelectBanks(void *task);
void Loading_Stop(void);
void Sound_PlaySe72(void);
void CharaSel_Leave(void);
void Save_LoadSlot(u8);
s32 CharaSel_IsDone(void);
u8 SeqA_GetState(void);
void Task_SetBootMode2(void);
void Chara_Frame(TaskSlot *p);
void Gfx_SetBlend(s32, s32);
s32 Card_SlotReady(s32 slot);

/* Character creation and selection, the task: loads the three save
 * slots' characters, picks one (left/right, skipping empty slots), asks
 * to confirm, then starts the game. */
/* One u16 local holds the pad's repeat bits and, in step 0, the area id
 * (the game's saved registers come from that reuse); the save slot address is
 * read, then advanced by the slot. */
void Chara_SelectTask(TaskSlot *t) {
    ChrEdit *c;
    Player *p;
    s32 trig;
    SaveData *save;
    s16 i;
    Player *pl;
    s16 r;
    u8 start;
    u8 old;
    u16 rep;

    c = &gChrEdit;
    p = gPlayers;
    trig = gPadState[0].pressed;
    rep = trig | gPadState[0].repeat;
    (gScene.frame)++;
    Gfx_SetBlend(4, 5);
    switch (t->mode) {
    case 0:
        t->mode++;
        Game_ResetSubsystems();
        Tex_LoadSystemPack(0);
        if (func_8FC080() == 1) {
            Sel_LoadCommonTextures();
            Tex_LoadPack(gSystemTexFiles[1], 0x144, 0);
            CharaSel_Start();
        }
        SeqA_Start(2);
        select_entry(0);
        D_006A39E8 = t;
        D_006A39E4 = Chara_DrawPrompts;
        Camera_Reset();
        CamCtl_Clear();
        gScene.area = 0;
        gScene.questId = 0;
        Quest_Clear();
        Light_InitAll(gScene.area);
        rep = gScene.area;
        gSysSettings.soundLoading = 1;
        Area_Enter(rep);
        gSysSettings.soundLoading = 0;
        gCamera->fov = 0.87266463f;
        gCamera->roll = 0.0f;
        NetMenu_Open(4);
        break;
    case 1:
        r = NetMenu_Update();
        if (r == 0) {
            break;
        }
        if (r == 2) {
            Sound_PlaySeMusicVol(1, 2, 0);
            Sound_PlaySeMusicVol(1, 3, 0);
            if (gSysSettings.pspCharaWait != 0) {
                gSysSettings.pspCharaWait = 0;
                TaskSlot_End(TaskSlot_Get(5));
                Task_SetBootMode1();
            } else {
                TaskSlot_End(t);
                Task_SetGmSub();
            }
            if (func_8FC080() == 1) {
                Sel_FreeTextures();
                Tex_FreeRange(0x144, 1);
            }
            return;
        }
        c->unk42 = gSysSave.options.equipView;
        c->slot = 0xFF;
        if (Card_ReadyCount() != 0 && Chara_EquipViewOn() == 0) {
            Loading_Start();
        }
        save = gCardSlots;
        for (i = 0; i < 3; i++) {
            p->rot[1] = 0;
            if (save->used == 0) {
                p->active = 0;
                p->pos[0] = gStartPos[gScene.area].x;
                p->pos[1] = gStartPos[gScene.area].y;
                p->pos[2] = gStartPos[gScene.area].z;
            } else {
                if (c->slot == 0xFF) {
                    c->slot = i;
                }
                if (Card_SlotReady(i) == 1) {
                    Chara_LoadSlot((s32)c, p, i);
                    if (Chara_EquipViewOn() == 0) {
                        Chara_SetupNewPlayer(p, 1, i);
                        p->visible = 0;
                    }
                } else {
                    p->pos[0] = gStartPos[gScene.area].x;
                    p->pos[1] = gStartPos[gScene.area].y;
                    p->pos[2] = gStartPos[gScene.area].z;
                }
            }
            p++;
            save++;
        }
        start = gSysSave.lastSlot;
        i = start;
        while (1) {
            if (gCardSlots[i].used == 0) {
                i++;
                if (i >= 3) {
                    i = 0;
                }
                if (i == start) {
                    break;
                }
            } else {
                gSysSave.lastSlot = i;
                break;
            }
        }
        c->slot = gSysSave.lastSlot;
        Chara_SetCamera(&gPlayers[c->slot], 0, 0);
        if (Chara_EquipViewOn() == 0) {
            Anim_LoadCommon(4);
        }
        if (func_8FC080() == 1) {
            CharaSel_Start();
        }
        t->mode++;
    case 2:
        if (Sound_LoadSelectBanks(t) == 0) {
            break;
        }
        t->mode++;
        t->step = 0;
        save = gCardSlots;
        save += c->slot;
        if (Card_SlotReady(c->slot) == 1 && save->used != 0) {
            Sound_LoadVoiceBank(c->gender, c->voice);
            c->panel = c->slot;
            if (Chara_EquipViewOn() == 0) {
                p->visible = 1;
            }
        }
        Loading_Stop();
        Sound_PlaySeMusicVol(6, 0, 0);
        Sound_PlaySeMusicVol(6, 1, 0);
        Sound_PlaySeFlat(0, 0x19, 0);
        Chara_EquipViewOn();
        c->timer = 1;
    case 3:
        if (c->timer > 0) {
            c->timer--;
            goto check;
        }
        if (func_8FC080() == 1 && CharaSel_IsIdle() == 0) {
            goto check;
        }
        save = gCardSlots;
        save += c->slot;
        if (Card_SlotReady(c->slot) == 1 && save->used != 0 && Chara_EquipViewOn() == 0) {
            Chara_RotateByPad();
        }
        if ((trig & 0x20) && Card_SlotReady(c->slot) == 1) {
            t->mode++;
            c->unk3 = 0;
            Chara_PlayDecideSe();
            break;
        }
        if (trig & 0x40) {
            Chara_PlayCancelSe();
            Sound_PlaySeMusicVol(1, 2, 0);
            Sound_PlaySeMusicVol(1, 3, 0);
            TaskSlot_End(t);
            if (gSysSettings.pspCharaWait != 0) {
                gSysSettings.pspCharaWait = 0;
                TaskSlot_End(TaskSlot_Get(5));
                Task_SetBootMode1();
            } else {
                Task_SetGmSub();
                if (func_8FC080() == 1) {
                    Sel_FreeTextures();
                    Tex_FreeRange(0x144, 1);
                }
            }
            return;
        }
        if (trig & 0x2000) {
            old = c->slot;
            do {
                if (c->slot == 0) {
                    c->slot = 2;
                } else {
                    c->slot--;
                }
                save = gCardSlots;
        save += c->slot;
            } while (Card_SlotReady(c->slot) != 0 && save->used == 0);
            if (old != c->slot) {
                Sound_PlaySe72();
                if (Chara_EquipViewOn() == 0) {
                    *(s32 *)((u8 *)&gPlayers[0].unk53C[4] + c->slot * 0x970) = 0x01000000;
                    *(s32 *)((u8 *)&gPlayers[0].unk53C[8] + c->slot * 0x970) = 0x01000000;
                    *(s32 *)((u8 *)&gPlayers[0].unk53C[12] + c->slot * 0x970) = 0x01000000;
                }
            }
        } else if (trig & 0x1000) {
            old = c->slot;
            do {
                if (c->slot >= 2) {
                    c->slot = 0;
                } else {
                    c->slot++;
                }
                save = gCardSlots;
        save += c->slot;
            } while (Card_SlotReady(c->slot) != 0 && save->used == 0);
            if (old != c->slot) {
                Sound_PlaySe72();
                if (c->slot != 0xFF && Chara_EquipViewOn() == 0) {
                    *(s32 *)((u8 *)&gPlayers[0].unk53C[4] + c->slot * 0x970) = 0x01000000;
                    *(s32 *)((u8 *)&gPlayers[0].unk53C[8] + c->slot * 0x970) = 0x01000000;
                    *(s32 *)((u8 *)&gPlayers[0].unk53C[12] + c->slot * 0x970) = 0x01000000;
                }
            }
        }
    check:
        if (c->panel != c->slot) {
            c->panel = c->slot;
        }
        break;
    case 4:
        if (Chara_EquipViewOn() == 0) {
            Chara_RotateByPad();
        }
        if ((trig & 0x20) && c->unk3 == 0) {
            if (func_8FC080() == 1) {
                CharaSel_Leave();
            }
            t->mode++;
            Chara_PlayDecideSe();
            gSysSave.lastSlot = c->slot;
            pl = &gPlayers[c->slot];
            Chara_SetCamera(pl, 2, 1);
            pl->rot[1] = 0;
            c->timer = 0;
            gQuestSync.charIndex = c->slot;
            Save_LoadSlot(c->slot);
        } else if (((trig & 0x20) && c->unk3 == 1) || (trig & 0x40)) {
            Chara_PlayCancelSe();
            t->mode = 3;
        } else if ((rep & 0x800) && c->unk3 != 0) {
            Sound_PlaySeFlat(0, 0x12, 0);
            c->unk3 = 0;
        } else if ((rep & 0x400) && c->unk3 == 0) {
            Sound_PlaySeFlat(0, 0x12, 0);
            c->unk3 = 1;
        }
        break;
    case 5:
        c->timer++;
        if (trig & 0x20) {
            gSysSettings.systemSaved = 1;
            t->mode++;
            Sound_PlaySeFlat(1, 4, 0);
            Sound_PlaySeMusicVol(1, 2, 0);
            Sound_PlaySeMusicVol(1, 3, 0);
            if (Chara_EquipViewOn() == 0) {
                pl = &gPlayers[c->slot];
                Chara_SetVoiceMotion(pl, pl->gender, pl->voice);
                c->timer = 0;
            } else {
                c->timer = 0x41;
            }
        }
        break;
    case 6:
        if (func_8FC080() == 1) {
            if (CharaSel_IsDone() != 0) {
                if (++c->timer >= 0x41) {
                    t->mode++;
                    SeqA_Start(5);
                }
            }
        } else {
            if (++c->timer >= 0x41) {
                t->mode++;
                SeqA_Start(5);
            }
        }
        break;
    case 7:
        if (SeqA_GetState() != 1) {
            t->mode++;
            c->timer = 0x3C;
        }
        break;
    case 8:
        if (--c->timer <= 0) {
            TaskSlot_End(t);
            if (gSysSettings.pspCharaWait != 0) {
                gSysSettings.pspCharaWait = 0;
                SeqA_Start(2);
            } else {
                TaskSlot_End(TaskSlot_Get(2));
                Task_SetBootMode2();
                SeqA_Start(2);
            }
            return;
        }
        break;
    }
    c->unk36++;
    Chara_Frame(t);
}

s32 CharaSel_IsShown(void);
void Player_TickTimers(Player *p);
void Player_UpdateHitStop(void *obj);
void Player_UpdateMotionStep(Player *p);
void Players_RunEquipEffects(void);
void Players_UpdateMatrices(void);
void LightSet_UpdateAll(void);
void Camera_Update(void);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
s32 OT_AddAt(u32 *table, void *node, s32 idx, s32 n);
void CharaSel_Draw(void);
void Frame_Draw(void);
extern u8 gSelectDrawHook[];

/* Character creation and selection, every frame: the players, the camera
 * easing to its target (a tenth of the way per frame) and the drawing. */
void Chara_Frame(TaskSlot *p) {
    ChrEdit *c;
    Player *pl;
    s32 i;

    c = &gChrEdit;
    if (func_8FC080() == 1) {
        if (p->mode >= 3 && p->mode < 7) {
            CharaSel_Update();
        }
        if (CharaSel_IsShown() == 0) {
            goto draw;
        }
    }
    if (Chara_EquipViewOn() != 1 && Card_SlotReady(c->slot) != 0) {
        if (p->mode >= 2) {
            for (i = 0, pl = gPlayers; i < 3; i++, pl++) {
                if (pl->active != 0) {
                    if (i == c->slot) {
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
        gCamera->at[0] += (c->camTarget[0] - gCamera->at[0]) / 10.0f;
        gCamera->at[1] += (c->camTarget[1] - gCamera->at[1]) / 10.0f;
        gCamera->at[2] += (c->camTarget[2] - gCamera->at[2]) / 10.0f;
        gCamera->eye[0] += (c->camPos[0] - gCamera->eye[0]) / 10.0f;
        gCamera->eye[1] += (c->camPos[1] - gCamera->eye[1]) / 10.0f;
        gCamera->eye[2] += (c->camPos[2] - gCamera->eye[2]) / 10.0f;
        Camera_Update();
    }
draw:
    if (p->mode < 8) {
        if (Card_SlotReady(c->slot) == 1 && p->mode >= 2 && Chara_EquipViewOn() == 0) {
            pl = &gPlayers[c->slot];
            if (p->mode >= 2 && pl->drawSlot != 0) {
                pl->drawSlot->pos[0] = pl->pos[0];
                pl->drawSlot->pos[1] = pl->pos[1];
                pl->drawSlot->pos[2] = pl->pos[2];
                OT_AddByDepth(gDrawOt32, pl->drawSlot, 0x20, 0);
            }
        }
        if (func_8FC080() == 0) {
            OT_AddAt(gDrawOtSorted, gSelectDrawHook, 0, 0x40);
            Chara_DrawHeader(c);
        } else {
            CharaSel_Draw();
            OT_AddAt(gDrawOtSorted, gSelectDrawHook, 0, 0x40);
        }
        Frame_Draw();
    }
}

s32 Chara_CheckName(s8 *name) {
    return NgWord_Check(name);
}
