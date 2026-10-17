#include "common.h"
#include "sprite.h"
#include "chat_log.h"
#include "quest.h"
#include "player.h"
#include "em.h"
#include "scene.h"
#include "syssettings.h"
#include "objhead.h"
#include "objattach.h"
#include "obj.h"
#include "window.h"
#include "render_state.h"
#include "area.h"

/* A window: position and size, then text. */

extern u8 gChatHudColor;
extern u8 gChatLogHead;
extern u8 gChatLogCount;
extern u8 gChatLogScroll;
extern u8 gChatLogAtEnd;
extern s8 gChatTarget;
extern s8 gChatWinState;
extern s8 gChatWinHold;
extern u8 gChatHideNames;           /* nonzero: names from gPlayerIds */
extern u16 gChatRecipients;          /* chat recipients, one bit per player */
extern u8 gChatToAll;
extern u8 gChatMsgClass;
extern s8 gChatTargetId[];         /* the recipient's name */
extern s8 gPlayerIds[][8];      /* per player */
extern char *gChatTargetTexts[];          /* the "everyone" target text */
extern s8 *gChatTargetFmt;          /* the format of a player target */

/* A network member (0x11C bytes). */
typedef struct NetMember {
    s8 id[8];                   /* 0x000 */
    s8 name[0x11C - 8];         /* 0x008 */
} NetMember;

/* unit's own declaration: the members as bytes, rows read as gNetUsers + i * 0x11C, as the
 * lobby units declare and read them (unk_76DB40.c, unk_7A44D0.c); Chat_NextTarget's name read
 * and Chat_SetRecipients view a row as a NetMember. */
extern u8 gNetUsers[];

s32 Quest_IsDrill(void);
void sprintf(s8 *out, s8 *fmt, ...);
s32 ObjBuf_FindFree(s32 n);
char *strcpy(void *dst, const void *src);
s32 strcmp(s8 *a, s8 *b);
u16 QuestParty_UserMask(void);
s32 NetUser_StateByName(u8 *name);
extern ListPanel gBoxDefs[];
extern s32 gChatTypeTexts[];
extern u8 gChatTextSizes[];

extern u8 gObjBufUsed[0x100];    /* one per block of gObjBufBlocks: in use */
extern u8 *gObjBufBlocks;          /* 0x100 blocks of 0x200 bytes */
extern Obj **gObjFreeList;        /* free list (a stack) of the first pool */
extern s32 gObjFreeCount;          /* its number of free objects */
extern Obj gObjPool[0x80];
extern Obj *gObjList;         /* the objects in use */
extern ObjAttach **gObjAttachFreeList;
extern s32 gObjAttachFreeCount;
extern ObjAttach gObjAttachPool[0x40];
/* gObjAttachPool[0x3F]: a separate symbol, as a HI16 of gObjAttachPool + 0x342C
 * between the HI16 and LO16 of gObjAttachPool is paired wrongly by the linker. */
extern ObjAttach D_005BA4FC;
extern ObjAttach *gObjAttachList;
extern Em gEms[];
extern u8 gEmIsLarge[]; /* the game overlay's gEmIsLarge: 1 for the large monsters, by kind */


/* Music (BGM) state. */
typedef struct BgmEntry {
    u16 id;                     /* 0x0 */
    u16 vol;                    /* 0x2 */
} BgmEntry;

typedef struct BgmEntry8 {
    u16 id;
    u8 vol;
    u8 unk3;
} BgmEntry8;

typedef struct BgmAlt {
    u16 area;                   /* 0x0: 0xFFFF ends the list */
    u16 variant;                /* 0x2: compared with gBgmVariant */
    u16 id;                     /* 0x4 */
    u8 vol;                     /* 0x6 */
    u8 unk7;
} BgmAlt;

typedef struct BgmKind {
    u16 kind;                   /* 0x0: monster kind */
    u16 id;                     /* 0x2 */
} BgmKind;

typedef struct Jingle {
    u8 variant;                 /* 0x0: 0xFF ends the list */
    u8 variant2;                /* 0x1 */
    u8 a;                       /* 0x2 */
    u8 b;                       /* 0x3 */
} Jingle;

typedef struct JingleList {
    u16 area;                   /* 0x0: 0xFFFF ends the list */
    Jingle *list;               /* 0x4 */
} JingleList;

typedef struct Reverb {
    s32 mode0;                  /* 0x0 */
    s32 depth0;                 /* 0x4 */
    s32 mode1;                  /* 0x8 */
    s32 depth1;                 /* 0xC */
} Reverb;

extern u16 gBgmId;          /* the music playing (0xFF: none) */
extern u8 gBgmVolume;           /* its volume */
extern u8 gBgmVariant;
extern u8 gBgmIndex;
extern u8 gBgmPaused;           /* nonzero: no music updates */
extern u8 gBgmFieldPm;
extern u8 gBgmFieldSeason;
extern u8 D_005BA780[];
extern BgmEntry gBgmFieldEvents[];
extern BgmEntry gBgmFieldSeasons[];
extern BgmEntry gBgmFieldTimes[];
extern u16 gBgmArea83[];        /* id, volume pairs */
extern u16 gBgmArea84[];
extern u16 gBgmAreasADtoAF[];
extern u16 gBgmFarm[];
extern u16 gBgmAreas56and57[];        /* id, volume, volume */
extern BgmEntry gAreaBgm[];
extern s16 gBgmTimers[];
extern BgmEntry8 gAreaMusic[];
extern Reverb gReverbs[];
extern u8 gAreaReverbIdx[];
extern BgmAlt gAreaMusicAlts[];
extern JingleList gAreaJingles[];
extern BgmKind gBattleBgmKinds[];
extern u16 D_0043C830[];
extern BgmEntry gAreaBgmVolumes[];

u16 Rand_Next(s32 stream);
s32 Player_IsLocal(void *obj);
s32 Quest_IsRunning(s32 ending);
void Sound_SetEffectOnce(s32 core, s32 mode, u32 depth, s32 delay, s32 feedback);
s32 Quest_IsVersus(void);
u16 Map_DayId(u16 area);
void Bgm_PlayField(s32 type, s32 i);
void Bgm_PlayAreaMusic(u16 area);
void Jingle_Play(void *obj, s32 id);
s32 func_003676D0(void);
s32 Bgm_IsEventTune(void);
void Bgm_PlayBattle(void);
s32 Bgm_AreaVolume(u8 area);
u16 Bgm_GetAreaMusic(u16 area);
void Stream_Play(s32 a, s32 id);
void Stream_PlayMusicVol(s32 a, s32 id);
void Stream_PlayLevel(s32 a, s32 id, s32 vol);
/* unit's own declaration: with the definition's void (s32 ch, s32 flid, s16 frames, s16 level) (main/unk_1937A0.c), Bgm_PlayAreaMusic: 250 of 407 words differ, Bgm_Update: 100 of 242 words differ */
void Stream_PlayFadeIn(s32 a, s32 id, s32 fade, s32 vol);
void Stream_Pause(s32 a, s32 b);
void Stream_Stop(s32 a);
void Stream_SetVol(s32 a, s32 b);
s32 Stream_GetStat(s32 a);
void Stream_FadeOut(s32 ch, s32 frames);
void Stream_FadeToMax(s32 ch, s16 frames);
/* unit's own declaration: with the definition's void (s32 ch, s16 frames, s16 level) (main/unk_193CB0.c), Bgm_FadeInFromZero: 8 of 17 words differ, Bgm_FadeToVolume: 4 of 6 words differ ... */
void Stream_FadeTo(s32 a, s32 fade, s32 vol);
s32 ElderRaid_InState1(void);
s32 Village_Is3924Kind4(void);
u8 Village_AverageLevel(void);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
s32 LobbyTime_IsPm(void);
s32 LobbyWorld_GetSeason(void);
void LobbyBgm_Update(void);
s32 LobbyBgm_GetState(void);

/* A 2D primitive queued for drawing (0x50 bytes, gPrims). */
typedef struct Prim {
    u8 flags;                   /* 0x00: bits 0-1 kind, 2, 3 */
    u8 unk1;                    /* 0x01 */
    u16 tex;                    /* 0x02 */
    f32 z;                      /* 0x04 */
    u8 data[0x50 - 0x08];       /* 0x08: passed to the draw function */
} Prim;

typedef struct Line {
    s16 x0;                     /* 0x0 */
    s16 y0;                     /* 0x2 */
    s16 x1;                     /* 0x4 */
    s16 y1;                     /* 0x6 */
    s32 color;                  /* 0x8 */
} Line;

typedef struct Quad {
    s16 x;                      /* 0x00 */
    s16 y;                      /* 0x02 */
    s16 w;                      /* 0x04 */
    s16 h;                      /* 0x06 */
    s32 color;                  /* 0x08 */
    s32 uv0;                    /* 0x0C */
    s32 uv1;                    /* 0x10 */
} Quad;

extern Prim *gPrims;
extern s16 gPrimCount;          /* number of queued primitives */
extern s32 gTexHandles[];
extern u8 gViewMatrix[];
extern s32 D_0043C820[];
extern u16 gFrameTick;

void Camera_SetPreset(s32 a);
void Gfx_ResetRenderStates(s32 a);
void Mtx_Identity(f32 *m);
struct MtxSlot;
void MtxSlot_Store(s32 i, struct MtxSlot *m);
s32 Gs_SetState(u8 reg, u32 v);
void Prim_Draw(Prim *p);
/* A textured rectangle, drawn rotated by TexRect_DrawRotatedAt. */


typedef struct TexTri {
    s16 x0;                     /* 0x00 */
    s16 y0;                     /* 0x02 */
    s16 x1;                     /* 0x04 */
    s16 y1;                     /* 0x06 */
    s16 x2;                     /* 0x08 */
    s16 y2;                     /* 0x0A */
    s32 color;                  /* 0x0C */
    UV uv0;                     /* 0x10 */
    UV uv1;                     /* 0x14 */
    UV uv2;                     /* 0x18 */
} TexTri;

void TexRect_DrawRotatedAt(TexRect *r, s16 cx, s16 cy, u32 angle);
void Vec_RotateZ(f32 *v, f32 a);
struct PrimTexTri;
s32 Prim_DrawTexTri(struct PrimTexTri *t);
void Gfx_SetTexFilter(s32 a);
void Gfx_SetBlend(s32 a, s32 b);
void Tex_Bind(s32 i);
void Tex_Prepare(s32 n, s32 tex);
struct PrimLineF;
s32 Prim_FillRectF(struct PrimLineF *r);
struct PrimSpriteF;
s32 Prim_DrawSpriteF(struct PrimSpriteF *sp);
struct PrimRect3D;
s32 Prim_DrawRect3D(struct PrimRect3D *r);
struct PrimSprite3D;
s32 Prim_DrawSprite3D(struct PrimSprite3D *r);
struct PrimTexTri3D;
s32 Prim_DrawTexTri3D(struct PrimTexTri3D *r);
struct PrimLine;
s32 Prim_DrawLine(struct PrimLine *l);
s32 Prim_FillRect(struct PrimLine *r);
struct Sprite;
s32 Sprite_Draw(struct Sprite *q);

/* A sprite description (see Sprite_Queue). */
typedef struct SprDef {
    s32 tex;                    /* 0x00 */
    f32 pos[3];                 /* 0x04 */
    f32 size[2];                /* 0x10 */
    f32 scale[2];               /* 0x18 */
    f32 uv[4];                  /* 0x20 */
    f32 alpha;                  /* 0x30 */
    s32 unk34;                  /* 0x34 */
    s32 flags;                  /* 0x38 */
    u32 rgb : 24;               /* 0x3C */
    u32 unk3C : 8;
    f32 rot[3];                 /* 0x40: degrees */
} SprDef;

typedef struct Vec3 {
    f32 x;
    f32 y;
    f32 z;
} Vec3;

/* Prim.data for kinds 0 and 2. */
typedef struct PrimQuad {
    f32 v0[4];                  /* 0x00 */
    f32 v1[4];                  /* 0x10 */
    u32 color;                  /* 0x20 */
    f32 uv[4];                  /* 0x24: u0, v0, u1, v1 */
} PrimQuad;

/* Prim.data for kinds 1 and 3. */
typedef struct PrimFlat {
    Vec3 p0;                    /* 0x00 */
    Vec3 p1;                    /* 0x0C */
    u32 color;                  /* 0x18 */
    f32 uv[4];                  /* 0x1C */
} PrimFlat;

/* Prim.data for kind 3 when rotated: a triangle. */
typedef struct PrimTri {
    Vec3 v[3];                  /* 0x00 */
    u32 c[3];                   /* 0x24 */
    f32 uv[3][2];               /* 0x30 */
} PrimTri;

typedef union PrimData {
    PrimQuad quad;
    PrimFlat flat;
    PrimTri tri;
} PrimData;

f32 Math_Floor(f32 x);
void *memcpy(void *d, void *s, u32 n);

void Mtx_Inverse(f32 *dst, f32 *src);
void Mtx_SetRotZYX(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Mul(f32 *dst, f32 *a, f32 *b);
f32 *Mtx_RotateVec(f32 *m, f32 *v, f32 *out);

void *memset(void *p, s32 c, u32 n);
s32 Kbd_KeysShown(void);
void Kbd_Draw(u8 a);
void Kbd_GetPos(f32 *x, s16 *y);
struct Frame;
void WindowFrame_Draw(struct Frame *b, s16 *off);
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
void Gfx_DrawGauge(s16 x, s16 y, s16 w, s16 h, s32 flags);
void StageObj_InitPool(void);
void StageObj_FreeUnkept(void);
void ObjBuf_MarkFree(s32 start, s32 n);
void Obj_InitPool(void);
void ObjAttach_InitPool(void);
void Obj_FreeUnkept(void);
void ObjAttach_FreeUnkept(void);
void Obj_Free(Obj *p);
void ObjAttach_Free(ObjAttach *p);
s32 Area_NoBarrelBombs(u16 a);
u16 Area_DayId(u16 a);

void Chat_DrawKbdBox(s32 color)
{
    f32 x;
    Frame b;

    Kbd_GetPos(&x, &b.y);
    if (Kbd_KeysShown() != 0) {
        b.x = x;
        b.y -= 3;
        b.w = 0x1E8;
        b.h = 0x66;
    } else {
        b.x = (s16)x - 0x38;
        b.y -= 3;
        b.w = 0x220;
        b.h = 0x1A;
    }
    b.style = 0x80000003;
    b.color = color;
    b.color2 = 0;
    WindowFrame_Draw(&b, 0);
    Kbd_Draw(gSysSettings.unk31);
}

void Chat_SetTarget(s8 target)
{
    gChatTarget = target;
}

/* The ids are read as byte rows (gNetUsers + i * 0x11C), the name through a NetMember view of
 * the row ((NetMember *)gNetUsers)[i].name, with gNetUsers declared as the lobby units declare
 * it (u8[]). Through NetMember alone (no view) 40 of 177 words differ: MWCC folds the name's
 * offset into the array's address. */
s32 Chat_NextTarget(s8 *out, s32 next)
{
    u32 i;
    u32 max;
    Player *pl;

    if (gChatTarget < 0) {
        return -1;
    }
    if ((u8)next) {
        gChatTarget++;
    }
    if (gChatTarget == 0) {
        out[0] = 0;
        return 0;
    }
    if (gChatTarget == 1) {
        if (gScene.online != 0) {
            strcpy(out, gChatTargetTexts[0]);
            return 3;
        }
        gChatTarget++;
    }
    if (gScene.online == 0) {
        max = 4;
    } else {
        max = 10;
    }
    i = gChatTarget - 2;
    while (1) {
        if (i == gScene.localPlayer) {
            i++;
        }
        if (i >= max) {
            gChatTarget = 0;
            out[0] = 0;
            return 0;
        }
        pl = &gPlayers[i];
        if (pl->active != 0) {
            if (gScene.online == 0) {
                if (Quest_IsDrill() != 1 || gScene.allLocal == 0 || gScene.inQuest[i] == 1) {
                    if (gChatHideNames != 0) {
                        sprintf(out, gChatTargetFmt, gPlayerIds[i]);
                    } else {
                        sprintf(out, gChatTargetFmt, pl->name);
                    }
                    strcpy(gChatTargetId, gPlayerIds[i]);
                    goto found;
                }
            } else {
                if (pl->area == 0) {
                    i++;
                    continue;
                }
                if (gChatHideNames != 0) {
                    sprintf(out, gChatTargetFmt, gNetUsers + i * 0x11C);
                } else {
                    sprintf(out, gChatTargetFmt, ((NetMember *)gNetUsers)[i].name);
                }
                strcpy(gChatTargetId, gNetUsers + i * 0x11C);
            found:
                gChatTarget = i + 2;
                return 5;
            }
        }
        i++;
    }
}

s32 Chat_SetRecipients(s8 *msg)
{
    u8 i;
    u8 idx;

    if (msg[0] == '/') {
        for (i = 1; msg[i] != 0;) {
            if (msg[i++] == ':') {
                return -2;
            }
        }
    }
    if (gChatTarget <= 0) {
        gChatRecipients = 0xFFFF;
        gChatMsgClass = 3;
        gChatToAll = 1;
        return 0;
    }
    gChatToAll = 0;
    if (gChatTarget == 1) {
        if (gScene.online != 0) {
            if ((gChatRecipients = QuestParty_UserMask()) != 0) {
                gChatRecipients &= 0xFFFF - (1 << gScene.localPlayer);
                gChatMsgClass = 2;
                return 1;
            }
        }
        return -1;
    }
    idx = gChatTarget - 2;
    if (gScene.online == 0) {
        if (strcmp(gChatTargetId, gPlayerIds[idx]) != 0) {
            return -1;
        }
    } else {
        if (NetUser_StateByName((u8 *)((NetMember *)gNetUsers)[idx].id) != 0) {
            return -1;
        }
    }
    gChatMsgClass = 1;
    gChatRecipients = 1 << idx;
    return gChatTarget;
}

void Chat_SetWindowMode(s8 a, s8 b)
{
    gChatWinState = a;
    gChatWinHold = b;
}

void Chat_ClearLog(void)
{
    gChatLogHead = 0;
    gChatLogCount = 0;
    gChatLogAtEnd = 1;
    gChatLogScroll = 0;
}

s32 Chat_CountLogRows(void)
{
    s32 rows = 0;
    s32 i = gChatLogHead - 1;
    s32 n = gChatLogCount;

    for (; n > 0; n--, i--) {
        ChatLine *l = &gChat.lines[i & 0x3F];
        rows += l->rows;
        if (l->id[0] != 0) {
            rows++;
        }
    }
    return rows;
}

/* Draws box idx of gBoxDefs (kind type: 0 plain, 1 and 2 with a gauge of width
 * gChatTextSizes[idx], 1 to 3 with the default text gChatTypeTexts[type] when text is 0). */
void Box_Draw(s32 type, u8 idx, s32 text)
{
    ListPanel *b = &gBoxDefs[idx];
    u8 bar;

    b->flags = 0x8003;
    bar = 0;
    switch (type) {
    case 0:
        b->flags = 0x8002;
        break;
    case 1:
    case 2:
        bar = 1;
    case 3:
        if (text == 0) {
            text = gChatTypeTexts[type];
        }
        break;
    }
    ListPanel_Draw(b, (s16 *)0, (char *)text, -1);
    if (bar) {
        idx = gChatTextSizes[idx];
        Gfx_DrawGauge(b->x, b->y, idx, idx * 18 / 21, 1);
    }
}

void Chat_SetColor(s16 color)
{
    if (color < 0) {
        gChatHudColor = 0;
    } else {
        gChatHudColor = color | 0x80;
    }
}

void Obj_InitAll(void)
{
    Obj_InitPool();
    StageObj_InitPool();
    ObjAttach_InitPool();
    ObjBuf_MarkFree(0, 0x100);
}

void Obj_FreeAll(void)
{
    Obj_FreeUnkept();
    StageObj_FreeUnkept();
    ObjAttach_FreeUnkept();
}

/* j, the sum loop's counter, holds the end index (j = i + n) for the bound test: it is dead
 * there (on a later pass its value is the previous sum loop's count), a dead variable reused
 * for a later value of its type. A separate end local keeps j and the walked pointer in
 * swapped registers (a2/a3). The goto loop keeps gObjBufUsed from being hoisted, the
 * if/do-while gives the search loop its top test. */
s32 ObjBuf_FindFree(s32 n)
{
    s32 i;
    s32 sum;
    s32 j;

    i = 0;
    if (n == 0) {
        return 0;
    }
loop:
        if (i < 0x100) {
            do {
                if (gObjBufUsed[i] == 0) {
                    break;
                }
                i++;
            } while (i < 0x100);
        }
        if (i >= 0x100) {
            return -1;
        }
        j = i + n;
        if (j > 0x100) {
            return -1;
        }
        for (j = sum = 0; j < n; j++) {
            sum += gObjBufUsed[i + j];
        }
        if (sum != 0) {
            i += n;
            goto loop;
        }
        return i;
}

u8 *ObjBuf_GetBlock(s32 blk)
{
    return (u8 *)((s32)gObjBufBlocks + (blk << 9));
}

void ObjBuf_MarkUsed(s32 start, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++) {
        gObjBufUsed[start + i] = 1;
    }
}

void ObjBuf_MarkFree(s32 start, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++) {
        gObjBufUsed[start + i] = 0;
    }
}

void Obj_InitPool(void)
{
    s32 i;
    Obj *p;

    memset(gObjPool, 0, sizeof(gObjPool));
    gObjList = NULL;
    p = &gObjPool[0x7F];
    gObjFreeList = (Obj **)gObjPool;
    for (i = 0; i < 0x80; i++, p--) {
        *--gObjFreeList = p;
    }
    gObjFreeCount = 0x80;
}

void Obj_FreeUnkept(void)
{
    Obj *p = gObjList;
    Obj *q;

    while (p != NULL) {
        if (p->active != 0 && p->keep == 0) {
            q = p;
            p = p->next;
            Obj_Free(q);
        } else {
            p = p->next;
        }
    }
}

Obj *Obj_Alloc(s32 n)
{
    s32 blk;
    Obj *p;

    if ((blk = ObjBuf_FindFree(n)) < 0) {
        return NULL;
    }
    if (gObjFreeCount == 0) {
        return NULL;
    }
    gObjFreeCount--;
    p = *gObjFreeList++;
    p->prev = NULL;
    p->next = gObjList;
    gObjList = p;
    if (p->next != NULL) {
        p->next->prev = p;
    }
    p->active = 1;
    p->buf = (n > 0) ? ObjBuf_GetBlock(blk) : (u8 *)-1;
    p->blk = blk;
    p->nblk = n;
    p->keep = 0;
    ObjBuf_MarkUsed(blk, n);
    return p;
}

void Obj_Free(Obj *p)
{
    if (p->prev == NULL) {
        gObjList = p->next;
    } else {
        p->prev->next = p->next;
    }
    if (p->next != NULL) {
        p->next->prev = p->prev;
    }
    gObjFreeCount++;
    *--gObjFreeList = p;
    if (p->nblk > 0) {
        ObjBuf_MarkFree(p->blk, p->nblk);
    }
    memset(p, 0, 8);
}

void Obj_UpdateAll(void)
{
    Obj *p;

    for (p = gObjList; p != NULL; p = p->next) {
        if (p->active != 0) {
            p->update(p);
        }
    }
}

void Obj_DrawAll(void)
{
    Obj *p;

    for (p = gObjList; p != NULL; p = p->next) {
        if (p->active != 0 && p->visible != 0) {
            if (p->draw != NULL) {
                p->draw(p);
            }
        }
    }
}

void ObjAttach_InitPool(void)
{
    s32 i;
    ObjAttach *p;

    memset(gObjAttachPool, 0, sizeof(gObjAttachPool));
    gObjAttachList = NULL;
    p = &D_005BA4FC;
    gObjAttachFreeList = (ObjAttach **)gObjAttachPool;
    for (i = 0; i < 0x40; i++, p--) {
        *--gObjAttachFreeList = p;
    }
    gScene.placed1 = 0;
    gObjAttachFreeCount = 0x40;
    gScene.placed2 = 0;
}

void ObjAttach_FreeUnkept(void)
{
    ObjAttach *p = gObjAttachList;
    ObjAttach *q;

    while (p != NULL) {
        if (p->active != 0 && p->keep == 0) {
            q = p;
            p = p->next;
            ObjAttach_Free(q);
        } else {
            p = p->next;
        }
    }
}

void ObjAttach_Reset(ObjAttach *p)
{
    p->wasHit = 0;
    p->hitBy = 0;
    p->keep = 0;
    p->side = 0;
    p->hitScale = 1.0f;
    p->powerPct = 0;
}

ObjAttach *ObjAttach_Alloc(s32 n)
{
    s32 blk;
    ObjAttach *p;

    if ((blk = ObjBuf_FindFree(n)) < 0) {
        return NULL;
    }
    if (gObjAttachFreeCount == 0) {
        return NULL;
    }
    gObjAttachFreeCount--;
    p = *gObjAttachFreeList++;
    p->prev = NULL;
    p->next = gObjAttachList;
    gObjAttachList = p;
    if (p->next != NULL) {
        p->next->prev = p;
    }
    p->active = 1;
    p->buf = ObjBuf_GetBlock(blk);
    p->blk = blk;
    p->nblk = n;
    ObjBuf_MarkUsed(blk, n);
    ObjAttach_Reset(p);
    return p;
}

void ObjAttach_Free(ObjAttach *p)
{
    if (p->prev == NULL) {
        gObjAttachList = p->next;
    } else {
        p->prev->next = p->next;
    }
    if (p->next != NULL) {
        p->next->prev = p->prev;
    }
    gObjAttachFreeCount++;
    *--gObjAttachFreeList = p;
    ObjBuf_MarkFree(p->blk, p->nblk);
    memset(p, 0, 0xC);
    p->attackFlags = 0;
    p->hitList = 0;
    p->hurtList = 0;
    p->drawSlot = 0;
    p->drawSlotBuf = 0;
    p->hitFx &= 0x3F;
}

void ObjAttach_UpdateAll(void)
{
    ObjAttach *p;

    for (p = gObjAttachList; p != NULL; p = p->next) {
        if (p->active != 0) {
            if (p->delay != 0) {
                p->delay--;
            } else {
                p->update(p);
            }
        }
    }
}

void ObjAttach_DrawAll(void)
{
    ObjAttach *p;

    for (p = gObjAttachList; p != NULL; p = p->next) {
        if (p->isEm == 0) {
            Player *pl = &gPlayers[p->idx];

            if (p->active != 0 && p->visible != 0 && pl->visible != 0) {
                if (p->draw != NULL) {
                    p->draw(p);
                }
            }
        } else {
            Em *em = &gEms[p->idx];

            if (p->active != 0 && p->visible != 0 && em->visible != 0) {
                if (p->draw != NULL) {
                    p->draw(p);
                }
            }
        }
    }
}

s32 Item_RideItemUsable(void)
{
    return 1;
}

s32 Item_CanPlaceLargeBarrelBomb(void)
{
    if (gScene.area == gScene.campArea) {
        return 0;
    }
    if (Area_NoBarrelBombs(gScene.area) == 1) {
        return 0;
    }
    if (gScene.placed1 < 2) {
        return 1;
    }
    return 0;
}

s32 Item_CanUseSmallBarrelBomb(void)
{
    return Area_NoBarrelBombs(gScene.area) != 1;
}

s32 Scene_CanPlaceKind2(void)
{
    if (gScene.placed2 <= 0) {
        if (Area_DayId(gScene.area) == AREA_SNOWY_MOUNTAINS_3 && gPlayers[gScene.localPlayer].pos[1] > 100.0f) {
            return 0;
        }
        return 1;
    }
    return 0;
}

s32 Item_CanPlaceKind2(void)
{
    if (gScene.area == gScene.campArea) {
        return 0;
    }
    return Scene_CanPlaceKind2();
}

s32 Scene_InNoBarrelBombArea(void)
{
    return Area_NoBarrelBombs(gScene.area) == 1;
}

s32 Item_CanPlaceKind1(void)
{
    if (gScene.area == gScene.campArea) {
        return 0;
    }
    if (gScene.placed1 < 2) {
        return 1;
    }
    return 0;
}

s32 func_00364AE0(u8 *p, u16 r)
{
    if (p[4] == 9) {
        return 0;
    }
    if (r % 100 < 40) {
        return 1;
    }
    return 0;
}

f32 func_00364B20(u16 kind, f32 v)
{
    switch (kind) {
    case 0x15:
    case 0x2E:
    case 0x11:
    case 0x2F:
    case 0xE:
    case 0x2B:
    case 0x1A:
    case 0x2C:
    case 0x35:
        return 1.25f * v;
    }
    return v;
}

s32 Debug_IsOn(s32 unused0)
{
    return 0;
}

s32 func_00364BC0(void)
{
    return 0;
}

void func_00364BD0(s32 unused0)
{
}

void func_00364BE0(s32 unused0)
{
}

void Sprite_RotateVerts(f32 (*v)[3], f32 *c, s32 n, SprDef *s)
{
    f32 r[4];
    f32 d[4];
    f32 rot[4][4];
    f32 m[4][4];
    s32 i;

    if (s->flags & 0x2000) {
        Mtx_Inverse((f32 *) m, (f32 *) gViewMatrix);
    } else {
        Mtx_Identity((f32 *) m);
    }
    Mtx_Identity((f32 *) rot);
    Mtx_SetRotZYX((f32 *) rot, 6.2831855f * s->rot[0] / 360.0f, 6.2831855f * s->rot[1] / 360.0f, 6.2831855f * s->rot[2] / 360.0f);
    Mtx_Mul((f32 *) rot, (f32 *) rot, (f32 *) m);
    for (i = 0; i < n; i++, v++) {
        d[0] = (*v)[0] - c[0];
        d[1] = (*v)[1] - c[1];
        d[2] = (*v)[2] - c[2];
        Mtx_RotateVec((f32 *) rot, (f32 *) d, (f32 *) r);
        (*v)[0] = r[0] + c[0];
        (*v)[1] = r[1] + c[1];
        (*v)[2] = r[2] + c[2];
    }
}

void Sprite_Queue(SprDef *s)
{
    Vec3 p0;
    Vec3 p1;
    Vec3 center;
    Vec3 corners[4];
    Prim *pr = gPrims;
    PrimQuad *q;
    PrimQuad *q2;
    PrimTri *t;
    u32 color;
    s32 rgb;
    f32 sign;
    f32 w;
    f32 u0;
    f32 u1;
    f32 v0;
    f32 v1;

    if (gPrimCount != 0x100) {
        pr += gPrimCount;
        pr->tex = s->tex;
        pr->z = s->pos[2];
        pr->flags = s->unk34 & 0xF;
        if (s->flags & 0x1000) {
            pr->flags |= 8;
        }
        if (s->flags & 0x4000) {
            pr->unk1 = 1;
        } else {
            pr->unk1 = 0;
        }
        if ((pr->flags & 1) == 0) {
            sign = 1.0f;
        } else {
            sign = -1.0f;
        }
        switch (s->flags & 3) {
        case 0:
        case 1:
            if (s->flags & 0x10) {
                p0.x = s->pos[0] - s->size[0] * s->scale[0];
                p1.x = s->pos[0];
            } else {
                p0.x = s->pos[0];
                p1.x = p0.x + s->size[0] * s->scale[0];
            }
            break;
        case 2:
            w = s->size[0] * s->scale[0];
            p0.x = s->pos[0] - w / 2.0f;
            p1.x = p0.x + w;
            break;
        case 3:
            if (s->flags & 0x10) {
                p0.x = s->pos[0];
                p1.x = p0.x + s->size[0] * s->scale[0];
            } else {
                p0.x = s->pos[0] - s->size[0] * s->scale[0];
                p1.x = s->pos[0];
            }
            break;
        }
        switch ((s->flags & 0xC) >> 2) {
        case 0:
        case 1:
            if (s->flags & 0x20) {
                p0.y = s->pos[1] + sign * (-1.0f * (s->size[1] * s->scale[1]));
                p1.y = s->pos[1];
            } else {
                p0.y = s->pos[1];
                p1.y = p0.y + sign * (s->size[1] * s->scale[1]);
            }
            break;
        case 2:
            w = s->size[1] * s->scale[1];
            p0.y = s->pos[1] + sign * (-1.0f * (w / 2.0f));
            p1.y = p0.y + sign * w;
            break;
        case 3:
            if (s->flags & 0x20) {
                p0.y = s->pos[1];
                p1.y = p0.y + sign * (s->size[1] * s->scale[1]);
            } else {
                p0.y = s->pos[1] + sign * (-1.0f * (s->size[1] * s->scale[1]));
                p1.y = s->pos[1];
            }
            break;
        }
        p0.z = p1.z = s->pos[2];
        rgb = s->rgb;
        color = rgb | (u32)(255.0f * s->alpha) << 24;
        switch (pr->flags & 3) {
        case 0:
            q = (PrimQuad *)pr->data;
            q->v0[0] = Math_Floor(p0.x);
            q->v0[1] = Math_Floor(p0.y);
            q->v0[2] = p0.z;
            q->v0[3] = 1.0f;
            q->v1[0] = Math_Floor(p1.x);
            q->v1[1] = Math_Floor(p1.y);
            q->v1[2] = p1.z;
            q->v1[3] = 1.0f;
            q->color = color;
            break;
        case 1:
            ((PrimFlat *)pr->data)->p0 = p0;
            ((PrimFlat *)pr->data)->p1 = p1;
            ((PrimFlat *)pr->data)->color = color;
            break;
        case 2:
            q2 = (PrimQuad *)pr->data;
            q2->v0[0] = Math_Floor(p0.x);
            q2->v0[1] = Math_Floor(p0.y);
            q2->v0[2] = p0.z;
            q2->v0[3] = 1.0f;
            q2->v1[0] = Math_Floor(p1.x);
            q2->v1[1] = Math_Floor(p1.y);
            q2->v1[2] = p1.z;
            q2->v1[3] = 1.0f;
            q2->color = color;
            if (s->flags & 0x10) {
                u0 = s->uv[2];
                u1 = s->uv[0];
            } else {
                u0 = s->uv[0];
                u1 = s->uv[2];
            }
            if (s->flags & 0x20) {
                v0 = s->uv[3];
                v1 = s->uv[1];
            } else {
                v0 = s->uv[1];
                v1 = s->uv[3];
            }
            q2->uv[0] = u0;
            q2->uv[1] = v0;
            q2->uv[2] = u1;
            q2->uv[3] = v1;
            break;
        case 3:
            if (s->flags & 0x10) {
                u0 = s->uv[2];
                u1 = s->uv[0];
            } else {
                u0 = s->uv[0];
                u1 = s->uv[2];
            }
            if (s->flags & 0x20) {
                v0 = s->uv[3];
                v1 = s->uv[1];
            } else {
                v0 = s->uv[1];
                v1 = s->uv[3];
            }
            if (s->flags & 0x2000) {
                if (!(s->unk34 & 4)) {
                    s->unk34 |= 4;
                    s->rot[2] = 0.0f;
                    s->rot[1] = 0.0f;
                    s->rot[0] = 0.0f;
                }
            }
            if (s->unk34 & 4) {
                    t = (PrimTri *)pr->data;
                center.x = s->pos[0];
                center.y = s->pos[1];
                center.z = s->pos[2];
                corners[0] = p0;
                corners[1] = p1;
                corners[1].y = p0.y;
                corners[2] = p0;
                corners[2].y = p1.y;
                corners[3] = p1;
                Sprite_RotateVerts((f32 (*)[3])corners, &center.x, 4, s);
                t->v[0] = corners[0];
                t->v[1] = corners[1];
                t->v[2] = corners[2];
                t->c[0] = t->c[1] = t->c[2] = color;
                t->uv[0][0] = u0;
                t->uv[0][1] = v0;
                t->uv[1][0] = u1;
                t->uv[1][1] = v0;
                t->uv[2][0] = u0;
                t->uv[2][1] = v1;
                if (gPrimCount != 0xFF) {
                    memcpy(pr + 1, pr, sizeof(Prim));
                    gPrimCount++;
                    ((PrimData *)pr[1].data)->tri.v[0] = corners[1];
                    ((PrimData *)pr[1].data)->tri.v[1] = corners[2];
                    ((PrimData *)pr[1].data)->tri.v[2] = corners[3];
                    ((PrimData *)pr[1].data)->tri.uv[0][0] = u1;
                    ((PrimData *)pr[1].data)->tri.uv[0][1] = v0;
                    ((PrimData *)pr[1].data)->tri.uv[1][0] = u0;
                    ((PrimData *)pr[1].data)->tri.uv[1][1] = v1;
                    ((PrimData *)pr[1].data)->tri.uv[2][0] = u1;
                    ((PrimData *)pr[1].data)->tri.uv[2][1] = v1;
                }
            } else {
                ((PrimFlat *)pr->data)->p0 = p0;
                ((PrimFlat *)pr->data)->p1 = p1;
                ((PrimFlat *)pr->data)->color = color;
                ((PrimFlat *)pr->data)->uv[0] = u0;
                ((PrimFlat *)pr->data)->uv[1] = v0;
                ((PrimFlat *)pr->data)->uv[2] = u1;
                ((PrimFlat *)pr->data)->uv[3] = v1;
            }
            break;
        }
        gPrimCount++;
    }
}

void Prim_DrawAll(void)
{
    s32 i;
    Prim *p = gPrims;
    f32 m[4][4];

    Camera_SetPreset(0);
    MtxSlot_Store(0x21, (struct MtxSlot *) gViewMatrix);
    Mtx_Identity((f32 *) m);
    Gs_SetState(RS_MTX_SLOT(0), (u32)m);
    Gs_SetState(RS_ALPHA_REF, 0);
    for (i = 0; i < gPrimCount; i++, p++) {
        Prim_Draw(p);
    }
    Gfx_ResetRenderStates(1);
}

void Prim_Draw(Prim *p)
{
    Gfx_SetTexFilter((p->flags & 8) / 8);
    if (p->unk1 != 0) {
        Gfx_SetBlend(4, 1);
    } else {
        Gfx_SetBlend(4, 5);
    }
    switch (p->flags & 3) {
    case 0:
        Prim_FillRectF((struct PrimLineF *)p->data);
        break;
    case 1:
        Prim_DrawRect3D((struct PrimRect3D *)p->data);
        break;
    case 2:
        if (gTexHandles[p->tex] != 0) {
            Tex_Prepare(1, p->tex);
            Tex_Bind(p->tex);
            Prim_DrawSpriteF((struct PrimSpriteF *)p->data);
        }
        break;
    case 3:
        if (gTexHandles[p->tex] != 0) {
            Tex_Prepare(1, p->tex);
            Tex_Bind(p->tex);
            if (p->flags & 4) {
                Prim_DrawTexTri3D((struct PrimTexTri3D *)p->data);
            } else {
                Prim_DrawSprite3D((struct PrimSprite3D *)p->data);
            }
        }
        break;
    }
}

void TexRect_DrawRotated(TexRect *r, s32 angle)
{
    TexRect_DrawRotatedAt(r, r->x + (s16)(0.5f * r->w), r->y + (s16)(0.5f * r->h), angle);
}

void TexRect_DrawRotatedAt(TexRect *r, s16 cx, s16 cy, u32 angle)
{
    f32 d[3];
    TexTri t;
    s16 h = r->h;
    f32 a;

    r->h = h / 0.93333334f;
    d[2] = 0.0f;
    d[0] = r->x - cx;
    d[1] = r->y - cy;
    a = 6.2831855f * angle / 65536.0f;
    Vec_RotateZ(d, a);
    t.x0 = cx + d[0];
    t.y0 = cy + 0.93333334f * d[1];
    d[0] = r->x + r->w - cx;
    d[1] = r->y + r->h - cy;
    Vec_RotateZ(d, a);
    t.x1 = cx + d[0];
    t.y1 = cy + 0.93333334f * d[1];
    d[0] = r->x + r->w - cx;
    d[1] = r->y - cy;
    Vec_RotateZ(d, a);
    t.x2 = cx + d[0];
    t.y2 = cy + 0.93333334f * d[1];
    t.color = r->color;
    t.uv0 = r->uv0;
    t.uv1 = r->uv1;
    t.uv2.u = r->uv1.u;
    t.uv2.v = r->uv0.v;
    Prim_DrawTexTri((struct PrimTexTri *)&t);
    d[0] = r->x - cx;
    d[1] = r->y + r->h - cy;
    Vec_RotateZ(d, a);
    t.x2 = cx + d[0];
    t.y2 = cy + 0.93333334f * d[1];
    t.color = r->color;
    t.uv0 = r->uv0;
    t.uv1 = r->uv1;
    t.uv2.u = r->uv0.u;
    t.uv2.v = r->uv1.v;
    Prim_DrawTexTri((struct PrimTexTri *)&t);
    r->h = h;
}

void TexRect_DrawRotated2(TexRect *r, s16 cx, s16 cy, s16 cx2, s16 cy2, u32 angle1, u32 angle2)
{
    f32 d[3];
    TexTri t;
    s16 h = r->h;
    f32 a1;
    f32 a2;

    r->h = h / 0.93333334f;
    d[2] = 0.0f;
    d[0] = r->x - cx;
    d[1] = r->y - cy;
    a1 = 6.2831855f * angle1 / 65536.0f;
    Vec_RotateZ(d, a1);
    t.x0 = cx + d[0];
    t.y0 = cy + 0.93333334f * d[1];
    d[0] = t.x0 - cx2;
    d[1] = t.y0 - cy2;
    a2 = 6.2831855f * angle2 / 65536.0f;
    Vec_RotateZ(d, a2);
    t.x0 = cx2 + d[0];
    t.y0 = cy2 + 0.93333334f * d[1];
    d[0] = r->x + r->w - cx;
    d[1] = r->y + r->h - cy;
    Vec_RotateZ(d, a1);
    t.x1 = cx + d[0];
    t.y1 = cy + 0.93333334f * d[1];
    d[0] = t.x1 - cx2;
    d[1] = t.y1 - cy2;
    Vec_RotateZ(d, a2);
    t.x1 = cx2 + d[0];
    t.y1 = cy2 + 0.93333334f * d[1];
    d[0] = r->x + r->w - cx;
    d[1] = r->y - cy;
    Vec_RotateZ(d, a1);
    t.x2 = cx + d[0];
    t.y2 = cy + 0.93333334f * d[1];
    d[0] = t.x2 - cx2;
    d[1] = t.y2 - cy2;
    Vec_RotateZ(d, a2);
    t.x2 = cx2 + d[0];
    t.y2 = cy2 + 0.93333334f * d[1];
    t.color = r->color;
    t.uv0 = r->uv0;
    t.uv1 = r->uv1;
    t.uv2.u = r->uv1.u;
    t.uv2.v = r->uv0.v;
    Prim_DrawTexTri((struct PrimTexTri *)&t);
    d[0] = r->x - cx;
    d[1] = r->y + r->h - cy;
    Vec_RotateZ(d, a1);
    t.x2 = cx + d[0];
    t.y2 = cy + 0.93333334f * d[1];
    d[0] = t.x2 - cx2;
    d[1] = t.y2 - cy2;
    Vec_RotateZ(d, a2);
    t.x2 = cx2 + d[0];
    t.y2 = cy2 + 0.93333334f * d[1];
    t.color = r->color;
    t.uv0 = r->uv0;
    t.uv1 = r->uv1;
    t.uv2.u = r->uv0.u;
    t.uv2.v = r->uv1.v;
    Prim_DrawTexTri((struct PrimTexTri *)&t);
    r->h = h;
}

void TexRect_Draw(TexRect *r, s8 mode)
{
    TexTri t;
    TexRect q;
    s16 x1;
    s16 y1;
    s16 u0;
    s16 u1;
    s16 x0;
    s16 y0;
    s16 v0;
    s16 v1;

    if (mode < 2) {
        q = *r;
        if (mode == 0) {
            q.uv0.u = r->uv1.u;
            q.uv1.u = r->uv0.u;
        }
        Sprite_Draw((struct Sprite *)&q);
        return;
    }
    x0 = r->x;
    y0 = r->y;
    u0 = r->uv0.u;
    u1 = r->uv1.u;
    v0 = r->uv0.v;
    v1 = r->uv1.v;
    x1 = x0 + r->w;
    y1 = y0 + r->h;
    t.color = r->color;
    switch (mode) {
    case 2:
        t.x0 = t.x1 = x0;
        t.x2 = x1;
        t.y0 = y0;
        t.y1 = t.y2 = y1;
        t.uv0.u = u1;
        t.uv1.u = t.uv2.u = u0;
        t.uv0.v = t.uv1.v = v0;
        t.uv2.v = v1;
        Prim_DrawTexTri((struct PrimTexTri *)&t);
        t.x0 = t.x1 = x1;
        t.x2 = x0;
        t.y0 = y1;
        t.y1 = t.y2 = y0;
        t.uv0.u = u0;
        t.uv1.u = t.uv2.u = u1;
        t.uv0.v = t.uv1.v = v1;
        t.uv2.v = v0;
        Prim_DrawTexTri((struct PrimTexTri *)&t);
        break;
    case 3:
        t.x0 = t.x1 = x0;
        t.x2 = x1;
        t.y0 = y0;
        t.y1 = t.y2 = y1;
        t.uv0.u = u0;
        t.uv1.u = t.uv2.u = u1;
        t.uv0.v = t.uv1.v = v1;
        t.uv2.v = v0;
        Prim_DrawTexTri((struct PrimTexTri *)&t);
        t.x0 = t.x1 = x1;
        t.x2 = x0;
        t.y0 = y1;
        t.y1 = t.y2 = y0;
        t.uv0.u = u1;
        t.uv1.u = t.uv2.u = u0;
        t.uv0.v = t.uv1.v = v0;
        t.uv2.v = v1;
        Prim_DrawTexTri((struct PrimTexTri *)&t);
        break;
    }
}

void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color)
{
    Line l;

    l.x0 = (f32)x;
    l.y0 = y;
    l.x1 = (f32)(x + w);
    l.y1 = y + h;
    l.color = color;
    Prim_FillRect((struct PrimLine *)&l);
}

void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 color)
{
    Line l;

    x = (f32)x;
    w = (f32)w;

    l.color = color;
    l.x0 = l.x1 = x;
    l.y0 = y;
    l.y1 = y + h;
    Prim_DrawLine((struct PrimLine *)&l);
    l.x1 = x + w;
    l.x0 = w + x;
    Prim_DrawLine((struct PrimLine *)&l);
    l.y0 = l.y1 = y;
    l.x0 = x;
    Prim_DrawLine((struct PrimLine *)&l);
    l.y0 = l.y1 = h + y;
    Prim_DrawLine((struct PrimLine *)&l);
}

void Gfx_DrawGauge(s16 x, s16 y, s16 w, s16 h, s32 flags)
{
    Quad q;
    s32 t;

    q.x = x;
    q.y = y;
    q.w = w;
    q.h = h;
    q.color = D_0043C820[flags & 3];
    q.uv0 = 0xED00DC;
    q.uv1 = 0xFF00F0;
    Sprite_Draw((struct Sprite *)&q);
    if (flags & 8) {
        q.color = 0xA0FF2828;
        q.uv0 = 0xC50050;
        q.uv1 = 0xD70064;
        Sprite_Draw((struct Sprite *)&q);
    }
    if (flags & 4) {
        t = gFrameTick & 0x3F;
        if (t > 0x15) {
            q.x += w;
            q.color = -1;
            if (t < 0x2B) {
                q.w >>= 2;
                q.uv0 = 0xC500A5;
                q.uv1 = 0xD700AA;
            } else {
                q.w >>= 1;
                q.uv0 = 0xC500AC;
                q.uv1 = 0xD700B6;
            }
            Sprite_Draw((struct Sprite *)&q);
        }
    }
}

void Bgm_Init(void)
{
    memset(D_005BA780, 0, 0xA);
    gBgmId = 0xFF;
}

s32 Bgm_IsBattle(void)
{
    s16 i;
    s16 found = 0;
    Em *em = gEms;
    Player *pl = &gPlayers[gScene.localPlayer];

    if (gScene.eventRunning != 0) {
        return 0;
    }
    if (gScene.campArea == pl->area) {
        return 0;
    }
    if (gQuest.flags & 0x600) {
        return 0;
    }
    for (i = 0; i < 20; i++, em++) {
        if (em->active == 0 || em->visible == 0) {
            continue;
        }
        if (Quest_IsVersus() == 0) {
            if (em->type == 0xF || em->type == 0x2D) {
                return 0;
            }
            if (em->type == 0x3B) {
                found = 1;
            }
        }
        if (gEmIsLarge[em->type] == 1 && em->stateGroup != 5 && em->combat == 1 &&
            em->area == pl->area && em->type != 0x1D) {
            if (em->type == 0x3B) {
                if (em->rage != 0) {
                    return 2;
                }
            } else {
                return 2;
            }
        }
    }
    if (found != 0) {
        return 0;
    }
    for (pl = gPlayers, i = 0; i < gScene.playerCount; i++, pl++) {
        if (pl->active != 0 && pl->inBattle != 0 && pl->area == gScene.area) {
            return 1;
        }
    }
    return 0;
}

s32 Bgm_IsBattleState(void)
{
    if (gScene.musicState == 4 || gScene.musicState == 5) {
        return 1;
    }
    return 0;
}

s16 Area_MonsterState(u16 area);
void Jingle_PlayForArea(u16 area);
void Bgm_EnterArea(u16 area);

void Bgm_FieldUpdate(void)
{
    u8 s;
    s32 r;

    gBgmVariant = Area_MonsterState(gScene.area);
    if (gScene.musicTimer > 0) {
        gScene.musicTimer--;
    }
    if ((gScene.musicFlags & 2) && Stream_GetStat(1) >= 4) {
        if (gScene.musicState >= 2) {
            Stream_FadeTo(0, 0xF, 0x7F);
        } else {
            Stream_FadeTo(0, 0xF, gBgmVolume);
        }
        gScene.musicFlags &= ~2;
    }
    if (gScene.musicState < 8) {
        r = Quest_IsRunning(1);
        if (r != 0) {
            if (r == 1) {
                if (gQuest.flags & 2) {
                    gScene.musicState = 8;
                    gScene.musicTimer = 0x64;
                } else {
                    Stream_FadeOut(0, 0xF);
                    gScene.musicTimer = 0xF;
                    gScene.musicState = 9;
                }
            } else {
                Stream_FadeOut(0, 0xF);
                gScene.musicTimer = 0xF;
                gScene.musicState = 0xB;
            }
        }
    }
    s = gScene.musicState;
    switch (s) {
    case 0:
        if (Bgm_IsBattle() != 0) {
            gScene.musicState = 2;
            Stream_FadeOut(0, 0xF);
            gScene.musicTimer = 0xF;
        } else if (gBgmVariant != gBgmIndex && gScene.eventRunning == 0) {
            gScene.musicState++;
            Stream_FadeOut(0, 0xF);
            gScene.musicTimer = 0xF;
            Jingle_PlayForArea(gScene.area);
        }
        break;
    case 1:
        if (gScene.musicTimer <= 0) {
            Bgm_EnterArea(gScene.area);
        }
        break;
    case 2:
        if (gScene.musicTimer <= 0) {
            gScene.musicState = s + 1;
            gScene.musicTimer = 0x96;
            Stream_PlayFadeIn(0, 0xD3, 0xF, 0x7F);
            gBgmId = 0xD3;
        }
        break;
    case 3:
        if (gScene.musicTimer <= 0) {
            switch (Bgm_IsBattle()) {
            case 0:
                gScene.musicState = 6;
                Stream_FadeOut(0, 0xF);
                break;
            case 1:
                break;
            case 2:
                gScene.musicState++;
                Stream_FadeOut(0, 0xF);
                gScene.musicTimer = 0xF;
                break;
            }
        }
        break;
    case 4:
        if (gScene.musicTimer <= 0) {
            gScene.musicState = s + 1;
            gScene.musicTimer = 0x96;
            Bgm_PlayBattle();
        }
        break;
    case 5:
        if (gScene.musicTimer <= 0) {
            switch (Bgm_IsBattle()) {
            case 0:
                gScene.musicTimer = 0xF;
                Stream_FadeOut(0, 0xF);
                gScene.musicState++;
                break;
            case 1:
                break;
            case 2:
                break;
            }
        }
        break;
    case 6:
        if (gScene.musicTimer <= 0) {
            gScene.musicState = 0;
            Bgm_EnterArea(gScene.area);
        }
        break;
    case 8:
        if (gScene.musicTimer > 0) {
            break;
        }
        Stream_FadeOut(0, 0xF);
        gScene.musicTimer = 0xF;
        gScene.musicState = 9;
    case 9:
        if (gScene.musicTimer > 0) {
            break;
        }
        gScene.musicState++;
        Stream_PlayFadeIn(0, 0xB6, 0xF, 0x7F);
        gBgmId = 0xB6;
    case 10:
        if (Quest_IsRunning(1) == -1) {
            gScene.musicState = 0xC;
            Stream_PlayFadeIn(0, 0xD4, 0xF, 0x7F);
            gBgmId = 0xD4;
        }
        break;
    case 11:
        if (gScene.musicTimer > 0) {
            break;
        }
        gScene.musicState = s + 1;
        Stream_PlayFadeIn(0, 0xD4, 0xF, 0x7F);
        gBgmId = 0xD4;
    case 12:
        if (Quest_IsRunning(1) == 1) {
            gScene.musicState = 0xA;
            Stream_PlayFadeIn(0, 0xB6, 0xF, 0x7F);
            gBgmId = 0xB6;
        }
        break;
    }
}

void Bgm_Update(void)
{
    u8 r;
    u16 id;
    s32 vol;

    if (gBgmPaused == 0 && LobbyBgm_GetState() != 2) {
        if (gScene.musicTimer > 0) {
            gScene.musicTimer--;
        }
        if ((gScene.musicFlags & 2) && Stream_GetStat(1) >= 4) {
            Stream_FadeTo(0, 0xF, gBgmVolume);
            gScene.musicFlags &= ~2;
        }
        if (func_003676D0() != 1) {
            switch (gScene.musicState) {
            case 0:
                if (gScene.area == AREA_HUB_0x86) {
                    if (gScene.musicTimer <= 0) {
                        gScene.musicState++;
                        Stream_FadeOut(0, 0xF);
                        gScene.musicTimer = 0xF;
                        Sound_PlaySeFlat(7, 0x18, 0);
                    }
                } else if (gBgmFieldPm != (u8)LobbyTime_IsPm() || gBgmFieldSeason != LobbyWorld_GetSeason()) {
                    gScene.musicState = 2;
                    Stream_FadeOut(0, 0xF);
                    gScene.musicTimer = 0xF;
                }
                break;
            case 1:
                if (gScene.musicTimer <= 0) {
                    if (gScene.area == AREA_HUB_0x86) {
                        gScene.musicState = 0;
                        r = Rand_Next(0) % 6;
                        if (r == gBgmIndex) {
                            gBgmIndex = (r + 1) % 6;
                        } else {
                            gBgmIndex = r;
                        }
                        id = gAreaBgm[gBgmIndex].id;
                        vol = gAreaBgm[gBgmIndex].vol;
                        gBgmVolume = vol;
                        if ((gScene.musicFlags & 2) && (gScene.bgmId == 0x28 || gScene.bgmId == 0x29)) {
                            vol = 0;
                        }
                        Stream_PlayFadeIn(0, id, 0xF, vol);
                        gBgmId = id;
                        gScene.musicTimer = gBgmTimers[gBgmIndex];
                    } else {
                        Bgm_PlayAreaMusic(gScene.area);
                    }
                }
                break;
            case 2:
                if (gScene.musicTimer <= 0) {
                    if (gScene.area == AREA_HUB_0x87 && gBgmFieldPm == 0) {
                        gScene.musicState = 0;
                        Bgm_PlayAreaMusic(gScene.area);
                    } else {
                        gScene.musicState++;
                        if (gBgmFieldSeason != LobbyWorld_GetSeason()) {
                            Bgm_PlayField(1, LobbyWorld_GetSeason());
                        } else if (gBgmFieldPm != (u8)LobbyTime_IsPm()) {
                            Bgm_PlayField(2, LobbyTime_IsPm());
                        }
                    }
                }
                break;
            case 3:
                if (Stream_GetStat(0) >= 4) {
                    gScene.musicState = 0;
                    Bgm_PlayAreaMusic(gScene.area);
                }
                break;
            }
        }
    }
    LobbyBgm_Update();
}

void Jingle_Play(void *obj, s32 id)
{
    ObjHead *o = obj;
    if (Player_IsLocal(o) == 1) {
        gScene.bgmId = id;
        if (gScene.online != 0) {
            switch (id) {
            case 4:
                gScene.musicFlags |= 2;
                Stream_FadeTo(0, 0xF, 0x1E);
                Stream_PlayMusicVol(1, id);
                break;
            case 0xB1:
            case 0x25:
            case 0x23:
            case 0x24:
            case 0x26:
            case 0x27:
            case 0x28:
            case 0x29:
                gScene.musicFlags |= 2;
                Stream_FadeTo(0, 0xF, 0);
                Stream_PlayMusicVol(1, id);
                break;
            default:
                Stream_Play(1, id);
                break;
            }
        } else {
            Stream_Play(1, id);
            if (gScene.musicState >= 2 && Quest_IsRunning(1) == 0) {
                gScene.musicFlags |= 2;
                Stream_FadeTo(0, 0xF, 0x3C);
            }
        }
    }
}

s32 func_00367690(void)
{
    if (gScene.bgmId == 0xB1 && (gScene.musicFlags & 2)) {
        return 1;
    }
    return 0;
}

s32 func_003676D0(void)
{
    switch (gScene.bgmId) {
    case 0xB1:
    case 4:
    case 0x25:
    case 0x23:
    case 0x24:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
        if (gScene.musicFlags & 2) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Bgm_IsEventTune(void)
{
    switch (gScene.bgmId) {
    case 0xD:
    case 0x1E:
    case 0x19:
    case 0x20:
    case 0x1F:
    case 0x13:
    case 0x16:
    case 4:
    case 0xB1:
        return 1;
    }
    return 0;
}

void Bgm_PlayEvent(s32 id)
{
    if (Bgm_IsEventTune() == 1 && Stream_GetStat(1) < 4) {
        return;
    }
    Stream_Play(1, id);
    gScene.bgmId = id;
    if (Quest_IsRunning(1) == 0) {
        gScene.musicFlags |= 2;
        Stream_FadeTo(0, 0xF, 0x1E);
    }
}

void Bgm_StopEvent(void *obj)
{
    ObjHead *o = obj;
    if (Player_IsLocal(o) == 1) {
        if (Bgm_IsEventTune() == 1) {
            Stream_Stop(1);
        }
        if (gScene.musicFlags & 2) {
            gScene.musicFlags &= ~2;
            if (gScene.online != 0) {
                Stream_FadeTo(0, 0xF, gBgmVolume);
            } else if (gScene.musicState < 2) {
                Stream_FadeTo(0, 0xF, gBgmVolume);
            } else {
                Stream_FadeTo(0, 0xF, 0x7F);
            }
        }
    }
}

void Bgm_StopEventFade(Player *p)
{
    if (Player_IsLocal(p) == 1) {
        switch (gScene.bgmId) {
        case 0xD:
        case 0x1E:
        case 4:
        case 0x133:
            Stream_Stop(1);
            break;
        case 0x25:
        case 0x23:
        case 0x28:
        case 0xB1:
            Stream_FadeOut(1, 0x3C);
            break;
        }
        if (gScene.musicFlags & 2) {
            gScene.musicFlags &= ~2;
            if (gScene.online != 0) {
                Stream_FadeTo(0, 0xF, gBgmVolume);
            } else if (gScene.musicState < 2) {
                Stream_FadeTo(0, 0xF, gBgmVolume);
            } else {
                Stream_FadeTo(0, 0xF, 0x7F);
            }
        }
    }
}

void func_00367A90(void)
{
    if (Quest_IsVersus() != 1) {
        gScene.musicState = 7;
        Stream_PlayFadeIn(0, 0xB8, 0xF, 0x7F);
        gBgmId = 0xB8;
    }
}

void Bgm_SetPaused(u8 off)
{
    gBgmPaused = off;
}

void Bgm_ForgetCurrent(void)
{
    gBgmId = 0xFF;
}

void Bgm_PlayField(s32 type, s32 i)
{
    u16 id;
    u16 vol;

    gScene.musicState = 3;
    switch (type) {
    case 0:
        id = gBgmFieldEvents[i].id;
        vol = gBgmFieldEvents[i].vol;
        break;
    case 1:
        id = gBgmFieldSeasons[i].id;
        vol = gBgmFieldSeasons[i].vol;
        break;
    case 2:
    default:
        id = gBgmFieldTimes[i].id;
        vol = gBgmFieldTimes[i].vol;
        break;
    }
    Stream_PlayLevel(0, id, vol);
    gBgmFieldPm = LobbyTime_IsPm();
    gBgmFieldSeason = LobbyWorld_GetSeason();
    gBgmId = id;
}

void Bgm_PlayAreaMusic(u16 area)
{
    u8 tod;
    u8 variant;
    u16 id;
    s32 vol;
    s32 k;

    gScene.musicState = 0;
    variant = LobbyTime_IsPm();
    tod = LobbyWorld_GetSeason();
    gBgmFieldPm = variant;
    gBgmFieldSeason = tod;
    switch (area) {
    case AREA_HUB_0x83:
        gBgmId = 0;
        if (ElderRaid_InState1() == 1) {
            id = 0xA2;
            vol = 100;
        } else if (Village_Is3924Kind4() == 1) {
            id = 0xA4;
            vol = 100;
        } else {
            k = tod * 4 + variant * 2;
            id = gBgmArea83[k];
            vol = gBgmArea83[k + 1];
        }
        break;
    case AREA_HUB_0x84:
        gBgmId = 0;
        if (ElderRaid_InState1() == 1) {
            id = 0xA1;
            vol = 100;
        } else if (Village_Is3924Kind4() == 1) {
            id = 0xA3;
            vol = 100;
        } else {
            k = tod * 4 + variant * 2;
            id = gBgmArea84[k];
            vol = gBgmArea84[k + 1];
        }
        break;
    case AREA_HUB_0x85:
        id = 0x37;
        vol = 0x46;
        break;
    case AREA_HUB_0x86:
        id = gAreaBgm[gBgmIndex].id;
        if (gBgmId == id) {
            Stream_Pause(0, 0);
            Stream_SetVol(0, 0);
            vol = gAreaBgm[gBgmIndex].vol;
            Stream_FadeTo(0, 0xF, vol);
        } else {
            gBgmIndex = Rand_Next(0) % 6;
            id = gAreaBgm[gBgmIndex].id;
            vol = gAreaBgm[gBgmIndex].vol;
            Stream_PlayFadeIn(0, id, 0xF, vol);
            gScene.musicTimer = gBgmTimers[gBgmIndex];
        }
        gBgmId = id;
        gBgmVolume = vol;
        break;
    case AREA_HUB_0x87:
        if (variant == 0) {
            id = gBgmArea83[tod * 4 + variant * 2];
            vol = 0x1E;
            gBgmId = 0;
            if (ElderRaid_InState1() == 1) {
                id = 0xA2;
            } else if (Village_Is3924Kind4() == 1) {
                id = 0xA4;
            }
        } else {
            id = 0xB0;
            vol = 100;
        }
        break;
    case AREA_HUB_0x88:
    default:
        id = 0x132;
        vol = 0x5A;
        break;
    case AREA_ONLINE_HOUSE_0:
        k = variant * 2;
        id = gBgmAreasADtoAF[k];
        vol = gBgmAreasADtoAF[k + 1];
        break;
    case AREA_ONLINE_HOUSE_1:
        k = variant * 2 + 4;
        id = gBgmAreasADtoAF[k];
        vol = gBgmAreasADtoAF[k + 1];
        break;
    case AREA_ONLINE_HOUSE_2:
        k = variant * 2 + 8;
        id = gBgmAreasADtoAF[k];
        vol = gBgmAreasADtoAF[k + 1];
        break;
    case AREA_HUB_0x89:
        k = (u8)Village_AverageLevel();
        k = k * 4 + variant * 2;
        id = gBgmFarm[k];
        vol = gBgmFarm[k + 1];
        break;
    case AREA_HUB_0xAA:
    case AREA_HUB_0xAB:
    case AREA_HUB_0xAC:
        k = (u8)Village_AverageLevel();
        k = k * 4 + variant * 2;
        id = gBgmFarm[k];
        vol = (u32)gBgmFarm[k + 1] >> 1;
        break;
    case AREA_HUB_0x57:
        k = variant * 3;
        id = gBgmAreas56and57[k];
        vol = gBgmAreas56and57[k + 1];
        break;
    case AREA_HOUSE:
        k = variant * 3;
        id = gBgmAreas56and57[k];
        vol = gBgmAreas56and57[k + 2];
        break;
    }
    if (area != AREA_HUB_0x86) {
        if (gBgmId == id) {
            Stream_Pause(0, 0);
            Stream_SetVol(0, 0);
            Stream_FadeTo(0, 0xF, vol);
        } else {
            Stream_PlayFadeIn(0, id, 0xF, vol);
        }
        gBgmId = id;
        gBgmVolume = vol;
    }
    Sound_SetEffectOnce(0, gReverbs[gAreaReverbIdx[area]].mode0, gReverbs[gAreaReverbIdx[area]].depth0, 0, 0);
    Sound_SetEffectOnce(1, gReverbs[gAreaReverbIdx[area]].mode1, gReverbs[gAreaReverbIdx[area]].depth1, 0, 0);
}

void Bgm_FadeInFromZero(s32 fade)
{
    Stream_SetVol(0, 0);
    Stream_FadeTo(0, fade, gBgmVolume);
}

void Bgm_FadeToVolume(s32 fade)
{
    Stream_FadeTo(0, fade, gBgmVolume);
}

void Bgm_PlayWithVolume(s32 id, s32 vol)
{
    Stream_PlayLevel(0, id, vol);
    gBgmId = id;
}

void Bgm_PlayBattle(void)
{
    Em *em = gEms;
    u16 id;
    s32 i;
    s32 j;
    u16 found;

    if (Quest_IsVersus() == 1) {
        id = 0xC4;
    } else if (Map_DayId(gScene.map) != MAP_FORTRESS) {
        id = 0xFFFF;
        for (i = 0; i < 20; i++, em++) {
            if (em->active != 0) {
                for (j = 0; j < 9; j++) {
                    if (em->type == gBattleBgmKinds[j].kind) {
                        id = gBattleBgmKinds[j].id;
                        break;
                    }
                }
                if (id != 0xFFFF) {
                    break;
                }
            }
        }
        if (id == 0xFFFF) {
            id = D_0043C830[gScene.map];
        }
    } else {
        found = 0;
        for (i = 0; i < 20; i++, em++) {
            if (em->active != 0 && em->type == 0x37) {
                found = 1;
                break;
            }
        }
        if (found == 0) {
            switch (Area_DayId(gScene.area)) {
            case AREA_FORTRESS_1:
                id = 0xBC;
                break;
            case AREA_FORTRESS_2:
                id = 0xB9;
                break;
            case AREA_FORTRESS_3:
                id = 0xBA;
                break;
            case AREA_FORTRESS_4:
                id = 0xBB;
                break;
            case AREA_FORTRESS_5:
                id = 0xB4;
                break;
            default:
                id = 0xBC;
                break;
            }
        } else {
            switch (Area_DayId(gScene.area)) {
            case AREA_FORTRESS_5:
                id = 0xD1;
                break;
            default:
                id = 0xD0;
                break;
            }
        }
    }
    Stream_PlayFadeIn(0, id, 0xF, 0x7F);
    gBgmId = id;
}

s32 Bgm_AreaVolume(u8 area)
{
    BgmEntry *e = gAreaBgmVolumes;
    u16 key = gAreaBgmVolumes[0].id;

    while (key != 0xFFFF) {
        if (key == area) {
            return e->vol;
        }
        e++;
        key = e->id;
    }
    return 0;
}

u16 Bgm_GetAreaMusic(u16 area)
{
    BgmAlt *a = gAreaMusicAlts;
    u8 vol = gAreaMusic[area].vol;
    u16 id = gAreaMusic[area].id;

    if (gBgmVariant != 0) {
        while (a->area != 0xFFFF) {
            if (a->area == area && a->variant == gBgmVariant) {
                id = a->id;
                vol = a->vol;
                break;
            }
            a++;
        }
    }
    gBgmVolume = vol;
    return id;
}

void Jingle_PlayForArea(u16 area)
{
    JingleList *l;
    Jingle *j;

    for (l = gAreaJingles;; l++) {
        if (l->area == 0xFFFF) {
            return;
        }
        if (l->area == area) {
            j = l->list;
            break;
        }
    }
    for (;; j++) {
        if (j->variant == 0xFF) {
            return;
        }
        if (j->variant == gBgmIndex && j->variant2 == gBgmVariant) {
            Sound_PlaySeFlat(j->a, j->b, 0);
            return;
        }
    }
}

void Bgm_Play(s32 id)
{
    gScene.musicState = 0;
    Stream_PlayFadeIn(0, id, 0xF, 0x7F);
    gBgmId = id;
}

void Jingle_OnAreaChange(u16 area)
{
    if (gQuest.stage != 0 && gStageHeader->randomStart != 0 && !(gScene.musicFlags & 1)) {
        switch (Map_DayId(gScene.map)) {
        case MAP_FOREST_HILLS:
        case MAP_DESERT:
        case MAP_SWAMP:
        case MAP_VOLCANO:
        case MAP_JUNGLE:
        case MAP_SNOWY_MOUNTAINS:
        case MAP_BATTLE_TOWN:
        case MAP_TOWER:
        case MAP_TOWER_B:
        case MAP_TOWER_C:
            if (gScene.campArea != area) {
                Jingle_Play(&gPlayers[gScene.localPlayer], 0x30);
            }
            break;
        }
    }
}

void Bgm_EnterArea(u16 area)
{
    s32 q;
    s32 id;

    q = Quest_IsRunning(1);
    if (q != 0) {
        if (q == 1) {
            if (gBgmId == 0xB6) {
                Stream_Pause(0, 0);
                Stream_SetVol(0, 0);
                Stream_FadeToMax(0, 0xF);
            } else {
                gScene.musicState = 0xA;
                Stream_PlayFadeIn(0, 0xB6, 0xF, 0x7F);
                gBgmId = 0xB6;
            }
        } else if (gBgmId == 0xD4) {
            Stream_Pause(0, 0);
            Stream_SetVol(0, 0);
            Stream_FadeToMax(0, 0xF);
        } else {
            gScene.musicState = 0xC;
            Stream_PlayFadeIn(0, 0xD4, 0xF, 0x7F);
            gBgmId = 0xD4;
        }
    } else {
        if (gQuest.flags & 0x800) {
            Stream_PlayFadeIn(0, 0xC3, 0xF, 0x7F);
            gBgmId = 0xC3;
            gBgmVolume = 0x7F;
        } else if (gQuest.flags & 0x400) {
            gBgmVolume = 0x7F;
            Bgm_PlayBattle();
        } else if (gScene.musicFlags == 0 && (id = Bgm_AreaVolume(area)) != 0) {
            Stream_PlayFadeIn(0, id, 0xF, 0x7F);
            gBgmId = id;
            gBgmVolume = 0x7F;
            gScene.musicFlags |= 1;
        } else {
            id = Bgm_GetAreaMusic(area);
            if (gBgmId == id) {
                Stream_Pause(0, 0);
                Stream_SetVol(0, 0);
                Stream_FadeTo(0, 0xF, gBgmVolume);
            } else {
                Stream_PlayFadeIn(0, id, 0xF, gBgmVolume);
            }
            gBgmId = id;
        }
        gScene.musicState = 0;
    }
    gBgmIndex = gBgmVariant;
    Sound_SetEffectOnce(0, gReverbs[gAreaReverbIdx[area]].mode0, gReverbs[gAreaReverbIdx[area]].depth0, 0, 0);
    Sound_SetEffectOnce(1, gReverbs[gAreaReverbIdx[area]].mode1, gReverbs[gAreaReverbIdx[area]].depth1, 0, 0);
}

/* unk_363550's .data (0x43B0E0-0x43C8C0) */
extern u32 D_0043C5D0[];
ListPanel gBoxDefs[3] = {
#include "sub_main/0043B0E0.inc"
};
BgmEntry gBgmFieldEvents[4] = {
#include "sub_main/0043B140.inc"
};
BgmEntry gBgmFieldSeasons[4] = {
#include "sub_main/0043B150.inc"
};
BgmEntry gBgmFieldTimes[4] = {
#include "sub_main/0043B160.inc"
};
u16 gBgmArea83[16] = {
#include "sub_main/0043B170.inc"
};
u16 gBgmArea84[16] = {
#include "sub_main/0043B190.inc"
};
u16 gBgmAreasADtoAF[16] = {
#include "sub_main/0043B1B0.inc"
};
u16 gBgmFarm[12] = {
#include "sub_main/0043B1D0.inc"
};
u16 gBgmAreas56and57[6] = {
#include "sub_main/0043B1E8.inc"
};
BgmEntry gAreaBgm[6] = {
#include "sub_main/0043B200.inc"
};
s16 gBgmTimers[6] = {
#include "sub_main/0043B218.inc"
};
BgmEntry8 gAreaMusic[200] = {
#include "sub_main/0043B230.inc"
};
Reverb gReverbs[10] = {
#include "sub_main/0043B550.inc"
};
u8 gAreaReverbIdx[208] = {
#include "sub_main/0043B5F0.inc"
};
u32 gAreaSeBanks[128] = {
#include "sub_main/0043B6C0.inc"
};
BgmAlt gAreaMusicAlts[68] = {
#include "sub_main/0043B8C0.inc"
};
u32 gAreaJingles_0[8] = {
#include "sub_main/0043BAE0.inc"
};
u32 gAreaJingles_20[8] = {
#include "sub_main/0043BB00.inc"
};
u32 gAreaJingles_14[8] = {
#include "sub_main/0043BB20.inc"
};
u32 gAreaJingles_27[8] = {
#include "sub_main/0043BB40.inc"
};
JingleList gAreaJingles[35] = {
#include "sub_main/0043BB60.inc"
};
u16 gQuestSoundBanks[344] = {
#include "sub_main/0043BC80.inc"
};
u32 D_0043BF30[56] = {
#include "sub_main/0043BF30.inc"
};
u32 D_0043C010[56] = {
#include "sub_main/0043C010.inc"
};
u32 D_0043C0F0[64] = {
#include "sub_main/0043C0F0.inc"
};
u32 D_0043C1F0[64] = {
#include "sub_main/0043C1F0.inc"
};
u32 D_0043C2F0[64] = {
#include "sub_main/0043C2F0.inc"
};
u32 D_0043C3F0[64] = {
#include "sub_main/0043C3F0.inc"
};
u32 D_0043C4F0[56] = {
#include "sub_main/0043C4F0.inc"
};
u32 D_0043C5D0[108] = {
#include "sub_main/0043C5D0.inc"
};
u32 gSeVolumeByType[20] = {
#include "sub_main/0043C780.inc"
};
BgmKind gBattleBgmKinds[20] = {
#include "sub_main/0043C7D0.inc"
};
s32 D_0043C820[4] = {
#include "sub_main/0043C820.inc"
};
u16 D_0043C830[8] = {
#include "sub_main/0043C830.inc"
};
u32 gTextCodeColors[12] = {
#include "sub_main/0043C840.inc"
};
BgmEntry gAreaBgmVolumes[20] = {
#include "sub_main/0043C870.inc"
};
