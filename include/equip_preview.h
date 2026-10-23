#ifndef EQUIP_PREVIEW_H
#define EQUIP_PREVIEW_H

#include "common.h"
#include "player.h"

/* EquipPreview.equipType: the piece shown is an EquipType (item.h), or this: a model of the
 * gender's own table (Load_EquipModelPack), turning on the spot in a fixed window and coloured
 * with EquipPreview.color. */
#define EQUIP_PREVIEW_FIXED 0xFF

/*
 * The equipment preview (gEquipPreview, 0x5BAD60, 0xB0 bytes): the model of one piece of
 * equipment, shown in a window of the menus. EquipPreview_Load loads its model,
 * EquipPreview_Free frees it, and EquipPreview_Draw draws it each frame with its own camera and
 * lights. The pad pans and zooms the view (except in the fixed window); L3 resets it
 * (EquipPreview_Reset).
 */
typedef struct EquipPreview {
    u8 active;                  /* 0x00: nonzero once the model is loaded: the preview is drawn */
    u8 unk01[0x04 - 0x01];
    u8 unk04[3];                /* 0x04: cleared when a model is loaded */
    u8 settled;                 /* 0x07: the head's swinging parts have run their first ten
                                   passes (EquipPreview_Draw), cleared when a model is loaded */
    s16 modelCount;             /* 0x08: the model slots it took */
    s16 modelFirst;             /* 0x0A: its first model slot */
    struct Model *model;        /* 0x0C: the piece's model */
    u8 female;                  /* 0x10: the model's gender (the swing chains' table) */
    u8 unk11[0x18 - 0x11];
    f32 panX;                   /* 0x18: -60..60, added to the eye point */
    f32 panY;                   /* 0x1C: -30..30 */
    f32 unk20;                  /* 0x20: cleared with the pan and zoom (EquipPreview_Reset) */
    f32 zoom;                   /* 0x24: -30..30, percent added to the field of view */
    EquipFxSwing equipFx[4];    /* 0x28: the head's swinging parts (Effect_RunList) */
    u16 equipType;              /* 0xA8: an EquipType, or EQUIP_PREVIEW_FIXED */
    u16 equipId;                /* 0xAA: the piece's id: a weapon's kind for EQUIP_BLADE and
                                   EQUIP_GUN, the armour's kind for the swing chains */
    u32 color;                  /* 0xAC: 0xRRGGBB, for EQUIP_PREVIEW_FIXED */
} EquipPreview;
typedef char EquipPreview_size_check[(sizeof(EquipPreview) == 0xB0) ? 1 : -1];

extern EquipPreview gEquipPreview;

/* An eye point of the preview (gEquipPreviewEyes), copied whole: the copy moves the three words
 * as integers, as the game's code does. */
typedef struct EquipPreviewEye {
    f32 x, y, z;
} EquipPreviewEye;

void Em_DrawParts(DrawSlot *drawSlot);

#endif
