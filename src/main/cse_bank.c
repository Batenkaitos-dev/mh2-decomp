/*
 * CSE bank table: the EE addresses of up to 16 loaded bank headers, and the
 * bank calls that go through the SPU memory map.
 */
#include "cse.h"
#include "main/cse_bank.h"   /* the texts this file uses (tools/content_gen.py) */

extern void *gCseBanks[16];

void scePrintf(char *fmt, ...);

/*
 * Starts the SPU memory map from map (CseSpuMap_Init, which selects page
 * 0) and empties the bank table gCseBanks. Returns 0, or
 * CseSpuMap_Init's negative error code when map is not a valid map.
 */
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

/*
 * Selects page page of the SPU memory map (CseSpuMap_SetPage), which lays
 * out the 16 banks' SPU areas. Returns 0, or the negative error code.
 */
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

/*
 * Records head as the header of bank number bank (0..15) in gCseBanks.
 * Returns 0, or 0x80000001 with a debug message when bank is out of
 * range.
 */
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

/*
 * The header recorded for bank number bank (0..15); NULL when none was
 * set, or with a debug message when bank is out of range.
 */
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
    u32 size;           /* 0x04: 32 */
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
/*
 * Builds one bank header (Head, Prog, Smpl and Vagi chunks) at out from
 * the count bank headers listed in banks: the programs keep their
 * numbers (a program present in two banks is an error, -1), the samples
 * and Vagi records are appended, their indices and SPU addresses moved
 * past the banks before them. Each chunk is padded to 16 bytes with
 * bytes of 255. Returns the merged header's size (also stored in its unkC).
 * Compiled at optimization level 4: at level 3 the same C allocates
 * other registers (the max loop's counter, the unrolled loop's limit and
 * the Prog merge loop's walkers). max is unsigned (the search compares
 * it so) and the -1 fill loop compares it as an s32. The tags are read
 * byte by byte from the literals, as in the original (Head ... Vagi are
 * this file's last .rodata).
 */
s32 CseBank_Merge(void *out, void *banks, s32 count)
{
    CseMergeHead *mergedHead = out;
    CseMergeHead **srcHeads = banks;
    CseMergeHead *srcHead;
    CseChunk *vagi;
    CseChunk *smpl;
    u32 max;
    s32 j;
    s32 k;
    s32 size;
    CseMergeProg *prog;
    s32 i;
    CseMergeProg *srcProgs;
    CseMergeProgram *srcProgram;
    CseMergeProgram *dstProgram;
    u32 spuBase;
    s32 pos;
    CseChunk *smplCountChunk;
    u16 smplBase;
    CseChunk *srcSmplChunk;
    CseMergeSmpl *dstSmpls;
    CseChunk *vagiCountChunk;
    u16 vagiBase;
    CseChunk *srcVagiChunk;
    CseMergeVagi *dstVagis;

    pos = 0;
    mergedHead->tag = CSE_TAG("Head");
    mergedHead->size = 32;
    mergedHead->version = 1;
    mergedHead->unkC = 0;
    mergedHead->bodySize = 0;
    mergedHead->progOffset = 0;
    mergedHead->smplOffset = 0;
    mergedHead->vagiOffset = 0;
    for (i = 0; i < count; i++) {
        mergedHead->bodySize += srcHeads[i]->bodySize;
    }

    pos += 32;
    mergedHead->progOffset = pos;
    prog = (CseMergeProg *)((u8 *)mergedHead + pos);
    smplBase = 0;
    size = 0;
    max = 0;
    size += 16;
    prog->h.tag = CSE_TAG("Prog");
    prog->h.size = 0;
    prog->h.max = 0;
    prog->h.unkC = -1;
    for (i = 0; i < count; i++) {
        srcProgs = (CseMergeProg *)((u8 *)srcHeads[i] + srcHeads[i]->progOffset);
        if (max < srcProgs->h.max) {
            max = srcProgs->h.max;
        }
    }
    prog->h.max = max;
    for (i = 0; i < (s32)max + 1; i++) {
        prog->offs[i] = -1;
    }
    size += (max + 1) * sizeof(s32);
    for (i = 0; i < count; i++) {
        srcHead = srcHeads[i];
        srcProgs = (CseMergeProg *)((u8 *)srcHead + srcHead->progOffset);
        smplCountChunk = (CseChunk *)((u8 *)srcHead + srcHead->smplOffset);
        for (j = 0; j < srcProgs->h.max + 1; j++) {
            if (srcProgs->offs[j] != -1) {
                srcProgram = (CseMergeProgram *)((u8 *)srcProgs + srcProgs->offs[j]);
                if (prog->offs[j] != -1) {
                    return -1;
                }
                prog->offs[j] = size;
                dstProgram = (CseMergeProgram *)((u8 *)prog + prog->offs[j]);
                size += srcProgram->h.numSplits * sizeof(CseMergeSplit) + sizeof(CseProgHead);
                dstProgram->h = srcProgram->h;
                for (k = 0; k < dstProgram->h.numSplits; k++) {
                    dstProgram->splits[k] = srcProgram->splits[k];
                    dstProgram->splits[k].w[6] += smplBase;
                }
            }
        }
        smplBase += smplCountChunk->max + 1;
    }
    for (; size & 0xF; size++) {
        ((u8 *)prog)[size] = 255;
    }
    pos += size;
    prog->h.size = size;

    mergedHead->smplOffset = pos;
    smpl = (CseChunk *)((u8 *)mergedHead + pos);
    size = 0;
    vagiBase = 0;
    size += 16;
    smpl->tag = CSE_TAG("Smpl");
    smpl->size = 0;
    smpl->max = 0;
    smpl->unkC = -1;
    for (i = 0; i < count; i++) {
        dstSmpls = (CseMergeSmpl *)((u8 *)smpl + size);
        srcHead = srcHeads[i];
        srcSmplChunk = (CseChunk *)((u8 *)srcHead + srcHead->smplOffset);
        vagiCountChunk = (CseChunk *)((u8 *)srcHead + srcHead->vagiOffset);
        smpl->max += srcSmplChunk->max + 1;
        for (j = 0; j < srcSmplChunk->max + 1; j++) {
            dstSmpls[j] = ((CseSmplChunk *)srcSmplChunk)->smpl[j];
            dstSmpls[j].w[5] += vagiBase;
        }
        size += ((u32)srcSmplChunk->max + 1) * sizeof(CseMergeSmpl);
        vagiBase += vagiCountChunk->max + 1;
    }
    smpl->max--;
    for (; size & 0xF; size++) {
        ((u8 *)smpl)[size] = 255;
    }
    pos += size;
    smpl->size = size;

    mergedHead->vagiOffset = pos;
    vagi = (CseChunk *)((u8 *)mergedHead + pos);
    size = 0;
    spuBase = 0;
    size += 16;
    vagi->tag = CSE_TAG("Vagi");
    vagi->size = 0;
    vagi->max = 0;
    vagi->unkC = -1;
    for (i = 0; i < count; i++) {
        dstVagis = (CseMergeVagi *)((u8 *)vagi + size);
        srcHead = srcHeads[i];
        srcVagiChunk = (CseChunk *)((u8 *)srcHead + srcHead->vagiOffset);
        vagi->max += srcVagiChunk->max + 1;
        for (j = 0; j < srcVagiChunk->max + 1; j++) {
            dstVagis[j] = ((CseVagiChunk *)srcVagiChunk)->vagi[j];
            dstVagis[j].addr += spuBase;
        }
        size += ((u32)srcVagiChunk->max + 1) * sizeof(CseMergeVagi);
        spuBase += srcHead->bodySize;
    }
    vagi->max--;
    for (; size & 0xF; size++) {
        ((u8 *)vagi)[size] = 255;
    }
    vagi->size = size;
    pos += size;
    mergedHead->unkC = pos;
    return pos;
}
#pragma optimization_level reset
