/*
 * Five small services of the field and the town, which the game's code
 * keeps together:
 *
 * - The chat's targets (Chat_NextTarget, Chat_SetRecipients): the command
 *   that sends a chat line to one player cycles through the players by
 *   gChatTarget (0 none, 1 the quest's party online, 2 + n player n), and
 *   the recipients' bits are set from it. Chat_DrawBox draws the chat box
 *   by the send target, Chat_DrawKbdBox the frame of the on-screen
 *   keyboard.
 * - The object pools: Obj (0x80, gObjPool) and ObjAttach (0x40,
 *   gObjAttachPool), each a free list (a stack) and a doubly linked list of
 *   the objects in use, and the 0x100 blocks of 0x200 bytes (gObjBufBlocks)
 *   an object takes for its work. Obj_InitAll and Obj_FreeAll run with the
 *   scene; the update and draw walks run every frame.
 * - The placing rules of the barrel bombs and the other placed items
 *   (Item_CanPlace*), and two helpers of the game's traps and arrows.
 * - The 2D drawing helpers: the sprite queue (Sprite_Queue and Prim_DrawAll,
 *   sprite_queue.h), the rotated textured rectangles (TexRect_*), lines,
 *   frames and the window texture's marker icon.
 * - The music manager (bgm.h): the field's state machine (Bgm_FieldUpdate:
 *   the area's music, the battle music, the music of the goal met or
 *   the quest lost), the town's
 *   (Bgm_TownUpdate: the music by season and time of day), the jingles and
 *   event tunes on the second stream channel, the areas' reverb. Its tables
 *   are this file's data.
 */
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
#include "hud.h"
#include "adx.h"
#include "sound.h"
#include "bgm.h"
#include "sprite_queue.h"
#include "lobby_state.h"

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

/* The network members' rows (NetUser) as bytes, as the lobby's code declares and reads them;
 * Chat_NextTarget and Chat_SetRecipients read a row's fields through a NetUser cast.
 * unit's own declaration: u8[] rather than the definition's NetUser rows */
extern u8 gNetUsers[];

s32 Quest_IsDrill(void);
void sprintf(s8 *out, s8 *fmt, ...);
s32 ObjBuf_FindFree(s32 n);
char *strcpy(void *dst, const void *src);
s32 strcmp(s8 *a, s8 *b);
u16 QuestParty_UserMask(void);
s32 NetUser_StateByName(u8 *name);
extern ListPanel gChatBoxPanels[];
extern s32 gChatTypeTexts[];
extern u8 gChatTextSizes[];

#define OBJ_POOL_SIZE       0x80    /* gObjPool's objects */
#define OBJ_ATTACH_POOL_SIZE 0x40   /* gObjAttachPool's */
#define OBJ_BUF_BLOCKS      0x100   /* gObjBufBlocks' blocks */
#define OBJ_BUF_BLOCK_SHIFT 9       /* of 0x200 bytes */

extern u8 gObjBufUsed[OBJ_BUF_BLOCKS];  /* one per block of gObjBufBlocks: in use */
extern u8 *gObjBufBlocks;          /* OBJ_BUF_BLOCKS blocks of 0x200 bytes */
extern Obj **gObjFreeList;        /* free list (a stack) of the first pool */
extern s32 gObjFreeCount;          /* its number of free objects */
extern Obj gObjPool[OBJ_POOL_SIZE];
extern Obj *gObjList;         /* the objects in use */
extern ObjAttach **gObjAttachFreeList;
extern s32 gObjAttachFreeCount;
extern ObjAttach gObjAttachPool[OBJ_ATTACH_POOL_SIZE];
/* gObjAttachPool[0x3F], declared as an object of its own: the linker pairs
 * the two halves of the address gObjAttachPool + 0x342C wrongly when they
 * fall between the halves of gObjAttachPool's own address. */
extern ObjAttach gObjAttachPoolLast;
extern ObjAttach *gObjAttachList;
extern Em gEms[];
extern u8 gEmIsLarge[]; /* the game overlay's gEmIsLarge: 1 for the large monsters, by kind */

extern u8 gBgmVars[];           /* the first of the music's variables, which Bgm_Init clears together */
extern BgmEntry gTownTuneEvents[];
extern BgmEntry gTownTuneSeasons[];
extern BgmEntry gTownTuneTimes[];
extern u16 gBgmHub83[];        /* id, volume pairs */
extern u16 gBgmHub84[];
extern u16 gBgmOnlineHouse[];
extern u16 gBgmFarm[];
extern u16 gBgmHouse[];        /* id, volume, volume */
extern BgmEntry gBgmTavernTracks[];
extern s16 gBgmTavernTrackFrames[];
extern AreaMusic gAreaMusic[];
extern AreaReverb gAreaReverbs[];
extern u8 gAreaReverbIdx[];
extern AreaMusicAlt gAreaMusicAlts[];
extern AreaJingles gAreaJingles[];
extern BgmKey gBattleBgmKinds[];
extern u16 gMapBattleBgm[];
extern BgmKey gAreaTunes[];

u16 Rand_Next(s32 stream);
s32 Player_IsLocal(void *obj);
s32 Quest_GetOutcome(s32 withLoss);
void Sound_SetEffectOnce(s32 core, s32 mode, u32 depth, s32 delay, s32 feedback);
s32 Quest_IsVersus(void);
u16 Map_DayId(u16 area);
void Bgm_PlayTownTune(s32 kind, s32 i);
void Bgm_PlayAreaMusic(u16 area);
void Jingle_Play(void *owner, s32 id);
s32 Bgm_IsDuckingTune(void);
s32 Bgm_IsEventTune(void);
void Bgm_PlayBattle(void);
s32 Bgm_AreaTune(u8 area);
u16 Bgm_GetAreaMusic(u16 area);
void Stream_Play(s32 a, s32 id);
void Stream_PlayMusicVol(s32 a, s32 id);
void Stream_PlayLevel(s32 a, s32 id, s32 vol);
/* unit's own declaration: frames and level as int (s16 in the definition): with s16 the calls would narrow them, which the game's calls don't */
void Stream_PlayFadeIn(s32 a, s32 id, s32 fade, s32 vol);
void Stream_Pause(s32 a, s32 b);
void Stream_Stop(s32 a);
void Stream_SetVol(s32 a, s32 b);
s32 Stream_GetStat(s32 a);
void Stream_FadeOut(s32 ch, s32 frames);
void Stream_FadeToMax(s32 ch, s16 frames);
/* unit's own declaration: frames and level as int (s16 in the definition): with s16 the calls would narrow them, which the game's calls don't */
void Stream_FadeTo(s32 a, s32 fade, s32 vol);
s32 ElderRaid_InState1(void);
s32 Village_Is3924Kind4(void);
u8 Village_AverageLevel(void);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
s32 LobbyTime_IsPm(void);
s32 LobbyWorld_GetSeason(void);
void LobbyBgm_Update(void);
s32 LobbyBgm_GetState(void);

/* The height of a screen pixel to its width (14 / 15), which the rotated
 * rectangles correct for. */
#define SCREEN_PIXEL_ASPECT 0.93333334f

/* A textured triangle (prim.h's PrimTexTri) with its texture corners as UV
 * pairs: the rotated rectangles copy a corner with one word move. */
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

extern s32 gTexHandles[];
extern u8 gViewMatrix[];
extern s32 gMarkerColors[];
extern u16 gFrameTick;

void Camera_SetPreset(s32 a);
void Gfx_ResetRenderStates(s32 a);
void Mtx_Identity(f32 *m);
struct MtxSlot;
void MtxSlot_Store(s32 i, struct MtxSlot *m);
s32 Gs_SetState(u8 reg, u32 v);
void Prim_Draw(Prim *p);

void TexRect_DrawRotatedAt(TexRect *r, s16 cx, s16 cy, u32 angle);
void Vec_RotateZ(f32 *v, f32 a);
s32 Prim_DrawTexTri(struct PrimTexTri *t);
void Gfx_SetTexFilter(s32 a);
void Gfx_SetBlend(s32 a, s32 b);
void Tex_Bind(s32 i);
void Tex_Prepare(s32 n, s32 tex);
s32 Prim_FillRectF(struct PrimLineF *r);
s32 Prim_DrawSpriteF(struct PrimSpriteF *sp);
s32 Prim_DrawRect3D(struct PrimRect3D *r);
s32 Prim_DrawSprite3D(struct PrimSprite3D *r);
s32 Prim_DrawTexTri3D(struct PrimTexTri3D *r);
s32 Prim_DrawLine(struct PrimLine *l);
s32 Prim_FillRect(struct PrimLine *r);
s32 Sprite_Draw(struct Sprite *q);

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
void Gfx_DrawMarker(s16 x, s16 y, s16 w, s16 h, s32 flags);
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

/*
 * Draws the on-screen keyboard with a frame of colour color around it: the
 * whole keyboard while its keys are shown, else only its text line.
 */
void Chat_DrawKbdBox(s32 color)
{
    f32 kbdX;
    Frame frame;

    Kbd_GetPos(&kbdX, &frame.y);
    if (Kbd_KeysShown() != 0) {
        frame.x = kbdX;
        frame.y -= 3;
        frame.w = 0x1E8;
        frame.h = 0x66;
    } else {
        frame.x = (s16)kbdX - 0x38;
        frame.y -= 3;
        frame.w = 0x220;
        frame.h = 0x1A;
    }
    frame.style = FRAME_FILL | 3;
    frame.color = color;
    frame.color2 = 0;
    WindowFrame_Draw(&frame, 0);
    Kbd_Draw(gSysSettings.unk31);
}

/* Sets the chat's target (gChatTarget): -1 off, 0 none, 1 the party, 2 + n player n. */
void Chat_SetTarget(s8 target)
{
    gChatTarget = target;
}

/*
 * Gives the chat's target as a text in out and returns its kind: 0 none
 * (out empty), 3 the quest's party (in the town: target 1), 5 a player (target
 * 2 + its index). With next set, it first moves on to the next target:
 * after the party come the active players other than the local one (in a
 * drill with every player local, only those in the quest; in the town, only
 * those in an area), then none again. A player's text is gChatTargetFmt
 * with its name, or its id while gChatHideNames is set, and its id goes
 * to gChatTargetId for Chat_SetRecipients. The quest's case jumps to the
 * town's case's tail (found), which the game's code shares.
 * In the town, a member's id is read as bytes of the row (gNetUsers + i * 0x11C)
 * and its name through the row as a NetUser: through NetUser alone the
 * compiler folds the name's offset into the array's address, which the
 * game's code doesn't.
 */
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
        if (gScene.inLobby != 0) {
            strcpy(out, gChatTargetTexts[0]);
            return 3;
        }
        gChatTarget++;
    }
    if (gScene.inLobby == 0) {
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
            if (gScene.inLobby == 0) {
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
                    sprintf(out, gChatTargetFmt, ((NetUser *)gNetUsers)[i].name);
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

/*
 * Sets the recipients of the chat line msg from the chat's target:
 * everyone (gChatRecipients 0xFFFF, class 3) when there is none, the
 * party but the local player (class 2) for target 1, else the one player
 * (class 1), if it is still the one gChatTargetId names. Returns 0 for
 * everyone, 1 for the party, the target for a player, -1 when the target
 * is gone and -2 for a command line ("/" with a ":" after it).
 */
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
        gChatRecipients = CHAT_RECIPIENTS_ALL;
        gChatMsgClass = CHAT_CLASS_ALL;
        gChatToAll = 1;
        return 0;
    }
    gChatToAll = 0;
    if (gChatTarget == 1) {
        if (gScene.inLobby != 0) {
            if ((gChatRecipients = QuestParty_UserMask()) != 0) {
                gChatRecipients &= 0xFFFF - (1 << gScene.localPlayer);
                gChatMsgClass = CHAT_CLASS_PARTY;
                return 1;
            }
        }
        return -1;
    }
    idx = gChatTarget - 2;
    if (gScene.inLobby == 0) {
        if (strcmp(gChatTargetId, gPlayerIds[idx]) != 0) {
            return -1;
        }
    } else {
        if (NetUser_StateByName((u8 *)((NetUser *)gNetUsers)[idx].id) != 0) {
            return -1;
        }
    }
    gChatMsgClass = CHAT_CLASS_PLAYER;
    gChatRecipients = 1 << idx;
    return gChatTarget;
}

/* Sets the chat window's state and hold (gChatWinState, gChatWinHold). */
void Chat_SetWindowMode(s8 state, s8 hold)
{
    gChatWinState = state;
    gChatWinHold = hold;
}

/* Empties the chat log and puts its view at the end. */
void Chat_ClearLog(void)
{
    gChatLogHead = 0;
    gChatLogCount = 0;
    gChatLogAtEnd = 1;
    gChatLogScroll = 0;
}

/* The screen rows of the chat log's lines: each line's rows, and one more
 * for the name of a line that has one. */
s32 Chat_CountLogRows(void)
{
    s32 rows = 0;
    s32 i = gChatLogHead - 1;
    s32 n = gChatLogCount;

    for (; n > 0; n--, i--) {
        ChatLine *line = &gChat.lines[i & 0x3F];
        rows += line->rows;
        if (line->id[0] != 0) {
            rows++;
        }
    }
    return rows;
}

/*
 * Draws the chat box for the send target sendTo (CHAT_SEND_*) at chat size
 * size (gChatBoxPanels): for CHAT_SEND_LOBBY a plain box; for the others
 * the title (title, or gChatTypeTexts[sendTo] when it is 0), and for
 * CHAT_SEND_TARGETS and CHAT_SEND_PARTY a marker at the box's corner, as
 * large as the size's text.
 */
void Chat_DrawBox(s32 sendTo, u8 size, s32 title)
{
    ListPanel *panel = &gChatBoxPanels[size];
    u8 marker;

    panel->flags = WINDOW_FILL | 3;
    marker = 0;
    switch (sendTo) {
    case CHAT_SEND_LOBBY:
        panel->flags = WINDOW_FILL | 2;
        break;
    case CHAT_SEND_TARGETS:
    case CHAT_SEND_PARTY:
        marker = 1;
    case CHAT_SEND_LOCAL:
        if (title == 0) {
            title = gChatTypeTexts[sendTo];
        }
        break;
    }
    ListPanel_Draw(panel, (s16 *)0, (char *)title, -1);
    if (marker) {
        size = gChatTextSizes[size];
        Gfx_DrawMarker(panel->x, panel->y, size, size * 18 / 21, 1);
    }
}

/* Sets the chat text's colour on the HUD: a negative color clears it. */
void Chat_SetColor(s16 color)
{
    if (color < 0) {
        gChatHudColor = 0;
    } else {
        gChatHudColor = color | 0x80;
    }
}

/* Empties the three object pools (Obj, StageObj, ObjAttach) and frees every work block. */
void Obj_InitAll(void)
{
    Obj_InitPool();
    StageObj_InitPool();
    ObjAttach_InitPool();
    ObjBuf_MarkFree(0, OBJ_BUF_BLOCKS);
}

/* Frees the objects of the three pools that are not marked to keep. */
void Obj_FreeAll(void)
{
    Obj_FreeUnkept();
    StageObj_FreeUnkept();
    ObjAttach_FreeUnkept();
}

/*
 * Finds n free work blocks in a row: returns the first one's index, 0 for
 * n 0, or -1. The search starts at a free block and, when the n blocks from
 * there are not all free, goes on n blocks further.
 * Kept for the match: j, the sum loop's counter, also holds the end index
 * (i + n) for the bound test, as the game's code reuses the one register;
 * with a variable of its own the compiler swaps two registers. The loop is
 * a goto, so that gObjBufUsed's address is loaded on each pass, and the
 * search is an if around a do-while, which gives its test at the top.
 */
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
        if (i < OBJ_BUF_BLOCKS) {
            do {
                if (gObjBufUsed[i] == 0) {
                    break;
                }
                i++;
            } while (i < OBJ_BUF_BLOCKS);
        }
        if (i >= OBJ_BUF_BLOCKS) {
            return -1;
        }
        j = i + n;
        if (j > OBJ_BUF_BLOCKS) {
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

/* The address of work block blk. */
u8 *ObjBuf_GetBlock(s32 blk)
{
    return (u8 *)((s32)gObjBufBlocks + (blk << OBJ_BUF_BLOCK_SHIFT));
}

/* Marks the n work blocks from start as in use. */
void ObjBuf_MarkUsed(s32 start, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++) {
        gObjBufUsed[start + i] = 1;
    }
}

/* Marks the n work blocks from start as free. */
void ObjBuf_MarkFree(s32 start, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++) {
        gObjBufUsed[start + i] = 0;
    }
}

/* Empties the Obj pool: every object goes on the free list, in order. */
void Obj_InitPool(void)
{
    s32 i;
    Obj *obj;

    memset(gObjPool, 0, sizeof(gObjPool));
    gObjList = NULL;
    obj = &gObjPool[OBJ_POOL_SIZE - 1];
    gObjFreeList = (Obj **)gObjPool;
    for (i = 0; i < OBJ_POOL_SIZE; i++, obj--) {
        *--gObjFreeList = obj;
    }
    gObjFreeCount = OBJ_POOL_SIZE;
}

/* Frees the active objects not marked to keep. */
void Obj_FreeUnkept(void)
{
    Obj *obj = gObjList;
    Obj *dead;

    while (obj != NULL) {
        if (obj->active != 0 && obj->keep == 0) {
            dead = obj;
            obj = obj->next;
            Obj_Free(dead);
        } else {
            obj = obj->next;
        }
    }
}

/*
 * Takes an object from the pool with n work blocks (none for 0: its buf is
 * then -1), puts it first in the list in use and marks it active. Returns
 * NULL when the pool or the blocks run out.
 */
Obj *Obj_Alloc(s32 n)
{
    s32 blk;
    Obj *obj;

    if ((blk = ObjBuf_FindFree(n)) < 0) {
        return NULL;
    }
    if (gObjFreeCount == 0) {
        return NULL;
    }
    gObjFreeCount--;
    obj = *gObjFreeList++;
    obj->prev = NULL;
    obj->next = gObjList;
    gObjList = obj;
    if (obj->next != NULL) {
        obj->next->prev = obj;
    }
    obj->active = 1;
    obj->buf = (n > 0) ? ObjBuf_GetBlock(blk) : (u8 *)-1;
    obj->blk = blk;
    obj->nblk = n;
    obj->keep = 0;
    ObjBuf_MarkUsed(blk, n);
    return obj;
}

/* Returns an object to the pool and frees its work blocks; its first 8
 * bytes (the flags and the list links) are cleared. */
void Obj_Free(Obj *obj)
{
    if (obj->prev == NULL) {
        gObjList = obj->next;
    } else {
        obj->prev->next = obj->next;
    }
    if (obj->next != NULL) {
        obj->next->prev = obj->prev;
    }
    gObjFreeCount++;
    *--gObjFreeList = obj;
    if (obj->nblk > 0) {
        ObjBuf_MarkFree(obj->blk, obj->nblk);
    }
    memset(obj, 0, 8);
}

/* Runs every active object's update. */
void Obj_UpdateAll(void)
{
    Obj *obj;

    for (obj = gObjList; obj != NULL; obj = obj->next) {
        if (obj->active != 0) {
            obj->update(obj);
        }
    }
}

/* Runs the draw function of every active, visible object that has one. */
void Obj_DrawAll(void)
{
    Obj *obj;

    for (obj = gObjList; obj != NULL; obj = obj->next) {
        if (obj->active != 0 && obj->visible != 0) {
            if (obj->draw != NULL) {
                obj->draw(obj);
            }
        }
    }
}

/* Empties the ObjAttach pool, as Obj_InitPool does, and zeroes the
 * scene's counts of placed items (placed1, placed2). */
void ObjAttach_InitPool(void)
{
    s32 i;
    ObjAttach *att;

    memset(gObjAttachPool, 0, sizeof(gObjAttachPool));
    gObjAttachList = NULL;
    att = &gObjAttachPoolLast;
    gObjAttachFreeList = (ObjAttach **)gObjAttachPool;
    for (i = 0; i < OBJ_ATTACH_POOL_SIZE; i++, att--) {
        *--gObjAttachFreeList = att;
    }
    gScene.placed1 = 0;
    gObjAttachFreeCount = OBJ_ATTACH_POOL_SIZE;
    gScene.placed2 = 0;
}

/* Frees the active attachments not marked to keep. */
void ObjAttach_FreeUnkept(void)
{
    ObjAttach *att = gObjAttachList;
    ObjAttach *dead;

    while (att != NULL) {
        if (att->active != 0 && att->keep == 0) {
            dead = att;
            att = att->next;
            ObjAttach_Free(dead);
        } else {
            att = att->next;
        }
    }
}

/* Clears an attachment's hit state, keep flag and side, and sets its hit
 * scale to 1 and its power to 0. */
void ObjAttach_Reset(ObjAttach *att)
{
    att->wasHit = 0;
    att->hitBy = 0;
    att->keep = 0;
    att->side = 0;
    att->hitScale = 1.0f;
    att->powerPct = 0;
}

/* Takes an attachment from the pool with n work blocks, as Obj_Alloc does,
 * and resets it (ObjAttach_Reset). Returns NULL when the pool or the blocks
 * run out. */
ObjAttach *ObjAttach_Alloc(s32 n)
{
    s32 blk;
    ObjAttach *att;

    if ((blk = ObjBuf_FindFree(n)) < 0) {
        return NULL;
    }
    if (gObjAttachFreeCount == 0) {
        return NULL;
    }
    gObjAttachFreeCount--;
    att = *gObjAttachFreeList++;
    att->prev = NULL;
    att->next = gObjAttachList;
    gObjAttachList = att;
    if (att->next != NULL) {
        att->next->prev = att;
    }
    att->active = 1;
    att->buf = ObjBuf_GetBlock(blk);
    att->blk = blk;
    att->nblk = n;
    ObjBuf_MarkUsed(blk, n);
    ObjAttach_Reset(att);
    return att;
}

/* Returns an attachment to the pool and frees its work blocks; clears its
 * first 0xC bytes, its attack flags, hit and hurt lists and draw slots, and
 * the low bits of hitFx. */
void ObjAttach_Free(ObjAttach *att)
{
    if (att->prev == NULL) {
        gObjAttachList = att->next;
    } else {
        att->prev->next = att->next;
    }
    if (att->next != NULL) {
        att->next->prev = att->prev;
    }
    gObjAttachFreeCount++;
    *--gObjAttachFreeList = att;
    ObjBuf_MarkFree(att->blk, att->nblk);
    memset(att, 0, 0xC);
    att->attackFlags = 0;
    att->hitList = 0;
    att->hurtList = 0;
    att->drawSlot = 0;
    att->drawSlotBuf = 0;
    att->hitFx &= 0x3F;
}

/* Runs every active attachment's update, or counts down its delay first. */
void ObjAttach_UpdateAll(void)
{
    ObjAttach *att;

    for (att = gObjAttachList; att != NULL; att = att->next) {
        if (att->active != 0) {
            if (att->delay != 0) {
                att->delay--;
            } else {
                att->update(att);
            }
        }
    }
}

/* Runs the draw function of every active, visible attachment whose owner
 * (the player or the monster it follows) is visible. */
void ObjAttach_DrawAll(void)
{
    ObjAttach *att;

    for (att = gObjAttachList; att != NULL; att = att->next) {
        if (att->isEm == 0) {
            Player *pl = &gPlayers[att->idx];

            if (att->active != 0 && att->visible != 0 && pl->visible != 0) {
                if (att->draw != NULL) {
                    att->draw(att);
                }
            }
        } else {
            Em *em = &gEms[att->idx];

            if (att->active != 0 && att->visible != 0 && em->visible != 0) {
                if (att->draw != NULL) {
                    att->draw(att);
                }
            }
        }
    }
}

/* Whether the monster bomb can be set while riding: always 1. */
s32 Item_RideItemUsable(void)
{
    return 1;
}

/* 1 when a large barrel bomb can be placed: not in the camp's area nor where
 * barrel bombs are barred, and fewer than 2 of the large bombs placed
 * (gScene.placed1). */
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

/* 1 unless the area bars barrel bombs. */
s32 Item_CanUseSmallBarrelBomb(void)
{
    return Area_NoBarrelBombs(gScene.area) != 1;
}

/* 1 when a trap can be set: none of the local player's is set
 * (gScene.placed2), and the player is not on the high ground (above 100)
 * of AREA_SNOWY_MOUNTAINS_3. */
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

/* 1 when a trap can be set (Scene_CanPlaceKind2), and not in the camp's area. */
s32 Item_CanPlaceKind2(void)
{
    if (gScene.area == gScene.campArea) {
        return 0;
    }
    return Scene_CanPlaceKind2();
}

/* 1 in an area that bars barrel bombs. */
s32 Scene_InNoBarrelBombArea(void)
{
    return Area_NoBarrelBombs(gScene.area) == 1;
}

/* 1 when ITEM_ID_PLACED_BOMB can be placed: not in the camp's area, and
 * fewer than 2 of the large bombs placed (gScene.placed1). */
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

/*
 * Whether the monster bats back the deflected arrow att, by the random
 * number roll: never when an aura deflected it (ATTACH_DEFLECTED_AURA),
 * else 40 times in 100.
 */
s32 ObjAttach_RollBatBack(ObjAttach *att, u16 roll)
{
    if (att->state == ATTACH_DEFLECTED_AURA) {
        return 0;
    }
    if (roll % 100 < 40) {
        return 1;
    }
    return 0;
}

/* A trap's reach for a monster of kind kind: reach, or a quarter more for
 * the largest kinds. */
f32 Em_TrapReach(u16 kind, f32 reach)
{
    switch (kind) {
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
    case EM_KIND_GRAVIOS:
    case EM_KIND_BLACK_GRAVIOS:
    case EM_KIND_DIABLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_WHITE_MONOBLOS:
    case EM_KIND_RAJANG:
        return 1.25f * reach;
    }
    return reach;
}

/* A debug switch (ids 0x17 to 0xAE are tested): always off in this build. */
s32 Debug_IsOn(s32 unused0)
{
    return 0;
}

/* The monster kind a quest without one would take: always 0 in this build. */
s32 Quest_DefaultEmKind(void)
{
    return 0;
}

/* Empty in this build. The scene's set-up gives it the debug switch ids
 * 0x64 to 0x68 in every area but area 0xD. */
void Debug_SwitchStub(s32 unused0)
{
}

/* Empty in this build. The scene's set-up gives it the debug switch ids
 * 0x64 to 0x66 in area 0xD. */
void Debug_SwitchStubAlt(s32 unused0)
{
}

/*
 * Turns the n points v about the point c by the sprite's rotation (rot, in
 * degrees); for SPR_BILLBOARD, also by the inverse of the view matrix, so
 * that the sprite faces the camera.
 */
void Sprite_RotateVerts(f32 (*verts)[3], f32 *center, s32 count, SprDef *spr)
{
    f32 rotated[4];
    f32 offset[4];
    f32 rotMtx[4][4];
    f32 baseMtx[4][4];
    s32 i;

    if (spr->flags & SPR_BILLBOARD) {
        Mtx_Inverse((f32 *) baseMtx, (f32 *) gViewMatrix);
    } else {
        Mtx_Identity((f32 *) baseMtx);
    }
    Mtx_Identity((f32 *) rotMtx);
    Mtx_SetRotZYX((f32 *) rotMtx, 6.2831855f * spr->rot[0] / 360.0f, 6.2831855f * spr->rot[1] / 360.0f, 6.2831855f * spr->rot[2] / 360.0f);
    Mtx_Mul((f32 *) rotMtx, (f32 *) rotMtx, (f32 *) baseMtx);
    for (i = 0; i < count; i++, verts++) {
        offset[0] = (*verts)[0] - center[0];
        offset[1] = (*verts)[1] - center[1];
        offset[2] = (*verts)[2] - center[2];
        Mtx_RotateVec((f32 *) rotMtx, (f32 *) offset, (f32 *) rotated);
        (*verts)[0] = rotated[0] + center[0];
        (*verts)[1] = rotated[1] + center[1];
        (*verts)[2] = rotated[2] + center[2];
    }
}

/*
 * Queues the sprite s as a primitive in gPrims, unless the queue is full.
 * Its rectangle runs from the anchor pos by size * scale, by the alignment
 * and the flips (in the world kinds y grows upward); its colour is rgb with
 * alpha. A PRIM_SPRITE_3D with PRIM_ROTATED (always for SPR_BILLBOARD,
 * whose rotation starts at 0) is turned about pos (Sprite_RotateVerts) and
 * queued as two triangles, the second only when there is room for it.
 */
void Sprite_Queue(SprDef *spr)
{
    PrimPoint corner0;
    PrimPoint corner1;
    PrimPoint center;
    PrimPoint corners[4];
    Prim *prim = gPrims;
    PrimSpriteF *fillQuad;
    PrimSpriteF *spriteQuad;
    PrimTri3DView *tri;
    u32 color;
    s32 rgb;
    f32 sign;
    f32 len;
    f32 uLeft;
    f32 uRight;
    f32 vTop;
    f32 vBottom;

    if (gPrimCount != PRIM_QUEUE_MAX) {
        prim += gPrimCount;
        prim->tex = spr->tex;
        prim->z = spr->pos[2];
        prim->flags = spr->primFlags & PRIM_FLAGS_MASK;
        if (spr->flags & SPR_FILTER) {
            prim->flags |= PRIM_FILTER;
        }
        if (spr->flags & SPR_ADDITIVE) {
            prim->additive = 1;
        } else {
            prim->additive = 0;
        }
        if ((prim->flags & PRIM_IS_3D) == 0) {
            sign = 1.0f;
        } else {
            sign = -1.0f;
        }
        switch (spr->flags & SPR_ALIGN_X_MASK) {
        case 0:
        case 1:
            if (spr->flags & SPR_FLIP_X) {
                corner0.x = spr->pos[0] - spr->size[0] * spr->scale[0];
                corner1.x = spr->pos[0];
            } else {
                corner0.x = spr->pos[0];
                corner1.x = corner0.x + spr->size[0] * spr->scale[0];
            }
            break;
        case SPR_ALIGN_CENTER:
            len = spr->size[0] * spr->scale[0];
            corner0.x = spr->pos[0] - len / 2.0f;
            corner1.x = corner0.x + len;
            break;
        case SPR_ALIGN_END:
            if (spr->flags & SPR_FLIP_X) {
                corner0.x = spr->pos[0];
                corner1.x = corner0.x + spr->size[0] * spr->scale[0];
            } else {
                corner0.x = spr->pos[0] - spr->size[0] * spr->scale[0];
                corner1.x = spr->pos[0];
            }
            break;
        }
        switch ((spr->flags & SPR_ALIGN_Y_MASK) >> SPR_ALIGN_Y_SHIFT) {
        case 0:
        case 1:
            if (spr->flags & SPR_FLIP_Y) {
                corner0.y = spr->pos[1] + sign * (-1.0f * (spr->size[1] * spr->scale[1]));
                corner1.y = spr->pos[1];
            } else {
                corner0.y = spr->pos[1];
                corner1.y = corner0.y + sign * (spr->size[1] * spr->scale[1]);
            }
            break;
        case SPR_ALIGN_CENTER:
            len = spr->size[1] * spr->scale[1];
            corner0.y = spr->pos[1] + sign * (-1.0f * (len / 2.0f));
            corner1.y = corner0.y + sign * len;
            break;
        case SPR_ALIGN_END:
            if (spr->flags & SPR_FLIP_Y) {
                corner0.y = spr->pos[1];
                corner1.y = corner0.y + sign * (spr->size[1] * spr->scale[1]);
            } else {
                corner0.y = spr->pos[1] + sign * (-1.0f * (spr->size[1] * spr->scale[1]));
                corner1.y = spr->pos[1];
            }
            break;
        }
        corner0.z = corner1.z = spr->pos[2];
        rgb = spr->rgb;
        color = rgb | (u32)(255.0f * spr->alpha) << 24;
        switch (prim->flags & PRIM_KIND_MASK) {
        case PRIM_FILL_2D:
            fillQuad = &prim->data.sprite;
            fillQuad->v[0].x = Math_Floor(corner0.x);
            fillQuad->v[0].y = Math_Floor(corner0.y);
            fillQuad->v[0].z = corner0.z;
            fillQuad->v[0].q = 1.0f;
            fillQuad->v[1].x = Math_Floor(corner1.x);
            fillQuad->v[1].y = Math_Floor(corner1.y);
            fillQuad->v[1].z = corner1.z;
            fillQuad->v[1].q = 1.0f;
            fillQuad->color = color;
            break;
        case PRIM_FILL_3D:
            prim->data.rect.p0 = corner0;
            prim->data.rect.p1 = corner1;
            prim->data.rect.color = color;
            break;
        case PRIM_SPRITE_2D:
            spriteQuad = &prim->data.sprite;
            spriteQuad->v[0].x = Math_Floor(corner0.x);
            spriteQuad->v[0].y = Math_Floor(corner0.y);
            spriteQuad->v[0].z = corner0.z;
            spriteQuad->v[0].q = 1.0f;
            spriteQuad->v[1].x = Math_Floor(corner1.x);
            spriteQuad->v[1].y = Math_Floor(corner1.y);
            spriteQuad->v[1].z = corner1.z;
            spriteQuad->v[1].q = 1.0f;
            spriteQuad->color = color;
            if (spr->flags & SPR_FLIP_X) {
                uLeft = spr->uv[2];
                uRight = spr->uv[0];
            } else {
                uLeft = spr->uv[0];
                uRight = spr->uv[2];
            }
            if (spr->flags & SPR_FLIP_Y) {
                vTop = spr->uv[3];
                vBottom = spr->uv[1];
            } else {
                vTop = spr->uv[1];
                vBottom = spr->uv[3];
            }
            spriteQuad->st[0][0] = uLeft;
            spriteQuad->st[0][1] = vTop;
            spriteQuad->st[1][0] = uRight;
            spriteQuad->st[1][1] = vBottom;
            break;
        case PRIM_SPRITE_3D:
            if (spr->flags & SPR_FLIP_X) {
                uLeft = spr->uv[2];
                uRight = spr->uv[0];
            } else {
                uLeft = spr->uv[0];
                uRight = spr->uv[2];
            }
            if (spr->flags & SPR_FLIP_Y) {
                vTop = spr->uv[3];
                vBottom = spr->uv[1];
            } else {
                vTop = spr->uv[1];
                vBottom = spr->uv[3];
            }
            if (spr->flags & SPR_BILLBOARD) {
                if (!(spr->primFlags & PRIM_ROTATED)) {
                    spr->primFlags |= PRIM_ROTATED;
                    spr->rot[2] = 0.0f;
                    spr->rot[1] = 0.0f;
                    spr->rot[0] = 0.0f;
                }
            }
            if (spr->primFlags & PRIM_ROTATED) {
                tri = &prim->data.tri;
                center.x = spr->pos[0];
                center.y = spr->pos[1];
                center.z = spr->pos[2];
                corners[0] = corner0;
                corners[1] = corner1;
                corners[1].y = corner0.y;
                corners[2] = corner0;
                corners[2].y = corner1.y;
                corners[3] = corner1;
                Sprite_RotateVerts((f32 (*)[3])corners, &center.x, 4, spr);
                tri->v[0] = corners[0];
                tri->v[1] = corners[1];
                tri->v[2] = corners[2];
                tri->color[0] = tri->color[1] = tri->color[2] = color;
                tri->st[0][0] = uLeft;
                tri->st[0][1] = vTop;
                tri->st[1][0] = uRight;
                tri->st[1][1] = vTop;
                tri->st[2][0] = uLeft;
                tri->st[2][1] = vBottom;
                if (gPrimCount != PRIM_QUEUE_MAX - 1) {
                    memcpy(prim + 1, prim, sizeof(Prim));
                    gPrimCount++;
                    prim[1].data.tri.v[0] = corners[1];
                    prim[1].data.tri.v[1] = corners[2];
                    prim[1].data.tri.v[2] = corners[3];
                    prim[1].data.tri.st[0][0] = uRight;
                    prim[1].data.tri.st[0][1] = vTop;
                    prim[1].data.tri.st[1][0] = uLeft;
                    prim[1].data.tri.st[1][1] = vBottom;
                    prim[1].data.tri.st[2][0] = uRight;
                    prim[1].data.tri.st[2][1] = vBottom;
                }
            } else {
                prim->data.rect.p0 = corner0;
                prim->data.rect.p1 = corner1;
                prim->data.rect.color = color;
                prim->data.rect.st[0] = uLeft;
                prim->data.rect.st[1] = vTop;
                prim->data.rect.st[2] = uRight;
                prim->data.rect.st[3] = vBottom;
            }
            break;
        }
        gPrimCount++;
    }
}

/* Draws the frame's queued primitives in order, with the camera's view
 * matrix and an identity model matrix, then restores the render states. */
void Prim_DrawAll(void)
{
    s32 i;
    Prim *prim = gPrims;
    f32 identity[4][4];

    Camera_SetPreset(0);
    MtxSlot_Store(0x21, (struct MtxSlot *) gViewMatrix);
    Mtx_Identity((f32 *) identity);
    Gs_SetState(RS_MTX_SLOT(0), (u32)identity);
    Gs_SetState(RS_ALPHA_REF, 0);
    for (i = 0; i < gPrimCount; i++, prim++) {
        Prim_Draw(prim);
    }
    Gfx_ResetRenderStates(1);
}

/* Draws one queued primitive: its filter and blend, then its kind's draw
 * function. A textured one is skipped while its texture is not loaded. */
void Prim_Draw(Prim *prim)
{
    Gfx_SetTexFilter((prim->flags & PRIM_FILTER) / PRIM_FILTER);
    if (prim->additive != 0) {
        Gfx_SetBlend(4, 1);
    } else {
        Gfx_SetBlend(4, 5);
    }
    switch (prim->flags & PRIM_KIND_MASK) {
    case PRIM_FILL_2D:
        Prim_FillRectF(&prim->data.fill);
        break;
    case PRIM_FILL_3D:
        Prim_DrawRect3D(&prim->data.fill3d);
        break;
    case PRIM_SPRITE_2D:
        if (gTexHandles[prim->tex] != 0) {
            Tex_Prepare(1, prim->tex);
            Tex_Bind(prim->tex);
            Prim_DrawSpriteF(&prim->data.sprite);
        }
        break;
    case PRIM_SPRITE_3D:
        if (gTexHandles[prim->tex] != 0) {
            Tex_Prepare(1, prim->tex);
            Tex_Bind(prim->tex);
            if (prim->flags & PRIM_ROTATED) {
                Prim_DrawTexTri3D(&prim->data.tri3d);
            } else {
                Prim_DrawSprite3D(&prim->data.sprite3d);
            }
        }
        break;
    }
}

/* Draws the textured rectangle r turned by angle (0x10000 a full turn)
 * about its centre. */
void TexRect_DrawRotated(TexRect *rect, s32 angle)
{
    TexRect_DrawRotatedAt(rect, rect->x + (s16)(0.5f * rect->w), rect->y + (s16)(0.5f * rect->h), angle);
}

/*
 * Draws the textured rectangle r turned by angle (0x10000 a full turn)
 * about the point (cx, cy), as two textured triangles. The turn is made
 * with the heights scaled to square pixels (by 1 / SCREEN_PIXEL_ASPECT, and
 * back after), so the shape keeps its proportions; r's height is restored
 * on return.
 */
void TexRect_DrawRotatedAt(TexRect *rect, s16 centerX, s16 centerY, u32 angle)
{
    f32 offset[3];
    TexTri tri;
    s16 height = rect->h;
    f32 rad;

    rect->h = height / SCREEN_PIXEL_ASPECT;
    offset[2] = 0.0f;
    offset[0] = rect->x - centerX;
    offset[1] = rect->y - centerY;
    rad = 6.2831855f * angle / 65536.0f;
    Vec_RotateZ(offset, rad);
    tri.x0 = centerX + offset[0];
    tri.y0 = centerY + SCREEN_PIXEL_ASPECT * offset[1];
    offset[0] = rect->x + rect->w - centerX;
    offset[1] = rect->y + rect->h - centerY;
    Vec_RotateZ(offset, rad);
    tri.x1 = centerX + offset[0];
    tri.y1 = centerY + SCREEN_PIXEL_ASPECT * offset[1];
    offset[0] = rect->x + rect->w - centerX;
    offset[1] = rect->y - centerY;
    Vec_RotateZ(offset, rad);
    tri.x2 = centerX + offset[0];
    tri.y2 = centerY + SCREEN_PIXEL_ASPECT * offset[1];
    tri.color = rect->color;
    tri.uv0 = rect->uv0;
    tri.uv1 = rect->uv1;
    tri.uv2.u = rect->uv1.u;
    tri.uv2.v = rect->uv0.v;
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
    offset[0] = rect->x - centerX;
    offset[1] = rect->y + rect->h - centerY;
    Vec_RotateZ(offset, rad);
    tri.x2 = centerX + offset[0];
    tri.y2 = centerY + SCREEN_PIXEL_ASPECT * offset[1];
    tri.color = rect->color;
    tri.uv0 = rect->uv0;
    tri.uv1 = rect->uv1;
    tri.uv2.u = rect->uv0.u;
    tri.uv2.v = rect->uv1.v;
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
    rect->h = height;
}

/*
 * Draws the textured rectangle r turned twice: by angle1 about (cx, cy),
 * then by angle2 about (cx2, cy2), as TexRect_DrawRotatedAt does (a hand
 * turning on a dial that turns too, say).
 */
void TexRect_DrawRotated2(TexRect *rect, s16 centerX, s16 centerY, s16 pivotX, s16 pivotY, u32 angle, u32 pivotAngle)
{
    f32 offset[3];
    TexTri tri;
    s16 height = rect->h;
    f32 rad;
    f32 pivotRad;

    rect->h = height / SCREEN_PIXEL_ASPECT;
    offset[2] = 0.0f;
    offset[0] = rect->x - centerX;
    offset[1] = rect->y - centerY;
    rad = 6.2831855f * angle / 65536.0f;
    Vec_RotateZ(offset, rad);
    tri.x0 = centerX + offset[0];
    tri.y0 = centerY + SCREEN_PIXEL_ASPECT * offset[1];
    offset[0] = tri.x0 - pivotX;
    offset[1] = tri.y0 - pivotY;
    pivotRad = 6.2831855f * pivotAngle / 65536.0f;
    Vec_RotateZ(offset, pivotRad);
    tri.x0 = pivotX + offset[0];
    tri.y0 = pivotY + SCREEN_PIXEL_ASPECT * offset[1];
    offset[0] = rect->x + rect->w - centerX;
    offset[1] = rect->y + rect->h - centerY;
    Vec_RotateZ(offset, rad);
    tri.x1 = centerX + offset[0];
    tri.y1 = centerY + SCREEN_PIXEL_ASPECT * offset[1];
    offset[0] = tri.x1 - pivotX;
    offset[1] = tri.y1 - pivotY;
    Vec_RotateZ(offset, pivotRad);
    tri.x1 = pivotX + offset[0];
    tri.y1 = pivotY + SCREEN_PIXEL_ASPECT * offset[1];
    offset[0] = rect->x + rect->w - centerX;
    offset[1] = rect->y - centerY;
    Vec_RotateZ(offset, rad);
    tri.x2 = centerX + offset[0];
    tri.y2 = centerY + SCREEN_PIXEL_ASPECT * offset[1];
    offset[0] = tri.x2 - pivotX;
    offset[1] = tri.y2 - pivotY;
    Vec_RotateZ(offset, pivotRad);
    tri.x2 = pivotX + offset[0];
    tri.y2 = pivotY + SCREEN_PIXEL_ASPECT * offset[1];
    tri.color = rect->color;
    tri.uv0 = rect->uv0;
    tri.uv1 = rect->uv1;
    tri.uv2.u = rect->uv1.u;
    tri.uv2.v = rect->uv0.v;
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
    offset[0] = rect->x - centerX;
    offset[1] = rect->y + rect->h - centerY;
    Vec_RotateZ(offset, rad);
    tri.x2 = centerX + offset[0];
    tri.y2 = centerY + SCREEN_PIXEL_ASPECT * offset[1];
    offset[0] = tri.x2 - pivotX;
    offset[1] = tri.y2 - pivotY;
    Vec_RotateZ(offset, pivotRad);
    tri.x2 = pivotX + offset[0];
    tri.y2 = pivotY + SCREEN_PIXEL_ASPECT * offset[1];
    tri.color = rect->color;
    tri.uv0 = rect->uv0;
    tri.uv1 = rect->uv1;
    tri.uv2.u = rect->uv0.u;
    tri.uv2.v = rect->uv1.v;
    Prim_DrawTexTri((struct PrimTexTri *)&tri);
    rect->h = height;
}

/*
 * Draws the textured rectangle r: mode 1 as it is, 0 mirrored left to
 * right (as sprites); 2 and 3 as two triangles with the texture turned a
 * quarter turn one way or the other.
 */
void TexRect_Draw(TexRect *rect, s8 mode)
{
    TexTri tri;
    TexRect copy;
    s16 right;
    s16 bottom;
    s16 uLeft;
    s16 uRight;
    s16 left;
    s16 top;
    s16 vTop;
    s16 vBottom;

    if (mode < 2) {
        copy = *rect;
        if (mode == 0) {
            copy.uv0.u = rect->uv1.u;
            copy.uv1.u = rect->uv0.u;
        }
        Sprite_Draw((struct Sprite *)&copy);
        return;
    }
    left = rect->x;
    top = rect->y;
    uLeft = rect->uv0.u;
    uRight = rect->uv1.u;
    vTop = rect->uv0.v;
    vBottom = rect->uv1.v;
    right = left + rect->w;
    bottom = top + rect->h;
    tri.color = rect->color;
    switch (mode) {
    case 2:
        tri.x0 = tri.x1 = left;
        tri.x2 = right;
        tri.y0 = top;
        tri.y1 = tri.y2 = bottom;
        tri.uv0.u = uRight;
        tri.uv1.u = tri.uv2.u = uLeft;
        tri.uv0.v = tri.uv1.v = vTop;
        tri.uv2.v = vBottom;
        Prim_DrawTexTri((struct PrimTexTri *)&tri);
        tri.x0 = tri.x1 = right;
        tri.x2 = left;
        tri.y0 = bottom;
        tri.y1 = tri.y2 = top;
        tri.uv0.u = uLeft;
        tri.uv1.u = tri.uv2.u = uRight;
        tri.uv0.v = tri.uv1.v = vBottom;
        tri.uv2.v = vTop;
        Prim_DrawTexTri((struct PrimTexTri *)&tri);
        break;
    case 3:
        tri.x0 = tri.x1 = left;
        tri.x2 = right;
        tri.y0 = top;
        tri.y1 = tri.y2 = bottom;
        tri.uv0.u = uLeft;
        tri.uv1.u = tri.uv2.u = uRight;
        tri.uv0.v = tri.uv1.v = vBottom;
        tri.uv2.v = vTop;
        Prim_DrawTexTri((struct PrimTexTri *)&tri);
        tri.x0 = tri.x1 = right;
        tri.x2 = left;
        tri.y0 = bottom;
        tri.y1 = tri.y2 = top;
        tri.uv0.u = uRight;
        tri.uv1.u = tri.uv2.u = uLeft;
        tri.uv0.v = tri.uv1.v = vTop;
        tri.uv2.v = vBottom;
        Prim_DrawTexTri((struct PrimTexTri *)&tri);
        break;
    }
}

/* Fills the rectangle of w by h at (x, y) with color (a line when one of
 * them is 0). */
void Gfx_DrawLine(s16 x, s16 y, s16 width, s16 height, s32 color)
{
    PrimLine line;

    line.x0 = (f32)x;
    line.y0 = y;
    line.x1 = (f32)(x + width);
    line.y1 = y + height;
    line.color = color;
    Prim_FillRect(&line);
}

/* Draws the outline of the rectangle of w by h at (x, y) in color, as four
 * lines. */
void Gfx_DrawRectFrame(s16 x, s16 y, s16 width, s16 height, s32 color)
{
    PrimLine line;

    x = (f32)x;
    width = (f32)width;

    line.color = color;
    line.x0 = line.x1 = x;
    line.y0 = y;
    line.y1 = y + height;
    Prim_DrawLine(&line);
    line.x1 = x + width;
    line.x0 = width + x;
    Prim_DrawLine(&line);
    line.y0 = line.y1 = y;
    line.x0 = x;
    Prim_DrawLine(&line);
    line.y0 = line.y1 = height + y;
    Prim_DrawLine(&line);
}

/*
 * Draws the window texture's marker icon in the rectangle of w by h at
 * (x, y), in the colour gMarkerColors[flags & MARKER_COLOR_MASK]. With
 * MARKER_ALERT a red cell is drawn over it; with MARKER_CUE a cue to its
 * right blinks (gFrameTick: off for 22 frames of 64, then short, then
 * long).
 */
void Gfx_DrawMarker(s16 x, s16 y, s16 width, s16 height, s32 flags)
{
    SpriteWords spr;
    s32 tick;

    spr.x = x;
    spr.y = y;
    spr.w = width;
    spr.h = height;
    spr.color = gMarkerColors[flags & MARKER_COLOR_MASK];
    spr.uv0 = TEX_UV(0xDC, 0xED);
    spr.uv1 = TEX_UV(0xF0, 0xFF);
    Sprite_Draw((struct Sprite *)&spr);
    if (flags & MARKER_ALERT) {
        spr.color = 0xA0FF2828;
        spr.uv0 = TEX_UV(0x50, 0xC5);
        spr.uv1 = TEX_UV(0x64, 0xD7);
        Sprite_Draw((struct Sprite *)&spr);
    }
    if (flags & MARKER_CUE) {
        tick = gFrameTick & 0x3F;
        if (tick > 0x15) {
            spr.x += width;
            spr.color = -1;
            if (tick < 0x2B) {
                spr.w >>= 2;
                spr.uv0 = TEX_UV(0xA5, 0xC5);
                spr.uv1 = TEX_UV(0xAA, 0xD7);
            } else {
                spr.w >>= 1;
                spr.uv0 = TEX_UV(0xAC, 0xC5);
                spr.uv1 = TEX_UV(0xB6, 0xD7);
            }
            Sprite_Draw((struct Sprite *)&spr);
        }
    }
}

/* Clears the music's variables and forgets the music playing. */
void Bgm_Init(void)
{
    memset(gBgmVars, 0, 0xA);
    gBgmId = BGM_NONE;
}

/*
 * What fight the local player's area holds, for the field's music:
 * FIGHT_LARGE_MONSTER when an active, visible large monster is in combat in
 * the player's area and not defeated (Chameleos only while enraged);
 * FIGHT_PLAYERS when a player in the scene's area is in battle; else
 * FIGHT_NONE, as always during an event, in the camp's area or in a quest
 * with gQuest.flags 0x600. Outside versus quests, a Khezu or a Red Khezu
 * anywhere in the quest gives FIGHT_NONE, and so does a Chameleos that is
 * not enraged.
 */
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
    for (i = 0; i < EM_COUNT; i++, em++) {
        if (em->active == 0 || em->visible == 0) {
            continue;
        }
        if (Quest_IsVersus() == 0) {
            if (em->type == EM_KIND_KHEZU || em->type == EM_KIND_RED_KHEZU) {
                return FIGHT_NONE;
            }
            if (em->type == EM_KIND_CHAMELEOS) {
                found = 1;
            }
        }
        if (gEmIsLarge[em->type] == 1 && em->stateGroup != EM_GROUP_DEFEAT && em->combat == 1 &&
            em->area == pl->area && em->type != 0x1D) {
            if (em->type == EM_KIND_CHAMELEOS) {
                if (em->rage != 0) {
                    return FIGHT_LARGE_MONSTER;
                }
            } else {
                return FIGHT_LARGE_MONSTER;
            }
        }
    }
    if (found != 0) {
        return FIGHT_NONE;
    }
    for (pl = gPlayers, i = 0; i < gScene.playerCount; i++, pl++) {
        if (pl->active != 0 && pl->inBattle != 0 && pl->area == gScene.area) {
            return FIGHT_PLAYERS;
        }
    }
    return FIGHT_NONE;
}

/* 1 while a large monster's battle music plays or is about to. */
s32 Bgm_IsBattleState(void)
{
    if (gScene.musicState == MUSIC_MONSTER_FADE || gScene.musicState == MUSIC_MONSTER_BATTLE) {
        return 1;
    }
    return 0;
}

s16 Area_MonsterState(u16 area);
void Jingle_PlayForArea(u16 area);
void Bgm_EnterArea(u16 area);

/*
 * The field's music, every frame (enum MusicState). Counts down
 * gScene.musicTimer, restores the music's volume once a tune on
 * BGM_CH_TUNE has ended, and moves to BGM_GOAL_MET when the goal is
 * met (Quest_GetOutcome(1) turns 1), to BGM_QUEST_LOST when the quest is
 * lost (-1). From the area's music, a fight
 * (Bgm_IsBattle) fades to BGM_BATTLE, and a change of the area's variant
 * (outside events) to the music for the new variant, with its jingle. The
 * battle music plays at least BGM_BATTLE_FRAMES; then a large monster's
 * fight moves on to its own music, the end of the fight back to the
 * area's.
 */
void Bgm_FieldUpdate(void)
{
    u8 state;
    s32 outcome;

    gBgmVariant = Area_MonsterState(gScene.area);
    if (gScene.musicTimer > 0) {
        gScene.musicTimer--;
    }
    if ((gScene.musicFlags & MUSIC_FLAG_DUCKED) && Stream_GetStat(BGM_CH_TUNE) >= ADXT_STAT_DECEND) {
        if (gScene.musicState >= MUSIC_BATTLE_FADE) {
            Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, BGM_VOL_MAX);
        } else {
            Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, gBgmVolume);
        }
        gScene.musicFlags &= ~MUSIC_FLAG_DUCKED;
    }
    if (gScene.musicState < MUSIC_GOAL_MET_WAIT) {
        outcome = Quest_GetOutcome(1);
        if (outcome != 0) {
            if (outcome == 1) {
                if (gQuest.flags & 2) {
                    gScene.musicState = MUSIC_GOAL_MET_WAIT;
                    gScene.musicTimer = 100;
                } else {
                    Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
                    gScene.musicTimer = BGM_FADE_FRAMES;
                    gScene.musicState = MUSIC_GOAL_MET_FADE;
                }
            } else {
                Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
                gScene.musicTimer = BGM_FADE_FRAMES;
                gScene.musicState = MUSIC_LOST_FADE;
            }
        }
    }
    state = gScene.musicState;
    switch (state) {
    case MUSIC_AREA:
        if (Bgm_IsBattle() != FIGHT_NONE) {
            gScene.musicState = MUSIC_BATTLE_FADE;
            Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
            gScene.musicTimer = BGM_FADE_FRAMES;
        } else if (gBgmVariant != gBgmIndex && gScene.eventRunning == 0) {
            gScene.musicState++;
            Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
            gScene.musicTimer = BGM_FADE_FRAMES;
            Jingle_PlayForArea(gScene.area);
        }
        break;
    case MUSIC_VARIANT_FADE:
        if (gScene.musicTimer <= 0) {
            Bgm_EnterArea(gScene.area);
        }
        break;
    case MUSIC_BATTLE_FADE:
        if (gScene.musicTimer <= 0) {
            gScene.musicState = state + 1;
            gScene.musicTimer = BGM_BATTLE_FRAMES;
            Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_BATTLE, BGM_FADE_FRAMES, BGM_VOL_MAX);
            gBgmId = BGM_BATTLE;
        }
        break;
    case MUSIC_BATTLE:
        if (gScene.musicTimer <= 0) {
            switch (Bgm_IsBattle()) {
            case FIGHT_NONE:
                gScene.musicState = MUSIC_BATTLE_OVER;
                Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
                break;
            case FIGHT_PLAYERS:
                break;
            case FIGHT_LARGE_MONSTER:
                gScene.musicState++;
                Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
                gScene.musicTimer = BGM_FADE_FRAMES;
                break;
            }
        }
        break;
    case MUSIC_MONSTER_FADE:
        if (gScene.musicTimer <= 0) {
            gScene.musicState = state + 1;
            gScene.musicTimer = BGM_BATTLE_FRAMES;
            Bgm_PlayBattle();
        }
        break;
    case MUSIC_MONSTER_BATTLE:
        if (gScene.musicTimer <= 0) {
            switch (Bgm_IsBattle()) {
            case FIGHT_NONE:
                gScene.musicTimer = BGM_FADE_FRAMES;
                Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
                gScene.musicState++;
                break;
            case FIGHT_PLAYERS:
                break;
            case FIGHT_LARGE_MONSTER:
                break;
            }
        }
        break;
    case MUSIC_BATTLE_OVER:
        if (gScene.musicTimer <= 0) {
            gScene.musicState = MUSIC_AREA;
            Bgm_EnterArea(gScene.area);
        }
        break;
    case MUSIC_GOAL_MET_WAIT:
        if (gScene.musicTimer > 0) {
            break;
        }
        Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
        gScene.musicTimer = BGM_FADE_FRAMES;
        gScene.musicState = MUSIC_GOAL_MET_FADE;
    case MUSIC_GOAL_MET_FADE:
        if (gScene.musicTimer > 0) {
            break;
        }
        gScene.musicState++;
        Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_GOAL_MET, BGM_FADE_FRAMES, BGM_VOL_MAX);
        gBgmId = BGM_GOAL_MET;
    case MUSIC_GOAL_MET:
        if (Quest_GetOutcome(1) == -1) {
            gScene.musicState = MUSIC_LOST;
            Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_QUEST_LOST, BGM_FADE_FRAMES, BGM_VOL_MAX);
            gBgmId = BGM_QUEST_LOST;
        }
        break;
    case MUSIC_LOST_FADE:
        if (gScene.musicTimer > 0) {
            break;
        }
        gScene.musicState = state + 1;
        Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_QUEST_LOST, BGM_FADE_FRAMES, BGM_VOL_MAX);
        gBgmId = BGM_QUEST_LOST;
    case MUSIC_LOST:
        if (Quest_GetOutcome(1) == 1) {
            gScene.musicState = MUSIC_GOAL_MET;
            Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_GOAL_MET, BGM_FADE_FRAMES, BGM_VOL_MAX);
            gBgmId = BGM_GOAL_MET;
        }
        break;
    }
}

/*
 * The town's music, every frame, unless paused (gBgmPaused) or while the
 * lobby's music runs (LobbyBgm_GetState 2); the lobby's own update runs
 * after it. In AREA_TAVERN the six tracks of gBgmTavernTracks follow one
 * another, each after its time, in a random order with no track twice in a
 * row, a sound marking the change. Elsewhere, a new season or time of day
 * fades the music out and plays its tune (Bgm_PlayTownTune), then the
 * area's music again; in AREA_TOWN_ARENA the evening comes straight back to
 * the area's music. Nothing changes while a town tune plays over the music.
 */
void Bgm_TownUpdate(void)
{
    u8 track;
    u16 id;
    s32 vol;

    if (gBgmPaused == 0 && LobbyBgm_GetState() != 2) {
        if (gScene.musicTimer > 0) {
            gScene.musicTimer--;
        }
        if ((gScene.musicFlags & MUSIC_FLAG_DUCKED) && Stream_GetStat(BGM_CH_TUNE) >= ADXT_STAT_DECEND) {
            Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, gBgmVolume);
            gScene.musicFlags &= ~MUSIC_FLAG_DUCKED;
        }
        if (Bgm_IsDuckingTune() != 1) {
            switch (gScene.musicState) {
            case MUSIC_TOWN_AREA:
                if (gScene.area == AREA_TAVERN) {
                    if (gScene.musicTimer <= 0) {
                        gScene.musicState++;
                        Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
                        gScene.musicTimer = BGM_FADE_FRAMES;
                        Sound_PlaySeFlat(7, 0x18, 0);   /* the track's change */
                    }
                } else if (gBgmPm != (u8)LobbyTime_IsPm() || gBgmSeason != LobbyWorld_GetSeason()) {
                    gScene.musicState = MUSIC_TOWN_CHANGE;
                    Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
                    gScene.musicTimer = BGM_FADE_FRAMES;
                }
                break;
            case MUSIC_TOWN_NEXT:
                if (gScene.musicTimer <= 0) {
                    if (gScene.area == AREA_TAVERN) {
                        gScene.musicState = MUSIC_TOWN_AREA;
                        track = Rand_Next(0) % 6;
                        if (track == gBgmIndex) {
                            gBgmIndex = (track + 1) % 6;
                        } else {
                            gBgmIndex = track;
                        }
                        id = gBgmTavernTracks[gBgmIndex].id;
                        vol = gBgmTavernTracks[gBgmIndex].vol;
                        gBgmVolume = vol;
                        if ((gScene.musicFlags & MUSIC_FLAG_DUCKED) && (gScene.bgmId == 0x28 || gScene.bgmId == 0x29)) {
                            vol = 0;
                        }
                        Stream_PlayFadeIn(BGM_CH_MUSIC, id, BGM_FADE_FRAMES, vol);
                        gBgmId = id;
                        gScene.musicTimer = gBgmTavernTrackFrames[gBgmIndex];
                    } else {
                        Bgm_PlayAreaMusic(gScene.area);
                    }
                }
                break;
            case MUSIC_TOWN_CHANGE:
                if (gScene.musicTimer <= 0) {
                    if (gScene.area == AREA_TOWN_ARENA && gBgmPm == 0) {
                        gScene.musicState = MUSIC_TOWN_AREA;
                        Bgm_PlayAreaMusic(gScene.area);
                    } else {
                        gScene.musicState++;
                        if (gBgmSeason != LobbyWorld_GetSeason()) {
                            Bgm_PlayTownTune(TOWN_TUNE_SEASON, LobbyWorld_GetSeason());
                        } else if (gBgmPm != (u8)LobbyTime_IsPm()) {
                            Bgm_PlayTownTune(TOWN_TUNE_TIME, LobbyTime_IsPm());
                        }
                    }
                }
                break;
            case MUSIC_TOWN_TUNE:
                if (Stream_GetStat(BGM_CH_MUSIC) >= ADXT_STAT_DECEND) {
                    gScene.musicState = MUSIC_TOWN_AREA;
                    Bgm_PlayAreaMusic(gScene.area);
                }
                break;
            }
        }
    }
    LobbyBgm_Update();
}

/*
 * Plays the tune id on BGM_CH_TUNE when owner is the local player, and
 * notes it in gScene.bgmId. In the town, the town's tunes (4, BGM_TUNE_NPC and
 * 0x23 to 0x29) turn the music down under them, to 0x1E for tune 4 and to
 * silence for the others; outside it, a tune during the battle music (before the
 * goal is met or the quest lost) turns it down to 0x3C.
 */
void Jingle_Play(void *owner, s32 id)
{
    ObjHead *head = owner;
    if (Player_IsLocal(head) == 1) {
        gScene.bgmId = id;
        if (gScene.inLobby != 0) {
            switch (id) {
            case 4:
                gScene.musicFlags |= MUSIC_FLAG_DUCKED;
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, 0x1E);
                Stream_PlayMusicVol(BGM_CH_TUNE, id);
                break;
            case BGM_TUNE_NPC:
            case 0x25:
            case 0x23:
            case 0x24:
            case 0x26:
            case 0x27:
            case 0x28:
            case 0x29:
                gScene.musicFlags |= MUSIC_FLAG_DUCKED;
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, 0);
                Stream_PlayMusicVol(BGM_CH_TUNE, id);
                break;
            default:
                Stream_Play(BGM_CH_TUNE, id);
                break;
            }
        } else {
            Stream_Play(BGM_CH_TUNE, id);
            if (gScene.musicState >= MUSIC_BATTLE_FADE && Quest_GetOutcome(1) == 0) {
                gScene.musicFlags |= MUSIC_FLAG_DUCKED;
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, 0x3C);
            }
        }
    }
}

/* 1 while BGM_TUNE_NPC plays over the turned-down music. */
s32 Bgm_IsNpcTunePlaying(void)
{
    if (gScene.bgmId == BGM_TUNE_NPC && (gScene.musicFlags & MUSIC_FLAG_DUCKED)) {
        return 1;
    }
    return 0;
}

/* 1 while one of the town's tunes that turn the music down (Jingle_Play)
 * plays over it; Bgm_TownUpdate then changes nothing. */
s32 Bgm_IsDuckingTune(void)
{
    switch (gScene.bgmId) {
    case BGM_TUNE_NPC:
    case 4:
    case 0x25:
    case 0x23:
    case 0x24:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
        if (gScene.musicFlags & MUSIC_FLAG_DUCKED) {
            return 1;
        }
        break;
    }
    return 0;
}

/* 1 when gScene.bgmId is one of the event tunes, which Bgm_PlayEvent lets
 * finish and Bgm_StopEvent stops. */
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
    case BGM_TUNE_NPC:
        return 1;
    }
    return 0;
}

/*
 * Plays the event tune id on BGM_CH_TUNE, unless an event tune is still
 * playing, and turns the music down to 0x1E under it until the
 * goal is met or the quest lost (Quest_GetOutcome(1) is 0).
 */
void Bgm_PlayEvent(s32 id)
{
    if (Bgm_IsEventTune() == 1 && Stream_GetStat(BGM_CH_TUNE) < ADXT_STAT_DECEND) {
        return;
    }
    Stream_Play(BGM_CH_TUNE, id);
    gScene.bgmId = id;
    if (Quest_GetOutcome(1) == 0) {
        gScene.musicFlags |= MUSIC_FLAG_DUCKED;
        Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, 0x1E);
    }
}

/*
 * For the local player owner: stops an event tune that plays, and
 * restores the music's volume if a tune turned it down (the area's volume,
 * or full during the field's battle music).
 */
void Bgm_StopEvent(void *owner)
{
    ObjHead *head = owner;
    if (Player_IsLocal(head) == 1) {
        if (Bgm_IsEventTune() == 1) {
            Stream_Stop(BGM_CH_TUNE);
        }
        if (gScene.musicFlags & MUSIC_FLAG_DUCKED) {
            gScene.musicFlags &= ~MUSIC_FLAG_DUCKED;
            if (gScene.inLobby != 0) {
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, gBgmVolume);
            } else if (gScene.musicState < MUSIC_BATTLE_FADE) {
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, gBgmVolume);
            } else {
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, BGM_VOL_MAX);
            }
        }
    }
}

/*
 * For the local player pl: stops the town tune that plays (some at once,
 * others with a 60-frame fade) and restores the music's volume, as
 * Bgm_StopEvent does.
 */
void Bgm_StopEventFade(Player *pl)
{
    if (Player_IsLocal(pl) == 1) {
        switch (gScene.bgmId) {
        case 0xD:
        case 0x1E:
        case 4:
        case 0x133:
            Stream_Stop(BGM_CH_TUNE);
            break;
        case 0x25:
        case 0x23:
        case 0x28:
        case BGM_TUNE_NPC:
            Stream_FadeOut(BGM_CH_TUNE, 60);
            break;
        }
        if (gScene.musicFlags & MUSIC_FLAG_DUCKED) {
            gScene.musicFlags &= ~MUSIC_FLAG_DUCKED;
            if (gScene.inLobby != 0) {
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, gBgmVolume);
            } else if (gScene.musicState < MUSIC_BATTLE_FADE) {
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, gBgmVolume);
            } else {
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, BGM_VOL_MAX);
            }
        }
    }
}

/* Plays BGM_FAINT, except in a versus quest. */
void Bgm_PlayFaintMusic(void)
{
    if (Quest_IsVersus() != 1) {
        gScene.musicState = MUSIC_FAINT;
        Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_FAINT, BGM_FADE_FRAMES, BGM_VOL_MAX);
        gBgmId = BGM_FAINT;
    }
}

/* Pauses (nonzero) or resumes Bgm_TownUpdate. */
void Bgm_SetPaused(u8 paused)
{
    gBgmPaused = paused;
}

/* Forgets the music playing, so that the next one starts from the top. */
void Bgm_ForgetCurrent(void)
{
    gBgmId = BGM_NONE;
}

/*
 * Plays the town's tune i of the kind kind (TOWN_TUNE_*): a farm or village
 * event, a season, a time of day; the town's update returns to the area's
 * music when it ends. Notes the season and time of day it was chosen for.
 */
void Bgm_PlayTownTune(s32 kind, s32 i)
{
    u16 id;
    u16 vol;

    gScene.musicState = MUSIC_TOWN_TUNE;
    switch (kind) {
    case TOWN_TUNE_EVENT:
        id = gTownTuneEvents[i].id;
        vol = gTownTuneEvents[i].vol;
        break;
    case TOWN_TUNE_SEASON:
        id = gTownTuneSeasons[i].id;
        vol = gTownTuneSeasons[i].vol;
        break;
    case TOWN_TUNE_TIME:
    default:
        id = gTownTuneTimes[i].id;
        vol = gTownTuneTimes[i].vol;
        break;
    }
    Stream_PlayLevel(BGM_CH_MUSIC, id, vol);
    gBgmPm = LobbyTime_IsPm();
    gBgmSeason = LobbyWorld_GetSeason();
    gBgmId = id;
}

/*
 * Plays the music of the town's area area and sets its reverb. The music
 * is picked by the area: by season and time of day (AREA_HUB_0x83,
 * AREA_HUB_0x84, AREA_TOWN_ARENA by day), by time of day (the houses), by the
 * village's level and time of day (the farm areas), a random one of six
 * tracks (AREA_TAVERN), or one tune; the elder dragon raid and the
 * village's state 4 have their own in AREA_HUB_0x83, AREA_HUB_0x84 and
 * AREA_TOWN_ARENA. Music already playing is only faded to its volume.
 */
void Bgm_PlayAreaMusic(u16 area)
{
    u8 season;
    u8 isPm;
    u16 id;
    s32 vol;
    s32 row;

    gScene.musicState = MUSIC_TOWN_AREA;
    isPm = LobbyTime_IsPm();
    season = LobbyWorld_GetSeason();
    gBgmPm = isPm;
    gBgmSeason = season;
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
            row = season * 4 + isPm * 2;
            id = gBgmHub83[row];
            vol = gBgmHub83[row + 1];
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
            row = season * 4 + isPm * 2;
            id = gBgmHub84[row];
            vol = gBgmHub84[row + 1];
        }
        break;
    case AREA_HUB_0x85:
        id = 0x37;
        vol = 0x46;
        break;
    case AREA_TAVERN:
        id = gBgmTavernTracks[gBgmIndex].id;
        if (gBgmId == id) {
            Stream_Pause(BGM_CH_MUSIC, 0);
            Stream_SetVol(BGM_CH_MUSIC, 0);
            vol = gBgmTavernTracks[gBgmIndex].vol;
            Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, vol);
        } else {
            gBgmIndex = Rand_Next(0) % 6;
            id = gBgmTavernTracks[gBgmIndex].id;
            vol = gBgmTavernTracks[gBgmIndex].vol;
            Stream_PlayFadeIn(BGM_CH_MUSIC, id, BGM_FADE_FRAMES, vol);
            gScene.musicTimer = gBgmTavernTrackFrames[gBgmIndex];
        }
        gBgmId = id;
        gBgmVolume = vol;
        break;
    case AREA_TOWN_ARENA:
        if (isPm == 0) {
            id = gBgmHub83[season * 4 + isPm * 2];
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
    case AREA_ELDER_HALL:
    default:
        id = 0x132;
        vol = 0x5A;
        break;
    case AREA_ONLINE_HOUSE_0:
        row = isPm * 2;
        id = gBgmOnlineHouse[row];
        vol = gBgmOnlineHouse[row + 1];
        break;
    case AREA_ONLINE_HOUSE_1:
        row = isPm * 2 + 4;
        id = gBgmOnlineHouse[row];
        vol = gBgmOnlineHouse[row + 1];
        break;
    case AREA_ONLINE_HOUSE_2:
        row = isPm * 2 + 8;
        id = gBgmOnlineHouse[row];
        vol = gBgmOnlineHouse[row + 1];
        break;
    case AREA_HUB_0x89:
        row = (u8)Village_AverageLevel();
        row = row * 4 + isPm * 2;
        id = gBgmFarm[row];
        vol = gBgmFarm[row + 1];
        break;
    case AREA_HUB_0xAA:
    case AREA_HUB_0xAB:
    case AREA_HUB_0xAC:
        row = (u8)Village_AverageLevel();
        row = row * 4 + isPm * 2;
        id = gBgmFarm[row];
        vol = (u32)gBgmFarm[row + 1] >> 1;
        break;
    case AREA_HUB_0x57:
        row = isPm * 3;
        id = gBgmHouse[row];
        vol = gBgmHouse[row + 1];
        break;
    case AREA_HOUSE:
        row = isPm * 3;
        id = gBgmHouse[row];
        vol = gBgmHouse[row + 2];
        break;
    }
    if (area != AREA_TAVERN) {
        if (gBgmId == id) {
            Stream_Pause(BGM_CH_MUSIC, 0);
            Stream_SetVol(BGM_CH_MUSIC, 0);
            Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, vol);
        } else {
            Stream_PlayFadeIn(BGM_CH_MUSIC, id, BGM_FADE_FRAMES, vol);
        }
        gBgmId = id;
        gBgmVolume = vol;
    }
    Sound_SetEffectOnce(0, gAreaReverbs[gAreaReverbIdx[area]].mode0, gAreaReverbs[gAreaReverbIdx[area]].depth0, 0, 0);
    Sound_SetEffectOnce(1, gAreaReverbs[gAreaReverbIdx[area]].mode1, gAreaReverbs[gAreaReverbIdx[area]].depth1, 0, 0);
}

/* Fades the music in from silence to its volume over fade frames. */
void Bgm_FadeInFromZero(s32 fade)
{
    Stream_SetVol(BGM_CH_MUSIC, 0);
    Stream_FadeTo(BGM_CH_MUSIC, fade, gBgmVolume);
}

/* Fades the music to its volume over fade frames. */
void Bgm_FadeToVolume(s32 fade)
{
    Stream_FadeTo(BGM_CH_MUSIC, fade, gBgmVolume);
}

/* Plays the music id at volume vol. */
void Bgm_PlayWithVolume(s32 id, s32 vol)
{
    Stream_PlayLevel(BGM_CH_MUSIC, id, vol);
    gBgmId = id;
}

/*
 * Plays the battle music: BGM_VERSUS in a versus quest; in the fortress
 * the area's (another set while Shen Gaoren is there); elsewhere the first
 * active monster's own (gBattleBgmKinds), or the map's (gMapBattleBgm).
 */
void Bgm_PlayBattle(void)
{
    Em *em = gEms;
    u16 id;
    s32 i;
    s32 j;
    u16 found;

    if (Quest_IsVersus() == 1) {
        id = BGM_VERSUS;
    } else if (Map_DayId(gScene.map) != MAP_FORTRESS) {
        id = 0xFFFF;
        for (i = 0; i < EM_COUNT; i++, em++) {
            if (em->active != 0) {
                for (j = 0; j < BATTLE_BGM_KINDS; j++) {
                    if (em->type == gBattleBgmKinds[j].key) {
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
            id = gMapBattleBgm[gScene.map];
        }
    } else {
        found = 0;
        for (i = 0; i < EM_COUNT; i++, em++) {
            if (em->active != 0 && em->type == EM_KIND_SHEN_GAOREN) {
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
    Stream_PlayFadeIn(BGM_CH_MUSIC, id, BGM_FADE_FRAMES, BGM_VOL_MAX);
    gBgmId = id;
}

/* The area's own tune (gAreaTunes), or 0 when it has none. */
s32 Bgm_AreaTune(u8 area)
{
    BgmKey *entry = gAreaTunes;
    u16 key = gAreaTunes[0].key;

    while (key != BGM_LIST_END) {
        if (key == area) {
            return entry->id;
        }
        entry++;
        key = entry->key;
    }
    return 0;
}

/* The field area's music for the area's variant (gAreaMusicAlts, else
 * gAreaMusic), with its volume put in gBgmVolume. */
u16 Bgm_GetAreaMusic(u16 area)
{
    AreaMusicAlt *alt = gAreaMusicAlts;
    u8 vol = gAreaMusic[area].vol;
    u16 id = gAreaMusic[area].id;

    if (gBgmVariant != 0) {
        while (alt->area != BGM_LIST_END) {
            if (alt->area == area && alt->variant == gBgmVariant) {
                id = alt->id;
                vol = alt->vol;
                break;
            }
            alt++;
        }
    }
    gBgmVolume = vol;
    return id;
}

/* Plays the area's jingle for the change of its variant, from the one the
 * music plays for (gBgmIndex) to the area's now (gBgmVariant), if it has
 * one. */
void Jingle_PlayForArea(u16 area)
{
    AreaJingles *areaList;
    VariantJingle *jingle;

    for (areaList = gAreaJingles;; areaList++) {
        if (areaList->area == BGM_LIST_END) {
            return;
        }
        if (areaList->area == area) {
            jingle = areaList->list;
            break;
        }
    }
    for (;; jingle++) {
        if (jingle->from == JINGLE_LIST_END) {
            return;
        }
        if (jingle->from == gBgmIndex && jingle->to == gBgmVariant) {
            Sound_PlaySeFlat(jingle->slot, jingle->se, 0);
            return;
        }
    }
}

/* Plays the music id at full volume and restarts the field's state machine
 * from the area's music. */
void Bgm_Play(s32 id)
{
    gScene.musicState = MUSIC_AREA;
    Stream_PlayFadeIn(BGM_CH_MUSIC, id, BGM_FADE_FRAMES, BGM_VOL_MAX);
    gBgmId = id;
}

/* In a quest whose stage has a random start, entering an area of the hunt
 * (not the camp's) plays BGM_TUNE_RANDOM_START, until an area's own tune has
 * played. */
void Jingle_OnAreaChange(u16 area)
{
    if (gQuest.stage != 0 && gStageHeader->randomStart != 0 && !(gScene.musicFlags & MUSIC_FLAG_AREA_TUNE)) {
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
                Jingle_Play(&gPlayers[gScene.localPlayer], BGM_TUNE_RANDOM_START);
            }
            break;
        }
    }
}

/*
 * Starts the field's music for the area entered, and sets its reverb.
 * Once the goal is met or the quest lost, BGM_GOAL_MET or BGM_QUEST_LOST
 * goes on (or starts). Otherwise: BGM_QUEST_THEME or the battle music in quests
 * with gQuest.flags 0x800 or 0x400; the area's own tune, the first time;
 * else the area's music for its variant. Music already playing is only
 * faded back in.
 */
void Bgm_EnterArea(u16 area)
{
    s32 outcome;
    s32 id;

    outcome = Quest_GetOutcome(1);
    if (outcome != 0) {
        if (outcome == 1) {
            if (gBgmId == BGM_GOAL_MET) {
                Stream_Pause(BGM_CH_MUSIC, 0);
                Stream_SetVol(BGM_CH_MUSIC, 0);
                Stream_FadeToMax(BGM_CH_MUSIC, BGM_FADE_FRAMES);
            } else {
                gScene.musicState = MUSIC_GOAL_MET;
                Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_GOAL_MET, BGM_FADE_FRAMES, BGM_VOL_MAX);
                gBgmId = BGM_GOAL_MET;
            }
        } else if (gBgmId == BGM_QUEST_LOST) {
            Stream_Pause(BGM_CH_MUSIC, 0);
            Stream_SetVol(BGM_CH_MUSIC, 0);
            Stream_FadeToMax(BGM_CH_MUSIC, BGM_FADE_FRAMES);
        } else {
            gScene.musicState = MUSIC_LOST;
            Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_QUEST_LOST, BGM_FADE_FRAMES, BGM_VOL_MAX);
            gBgmId = BGM_QUEST_LOST;
        }
    } else {
        if (gQuest.flags & 0x800) {
            Stream_PlayFadeIn(BGM_CH_MUSIC, BGM_QUEST_THEME, BGM_FADE_FRAMES, BGM_VOL_MAX);
            gBgmId = BGM_QUEST_THEME;
            gBgmVolume = 0x7F;
        } else if (gQuest.flags & 0x400) {
            gBgmVolume = 0x7F;
            Bgm_PlayBattle();
        } else if (gScene.musicFlags == 0 && (id = Bgm_AreaTune(area)) != 0) {
            Stream_PlayFadeIn(BGM_CH_MUSIC, id, BGM_FADE_FRAMES, BGM_VOL_MAX);
            gBgmId = id;
            gBgmVolume = 0x7F;
            gScene.musicFlags |= MUSIC_FLAG_AREA_TUNE;
        } else {
            id = Bgm_GetAreaMusic(area);
            if (gBgmId == id) {
                Stream_Pause(BGM_CH_MUSIC, 0);
                Stream_SetVol(BGM_CH_MUSIC, 0);
                Stream_FadeTo(BGM_CH_MUSIC, BGM_FADE_FRAMES, gBgmVolume);
            } else {
                Stream_PlayFadeIn(BGM_CH_MUSIC, id, BGM_FADE_FRAMES, gBgmVolume);
            }
            gBgmId = id;
        }
        gScene.musicState = MUSIC_AREA;
    }
    gBgmIndex = gBgmVariant;
    Sound_SetEffectOnce(0, gAreaReverbs[gAreaReverbIdx[area]].mode0, gAreaReverbs[gAreaReverbIdx[area]].depth0, 0, 0);
    Sound_SetEffectOnce(1, gAreaReverbs[gAreaReverbIdx[area]].mode1, gAreaReverbs[gAreaReverbIdx[area]].depth1, 0, 0);
}

/* The file's data: the chat boxes, then the music's tables, the sound
 * tables of the areas and quests, and the marker's colours. */
extern u32 D_0043C5D0[];
ListPanel gChatBoxPanels[3] = {
#include "sub_main/0043B0E0.inc"
};
BgmEntry gTownTuneEvents[4] = {
#include "sub_main/0043B140.inc"
};
BgmEntry gTownTuneSeasons[4] = {
#include "sub_main/0043B150.inc"
};
BgmEntry gTownTuneTimes[4] = {
#include "sub_main/0043B160.inc"
};
u16 gBgmHub83[16] = {
#include "sub_main/0043B170.inc"
};
u16 gBgmHub84[16] = {
#include "sub_main/0043B190.inc"
};
u16 gBgmOnlineHouse[16] = {
#include "sub_main/0043B1B0.inc"
};
u16 gBgmFarm[12] = {
#include "sub_main/0043B1D0.inc"
};
u16 gBgmHouse[6] = {
#include "sub_main/0043B1E8.inc"
};
BgmEntry gBgmTavernTracks[6] = {
#include "sub_main/0043B200.inc"
};
s16 gBgmTavernTrackFrames[6] = {
#include "sub_main/0043B218.inc"
};
AreaMusic gAreaMusic[200] = {
#include "sub_main/0043B230.inc"
};
AreaReverb gAreaReverbs[10] = {
#include "sub_main/0043B550.inc"
};
u8 gAreaReverbIdx[208] = {
#include "sub_main/0043B5F0.inc"
};
u32 gAreaSeBanks[128] = {
#include "sub_main/0043B6C0.inc"
};
AreaMusicAlt gAreaMusicAlts[68] = {
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
AreaJingles gAreaJingles[35] = {
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
BgmKey gBattleBgmKinds[20] = {
#include "sub_main/0043C7D0.inc"
};
s32 gMarkerColors[4] = {
#include "sub_main/0043C820.inc"
};
u16 gMapBattleBgm[8] = {
#include "sub_main/0043C830.inc"
};
u32 gTextCodeColors[12] = {
#include "sub_main/0043C840.inc"
};
BgmKey gAreaTunes[20] = {
#include "sub_main/0043C870.inc"
};
