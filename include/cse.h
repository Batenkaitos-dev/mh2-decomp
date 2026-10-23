#ifndef CSE_H
#define CSE_H

/*
 * EE side of the Capcom Sound Engine (CSE.IRX, "CAPCOM Sound Engine for IOP
 * Ver.0.17") and of the emlRpcQueue request queue, in the boot ELF (the
 * cse*.c files and ml_rpc_queue.c).
 *
 * Layers, from the game down:
 *   Snd_*         the game's sound calls (the only entry points used outside)
 *   Cse_*         the engine API; Cse_Req* build one COMM request each
 *   MlRpcQueue_*  64 queued requests, sent once per frame by Cse_Update
 *   CseRpc_*      sceSifCallRpc to the IOP servers COMM, STAT and THMN
 */

#include "common.h"

/* SIF RPC servers and the function numbers CseRpc_Call takes. */
#define CSE_SID_COMM 0x434F4D4D     /* "COMM": requests (fno 1, 3, 777) */
#define CSE_SID_STAT 0x53544154     /* "STAT": status (fno 2) */
#define CSE_SID_THMN 0x54484D4E     /* "THMN" (CSE_FNO_THMN) */

#define CSE_FNO_COMM 1              /* a request for the IOP's sound driver thread */
#define CSE_FNO_STAT 2              /* status: reply bytes 8-10 are "transfer busy" */
#define CSE_FNO_UPLOAD 3            /* a CSE_REQ_UPLOAD request */
#define CSE_FNO_THMN 0x63           /* 1024-byte reply */
#define CSE_FNO_INIT 0x309          /* sent directly (not queued) by Cse_Init */

/*
 * Word 0 of a request. Word 1 is a sequence number (never 0) that
 * MlRpcQueue_Set assigns; the arguments follow from byte 8. The IOP accepts
 * commands 0-13 except 5; the EE library sends the ones below.
 */
#define CSE_REQ_KEYON 0x10000000    /* CseParam + CseVoice: start a voice */
#define CSE_REQ_KEYOFF 0x10000001   /* CseParam: release the matching voices */
#define CSE_REQ_STOP 0x10000002     /* CseParam: stop the matching voices */
#define CSE_REQ_SETPARAM 0x10000003 /* CseParam: change the matching voices */
#define CSE_REQ_MODULATE 0x10000004 /* CseParam + 4 u16: modulation of matching voices */
#define CSE_REQ_EFFECT 0x10000007   /* core, mode, depth, delay, feedback, end address */
#define CSE_REQ_MIXER 0x10000008    /* core, SD_P_MMIX value */
#define CSE_REQ_MONO 0x10000009     /* 1: mix every voice to mono */
#define CSE_REQ_VOLUME 0x1000000A   /* channel (0xFF: master), volume 0-127 */
#define CSE_REQ_STATUS 0x20000000   /* the STAT request */
#define CSE_REQ_UPLOAD 0x30000000   /* EE address, SPU address, size */

/* CseParam flags. FLAG_GLOBAL, FLAG_PHYSICAL, FLAG_BANK and ID2 are the
 * engine's own names (from its error messages). */
#define CSE_F_PRIO 0x1              /* prio is valid (else 127) */
#define CSE_F_GLOBAL 0x2            /* FLAG_GLOBAL: every bank */
#define CSE_F_PHYSICAL 0x4          /* FLAG_PHYSICAL */
#define CSE_F_BANK 0x8              /* FLAG_BANK: bank is valid */
#define CSE_F_KEY 0x10              /* key is valid */
#define CSE_F_ID2 0x20              /* id2 is valid; needs one of the three above */
#define CSE_F_ID3 0x40              /* id3 is valid; needs ID2 */

/*
 * The parameter block of CSE_REQ_KEYON..CSE_REQ_MODULATE (32 bytes): which
 * voices a request applies to, and the values it sets.
 */
typedef struct CseParam {
    u32 flags;          /* 0x00: CSE_F_* and value flags */
    u8 unk4;            /* 0x04 */
    u8 prio;            /* 0x05 */
    u8 bank;            /* 0x06 */
    u8 key;             /* 0x07: note */
    u8 id2;             /* 0x08 */
    u8 id3;             /* 0x09 */
    s16 vol;            /* 0x0A: 0-127 */
    s16 pan;            /* 0x0C: 0-127 */
    s16 unkE;           /* 0x0E */
    s16 bend;           /* 0x10: -8192..8191 */
    u8 unk12;           /* 0x12 */
    u8 unk13;           /* 0x13 */
    u8 unk14;           /* 0x14 */
    u8 pad15[3];
    u32 unk18;          /* 0x18 */
    s32 unk1C;
} CseParam;

/* Optional values for Snd_RequestSe and Snd_SetParam: bit n of flags says
 * that value n is valid (v[0..12], then b38 and b39). */
typedef struct CseOpt {
    u32 flags;
    s32 v[13];
    u8 b38;
    u8 b39;
} CseOpt;

/* The same values unpacked, with defaults, for CseSe_Request. */
typedef struct CseSeOpt {
    s32 v[15];
} CseSeOpt;

/* One record of an SE table (a bank's list of sound effects). */
typedef struct CseSeRec {
    u8 type;            /* 0x00: 1 key on, 2 key off, 3 stop, 5 modulation */
    u8 unk1[3];
    u16 flags;          /* 0x04: CseParam flags */
    u8 prog;            /* 0x06: program, for type 1 */
    u8 key;             /* 0x07: note */
    u8 unk8;            /* 0x08 */
    u8 prio;            /* 0x09 */
    u8 id2;             /* 0x0A */
    u8 id3;             /* 0x0B */
    u8 vol;             /* 0x0C */
    u8 pan;             /* 0x0D */
    s16 unkE;           /* 0x0E */
    s32 unk10;          /* 0x10 */
    u8 unk14;           /* 0x14 */
    u8 unk15;
    u16 next;           /* 0x16: next record of this SE, 0xFFFF = last */
    u16 mod[4];         /* 0x18: CSE_REQ_MODULATE values; the game's code
                         * also reads [0] and [1] as random pitch and volume
                         * ranges (Sound_PlaySe) */
} CseSeRec;

typedef struct CseSeTable {
    u8 unk0[0xC];
    u32 count;          /* 0x0C */
    CseSeRec recs[1];   /* 0x10 */
} CseSeTable;

/* Effect (reverb) settings, as Snd_SetEffect takes them. */
typedef struct CseEffect {
    u16 mode;
    s16 depth;
    u16 delay;
    u16 feedback;
} CseEffect;

/* One voice's attributes, from CseBankFile_GetVoice (20 bytes). */
typedef struct CseVoice {
    u8 vol;             /* 0x00: 0-127 */
    u8 pan;             /* 0x01: 0-127, 64 = centre */
    s16 pitch;          /* 0x02: in cents */
    u16 adsr1;          /* 0x04 */
    u16 adsr2;          /* 0x06 */
    u16 unk8;           /* 0x08 */
    u16 unkA;           /* 0x0A */
    u32 unkC;           /* 0x0C */
    u32 addr;           /* 0x10: SPU address of the sample */
} CseVoice;

/* A bank file's header chunk ("Head"); the other chunks follow it. */
typedef struct CseBankHead {
    char tag[4];        /* 0x00: "Head" */
    u32 unk4[3];
    u32 bodySize;       /* 0x10: size of the sample data (uploaded to SPU RAM) */
    u32 progOffset;     /* 0x14: "Prog" chunk, from the header */
    u32 smplOffset;     /* 0x18: "Smpl" chunk */
    u32 vagiOffset;     /* 0x1C: "Vagi" chunk */
} CseBankHead;

typedef struct CseSplit CseSplit;
typedef struct CseProgram CseProgram;
typedef struct CseSample CseSample;
typedef struct CseVagi CseVagi;

/* The four records that describe one tone (CseBankFile_GetTone). */
typedef struct CseTone {
    CseProgram *prog;
    CseSplit *split;
    CseSample *sample;
    CseVagi *vagi;
} CseTone;

/* cse_spumap.c */
s32 CseSpuMap_Init(void *map);
s32 CseSpuMap_SetPage(u32 page);
u32 CseSpuMap_GetBankAddr(u32 bank);
u32 CseSpuMap_GetEffectEnd(s32 core);

/* cse_rpc.c */
s32 CseRpc_Init(void);
void *CseRpc_Call(s32 fno, void *data, u32 size);
void CseRpc_Send(void);

/* cse.c */
s32 Cse_Init(s32 arg);
s32 Cse_Update(void);
u32 cseCheckVtransStatus(s32 mode);
s32 CseSe_Init(void);
s32 CseSe_Update(void);
s32 CseSe_SetTable(u32 bank, CseSeTable *table);
CseSeRec *CseSe_GetRecord(u32 bank, u32 id);
s32 CseSe_Request(u16 bank, u16 id, CseSeOpt *opt);
void Snd_Init(void);
s32 Snd_Update(void);
void Snd_SetOutputMode(s32 mode);
s32 Snd_SetMasterVolume(s32 vol);
s32 Snd_InitSpuMap(void *map);
s32 Snd_SetSpuMapPage(u32 page);
s32 Snd_SetBank(u32 bank, void *head);
s32 Snd_SetSeTable(u32 bank, CseSeTable *table);
CseSeRec *Snd_GetSeRecord(u32 bank, u32 id);
s32 Snd_UploadBank(u32 bank, u32 src, u32 size, u32 offset);
u32 Snd_CheckTransfer(s32 mode);
s32 Snd_GetBankBodySize(void *head);
s32 Snd_MergeBanks(void *out, void *banks, s32 count);
s32 Snd_SetEffect(s32 core, CseEffect *e);
s32 Snd_StopAll(void);
s32 Snd_StopBank(s32 bank);
s32 Snd_SetParam(s32 bank, s32 id2, s32 id3, CseOpt *opt);
s32 Snd_RequestSe(u32 bank, u32 id, CseOpt *opt);

/* cse_bank.c */
s32 CseBank_Init(void *map);
s32 CseBank_SetPage(u32 page);
u32 CseBank_GetSpuAddr(u32 bank);
u32 CseBank_GetEffectEnd(s32 core);
s32 CseBank_Set(u32 bank, void *head);
void *CseBank_Get(u32 bank);
s32 CseBank_Merge(void *out, void *banks, s32 count);

/* cse_bankfile.c */
s32 CseBankFile_IsHead(void *p);
s32 CseBankFile_IsProg(void *p);
s32 CseBankFile_IsSmpl(void *p);
s32 CseBankFile_IsVagi(void *p);
s32 CseBankFile_GetNumSplits(CseBankHead *head, u8 prog);
s32 CseBankFile_GetTone(CseTone *out, CseBankHead *head, u8 prog, u8 key, u8 split);
s32 CseBankFile_GetVoice(CseVoice *v, CseTone *t, u8 key, u32 spuBase);
s32 CseBankFile_GetBodySize(CseBankHead *head);

/* ml_rpc_queue.c */
s32 MlRpcQueue_Init(void);
s32 MlRpcQueue_NumUploads(void);
s32 mlRpcQueueReferStatus(void);
u32 MlRpcQueue_Set(s32 fno, void *req, s32 size);
s32 mlRpcQueueSend(void);
s32 mlRpcQueueSendIntr(void);

/* cse_cmd.c */
s32 Cse_ReqInit(s32 arg);
s32 Cse_ReqSetMono(s32 on);
s32 Cse_ReqSetMasterVolume(s32 vol);
s32 Cse_ReqSetVolume(s32 ch, s32 vol);
s32 Cse_ReqModulate(CseParam *p, u16 a, u16 b, u16 c, u16 d);
s32 Cse_ReqStop(CseParam *p);
s32 Cse_ReqKeyOff(CseParam *p);
s32 Cse_ReqStopAll(void);
s32 Cse_ReqStopBank(s32 bank);
s32 Cse_ReqSetParam(s32 bank, s32 id2, s32 id3, CseOpt *opt);
s32 Cse_ReqSetEffect(s32 core, s32 mode, s32 depth, u8 delay, u8 feedback);
s32 Cse_ReqUpload(u32 src, u32 dst, u32 size);
s32 Cse_ReqKeyOn(CseVoice *v, CseParam *p);
s32 Cse_KeyOnProgram(CseParam *p, s32 prog);
s32 Cse_CheckParam(CseParam *p);
s32 Cse_ReqParam(CseParam *p, u32 cmd);
s32 Cse_ReqSetMixer(s16 core, s16 mmix);
s32 Cse_InitSeq(void);
u32 Cse_NextSeq(void);

#endif /* CSE_H */
