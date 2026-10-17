/*
 * The ending (plsel 0xD64C80-0xD67530): the staff roll with the pictures
 * and animated objects behind it, then the closing message. sub_main's
 * scene steps (unk_3240E0.c) and select's gallery run it (Ending_Update
 * each frame).
 *
 * The roll (gEndingRoll) is a page of entries (text, icons, titles,
 * pictures, and spawn marks that start objects) that scrolls up; behind
 * it, picture steps (gEndingSteps) fade one picture in and out and pan
 * another; objects (EndObj, up to ENDING_OBJ_SLOTS) move and animate
 * through tables of steps (gEndingObjDefs). The work area (EndWork) is
 * sub_main's buffer behind gObjBufBlocks; the textures come from the AFS
 * partition endtex. The structs and the states are this file's
 * (EndWork ... EndObj below, ENDING_* in plsel.h).
 */
#include "common.h"
#include "sprite.h"
#include "syssettings.h"
#include "pad.h"
#include "plsel.h"
#include "render_state.h"
#include "hw_regs.h"
#include "plsel/ending.h"   /* the texts this file uses */

/* The game compiled this file at optimization level 4 (MWCC's level-4
 * passes on top of the build's -O3,p): Ending_DrawStaffRoll and
 * Ending_DrawTitle build as the game's only there. */
#pragma optimization_level 4

void Mem_Compact(void);
void Tex_FreeRange(s32 first, s32 count);

/* The ending's work area (0xA04 bytes), in sub_main's buffer behind
 * gObjBufBlocks (0x6ADFF0). The roll scrolls up through a virtual page
 * (scrollY is the screen's top on it); the pictures behind it come in
 * steps (gEndingSteps), each a fading picture and a panning one. */
typedef struct EndWork {
    u8 state;                   /* 0x00: ENDING_* (Ending_Update) */
    u8 unk01[0x04 - 0x01];
    s32 messageTimer;           /* 0x04: frames of ENDING_MESSAGE left (210 when the roll ends) */
    u8 unk08[0x1C - 0x08];
    f32 scrollY;                /* 0x1C: the screen's top on the roll's page (starts a screen below) */
    f32 scrollSpeed;            /* 0x20: added to scrollY each frame */
    f32 scrollMax;              /* 0x24: scrollY's limit; Ending_DrawStaffRoll stops there */
    s32 frameCount;             /* 0x28: frame counter (the icons' frame steps every 4) */
    s32 iconFrame;              /* 0x2C: 0-2, the roll's icons' animation frame */
    s32 picActive;              /* 0x30: 0 after the steps' end mark */
    s32 picStarted;             /* 0x34: 1 once the screen's bottom reaches picStartY */
    s32 fadePic;                /* 0x38: the fading picture (gEndingPics), -1: none */
    s32 panPic;                 /* 0x3C: the panning picture, drawn opaque over the screen */
    s32 picStartY;              /* 0x40: the step's start on the roll's page */
    s32 picEndY;                /* 0x44: its end: the fade-out starts before it */
    f32 fadeX;                  /* 0x48: the fading picture's position */
    f32 fadeY;                  /* 0x4C */
    f32 panX;                   /* 0x50: the panning picture's position */
    f32 panY;                   /* 0x54 */
    f32 fadeVx;                 /* 0x58: the fading picture's speed */
    f32 fadeVy;                 /* 0x5C */
    f32 panVx;                  /* 0x60: the panning picture's speed */
    f32 panVy;                  /* 0x64 */
    s32 stepIndex;              /* 0x68: the step of gEndingSteps */
    u32 fadeColor;              /* 0x6C: the fading picture's colour, its alpha in the top byte */
    s32 fadeStep;               /* 0x70: the alpha's change a frame */
    s32 fadePhase;              /* 0x74: ENDING_FADE_* */
    u8 slots[ENDING_OBJ_SLOTS][0x98]; /* 0x78: the objects (EndObj; Ending_AllocObject) */
    s32 timeLeft;               /* 0x9F8: frames before the roll ends anyway */
    s32 showMessage;            /* 0x9FC: show the closing message */
    f32 messageFade;            /* 0xA00: its fade-in, 0..1 */
} EndWork;

extern EndWork *gObjBufBlocks;
#define gEndWork (gObjBufBlocks)

/* A step of the pictures behind the roll (gEndingSteps, 32 bytes each):
 * copied to the work area by Ending_LoadPicStep. */
typedef struct EndStep {
    s8 endMark;                 /* 0x00: -1 ends the steps */
    s8 fadePic;                 /* 0x01: -> EndWork.fadePic */
    s8 panPic;                  /* 0x02: -> EndWork.panPic */
    u8 unk03;                   /* not read */
    s16 startY;                 /* 0x04: -> EndWork.picStartY */
    s16 endY;                   /* 0x06: -> EndWork.picEndY */
    s16 fadeX;                  /* 0x08 */
    s16 fadeY;                  /* 0x0A */
    f32 fadeVx;                 /* 0x0C */
    f32 fadeVy;                 /* 0x10 */
    s16 panX;                   /* 0x14 */
    s16 panY;                   /* 0x16 */
    f32 panVx;                  /* 0x18 */
    f32 panVy;                  /* 0x1C */
} EndStep;

/* A pair of s16: a roll picture's width and height (gEndingPoints), or a
 * cell's screen size (EndDef.cells). */
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

/* An entry of the staff roll gEndingRoll (12 bytes), at (x, y) on the
 * roll's page. */
typedef struct EndRoll {
    s16 x;                      /* 0x0 */
    s16 y;                      /* 0x2 */
    s8 type;                    /* 0x4: ENDING_ROLL_* */
    u8 id;                      /* 0x5: text: the shadow colour; icon: the icon; picture: the picture (gEndingPoints, gEndingPictureUV); spawn: the spawn kind (Ending_SpawnObjects) */
    s16 param;                  /* 0x6: text: the font size (and line height); icon: the tint (gEndingGlyphColors); spawn: the place (gEndingObjPlaces) */
    char *text;                 /* 0x8: text and title */
} EndRoll;

/* An entry of gEndingGlyphs (10 bytes): one frame of a roll icon. */
typedef struct EndGlyph {
    u8 tex;
    u8 filter;                  /* 0x1: Gfx_SetTexFilter's texture filter */
    UV uv0;
    UV uv1;
} EndGlyph;

/* A step of an object's animation: the tables gEndingObjDefs points to. */
typedef struct EndDef {
    s8 index;                   /* 0x00: the step's own number (not read) */
    s8 loopMark;                /* 0x01: -1: the end of a loop */
    s8 next;                    /* 0x02: the next step, < 0: none */
    s8 place;                   /* 0x03: kinds 8 and 13: the place in gEndingObjPlaces2 */
    s16 wait;                   /* 0x04: frames to wait */
    s16 offX;                   /* 0x06: drawing offset */
    s16 offY;                   /* 0x08 */
    s16 moveX;                  /* 0x0A: added to the position on entering the step (x negated when flipped) */
    s16 moveY;                  /* 0x0C */
    s16 tex;                    /* 0x0E: texture (gTexHandles + ENDING_TEX_FIRST) */
    s16 cell;                   /* 0x10: cell of cells */
    u8 unk12[2];
    s16 *cells;                 /* 0x14: cells: u, v, texture w, h, screen w, h */
} EndDef;

/* An animation state (0x3C bytes). */
typedef struct EndAnim {
    s32 active;                 /* 0x00: 1 while it runs (set and cleared, not read) */
    u8 unk04[0x04];
    s32 wait;                   /* 0x08: frames left on the step */
    s32 loopCount;              /* 0x0C: loops done */
    s32 flip;                   /* 0x10: mirrored (the place's third value) */
    s32 step;                   /* 0x14: the step of defs */
    f32 moveSumX;               /* 0x18: the moves added up (not read: EndObj.x and y are the position) */
    f32 moveSumY;               /* 0x1C */
    u8 unk20[0x28 - 0x20];
    s32 unk28;                  /* 0x28: only cleared */
    s32 unk2C;                  /* 0x2C: only cleared */
    u8 unk30[0x34 - 0x30];
    s32 kind;                   /* 0x34: the animation kind, gEndingObjDefs' index */
    EndDef *defs;               /* 0x38 */
} EndAnim;

/* One of the work area's 16 objects (0x98 bytes). */
typedef struct EndObj {
    s32 used;                   /* 0x00 */
    s32 animating;              /* 0x04: 0 while a movement waits (Ending_AnimateObject returns) */
    s32 moveKind;               /* 0x08: ENDING_MOVE_* (Ending_MoveObjects) */
    s8 step;                    /* 0x0C: step of the kind's movement */
    u8 unk0D[0x10 - 0x0D];
    s32 timer;                  /* 0x10 */
    u8 unk14[0x20 - 0x14];
    f32 speed;                  /* 0x20 */
    f32 speedX;                 /* 0x24: ENDING_MOVE_FLY's x speed */
    u8 unk28[0x30 - 0x28];
    f32 x;                      /* 0x30 */
    f32 y;                      /* 0x34 */
    u8 unk38[0x40 - 0x38];
    s16 w;                      /* 0x40: the first cell's screen size (the off-screen tests) */
    s16 h;                      /* 0x42 */
    u8 unk44[0x5C - 0x44];
    EndAnim anim;               /* 0x5C */
} EndObj;

void *memset(void *, s32, u32);
void Loading_SetMessage(u32 mode);
void Loading_Stop(void);
void Tex_LoadSystemPackB(void);
void Ending_LoadTextures(void);
void Ending_LoadPicStep(EndWork *w);
void Afs_LoadPartition(s32 ptid, s32 flid, void *ptinfo, char *name);
s32 Afs_ReadFileIdle(s32 flid, s32 ptid, void *buf);
u32 Tex_CreateFromTim2(u8 *tim2, s32 mode);
extern s32 gOvlTex[];

void Gfx_SetTexFilter(s32);
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
void Ending_UpdateRoll(EndWork *w);

char *strcpy(char *dst, const char *src);
void strupr(char *);

s32 Gs_GetState(u8 reg);
s32 Gs_SetState(u8 reg, u32 v);
void Ending_DrawStepPictures(EndWork *w);
void Ending_DrawObjects(void);
void Ending_DrawStaffRoll(EndWork *w);
s32 strlen(char *s);
void Font_SetSpacing(u8 a, u8 b);
void Text_DrawShadowRgb(s16 x, s16 y, u32 c1, u32 c2, char *s);

void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
void Ending_SpawnObjects(EndRoll *);
void Ending_DrawTitle(s16 x, s16 y, char *str);

void Ending_MoveObjects(void);

extern s32 gTexHandles[];
s32 Tex_Use(s32 n, u32 *handles);
void Ending_DrawRectF(TexRect *, f32, f32);

/* .data (0xD76100-0xD78B10). gEndingRoll is the staff roll: its strings
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
EndRoll gEndingRoll[345] = {
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
u32 gEndingCells_2[48] = {
#include "plsel/00D77660.inc"
};
u32 gEndingObjDefs_2[116] = {
#include "plsel/00D77720.inc"
};
u32 gEndingCells_0[48] = {
#include "plsel/00D778F0.inc"
};
u32 gEndingObjDefs_0[188] = {
#include "plsel/00D779B0.inc"
};
u32 gEndingObjDefs_11[188] = {
#include "plsel/00D77CA0.inc"
};
u32 gEndingCells_1[24] = {
#include "plsel/00D77F90.inc"
};
u32 gEndingObjDefs_1[48] = {
#include "plsel/00D77FF0.inc"
};
u32 gEndingCells_3[16] = {
#include "plsel/00D780B0.inc"
};
u32 gEndingObjDefs_3[48] = {
#include "plsel/00D780F0.inc"
};
u32 gEndingCells_4[16] = {
#include "plsel/00D781B0.inc"
};
u32 gEndingObjDefs_4[48] = {
#include "plsel/00D781F0.inc"
};
u32 gEndingCells_8[12] = {
#include "plsel/00D782B0.inc"
};
u32 gEndingObjDefs_8[68] = {
#include "plsel/00D782E0.inc"
};
u32 gEndingObjDefs_13[32] = {
#include "plsel/00D783F0.inc"
};
u32 gEndingCells_5A[28] = {
#include "plsel/00D78470.inc"
};
u32 gEndingCells_5B[40] = {
#include "plsel/00D784E0.inc"
};
u32 gEndingObjDefs_5[140] = {
#include "plsel/00D78580.inc"
};
u32 gEndingCells_7[32] = {
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

/*
 * Sets up the ending: clears the work area, loads the textures, puts the
 * roll's page a screen below the top at its speed and limit, loads the first
 * picture step and the time-out. In the gallery the loading message shows
 * meanwhile.
 */
void Ending_Init(void)
{
    EndWork *work;

    work = gEndWork;
    if (gSysSettings.galleryPlay != 0) {
        Loading_SetMessage(5);
    }
    memset(work, 0, sizeof(EndWork));
    Tex_LoadSystemPackB();
    Ending_LoadTextures();
    work->scrollY = -448.0f;
    work->scrollSpeed = 1.2f;
    work->scrollMax = 13000.0f;
    work->stepIndex = 0;
    work->fadeStep = 0x10;
    Ending_LoadPicStep(work);
    work->timeLeft = 8990; /* frames: about 5 minutes */
    if (gSysSettings.galleryPlay != 0) {
        Loading_Stop();
    }
}

/*
 * The ending each frame (select's gallery and sub_main's scene steps call
 * it): runs the states (EndingState), then draws. In the roll, PAD_START
 * (PAD_CROSS when played from the gallery) or the time-out skips to the
 * fade-out once no fade runs. Returns 1 while it runs, 0 when it is over
 * (the music stopped and the textures freed).
 */
s32 Ending_Update(void)
{
    EndWork *work;

    work = gEndWork;
    Text_SetDefaults();
    switch (work->state) {
    case ENDING_START:
        FadeA_Start(2);
        Stream_Prepare(0, 0x134, 0, 0);
        work->state++;
    case ENDING_MUSIC:
        work->state++;
        Stream_StartPrepared(0);
        FadeA_Start(2);
        break;
    case ENDING_ROLL:
        work->timeLeft--;
        if (FadeA_GetState() != 1) {
            if (gSysSettings.galleryPlay != 0) {
                if ((gPadState[0].pressed & PAD_CROSS) || work->timeLeft == 0) {
                    work->state = ENDING_SKIPPED;
                    FadeA_Start(1);
                    break;
                }
            } else if ((gPadState[0].pressed & PAD_START) || work->timeLeft == 0) {
                work->state = ENDING_SKIPPED;
                FadeA_Start(1);
                break;
            }
        }
        Ending_UpdateRoll(work);
        break;
    case ENDING_MESSAGE:
        if (--work->messageTimer == 0) {
            work->state = ENDING_DONE;
            FadeA_Start(1);
        }
        break;
    case ENDING_DONE:
    case ENDING_SKIPPED:
        if (FadeA_GetState() != 1) {
            Movie_Stop();
            Stream_Stop(0);
            Ending_FreeTextures();
            return 0;
        }
        break;
    case ENDING_RESTART:
        FadeA_Start(2);
        work->state = ENDING_MUSIC;
        break;
    }
    Ending_Draw(work);
    Font_DrawAllPages();
    return 1;
}

/*
 * ENDING_ROLL's frame: scrolls the roll, steps the icons' frame every 4
 * frames, runs the picture step (waits for its start on the page, then
 * fades the picture in, holds it and fades it out into the next step),
 * moves the objects, and at the roll's end starts the closing message
 * (ENDING_MESSAGE). While a step waits for its start it returns at once,
 * so the objects and the end test wait too.
 */
void Ending_UpdateRoll(EndWork *work)
{
    u32 alpha;

    work->scrollY += work->scrollSpeed;
    if (work->scrollY >= work->scrollMax) {
        work->scrollY = work->scrollMax;
    }
    work->frameCount++;
    if ((work->frameCount & 3) == 0) {
        work->iconFrame++;
        work->iconFrame %= 3;
    }
    if (work->picActive != 0) {
        if (work->picStarted == 0) {
            if (work->picStartY > (s32)(448.0f + work->scrollY)) {
                return;
            }
            work->picStarted = 1;
        }
        work->fadeX += work->fadeVx;
        work->fadeY += work->fadeVy;
        work->panX += work->panVx;
        work->panY += work->panVy;
        switch (work->fadePhase) {
        case ENDING_FADE_IN:
            alpha = ((work->fadeColor & 0xFF000000) >> 24) + work->fadeStep;
            if (alpha >= 0xFF) {
                work->fadePhase = ENDING_FADE_HOLD;
                alpha = 0xFF;
            }
            work->fadeColor = (alpha << 24) | (work->fadeColor & 0xFFFFFF);
            break;
        case ENDING_FADE_HOLD:
            if ((s32)work->scrollY >= work->picEndY - (s32)(32.0f * work->scrollSpeed)) {
                work->fadePhase = ENDING_FADE_OUT;
            }
            break;
        case ENDING_FADE_OUT:
            alpha = (work->fadeColor & 0xFF000000) >> 24;
            alpha -= work->fadeStep;
            if (alpha >= 0xFF) {
                work->fadePhase = ENDING_FADE_IN;
                work->stepIndex++;
                alpha = 0;
                Ending_LoadPicStep(work);
            }
            work->fadeColor = (alpha << 24) | (work->fadeColor & 0xFFFFFF);
            break;
        }
    }
    Ending_MoveObjects();
    if (work->scrollY >= 10110.0f) {
        work->state = ENDING_MESSAGE;
        work->messageTimer = 210; /* frames: 7 s */
        work->showMessage = 1;
        work->messageFade = 0.0f;
    }
}

/*
 * Draws roll picture `picture` of the roll pictures' texture at (x, y) with
 * its size from gEndingPoints and its UVs from gEndingPictureUV; nothing
 * when its UVs are all zero or it is off the screen.
 */
void Ending_DrawRollPicture(s16 x, s16 y, u8 picture)
{
    TexRect rect;
    UV *uv;
    EndPt *size;

    size = (EndPt *)gEndingPoints + picture;
    uv = (UV *)gEndingPictureUV + picture * 2;
    Gfx_SetTexFilter(1);
    Tex_Prepare(1, ENDING_ROLL_PIC_TEX);
    Tex_Bind(ENDING_ROLL_PIC_TEX);
    rect.x = x;
    rect.y = y;
    rect.w = size->x;
    rect.h = size->y;
    rect.color = -1;
    rect.uv0.u = uv->u;
    rect.uv0.v = uv->v;
    rect.uv1.u = (++uv)->u;
    rect.uv1.v = uv->v;
    if ((rect.uv0.u != 0 || rect.uv0.v != 0 || rect.uv1.u != 0 || rect.uv1.v != 0) && rect.y < 0x1C1 && rect.y + rect.h > 0) {
        Sprite_Draw((struct Sprite *)&rect);
    }
}

/*
 * Draws frame `frame` (0-2) of roll icon `icon` (gEndingGlyphs, three
 * frames each) at (x, y), 38x32, tinted by gEndingGlyphColors[colorIdx].
 */
void Ending_DrawRollIcon(s32 icon, s16 x, s16 y, s8 colorIdx, s32 frame)
{
    TexRect rect;
    EndGlyph glyph;

    glyph = ((EndGlyph *)gEndingGlyphs)[frame + icon * 3];
    Gfx_SetTexFilter(glyph.filter);
    Tex_Prepare(1, glyph.tex);
    Tex_Bind(glyph.tex);
    rect.x = x;
    rect.y = y - 8;
    rect.w = 0x26;
    rect.h = 0x20;
    rect.color = gEndingGlyphColors[colorIdx];
    rect.uv0.u = glyph.uv0.u;
    rect.uv0.v = glyph.uv0.v;
    rect.uv1.u = glyph.uv1.u;
    rect.uv1.v = glyph.uv1.v;
    Sprite_Draw((struct Sprite *)&rect);
}

/*
 * Draws `str` upper-cased in the 20x20 letter cells of texture 8; a space
 * advances 14 pixels. The space's width is a float converted to s16 at each
 * space: so the conversion stays in the loop, as in the game's code.
 */
void Ending_DrawTitle(s16 x, s16 y, char *str)
{
    char upper[0x20];
    TexRect rect;
    char *ch;
    s32 letter;
    f32 space;

    space = 14.0f;
    Tex_Prepare(1, 8);
    Tex_Bind(8);
    Gfx_SetTexFilter(1);
    rect.w = rect.h = 0x14;
    upper[0] = 0;
    strcpy(upper, str);
    strupr(upper);
    ch = upper;
    while (*ch != 0) {
        if (*ch == ' ') {
            ch++;
            x += (s16)space;
        } else {
            letter = *ch - 'A';
            rect.uv0.u = (letter % 8) * 32;
            rect.uv0.v = (letter / 8) * 32;
            rect.uv1.u = rect.uv0.u + 0x1F;
            rect.uv1.v = rect.uv0.v + 0x1F;
            rect.color = 0xFF2691A1;
            rect.x = x;
            rect.y = y;
            Sprite_Draw((struct Sprite *)&rect);
            x += 0x14;
            ch++;
        }
    }
}

/*
 * Loads picture step work->stepIndex of gEndingSteps into the work area
 * (the two pictures, their positions, speeds and the step's span on the
 * page) and starts its fade-in; at the steps' end mark it turns the
 * pictures off.
 */
void Ending_LoadPicStep(EndWork *work)
{
    EndStep *step;

    step = (EndStep *)gEndingSteps + work->stepIndex;
    if (step->endMark == -1) {
        work->picActive = 0;
    } else {
        work->picActive = 1;
        work->picStarted = 0;
        work->fadeColor = -1;
        work->fadePhase = ENDING_FADE_IN;
        work->fadePic = step->fadePic;
        work->panPic = step->panPic;
        work->picStartY = step->startY;
        work->picEndY = step->endY;
        work->fadeX = step->fadeX;
        work->fadeY = step->fadeY;
        work->fadeVx = step->fadeVx;
        work->fadeVy = step->fadeVy;
        work->panX = step->panX;
        work->panY = step->panY;
        work->panVx = step->panVx;
        work->panVy = step->panVy;
    }
}

/*
 * Draws the step's pictures: the fading one in its colour, then the panning
 * one opaque, kept over the whole screen (its position clamped, panX
 * written back).
 */
void Ending_DrawStepPictures(EndWork *work)
{
    TexRect rect;
    s32 *handle;

    if (work->picStarted == 0) {
        return;
    }
    Gs_SetState(0x5E, 0x32);
    Gs_SetState(RS_TEX_FILTER, 0);
    if (work->fadePic != -1) {
        handle = &gTexHandles[work->fadePic + ENDING_TEX_FIRST];
        Tex_Use(1, (u32 *)handle);
        Gs_SetState(RS_TEXTURE, *handle);
        rect.uv0.u = 0;
        rect.uv0.v = 0;
        rect.uv1.u = gEndingPics[work->fadePic].w - 1;
        rect.uv1.v = gEndingPics[work->fadePic].h - 1;
        rect.w = gEndingPics[work->fadePic].dw;
        rect.h = gEndingPics[work->fadePic].dh;
        rect.color = work->fadeColor;
        rect.x = work->fadeX;
        rect.y = work->fadeY;
        Ending_DrawRectF(&rect, work->fadeX, work->fadeY);
    }
    if (work->panPic != -1) {
        handle = &gTexHandles[work->panPic + ENDING_TEX_FIRST];
        Tex_Use(1, (u32 *)handle);
        Gs_SetState(RS_TEXTURE, *handle);
        rect.uv0.u = 0;
        rect.uv0.v = 0;
        rect.uv1.u = gEndingPics[work->panPic].w - 1;
        rect.uv1.v = gEndingPics[work->panPic].h - 1;
        rect.w = gEndingPics[work->panPic].dw;
        rect.h = gEndingPics[work->panPic].dh;
        rect.color = -1;
        rect.x = work->panX;
        rect.y = work->panY;
        if (rect.x > 0) {
            rect.x = 0;
        }
        if (rect.x + rect.w < 640) {
            rect.x = 640 - rect.w;
        }
        if (rect.y > 0) {
            rect.y = 0;
        }
        if (rect.y + rect.h < 448) {
            rect.y = 448 - rect.h;
        }
        if (rect.w < (s16)work->panX + 640) {
            work->panX = rect.w - 640;
        }
        Ending_DrawRectF(&rect, work->panX, work->panY);
    }
}

/*
 * Draws the roll's entries on the screen (text, icon, title, picture, by
 * EndRoll.type), from the top of the page down to scrollMax, and starts the
 * objects of the spawn entries whose y crosses the screen's bottom this
 * frame.
 */
void Ending_DrawStaffRoll(EndWork *work)
{
    EndRoll *entry;
    s16 y;
    f32 spawnBottom;
    s16 top;
    s16 bottom;

    entry = gEndingRoll;
    top = work->scrollY;
    bottom = top + 448;
    y = entry->y;
    while (y < work->scrollMax) {
        if (y < bottom) {
            switch (entry->type) {
            case ENDING_ROLL_TEXT:
                if (top < y + entry->param) {
                    Font_SetSpacing((u8)entry->param, (u8)entry->param);
                    Text_DrawShadow(entry->x, entry->y - top, 1, entry->id, entry->text);
                }
                break;
            case ENDING_ROLL_ICON:
                if (top < y + 0x20) {
                    Ending_DrawRollIcon(entry->id, entry->x, y - top, entry->param, work->iconFrame);
                }
                break;
            case ENDING_ROLL_TITLE:
                if (top < y + 0x1A) {
                    Ending_DrawTitle(entry->x, y - top, entry->text);
                }
                break;
            case ENDING_ROLL_PICTURE:
                if (top < y + ((EndPt *)gEndingPoints)[entry->id].y) {
                    Ending_DrawRollPicture(entry->x, y - top, entry->id);
                }
                break;
            }
        }
        if (entry->type == ENDING_ROLL_SPAWN) {
            spawnBottom = 448.0f + work->scrollY + work->scrollSpeed;
            if (entry->y >= bottom && entry->y < (s16)spawnBottom) {
                Ending_SpawnObjects(entry);
            }
        }
        entry++;
        y = entry->y;
    }
}

/*
 * Draws the ending's frame: with the z-buffer off (render state 0x6C), the
 * step's pictures, the objects, the roll and, once its timer allows, the
 * closing message fading in; the render states it changes (0x6C,
 * RS_TEX_FILTER and the alpha state 0x5E) are put back after.
 */
void Ending_Draw(EndWork *work)
{
    s32 savedZbuf;
    s32 savedFilter;
    s32 savedAlpha;
    u32 alpha;
    s32 len;

    savedZbuf = Gs_GetState(0x6C);
    savedFilter = Gs_GetState(RS_TEX_FILTER);
    savedAlpha = Gs_GetState(0x5E);
    Gs_SetState(0x6C, 0);
    Ending_DrawStepPictures(work);
    Ending_DrawObjects();
    Ending_DrawStaffRoll(work);
    if (work->showMessage != 0 && work->messageTimer < 151) {
        work->messageFade += 0.05f;
        if (work->messageFade > 1.0f) {
            work->messageFade = 1.0f;
        }
        alpha = 255.0f * work->messageFade;
        len = strlen(TXT(0x00D7BA50) /* the closing message */);
        Font_SetSpacing(0x12, 0x12);
        Text_DrawShadowRgb((640 - len * 9) / 2, 0xFA, 0xFF000000, 0xFF000000 | ((u8)alpha << 16) | ((u8)alpha << 8) | (u8)alpha, TXT(0x00D7BA50) /* the closing message */);
    }
    Gs_SetState(0x6C, savedZbuf);
    Gs_SetState(RS_TEX_FILTER, savedFilter);
    Gs_SetState(0x5E, savedAlpha);
}

/*
 * Opens the endtex partition of the AFS (partition 5) and loads its
 * ENDING_TEX_COUNT textures into the handles from ENDING_TEX_FIRST.
 */
void Ending_LoadTextures(void)
{
    s32 i;

    Afs_LoadPartition(5, 0x19F3, gSysSettings.readBuf, "endtex");
    for (i = 0; i < ENDING_TEX_COUNT; i++) {
        Afs_ReadFileIdle(i, 5, gSysSettings.loadBuf);
        gOvlTex[i] = Tex_CreateFromTim2(gSysSettings.loadBuf, 4);
    }
    Mem_Compact();
}

/* Frees the ending's textures and compacts the memory. */
void Ending_FreeTextures(void)
{
    Tex_FreeRange(ENDING_TEX_FIRST, ENDING_TEX_COUNT);
    Mem_Compact();
}

/*
 * Starts animation `defTable` on `anim` at its first step, with the
 * step's move added up.
 */
static void Ending_StartAnim(EndAnim *anim, EndDef *defTable)
{
    s32 dx;
    s32 dy;

    anim->active = 1;
    anim->wait = 0;
    anim->loopCount = 0;
    anim->unk28 = 0;
    anim->unk2C = 0;
    anim->defs = defTable;
    anim->step = 0;
    anim->wait = defTable->wait;
    dx = defTable->moveX;
    if (anim->flip != 0) {
        dx = -dx;
    }
    dy = defTable->moveY;
    anim->moveSumX += dx;
    anim->moveSumY += dy;
}

/*
 * Starts object `obj` at (x, y) with movement and animation `kind`, its size
 * that of the animation's first cell. `order` is not read.
 */
static void Ending_StartObject(EndObj *obj, s32 kind, s32 x, s32 y, s32 order)
{
    s16 *firstCell;

    obj->used = 1;
    obj->moveKind = kind;
    obj->x = x;
    obj->y = y;
    Ending_StartAnim(&obj->anim, (EndDef *)gEndingObjDefs[kind]);
    obj->anim.kind = kind;
    firstCell = obj->anim.defs->cells;
    obj->w = firstCell[4];
    obj->h = firstCell[5];
}

/*
 * Steps an object's animation: waits out the step, then goes to the next
 * one (by the step's own link, with loop counts per animation kind: the
 * gotos jump to a fixed step after the loops) and applies its move. `slot`
 * is not read.
 */
static void Ending_AnimateObject(EndObj *obj, s32 slot)
{
    EndDef *def;
    s32 dx;
    s32 dy;

    if (obj->animating == 0) {
        return;
    }
    if (obj->anim.wait > 0) {
        obj->anim.wait--;
        return;
    }
    def = &obj->anim.defs[obj->anim.step];
    switch (obj->anim.kind) {
    case 0:
        if (def->loopMark == -1) {
            if (def->next < 0) {
                obj->used = 0;
                return;
            }
            obj->anim.loopCount++;
            if (obj->anim.loopCount == 2) {
                obj->anim.step = 0x12;
                obj->anim.loopCount = 0;
                goto apply;
            }
        }
        break;
    case 1:
        if (def->loopMark == -1) {
            if (def->next < 0) {
                obj->used = 0;
                return;
            }
            obj->anim.loopCount++;
            if (obj->anim.loopCount == 4) {
                obj->anim.step = 5;
                obj->anim.loopCount = 0;
                goto apply;
            }
        }
        break;
    case 2:
        if (def->loopMark == -1) {
            obj->used = 0;
            return;
        }
        break;
    case 8:
        if (def->loopMark == -1) {
            if (def->next < 0) {
                obj->used = 0;
                return;
            }
            obj->anim.loopCount++;
            if (obj->anim.loopCount == 2) {
                obj->used = 0;
                return;
            }
        }
        break;
    case 11:
        if (def->loopMark == -1) {
            if (def->next < 0) {
                obj->used = 0;
                return;
            }
            obj->anim.loopCount++;
            if (obj->anim.loopCount == 2) {
                obj->anim.step = 0x12;
                obj->anim.loopCount = 0;
                goto apply;
            }
        }
        break;
    case 13:
        if (def->next < 0) {
            obj->used = 0;
            return;
        }
        break;
    }
    def = &obj->anim.defs[obj->anim.step];
    if (def->next < 0) {
        obj->anim.active = 0;
        return;
    }
    obj->anim.step = def->next;
apply:
    def = &obj->anim.defs[obj->anim.step];
    obj->anim.wait = def->wait;
    dx = def->moveX;
    if (obj->anim.flip != 0) {
        dx = -dx;
    }
    dy = def->moveY;
    obj->x += dx;
    obj->y += dy;
}

/* Returns a free object slot of the work area, cleared, or NULL. */
static u8 *Ending_AllocObject(void)
{
    u8 *slot;
    s32 i;

    slot = gEndWork->slots[0];
    for (i = 0; i < 0x10; i++, slot += 0x98) {
        if (*(s32 *)slot == 0) {
            memset(slot, 0, 0x98);
            return slot;
        }
    }
    return NULL;
}

/*
 * Starts the objects of a spawn entry of the roll (ENDING_ROLL_SPAWN) as it
 * comes into view: entry->id picks the animation and movement kinds,
 * entry->param the place in gEndingObjPlaces (x, y, flipped).
 */
void Ending_SpawnObjects(EndRoll *entry)
{
    EndObj *obj;
    s32 order;
    s16 *place;

    obj = (EndObj *)Ending_AllocObject();
    if (obj != NULL) {
        place = (s16 *)gEndingObjPlaces + entry->param * 3;
        order = 0;
        switch (entry->id) {
        case 0:
            Ending_StartObject(obj, 0, entry->x, entry->y, 0);
            obj->moveKind = 0;
            obj->x = *place++;
            obj->y = *place++;
            obj->anim.flip = *place;
            break;
        case 1:
            Ending_StartObject(obj, 1, entry->x, entry->y, 0);
            obj->moveKind = 0;
            obj->x = *place++;
            obj->y = *place++;
            obj->anim.flip = *place;
            break;
        case 2:
            Ending_StartObject(obj, 2, entry->x, entry->y, 0);
            obj->moveKind = 0;
            obj->x = *place++;
            obj->y = *place++;
            obj->anim.flip = *place;
            break;
        case 3:
            Ending_StartObject(obj, 4, entry->x, entry->y, order++);
            obj->x = 640.0f;
            obj->y = place[1];
            obj->anim.flip = place[2];
            obj = (EndObj *)Ending_AllocObject();
            if (obj == NULL) {
                break;
            }
            Ending_StartObject(obj, 5, entry->x, entry->y, order++);
            obj->x = 640.0f;
            obj->y = place[1];
            obj->anim.flip = place[2];
            obj = (EndObj *)Ending_AllocObject();
            if (obj == NULL) {
                break;
            }
            Ending_StartObject(obj, 3, entry->x, entry->y, order++);
            obj->x = 640.0f;
            obj->y = place[1];
            obj->anim.flip = place[2];
            break;
        case 4:
            Ending_StartObject(obj, 6, entry->x, entry->y, 0);
            obj->moveKind = 2;
            obj->x = *place++;
            obj->y = *place++;
            obj->anim.flip = *place;
            break;
        case 5:
            Ending_StartObject(obj, 7, entry->x, entry->y, 0);
            obj->moveKind = 1;
            obj->x = *place++;
            place++;
            obj->anim.flip = *place;
            obj->y = 448.0f;
            break;
        case 6:
            Ending_StartObject(obj, 8, entry->x, entry->y, 0);
            obj->moveKind = 0;
            obj->x = *place++;
            obj->y = *place++;
            obj->anim.flip = *place;
            break;
        case 7:
            Ending_StartObject(obj, 9, entry->x, entry->y, 0);
            obj->moveKind = 6;
            obj->x = *place++;
            place++;
            obj->anim.flip = *place;
            obj->y = -48.0f;
            break;
        case 8:
            Ending_StartObject(obj, 0xA, entry->x, entry->y, 0);
            obj->moveKind = 7;
            obj->x = *place++;
            obj->y = *place++;
            obj->anim.flip = *place;
            if (obj->anim.flip == 0) {
                obj->x = 640.0f;
            } else {
                obj->x = -obj->w;
            }
            break;
        case 9:
            Ending_StartObject(obj, 0xB, entry->x, entry->y, 0);
            obj->moveKind = 8;
            obj->x = *place++;
            obj->y = *place++;
            obj->anim.flip = *place;
            break;
        case 10:
            Ending_StartObject(obj, 0xB, entry->x, entry->y, 0);
            obj->moveKind = 9;
            obj->x = *place++;
            obj->y = *place++;
            obj->anim.flip = *place;
            break;
        case 11:
            Ending_StartObject(obj, 0xD, entry->x, entry->y, 0);
            obj->moveKind = 0;
            obj->x = *place++;
            obj->y = *place++;
            obj->anim.flip = *place;
            break;
        }
    }
}

/*
 * Moves the objects by their movement kind (each kind's steps, with a
 * countdown in timer), frees those that are done or off the screen, then
 * animates the others. Within each kind the later steps' case labels sit
 * inside the earlier steps' tests: a step falls through into the next on
 * the frame it ends, as the game's code does.
 */
void Ending_MoveObjects(void)
{
    EndWork *work = gEndWork;
    EndObj *obj;
    s32 i;

    for (i = 0, obj = (EndObj *)work->slots[0]; i < ENDING_OBJ_SLOTS; i++, obj++) {
        if (obj->used == 0) {
            continue;
        }
        switch (obj->moveKind) {
        case ENDING_MOVE_STAY:
            switch (obj->step) {
            case 0:
                obj->timer = 300;
                obj->animating = 1;
                obj->step++;
            case 1:
                if (--obj->timer == 0) {
                    obj->animating = 0;
                    obj->used = 0;
                }
                break;
            }
            break;
        case ENDING_MOVE_RISE_WITH_ROLL:
            switch (obj->step) {
            case 0:
                obj->animating = 1;
                obj->step++;
            case 1:
                obj->y -= work->scrollSpeed;
                if (obj->y + obj->h < 0.0f) {
                    obj->animating = 0;
                    obj->used = 0;
                }
                break;
            }
            break;
        case ENDING_MOVE_BOB:
            switch (obj->step) {
            case 0:
                obj->timer = 150;
                obj->animating = 1;
                obj->step++;
                obj->speed = 0.2f;
            case 1:
                obj->y += obj->speed;
                if (--obj->timer == 0) {
                    obj->timer = 150;
                    obj->speed = -0.2f;
                    obj->step++;
            case 2:
                    obj->y += obj->speed;
                    if (--obj->timer == 0) {
                        obj->animating = 0;
                        obj->used = 0;
                    }
                }
                break;
            }
            break;
        case ENDING_MOVE_WALK_LATE:
            switch (obj->step) {
            case 0:
                obj->timer = 180;
                obj->animating = 0;
                obj->step++;
            case 1:
                if (--obj->timer == 0) {
                    obj->animating = 1;
                    obj->step++;
                    if (obj->anim.flip == 0) {
                        obj->speed = -4.0f;
                    } else {
                        obj->speed = 4.0f;
                    }
            case 2:
                    obj->x += obj->speed;
                    if (obj->x + obj->w < 0.0f) {
                        obj->animating = 0;
                        obj->used = 0;
                    }
                }
                break;
            }
            break;
        case ENDING_MOVE_WALK:
            switch (obj->step) {
            case 0:
                obj->animating = 1;
                obj->step++;
                if (obj->anim.flip == 0) {
                    obj->speed = -4.0f;
                } else {
                    obj->speed = 4.0f;
                }
            case 1:
                obj->x += obj->speed;
                if (obj->x + obj->w < 0.0f) {
                    obj->animating = 0;
                    obj->used = 0;
                }
                break;
            }
            break;
        case ENDING_MOVE_WALK_SOON:
            switch (obj->step) {
            case 0:
                obj->timer = 90;
                obj->animating = 0;
                obj->step++;
            case 1:
                if (--obj->timer == 0) {
                    obj->animating = 1;
                    obj->step++;
                    if (obj->anim.flip == 0) {
                        obj->speed = -4.0f;
                    } else {
                        obj->speed = 4.0f;
                    }
            case 2:
                    obj->x += obj->speed;
                    if (obj->x + obj->w < 0.0f) {
                        obj->animating = 0;
                        obj->used = 0;
                    }
                }
                break;
            }
            break;
        case ENDING_MOVE_FALL:
            switch (obj->step) {
            case 0:
                obj->animating = 1;
                obj->step++;
                obj->speed = 8.0f;
            case 1:
                obj->y += obj->speed;
                if (obj->y + obj->h > 480.0f) {
                    obj->animating = 0;
                    obj->used = 0;
                }
                break;
            }
            break;
        case ENDING_MOVE_FLY:
            switch (obj->step) {
            case 0:
                obj->timer = 30;
                obj->animating = 1;
                obj->step++;
                obj->speed = 0.4f;
                if (obj->anim.flip == 0) {
                    obj->speedX = -2.0f;
                } else {
                    obj->speedX = 2.0f;
                }
            case 1:
                obj->x += obj->speedX;
                obj->y += obj->speed;
                if (--obj->timer != 0) {
                    if (obj->anim.flip == 0) {
                        if (obj->x < -(obj->w * 2)) {
                            obj->animating = 0;
                            obj->used = 0;
                        }
                    } else if (obj->x >= 640.0f) {
                        obj->animating = 0;
                        obj->used = 0;
                    }
                    break;
                }
                obj->timer = 30;
                obj->speed = -0.4f;
                obj->step++;
            case 2:
                obj->x += obj->speedX;
                obj->y += obj->speed;
                if (--obj->timer != 0) {
                    if (obj->anim.flip == 0) {
                        if (obj->x < -(obj->w * 2)) {
                            obj->animating = 0;
                            obj->used = 0;
                        }
                    } else if (obj->x >= 640.0f) {
                        obj->animating = 0;
                        obj->used = 0;
                    }
                    break;
                }
                obj->timer = 30;
                obj->speed = 0.4f;
                obj->step--;
                break;
            }
            break;
        case ENDING_MOVE_SINK:
            switch (obj->step) {
            case 0:
                obj->animating = 1;
                obj->step++;
            case 1:
                obj->y += 1.2f;
                if (obj->y >= 450.0f) {
                    obj->animating = 0;
                    obj->used = 0;
                }
                break;
            }
            break;
        case ENDING_MOVE_RISE:
            switch (obj->step) {
            case 0:
                obj->animating = 1;
                obj->step++;
            case 1:
                obj->y -= 1.2f;
                if (obj->y + obj->h < 0.0f) {
                    obj->animating = 0;
                    obj->used = 0;
                }
                break;
            }
            break;
        }
        if (obj->used != 0 && obj->step != 0) {
            Ending_AnimateObject(obj, i);
        }
    }
}

void Gfx_SetBlend(s32, s32);
void TexRect_DrawRotated(TexRect *r, u16 angle); /* unit's own declaration: the angle as u16 here; with the definition's s32 (sub_main/unk_363550.c), Ending_DrawObjects' code differs */

/*
 * Draws the objects: the current step's cell (texture rectangle and size,
 * mirrored when flipped); animation kinds 8 and 13 are drawn rotated at the
 * places of gEndingObjPlaces2.
 */
void Ending_DrawObjects(void)
{
    TexRect rect;
    EndObj *obj;
    EndDef *def;
    EndPt *cellPt;
    s16 *placeRow;
    s32 i;
    s16 texU;
    s16 texV;
    s16 texW;
    s16 texH;
    s16 w;
    s16 handle;

    obj = (EndObj *)gEndWork->slots[0];
    for (i = 0; i < ENDING_OBJ_SLOTS; i++, obj++) {
        if (obj->used == 0) {
            continue;
        }
        def = &obj->anim.defs[obj->anim.step];
        cellPt = (EndPt *)(def->cells + def->cell * 6);
        Gfx_SetBlend(4, 5);
        Tex_Prepare(1, def->tex + 0xA3);
        Tex_Bind(def->tex + 0xA3);
        Gs_SetState(RS_TEX_FILTER, 0);
        texU = cellPt->x;
        texV = cellPt->y;
        texW = (++cellPt)->x;
        texH = cellPt->y;
        w = (++cellPt)->x;
        handle = cellPt->y;
        if (obj->anim.kind == 8 || obj->anim.kind == 13) {
            rect.w = w;
            rect.h = handle;
            rect.color = -1;
            if (obj->anim.flip == 0) {
                rect.uv0.u = texU;
                rect.uv1.u = rect.uv0.u + texW - 1;
            } else {
                rect.uv1.u = texU;
                rect.uv0.u = rect.uv1.u + texW - 1;
            }
            rect.uv0.v = texV;
            rect.uv1.v = rect.uv0.v + texH - 1;
            placeRow = (s16 *)gEndingObjPlaces2 + def->place * 3;
            rect.x = *placeRow++;
            rect.y = *placeRow++;
            TexRect_DrawRotated(&rect, *placeRow);
        } else {
            if (obj->anim.flip == 0) {
                rect.x = (s16)obj->x + def->offX;
            } else {
                rect.x = (s16)obj->x - (w + def->offX);
            }
            rect.y = (s16)obj->y + def->offY;
            rect.w = w;
            rect.h = handle;
            rect.color = -1;
            if (obj->anim.flip == 0) {
                rect.uv0.u = texU;
                rect.uv1.u = rect.uv0.u + texW - 1;
            } else {
                rect.uv1.u = texU;
                rect.uv0.u = rect.uv1.u + texW - 1;
            }
            rect.uv0.v = texV;
            rect.uv1.v = rect.uv0.v + texH - 1;
            Sprite_Draw((struct Sprite *)&rect);
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
extern f32 gSysZMax; /* read through `*(f32 *)&gSysZMax`: by name, Ending_DrawRectF's code differs */

/*
 * Draws a texture rectangle (as Sprite_Draw does) at a position given in
 * floats: x and the width scaled to the display's width, y halved in field
 * mode, half a texel added when the textures are not filtered.
 */
void Ending_DrawRectF(TexRect *rect, f32 posX, f32 posY)
{
    u64 *pkt;
    s32 x0;
    s32 y0;
    s32 x1;
    s32 y1;
    s32 width;
    s32 height;
    s32 u1;
    s32 u0;
    s32 v0;
    s32 v1;
    s32 offX;
    s32 offY;
    f32 scale;
    u32 gsColor;

    if (gSysNoDraw != 0) {
        return;
    }
    offX = ((0x1000 - gDispWidth) / 2) << 4;
    offY = ((0x1000 - gDispHeight) / 2) << 4;
    scale = (f32)gDispWidth / 640.0f;
    x0 = 16.0f * (posX * scale);
    y0 = 16.0f * posY;
    width = 16.0f * (rect->w * scale);
    height = rect->h << 4;
    if (gSysFrameMode == 0) {
        y0 >>= 1;
        height >>= 1;
    }
    x0 += offX;
    x1 = x0 + width;
    y0 += offY;
    y1 = y0 + height;
    Mem_CopyQwords(gEndingSpriteGif, (void *)SCRATCHPAD, 3);
    pkt = (u64 *)(SCRATCHPAD + 0x28);
    gsColor = Gs_ColorToGs(rect->color, 0);
    *pkt++ = GS_RGBAQ((gsColor >> 16) & 0xFF, (gsColor >> 8) & 0xFF, gsColor & 0xFF, (gsColor >> 24) & 0xFF, 1);
    u0 = rect->uv0.u << 4;
    u1 = rect->uv1.u << 4;
    v0 = rect->uv0.v << 4;
    v1 = rect->uv1.v << 4;
    if (!(gRenderFlags & 0x10000)) {
        u0 += 8;
        v0 += 8;
        u1 += 8;
        v1 += 8;
    }
    *pkt++ = GS_UV(u0, v0);
    *pkt++ = GS_XYZ(x0, y0, (u32)*(f32 *)&gSysZMax);
    *pkt++ = GS_UV(u1, v1);
    *pkt++ = GS_XYZ(x1, y1, (u32)*(f32 *)&gSysZMax);
    Prim_SendPacket((u32 *)SCRATCHPAD);
}
