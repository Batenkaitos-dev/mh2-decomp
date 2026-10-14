/*
 * CSE engine API and sound effects (SE).
 *
 * Cse_* are the engine calls; Snd_* (0x190940-0x190B10) are the thin layer
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

/* arg is sent to the IOP as the CSE_FNO_INIT request. */
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
    u32 st;
    u32 ret;
    s32 n;

    req[0] = CSE_REQ_STATUS;
    reply = CseRpc_Call(CSE_FNO_STAT, req, sizeof(req));
    n = MlRpcQueue_NumUploads();
    st = 0;
    if (n != 0) {
        st |= 1;
    }
    if (reply[10] != 0) {
        st |= 0x10;
    }
    if (reply[9] != 0) {
        st |= 0x100;
    }
    if (reply[8] != 0) {
        st |= 0x1000;
    }
    switch (mode) {
    case 0:
        if (st == 0) {
            ret = 0;
        } else {
            ret = st | 0x80000000;
        }
        break;
    case 1:
        if ((st & 0x111) == 0) {
            ret = 0;
        } else {
            ret = st | 0x80000000;
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

/* Unpacks the optional values (defaults: v[11] 100, v[13] and v[14] 0x7F). */
s32 Cse_RequestSe(u16 bank, u16 id, CseOpt *opt)
{
    CseSeOpt so = {0};
    u32 flags;
    u32 bit;

    so.v[11] = 100;
    so.v[13] = 0x7F;
    so.v[14] = 0x7F;
    if (opt != NULL && (flags = opt->flags) != 0) {
        bit = 1;
        do {
            switch (flags & bit) {
            case 1:
                so.v[0] = opt->v[0];
                break;
            case 2:
                so.v[1] = opt->v[1];
                break;
            case 4:
                so.v[2] = opt->v[2];
                break;
            case 8:
                so.v[3] = opt->v[3];
                break;
            case 0x10:
                so.v[4] = opt->v[4];
                break;
            case 0x20:
                so.v[5] = opt->v[5];
                break;
            case 0x40:
                so.v[6] = opt->v[6];
                break;
            case 0x80:
                so.v[7] = opt->v[7];
                break;
            case 0x100:
                so.v[8] = opt->v[8];
                break;
            case 0x200:
                so.v[9] = opt->v[9];
                break;
            case 0x400:
                so.v[10] = opt->v[10];
                break;
            case 0x800:
                so.v[11] = opt->v[11];
                break;
            case 0x1000:
                so.v[12] = opt->v[12];
                break;
            case 0x2000:
                so.v[13] = opt->b38;
                break;
            case 0x4000:
                so.v[14] = opt->b39;
                break;
            }
            flags &= ~bit;
            bit <<= 1;
        } while (flags != 0);
    }
    return CseSe_Request(bank, id, &so);
}

s32 Cse_SetParam(s32 bank, s32 id2, s32 id3, CseOpt *opt)
{
    return Cse_ReqSetParam(bank, id2, id3, opt);
}

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

s32 Snd_SetEffect(s32 core, CseEffect *e)
{
    return Cse_ReqSetEffect(core, e->mode, e->depth, e->delay, e->feedback);
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

/* A key-on record: starts the program's tones for p->key. The caller
 * passes prog unmasked (an int); it is a u16 from here on. */
s32 CseSe_KeyOn(CseSeRec *rec, CseParam *p, s32 prog)
{
    if (rec->flags & CSE_F_GLOBAL) {
        p->flags = rec->flags & ~CSE_F_BANK;
    } else {
        p->flags = rec->flags | CSE_F_BANK;
    }
    if (gCseSeKeyOnHook != NULL) {
        gCseSeKeyOnHook(p, rec);
    }
    return Cse_KeyOnProgram(p, (u16)prog);
}

/*
 * Plays SE `id` of a bank: one request per record, following the chain of
 * records (next) until 0xFFFF or a record pointing back at itself. The
 * record's values are offset by opt's, the volume scaled by opt->v[11] %,
 * then clamped.
 */
s32 CseSe_Request(u16 bank, u16 id, CseSeOpt *opt)
{
    CseSeRec *rec;
    CseParam p = {0};
    s32 ret = 0;

    for (;;) {
        rec = CseSe_GetRecord(bank, id);
        if (rec == NULL) {
            return -1;
        }
        p.flags = rec->flags;
        p.bank = bank;
        p.key = rec->key;
        p.vol = rec->vol;
        p.pan = rec->pan;
        p.unkE = rec->unkE;
        p.prio = rec->prio;
        p.id2 = rec->id2;
        p.id3 = rec->id3;
        p.unk18 = rec->unk10;
        p.unk4 = rec->unk8;
        p.unk12 = rec->unk14;
        p.key += opt->v[1];
        p.id2 += opt->v[2];
        p.id3 += opt->v[3];
        p.prio += opt->v[4];
        p.vol += opt->v[5];
        p.pan += opt->v[6];
        p.unkE += opt->v[7];
        p.bend += opt->v[8];
        p.unk18 += opt->v[9];
        p.unk12 += opt->v[10];
        p.vol = p.vol * opt->v[11] / 100;
        p.unk4 |= opt->v[12];
        p.unk13 = opt->v[13];
        p.unk14 = opt->v[14];
        if (p.vol < 0) {
            p.vol = 0;
        }
        if (p.vol > 0x7F) {
            p.vol = 0x7F;
        }
        if (p.pan < 0) {
            p.pan = 0;
        }
        if (p.pan > 0x7F) {
            p.pan = 0x7F;
        }
        if (p.bend < -0x2000) {
            p.bend = -0x2000;
        }
        if (p.bend > 0x2063) {
            p.bend = 0x1FFF;
        }
        switch (rec->type) {
        case 0:
            break;
        case 1:
            ret = CseSe_KeyOn(rec, &p, rec->prog + opt->v[0]);
            break;
        case 2:
            ret = Cse_ReqKeyOff(&p);
            break;
        case 3:
            ret = Cse_ReqStop(&p);
            break;
        case 5:
            ret = Cse_ReqModulate(&p, rec->mod[0], rec->mod[1], rec->mod[2], rec->mod[3]);
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

/* cse's .data (0x1BCC98-0x1BCCA0) */
u8 gSndOutputMode = {
#include "main/001BCC98.inc"
};
