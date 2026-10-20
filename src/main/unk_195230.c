/*
 * Loading the sound banks of a scene from
 * afs01.afs (partition 1), a step per frame through the file queue
 * (SndLoad_*), or at once with Sound_ReadBank (0x195BF0).
 */
#include "common.h"
#include "syssettings.h"
#include "stream.h"

/* Progress of a staged load. */
typedef struct SndLoad {
    /* 0x0 */ s8 state;
    /* 0x1 */ s8 index;         /* next entry of the list */
    /* 0x2 */ s8 count;         /* entries in the list */
    /* 0x3 */ s8 kinds[4];      /* banks already queued */
} SndLoad;

extern SndLoad gSndLoad;
/* sub_main tables of bank numbers (file 0x12C + n / 0x14D + n / 0x39 + n) */
extern s8 D_43C6E0[];
extern s8 gVoiceBanks[][16];
extern s16 gQuestSoundBanks[];

void *memset(void *dst, s32 c, u32 n);
void FlushCache(s32 op);
s32 Sound_StopAll(void);
s32 Sound_StopBank(s32 slot);
struct SndFile;
s32 Sound_LoadBankWait(struct SndFile *f, u32 slot);
void Sound_InitStaging(u8 *headBase, u8 *bodyBase);
void Sound_StageBank(struct SndFile *f);
void Sound_QueueBank(u32 id, void *buf);
s32 Sound_ReadBank(u32 id, void *buf);


void Sound_Silence(void)
{
    Sound_StopAll();
}

void SndLoad_Init(s8 count)
{
    memset(&gSndLoad, 0, sizeof(SndLoad));
    gSndLoad.count = count;
}

/* Stages bank file 0x14D, then one bank per different kind of the list
 * (each id's kind from sub_main's table of bank kinds; kinds 0x26/0x1C, 0x27/0x1D and 0x1B/1 count as the
 * same); returns 1 when done. */
s32 SndLoad_ListBanks(u8 *buf, u8 *list)
{
    s32 done = 0;
    s32 i;
    s8 kind;
    u8 id;

    switch (gSndLoad.state) {
    case 0:
        Sound_InitStaging(buf + 0xB4800, buf + 0xD4800);
        Sound_QueueBank(0x1014D, buf);
        gSndLoad.state++;
        break;
    case 1:
        if (!FileQueue_IsBusy()) {
            gSndLoad.state++;
        }
        break;
    case 2:
        id = list[gSndLoad.index];
        if (id == 0) {
            if (++gSndLoad.index >= gSndLoad.count) {
                done = 1;
            }
            break;
        }
        kind = D_43C6E0[id];
        if (kind == 0) {
            if (++gSndLoad.index >= gSndLoad.count) {
                done = 1;
            }
            break;
        }
        if (kind == 0x26) {
            for (i = 0; i < 4; i++) {
                if (kind == gSndLoad.kinds[i] || gSndLoad.kinds[i] == 0x1C) {
                    break;
                }
            }
        } else if (kind == 0x27) {
            for (i = 0; i < 4; i++) {
                if (kind == gSndLoad.kinds[i] || gSndLoad.kinds[i] == 0x1D) {
                    break;
                }
            }
        } else if (kind == 0x1B) {
            for (i = 0; i < 4; i++) {
                if (kind == gSndLoad.kinds[i] || gSndLoad.kinds[i] == 1) {
                    break;
                }
            }
        } else {
            for (i = 0; i < 4; i++) {
                if (kind == gSndLoad.kinds[i]) {
                    break;
                }
            }
        }
        if (i < 4) {
            if (++gSndLoad.index >= gSndLoad.count) {
                done = 1;
            }
            break;
        }
        gSndLoad.kinds[gSndLoad.index] = kind;
        Sound_QueueBank((kind + 0x14D) | 0x10000, buf + 0x4800);
        gSndLoad.state++;
        break;
    case 3:
        if (!FileQueue_IsBusy()) {
            Sound_StageBank((struct SndFile *) (buf + 0x4800));
            if (++gSndLoad.index >= gSndLoad.count) {
                done = 1;
                gSndLoad.state++;
            } else {
                gSndLoad.state--;
            }
        }
        break;
    default:
        done = 1;
        break;
    }
    return done;
}

/* The two banks of a quest: its area's (file 0x39 + gQuestSoundBanks[area], or
 * 0x12B) and its monster's (0x12C + gVoiceBanks[set][monster], or 0x14C). */
s32 SndLoad_QuestBanks(u8 *buf, u8 *quest)
{
    s32 done = 0;
    u32 id;

    switch (gSndLoad.state) {
    case 0:
        Sound_InitStaging(buf + 0x40000, buf + 0x80000);
        if (quest[0] != 0) {
            id = (gQuestSoundBanks[*(u16 *)(quest + 0x30C)] + 0x39) | 0x10000;
        } else {
            id = 0x1012B;
        }
        Sound_QueueBank(id, buf);
        gSndLoad.state++;
        break;
    case 1:
        if (!FileQueue_IsBusy()) {
            gSndLoad.state++;
            Sound_StageBank((struct SndFile *) buf);
        }
        break;
    case 2:
        if (quest[0] != 0) {
            if (quest[0x11] == 0) {
                if (*(u16 *)(quest + 0x30C) == 0x6F || *(u16 *)(quest + 0x30C) == 0xAA) {
                    id = (gVoiceBanks[0][15] + 0x12C) | 0x10000;
                } else {
                    id = (gVoiceBanks[0][quest[0x82B]] + 0x12C) | 0x10000;
                }
            } else {
                if (*(u16 *)(quest + 0x30C) == 0x6F || *(u16 *)(quest + 0x30C) == 0xAA) {
                    id = (gVoiceBanks[1][15] + 0x12C) | 0x10000;
                } else {
                    id = (gVoiceBanks[1][quest[0x82B]] + 0x12C) | 0x10000;
                }
            }
        } else {
            id = 0x1014C;
        }
        Sound_QueueBank(id, buf + 0x20000);
        gSndLoad.state++;
        break;
    case 3:
        if (!FileQueue_IsBusy()) {
            Sound_StageBank((struct SndFile *) (buf + 0x20000));
            done = 1;
            gSndLoad.state++;
        }
        break;
    default:
        done = 1;
        break;
    }
    return done;
}

/* Bank sets read at once through the sub_main work buffer: slot 1 gets
 * file 0 of afs01.afs, slot 6 file 3. */
void Sound_LoadCommonBanks(void)
{
    void *buf = gSysSettings.loadBuf;

    Sound_StopBank(1);
    FlushCache(0);
    Sound_ReadBank(0x10000, buf);
    Sound_LoadBankWait(buf, 1);
    Sound_StopBank(6);
    FlushCache(0);
    Sound_ReadBank(0x10003, buf);
    Sound_LoadBankWait(buf, 6);
}

void Sound_LoadGmSubBanks(void)
{
    void *buf = gSysSettings.loadBuf;

    Sound_StopBank(1);
    FlushCache(0);
    Sound_ReadBank(0x10002, buf);
    Sound_LoadBankWait(buf, 1);
    Sound_StopBank(6);
    FlushCache(0);
    Sound_ReadBank(0x10009, buf);
    Sound_LoadBankWait(buf, 6);
    Sound_StopBank(7);
    FlushCache(0);
    Sound_ReadBank(0x10008, buf);
    Sound_LoadBankWait(buf, 7);
}

/* The select screen passes its task, which it does not read. */
s32 Sound_LoadSelectBanks(void *task)
{
    void *buf = gSysSettings.loadBuf;

    gSysSettings.soundLoading = 1;
    Sound_StopBank(1);
    FlushCache(0);
    Sound_ReadBank(0x10002, buf);
    Sound_LoadBankWait(buf, 1);
    Sound_StopBank(6);
    FlushCache(0);
    Sound_ReadBank(0x10007, buf);
    Sound_LoadBankWait(buf, 6);
    gSysSettings.soundLoading = 0;
    return 1;
}

void Sound_OnBanksLoaded(void)
{
}
