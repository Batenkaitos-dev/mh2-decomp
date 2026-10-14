/*
 * The game's sound manager, over the CSE's Snd_* calls (include/cse.h):
 * bank slots and their buffers, loading .snd files (SE table, bank header,
 * sample data upload), and sound effect requests with the random variation
 * each SE record allows.
 *
 * A .snd file begins with offsets and sizes of its three parts (SndFile).
 * Slot i has fixed buffers for its SE table and bank header (sizes in
 * gSndSeBufSizes and gSndBankBufSizes); the sample data goes to SPU RAM.
 * Several banks can also be staged (Sound_StageBank) and merged into one
 * slot (Sound_LoadStaged).
 */
#include "cse.h"
#include "syssettings.h"

typedef struct SndFile {
    u32 unk0[2];
    u32 headOffset;     /* 0x08: bank header (Head/Prog/Smpl/Vagi) */
    u32 headSize;       /* 0x0C */
    u32 bodyOffset;     /* 0x10: sample data */
    u32 bodySize;       /* 0x14 */
    u32 seOffset;       /* 0x18: SE table */
    u32 seSize;         /* 0x1C */
} SndFile;

/* Positional SEs (Sound_PlaySePos): the distance range of each kind of
 * source; the volume curves have one value per step. */
typedef struct SndRange {
    f32 near;           /* 0x00: full volume up to here */
    f32 step;           /* 0x04: distance per curve entry */
    f32 far;            /* 0x08: silent from here (kind 4: clamped to it) */
    f32 idOffset;       /* 0x0C: added to the SE id */
} SndRange;

typedef struct SndVec {
    f32 x, y, z;
} SndVec;

extern s32 gFlipCountdown;
extern u8 gSndSpuMapData[];     /* 0x24C180 */
extern u8 gSndBankData[];       /* 0x24C380 */
extern u8 gSndSeData[];         /* 0x25C380 */
extern u8 *gSndSpuMapBuf;
extern u8 *gSndSeBufs[16];
extern u32 gSndSeBufSizes[16];
extern u8 *gSndBankBufs[16];
extern u32 gSndBankBufSizes[16];
extern s32 gSndStageCount;
extern u8 *gSndStageHeads[4];
extern u32 gSndStageHeadUsed;
extern u32 gSndStageBodyUsed;
extern u8 *gSndStageHeadBase;
extern u8 *gSndStageBodyBase;
extern SndRange gSePosRanges[];

void *memcpy(void *dst, void *src, u32 n);
s32 Sound_ReadBank(u32 id, void *dst);   /* loads a file by id */
s32 Sys_FlipFrame(s32 unused0);             /* flips a frame */
s32 Sound_PlaySe(u32 bank, u32 id, CseOpt *opt);
u16 Rand_Next(s32 stream);            /* Rand_Next (sub_main) */
s32 Sound_LoadBankFile(s32 data, s32 slot);
s32 Sound_LoadMergedBanks(SndFile *f, u8 *out, u32 slot);
s32 Sound_PlaySePos(u32 slot, u32 id, s32 prog, void *pos, s32 kind, s32 update);

void Sound_Init(void)
{
    Snd_Init();
}

void Sound_InitBuffers(void)
{
    s32 i;
    u8 *se;
    u8 *bank;

    gSndSpuMapBuf = gSndSpuMapData;
    se = gSndSeData;
    bank = gSndBankData;
    for (i = 0; i < 16; i++) {
        gSndSeBufs[i] = se;
        gSndBankBufs[i] = bank;
        se += gSndSeBufSizes[i];
        bank += gSndBankBufSizes[i];
    }
}

/* Loads SpuMap.bin (file 0x1019D) and sets the master volume. */
void Sound_LoadSpuMap(void)
{
    Sound_ReadBank(0x1019D, gSndSpuMapBuf);
    Snd_InitSpuMap(gSndSpuMapBuf);
    Snd_SetMasterVolume(100);
}

s32 Sound_Update(void)
{
    return Snd_Update();
}

static u8 *Sound_GetSeBuf(u32 slot)
{
    if (slot >= 16) {
        return NULL;
    }
    return gSndSeBufs[slot];
}

static u32 Sound_GetSeBufSize(u32 slot)
{
    if (slot >= 16) {
        return 0;
    }
    return gSndSeBufSizes[slot];
}

static u8 *Sound_GetBankBuf(u32 slot)
{
    if (slot >= 16) {
        return NULL;
    }
    return gSndBankBufs[slot];
}

static u32 Sound_GetBankBufSize(u32 slot)
{
    if (slot >= 16) {
        return 0;
    }
    return gSndBankBufSizes[slot];
}

/* Copies an SE table (a bank header) into slot's buffer and hands it to the
 * engine. Inlined: their arguments are evaluated before the buffer calls. */
static inline void Sound_SetSe(u32 slot, u8 *src, u32 size)
{
    u8 *buf = Sound_GetSeBuf(slot);

    Sound_GetSeBufSize(slot);
    memcpy(buf, src, size);
    Snd_SetSeTable(slot, (CseSeTable *)buf);
}

static inline void Sound_SetBank(u32 slot, u8 *src, u32 size)
{
    u8 *buf = Sound_GetBankBuf(slot);

    Sound_GetBankBufSize(slot);
    memcpy(buf, src, size);
    Snd_SetBank(slot, buf);
}

/* Copies the SE table and bank header of f into slot's buffers and queues
 * the upload of its sample data. */
/* data is the address of the loaded file, as an integer. */
s32 Sound_LoadBankFile(s32 data, s32 slot)
{
    SndFile *f = (SndFile *)data;
    u8 *head;

    Sound_SetSe(slot, (u8 *)(data + f->seOffset), f->seSize);
    head = (u8 *)(data + f->headOffset);
    Sound_SetBank(slot, head, f->headSize);
    Snd_UploadBank(slot, (u32)(data + f->bodyOffset), Snd_GetBankBodySize(head), 0);
    return 0;
}

/* Like Sound_LoadBankFile with the staged banks merged into one header
 * (built in out) and the staged sample data. */
s32 Sound_LoadMergedBanks(SndFile *f, u8 *out, u32 slot)
{
    Sound_SetSe(slot, (u8 *)f + f->seOffset, f->seSize);
    Sound_SetBank(slot, out, Snd_MergeBanks(out, gSndStageHeads, gSndStageCount));
    Snd_UploadBank(slot, (u32)gSndStageBodyBase, Snd_GetBankBodySize(out), 0);
    return 0;
}

s32 Sound_LoadBank(SndFile *f, u32 slot)
{
    s32 ret;

    ret = Sound_LoadBankFile((s32)f, slot);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

s32 Sound_CheckTransfer(void);

/* Loads a bank and waits (flipping frames) until its upload is done. */
s32 Sound_LoadBankWait(SndFile *f, u32 slot)
{
    s32 ret;

    ret = Sound_LoadBankFile((s32)f, slot);
    if (ret < 0) {
        return ret;
    }
    while (Sound_CheckTransfer() < 0) {
        gFlipCountdown = 2;
        Sys_FlipFrame(0);
    }
    return 0;
}

s32 Sound_CheckTransfer(void)
{
    return Snd_CheckTransfer(0);
}

s32 Sound_LoadStaged(SndFile *f, u8 *out, u32 slot)
{
    s32 ret;

    if (gSndStageCount <= 0) {
        return 0;
    }
    ret = Sound_LoadMergedBanks(f, out, slot);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

s32 Sound_SetSpuMapPage(u32 page)
{
    return Snd_SetSpuMapPage(page);
}

s32 Sound_SetEffect(s32 core, s32 mode, s32 depth, s32 delay, s32 feedback)
{
    CseEffect e;

    e.mode = mode;
    e.depth = depth;
    e.delay = delay;
    e.feedback = feedback;
    return Snd_SetEffect(core, &e);
}

s32 Sound_StopAll(void)
{
    return Snd_StopAll();
}

s32 Sound_StopBank(s32 slot)
{
    return Snd_StopBank(slot);
}

void Sound_SetOutputMode(s32 mode)
{
    switch (mode) {
    case 0:
        Snd_SetOutputMode(0);
        break;
    case 1:
        Snd_SetOutputMode(1);
        break;
    case 2:
    default:
        Snd_SetOutputMode(2);
        break;
    }
}

/* One of an SE's four CSE_REQ_MODULATE values (0 if there is no such SE). */
s32 Sound_GetSeModValue(u32 slot, u32 id, s32 i)
{
    CseSeRec *rec = Snd_GetSeRecord(slot, id);

    if (rec == NULL) {
        return 0;
    }
    switch (i) {
    case 0:
        return rec->mod[0];
    case 1:
        return rec->mod[1];
    case 2:
        return rec->mod[2];
    case 3:
        return rec->mod[3];
    }
    return 0;
}

/* Sets an SE record's id2 and id3; ids 70-99 get id3 + 30. The id is offset
 * by a per-table float. */
void Sound_SetSeIds(u32 slot, u32 id, u8 id2, u8 id3, s32 table)
{
    CseSeRec *rec;

    if (id >= 70 && id < 100) {
        id3 += 30;
    }
    rec = Snd_GetSeRecord(slot, (u32)((f32)id + gSePosRanges[table].idOffset));
    if (rec != NULL) {
        rec->id2 = id2;
        rec->id3 = id3;
    }
}

/* Plays an SE with a volume (0-127, sent as a percentage), a pan and a
 * program offset. */
s32 Sound_PlaySeVolPan(u32 slot, u32 id, s32 vol, s32 pan, s32 unused, s32 prog)
{
    CseOpt opt = {0};

    opt.flags = 0x841;
    opt.v[6] = pan;
    opt.v[0] = prog;
    opt.v[11] = vol * 100 / 127;
    return Sound_PlaySe(slot, id, &opt);
}

/*
 * Plays an SE. If its first record says so, adds a random pitch bend in
 * [-mod[0], mod[0]) and a random volume change in [-mod[1], mod[1]),
 * drawn from random stream 0 (Rand_Next, sub_main).
 */
s32 Sound_PlaySe(u32 slot, u32 id, CseOpt *opt)
{
    CseSeRec *rec = Snd_GetSeRecord(slot, id);
    s32 r;

    if (rec == NULL) {
        return 0;
    }
    if (rec->mod[0] != 0) {
        opt->flags |= 0x100;
        r = rec->mod[0];
        opt->v[8] = Rand_Next(0) % (r * 2) - r;
    }
    if (rec->mod[1] != 0) {
        opt->flags |= 0x20;
        r = rec->mod[1];
        opt->v[5] += Rand_Next(0) % (r * 2) - r;
    }
    return Snd_RequestSe(slot, id, opt);
}

/* Positional SEs: the volume curves, the listener. */
extern s32 *gSePosVolCurves[];       /* volume curves */
extern s32 *gSePosCurves2[];       /* second curves (CseOpt.v[7]), or NULL */
extern SndVec gSndForwardAxis;       /* the listener's forward axis */
extern f32 gSndVolumeScale[];        /* the SE volume settings */
extern u8 gViewMatrix[];           /* sub_main: the listener's rotation */
extern SndVec gCameraPos;      /* sub_main 0x6A9DE0, the listener */
#define gSndOutputSetting (gSysSettings.soundMode)
#define gSndSeVolSetting (gSysSettings.seVolume)
/* sub_main's vector helpers (asm there, with f32 * parameters): declared
   here with the SndVec * this file passes */
void Vec_RotateSelf(f32 *v, f32 *m);
void Vec_Normalize(f32 *v);
f32 Vec_Dot(f32 *a, f32 *b);
void Vec_Cross(f32 *out, f32 *a, f32 *b);
f32 Math_Acos(f32 x);         /* acos */
f32 Math_Floor(f32 x);
f32 Math_Sqrtf(f32 x);
s32 Bgm_IsBattleState(void);

/*
 * Plays or updates an SE placed at pos: the volume from the kind's curve by
 * the distance to the listener (0x6A9DE0, rotated into its frame), the pan
 * and the front/back flags from the direction. update != 0 only moves the
 * pan of the playing SE. -1 when out of range or silent.
 */
s32 Sound_PlaySePos(u32 slot, u32 id, s32 prog, void *pos, s32 kind, s32 update)
{
    CseOpt opt = {0};
    SndVec d;
    SndVec side;
    SndVec fwd;
    f32 dist;
    f32 step;
    f32 far;
    f32 dot;
    f32 ang;
    f32 sy;
    f32 frac;
    s32 *curve;
    u8 flags;
    s32 *curve2;
    s32 pan;
    s32 vol;
    s32 vol2;
    s32 n;
    s32 c0;

    fwd = gSndForwardAxis;
    flags = 0;
    vol2 = 0;
    step = gSePosRanges[kind].step;
    far = gSePosRanges[kind].far;
    id = (u32)((f32)id + gSePosRanges[kind].idOffset);
    curve2 = gSePosCurves2[kind];
    d.x = gCameraPos.x - ((f32 *)pos)[0];
    d.y = gCameraPos.y - ((f32 *)pos)[1];
    d.z = gCameraPos.z - ((f32 *)pos)[2];
    Vec_RotateSelf((f32 *) &d, (f32 *) gViewMatrix);
    d.z -= 238.0f;
    dist = Math_Sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
    curve = gSePosVolCurves[kind];
    if (Bgm_IsBattleState() == 1) {
        curve += 17;
    }
    if (kind == 4) {
        if (dist >= far) {
            dist = far;
        }
    } else if (dist >= far) {
        return -1;
    }
    if (dist <= gSePosRanges[kind].near) {
        vol = curve[0];
        pan = 0;
        if (curve2 != NULL) {
            vol2 = curve2[0];
        }
    } else {
        Vec_Normalize((f32 *) &d);
        dot = Vec_Dot((f32 *) &fwd, (f32 *) &d);
        Vec_Cross((f32 *) &side, (f32 *) &fwd, (f32 *) &d);
        Vec_Normalize((f32 *) &side);
        ang = 360.0f * Math_Acos(dot) / 6.2831855f;
        ang = (ang < 90.0f) ? ang : 90.0f - (ang - 90.0f);
        sy = side.y;
        if (sy > 0.0f) {
            pan = -(64.0f * (ang / 90.0f));
        } else {
            pan = 64.0f * (ang / 90.0f);
        }
        flags |= (sy > 0.0f) ? 1 : 0x40;
        if (Sound_GetSeModValue(slot, id, 3) == 1) {
            flags |= 1;
        } else {
            switch (gSndOutputSetting) {
            case 0:
            case 1:
                flags |= 1;
                break;
            case 2:
            default:
                flags |= (dot > 0.0f) ? 1 : 0x80;
                break;
            }
        }
        n = Math_Floor(dist / step);
        c0 = curve[n];
        frac = (dist - step * n) / step;
        vol = c0 - (s32)(frac * (c0 - curve[n + 1]));
        if (curve2 != NULL) {
            vol2 = curve2[n] - (s32)(frac * (curve2[n] - curve2[n + 1]));
        }
    }
    vol *= gSndVolumeScale[gSndSeVolSetting];
    vol = vol * 100 / 127;
    if (vol == 0) {
        return -1;
    }
    if (update == 0) {
        opt.v[11] = vol;
        opt.v[12] = flags;
        opt.v[6] = pan;
        opt.v[0] = prog;
        opt.flags = 0x1841;
        if (curve2 != NULL) {
            opt.v[7] = vol2;
            opt.flags |= 0x80;
        }
        return Sound_PlaySe(slot, id, &opt);
    } else {
        CseSeRec *rec = Snd_GetSeRecord(slot, id);

        if (rec == NULL) {
            return -1;
        }
        opt.flags = 0x40;
        opt.v[6] = rec->pan + pan;
        Snd_SetParam(slot, rec->id2, rec->id3, &opt);
        return 0;
    }
}

s32 Sound_PlaySeAtPos(u32 slot, u32 id, s32 prog, void *pos, s32 kind, s32 update)
{
    return Sound_PlaySePos(slot, id, prog, pos, kind, update);
}

void Sound_InitStaging(u8 *headBase, u8 *bodyBase)
{
    gSndStageHeadBase = headBase;
    gSndStageBodyBase = bodyBase;
    gSndStageCount = 0;
    gSndStageHeadUsed = 0;
    gSndStageBodyUsed = 0;
}

/* Copies a .snd file's bank header and sample data to the staging areas.
 * The volatile read of headSize reads it through a0, before the parameter
 * is copied to s0, as the game does. */
void Sound_StageBank(SndFile *f)
{
    u8 *head = gSndStageHeadBase + gSndStageHeadUsed;

    memcpy(head, (u8 *)f + f->headOffset, ((volatile SndFile *)f)->headSize);
    gSndStageHeads[gSndStageCount] = head;
    memcpy(gSndStageBodyBase + gSndStageBodyUsed, (u8 *)f + f->bodyOffset, f->bodySize);
    gSndStageHeadUsed += f->headSize;
    gSndStageBodyUsed += f->bodySize;
    gSndStageCount++;
}

/* sound's .data (0x1BB9E0-0x1BCC80) */
u32 gSndSeBufSizes[16] = {
#include "main/001BB9E0.inc"
};
u32 gSndBankBufSizes[16] = {
#include "main/001BBA20.inc"
};
s32 gSePosVolCurves_0[36] = {
#include "main/001BBA60.inc"
};
s32 gSePosVolCurves_1[36] = {
#include "main/001BBAF0.inc"
};
s32 gSePosVolCurves_2[36] = {
#include "main/001BBB80.inc"
};
s32 gSePosVolCurves_3[36] = {
#include "main/001BBC10.inc"
};
s32 gSePosVolCurves_4[36] = {
#include "main/001BBCA0.inc"
};
s32 gSePosVolCurves_5[36] = {
#include "main/001BBD30.inc"
};
s32 gSePosVolCurves_6[36] = {
#include "main/001BBDC0.inc"
};
s32 gSePosVolCurves_7[36] = {
#include "main/001BBE50.inc"
};
s32 gSePosVolCurves_8[36] = {
#include "main/001BBEE0.inc"
};
s32 gSePosVolCurves_9[36] = {
#include "main/001BBF70.inc"
};
s32 gSePosVolCurves_10[36] = {
#include "main/001BC000.inc"
};
s32 gSePosVolCurves_11[36] = {
#include "main/001BC090.inc"
};
s32 gSePosVolCurves_12[36] = {
#include "main/001BC120.inc"
};
s32 gSePosVolCurves_13[36] = {
#include "main/001BC1B0.inc"
};
s32 gSePosVolCurves_14[36] = {
#include "main/001BC240.inc"
};
s32 gSePosVolCurves_15[36] = {
#include "main/001BC2D0.inc"
};
s32 gSePosVolCurves_16[36] = {
#include "main/001BC360.inc"
};
s32 gSePosVolCurves_17[36] = {
#include "main/001BC3F0.inc"
};
s32 gSePosVolCurves_18[36] = {
#include "main/001BC480.inc"
};
s32 gSePosVolCurves_19[36] = {
#include "main/001BC510.inc"
};
s32 gSePosVolCurves_20[36] = {
#include "main/001BC5A0.inc"
};
s32 gSePosVolCurves_21[36] = {
#include "main/001BC630.inc"
};
s32 gSePosVolCurves_22[36] = {
#include "main/001BC6C0.inc"
};
s32 gSePosVolCurves_23[36] = {
#include "main/001BC750.inc"
};
s32 gSePosVolCurves_24[36] = {
#include "main/001BC7E0.inc"
};
s32 gSePosVolCurves_25[36] = {
#include "main/001BC870.inc"
};
s32 gSePosVolCurves_26[36] = {
#include "main/001BC900.inc"
};
s32 *gSePosVolCurves[28] = {
    gSePosVolCurves_0, gSePosVolCurves_1, gSePosVolCurves_2, gSePosVolCurves_3,
    gSePosVolCurves_4, gSePosVolCurves_5, gSePosVolCurves_6, gSePosVolCurves_7,
    gSePosVolCurves_8, gSePosVolCurves_9, gSePosVolCurves_10, gSePosVolCurves_11,
    gSePosVolCurves_12, gSePosVolCurves_13, gSePosVolCurves_14, gSePosVolCurves_15,
    gSePosVolCurves_16, gSePosVolCurves_17, gSePosVolCurves_18, gSePosVolCurves_19,
    gSePosVolCurves_20, gSePosVolCurves_21, gSePosVolCurves_22, gSePosVolCurves_23,
    gSePosVolCurves_24, gSePosVolCurves_25, gSePosVolCurves_26, NULL,
};
SndRange gSePosRanges[27] = {
#include "main/001BCA00.inc"
};
s32 gSePosCurves2_24[20] = {
#include "main/001BCBB0.inc"
};
s32 *gSePosCurves2[28] = {
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    gSePosCurves2_24, NULL, NULL, NULL,
};
SndVec gSndForwardAxis = {
#include "main/001BCC70.inc"
};
