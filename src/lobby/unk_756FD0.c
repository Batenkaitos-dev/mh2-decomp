/* 0x756FD0-0x7571D0: the end of zlib 1.1.4's inftrees.c
 * (inflate_trees_fixed) and infutil.c (inflate_flush), an altered source
 * version of zlib's code under zlib's license (the notice is before them);
 * then lobby helpers, first-pass C from m2c's output. */
#include "common.h"
#include "pad.h"
#include "em.h"
#include "m2c_macros.h"
#define ZLIB_INTERNAL
#include "zlib.h"
#include "lobby_globals.h"

/* prototypes */
s32 LobbyPad_Pressed(s32 arg0);
s32 LobbyPad_Held(s32 arg0);
void Lobby_BackupWork(void);
void Lobby_RestoreWork(void);
void LobbyStream_FadeOut(s32 arg0);
s32 SeqA_IsPlaying(void);
void Text_SetColorIndex(s16 i);
s32 Lobby_IsSysBusy(void);
void EquipShop_Close(void);
/* end of prototypes */

void Stream_Stop(s32);
void NetMode_ReleaseTextures(M2C_UNK unused0);
extern M2C_UNK gNetSaveData;
extern M2C_UNK D_5BF608;
extern M2C_UNK gMailBlockList;
extern M2C_UNK gFriendList;
void Stream_FadeOut(s32, s32);
void Font_SetColorIndex(s16 i);
void CamFocus_End(void);
void StageModels_Free(void);
void Chat_SetColor(s16);
void LobbySe_Play(s32 se);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMode;
extern s32 gTownMenuOpen;

/* ---- zlib 1.1.4: the end of inftrees.c, and infutil.c ----
 *
 * Altered source version: zlib's code with ANSI definitions, in the file
 * layout of the decompilation. The original notice, from zlib.h:
 *
 * Copyright (C) 1995-2002 Jean-loup Gailly and Mark Adler
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Jean-loup Gailly        Mark Adler
 * jloup@gzip.org          madler@alumni.caltech.edu
 */

/* zlib inflate's fixed Huffman tables (inffixed.h). */
uInt fixed_bl = 9;
uInt fixed_bd = 5;
inflate_huft fixed_tl[] = {
    {{{96,7}},256}, {{{0,8}},80}, {{{0,8}},16}, {{{84,8}},115},
    {{{82,7}},31}, {{{0,8}},112}, {{{0,8}},48}, {{{0,9}},192},
    {{{80,7}},10}, {{{0,8}},96}, {{{0,8}},32}, {{{0,9}},160},
    {{{0,8}},0}, {{{0,8}},128}, {{{0,8}},64}, {{{0,9}},224},
    {{{80,7}},6}, {{{0,8}},88}, {{{0,8}},24}, {{{0,9}},144},
    {{{83,7}},59}, {{{0,8}},120}, {{{0,8}},56}, {{{0,9}},208},
    {{{81,7}},17}, {{{0,8}},104}, {{{0,8}},40}, {{{0,9}},176},
    {{{0,8}},8}, {{{0,8}},136}, {{{0,8}},72}, {{{0,9}},240},
    {{{80,7}},4}, {{{0,8}},84}, {{{0,8}},20}, {{{85,8}},227},
    {{{83,7}},43}, {{{0,8}},116}, {{{0,8}},52}, {{{0,9}},200},
    {{{81,7}},13}, {{{0,8}},100}, {{{0,8}},36}, {{{0,9}},168},
    {{{0,8}},4}, {{{0,8}},132}, {{{0,8}},68}, {{{0,9}},232},
    {{{80,7}},8}, {{{0,8}},92}, {{{0,8}},28}, {{{0,9}},152},
    {{{84,7}},83}, {{{0,8}},124}, {{{0,8}},60}, {{{0,9}},216},
    {{{82,7}},23}, {{{0,8}},108}, {{{0,8}},44}, {{{0,9}},184},
    {{{0,8}},12}, {{{0,8}},140}, {{{0,8}},76}, {{{0,9}},248},
    {{{80,7}},3}, {{{0,8}},82}, {{{0,8}},18}, {{{85,8}},163},
    {{{83,7}},35}, {{{0,8}},114}, {{{0,8}},50}, {{{0,9}},196},
    {{{81,7}},11}, {{{0,8}},98}, {{{0,8}},34}, {{{0,9}},164},
    {{{0,8}},2}, {{{0,8}},130}, {{{0,8}},66}, {{{0,9}},228},
    {{{80,7}},7}, {{{0,8}},90}, {{{0,8}},26}, {{{0,9}},148},
    {{{84,7}},67}, {{{0,8}},122}, {{{0,8}},58}, {{{0,9}},212},
    {{{82,7}},19}, {{{0,8}},106}, {{{0,8}},42}, {{{0,9}},180},
    {{{0,8}},10}, {{{0,8}},138}, {{{0,8}},74}, {{{0,9}},244},
    {{{80,7}},5}, {{{0,8}},86}, {{{0,8}},22}, {{{192,8}},0},
    {{{83,7}},51}, {{{0,8}},118}, {{{0,8}},54}, {{{0,9}},204},
    {{{81,7}},15}, {{{0,8}},102}, {{{0,8}},38}, {{{0,9}},172},
    {{{0,8}},6}, {{{0,8}},134}, {{{0,8}},70}, {{{0,9}},236},
    {{{80,7}},9}, {{{0,8}},94}, {{{0,8}},30}, {{{0,9}},156},
    {{{84,7}},99}, {{{0,8}},126}, {{{0,8}},62}, {{{0,9}},220},
    {{{82,7}},27}, {{{0,8}},110}, {{{0,8}},46}, {{{0,9}},188},
    {{{0,8}},14}, {{{0,8}},142}, {{{0,8}},78}, {{{0,9}},252},
    {{{96,7}},256}, {{{0,8}},81}, {{{0,8}},17}, {{{85,8}},131},
    {{{82,7}},31}, {{{0,8}},113}, {{{0,8}},49}, {{{0,9}},194},
    {{{80,7}},10}, {{{0,8}},97}, {{{0,8}},33}, {{{0,9}},162},
    {{{0,8}},1}, {{{0,8}},129}, {{{0,8}},65}, {{{0,9}},226},
    {{{80,7}},6}, {{{0,8}},89}, {{{0,8}},25}, {{{0,9}},146},
    {{{83,7}},59}, {{{0,8}},121}, {{{0,8}},57}, {{{0,9}},210},
    {{{81,7}},17}, {{{0,8}},105}, {{{0,8}},41}, {{{0,9}},178},
    {{{0,8}},9}, {{{0,8}},137}, {{{0,8}},73}, {{{0,9}},242},
    {{{80,7}},4}, {{{0,8}},85}, {{{0,8}},21}, {{{80,8}},258},
    {{{83,7}},43}, {{{0,8}},117}, {{{0,8}},53}, {{{0,9}},202},
    {{{81,7}},13}, {{{0,8}},101}, {{{0,8}},37}, {{{0,9}},170},
    {{{0,8}},5}, {{{0,8}},133}, {{{0,8}},69}, {{{0,9}},234},
    {{{80,7}},8}, {{{0,8}},93}, {{{0,8}},29}, {{{0,9}},154},
    {{{84,7}},83}, {{{0,8}},125}, {{{0,8}},61}, {{{0,9}},218},
    {{{82,7}},23}, {{{0,8}},109}, {{{0,8}},45}, {{{0,9}},186},
    {{{0,8}},13}, {{{0,8}},141}, {{{0,8}},77}, {{{0,9}},250},
    {{{80,7}},3}, {{{0,8}},83}, {{{0,8}},19}, {{{85,8}},195},
    {{{83,7}},35}, {{{0,8}},115}, {{{0,8}},51}, {{{0,9}},198},
    {{{81,7}},11}, {{{0,8}},99}, {{{0,8}},35}, {{{0,9}},166},
    {{{0,8}},3}, {{{0,8}},131}, {{{0,8}},67}, {{{0,9}},230},
    {{{80,7}},7}, {{{0,8}},91}, {{{0,8}},27}, {{{0,9}},150},
    {{{84,7}},67}, {{{0,8}},123}, {{{0,8}},59}, {{{0,9}},214},
    {{{82,7}},19}, {{{0,8}},107}, {{{0,8}},43}, {{{0,9}},182},
    {{{0,8}},11}, {{{0,8}},139}, {{{0,8}},75}, {{{0,9}},246},
    {{{80,7}},5}, {{{0,8}},87}, {{{0,8}},23}, {{{192,8}},0},
    {{{83,7}},51}, {{{0,8}},119}, {{{0,8}},55}, {{{0,9}},206},
    {{{81,7}},15}, {{{0,8}},103}, {{{0,8}},39}, {{{0,9}},174},
    {{{0,8}},7}, {{{0,8}},135}, {{{0,8}},71}, {{{0,9}},238},
    {{{80,7}},9}, {{{0,8}},95}, {{{0,8}},31}, {{{0,9}},158},
    {{{84,7}},99}, {{{0,8}},127}, {{{0,8}},63}, {{{0,9}},222},
    {{{82,7}},27}, {{{0,8}},111}, {{{0,8}},47}, {{{0,9}},190},
    {{{0,8}},15}, {{{0,8}},143}, {{{0,8}},79}, {{{0,9}},254},
    {{{96,7}},256}, {{{0,8}},80}, {{{0,8}},16}, {{{84,8}},115},
    {{{82,7}},31}, {{{0,8}},112}, {{{0,8}},48}, {{{0,9}},193},
    {{{80,7}},10}, {{{0,8}},96}, {{{0,8}},32}, {{{0,9}},161},
    {{{0,8}},0}, {{{0,8}},128}, {{{0,8}},64}, {{{0,9}},225},
    {{{80,7}},6}, {{{0,8}},88}, {{{0,8}},24}, {{{0,9}},145},
    {{{83,7}},59}, {{{0,8}},120}, {{{0,8}},56}, {{{0,9}},209},
    {{{81,7}},17}, {{{0,8}},104}, {{{0,8}},40}, {{{0,9}},177},
    {{{0,8}},8}, {{{0,8}},136}, {{{0,8}},72}, {{{0,9}},241},
    {{{80,7}},4}, {{{0,8}},84}, {{{0,8}},20}, {{{85,8}},227},
    {{{83,7}},43}, {{{0,8}},116}, {{{0,8}},52}, {{{0,9}},201},
    {{{81,7}},13}, {{{0,8}},100}, {{{0,8}},36}, {{{0,9}},169},
    {{{0,8}},4}, {{{0,8}},132}, {{{0,8}},68}, {{{0,9}},233},
    {{{80,7}},8}, {{{0,8}},92}, {{{0,8}},28}, {{{0,9}},153},
    {{{84,7}},83}, {{{0,8}},124}, {{{0,8}},60}, {{{0,9}},217},
    {{{82,7}},23}, {{{0,8}},108}, {{{0,8}},44}, {{{0,9}},185},
    {{{0,8}},12}, {{{0,8}},140}, {{{0,8}},76}, {{{0,9}},249},
    {{{80,7}},3}, {{{0,8}},82}, {{{0,8}},18}, {{{85,8}},163},
    {{{83,7}},35}, {{{0,8}},114}, {{{0,8}},50}, {{{0,9}},197},
    {{{81,7}},11}, {{{0,8}},98}, {{{0,8}},34}, {{{0,9}},165},
    {{{0,8}},2}, {{{0,8}},130}, {{{0,8}},66}, {{{0,9}},229},
    {{{80,7}},7}, {{{0,8}},90}, {{{0,8}},26}, {{{0,9}},149},
    {{{84,7}},67}, {{{0,8}},122}, {{{0,8}},58}, {{{0,9}},213},
    {{{82,7}},19}, {{{0,8}},106}, {{{0,8}},42}, {{{0,9}},181},
    {{{0,8}},10}, {{{0,8}},138}, {{{0,8}},74}, {{{0,9}},245},
    {{{80,7}},5}, {{{0,8}},86}, {{{0,8}},22}, {{{192,8}},0},
    {{{83,7}},51}, {{{0,8}},118}, {{{0,8}},54}, {{{0,9}},205},
    {{{81,7}},15}, {{{0,8}},102}, {{{0,8}},38}, {{{0,9}},173},
    {{{0,8}},6}, {{{0,8}},134}, {{{0,8}},70}, {{{0,9}},237},
    {{{80,7}},9}, {{{0,8}},94}, {{{0,8}},30}, {{{0,9}},157},
    {{{84,7}},99}, {{{0,8}},126}, {{{0,8}},62}, {{{0,9}},221},
    {{{82,7}},27}, {{{0,8}},110}, {{{0,8}},46}, {{{0,9}},189},
    {{{0,8}},14}, {{{0,8}},142}, {{{0,8}},78}, {{{0,9}},253},
    {{{96,7}},256}, {{{0,8}},81}, {{{0,8}},17}, {{{85,8}},131},
    {{{82,7}},31}, {{{0,8}},113}, {{{0,8}},49}, {{{0,9}},195},
    {{{80,7}},10}, {{{0,8}},97}, {{{0,8}},33}, {{{0,9}},163},
    {{{0,8}},1}, {{{0,8}},129}, {{{0,8}},65}, {{{0,9}},227},
    {{{80,7}},6}, {{{0,8}},89}, {{{0,8}},25}, {{{0,9}},147},
    {{{83,7}},59}, {{{0,8}},121}, {{{0,8}},57}, {{{0,9}},211},
    {{{81,7}},17}, {{{0,8}},105}, {{{0,8}},41}, {{{0,9}},179},
    {{{0,8}},9}, {{{0,8}},137}, {{{0,8}},73}, {{{0,9}},243},
    {{{80,7}},4}, {{{0,8}},85}, {{{0,8}},21}, {{{80,8}},258},
    {{{83,7}},43}, {{{0,8}},117}, {{{0,8}},53}, {{{0,9}},203},
    {{{81,7}},13}, {{{0,8}},101}, {{{0,8}},37}, {{{0,9}},171},
    {{{0,8}},5}, {{{0,8}},133}, {{{0,8}},69}, {{{0,9}},235},
    {{{80,7}},8}, {{{0,8}},93}, {{{0,8}},29}, {{{0,9}},155},
    {{{84,7}},83}, {{{0,8}},125}, {{{0,8}},61}, {{{0,9}},219},
    {{{82,7}},23}, {{{0,8}},109}, {{{0,8}},45}, {{{0,9}},187},
    {{{0,8}},13}, {{{0,8}},141}, {{{0,8}},77}, {{{0,9}},251},
    {{{80,7}},3}, {{{0,8}},83}, {{{0,8}},19}, {{{85,8}},195},
    {{{83,7}},35}, {{{0,8}},115}, {{{0,8}},51}, {{{0,9}},199},
    {{{81,7}},11}, {{{0,8}},99}, {{{0,8}},35}, {{{0,9}},167},
    {{{0,8}},3}, {{{0,8}},131}, {{{0,8}},67}, {{{0,9}},231},
    {{{80,7}},7}, {{{0,8}},91}, {{{0,8}},27}, {{{0,9}},151},
    {{{84,7}},67}, {{{0,8}},123}, {{{0,8}},59}, {{{0,9}},215},
    {{{82,7}},19}, {{{0,8}},107}, {{{0,8}},43}, {{{0,9}},183},
    {{{0,8}},11}, {{{0,8}},139}, {{{0,8}},75}, {{{0,9}},247},
    {{{80,7}},5}, {{{0,8}},87}, {{{0,8}},23}, {{{192,8}},0},
    {{{83,7}},51}, {{{0,8}},119}, {{{0,8}},55}, {{{0,9}},207},
    {{{81,7}},15}, {{{0,8}},103}, {{{0,8}},39}, {{{0,9}},175},
    {{{0,8}},7}, {{{0,8}},135}, {{{0,8}},71}, {{{0,9}},239},
    {{{80,7}},9}, {{{0,8}},95}, {{{0,8}},31}, {{{0,9}},159},
    {{{84,7}},99}, {{{0,8}},127}, {{{0,8}},63}, {{{0,9}},223},
    {{{82,7}},27}, {{{0,8}},111}, {{{0,8}},47}, {{{0,9}},191},
    {{{0,8}},15}, {{{0,8}},143}, {{{0,8}},79}, {{{0,9}},255}
};
inflate_huft fixed_td[] = {
    {{{80,5}},1}, {{{87,5}},257}, {{{83,5}},17}, {{{91,5}},4097},
    {{{81,5}},5}, {{{89,5}},1025}, {{{85,5}},65}, {{{93,5}},16385},
    {{{80,5}},3}, {{{88,5}},513}, {{{84,5}},33}, {{{92,5}},8193},
    {{{82,5}},9}, {{{90,5}},2049}, {{{86,5}},129}, {{{192,5}},24577},
    {{{80,5}},2}, {{{87,5}},385}, {{{83,5}},25}, {{{91,5}},6145},
    {{{81,5}},7}, {{{89,5}},1537}, {{{85,5}},97}, {{{93,5}},24577},
    {{{80,5}},4}, {{{88,5}},769}, {{{84,5}},49}, {{{92,5}},12289},
    {{{82,5}},13}, {{{90,5}},3073}, {{{86,5}},193}, {{{192,5}},24577}
};

/* inflate_trees_fixed: the fixed Huffman tables (inffixed.h). */
int inflate_trees_fixed(uIntf *bl, uIntf *bd, inflate_huft **tl, inflate_huft **td, z_streamp z)
{
    *bl = fixed_bl;
    *bd = fixed_bd;
    *tl = fixed_tl;
    *td = fixed_td;
    return Z_OK;
}

/* zlib inflate's inflate_mask (infutil.c): mask for n bits. */
uInt inflate_mask[17] = {
    0x0000, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};

/* inflate_flush: copies as much as possible from the sliding window to
 * the output area. */
int inflate_flush(inflate_blocks_statef *s, z_streamp z, int r)
{
    uInt n;
    Bytef *p;
    Bytef *q;

    /* local copies of source and destination pointers */
    p = z->next_out;
    q = s->read;

    /* compute number of bytes to copy as far as end of window */
    n = (uInt)((q <= s->write ? s->write : s->end) - q);
    if (n > z->avail_out) n = z->avail_out;
    if (n && r == Z_BUF_ERROR) r = Z_OK;

    /* update counters */
    z->avail_out -= n;
    z->total_out += n;

    /* update check information */
    if (s->checkfn != Z_NULL)
        z->adler = s->check = (*s->checkfn)(s->check, q, n);

    /* copy as far as end of window */
    zmemcpy(p, q, n);
    p += n;
    q += n;

    /* see if more to copy at beginning of window */
    if (q == s->end)
    {
        /* wrap pointers */
        q = s->window;
        if (s->write == s->end)
            s->write = s->window;

        /* compute bytes to copy */
        n = (uInt)(s->write - q);
        if (n > z->avail_out) n = z->avail_out;
        if (n && r == Z_BUF_ERROR) r = Z_OK;

        /* update counters */
        z->avail_out -= n;
        z->total_out += n;

        /* update check information */
        if (s->checkfn != Z_NULL)
            z->adler = s->check = (*s->checkfn)(s->check, q, n);

        /* copy */
        zmemcpy(p, q, n);
        p += n;
        q += n;
    }

    /* update pointers */
    z->next_out = p;
    s->read = q;

    /* done */
    return r;
}

/* ---- end of zlib ---- */

void Lobby_ReleaseNetTextures(void)
{
    NetMode_ReleaseTextures(0);
}

extern u16 D_008B2B40;
extern u16 D_008B2B42;
extern u16 D_008B2B40;
extern u16 D_008B2B42;

s32 LobbyMenu_MoveCursor(u8 *cur, u8 max, u8 mode)
{
    s32 kdec;
    s32 kinc;
    u16 *rep;

    if ((mode == 0) | (mode == 2)) {
        kdec = 0x2000;
        kinc = 0x1000;
        rep = &D_008B2B40;
    } else {
        kdec = 0x800;
        kinc = 0x400;
        rep = &D_008B2B42;
    }
    if (LobbyPad_Pressed(kdec)) {
        *rep = 20;
        goto dec;
    }
    if (LobbyPad_Pressed(kinc)) {
        *rep = 20;
        goto inc;
    }
    if (*rep) {
        *rep = *rep - 1;
        return 0;
    }
    if (LobbyPad_Held(kdec)) {
        *rep = 10;
        goto dec;
    }
    if (LobbyPad_Held(kinc)) {
        *rep = 10;
        goto inc;
    }
    return 0;
inc:
    if (mode < 2) {
        if (*cur == max) {
            *cur = 0;
        } else {
            (*cur)++;
        }
    } else {
        if (*cur == max) {
            return 0;
        }
        (*cur)++;
    }
    LobbySe_Play(1);
    return 1;
dec:
    if (mode < 2) {
        if (*cur == 0) {
            *cur = max;
        } else {
            (*cur)--;
        }
    } else {
        if (*cur == 0) {
            return 0;
        }
        (*cur)--;
    }
    LobbySe_Play(1);
    return 1;
}

s32 LobbyPad_Pressed(s32 button) {
    return (button & (gPadState[0].pressed | gPadState[0].repeat)) != 0;
}

s32 LobbyPad_Held(s32 mask) {
    return (gPadState[0].held & mask) != 0;
}

void Lobby_BackupWork(void) {
    memcpy(&gFriendList, &gNetSaveData, 0x960);
    memcpy(&gMailBlockList, &D_5BF608, 0xC0);
}

void Lobby_RestoreWork(void) {
    memcpy(&gNetSaveData, &gFriendList, 0x960);
    memcpy(&D_5BF608, &gMailBlockList, 0xC0);
}

void LobbyStream_Stop(void)
{
    Stream_Stop(0);
}

void LobbyStream_FadeOut(s32 arg0) {
    s32 frames;

    frames = arg0 & 0xFFFF;
    Stream_FadeOut(0, frames);
    Stream_FadeOut(1, frames);
}

void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);

void LobbySe_Play(s32 se)
{
    switch (se) {
    case 48:
        Sound_PlaySeFlat(1, 0xA4, 0);
        break;
    case 47:
        Sound_PlaySeFlat(1, 0xA3, 0);
        break;
    case 46:
        Sound_PlaySeFlat(1, 0xA2, 0);
        break;
    case 45:
        Sound_PlaySeFlat(1, 0xA1, 0);
        break;
    case 44:
        Sound_PlaySeFlat(1, 0xA0, 0);
        break;
    case 57:
        Sound_PlaySeFlat(0, 0x9, 0);
        break;
    case 27:
        Sound_PlaySeFlat(1, 0x91, 0);
        break;
    case 28:
        Sound_PlaySeFlat(1, 0x92, 0);
        break;
    case 0:
        Sound_PlaySeFlat(0, 0x13, 0);
        break;
    case 4:
        Sound_PlaySeFlat(0, 0x14, 0);
        break;
    case 5:
        Sound_PlaySeFlat(1, 0x93, 0);
        break;
    case 1:
        Sound_PlaySeFlat(0, 0x17, 0);
        break;
    case 2:
        Sound_PlaySeFlat(1, 0x90, 0);
        break;
    case 25:
        Sound_PlaySeFlat(6, 0x68, 0);
        break;
    case 26:
        Sound_PlaySeFlat(6, 0x6A, 0);
        break;
    case 7:
        Sound_PlaySeFlat(1, 0x80, 0);
        break;
    case 8:
        Sound_PlaySeFlat(0, 0x13, 0);
        break;
    case 10:
        Sound_PlaySeFlat(1, 0x83, 0);
        break;
    case 11:
        Sound_PlaySeFlat(0, 0x11, 0);
        break;
    case 12:
        Sound_PlaySeFlat(1, 0x87, 0);
        break;
    case 13:
        Sound_PlaySeFlat(6, 0x40, 0);
        break;
    case 14:
        Sound_PlaySeFlat(1, 0x85, 0);
        break;
    case 16:
        Sound_PlaySeFlat(0, 0x11, 0);
        break;
    case 15:
        Sound_PlaySeFlat(6, 0x47, 0);
        break;
    case 17:
        Sound_PlaySeFlat(1, 0x81, 0);
        break;
    case 18:
        Sound_PlaySeFlat(1, 0x89, 0);
        break;
    case 58:
        Sound_PlaySeFlat(1, 0x96, 0);
        break;
    case 59:
        Sound_PlaySeFlat(1, 0x97, 0);
        break;
    case 19:
        Sound_PlaySeFlat(1, 0x82, 0);
        break;
    case 20:
        Sound_PlaySeFlat(1, 0x8C, 0);
        break;
    case 21:
        Sound_PlaySeFlat(0, 0x85, 0);
        break;
    case 22:
        Sound_PlaySeFlat(1, 0x8D, 0);
        break;
    case 23:
        Sound_PlaySeFlat(1, 0x8E, 0);
        break;
    case 24:
        Sound_PlaySeFlat(1, 0x8F, 0);
        break;
    case 29:
        Sound_PlaySeFlat(6, 0xF0, 0);
        break;
    case 30:
        Sound_PlaySeFlat(6, 0xF1, 0);
        break;
    case 31:
        Sound_PlaySeFlat(6, 0xF2, 0);
        break;
    case 32:
        Sound_PlaySeFlat(6, 0xF3, 0);
        break;
    case 33:
        Sound_PlaySeFlat(6, 0xF4, 0);
        break;
    case 34:
        Sound_PlaySeFlat(6, 0xF5, 0);
        break;
    case 35:
        Sound_PlaySeFlat(6, 0xF6, 0);
        break;
    case 36:
        Sound_PlaySeFlat(6, 0xF7, 0);
        break;
    case 37:
        Sound_PlaySeFlat(6, 0xF8, 0);
        break;
    case 38:
        Sound_PlaySeFlat(6, 0xF9, 0);
        break;
    case 39:
        Sound_PlaySeFlat(6, 0xFA, 0);
        break;
    case 40:
        Sound_PlaySeFlat(6, 0xFB, 0);
        break;
    case 41:
        Sound_PlaySeFlat(6, 0x55, 0);
        break;
    case 42:
        Sound_PlaySeFlat(6, 0x56, 0);
        break;
    case 43:
        Sound_PlaySeFlat(6, 0x57, 0);
        break;
    case 49:
        Sound_PlaySeFlat(6, 0xD8, 0);
        break;
    case 50:
        Sound_PlaySeFlat(6, 0xD9, 0);
        break;
    case 51:
        Sound_PlaySeFlat(6, 0xDA, 0);
        break;
    case 52:
        Sound_PlaySeFlat(6, 0xDB, 0);
        break;
    case 53:
        Sound_PlaySeFlat(6, 0xDC, 0);
        break;
    case 54:
        Sound_PlaySeFlat(6, 0xDD, 0);
        break;
    case 55:
        Sound_PlaySeFlat(6, 0x6B, 0);
        break;
    case 56:
        Sound_PlaySeFlat(6, 0x6A, 0);
        break;
    case 6:
    case 9:
        Sound_PlaySeFlat(0, 0x15, 0);
        break;
    case 60:
        Sound_PlaySeFlat(1, 0x98, 0);
        break;
    case 3:
        Sound_PlaySeFlat(0, 0x10, 0);
        break;
    case 61:
        Sound_PlaySeFlat(0, 0x9B, 0);
        break;
    }
}

u8 SeqA_GetState(void);
s32 SeqA_IsPlaying(void) {
    s32 state;

    state = SeqA_GetState() & 0xFF;
    switch (state) {                              /* irregular */
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 0;
    default:
        return 0;
    }
}

void Font_SetPos(s16, s16);
void Text_Printf(const char *fmt, ...);

void Text_PrintAt(char *v, f32 x, f32 y)
{
    Font_SetPos(x, y);
    Text_Printf("%s", v);
}

void Text_SetColorIndex(s16 i) {
    Font_SetColorIndex(i);
}

s32 FriendList_FindFree(s8 *list, s32 max)
{
    s32 n;
    s32 ret;
    s8 *e;

    e = list;
    n = max & 0xFF;
    ret = 0;
    if (0 < n) {
loop_1:
        if (*e == 0) {
            return ret;
        }
        ret += 1;
        e += 0x30;
        if (ret >= n) {
            goto block_5;
        }
        goto loop_1;
    }
block_5:
    return -1;
}

s32 memcmp(void *, void *, s32);

s32 FriendList_FindId(s8 *list, s32 max, void *key)
{
    s32 ret;
    s8 *e;
    s32 n;

    n = max & 0xFF;
    ret = 0;
    if (0 < n) {
        e = list;
loop_2:
        if ((*e != 0) && (memcmp((void *) e, key, 8) == 0)) {
            return ret;
        }
        ret += 1;
        e += 0x30;
        if (ret >= n) {
            goto block_6;
        }
        goto loop_2;
    }
block_6:
    return -1;
}

typedef struct Rec30 {
    /* 0x00 */ u8 used;
    /* 0x01 */ u8 pad1[3];
    /* 0x04 */ f32 unk4;
    /* 0x08 */ u8 unk8;
    /* 0x09 */ u8 pad9[3];
    /* 0x0C */ f32 unkC[9];
} Rec30;

/* Removes record idx of the n in r (the later ones move down, the last is
 * cleared). */
s32 FriendList_DeleteAt(Rec30 *r, u8 idx, u8 n)
{
    u8 i;

    if (idx >= n) {
        return 0;
    }
    for (i = idx + 1; i <= n; i++) {
        if (i < n) {
            r[i - 1] = r[i];
            r[i].used = 0;
            r[i].unk8 = 0;
        } else {
            r[i - 1].used = 0;
            r[i - 1].unk8 = 0;
        }
    }
    return 1;
}

s32 IdList_Count(s8 *p, u8 n)
{
    s32 cnt = 0;
    s32 i;

    for (i = 0; i < n; i++, p += 0x18) {
        if (*p == 0) {
            break;
        }
        cnt++;
    }
    return cnt;
}

s32 IdList_FindFree(s8 *list, s32 max)
{
    s32 n;
    s32 ret;
    s8 *e;

    e = list;
    n = max & 0xFF;
    ret = 0;
    if (0 < n) {
loop_1:
        if (*e == 0) {
            return ret;
        }
        ret += 1;
        e += 0x18;
        if (ret >= n) {
            goto block_5;
        }
        goto loop_1;
    }
block_5:
    return -1;
}

s32 IdList_Find(s8 *list, s32 max, void *key)
{
    s32 ret;
    s8 *e;
    s32 n;

    n = max & 0xFF;
    ret = 0;
    if (0 < n) {
        e = list;
loop_2:
        if ((*e != 0) && (memcmp((void *) e, key, 8) == 0)) {
            return ret;
        }
        ret += 1;
        e += 0x18;
        if (ret >= n) {
            goto block_6;
        }
        goto loop_2;
    }
block_6:
    return -1;
}

typedef struct Rec18 {
    /* 0x00 */ s8 used;
    /* 0x01 */ s8 pad1[7];
    /* 0x08 */ s8 unk8;
    /* 0x09 */ s8 pad9[0xF];
} Rec18;

/* Removes record idx of the n in r (the later ones move down, the last is
 * cleared). */
s32 IdList_Remove(Rec18 *r, u8 idx, u8 n)
{
    u8 i;

    if (idx >= n) {
        return 0;
    }
    for (i = idx + 1; i <= n; i++) {
        if (i < n) {
            r[i - 1] = r[i];
            r[i].used = 0;
            r[i].unk8 = 0;
        } else {
            r[i - 1].used = 0;
            r[i - 1].unk8 = 0;
        }
    }
    return 1;
}

s32 Lobby_IsSysBusy(void) {
    return gSysBusy != 0;
}

void *memset(void *dst, s32 c, u32 n);
extern u8 gMenuWork[];
extern u8 gMenuInputLock;
void EquipShop_DrawTask(u8 *);
struct OfferTask;
s16 OfferMenu_BuildList(struct OfferTask *t, s32 b);

/* Opens the equipment shop (menu 8, task EquipShop_DrawTask) for the
 * townsperson npc: +0x8 the townsperson, +0xE4 set when it has offers. */
void EquipShop_Open(u8 *npc)
{
    u8 *w;

    gTownMenuOpen = 1;
    w = (u8 *)gMenuWork;
    gTownMode = 8;
    memset(w, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(EquipShop_DrawTask, 0, 0, 0);
    gMenuInputLock = 6;
    *(u8 **)(w + 8) = npc;
    if (OfferMenu_BuildList((struct OfferTask *) npc, 0) != 0) {
        w[0xE4] = 1;
    }
    w[0xE5] = 1;
    LobbySe_Play(0x1B);
}

void EquipShop_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    Chat_SetColor(-1);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    StageModels_Free();
    CamFocus_End();
    LobbySe_Play(4);
}