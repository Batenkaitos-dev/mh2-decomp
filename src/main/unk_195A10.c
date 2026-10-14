/*
 * main 0x195a10-0x195c00: more sound bank loads (see unk_195230.c), and
 * the two readers of bank files (id: partition << 16 | file).
 */
#include "common.h"
#include "syssettings.h"
#include "stream.h"

/* sub_main system settings (0x6ADF90): its work buffer, and the flag set
 * while banks load */

extern s8 gVoiceBanks[];     /* sub_main: voice banks, 16 per sex */

void FlushCache(s32 op);
s32 Sound_StopBank(s32 slot);
struct SndFile;
s32 Sound_LoadBankWait(struct SndFile *f, u32 slot);
s32 Afs_ReadId(u32 id, void *dst);
s32 Sound_ReadBank(u32 id, void *buf);

/* Loads the voice bank of a hunter into sound slot 2: bank 0x17D +
 * gVoiceBanks[sex][voice] (the select overlay passes the character's u8 sex
 * and voice when the voice is changed in the character editor). */
s32 Sound_LoadVoiceBank(s32 sex, s32 voice)
{
    void *buf = gSysSettings.loadBuf;

    gSysSettings.soundLoading = 1;
    Sound_StopBank(2);
    FlushCache(0);
    Sound_ReadBank((gVoiceBanks[sex * 16 + voice] + 0x17D) | 0x10000, buf);
    Sound_LoadBankWait(buf, 2);
    gSysSettings.soundLoading = 0;
    return 1;
}

void Sound_LoadPspBank(void)
{
    void *buf = gSysSettings.loadBuf;

    gSysSettings.soundLoading = 1;
    Sound_StopBank(6);
    FlushCache(0);
    Sound_ReadBank(0x1000A, buf);
    Sound_LoadBankWait(buf, 6);
    gSysSettings.soundLoading = 0;
}

void Sound_ReloadBank7(void)
{
    void *buf = gSysSettings.loadBuf;

    gSysSettings.soundLoading = 1;
    Sound_StopBank(7);
    FlushCache(0);
    Sound_ReadBank(0x1002D, buf);
    Sound_LoadBankWait(buf, 7);
    gSysSettings.soundLoading = 0;
}

void Sound_ReloadBank1(void)
{
    void *buf = gSysSettings.loadBuf;

    Sound_StopBank(1);
    FlushCache(0);
    Sound_ReadBank(0x10000, buf);
    Sound_LoadBankWait(buf, 1);
}

/* Through the file queue (a step of SndLoad_*). */
void Sound_QueueBank(u32 id, void *buf)
{
    FileQueue_AddFile(id, buf);
}

/* At once. */
s32 Sound_ReadBank(u32 id, void *buf)
{
    return Afs_ReadId(id, buf);
}
