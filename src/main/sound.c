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
#include "bgm.h"
#include "sound.h"
#include "display.h"
#include "engine_math.h"
#include "rand.h"

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

/* A positional SE kind's volume curve (gSePosVolCurves): the volume at
 * each step (SndRange.step) of the distance, interpolated between steps;
 * Sound_PlaySePos reads battleVol while Bgm_IsBattleState is 1. */
typedef struct SndVolCurve {
    s32 vol[17];        /* 0x00 */
    s32 battleVol[17];  /* 0x44 */
} SndVolCurve;

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
s32 Sound_LoadMergedBanks(SndFile *f, u8 *out, u32 slot);

void Sound_Init(void)
{
    Snd_Init();
}

/*
 * Points each bank slot's SE table and bank header buffers into
 * gSndSeData and gSndBankData, packed one after another by the sizes in
 * gSndSeBufSizes and gSndBankBufSizes, and the SPU map buffer at
 * gSndSpuMapData. Called once at startup (Sound_Boot).
 */
void Sound_InitBuffers(void)
{
    s32 i;
    u8 *seBuf;
    u8 *bankBuf;

    gSndSpuMapBuf = gSndSpuMapData;
    seBuf = gSndSeData;
    bankBuf = gSndBankData;
    for (i = 0; i < 16; i++) {
        gSndSeBufs[i] = seBuf;
        gSndBankBufs[i] = bankBuf;
        seBuf += gSndSeBufSizes[i];
        bankBuf += gSndBankBufSizes[i];
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

/* The SE table buffer of bank slot slot; NULL when slot is 16 or more. */
static u8 *Sound_GetSeBuf(u32 slot)
{
    if (slot >= 16) {
        return NULL;
    }
    return gSndSeBufs[slot];
}

/* The size in bytes of slot's SE table buffer; 0 when slot is 16 or more. */
static u32 Sound_GetSeBufSize(u32 slot)
{
    if (slot >= 16) {
        return 0;
    }
    return gSndSeBufSizes[slot];
}

/* The bank header buffer of bank slot slot; NULL when slot is 16 or more. */
static u8 *Sound_GetBankBuf(u32 slot)
{
    if (slot >= 16) {
        return NULL;
    }
    return gSndBankBufs[slot];
}

/*
 * The size in bytes of slot's bank header buffer; 0 when slot is 16 or
 * more.
 */
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

/*
 * Copies a bank header into slot's buffer and hands it to the engine
 * (Snd_SetBank). Inlined like Sound_SetSe: its arguments are evaluated
 * before the buffer calls.
 */
static inline void Sound_SetBank(u32 slot, u8 *src, u32 size)
{
    u8 *buf = Sound_GetBankBuf(slot);

    Sound_GetBankBufSize(slot);
    memcpy(buf, src, size);
    Snd_SetBank(slot, buf);
}

/* Copies the SE table and bank header of f into slot's buffers and queues
 * the upload of its sample data. */
/*
 * Copies the SE table and bank header of a .snd file already in memory
 * into slot's buffers and queues the upload of its sample data to SPU RAM.
 * data is the file's address as an integer (file is the same address
 * read as a SndFile). Returns 0.
 */
s32 Sound_LoadBankFile(s32 data, s32 slot)
{
    SndFile *file = (SndFile *)data;
    u8 *head;

    Sound_SetSe(slot, (u8 *)(data + file->seOffset), file->seSize);
    head = (u8 *)(data + file->headOffset);
    Sound_SetBank(slot, head, file->headSize);
    Snd_UploadBank(slot, (u32)(data + file->bodyOffset), Snd_GetBankBodySize(head), 0);
    return 0;
}

/* Like Sound_LoadBankFile with the staged banks merged into one header
 * (built in out) and the staged sample data. */
s32 Sound_LoadMergedBanks(SndFile *file, u8 *out, u32 slot)
{
    Sound_SetSe(slot, (u8 *)file + file->seOffset, file->seSize);
    Sound_SetBank(slot, out, Snd_MergeBanks(out, gSndStageHeads, gSndStageCount));
    Snd_UploadBank(slot, (u32)gSndStageBodyBase, Snd_GetBankBodySize(out), 0);
    return 0;
}

/*
 * Loads a .snd file already in memory into bank slot slot
 * (Sound_LoadBankFile) without waiting for the sample upload. Returns 0,
 * or the negative result of Sound_LoadBankFile.
 */
s32 Sound_LoadBank(SndFile *file, u32 slot)
{
    s32 ret;

    ret = Sound_LoadBankFile((s32)file, slot);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

/* Loads a bank and waits (flipping frames) until its upload is done. */
s32 Sound_LoadBankWait(SndFile *file, u32 slot)
{
    s32 ret;

    ret = Sound_LoadBankFile((s32)file, slot);
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

/*
 * Loads the banks staged with Sound_StageBank into one slot: the SE table
 * comes from file, the merged bank header is built in out, and the sample
 * data is the staged data. Does nothing when nothing is staged. Returns 0,
 * or the negative result of Sound_LoadMergedBanks.
 */
s32 Sound_LoadStaged(SndFile *file, u8 *out, u32 slot)
{
    s32 ret;

    if (gSndStageCount <= 0) {
        return 0;
    }
    ret = Sound_LoadMergedBanks(file, out, slot);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

s32 Sound_SetSpuMapPage(u32 page)
{
    return Snd_SetSpuMapPage(page);
}

/*
 * Sets the reverb of SPU core core (mode, depth, delay, feedback) through
 * Snd_SetEffect and returns its result.
 */
s32 Sound_SetEffect(s32 core, s32 mode, s32 depth, s32 delay, s32 feedback)
{
    CseEffect effect;

    effect.mode = mode;
    effect.depth = depth;
    effect.delay = delay;
    effect.feedback = feedback;
    return Snd_SetEffect(core, &effect);
}

s32 Sound_StopAll(void)
{
    return Snd_StopAll();
}

s32 Sound_StopBank(s32 slot)
{
    return Snd_StopBank(slot);
}

/*
 * Sets the sound engine's output from the sound option: 0 mono, 1 stereo,
 * 2 (and any other value) surround, through Snd_SetOutputMode. Called by
 * Sound_ApplyOutputMode.
 */
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

/*
 * One of the four CSE_REQ_MODULATE values of SE id in slot:
 * mod[modIndex] of its first record. 0 when there is no such SE or
 * modIndex is not 0-3.
 */
s32 Sound_GetSeModValue(u32 slot, u32 id, s32 modIndex)
{
    CseSeRec *rec = Snd_GetSeRecord(slot, id);

    if (rec == NULL) {
        return 0;
    }
    switch (modIndex) {
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

/*
 * Tags the record of SE id in slot with ownerType and ownerSlot (its id2
 * and id3, by which Snd_SetParam later finds its voices); ids 70-99 get
 * ownerSlot + 30. The id is offset by gSePosRanges[table].idOffset first.
 * seen: callers pass an Em's type (plus 100 for SE_SLOT_EM) and slotNo.
 */
void Sound_SetSeIds(u32 slot, u32 id, u8 ownerType, u8 ownerSlot, s32 table)
{
    CseSeRec *rec;

    if (id >= 70 && id < 100) {
        ownerSlot += 30;
    }
    rec = Snd_GetSeRecord(slot, (u32)((f32)id + gSePosRanges[table].idOffset));
    if (rec != NULL) {
        rec->id2 = ownerType;
        rec->id3 = ownerSlot;
    }
}

/* Plays an SE with a volume (0-127, sent as a percentage), a pan and a
 * program offset. */
s32 Sound_PlaySeVolPan(u32 slot, u32 id, s32 vol, s32 pan, s32 unused, s32 prog)
{
    CseOpt opt = {0};

    opt.flags = 2113;
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
    s32 modRange;

    if (rec == NULL) {
        return 0;
    }
    if (rec->mod[0] != 0) {
        opt->flags |= 0x100;
        modRange = rec->mod[0];
        opt->v[8] = Rand_Next(0) % (modRange * 2) - modRange;
    }
    if (rec->mod[1] != 0) {
        opt->flags |= 0x20;
        modRange = rec->mod[1];
        opt->v[5] += Rand_Next(0) % (modRange * 2) - modRange;
    }
    return Snd_RequestSe(slot, id, opt);
}

/* Positional SEs: the volume curves, the listener. */
extern s32 *gSePosVolCurves[];       /* volume curves */
extern s32 *gSePosCurves2[];       /* second curves (CseOpt.v[7]), or NULL */
extern SndVec gSndForwardAxis;       /* the listener's forward axis */
extern u8 gViewMatrix[];           /* sub_main: the listener's rotation */
extern SndVec gCamPos;      /* sub_main's camera position, the listener */
#define gSndOutputSetting (gSysSettings.soundMode)
#define gSndSeVolSetting (gSysSettings.seVolume)
/* sub_main's vector helpers (asm there, with f32 * parameters): declared
   here with the SndVec * this file passes */

/*
 * Plays or updates an SE placed at pos: the volume from the kind's curve by
 * the distance to the listener (gCamPos, rotated into its frame), the pan
 * and the front/back flags from the direction. update != 0 only moves the
 * pan of the playing SE. -1 when out of range or silent.
 */
s32 Sound_PlaySePos(u32 slot, u32 id, s32 prog, void *pos, s32 kind, s32 update)
{
    CseOpt opt = {0};
    SndVec toListener;
    SndVec side;
    SndVec forward;
    f32 dist;
    f32 step;
    f32 far;
    f32 dot;
    f32 angle;
    f32 sideY;
    f32 frac;
    s32 *curve;
    u8 flags;
    s32 *curve2;
    s32 pan;
    s32 vol;
    s32 curve2Vol;
    s32 stepIndex;
    s32 stepVol;

    forward = gSndForwardAxis;
    flags = 0;
    curve2Vol = 0;
    step = gSePosRanges[kind].step;
    far = gSePosRanges[kind].far;
    id = (u32)((f32)id + gSePosRanges[kind].idOffset);
    curve2 = gSePosCurves2[kind];
    toListener.x = gCamPos.x - ((f32 *)pos)[0];
    toListener.y = gCamPos.y - ((f32 *)pos)[1];
    toListener.z = gCamPos.z - ((f32 *)pos)[2];
    Vec_RotateSelf((f32 *) &toListener, (f32 *) gViewMatrix);
    toListener.z -= 238.0f;
    dist = Math_Sqrtf(toListener.x * toListener.x + toListener.y * toListener.y + toListener.z * toListener.z);
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
            curve2Vol = curve2[0];
        }
    } else {
        Vec_Normalize((f32 *) &toListener);
        dot = Vec_Dot((f32 *) &forward, (f32 *) &toListener);
        Vec_Cross((f32 *) &side, (f32 *) &forward, (f32 *) &toListener);
        Vec_Normalize((f32 *) &side);
        angle = 360.0f * Math_Acos(dot) / 6.2831855f;
        angle = (angle < 90.0f) ? angle : 90.0f - (angle - 90.0f);
        sideY = side.y;
        if (sideY > 0.0f) {
            pan = -(64.0f * (angle / 90.0f));
        } else {
            pan = 64.0f * (angle / 90.0f);
        }
        flags |= (sideY > 0.0f) ? 1 : 0x40;
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
        stepIndex = Math_Floor(dist / step);
        stepVol = curve[stepIndex];
        frac = (dist - step * stepIndex) / step;
        vol = stepVol - (s32)(frac * (stepVol - curve[stepIndex + 1]));
        if (curve2 != NULL) {
            curve2Vol = curve2[stepIndex] - (s32)(frac * (curve2[stepIndex] - curve2[stepIndex + 1]));
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
        opt.flags = 6209;
        if (curve2 != NULL) {
            opt.v[7] = curve2Vol;
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

/*
 * Empties the bank staging used by Sound_StageBank: bank headers are
 * staged from headBase and sample data from bodyBase.
 */
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
void Sound_StageBank(SndFile *file)
{
    u8 *head = gSndStageHeadBase + gSndStageHeadUsed;

    memcpy(head, (u8 *)file + file->headOffset, ((volatile SndFile *)file)->headSize);
    gSndStageHeads[gSndStageCount] = head;
    memcpy(gSndStageBodyBase + gSndStageBodyUsed, (u8 *)file + file->bodyOffset, file->bodySize);
    gSndStageHeadUsed += file->headSize;
    gSndStageBodyUsed += file->bodySize;
    gSndStageCount++;
}

/* This file's initialised data. */
u32 gSndSeBufSizes[16] = {
#include "main/001BB9E0.inc"
};
u32 gSndBankBufSizes[16] = {
#include "main/001BBA20.inc"
};
SndVolCurve gSePosVolCurves_0 = {
#include "main/001BBA60.inc"
};
SndVolCurve gSePosVolCurves_1 = {
#include "main/001BBAF0.inc"
};
SndVolCurve gSePosVolCurves_2 = {
#include "main/001BBB80.inc"
};
SndVolCurve gSePosVolCurves_3 = {
#include "main/001BBC10.inc"
};
SndVolCurve gSePosVolCurves_4 = {
#include "main/001BBCA0.inc"
};
SndVolCurve gSePosVolCurves_5 = {
#include "main/001BBD30.inc"
};
SndVolCurve gSePosVolCurves_6 = {
#include "main/001BBDC0.inc"
};
SndVolCurve gSePosVolCurves_7 = {
#include "main/001BBE50.inc"
};
SndVolCurve gSePosVolCurves_8 = {
#include "main/001BBEE0.inc"
};
SndVolCurve gSePosVolCurves_9 = {
#include "main/001BBF70.inc"
};
SndVolCurve gSePosVolCurves_10 = {
#include "main/001BC000.inc"
};
SndVolCurve gSePosVolCurves_11 = {
#include "main/001BC090.inc"
};
SndVolCurve gSePosVolCurves_12 = {
#include "main/001BC120.inc"
};
SndVolCurve gSePosVolCurves_13 = {
#include "main/001BC1B0.inc"
};
SndVolCurve gSePosVolCurves_14 = {
#include "main/001BC240.inc"
};
SndVolCurve gSePosVolCurves_15 = {
#include "main/001BC2D0.inc"
};
SndVolCurve gSePosVolCurves_16 = {
#include "main/001BC360.inc"
};
SndVolCurve gSePosVolCurves_17 = {
#include "main/001BC3F0.inc"
};
SndVolCurve gSePosVolCurves_18 = {
#include "main/001BC480.inc"
};
SndVolCurve gSePosVolCurves_19 = {
#include "main/001BC510.inc"
};
SndVolCurve gSePosVolCurves_20 = {
#include "main/001BC5A0.inc"
};
SndVolCurve gSePosVolCurves_21 = {
#include "main/001BC630.inc"
};
SndVolCurve gSePosVolCurves_22 = {
#include "main/001BC6C0.inc"
};
SndVolCurve gSePosVolCurves_23 = {
#include "main/001BC750.inc"
};
SndVolCurve gSePosVolCurves_24 = {
#include "main/001BC7E0.inc"
};
SndVolCurve gSePosVolCurves_25 = {
#include "main/001BC870.inc"
};
SndVolCurve gSePosVolCurves_26 = {
#include "main/001BC900.inc"
};
s32 *gSePosVolCurves[28] = {
    gSePosVolCurves_0.vol, gSePosVolCurves_1.vol, gSePosVolCurves_2.vol, gSePosVolCurves_3.vol,
    gSePosVolCurves_4.vol, gSePosVolCurves_5.vol, gSePosVolCurves_6.vol, gSePosVolCurves_7.vol,
    gSePosVolCurves_8.vol, gSePosVolCurves_9.vol, gSePosVolCurves_10.vol, gSePosVolCurves_11.vol,
    gSePosVolCurves_12.vol, gSePosVolCurves_13.vol, gSePosVolCurves_14.vol, gSePosVolCurves_15.vol,
    gSePosVolCurves_16.vol, gSePosVolCurves_17.vol, gSePosVolCurves_18.vol, gSePosVolCurves_19.vol,
    gSePosVolCurves_20.vol, gSePosVolCurves_21.vol, gSePosVolCurves_22.vol, gSePosVolCurves_23.vol,
    gSePosVolCurves_24.vol, gSePosVolCurves_25.vol, gSePosVolCurves_26.vol, NULL,
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
