/*
 * CSE bank files (.snd headers): a "Head" chunk, then "Prog" (programs,
 * each a list of key splits), "Smpl" (sample settings) and "Vagi" (where
 * each sample's ADPCM data is, relative to the bank's SPU area). A tone is
 * one split of a program; its voice attributes combine the program, split
 * and sample settings.
 */
#include "cse.h"
#include "main/cse_bankfile.h"   /* the texts this file uses (tools/content_gen.py) */

typedef struct CseBankProgs {
    char tag[4];        /* 0x00: "Prog" */
    u32 unk4;
    u32 maxProg;        /* 0x08: highest program number */
    u32 unkC;
    s32 offset[1];      /* 0x10: each program, from this chunk; -1 = none */
} CseBankProgs;

struct CseSplit {
    u16 unk0;
    u8 keyLow;          /* 0x02: keys this split plays */
    u8 keyHigh;         /* 0x03 */
    u16 adsr1;          /* 0x04 */
    u16 adsr2;          /* 0x06 */
    s8 vol;             /* 0x08 */
    s8 pan;             /* 0x09: 64 = centre */
    s8 coarse;          /* 0x0A: semitones */
    s8 fine;            /* 0x0B: cents */
    u16 sample;         /* 0x0C */
};

struct CseProgram {
    u8 numSplits;       /* 0x00 */
    u8 unk1;
    s8 vol;             /* 0x02 */
    s8 pan;             /* 0x03 */
    s8 coarse;          /* 0x04 */
    s8 fine;            /* 0x05 */
    u8 unk6[2];
    CseSplit splits[1]; /* 0x08 */
};

struct CseSample {
    u16 unk0;
    u16 unk2;
    u8 unk4;
    u8 root;            /* 0x05: key at which the sample plays unchanged */
    s8 vol;             /* 0x06 */
    s8 pan;             /* 0x07 */
    s8 coarse;          /* 0x08 */
    s8 fine;            /* 0x09 */
    u16 vagi;           /* 0x0A */
};

typedef struct CseBankSmpl {
    char tag[4];        /* 0x00: "Smpl" */
    u32 unk4[3];
    CseSample samples[1]; /* 0x10 */
} CseBankSmpl;

struct CseVagi {
    u32 addr;           /* 0x00: from the bank's SPU area */
    u32 unk4[2];
    u32 unkC;           /* 0x0C */
};

typedef struct CseBankVagi {
    char tag[4];        /* 0x00: "Vagi" */
    u32 unk4[3];
    CseVagi vagis[1];   /* 0x10 */
} CseBankVagi;


void scePrintf(char *fmt, ...);
s32 strncmp(char *a, char *b, u32 n);

s32 CseBankFile_IsHead(void *chunk)
{
    return strncmp(chunk, "Head", 4) == 0;
}

s32 CseBankFile_IsProg(void *chunk)
{
    return strncmp(chunk, "Prog", 4) == 0;
}

s32 CseBankFile_IsSmpl(void *chunk)
{
    return strncmp(chunk, "Smpl", 4) == 0;
}

s32 CseBankFile_IsVagi(void *chunk)
{
    return strncmp(chunk, "Vagi", 4) == 0;
}

/*
 * The number of key splits of program prog in the bank header head.
 * Returns 0x80000006 when head is not a Head chunk, 0x80000007 when its
 * Prog chunk is bad, 0x8000000E when the bank has no program prog.
 */
s32 CseBankFile_GetNumSplits(CseBankHead *head, u8 prog)
{
    CseBankProgs *progs;
    s32 programOffset;

    if (CseBankFile_IsHead(head) != 1) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf(TXT(0x001C8300) /* debug message: a bad HEAD chunk */);
        return 0x80000006;
    }
    progs = (CseBankProgs *)((u8 *)head + head->progOffset);
    if (CseBankFile_IsProg(progs) != 1) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf(TXT(0x001C8320) /* debug message: a bad PROG chunk */);
        return 0x80000007;
    }
    if (progs->maxProg < prog) {
        return 0x8000000E;
    }
    programOffset = progs->offset[prog];
    if (programOffset == -1) {
        return 0x8000000E;
    } else {
        return ((CseProgram *)((u8 *)progs + programOffset))->numSplits;
    }
}

/*
 * Finds split number split of program prog in the bank header head and
 * fills out with that program, split, sample and Vagi record. Returns 0;
 * 0x8000000F (out not filled) when the split does not play key;
 * 0x80000006, 0x80000007, 0x80000008 or 0x80000009 when the Head, Prog,
 * Smpl or Vagi chunk is bad. prog and split are not range-checked.
 */
s32 CseBankFile_GetTone(CseTone *out, CseBankHead *head, u8 prog, u8 key, u8 split)
{
    CseBankProgs *progs = (CseBankProgs *)((u8 *)head + head->progOffset);
    CseBankSmpl *smpl = (CseBankSmpl *)((u8 *)head + head->smplOffset);
    CseBankVagi *vagi = (CseBankVagi *)((u8 *)head + head->vagiOffset);
    CseVagi *toneVagi;
    CseSample *toneSample;
    CseSplit *toneSplit;
    CseProgram *toneProgram;

    if (CseBankFile_IsHead(head) != 1) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf(TXT(0x001C8300) /* debug message: a bad HEAD chunk */);
        return 0x80000006;
    }
    if (CseBankFile_IsProg(progs) != 1) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf(TXT(0x001C8320) /* debug message: a bad PROG chunk */);
        return 0x80000007;
    }
    if (CseBankFile_IsSmpl(smpl) != 1) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf(TXT(0x001C8340) /* debug message: a bad SMPL chunk */);
        return 0x80000008;
    }
    if (CseBankFile_IsVagi(vagi) != 1) {
        scePrintf("[EE]");
        scePrintf("(ERR)");
        scePrintf(TXT(0x001C8360) /* debug message: a bad VAGI chunk */);
        return 0x80000009;
    }
    toneProgram = (CseProgram *)((u8 *)progs + progs->offset[prog]);
    toneSplit = &toneProgram->splits[split];
    toneSample = &smpl->samples[toneSplit->sample];
    toneVagi = &vagi->vagis[toneSample->vagi];
    if (key < toneSplit->keyLow || key > toneSplit->keyHigh) {
        return 0x8000000F;
    }
    out->prog = toneProgram;
    out->split = toneSplit;
    out->sample = toneSample;
    out->vagi = toneVagi;
    return 0;
}

/*
 * Fills voice with the attributes of tone played at key: volume = the
 * program's * the split's * the sample's (each 0-127), pan = the sum of
 * the three offsets from 64 (clamped at each step), pitch in cents from
 * the three tunings and the distance from the sample's root key, the
 * split's ADSR, and the sample data's SPU address (spuBase, the bank's
 * SPU area, plus the Vagi record's addr). Returns 0.
 */
s32 CseBankFile_GetVoice(CseVoice *voice, CseTone *tone, u8 key, u32 spuBase)
{
    s16 pan;

    voice->vol = tone->prog->vol;
    voice->vol = voice->vol * tone->split->vol / 127;
    voice->vol = voice->vol * tone->sample->vol / 127;
    pan = tone->prog->pan - 64;
    if (pan < -64) {
        pan = -64;
    } else if (pan > 63) {
        pan = 63;
    }
    pan += (s16)(tone->split->pan - 64);
    if (pan < -64) {
        pan = -64;
    } else if (pan > 63) {
        pan = 63;
    }
    pan += (s16)(tone->sample->pan - 64);
    if (pan < -64) {
        pan = -64;
    } else if (pan > 63) {
        pan = 63;
    }
    voice->pan = pan + 64;
    voice->pitch = tone->prog->coarse * 100 + tone->prog->fine;
    voice->pitch += (s16)(tone->split->coarse * 100 + tone->split->fine);
    voice->pitch += (s16)(tone->sample->coarse * 100 + tone->sample->fine);
    voice->pitch += (s16)((key - tone->sample->root) * 100);
    voice->adsr1 = tone->split->adsr1;
    voice->adsr2 = tone->split->adsr2;
    voice->unk8 = tone->sample->unk0;
    voice->unkA = tone->sample->unk2;
    voice->addr = spuBase + tone->vagi->addr;
    voice->unkC = tone->vagi->unkC;
    return 0;
}

/*
 * The size of the bank's sample data (head->bodySize); 0x80000006 when
 * head is not a Head chunk.
 */
s32 CseBankFile_GetBodySize(CseBankHead *head)
{
    if (CseBankFile_IsHead(head) != 1) {
        return 0x80000006;
    }
    return head->bodySize;
}
