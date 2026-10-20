/*
 * CSE requests: each Cse_Req* function builds one request for the COMM
 * server and queues it (MlRpcQueue_Set), except Cse_ReqInit, which is sent
 * at once. Requests are cleared to zero first; the sequence number (word 1)
 * is assigned when queued.
 */
#include "cse.h"

typedef struct CseReqInit {
    u32 cmd;
    u32 seq;
    s32 arg;
} CseReqInit;

typedef struct CseReqB {
    u32 cmd;
    u32 seq;
    u8 a;
    u8 b;
} CseReqB;

typedef struct CseReqMixer {
    u32 cmd;
    u32 seq;
    s16 core;
    s16 mmix;
} CseReqMixer;

typedef struct CseReqModulate {
    u32 cmd;
    u32 seq;
    CseParam param;
    s16 v[4];
} CseReqModulate;

typedef struct CseReqParam {
    u32 cmd;
    u32 seq;
    CseParam param;
} CseReqParam;

typedef struct CseReqEffect {
    u32 cmd;
    u32 seq;
    s16 core;
    u16 mode;
    s16 depth;
    u8 delay;
    u8 feedback;
    u32 effectEnd;
} CseReqEffect;

typedef struct CseReqUpload {
    u32 cmd;
    u32 seq;
    u32 src;            /* EE address */
    u32 dst;            /* SPU address */
    u32 size;
} CseReqUpload;

typedef struct CseReqKeyOn {
    u32 cmd;
    u32 seq;
    CseParam param;
    CseVoice voice;
} CseReqKeyOn;

extern s32 gCseSeqInit;
extern s32 gCseSeq;

void scePrintf(char *fmt, ...);
s32 DIntr(void);
s32 EIntr(void);

/* Sent at once as CSE_FNO_INIT (the reply is not checked). */
s32 Cse_ReqInit(s32 arg)
{
    CseReqInit req = {0};

    req.arg = arg;
    CseRpc_Call(CSE_FNO_INIT, &req, sizeof(req));
    return 0;
}

/*
 * Queues CSE_REQ_MONO: enable 1 mixes every voice to mono, 0 turns that
 * off. Returns the request's sequence number. seen: Snd_SetOutputMode
 * passes 1 for the mono sound option.
 */
s32 Cse_ReqSetMono(s32 enable)
{
    CseReqB req = {0};

    req.cmd = CSE_REQ_MONO;
    req.a = enable & 1;
    return MlRpcQueue_Set(CSE_FNO_COMM, &req, sizeof(req));
}

s32 Cse_ReqSetMasterVolume(s32 vol)
{
    return Cse_ReqSetVolume(0xFF, vol);
}

/*
 * Queues CSE_REQ_VOLUME: channel 0-15, or 0xFF for the master volume; vol
 * 0-127. Returns the request's sequence number.
 */
s32 Cse_ReqSetVolume(s32 channel, s32 vol)
{
    CseReqB req = {0};

    req.cmd = CSE_REQ_VOLUME;
    req.a = (channel == 0xFF) ? channel : (channel & 0xF);
    req.b = vol & 0x7F;
    return MlRpcQueue_Set(CSE_FNO_COMM, &req, sizeof(req));
}

/*
 * Queues CSE_REQ_MODULATE for the voices voiceParam matches, with two
 * modulations (firstModA, firstModB and secondModA, secondModB). Returns
 * the request's sequence number. seen: the SE requests pass the mod[0..3]
 * of an SE record of type 5.
 */
s32 Cse_ReqModulate(CseParam *voiceParam, u16 firstModA, u16 firstModB, u16 secondModA, u16 secondModB)
{
    CseReqModulate req = {0};

    req.cmd = CSE_REQ_MODULATE;
    req.param = *voiceParam;
    req.v[0] = firstModA;
    req.v[1] = firstModB;
    req.v[2] = secondModA;
    req.v[3] = secondModB;
    return MlRpcQueue_Set(CSE_FNO_COMM, &req, sizeof(req));
}

s32 Cse_ReqStop(CseParam *voiceParam)
{
    return Cse_ReqParam(voiceParam, CSE_REQ_STOP);
}

s32 Cse_ReqKeyOff(CseParam *voiceParam)
{
    return Cse_ReqParam(voiceParam, CSE_REQ_KEYOFF);
}

/*
 * Queues a stop of every voice (an empty CseParam). Returns the request's
 * sequence number.
 */
s32 Cse_ReqStopAll(void)
{
    CseParam voiceParam = {0};

    return Cse_ReqParam(&voiceParam, CSE_REQ_STOP);
}

/*
 * Queues a stop of every voice of bank (0-15). Returns the request's
 * sequence number.
 */
s32 Cse_ReqStopBank(s32 bank)
{
    CseParam voiceParam = {0};

    voiceParam.flags = CSE_F_BANK;
    voiceParam.bank = bank & 0xF;
    return Cse_ReqParam(&voiceParam, CSE_REQ_STOP);
}

/*
 * Changes the voices of a bank (-1: every bank) whose id2 is groupId and
 * id3 is subGroupId (-1: any), with the values opt has (volume, pan,
 * bend and the others its flags mark). Returns the request's sequence
 * number, 0 when it is refused.
 */
s32 Cse_ReqSetParam(s32 bank, s32 groupId, s32 subGroupId, CseOpt *opt)
{
    CseParam voiceParam = {0};

    if (bank == -1) {
        voiceParam.flags |= CSE_F_GLOBAL;
    } else {
        voiceParam.bank = bank;
        voiceParam.flags |= CSE_F_BANK;
    }
    if (groupId != -1) {
        voiceParam.id2 = groupId;
        voiceParam.flags |= CSE_F_ID2;
    }
    if (subGroupId != -1) {
        voiceParam.id3 = subGroupId;
        voiceParam.flags |= CSE_F_ID3;
    }
    if (opt->flags & 0x20) {
        voiceParam.flags |= 0x100;
        voiceParam.vol = opt->v[5];
    }
    if (opt->flags & 0x40) {
        voiceParam.flags |= 0x800;
        voiceParam.pan = opt->v[6];
    }
    if (opt->flags & 0x80) {
        voiceParam.flags |= 0x2000;
        voiceParam.unkE = opt->v[7];
    }
    if (opt->flags & 0x100) {
        voiceParam.flags |= 0x8000;
        voiceParam.bend = opt->v[8];
    }
    if (opt->flags & 0x1000) {
        voiceParam.flags |= 0x800;
        voiceParam.unk4 = opt->v[12];
    }
    if (opt->flags & 0x2000) {
        voiceParam.flags |= 0x10000;
        voiceParam.unk13 = opt->b38;
    }
    if (opt->flags & 0x4000) {
        voiceParam.flags |= 0x20000;
        voiceParam.unk14 = opt->b39;
    }
    return Cse_ReqParam(&voiceParam, CSE_REQ_SETPARAM);
}

/* The core's effect (reverb): sceSdSetEffectAttr values, and the end of the
 * effect area from the SPU map. */
s32 Cse_ReqSetEffect(s32 core, s32 mode, s32 depth, u8 delay, u8 feedback)
{
    CseReqEffect req = {0};

    req.cmd = CSE_REQ_EFFECT;
    req.core = core;
    req.mode = mode;
    req.depth = depth;
    req.delay = delay;
    req.feedback = feedback;
    req.effectEnd = CseBank_GetEffectEnd(core);
    return MlRpcQueue_Set(CSE_FNO_COMM, &req, sizeof(req));
}

/* Copies size bytes from EE address src to SPU address dst. */
s32 Cse_ReqUpload(u32 src, u32 dst, u32 size)
{
    CseReqUpload req = {0};

    req.src = src;
    req.dst = dst;
    req.size = size;
    req.cmd = CSE_REQ_UPLOAD;
    return MlRpcQueue_Set(CSE_FNO_UPLOAD, &req, sizeof(req));
}

/*
 * Queues CSE_REQ_KEYON: starts one voice with the attributes voiceAttr
 * (from a tone of the bank) and the targeting and values of voiceParam.
 * Returns the request's sequence number.
 */
s32 Cse_ReqKeyOn(CseVoice *voiceAttr, CseParam *voiceParam)
{
    CseReqKeyOn req;

    req.cmd = CSE_REQ_KEYON;
    req.voice = *voiceAttr;
    req.param = *voiceParam;
    return MlRpcQueue_Set(CSE_FNO_COMM, &req, sizeof(req));
}

/*
 * Starts every split of program prog in bank voiceParam->bank that plays
 * key voiceParam->key. Returns the last Cse_ReqKeyOn result, 0 when no
 * split plays the key.
 */
s32 Cse_KeyOnProgram(CseParam *voiceParam, s32 prog)
{
    CseTone tone = {0};
    CseVoice voiceAttr;
    CseBankHead *head;
    s32 splitCount;
    s32 i;
    s32 ret = 0;

    head = CseBank_Get(voiceParam->bank);
    splitCount = CseBankFile_GetNumSplits(head, prog);
    for (i = 0; i < splitCount; i++) {
        if (CseBankFile_GetTone(&tone, head, prog, voiceParam->key, i) >= 0) {
            CseBankFile_GetVoice(&voiceAttr, &tone, voiceParam->key, CseBank_GetSpuAddr(voiceParam->bank));
            ret = Cse_ReqKeyOn(&voiceAttr, voiceParam);
        }
    }
    return ret;
}

/*
 * Checks and fixes a CseParam before it is sent: prio is masked to 0-0x7F
 * (or set to 0x7F with CSE_F_PRIO when unset) and key to 0-0x7F. Returns
 * -1, printing the engine's message, when CSE_F_ID2 is set without
 * CSE_F_GLOBAL, CSE_F_PHYSICAL or CSE_F_BANK, or CSE_F_ID3 without
 * CSE_F_ID2; else 0.
 */
s32 Cse_CheckParam(CseParam *voiceParam)
{
    if (voiceParam->flags & CSE_F_PRIO) {
        voiceParam->prio &= 0x7F;
    } else {
        voiceParam->flags |= CSE_F_PRIO;
        voiceParam->prio = 0x7F;
    }
    if (voiceParam->flags & CSE_F_ID2) {
        if (!(voiceParam->flags & (CSE_F_GLOBAL | CSE_F_PHYSICAL | CSE_F_BANK))) {
            scePrintf("[EE]");
            scePrintf("(ERR)");
            scePrintf("You should set FLAG_GLOBAL, FLAG_PHYSICAL, or FLAG_BANK\n");
            return -1;
        }
    } else if (voiceParam->flags & CSE_F_ID3) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf("You should set ID2\n");
        return -1;
    }
    if (voiceParam->flags & CSE_F_KEY) {
        voiceParam->key &= 0x7F;
    }
    return 0;
}

/*
 * Queues a CseParam request (key off, stop or set parameters). Returns the
 * request's sequence number, 0 for another cmd or when Cse_CheckParam
 * rejects voiceParam.
 */
s32 Cse_ReqParam(CseParam *voiceParam, u32 cmd)
{
    CseReqParam req = {0};
    u32 seq;

    switch (cmd) {
    case CSE_REQ_SETPARAM:
    case CSE_REQ_STOP:
    case CSE_REQ_KEYOFF:
        break;
    default:
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf(" : Match UNKNOWN Command!?\n");
        return 0;
    }
    req.cmd = cmd;
    req.param = *voiceParam;
    if (Cse_CheckParam(&req.param) == -1) {
        return 0;
    }
    seq = MlRpcQueue_Set(CSE_FNO_COMM, &req, sizeof(req));
    return seq;
}

/* The core's mixer setting (libsd SD_P_MMIX). */
s32 Cse_ReqSetMixer(s16 core, s16 mmix)
{
    CseReqMixer req = {0};

    req.cmd = CSE_REQ_MIXER;
    req.core = core;
    req.mmix = mmix;
    return MlRpcQueue_Set(CSE_FNO_COMM, &req, sizeof(req));
}

/*
 * Sets the request sequence counter gCseSeq to 0 the first time it is
 * called (gCseSeqInit), with interrupts off. Returns 0. Called by
 * Cse_Init.
 */
s32 Cse_InitSeq(void)
{
    if (gCseSeqInit == 0) {
        DIntr();
        gCseSeq = 0;
        EIntr();
        gCseSeqInit = 1;
    }
    return 0;
}

/* The next request sequence number; 0 is skipped. */
u32 Cse_NextSeq(void)
{
    u32 seq;

    DIntr();
    seq = gCseSeq++;
    if (seq == 0) {
        seq = gCseSeq++;
    }
    EIntr();
    return seq;
}

/* CRI ADX error callback (ADXT_EntryErrFunc): ignores the object and the message. */
void Adx_ErrorCallback(void *obj, char *msg)
{
}
