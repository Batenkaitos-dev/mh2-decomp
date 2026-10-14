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

s32 Cse_ReqSetMono(s32 on)
{
    CseReqB req = {0};

    req.cmd = CSE_REQ_MONO;
    req.a = on & 1;
    return MlRpcQueue_Set(CSE_FNO_COMM, &req, sizeof(req));
}

s32 Cse_ReqSetMasterVolume(s32 vol)
{
    return Cse_ReqSetVolume(0xFF, vol);
}

/* ch 0-15, or 0xFF for the master volume; vol 0-127. */
s32 Cse_ReqSetVolume(s32 ch, s32 vol)
{
    CseReqB req = {0};

    req.cmd = CSE_REQ_VOLUME;
    req.a = (ch == 0xFF) ? ch : (ch & 0xF);
    req.b = vol & 0x7F;
    return MlRpcQueue_Set(CSE_FNO_COMM, &req, sizeof(req));
}

/* Sets two modulations (a, b and c, d) of the matching voices. */
s32 Cse_ReqModulate(CseParam *p, u16 a, u16 b, u16 c, u16 d)
{
    CseReqModulate req = {0};

    req.cmd = CSE_REQ_MODULATE;
    req.param = *p;
    req.v[0] = a;
    req.v[1] = b;
    req.v[2] = c;
    req.v[3] = d;
    return MlRpcQueue_Set(CSE_FNO_COMM, &req, sizeof(req));
}

s32 Cse_ReqStop(CseParam *p)
{
    return Cse_ReqParam(p, CSE_REQ_STOP);
}

s32 Cse_ReqKeyOff(CseParam *p)
{
    return Cse_ReqParam(p, CSE_REQ_KEYOFF);
}

s32 Cse_ReqStopAll(void)
{
    CseParam p = {0};

    return Cse_ReqParam(&p, CSE_REQ_STOP);
}

s32 Cse_ReqStopBank(s32 bank)
{
    CseParam p = {0};

    p.flags = CSE_F_BANK;
    p.bank = bank & 0xF;
    return Cse_ReqParam(&p, CSE_REQ_STOP);
}

/* Changes the voices of a bank (-1: every bank) and sound id2 and id3 (-1:
 * any), with the values opt has. */
s32 Cse_ReqSetParam(s32 bank, s32 id2, s32 id3, CseOpt *opt)
{
    CseParam p = {0};

    if (bank == -1) {
        p.flags |= CSE_F_GLOBAL;
    } else {
        p.bank = bank;
        p.flags |= CSE_F_BANK;
    }
    if (id2 != -1) {
        p.id2 = id2;
        p.flags |= CSE_F_ID2;
    }
    if (id3 != -1) {
        p.id3 = id3;
        p.flags |= CSE_F_ID3;
    }
    if (opt->flags & 0x20) {
        p.flags |= 0x100;
        p.vol = opt->v[5];
    }
    if (opt->flags & 0x40) {
        p.flags |= 0x800;
        p.pan = opt->v[6];
    }
    if (opt->flags & 0x80) {
        p.flags |= 0x2000;
        p.unkE = opt->v[7];
    }
    if (opt->flags & 0x100) {
        p.flags |= 0x8000;
        p.bend = opt->v[8];
    }
    if (opt->flags & 0x1000) {
        p.flags |= 0x800;
        p.unk4 = opt->v[12];
    }
    if (opt->flags & 0x2000) {
        p.flags |= 0x10000;
        p.unk13 = opt->b38;
    }
    if (opt->flags & 0x4000) {
        p.flags |= 0x20000;
        p.unk14 = opt->b39;
    }
    return Cse_ReqParam(&p, CSE_REQ_SETPARAM);
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

s32 Cse_ReqKeyOn(CseVoice *v, CseParam *p)
{
    CseReqKeyOn req;

    req.cmd = CSE_REQ_KEYON;
    req.voice = *v;
    req.param = *p;
    return MlRpcQueue_Set(CSE_FNO_COMM, &req, sizeof(req));
}

/* Starts every split of program prog in bank p->bank that plays key
 * p->key. */
s32 Cse_KeyOnProgram(CseParam *p, s32 prog)
{
    CseTone tone = {0};
    CseVoice v;
    CseBankHead *head;
    s32 n;
    s32 i;
    s32 ret = 0;

    head = CseBank_Get(p->bank);
    n = CseBankFile_GetNumSplits(head, prog);
    for (i = 0; i < n; i++) {
        if (CseBankFile_GetTone(&tone, head, prog, p->key, i) >= 0) {
            CseBankFile_GetVoice(&v, &tone, p->key, CseBank_GetSpuAddr(p->bank));
            ret = Cse_ReqKeyOn(&v, p);
        }
    }
    return ret;
}

s32 Cse_CheckParam(CseParam *p)
{
    if (p->flags & CSE_F_PRIO) {
        p->prio &= 0x7F;
    } else {
        p->flags |= CSE_F_PRIO;
        p->prio = 0x7F;
    }
    if (p->flags & CSE_F_ID2) {
        if (!(p->flags & (CSE_F_GLOBAL | CSE_F_PHYSICAL | CSE_F_BANK))) {
            scePrintf("[EE]");
            scePrintf("(ERR)");
            scePrintf("You should set FLAG_GLOBAL, FLAG_PHYSICAL, or FLAG_BANK\n");
            return -1;
        }
    } else if (p->flags & CSE_F_ID3) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf("You should set ID2\n");
        return -1;
    }
    if (p->flags & CSE_F_KEY) {
        p->key &= 0x7F;
    }
    return 0;
}

/* Queues a CseParam request (key off, stop or set parameters). */
s32 Cse_ReqParam(CseParam *p, u32 cmd)
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
    req.param = *p;
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
