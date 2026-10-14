#include "common.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"

typedef struct SprUV {
    s16 u;
    s16 v;
} SprUV;
typedef struct SprRect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
    s32 color;
    SprUV uv0;
    SprUV uv1;
} SprRect;
typedef struct SprUV2 {
    SprUV uv0;
    SprUV uv1;
} SprUV2;
extern SprUV2 gLobbySpriteUVs[];
struct Sprite;
s32 Sprite_Draw(struct Sprite *);

void Text_PrintfColored(const char *, ...);
void Win_Draw(s32 x, s32 y, s32 w, s32 h, s32 color, s32 color2, s16 kind);
s32 MyHouse_InOwnHouse(void);
s8 MyHouse_LocalSlot(void);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
s32 Game_IsOnline(void);
extern char D_0083B7D0[];
extern char D_0083B7D8[];
extern char *gMyHouseLabelTexts[];
extern char *D_0080C1DC;
extern s32 gTownMode;
extern s16 gHouseCloseTimer;
extern u8 gNetUsers[];

void MyHouse_DrawLabel(void)
{
    s8 slot = MyHouse_LocalSlot();

    if (Game_IsOnline() == 0) {
        return;
    }
    switch (gScene.area) {
    case 0xAD:
    case 0xAE:
    case 0xAF:
        if (MyHouse_InOwnHouse() == 1) {
            return;
        }
        break;
    default:
        return;
    }
    if (gTownMode == 0x46) {
        return;
    }
    Win_Draw(400, 24, 210, 22, 0xBDB59F6C, 0x80202020, 6);
    Font_SetPos(400, 24);
    Font_SetSpacing(20, 20);
    if (gHouseCloseTimer != -1) {
        Font_SetColor(0x807F7F28);
        Text_PrintfColored(D_0083B7D0, D_0080C1DC, gHouseCloseTimer / 30 + 1);
        return;
    }
    if (slot != -1) {
        Font_SetColor(0x807F7F7F);
        if (gChatHideNames != 0) {
            Text_PrintfColored(D_0083B7D8, gMyHouseLabelTexts[0], gNetUsers + slot * 0x11C);
        } else {
            Text_PrintfColored(D_0083B7D8, gMyHouseLabelTexts[0], gNetUsers + slot * 0x11C + 8);
        }
    }
}

void Text_SetSize(s32);
void MyHouse_DrawLabel(void);
void NameTags_Draw(void *);
s32 Kbd_IsOpen(void); /* unit's own declaration: the definition (sub_main) returns s8;
 * NameTags_Draw is 487 words off with s8 */
extern s32 gTownMode;

void Lobby_DrawNameLayer(u8 *t)
{
    Text_SetSize(*(s32 *)(t + 0x18));
    switch (gTownMode) {
    case 0x1A:
        MyHouse_DrawLabel();
        break;
    case 0xB:
        MyHouse_DrawLabel();
        if (Kbd_IsOpen() != 0) {
            break;
        }
    case 0x2:
    case 0x7:
    case 0x1F:
    case 0x20:
    case 0x35:
    case 0x3F:
    case 0x34:
    case 0x1:
    case 0x14:
    case 0x4:
    case 0x5:
    case 0x2C:
    case 0x2D:
    case 0x4A:
    case 0x43:
    case 0x19:
    case 0x36:
    case 0x37:
    case 0x47:
    case 0x48:
    case 0x33:
    case 0x46:
    case 0x0:
        MyHouse_DrawLabel();
        NameTags_Draw(t);
        break;
    }
}

void Tex_Prepare(s32, s32);
void Tex_Bind(s32);
void Gfx_SetState63(s32);
s32 Gs_SetState(u8, u32);
void Camera_ProjectPoint(f32 *out, f32 *pos);
void Mtx_Identity(f32 *m);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
/* unit's own declaration: NameTags_Draw passes its (s16) casts as ints; (s32...) changes it by 190 words, (s16...) by 41 */
void Icon_Draw();
s32 TownPc_IsInMyArea(Player *p);
s32 NetUser_State(s8 i);
s32 Lobby_StrLength(char *);
s32 TownPc_IsSeatedState(u8, u8);
f32 Player_NameTagRaise(Player *p);
u32 Color_Cycle8(void);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void Lobby_DrawSpriteFE9B0(s32 x, s16 y, s32 w, s32 color, s16 kind);
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
void Icon_DrawKind48(s32, s32, s32, s32, s16, s8);
u8 Equip_GetClass(EquipItem *e);
void NameTags_DrawNpcs(void);
extern s32 gMemberFlagSprites[];
extern s32 gPlayerColorWords[];

/* The names over the players' heads (with the weapon class, the marks from
 * gLobby and an icon), then the name at gSave + 0x2C98 over the
 * townspeople of kind 0x14. The second loop uses the last player's name
 * length (len) for its offset. */
void NameTags_Draw(void *t)
{
    f32 pos[4];
    f32 scr[4];
    f32 m[16];
    s16 len;
    s16 sx;
    SaveData *save;
    s32 off;
    s16 i;
    Player *p;
    u8 *info;
    char *name;
    s16 sy;
    s16 c;
    u8 *st;
    s32 icon;
    Em *e;
    s8 kind;

    save = &gSave;
    switch (gTownMode) {
    case 11:
        if (Kbd_IsOpen() != 0) {
            return;
        }
    case 0:
    case 7:
    case 25:
        break;
    default:
        return;
    }
    {
        Text_SetSize(*(s32 *)((u8 *)t + 0x18));
        Tex_Prepare(1, 0x155);
        Tex_Bind(0x155);
        Gfx_SetState63(1);
        Gs_SetState(0x60, 0);
        i = 0;
        p = gPlayers;
        info = gNetUsers;
        off = 0;
        do {
            if (p->active != 0 && p->visible != 0 && (u8)TownPc_IsInMyArea(p) != 0 && NetUser_State(i) == 0 &&
                (p->pos[0] != 0.0f || p->pos[2] != 0.0f) &&
                (p->stateGroup != 0 || (u32)(p->state - 0x14) > 1)) {
                name = (char *)info + 8;
                if (Game_IsOnline() == 1 && gChatHideNames != 0) {
                    name = (char *)info;
                }
                len = Lobby_StrLength(name);
                Mtx_Identity(m);
                Gs_SetState(0x1A, (s32)m);
                if (TownPc_IsSeatedState(p->stateGroup, p->state) == 0) {
                    pos[0] = p->pos[0];
                    pos[1] = 190.0f + p->pos[1];
                    pos[2] = p->pos[2];
                } else {
                    Em_GetBonePos(p, 0x14, pos);
                    pos[1] += 40.0f;
                }
                pos[1] += Player_NameTagRaise(p);
                Camera_ProjectPoint(scr, pos);
                if (scr[0] < 700.0f && scr[0] > -60.0f && scr[1] < 500.0f && scr[1] > -20.0f && scr[3] > 0.0f) {
                    scr[0] -= (f32)(len / 2 * 8);
                    sx = 1.25f * scr[0];
                    sy = scr[1];
                    Font_SetSpacing(16, 16);
                    st = (u8 *)gLobby + off + 0xA32;
                    if (*(u16 *)st >= 0x33) {
                        c = 2;
                    } else if (*(u16 *)st >= 0x1F) {
                        c = 6;
                    } else {
                        c = 5;
                    }
                    Text_DrawShadow(1.25f * (s32)scr[0], scr[1], 1, c, name);
                    if (i == gScene.localPlayer) {
                        kind = Equip_GetClass(gSave.equip);
                        Icon_DrawKind48((s16)(sx - 0x16), (s16)(sy - 4), 0x16, -1, kind, 0);
                    } else {
                        Icon_DrawKind48((s16)(sx - 0x16), (s16)(sy - 4), 0x16, -1, st[3], 0);
                    }
                    if (st[8] & 0x80) {
                        Lobby_DrawSpriteFE9B0((s16)(sx + len * 4 - 0xC), sy - 0x1A, 0x16, gMemberFlagSprites[st[8] & 0xF], 10);
                    } else if (st[8] & 0x40) {
                        if (!(st[8] & 0x20) || (gFrameTick & 0x10)) {
                            Lobby_DrawSpriteFE9B0((s16)(sx + len * 4 - 0xC), sy - 0x1A, 0x16, gMemberFlagSprites[st[8] & 0xF], 9);
                        }
                    }
                    if (Game_IsOnline() == 1) {
                        icon = st[0xC];
                    } else if (gSave.styleBits == 0) {
                        icon = 0xFF;
                    } else {
                        icon = gPlayers[gScene.localPlayer].style;
                    }
                    if (icon != 0xFF) {
                        if (icon == 0xB) {
                            Icon_Draw((s16)(sx + len * 8 + 6), (s16)(sy - 4), 0x14, 0x14, Color_Cycle8(), 0x62);
                        } else {
                            Icon_Draw((s16)(sx + len * 8 + 6), (s16)(sy - 4), 0x14, 0x14, gPlayerColorWords[icon], 0x62);
                        }
                    }
                }
            }
            p++;
            off += 0x11C;
            i++;
            info += 0x11C;
        } while (i < 10);
        e = gEms;
        i = 0;
        do {
            if (e->active != 0 && e->work == 0x14 && (s8)save->poogieName[0] != 0 && MyHouse_InOwnHouse() == 1) {
                Em_BonePos(e, 0xA, pos);
                pos[1] += 50.0f;
                Camera_ProjectPoint(scr, pos);
                if (scr[0] < 700.0f && scr[0] > -60.0f && scr[1] < 500.0f && scr[1] > -20.0f && scr[3] > 0.0f) {
                    scr[0] -= (f32)(len / 2 * 8);
                    sx = 1.25f * scr[0];
                    sy = scr[1];
                    Font_SetSpacing(16, 16);
                    Text_DrawShadow(sx, sy, 1, 5, (char *)save->poogieName);
                }
            }
            i++;
            e++;
        } while (i < 20);
        NameTags_DrawNpcs();
    }
}

void Em_GetBonePos(void *obj, s16 bone, f32 *out);
void Icon_Draw(s32, s32, s32, s32, s32, s32); /* unit's own declaration: with the definition's void (s16 x, s16 y, s16 w, s16 h, u32 color, s32 i) (lobby/unk_790EC0.c), NameTags_DrawNpcs 41 of 325 words differ */
void Mtx_Identity(f32 *m);
s32 Npc_IsFree(Em *arg0);
void Vec_Copy(f32 *out, f32 *in);
f32 Vec_Distance(f32 *a, f32 *b);
f32 Vec_Length(f32 *v);
s32 Village_NpcRecA(u16 kind);
s32 Npc_IsBusyNotAct3C4B(Em *arg0);
s32 Npc_IsAct51to55(Em *arg0);
s32 Npc_IsAct4For50(Em *arg0);
u8 Area_GetSetIndex(u16 id);
s32 Gs_SetState(u8, u32);
void Camera_ProjectPoint(f32 *out, f32 *pos);

extern u16 gFrameTick; /* sub_main: the frame counter */

/* The frame counter is read as a named object (gFrameTick): the game loads
   it above the store of scr[0], which MWCC does only for a named one. */
void NameTags_DrawNpcs(void)
{
    f32 at[4];
    f32 d[4];
    f32 bone[4];
    f32 scr[4];
    f32 m[16];
    Player *me;
    Em *e;
    u8 *w;
    s16 i;
    u32 alpha;
    s32 sw;
    s32 sh;
    f32 scale;
    f32 dist;
    f32 lim;
    u32 color;

    e = gEms;
    i = 0;
    me = &gPlayers[gScene.localPlayer];
    do {
        w = &e->work;
        if (e->active != 0 && (*(s8 *)(w + 4) != 0 || Village_NpcRecA(w[0]) != 0)) {
            Mtx_Identity(m);
            Gs_SetState(0x1A, (s32)m);
            if (Npc_IsFree((Em *)((u8 *) e)) != 0) {
                Em_GetBonePos(e, 0x14, bone);
            } else if (Npc_IsAct51to55(e) != 0) {
                Em_GetBonePos(e, 0xA, bone);
            } else if (Npc_IsBusyNotAct3C4B((Em *)((u8 *) e)) != 0) {
                Em_GetBonePos(e, 0xC, bone);
            } else if (Npc_IsAct4For50(e) != 0) {
                Em_GetBonePos(e, 0xA, bone);
                bone[1] += 40.0f;
            } else {
                bone[0] = e->posX;
                bone[1] = e->posY + 200.0f * e->scaleY;
                bone[2] = e->posZ;
            }
            d[0] = *(f32 *)(w + 0x50) - bone[0];
            d[1] = *(f32 *)(w + 0x54) - bone[1];
            d[2] = *(f32 *)(w + 0x58) - bone[2];
            if (Vec_Length(d) < 20.0f) {
                Vec_Copy(at, (f32 *)(w + 0x50));
            } else {
                Vec_Copy(at, bone);
            }
            switch ((u8)Area_GetSetIndex(gScene.area)) {
            case 0xA:
            case 0xE:
            case 0xB:
            case 0xC:
            case 0xD:
                at[1] += 40.0f * e->scaleY;
                scale = 0.7f;
                break;
            default:
                at[1] += 70.0f * e->scaleY;
                scale = 1.0f;
                break;
            }
            Vec_Copy((f32 *)(w + 0x50), bone);
            Camera_ProjectPoint(scr, at);
            if (scr[0] < 700.0f && !(scr[0] <= -60.0f) && scr[1] < 500.0f && !(scr[1] <= -20.0f) && !(scr[3] <= 0.0f)) {
                scr[0] *= 1.25f;
                if (gFrameTick & 0x10) {
                    sw = 26;
                    sh = 22;
                } else {
                    sw = 22;
                    sh = 20;
                }
                dist = Vec_Distance(me->pos, &e->posX);
                if (dist <= 400.0f * scale) {
                    alpha = 0xC0;
                } else if (dist <= (lim = 1000.0f * scale)) {
                    alpha = 192.0f * ((lim - dist) / (600.0f * scale));
                } else {
                    alpha = 0;
                }
                if (Village_NpcRecA(w[0]) == 1 || *(s8 *)(w + 4) == 2) {
                    color = (alpha << 24) | 0xC08080;
                } else {
                    color = (alpha << 24) | 0x80C0C0;
                }
                Icon_Draw((s32)scr[0], (s32)scr[1], sw, sh, color, 0x54);
            }
        }
        i++;
        e++;
    } while (i < 20);
}

typedef struct ActSpot {
    u16 area;
    u16 kind;
} ActSpot;
s32 sprintf(char *buf, char *fmt, ...);
void *LobbyArea_Name(u16 id);
s32 TownPc_IsSeatedState(u8, u8);
s32 Lobby_StrLength(char *);
void Gfx_DrawPulseGradient(s16 x, s16 y, s16 w, s16 h, u32 color, u8 dir);
s16 Lobby_DrawSprite155(s16 x, s16 y, s32 i);
extern char *D_0080C15C;
extern char *gTargetSelectTexts[];
extern char *gSpotActionTexts[];
extern char *gStandUpTexts[];
extern char *D_0080D4E4;
extern char *gSaveGameTexts[];
extern char D_0083B7E0[];
extern char D_0083B7E8[];
extern s8 gNameTagCycle;
extern u8 gNameTagsHidden;
extern u8 gLockOnNoCycle;

void NameTags_DrawMembers(u8 *t)
{
    char buf[0x40];
    Player *me = &gPlayers[gScene.localPlayer];
    u8 *save = (u8 *)&gSave;
    u8 *obj;
    ActSpot *spot;
    u8 *w;
    u16 id;
    u32 color;

    switch (gTownMode) {
    case 7:
    case 0:
    case 6:
    case 0x19:
        break;
    default:
        return;
    }
    if (me->stateGroup != 0 || me->state != 0x25) {
        if (TownPc_IsSeatedState(me->stateGroup, me->state) == 1) {
            gNameTagsHidden = 1;
            return;
        }
    }
    if (((u8 *)me->target) == NULL) {
        gNameTagsHidden = 1;
        return;
    }
    if (me->stateGroup == 1) {
        gNameTagsHidden = 1;
        return;
    }
    if (me->stateGroup == 0) {
        switch (me->state) {
        case 0x15:
        case 0xA:
        case 0xB:
        case 0xC:
            gNameTagsHidden = 1;
            return;
        }
    }
    Text_SetSize(*(s32 *)(t + 0x18));
    Font_SetColor(0x807F7F7F);
    Font_SetSpacing(20, 20);
    if ((s8)Kbd_IsOpen() != 0) {
        gNameTagsHidden = 1;
        return;
    }
    Font_SetSpacing(20, 20);
    if (gNameTagsHidden == 1) {
        gNameTagsHidden = 0;
        gNameTagCycle = 40;
    } else if (--gNameTagCycle <= 0) {
        gNameTagCycle = 40;
    }
    obj = ((u8 *)me->target);
    if (obj == (u8 *)me && (spot = me->spot) != NULL) {
        if ((spot->kind == 1 || spot->kind == 10) && me->state == 0x25) {
            sprintf(buf, gStandUpTexts[0]);
        } else {
            switch (spot->kind) {
            default:
                if (gTownMode != 7) {
                    sprintf(buf, gSpotActionTexts[spot->kind]);
                    break;
                }
                return;
            case 0x11:
                if (gTownMode == 7) {
                    if (gLobby->questPaid != 0) {
                        return;
                    }
                    sprintf(buf, D_0080D4E4);
                } else {
                    sprintf(buf, gSpotActionTexts[spot->kind]);
                }
                break;
            case 5:
                if (Game_IsOnline() == 1) {
                    sprintf(buf, gSaveGameTexts[0]);
                } else {
                    sprintf(buf, gSpotActionTexts[((ActSpot *)me->spot)->kind]);
                }
                break;
            case 0x18:
                if (spot->area == 0x89 && Game_IsOnline() == 1) {
                    sprintf(buf, gSpotActionTexts[0], LobbyArea_Name(0x84));
                } else {
                    spot = me->spot;
                    if ((spot->area == 0x85 && gScene.area == 0x86) || (spot->area == 0x86 && gScene.area == 0x85)) {
                        sprintf(buf, gSpotActionTexts[spot->kind]);
                    } else {
                        sprintf(buf, gSpotActionTexts[0], LobbyArea_Name(spot->area));
                    }
                }
                break;
            }
        }
        color = 0x80006000;
    } else if (obj[0x1E] != 0) {
        if (gTownMode == 7) {
            return;
        }
        w = obj + 0x438;
        if (*(s8 *)(obj + 0x43A) == 1) {
            return;
        }
        sprintf(buf, D_0083B7E0, *(s32 *)(w + 8));
        if (w[0] == 0x14 && *(s8 *)(save + 0x2C98) == 0) {
            sprintf(buf, D_0083B7E8);
        }
        color = 0x80202060;
    } else {
        id = *(u16 *)(obj + 0xC);
        if (id == me->index) {
            return;
        }
        if (gTownMode == 7) {
            return;
        }
        if (gChatHideNames != 0) {
            sprintf(buf, D_0083B7D8, gNetUsers + id * 0x11C, D_0080C15C);
        } else {
            sprintf(buf, D_0083B7D8, gNetUsers + id * 0x11C + 8, D_0080C15C);
        }
        color = 0x80603000;
    }
    Gfx_DrawPulseGradient(0, 0x6F, Lobby_StrLength(buf) * 9 + 0x50, 0x1E, color, 0);
    if (gTownMode != 7 && TownPc_IsSeatedState(me->stateGroup, me->state) == 0 && gLockOnNoCycle == 0) {
        Gfx_DrawPulseGradient(0, 0x8F, 200, 0x1E, color, 0);
    }
    if (gPadState[0].pressed & 0x20) {
        Lobby_DrawSprite155(20, 0x74, 0);
    } else {
        Lobby_DrawSprite155(20, 0x72, 0);
    }
    if (gTownMode != 7 && TownPc_IsSeatedState(me->stateGroup, me->state) == 0 && gLockOnNoCycle == 0) {
        if (gPadState[0].pressed & 0xC00) {
            Lobby_DrawSprite155(20, 0x94, 16);
        } else {
            Lobby_DrawSprite155(20, 0x92, 16);
        }
    }
    Font_SetPos(0x38, 0x73);
    Text_PrintfColored(buf);
    if (gTownMode != 7 && TownPc_IsSeatedState(me->stateGroup, me->state) == 0 && gLockOnNoCycle == 0) {
        Font_SetPos(0x38, 0x93);
        Text_PrintfColored(D_0083B7E0, gTargetSelectTexts[0]);
    }
}

typedef struct GradRect {
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
    u32 c[4];
} GradRect;
f32 Math_Sin(f32 x);
s32 Gs_SetState(u8, u32);
struct PrimGradRect;
s32 Prim_FillGradRect(struct PrimGradRect *);

void Gfx_DrawPulseGradient(s16 x, s16 y, s16 w, s16 h, u32 color, u8 dir)
{
    GradRect r;
    u16 t = (gFrameTick & 0x3F) << 10;
    f32 s = Math_Sin(0.0000958738f * t);

    r.x0 = x;
    r.y0 = y;
    r.x1 = x + w;
    r.y1 = y + h;
    if (dir == 0) {
        r.c[0] = (color & 0xFFFFFF) | (((s8)(48.0f * s) + 0x90) << 24);
        r.c[1] = color & 0xFFFFFF;
    } else {
        r.c[0] = color & 0xFFFFFF;
        r.c[1] = (color & 0xFFFFFF) | (((s8)(48.0f * s) + 0x90) << 24);
    }
    r.c[2] = r.c[0];
    r.c[3] = r.c[1];
    Gs_SetState(0x60, 0);
    Prim_FillGradRect((struct PrimGradRect *)&r);
}

extern SprUV2 gSprite155UVs[];
void Gfx_SetState63(s32);
s32 Gs_SetState(u8, u32);
void Tex_Prepare(s32, s32);
void Tex_Bind(s32);

s16 Lobby_DrawSprite155(s16 x, s16 y, s32 i)
{
    SprRect r;
    SprUV2 *t = &gSprite155UVs[i];

    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    Gfx_SetState63(1);
    Gs_SetState(0x60, 0);
    r.y = y;
    r.x = (f32)x;
    r.w = (f32)(t->uv1.u - t->uv0.u);
    r.h = t->uv1.v - t->uv0.v;
    if (i == 9) {
        r.w = 50;
        r.h = 32;
    }
    r.color = -1;
    r.uv0 = t->uv0;
    r.uv1 = t->uv1;
    Sprite_Draw((struct Sprite *)&r);
    return (s16)x + (t->uv1.u - t->uv0.u);
}

void Icon_Draw(s32, s32, s32, s32, s32, s32); /* unit's own declaration: with the definition's void (s16 x, s16 y, s16 w, s16 h, u32 color, s32 i) (lobby/unk_790EC0.c), Icon_DrawKind48 41 of 47 words differ */

void Icon_DrawKind48(s32 a, s32 b, s32 c, s32 d, s16 e, s8 f)
{
    if (f != 0) {
        Icon_Draw(a, b, (s32)(1.2f * c), c, d, e + 0x81);
    } else {
        Icon_Draw(a, b, c, c, d, e + 0x48);
    }
}

s32 Forge_IsFixedPrice(s16 id);
u8 Weapon_GetClass(u8 type, u16 id);
/* unit's own declaration: with the definition's u8 (u8 type, u16 id) (sub_main/unk_3B4C80.c), Lobby_DrawEquipIcon: 9 of 96 words differ */
s32 Equip_GetRarity(s32 type, u16 id);
u32 Equip_Color(u8 idx, u8 alpha, s32 alt);
void Icon_DrawEquip(s32, s32, s32, s32, s32, s32);

void Lobby_DrawEquipIcon(s32 x, s32 y, s32 w, s32 h, EquipItem *e)
{
    s32 kind = e->type;
    s32 color;
    s16 spr;

    if (kind == 7 || kind == 6) {
        if (Forge_IsFixedPrice(e->id) != 0) {
            color = -1;
            spr = 0x10;
        } else {
            color = Equip_Color(Equip_GetRarity(e->type, e->id), 0xFF, 0);
            spr = Weapon_GetClass(e->type, e->id);
            spr += 5;
        }
    } else {
        switch (kind) {
        case 0:
            spr = 4;
            break;
        case 2:
            spr = 0;
            break;
        case 3:
            spr = 1;
            break;
        case 4:
            spr = 3;
            break;
        case 5:
            spr = 2;
            break;
        }
        color = Equip_Color(Equip_GetRarity(kind, e->id), 0xFF, 0);
    }
    Icon_DrawEquip(x, y, w, h, color, spr);
}

void Icon_Draw(s32, s32, s32, s32, s32, s32); /* unit's own declaration: with the definition's void (s16 x, s16 y, s16 w, s16 h, u32 color, s32 i) (lobby/unk_790EC0.c), Icon_DrawKind59 20 of 29 words differ */

void Icon_DrawKind59(s32 a, s32 b, s32 c, s32 d, s16 e)
{
    if (e != 0) {
        Icon_Draw(a, b, (s32)(1.25f * c), c, d, e + 0x59);
    }
}

void Lobby_DrawSpriteFE9B0(s32 x, s16 y, s32 w, s32 color, s16 kind)
{
    SprRect r;
    SprUV2 *t;

    if (kind != 0x16) {
        t = &gLobbySpriteUVs[kind];
        r.y = y;
        r.x = (f32)x;
        r.w = (f32)w;
        if (r.w < 0) {
            r.w = -r.w;
        }
        r.h = w;
        r.color = color;
        r.uv0 = t->uv0;
        r.uv1 = t->uv1;
        Sprite_Draw((struct Sprite *)&r);
    }
}

/* unk_78F070's .data (0x7FE850-0x7FEAF0) */
s32 gMemberFlagSprites[8] = {
#include "lobby/007FE850.inc"
};
u32 gLabelIconsNeg[20] = {
#include "lobby/007FE870.inc"
};
u32 gLabelIconsPos[20] = {
#include "lobby/007FE8C0.inc"
};
u32 gFrameStripColors[8] = {
#include "lobby/007FE910.inc"
};
u32 gStripTurnOffsets[32] = {
#include "lobby/007FE930.inc"
};
SprUV2 gLobbySpriteUVs[22] = {
#include "lobby/007FE9B0.inc"
};
SprUV2 gSprite155UVs[18] = {
#include "lobby/007FEA60.inc"
};

/* unk_78F070's .rodata (0x83B7D0-0x83B7F8) */
#pragma rodata on
char D_0083B7D0[0x8] = {
#include "lobby/0083B7D0.inc"
};
char D_0083B7D8[0x8] = {
#include "lobby/0083B7D8.inc"
};
char D_0083B7E0[0x8] = {
#include "lobby/0083B7E0.inc"
};
char D_0083B7E8[0xC] = {
#include "lobby/0083B7E8.inc"
};
#pragma rodata off
