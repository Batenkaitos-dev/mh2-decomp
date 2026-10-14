/*
 * CSE bank table: the EE addresses of up to 16 loaded bank headers, and the
 * bank calls that go through the SPU memory map.
 */
#include "cse.h"
#include "main/cse_bank.h"   /* the texts this file uses (tools/content_gen.py) */

extern void *gCseBanks[16];

void scePrintf(char *fmt, ...);

s32 CseBank_Init(void *map)
{
    s32 ret;
    s32 i;

    ret = CseSpuMap_Init(map);
    if (ret < 0) {
        return ret;
    }
    for (i = 0; i < 16; i++) {
        gCseBanks[i] = NULL;
    }
    return 0;
}

s32 CseBank_SetPage(u32 page)
{
    s32 ret;

    ret = CseSpuMap_SetPage(page);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

u32 CseBank_GetSpuAddr(u32 bank)
{
    return CseSpuMap_GetBankAddr(bank);
}

u32 CseBank_GetEffectEnd(s32 core)
{
    return CseSpuMap_GetEffectEnd(core);
}

s32 CseBank_Set(u32 bank, void *head)
{
    if (bank >= 16) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf(TXT(0x001C8290) /* debug message: no such bank number */);
        return 0x80000001;
    }
    gCseBanks[bank] = head;
    return 0;
}

void *CseBank_Get(u32 bank)
{
    if (bank >= 16) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf(TXT(0x001C8290) /* debug message: no such bank number */);
        return NULL;
    }
    return gCseBanks[bank];
}

/* The chunks of a bank file as CseBank_Merge writes them: a 16-byte header,
 * then the records. */
typedef struct CseChunk {
    u32 tag;            /* 0x00 */
    u32 size;           /* 0x04: bytes, the header included, padded to 16 */
    s32 max;            /* 0x08: highest record index */
    s32 unkC;           /* 0x0C: -1 */
} CseChunk;

typedef struct CseMergeProg {
    CseChunk h;
    s32 offs[1];        /* 0x10: per program, from the chunk; -1 = none */
} CseMergeProg;

typedef struct CseProgHead {
    u8 numSplits;
    u8 unk1;
    s16 unk2[3];
} CseProgHead;

typedef struct CseMergeSplit {
    u16 w[7];           /* 0x0C: the sample index */
} CseMergeSplit;

typedef struct CseMergeSmpl {
    u16 w[6];           /* 0x0A: the Vagi index */
} CseMergeSmpl;

typedef struct CseMergeProgram {
    CseProgHead h;
    CseMergeSplit splits[1];    /* 0x08 */
} CseMergeProgram;

typedef struct CseMergeVagi {
    u32 addr;           /* in the bank's SPU area */
    u32 unk4[3];
} CseMergeVagi;

/* A Smpl or Vagi chunk: the header, then the records. */
typedef struct CseSmplChunk {
    CseChunk h;
    CseMergeSmpl smpl[1];
} CseSmplChunk;

typedef struct CseVagiChunk {
    CseChunk h;
    CseMergeVagi vagi[1];
} CseVagiChunk;

typedef struct CseMergeHead {
    u32 tag;
    u32 size;           /* 0x04: 0x20 */
    u32 version;        /* 0x08: 1 */
    u32 unkC;
    u32 bodySize;       /* 0x10 */
    u32 progOffset;     /* 0x14 */
    u32 smplOffset;     /* 0x18 */
    u32 vagiOffset;     /* 0x1C */
} CseMergeHead;

#define CSE_TAG(s) ((s)[0] | ((s)[1] << 8) | ((s)[2] << 16) | ((s)[3] << 24))

/*
 * Builds one bank header (Head, Prog, Smpl and Vagi chunks) at out from the
 * count bank headers in banks: the programs keep their numbers (a program
 * present in two banks is an error, -1), the samples and Vagi records are
 * appended, their indices and SPU addresses moved by the banks before.
 * Returns the size.
 */
/* Compiled at optimization level 4: at level 3 the same C gives other
 * registers in 59 words (the max loop's counter, the unrolled loop's limit
 * and the Prog merge loop's walkers). max is unsigned (the search compares
 * it so) and the -1 fill loop compares it as an s32. The tags are read byte
 * by byte from the literals, as in the original ("Head" ... "Vagi" are this
 * file's last .rodata). */
#pragma optimization_level 4
s32 CseBank_Merge(void *out, void *banks, s32 count)
{
    CseMergeHead *h = out;
    CseMergeHead **in = banks;
    CseMergeHead *b;
    CseChunk *vagi;
    CseChunk *smpl;
    u32 max;
    s32 j;
    s32 k;
    s32 size;
    CseMergeProg *prog;
    s32 i;
    CseMergeProg *src;
    CseMergeProgram *sp;
    CseMergeProgram *dp;
    u32 body;
    s32 pos;
    CseChunk *ssrc;
    u16 base;
    CseChunk *ss;
    CseMergeSmpl *ds;
    CseChunk *vs;
    u16 vbase;
    CseChunk *vc;
    CseMergeVagi *dv;

    pos = 0;
    h->tag = CSE_TAG("Head");
    h->size = 0x20;
    h->version = 1;
    h->unkC = 0;
    h->bodySize = 0;
    h->progOffset = 0;
    h->smplOffset = 0;
    h->vagiOffset = 0;
    for (i = 0; i < count; i++) {
        h->bodySize += in[i]->bodySize;
    }

    pos += 0x20;
    h->progOffset = pos;
    prog = (CseMergeProg *)((u8 *)h + pos);
    base = 0;
    size = 0;
    max = 0;
    size += 0x10;
    prog->h.tag = CSE_TAG("Prog");
    prog->h.size = 0;
    prog->h.max = 0;
    prog->h.unkC = -1;
    for (i = 0; i < count; i++) {
        src = (CseMergeProg *)((u8 *)in[i] + in[i]->progOffset);
        if (max < src->h.max) {
            max = src->h.max;
        }
    }
    prog->h.max = max;
    for (i = 0; i < (s32)max + 1; i++) {
        prog->offs[i] = -1;
    }
    size += (max + 1) * sizeof(s32);
    for (i = 0; i < count; i++) {
        b = in[i];
        src = (CseMergeProg *)((u8 *)b + b->progOffset);
        ssrc = (CseChunk *)((u8 *)b + b->smplOffset);
        for (j = 0; j < src->h.max + 1; j++) {
            if (src->offs[j] != -1) {
                sp = (CseMergeProgram *)((u8 *)src + src->offs[j]);
                if (prog->offs[j] != -1) {
                    return -1;
                }
                prog->offs[j] = size;
                dp = (CseMergeProgram *)((u8 *)prog + prog->offs[j]);
                size += sp->h.numSplits * sizeof(CseMergeSplit) + sizeof(CseProgHead);
                dp->h = sp->h;
                for (k = 0; k < dp->h.numSplits; k++) {
                    dp->splits[k] = sp->splits[k];
                    dp->splits[k].w[6] += base;
                }
            }
        }
        base += ssrc->max + 1;
    }
    for (; size & 0xF; size++) {
        ((u8 *)prog)[size] = 0xFF;
    }
    pos += size;
    prog->h.size = size;

    h->smplOffset = pos;
    smpl = (CseChunk *)((u8 *)h + pos);
    size = 0;
    vbase = 0;
    size += 0x10;
    smpl->tag = CSE_TAG("Smpl");
    smpl->size = 0;
    smpl->max = 0;
    smpl->unkC = -1;
    for (i = 0; i < count; i++) {
        ds = (CseMergeSmpl *)((u8 *)smpl + size);
        b = in[i];
        ss = (CseChunk *)((u8 *)b + b->smplOffset);
        vs = (CseChunk *)((u8 *)b + b->vagiOffset);
        smpl->max += ss->max + 1;
        for (j = 0; j < ss->max + 1; j++) {
            ds[j] = ((CseSmplChunk *)ss)->smpl[j];
            ds[j].w[5] += vbase;
        }
        size += ((u32)ss->max + 1) * sizeof(CseMergeSmpl);
        vbase += vs->max + 1;
    }
    smpl->max--;
    for (; size & 0xF; size++) {
        ((u8 *)smpl)[size] = 0xFF;
    }
    pos += size;
    smpl->size = size;

    h->vagiOffset = pos;
    vagi = (CseChunk *)((u8 *)h + pos);
    size = 0;
    body = 0;
    size += 0x10;
    vagi->tag = CSE_TAG("Vagi");
    vagi->size = 0;
    vagi->max = 0;
    vagi->unkC = -1;
    for (i = 0; i < count; i++) {
        dv = (CseMergeVagi *)((u8 *)vagi + size);
        b = in[i];
        vc = (CseChunk *)((u8 *)b + b->vagiOffset);
        vagi->max += vc->max + 1;
        for (j = 0; j < vc->max + 1; j++) {
            dv[j] = ((CseVagiChunk *)vc)->vagi[j];
            dv[j].addr += body;
        }
        size += ((u32)vc->max + 1) * sizeof(CseMergeVagi);
        body += b->bodySize;
    }
    vagi->max--;
    for (; size & 0xF; size++) {
        ((u8 *)vagi)[size] = 0xFF;
    }
    vagi->size = size;
    pos += size;
    h->unkC = pos;
    return pos;
}
#pragma optimization_level reset
