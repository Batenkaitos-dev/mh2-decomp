/*
 * CSE engine API and sound effects (SE).
 *
 * Cse_* are the engine calls; Snd_* are the thin layer
 * the game's sound code uses, and the only functions called from outside the
 * library. An SE table belongs to a bank: each SE id is a chain of 32-byte
 * records (CseSeRec), and CseSe_Request turns each record into one request.
 */
#include "cse.h"

typedef struct CseFrame {
    s32 count;          /* frames since Cse_Init */
    s32 unk4[16];       /* set to -1 by Cse_Init */
} CseFrame;

extern char D_001C81E8[];   /* "[EE]" */
extern char D_001C81F0[];   /* "(ERR)" */
extern char D_001C8200[];   /* "[C.S.E] Unknown parameter in cseCheckVtransStatus().\n" */
extern u8 gSndOutputMode;
extern CseFrame gCseFrame;
extern void (*gCseSeKeyOnHook)(CseParam *p, CseSeRec *rec);
extern CseSeTable *gCseSeTables[16];

void scePrintf(char *fmt, ...);
s32 func_001145F0(void);    /* nonzero with interrupts disabled */
s32 CseSe_SetTable(u32 bank, CseSeTable *table);
CseSeRec *CseSe_GetRecord(u32 bank, u32 id);
s32 CseSe_Request(u16 bank, u16 id, CseSeOpt *opt);

/*
 * Starts the CSE engine: CseRpc_Init, MlRpcQueue_Init and Cse_InitSeq,
 * then sends arg to the IOP as the CSE_FNO_INIT request (Cse_ReqInit),
 * empties the SE tables (CseSe_Init) and resets gCseFrame (count 0, the
 * 16 unk4 entries -1). Returns 0.
 */
s32 Cse_Init(s32 arg)
{
    u32 i;

    CseRpc_Init();
    MlRpcQueue_Init();
    Cse_InitSeq();
    Cse_ReqInit(arg);
    CseSe_Init();
    gCseFrame.count = 0;
    for (i = 0; i < 16; i++) {
        gCseFrame.unk4[i] = -1;
    }
    return 0;
}

/* Once a frame: sends the queued requests. */
s32 Cse_Update(void)
{
    CseSe_Update();
    if (func_001145F0() == 0) {
        mlRpcQueueSend();
    } else {
        mlRpcQueueSendIntr();
    }
    gCseFrame.count++;
    return 0;
}

/*
 * Transfer status: bit 0 is set while bank uploads wait in the queue, bits
 * 4, 8 and 12 while the IOP reports transfers busy (STAT reply bytes 10, 9,
 * 8). Mode 0 tests all of them, mode 1 only bits 0, 4 and 8. Returns 0 when
 * idle, else the bits | 0x80000000.
 */
u32 cseCheckVtransStatus(s32 mode)
{
    u32 req[2];
    u8 *reply;
    u32 statusBits;
    u32 ret;
    s32 numUploads;

    req[0] = CSE_REQ_STATUS;
    reply = CseRpc_Call(CSE_FNO_STAT, req, sizeof(req));
    numUploads = MlRpcQueue_NumUploads();
    statusBits = 0;
    if (numUploads != 0) {
        statusBits |= 1;
    }
    if (reply[10] != 0) {
        statusBits |= 0x10;
    }
    if (reply[9] != 0) {
        statusBits |= 0x100;
    }
    if (reply[8] != 0) {
        statusBits |= 0x1000;
    }
    switch (mode) {
    case 0:
        if (statusBits == 0) {
            ret = 0;
        } else {
            ret = statusBits | 0x80000000;
        }
        break;
    case 1:
        if ((statusBits & 0x111) == 0) {
            ret = 0;
        } else {
            ret = statusBits | 0x80000000;
        }
        break;
    default:
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf("[C.S.E] Unknown parameter in cseCheckVtransStatus().\n");
        break;
    }
    return ret;
}

s32 Cse_InitSpuMap(void *map)
{
    return CseBank_Init(map);
}

s32 Cse_SetSpuMapPage(u32 page)
{
    return CseBank_SetPage(page);
}

s32 Cse_SetBank(u32 bank, void *head)
{
    return CseBank_Set(bank, head);
}

s32 Cse_SetSeTable(u32 bank, CseSeTable *table)
{
    return CseSe_SetTable(bank, table);
}

CseSeRec *Cse_GetSeRecord(u32 bank, u32 id)
{
    return CseSe_GetRecord(bank, id);
}

/* Uploads size bytes from src to the bank's SPU area, at offset. */
s32 Cse_UploadBank(u32 src, u32 size, u32 bank, u32 offset)
{
    return Cse_ReqUpload(src, offset + CseBank_GetSpuAddr(bank), size);
}

s32 Cse_GetBankBodySize(void *head)
{
    return CseBankFile_GetBodySize(head);
}

s32 Cse_MergeBanks(void *out, void *banks, s32 count)
{
    return CseBank_Merge(out, banks, count);
}

/*
 * Plays SE id of bank bank: copies each value of opt whose bit is set in
 * opt->flags (opt may be NULL) into a CseSeOpt that starts with the
 * defaults v[11] (volume percentage) 100 and v[13], v[14] 0x7F, and
 * passes it to CseSe_Request. Returns CseSe_Request's result.
 */
s32 Cse_RequestSe(u16 bank, u16 id, CseOpt *opt)
{
    CseSeOpt seOpt = {0};
    u32 flags;
    u32 bit;

    seOpt.v[11] = 100;
    seOpt.v[13] = 0x7F;
    seOpt.v[14] = 0x7F;
    if (opt != NULL && (flags = opt->flags) != 0) {
        bit = 1;
        do {
            switch (flags & bit) {
            case 1:
                seOpt.v[0] = opt->v[0];
                break;
            case 2:
                seOpt.v[1] = opt->v[1];
                break;
            case 4:
                seOpt.v[2] = opt->v[2];
                break;
            case 8:
                seOpt.v[3] = opt->v[3];
                break;
            case 0x10:
                seOpt.v[4] = opt->v[4];
                break;
            case 0x20:
                seOpt.v[5] = opt->v[5];
                break;
            case 0x40:
                seOpt.v[6] = opt->v[6];
                break;
            case 0x80:
                seOpt.v[7] = opt->v[7];
                break;
            case 0x100:
                seOpt.v[8] = opt->v[8];
                break;
            case 0x200:
                seOpt.v[9] = opt->v[9];
                break;
            case 0x400:
                seOpt.v[10] = opt->v[10];
                break;
            case 0x800:
                seOpt.v[11] = opt->v[11];
                break;
            case 0x1000:
                seOpt.v[12] = opt->v[12];
                break;
            case 0x2000:
                seOpt.v[13] = opt->b38;
                break;
            case 0x4000:
                seOpt.v[14] = opt->b39;
                break;
            }
            flags &= ~bit;
            bit <<= 1;
        } while (flags != 0);
    }
    return CseSe_Request(bank, id, &seOpt);
}

s32 Cse_SetParam(s32 bank, s32 id2, s32 id3, CseOpt *opt)
{
    return Cse_ReqSetParam(bank, id2, id3, opt);
}

/*
 * Starts the sound engine for the game (Sound_Init): Cse_Init with 0,
 * stereo output (mono off), and the mixer settings 0xFC0 for core 0 and
 * 0xFCF for core 1.
 */
void Snd_Init(void)
{
    Cse_Init(0);
    Cse_ReqSetMono(0);
    Cse_ReqSetMixer(0, 0xFC0);
    Cse_ReqSetMixer(1, 0xFCF);
}

s32 Snd_Update(void)
{
    return Cse_Update();
}

/* 0: mono; 1 and 2: stereo (the value is kept in gSndOutputMode). */
void Snd_SetOutputMode(s32 mode)
{
    switch ((u8)mode) {
    case 0:
        gSndOutputMode = 0;
        Cse_ReqSetMono(1);
        break;
    case 1:
        gSndOutputMode = 1;
        Cse_ReqSetMono(0);
        break;
    case 2:
        gSndOutputMode = 2;
        Cse_ReqSetMono(0);
        break;
    }
}

s32 Snd_SetMasterVolume(s32 vol)
{
    return Cse_ReqSetMasterVolume(vol);
}

s32 Snd_InitSpuMap(void *map)
{
    return Cse_InitSpuMap(map);
}

s32 Snd_SetSpuMapPage(u32 page)
{
    return Cse_SetSpuMapPage(page);
}

s32 Snd_SetBank(u32 bank, void *head)
{
    return Cse_SetBank(bank, head);
}

s32 Snd_SetSeTable(u32 bank, CseSeTable *table)
{
    return Cse_SetSeTable(bank, table);
}

CseSeRec *Snd_GetSeRecord(u32 bank, u32 id)
{
    return Cse_GetSeRecord(bank, id);
}

s32 Snd_UploadBank(u32 bank, u32 src, u32 size, u32 offset)
{
    return Cse_UploadBank(src, size, bank, offset);
}

u32 Snd_CheckTransfer(s32 mode)
{
    return cseCheckVtransStatus(mode);
}

s32 Snd_GetBankBodySize(void *head)
{
    return Cse_GetBankBodySize(head);
}

s32 Snd_MergeBanks(void *out, void *banks, s32 count)
{
    return Cse_MergeBanks(out, banks, count);
}

s32 Snd_SetEffect(s32 core, CseEffect *effect)
{
    return Cse_ReqSetEffect(core, effect->mode, effect->depth, effect->delay, effect->feedback);
}

s32 Snd_StopAll(void)
{
    return Cse_ReqStopAll();
}

s32 Snd_StopBank(s32 bank)
{
    return Cse_ReqStopBank(bank);
}

s32 Snd_SetParam(s32 bank, s32 id2, s32 id3, CseOpt *opt)
{
    return Cse_SetParam(bank, id2, id3, opt);
}

s32 Snd_RequestSe(u32 bank, u32 id, CseOpt *opt)
{
    return Cse_RequestSe(bank, id, opt);
}

/*
 * Empties the 16 banks' SE tables (gCseSeTables) and clears the key-on
 * hook gCseSeKeyOnHook. Returns 0.
 */
s32 CseSe_Init(void)
{
    u32 i;

    for (i = 0; i < 16; i++) {
        gCseSeTables[i] = NULL;
    }
    gCseSeKeyOnHook = NULL;
    return 0;
}

s32 CseSe_Update(void)
{
    return 0;
}

s32 CseSe_SetTable(u32 bank, CseSeTable *table)
{
    gCseSeTables[bank] = table;
    return 0;
}

/*
 * The record of SE id in bank bank's SE table; NULL when the bank has no
 * table or id is past its count.
 */
CseSeRec *CseSe_GetRecord(u32 bank, u32 id)
{
    CseSeTable *table = gCseSeTables[bank];

    if (table == NULL) {
        return NULL;
    }
    if (id >= table->count) {
        return NULL;
    }
    return &table->recs[id];
}

/*
 * A key-on record: sets param->flags from the record's flags (with
 * CSE_F_BANK cleared for a CSE_F_GLOBAL record, set otherwise), calls
 * gCseSeKeyOnHook when one is set, and starts the program's tones for
 * param->key (Cse_KeyOnProgram). The caller passes prog unmasked (an
 * int); it is a u16 from here on. Returns Cse_KeyOnProgram's result.
 */
s32 CseSe_KeyOn(CseSeRec *rec, CseParam *param, s32 prog)
{
    if (rec->flags & CSE_F_GLOBAL) {
        param->flags = rec->flags & ~CSE_F_BANK;
    } else {
        param->flags = rec->flags | CSE_F_BANK;
    }
    if (gCseSeKeyOnHook != NULL) {
        gCseSeKeyOnHook(param, rec);
    }
    return Cse_KeyOnProgram(param, (u16)prog);
}

/*
 * Plays SE id of a bank: one request per record, following the chain of
 * records (next) until 0xFFFF or a record pointing back at itself. The
 * record's values are offset by opt's, the volume scaled by opt->v[11]
 * percent, then volume, pan and bend clamped. Returns -1 when a record
 * is missing, else the last record's request result (0 when none ran).
 */
s32 CseSe_Request(u16 bank, u16 id, CseSeOpt *opt)
{
    CseSeRec *rec;
    CseParam param = {0};
    s32 ret = 0;

    for (;;) {
        rec = CseSe_GetRecord(bank, id);
        if (rec == NULL) {
            return -1;
        }
        param.flags = rec->flags;
        param.bank = bank;
        param.key = rec->key;
        param.vol = rec->vol;
        param.pan = rec->pan;
        param.unkE = rec->unkE;
        param.prio = rec->prio;
        param.id2 = rec->id2;
        param.id3 = rec->id3;
        param.unk18 = rec->unk10;
        param.unk4 = rec->unk8;
        param.unk12 = rec->unk14;
        param.key += opt->v[1];
        param.id2 += opt->v[2];
        param.id3 += opt->v[3];
        param.prio += opt->v[4];
        param.vol += opt->v[5];
        param.pan += opt->v[6];
        param.unkE += opt->v[7];
        param.bend += opt->v[8];
        param.unk18 += opt->v[9];
        param.unk12 += opt->v[10];
        param.vol = param.vol * opt->v[11] / 100;
        param.unk4 |= opt->v[12];
        param.unk13 = opt->v[13];
        param.unk14 = opt->v[14];
        if (param.vol < 0) {
            param.vol = 0;
        }
        if (param.vol > 0x7F) {
            param.vol = 0x7F;
        }
        if (param.pan < 0) {
            param.pan = 0;
        }
        if (param.pan > 0x7F) {
            param.pan = 0x7F;
        }
        if (param.bend < -0x2000) {
            param.bend = -0x2000;
        }
        if (param.bend > 0x2063) {
            param.bend = 0x1FFF;
        }
        switch (rec->type) {
        case 0:
            break;
        case 1:
            ret = CseSe_KeyOn(rec, &param, rec->prog + opt->v[0]);
            break;
        case 2:
            ret = Cse_ReqKeyOff(&param);
            break;
        case 3:
            ret = Cse_ReqStop(&param);
            break;
        case 5:
            ret = Cse_ReqModulate(&param, rec->mod[0], rec->mod[1], rec->mod[2], rec->mod[3]);
            break;
        case 4:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            break;
        }
        if (rec->next == 0xFFFF || rec->next == id) {
            break;
        }
        id = rec->next;
    }
    return ret;
}

/* This file's initialised data. */
u8 gSndOutputMode = {
#include "main/001BCC98.inc"
};
