/*
 * The ending (0xD64C80-0xD67530): the closing message, the staff roll
 * (the table at 0xD761A0 and its strings), "endtex".
 */
#include "common.h"
#include "sprite.h"
#include "syssettings.h"
#include "pad.h"
#include "m2c_macros.h"
#include "hw_regs.h"
#include "plsel/ending.h"   /* the texts this file uses (tools/content_gen.py) */

/* The game compiled this file at optimization level 4 (the build's -O3,p
 * plus MWCC's level-4 passes): Ending_DrawStaffRoll and Ending_DrawTitle
 * match only there, and every other function compiles the same at both
 * levels. */
#pragma optimization_level 4

void Mem_Compact(void);
void Tex_FreeRange(s32 first, s32 count);

/* The ending's work area (0xA04 bytes), behind the pointer at 0x6ADFF0. */
typedef struct EndWork {
    u8 unk00;                   /* 0x00: state (Ending_Update) */
    u8 unk01[0x04 - 0x01];
    s32 unk04;                  /* 0x04: countdown of state 3 */
    u8 unk08[0x1C - 0x08];
    f32 unk1C;                  /* 0x1C */
    f32 unk20;                  /* 0x20 */
    f32 unk24;                  /* 0x24 */
    s32 unk28;                  /* 0x28: frame counter */
    s32 unk2C;                  /* 0x2C */
    s32 unk30;                  /* 0x30 */
    s32 unk34;                  /* 0x34 */
    s32 unk38;                  /* 0x38 */
    s32 unk3C;                  /* 0x3C */
    s32 unk40;                  /* 0x40 */
    s32 unk44;                  /* 0x44 */
    f32 unk48;                  /* 0x48 */
    f32 unk4C;                  /* 0x4C */
    f32 unk50;                  /* 0x50 */
    f32 unk54;                  /* 0x54 */
    f32 unk58;                  /* 0x58 */
    f32 unk5C;                  /* 0x5C */
    f32 unk60;                  /* 0x60 */
    f32 unk64;                  /* 0x64 */
    s32 unk68;                  /* 0x68: index into gEndingSteps */
    u32 unk6C;                  /* 0x6C: color, alpha in the top byte */
    s32 unk70;                  /* 0x70 */
    s32 unk74;                  /* 0x74 */
    u8 slots[16][0x98];         /* 0x78: allocated by Ending_AllocObject */
    s32 unk9F8;                 /* 0x9F8 */
    s32 unk9FC;                 /* 0x9FC: show the closing message */
    f32 unkA00;                 /* 0xA00: its fade, 0..1 */
} EndWork;

extern EndWork *gObjBufBlocks;
#define gEndWork (gObjBufBlocks)

/* A step of the ending's sequence (gEndingSteps, 32 bytes each). */
typedef struct EndStep {
    s8 unk00;                   /* 0x00: -1 ends the sequence */
    s8 unk01;
    s8 unk02;
    u8 unk03;
    s16 unk04;
    s16 unk06;
    s16 unk08;
    s16 unk0A;
    f32 unk0C;
    f32 unk10;
    s16 unk14;
    s16 unk16;
    f32 unk18;
    f32 unk1C;
} EndStep;

typedef struct EndPt {
    s16 x;
    s16 y;
} EndPt;

/* An entry of gEndingPics (8 bytes): a picture's size. */
typedef struct EndPic {
    s16 w;
    s16 h;
    s16 dw;
    s16 dh;
} EndPic;

/* An entry of the staff roll D_00D761A0 (12 bytes). */
typedef struct EndRoll {
    s16 x;                      /* 0x0 */
    s16 y;                      /* 0x2 */
    s8 type;                    /* 0x4: 0 text, 1 glyph, 2 title, 3 picture, 4 end */
    u8 unk5;                    /* 0x5 */
    s16 unk6;                   /* 0x6 */
    char *unk8;                 /* 0x8 */
} EndRoll;

/* An entry of gEndingGlyphs (10 bytes): a font glyph. */
typedef struct EndGlyph {
    u8 tex;
    u8 unk1;
    UV uv0;
    UV uv1;
} EndGlyph;

/* A step of an object's animation: the tables gEndingObjDefs points to. */
typedef struct EndDef {
    s8 unk00;                   /* 0x00 */
    s8 unk01;                   /* 0x01: -1: the end of a loop */
    s8 unk02;                   /* 0x02: the next step, < 0: none */
    s8 unk03;                   /* 0x03: kinds 8 and 13: the place in gEndingObjPlaces2 */
    s16 unk04;                  /* 0x04: frames to wait */
    s16 unk06;                  /* 0x06: drawing offset */
    s16 unk08;                  /* 0x08 */
    s16 unk0A;                  /* 0x0A: move */
    s16 unk0C;                  /* 0x0C */
    s16 unk0E;                  /* 0x0E: texture (gTexHandles + 0xA3) */
    s16 unk10;                  /* 0x10: cell of unk14 */
    u8 unk12[2];
    s16 *unk14;                 /* 0x14: cells: u, v, texture w, h, screen w, h */
} EndDef;

/* An animation state (0x3C bytes). */
typedef struct EndAnim {
    s32 unk00;                  /* 0x00 */
    u8 unk04[0x04];
    s32 unk08;                  /* 0x08 */
    s32 unk0C;                  /* 0x0C */
    s32 unk10;                  /* 0x10 */
    s32 unk14;                  /* 0x14 */
    f32 unk18;                  /* 0x18 */
    f32 unk1C;                  /* 0x1C */
    u8 unk20[0x28 - 0x20];
    s32 unk28;                  /* 0x28 */
    s32 unk2C;                  /* 0x2C */
    u8 unk30[0x34 - 0x30];
    s32 unk34;                  /* 0x34 */
    EndDef *unk38;              /* 0x38 */
} EndAnim;

/* One of the work area's 16 slots (0x98 bytes). */
typedef struct EndObj {
    s32 unk00;                  /* 0x00 */
    s32 unk04;                  /* 0x04: animated */
    s32 unk08;                  /* 0x08: kind (Ending_UpdateObjects) */
    s8 step;                    /* 0x0C: step of the kind's movement */
    u8 unk0D[0x10 - 0x0D];
    s32 timer;                  /* 0x10 */
    u8 unk14[0x20 - 0x14];
    f32 speed;                  /* 0x20 */
    f32 speed2;                 /* 0x24: kind 7's x speed */
    u8 unk28[0x30 - 0x28];
    f32 unk30;                  /* 0x30 */
    f32 unk34;                  /* 0x34 */
    u8 unk38[0x40 - 0x38];
    s16 unk40;                  /* 0x40 */
    s16 unk42;                  /* 0x42 */
    u8 unk44[0x5C - 0x44];
    EndAnim anim;               /* 0x5C */
} EndObj;

void *memset(void *, s32, u32);
void Loading_SetMessage(u32 mode);
void Loading_Stop(void);
void Tex_LoadSystemPackB(void);
void Ending_LoadTextures(void);
void Ending_SetStep(EndWork *w);
void Afs_LoadPartition(s32 ptid, s32 flid, void *ptinfo, char *name);
s32 Afs_ReadFileIdle(s32 flid, s32 ptid, void *buf);
u32 Tex_CreateFromTim2(u8 *tim2, s32 mode);
extern s32 gOvlTex[];

void Gfx_SetState63(s32);
void Tex_Prepare(s32, s32);
void Tex_Bind(s32);
struct Sprite;
s32 Sprite_Draw(struct Sprite *);

void Text_SetDefaults(void);
void FadeA_Start(s32);
void Stream_Prepare(s32, s32, s32, s32);
void Stream_StartPrepared(s32);
u8 FadeA_GetState(void);
void Movie_Stop(void);
void Stream_Stop(s32);
void Ending_FreeTextures(void);
void Ending_Draw(EndWork *w);
void Font_DrawAllPages(void);
void Ending_Scroll(EndWork *w);

char *strcpy(char *dst, const char *src);
void strupr(char *);

s32 Gs_GetState(u8 reg);
s32 Gs_SetState(u8 reg, u32 v);
void Ending_DrawPictures(EndWork *w);
void Ending_DrawObjects(void);
void Ending_DrawStaffRoll(EndWork *w);
s32 strlen(char *s);
void Font_SetSpacing(u8 a, u8 b);
void Text_DrawShadowRgb(s16 x, s16 y, u32 c1, u32 c2, char *s);

void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
void Ending_SpawnObjects(EndRoll *);
void Ending_DrawTitle(s16 x, s16 y, char *str);

void Ending_UpdateObjects(void);

extern s32 gTexHandles[];
s32 Tex_Use(s32 n, u32 *handles);
void Ending_DrawSpriteF(TexRect *, f32, f32);

/* .data (0xD76100-0xD78B10). D_00D761A0 is the staff roll: its strings
 * are the file's first .rodata (0xD795D8). */
u32 gEndingGlyphColors[8] = {
#include "plsel/00D76100.inc"
};
u32 gEndingObjPlaces[24] = {
#include "plsel/00D76120.inc"
};
u32 gEndingPoints[8] = {
#include "plsel/00D76180.inc"
};
EndRoll D_00D761A0[345] = {
#include "plsel/00D761A0.inc"
};
EndPic gEndingPics[22] = {
#include "plsel/00D771D0.inc"
};
u32 gEndingSteps[96] = {
#include "plsel/00D77280.inc"
};
u32 gEndingGlyphs[152] = {
#include "plsel/00D77400.inc"
};
u32 D_00D77660[48] = {
#include "plsel/00D77660.inc"
};
u32 gEndingObjDefs_2[116] = {
#include "plsel/00D77720.inc"
};
u32 D_00D778F0[48] = {
#include "plsel/00D778F0.inc"
};
u32 gEndingObjDefs_0[188] = {
#include "plsel/00D779B0.inc"
};
u32 gEndingObjDefs_11[188] = {
#include "plsel/00D77CA0.inc"
};
u32 D_00D77F90[24] = {
#include "plsel/00D77F90.inc"
};
u32 gEndingObjDefs_1[48] = {
#include "plsel/00D77FF0.inc"
};
u32 D_00D780B0[16] = {
#include "plsel/00D780B0.inc"
};
u32 gEndingObjDefs_3[48] = {
#include "plsel/00D780F0.inc"
};
u32 D_00D781B0[16] = {
#include "plsel/00D781B0.inc"
};
u32 gEndingObjDefs_4[48] = {
#include "plsel/00D781F0.inc"
};
u32 D_00D782B0[12] = {
#include "plsel/00D782B0.inc"
};
u32 gEndingObjDefs_8[68] = {
#include "plsel/00D782E0.inc"
};
u32 gEndingObjDefs_13[32] = {
#include "plsel/00D783F0.inc"
};
u32 D_00D78470[28] = {
#include "plsel/00D78470.inc"
};
u32 D_00D784E0[40] = {
#include "plsel/00D784E0.inc"
};
u32 gEndingObjDefs_5[140] = {
#include "plsel/00D78580.inc"
};
u32 D_00D787B0[32] = {
#include "plsel/00D787B0.inc"
};
u32 gEndingObjDefs_7[84] = {
#include "plsel/00D78830.inc"
};
u32 gEndingObjDefs_9[8] = {
#include "plsel/00D78980.inc"
};
u32 gEndingObjDefs_6[8] = {
#include "plsel/00D789A0.inc"
};
u32 gEndingObjDefs_10[8] = {
#include "plsel/00D789C0.inc"
};
u32 gEndingObjDefs[16] = {
    (u32)gEndingObjDefs_0, (u32)gEndingObjDefs_1, (u32)gEndingObjDefs_2, (u32)gEndingObjDefs_3,
    (u32)gEndingObjDefs_4, (u32)gEndingObjDefs_5, (u32)gEndingObjDefs_6, (u32)gEndingObjDefs_7,
    (u32)gEndingObjDefs_8, (u32)gEndingObjDefs_9, (u32)gEndingObjDefs_10, (u32)gEndingObjDefs_11,
    (u32)gEndingObjDefs_11, (u32)gEndingObjDefs_13, 0x00000000, 0x00000000,
};
u32 gEndingObjPlaces2[24] = {
#include "plsel/00D78A20.inc"
};
u32 gEndingPictureUV[16] = {
#include "plsel/00D78A80.inc"
};
u32 gEndingSpriteGif[20] = {
#include "plsel/00D78AC0.inc"
};

void Ending_Init(void) {
    EndWork *w;

    w = gEndWork;
    if (gSysSettings.galleryPlay != 0) {
        Loading_SetMessage(5);
    }
    memset(w, 0, 0xA04);
    Tex_LoadSystemPackB();
    Ending_LoadTextures();
    w->unk1C = -448.0f;
    w->unk20 = 1.2f;
    w->unk24 = 13000.0f;
    w->unk68 = 0;
    w->unk70 = 0x10;
    Ending_SetStep(w);
    w->unk9F8 = 0x231E;
    if (gSysSettings.galleryPlay != 0) {
        Loading_Stop();
    }
}

s32 Ending_Update(void) {
    EndWork *w;

    w = gEndWork;
    Text_SetDefaults();
    switch (w->unk00) {
    case 0:
        FadeA_Start(2);
        Stream_Prepare(0, 0x134, 0, 0);
        w->unk00++;
    case 1:
        w->unk00++;
        Stream_StartPrepared(0);
        FadeA_Start(2);
        break;
    case 2:
        w->unk9F8--;
        if (FadeA_GetState() != 1) {
            if (gSysSettings.galleryPlay != 0) {
                if ((gPadState[0].pressed & 0x40) || w->unk9F8 == 0) {
                    w->unk00 = 5;
                    FadeA_Start(1);
                    break;
                }
            } else if ((gPadState[0].pressed & 0x8000) || w->unk9F8 == 0) {
                w->unk00 = 5;
                FadeA_Start(1);
                break;
            }
        }
        Ending_Scroll(w);
        break;
    case 3:
        if (--w->unk04 == 0) {
            w->unk00 = 4;
            FadeA_Start(1);
        }
        break;
    case 4:
    case 5:
        if (FadeA_GetState() != 1) {
            Movie_Stop();
            Stream_Stop(0);
            Ending_FreeTextures();
            return 0;
        }
        break;
    case 99:
        FadeA_Start(2);
        w->unk00 = 1;
        break;
    }
    Ending_Draw(w);
    Font_DrawAllPages();
    return 1;
}

void Ending_Scroll(EndWork *w) {
    u32 a;

    w->unk1C += w->unk20;
    if (w->unk1C >= w->unk24) {
        w->unk1C = w->unk24;
    }
    w->unk28++;
    if ((w->unk28 & 3) == 0) {
        w->unk2C++;
        w->unk2C %= 3;
    }
    if (w->unk30 != 0) {
        if (w->unk34 == 0) {
            if (w->unk40 > (s32)(448.0f + w->unk1C)) {
                return;
            }
            w->unk34 = 1;
        }
        w->unk48 += w->unk58;
        w->unk4C += w->unk5C;
        w->unk50 += w->unk60;
        w->unk54 += w->unk64;
        switch (w->unk74) {
        case 0:
            a = ((w->unk6C & 0xFF000000) >> 24) + w->unk70;
            if (a >= 0xFF) {
                w->unk74 = 1;
                a = 0xFF;
            }
            w->unk6C = (a << 24) | (w->unk6C & 0xFFFFFF);
            break;
        case 1:
            if ((s32)w->unk1C >= w->unk44 - (s32)(32.0f * w->unk20)) {
                w->unk74 = 2;
            }
            break;
        case 2:
            a = (w->unk6C & 0xFF000000) >> 24;
            a -= w->unk70;
            if (a >= 0xFF) {
                w->unk74 = 0;
                w->unk68++;
                a = 0;
                Ending_SetStep(w);
            }
            w->unk6C = (a << 24) | (w->unk6C & 0xFFFFFF);
            break;
        }
    }
    Ending_UpdateObjects();
    if (w->unk1C >= 10110.0f) {
        w->unk00 = 3;
        w->unk04 = 0xD2;
        w->unk9FC = 1;
        w->unkA00 = 0.0f;
    }
}

void Ending_DrawPicture(s16 x, s16 y, u8 idx) {
    TexRect s;
    UV *uv;
    EndPt *r;

    r = (EndPt *)gEndingPoints + idx;
    uv = (UV *)gEndingPictureUV + idx * 2;
    Gfx_SetState63(1);
    Tex_Prepare(1, 0xBC);
    Tex_Bind(0xBC);
    s.x = x;
    s.y = y;
    s.w = r->x;
    s.h = r->y;
    s.color = -1;
    s.uv0.u = uv->u;
    s.uv0.v = uv->v;
    s.uv1.u = (++uv)->u;
    s.uv1.v = uv->v;
    if ((s.uv0.u != 0 || s.uv0.v != 0 || s.uv1.u != 0 || s.uv1.v != 0) && s.y < 0x1C1 && s.y + s.h > 0) {
        Sprite_Draw((struct Sprite *)&s);
    }
}

void Ending_DrawGlyph(s32 idx, s16 x, s16 y, s8 col, s32 sub) {
    TexRect s;
    EndGlyph g;

    g = ((EndGlyph *)gEndingGlyphs)[sub + idx * 3];
    Gfx_SetState63(g.unk1);
    Tex_Prepare(1, g.tex);
    Tex_Bind(g.tex);
    s.x = x;
    s.y = y - 8;
    s.w = 0x26;
    s.h = 0x20;
    s.color = gEndingGlyphColors[col];
    s.uv0.u = g.uv0.u;
    s.uv0.v = g.uv0.v;
    s.uv1.u = g.uv1.u;
    s.uv1.v = g.uv1.v;
    Sprite_Draw((struct Sprite *)&s);
}

/* Draws an upper-cased title in the 20x20 letter cells of texture 8; a
 * space advances 14 pixels. The space width is a float converted to s16 at
 * each space (lui/mtc1/cvt.w.s inside the loop): a constant `(s32)14.0f` is
 * folded, and an s32 conversion of the float is hoisted out of the loop. */
void Ending_DrawTitle(s16 x, s16 y, char *str) {
    char buf[0x20];
    TexRect s;
    char *p;
    s32 c;
    f32 space;

    space = 14.0f;
    Tex_Prepare(1, 8);
    Tex_Bind(8);
    Gfx_SetState63(1);
    s.w = s.h = 0x14;
    buf[0] = 0;
    strcpy(buf, str);
    strupr(buf);
    p = buf;
    while (*p != 0) {
        if (*p == ' ') {
            p++;
            x += (s16)space;
        } else {
            c = *p - 'A';
            s.uv0.u = (c % 8) * 32;
            s.uv0.v = (c / 8) * 32;
            s.uv1.u = s.uv0.u + 0x1F;
            s.uv1.v = s.uv0.v + 0x1F;
            s.color = 0xFF2691A1;
            s.x = x;
            s.y = y;
            Sprite_Draw((struct Sprite *)&s);
            x += 0x14;
            p++;
        }
    }
}

void Ending_SetStep(EndWork *w) {
    EndStep *e;

    e = (EndStep *)gEndingSteps + w->unk68;
    if (e->unk00 == -1) {
        w->unk30 = 0;
    } else {
        w->unk30 = 1;
        w->unk34 = 0;
        w->unk6C = -1;
        w->unk74 = 0;
        w->unk38 = e->unk01;
        w->unk3C = e->unk02;
        w->unk40 = e->unk04;
        w->unk44 = e->unk06;
        w->unk48 = e->unk08;
        w->unk4C = e->unk0A;
        w->unk58 = e->unk0C;
        w->unk5C = e->unk10;
        w->unk50 = e->unk14;
        w->unk54 = e->unk16;
        w->unk60 = e->unk18;
        w->unk64 = e->unk1C;
    }
}

void Ending_DrawPictures(EndWork *w) {
    TexRect s;
    s32 *h;

    if (w->unk34 == 0) {
        return;
    }
    Gs_SetState(0x5E, 0x32);
    Gs_SetState(0x63, 0);
    if (w->unk38 != -1) {
        h = &gTexHandles[w->unk38 + 0xA3];
        Tex_Use(1, (u32 *)h);
        Gs_SetState(4, *h);
        s.uv0.u = 0;
        s.uv0.v = 0;
        s.uv1.u = gEndingPics[w->unk38].w - 1;
        s.uv1.v = gEndingPics[w->unk38].h - 1;
        s.w = gEndingPics[w->unk38].dw;
        s.h = gEndingPics[w->unk38].dh;
        s.color = w->unk6C;
        s.x = w->unk48;
        s.y = w->unk4C;
        Ending_DrawSpriteF(&s, w->unk48, w->unk4C);
    }
    if (w->unk3C != -1) {
        h = &gTexHandles[w->unk3C + 0xA3];
        Tex_Use(1, (u32 *)h);
        Gs_SetState(4, *h);
        s.uv0.u = 0;
        s.uv0.v = 0;
        s.uv1.u = gEndingPics[w->unk3C].w - 1;
        s.uv1.v = gEndingPics[w->unk3C].h - 1;
        s.w = gEndingPics[w->unk3C].dw;
        s.h = gEndingPics[w->unk3C].dh;
        s.color = -1;
        s.x = w->unk50;
        s.y = w->unk54;
        if (s.x > 0) {
            s.x = 0;
        }
        if (s.x + s.w < 0x280) {
            s.x = 0x280 - s.w;
        }
        if (s.y > 0) {
            s.y = 0;
        }
        if (s.y + s.h < 0x1C0) {
            s.y = 0x1C0 - s.h;
        }
        if (s.w < (s16)w->unk50 + 0x280) {
            w->unk50 = s.w - 0x280;
        }
        Ending_DrawSpriteF(&s, w->unk50, w->unk54);
    }
}

void Ending_DrawStaffRoll(EndWork *w) {
    EndRoll *e;
    s16 y;
    f32 lim;
    s16 top;
    s16 bottom;

    e = D_00D761A0;
    top = w->unk1C;
    bottom = top + 0x1C0;
    y = e->y;
    while (y < w->unk24) {
        if (y < bottom) {
            switch (e->type) {
            case 0:
                if (top < y + e->unk6) {
                    Font_SetSpacing((u8)e->unk6, (u8)e->unk6);
                    Text_DrawShadow(e->x, e->y - top, 1, e->unk5, e->unk8);
                }
                break;
            case 1:
                if (top < y + 0x20) {
                    Ending_DrawGlyph(e->unk5, e->x, y - top, e->unk6, w->unk2C);
                }
                break;
            case 2:
                if (top < y + 0x1A) {
                    Ending_DrawTitle(e->x, y - top, e->unk8);
                }
                break;
            case 3:
                if (top < y + ((EndPt *)gEndingPoints)[e->unk5].y) {
                    Ending_DrawPicture(e->x, y - top, e->unk5);
                }
                break;
            }
        }
        if (e->type == 4) {
            lim = 448.0f + w->unk1C + w->unk20;
            if (e->y >= bottom && e->y < (s16)lim) {
                Ending_SpawnObjects(e);
            }
        }
        e++;
        y = e->y;
    }
}

void Ending_Draw(EndWork *w) {
    s32 a;
    s32 b;
    s32 c;
    u32 alpha;
    s32 len;

    a = Gs_GetState(0x6C);
    b = Gs_GetState(0x63);
    c = Gs_GetState(0x5E);
    Gs_SetState(0x6C, 0);
    Ending_DrawPictures(w);
    Ending_DrawObjects();
    Ending_DrawStaffRoll(w);
    if (w->unk9FC != 0 && w->unk04 < 0x97) {
        w->unkA00 += 0.05f;
        if (w->unkA00 > 1.0f) {
            w->unkA00 = 1.0f;
        }
        alpha = 255.0f * w->unkA00;
        len = strlen(TXT(0x00D7BA50) /* the closing message */);
        Font_SetSpacing(0x12, 0x12);
        Text_DrawShadowRgb((0x280 - len * 9) / 2, 0xFA, 0xFF000000, 0xFF000000 | ((u8)alpha << 16) | ((u8)alpha << 8) | (u8)alpha, TXT(0x00D7BA50) /* the closing message */);
    }
    Gs_SetState(0x6C, a);
    Gs_SetState(0x63, b);
    Gs_SetState(0x5E, c);
}

void Ending_LoadTextures(void) {
    s32 i;

    Afs_LoadPartition(5, 0x19F3, gSysSettings.readBuf, "endtex");
    for (i = 0; i < 0x1A; i++) {
        Afs_ReadFileIdle(i, 5, gSysSettings.loadBuf);
        gOvlTex[i] = Tex_CreateFromTim2(gSysSettings.loadBuf, 4);
    }
    Mem_Compact();
}

void Ending_FreeTextures(void) {
    Tex_FreeRange(0xA3, 0x1A);
    Mem_Compact();
}

static void Ending_InitAnim(EndAnim *a, EndDef *d) {
    s32 dx;
    s32 dy;

    a->unk00 = 1;
    a->unk08 = 0;
    a->unk0C = 0;
    a->unk28 = 0;
    a->unk2C = 0;
    a->unk38 = d;
    a->unk14 = 0;
    a->unk08 = d->unk04;
    dx = d->unk0A;
    if (a->unk10 != 0) {
        dx = -dx;
    }
    dy = d->unk0C;
    a->unk18 += dx;
    a->unk1C += dy;
}

static void Ending_InitObject(EndObj *o, s32 kind, s32 x, s32 y, s32 n) {
    s16 *p;

    o->unk00 = 1;
    o->unk08 = kind;
    o->unk30 = x;
    o->unk34 = y;
    Ending_InitAnim(&o->anim, (EndDef *)gEndingObjDefs[kind]);
    o->anim.unk34 = kind;
    p = o->anim.unk38->unk14;
    o->unk40 = p[4];
    o->unk42 = p[5];
}

/* Steps an object's animation: waits, then moves to the next step (by the
 * step's own link, with loop counts per animation kind) and applies it. */
static void Ending_AnimateObject(EndObj *o, s32 n) {
    EndDef *d;
    s32 dx;
    s32 dy;

    if (o->unk04 == 0) {
        return;
    }
    if (o->anim.unk08 > 0) {
        o->anim.unk08--;
        return;
    }
    d = &o->anim.unk38[o->anim.unk14];
    switch (o->anim.unk34) {
    case 0:
        if (d->unk01 == -1) {
            if (d->unk02 < 0) {
                o->unk00 = 0;
                return;
            }
            o->anim.unk0C++;
            if (o->anim.unk0C == 2) {
                o->anim.unk14 = 0x12;
                o->anim.unk0C = 0;
                goto apply;
            }
        }
        break;
    case 1:
        if (d->unk01 == -1) {
            if (d->unk02 < 0) {
                o->unk00 = 0;
                return;
            }
            o->anim.unk0C++;
            if (o->anim.unk0C == 4) {
                o->anim.unk14 = 5;
                o->anim.unk0C = 0;
                goto apply;
            }
        }
        break;
    case 2:
        if (d->unk01 == -1) {
            o->unk00 = 0;
            return;
        }
        break;
    case 8:
        if (d->unk01 == -1) {
            if (d->unk02 < 0) {
                o->unk00 = 0;
                return;
            }
            o->anim.unk0C++;
            if (o->anim.unk0C == 2) {
                o->unk00 = 0;
                return;
            }
        }
        break;
    case 11:
        if (d->unk01 == -1) {
            if (d->unk02 < 0) {
                o->unk00 = 0;
                return;
            }
            o->anim.unk0C++;
            if (o->anim.unk0C == 2) {
                o->anim.unk14 = 0x12;
                o->anim.unk0C = 0;
                goto apply;
            }
        }
        break;
    case 13:
        if (d->unk02 < 0) {
            o->unk00 = 0;
            return;
        }
        break;
    }
    d = &o->anim.unk38[o->anim.unk14];
    if (d->unk02 < 0) {
        o->anim.unk00 = 0;
        return;
    }
    o->anim.unk14 = d->unk02;
apply:
    d = &o->anim.unk38[o->anim.unk14];
    o->anim.unk08 = d->unk04;
    dx = d->unk0A;
    if (o->anim.unk10 != 0) {
        dx = -dx;
    }
    dy = d->unk0C;
    o->unk30 += dx;
    o->unk34 += dy;
}

static u8 *Ending_AllocObject(void) {
    u8 *p;
    s32 i;

    p = gEndWork->slots[0];
    for (i = 0; i < 0x10; i++, p += 0x98) {
        if (*(s32 *)p == 0) {
            memset(p, 0, 0x98);
            return p;
        }
    }
    return NULL;
}

/* Starts the animated objects of a staff roll entry of type 4 when it comes
 * into view: unk5 picks the animation kinds, unk6 the place in gEndingObjPlaces
 * (x, y, flipped). */
void Ending_SpawnObjects(EndRoll *r) {
    EndObj *o;
    s32 n;
    s16 *pl;

    o = (EndObj *)Ending_AllocObject();
    if (o != NULL) {
        pl = (s16 *)gEndingObjPlaces + r->unk6 * 3;
        n = 0;
        switch (r->unk5) {
        case 0:
            Ending_InitObject(o, 0, r->x, r->y, 0);
            o->unk08 = 0;
            o->unk30 = *pl++;
            o->unk34 = *pl++;
            o->anim.unk10 = *pl;
            break;
        case 1:
            Ending_InitObject(o, 1, r->x, r->y, 0);
            o->unk08 = 0;
            o->unk30 = *pl++;
            o->unk34 = *pl++;
            o->anim.unk10 = *pl;
            break;
        case 2:
            Ending_InitObject(o, 2, r->x, r->y, 0);
            o->unk08 = 0;
            o->unk30 = *pl++;
            o->unk34 = *pl++;
            o->anim.unk10 = *pl;
            break;
        case 3:
            Ending_InitObject(o, 4, r->x, r->y, n++);
            o->unk30 = 640.0f;
            o->unk34 = pl[1];
            o->anim.unk10 = pl[2];
            o = (EndObj *)Ending_AllocObject();
            if (o == NULL) {
                break;
            }
            Ending_InitObject(o, 5, r->x, r->y, n++);
            o->unk30 = 640.0f;
            o->unk34 = pl[1];
            o->anim.unk10 = pl[2];
            o = (EndObj *)Ending_AllocObject();
            if (o == NULL) {
                break;
            }
            Ending_InitObject(o, 3, r->x, r->y, n++);
            o->unk30 = 640.0f;
            o->unk34 = pl[1];
            o->anim.unk10 = pl[2];
            break;
        case 4:
            Ending_InitObject(o, 6, r->x, r->y, 0);
            o->unk08 = 2;
            o->unk30 = *pl++;
            o->unk34 = *pl++;
            o->anim.unk10 = *pl;
            break;
        case 5:
            Ending_InitObject(o, 7, r->x, r->y, 0);
            o->unk08 = 1;
            o->unk30 = *pl++;
            pl++;
            o->anim.unk10 = *pl;
            o->unk34 = 448.0f;
            break;
        case 6:
            Ending_InitObject(o, 8, r->x, r->y, 0);
            o->unk08 = 0;
            o->unk30 = *pl++;
            o->unk34 = *pl++;
            o->anim.unk10 = *pl;
            break;
        case 7:
            Ending_InitObject(o, 9, r->x, r->y, 0);
            o->unk08 = 6;
            o->unk30 = *pl++;
            pl++;
            o->anim.unk10 = *pl;
            o->unk34 = -48.0f;
            break;
        case 8:
            Ending_InitObject(o, 0xA, r->x, r->y, 0);
            o->unk08 = 7;
            o->unk30 = *pl++;
            o->unk34 = *pl++;
            o->anim.unk10 = *pl;
            if (o->anim.unk10 == 0) {
                o->unk30 = 640.0f;
            } else {
                o->unk30 = -o->unk40;
            }
            break;
        case 9:
            Ending_InitObject(o, 0xB, r->x, r->y, 0);
            o->unk08 = 8;
            o->unk30 = *pl++;
            o->unk34 = *pl++;
            o->anim.unk10 = *pl;
            break;
        case 10:
            Ending_InitObject(o, 0xB, r->x, r->y, 0);
            o->unk08 = 9;
            o->unk30 = *pl++;
            o->unk34 = *pl++;
            o->anim.unk10 = *pl;
            break;
        case 11:
            Ending_InitObject(o, 0xD, r->x, r->y, 0);
            o->unk08 = 0;
            o->unk30 = *pl++;
            o->unk34 = *pl++;
            o->anim.unk10 = *pl;
            break;
        }
    }
}

/* Moves the 16 objects by kind (the steps of each kind's movement, with a
 * countdown in timer), frees those that are done or off the screen, then
 * animates the others. */
void Ending_UpdateObjects(void) {
    EndWork *w = gEndWork;
    EndObj *o;
    s32 i;

    for (i = 0, o = (EndObj *)w->slots[0]; i < 16; i++, o++) {
        if (o->unk00 == 0) {
            continue;
        }
        switch (o->unk08) {
        case 0:
            switch (o->step) {
            case 0:
                o->timer = 300;
                o->unk04 = 1;
                o->step++;
            case 1:
                if (--o->timer == 0) {
                    o->unk04 = 0;
                    o->unk00 = 0;
                }
                break;
            }
            break;
        case 1:
            switch (o->step) {
            case 0:
                o->unk04 = 1;
                o->step++;
            case 1:
                o->unk34 -= w->unk20;
                if (o->unk34 + o->unk42 < 0.0f) {
                    o->unk04 = 0;
                    o->unk00 = 0;
                }
                break;
            }
            break;
        case 2:
            switch (o->step) {
            case 0:
                o->timer = 150;
                o->unk04 = 1;
                o->step++;
                o->speed = 0.2f;
            case 1:
                o->unk34 += o->speed;
                if (--o->timer == 0) {
                    o->timer = 150;
                    o->speed = -0.2f;
                    o->step++;
            case 2:
                    o->unk34 += o->speed;
                    if (--o->timer == 0) {
                        o->unk04 = 0;
                        o->unk00 = 0;
                    }
                }
                break;
            }
            break;
        case 3:
            switch (o->step) {
            case 0:
                o->timer = 180;
                o->unk04 = 0;
                o->step++;
            case 1:
                if (--o->timer == 0) {
                    o->unk04 = 1;
                    o->step++;
                    if (o->anim.unk10 == 0) {
                        o->speed = -4.0f;
                    } else {
                        o->speed = 4.0f;
                    }
            case 2:
                    o->unk30 += o->speed;
                    if (o->unk30 + o->unk40 < 0.0f) {
                        o->unk04 = 0;
                        o->unk00 = 0;
                    }
                }
                break;
            }
            break;
        case 4:
            switch (o->step) {
            case 0:
                o->unk04 = 1;
                o->step++;
                if (o->anim.unk10 == 0) {
                    o->speed = -4.0f;
                } else {
                    o->speed = 4.0f;
                }
            case 1:
                o->unk30 += o->speed;
                if (o->unk30 + o->unk40 < 0.0f) {
                    o->unk04 = 0;
                    o->unk00 = 0;
                }
                break;
            }
            break;
        case 5:
            switch (o->step) {
            case 0:
                o->timer = 90;
                o->unk04 = 0;
                o->step++;
            case 1:
                if (--o->timer == 0) {
                    o->unk04 = 1;
                    o->step++;
                    if (o->anim.unk10 == 0) {
                        o->speed = -4.0f;
                    } else {
                        o->speed = 4.0f;
                    }
            case 2:
                    o->unk30 += o->speed;
                    if (o->unk30 + o->unk40 < 0.0f) {
                        o->unk04 = 0;
                        o->unk00 = 0;
                    }
                }
                break;
            }
            break;
        case 6:
            switch (o->step) {
            case 0:
                o->unk04 = 1;
                o->step++;
                o->speed = 8.0f;
            case 1:
                o->unk34 += o->speed;
                if (o->unk34 + o->unk42 > 480.0f) {
                    o->unk04 = 0;
                    o->unk00 = 0;
                }
                break;
            }
            break;
        case 7:
            switch (o->step) {
            case 0:
                o->timer = 30;
                o->unk04 = 1;
                o->step++;
                o->speed = 0.4f;
                if (o->anim.unk10 == 0) {
                    o->speed2 = -2.0f;
                } else {
                    o->speed2 = 2.0f;
                }
            case 1:
                o->unk30 += o->speed2;
                o->unk34 += o->speed;
                if (--o->timer != 0) {
                    if (o->anim.unk10 == 0) {
                        if (o->unk30 < -(o->unk40 * 2)) {
                            o->unk04 = 0;
                            o->unk00 = 0;
                        }
                    } else if (o->unk30 >= 640.0f) {
                        o->unk04 = 0;
                        o->unk00 = 0;
                    }
                    break;
                }
                o->timer = 30;
                o->speed = -0.4f;
                o->step++;
            case 2:
                o->unk30 += o->speed2;
                o->unk34 += o->speed;
                if (--o->timer != 0) {
                    if (o->anim.unk10 == 0) {
                        if (o->unk30 < -(o->unk40 * 2)) {
                            o->unk04 = 0;
                            o->unk00 = 0;
                        }
                    } else if (o->unk30 >= 640.0f) {
                        o->unk04 = 0;
                        o->unk00 = 0;
                    }
                    break;
                }
                o->timer = 30;
                o->speed = 0.4f;
                o->step--;
                break;
            }
            break;
        case 8:
            switch (o->step) {
            case 0:
                o->unk04 = 1;
                o->step++;
            case 1:
                o->unk34 += 1.2f;
                if (o->unk34 >= 450.0f) {
                    o->unk04 = 0;
                    o->unk00 = 0;
                }
                break;
            }
            break;
        case 9:
            switch (o->step) {
            case 0:
                o->unk04 = 1;
                o->step++;
            case 1:
                o->unk34 -= 1.2f;
                if (o->unk34 + o->unk42 < 0.0f) {
                    o->unk04 = 0;
                    o->unk00 = 0;
                }
                break;
            }
            break;
        }
        if (o->unk00 != 0 && o->step != 0) {
            Ending_AnimateObject(o, i);
        }
    }
}

void Gfx_SetBlend(s32, s32);
void TexRect_DrawRotated(TexRect *r, u16 angle); /* unit's own declaration: with the definition's void (TexRect *r, s32 angle) (sub_main/unk_363550.c), Ending_DrawObjects 1 of 198 words differ */

/* Draws the 16 objects: the current step's cell (texture rectangle and
 * size, mirrored when flipped); kinds 8 and 13 are drawn rotated at the
 * places of gEndingObjPlaces2. */
void Ending_DrawObjects(void) {
    TexRect s;
    EndObj *o;
    EndDef *d;
    EndPt *c;
    s16 *r;
    s32 i;
    s16 u;
    s16 v;
    s16 tw;
    s16 th;
    s16 w;
    s16 h;

    o = (EndObj *)gEndWork->slots[0];
    for (i = 0; i < 16; i++, o++) {
        if (o->unk00 == 0) {
            continue;
        }
        d = &o->anim.unk38[o->anim.unk14];
        c = (EndPt *)(d->unk14 + d->unk10 * 6);
        Gfx_SetBlend(4, 5);
        Tex_Prepare(1, d->unk0E + 0xA3);
        Tex_Bind(d->unk0E + 0xA3);
        Gs_SetState(0x63, 0);
        u = c->x;
        v = c->y;
        tw = (++c)->x;
        th = c->y;
        w = (++c)->x;
        h = c->y;
        if (o->anim.unk34 == 8 || o->anim.unk34 == 13) {
            s.w = w;
            s.h = h;
            s.color = -1;
            if (o->anim.unk10 == 0) {
                s.uv0.u = u;
                s.uv1.u = s.uv0.u + tw - 1;
            } else {
                s.uv1.u = u;
                s.uv0.u = s.uv1.u + tw - 1;
            }
            s.uv0.v = v;
            s.uv1.v = s.uv0.v + th - 1;
            r = (s16 *)gEndingObjPlaces2 + d->unk03 * 3;
            s.x = *r++;
            s.y = *r++;
            TexRect_DrawRotated(&s, *r);
        } else {
            if (o->anim.unk10 == 0) {
                s.x = (s16)o->unk30 + d->unk06;
            } else {
                s.x = (s16)o->unk30 - (w + d->unk06);
            }
            s.y = (s16)o->unk34 + d->unk08;
            s.w = w;
            s.h = h;
            s.color = -1;
            if (o->anim.unk10 == 0) {
                s.uv0.u = u;
                s.uv1.u = s.uv0.u + tw - 1;
            } else {
                s.uv1.u = u;
                s.uv0.u = s.uv1.u + tw - 1;
            }
            s.uv0.v = v;
            s.uv1.v = s.uv0.v + th - 1;
            Sprite_Draw((struct Sprite *)&s);
        }
    }
}

#define GS_RGBAQ(r, g, b, a, q)     ((u64)(r) | ((u64)(g) << 8) | ((u64)(b) << 16) | ((u64)(a) << 24) | ((u64)(q) << 32))
#define GS_UV(u, v) ((u64)(u) | ((u64)(v) << 16))
#define GS_XYZ(x, y, z) ((u64)(x) | ((u64)(y) << 16) | ((u64)(z) << 32))
void Mem_CopyQwords(void *src, void *dst, s32 n);
u32 Gs_ColorToGs(u32 color, s32 alphaOnly);
s32 Prim_SendPacket(u32 *pkt);
extern s32 gSysNoDraw;
extern s32 gDispWidth;
extern s32 gDispHeight;
extern s32 gSysFrameMode;
extern u32 gRenderFlags;
extern f32 gSysZMax; /* read through `*(f32 *)&gSysZMax`: by name, Ending_DrawSpriteF changes */

/* Sprite_Draw with the position given in floats (scaled like the width). */
void Ending_DrawSpriteF(TexRect *s, f32 fx, f32 fy)
{
    u64 *p;
    s32 x0;
    s32 y0;
    s32 x1;
    s32 y1;
    s32 w;
    s32 h;
    s32 u1;
    s32 u0;
    s32 v0;
    s32 v1;
    s32 ox;
    s32 oy;
    f32 scale;
    u32 c;

    if (gSysNoDraw != 0) {
        return;
    }
    ox = ((0x1000 - gDispWidth) / 2) << 4;
    oy = ((0x1000 - gDispHeight) / 2) << 4;
    scale = (f32)gDispWidth / 640.0f;
    x0 = 16.0f * (fx * scale);
    y0 = 16.0f * fy;
    w = 16.0f * (s->w * scale);
    h = s->h << 4;
    if (gSysFrameMode == 0) {
        y0 >>= 1;
        h >>= 1;
    }
    x0 += ox;
    x1 = x0 + w;
    y0 += oy;
    y1 = y0 + h;
    Mem_CopyQwords(gEndingSpriteGif, (void *)SCRATCHPAD, 3);
    p = (u64 *)(SCRATCHPAD + 0x28);
    c = Gs_ColorToGs(s->color, 0);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, 1);
    u0 = s->uv0.u << 4;
    u1 = s->uv1.u << 4;
    v0 = s->uv0.v << 4;
    v1 = s->uv1.v << 4;
    if (!(gRenderFlags & 0x10000)) {
        u0 += 8;
        v0 += 8;
        u1 += 8;
        v1 += 8;
    }
    *p++ = GS_UV(u0, v0);
    *p++ = GS_XYZ(x0, y0, (u32)*(f32 *)&gSysZMax);
    *p++ = GS_UV(u1, v1);
    *p++ = GS_XYZ(x1, y1, (u32)*(f32 *)&gSysZMax);
    Prim_SendPacket((u32 *)SCRATCHPAD);
}
