/*
 * The sound banks' loads, around the sound driver's own code.
 *
 * A quest's scene stages its banks one step per frame through the file
 * queue: each player's weapon and voice banks (SndLoad_PlayerBanks), then
 * the banks of the quest's monsters (SndLoad_EmBanks). The scene's set-up
 * runs the steps and loads each staged set into its slot. The screens
 * read their bank sets at once into the load buffer (the network mode,
 * the title menu, the select screen), and single banks are loaded the
 * same way: a hunter's voice, the PSP link's bank, the quest results' bank
 * and the shared bank as a quest ends. Sound_QueueBank and Sound_ReadBank
 * read one bank file (SND_BANK_FILE: partition 1's file n), through the
 * file queue or at once.
 */
#include "common.h"
#include "syssettings.h"
#include "stream.h"
#include "sound.h"
#include "player.h"
#include "bgm.h"
#include "main.h"
#include "libc.h"
#include "libkernl.h"

/* The steps of a staged load (SndLoad.state); past the last, it is done. */
enum SndLoadStep {
    SNDLOAD_QUEUE_FIRST = 0,    /* queue the first bank file */
    SNDLOAD_WAIT_FIRST = 1,     /* wait for it (a player's: and stage it) */
    SNDLOAD_QUEUE_NEXT = 2,     /* queue the next bank file */
    SNDLOAD_WAIT_NEXT = 3       /* wait for it and stage it */
};

/* The most entries a staged load's list has: the quest's monster slots. */
#define SNDLOAD_LIST_MAX 4

/* The progress of a staged load (gSndLoad; SndLoad_Init starts one). */
typedef struct SndLoad {
    /* 0x0 */ s8 state;                     /* enum SndLoadStep */
    /* 0x1 */ s8 index;                     /* the list's next entry */
    /* 0x2 */ s8 count;                     /* the list's entries */
    /* 0x3 */ s8 kinds[SNDLOAD_LIST_MAX];   /* per list entry, the bank kind staged for it */
} SndLoad;

extern SndLoad gSndLoad;
extern s8 gVoiceBanks[2][VOICE_BANKS_PER_GENDER];   /* per gender and voice, the voice's bank */
extern s16 gWeaponSeBanks[];  /* per weapon kind, its bank file after SND_BANK_WEAPONS */

struct SndFile;
s32 Sound_LoadBankWait(struct SndFile *f, u32 slot);
void Sound_StageBank(struct SndFile *f);

/* Stops every sound that plays. */
void Sound_Silence(void)
{
    Sound_StopAll();
}

/* Starts a staged load of a list of count entries. */
void SndLoad_Init(s8 count)
{
    memset(&gSndLoad, 0, sizeof(SndLoad));
    gSndLoad.count = count;
}

/*
 * Stages the banks of the quest's monsters, one step per call: the
 * monsters' shared bank first, then for each monster kind of the list
 * (gScene.emKind; 0 is an empty slot) the bank of its kind
 * (gEmSeBankFiles), unless a bank of that kind is staged already. Bank
 * kind 38 counts as staged when 28 is, 39 when 29 is, and 27 when 1 is.
 * buf is the load buffer (SNDLOAD_EM_*). Returns 1 once every entry is
 * done.
 */
s32 SndLoad_EmBanks(u8 *buf, u8 *emKinds)
{
    s32 done = 0;
    s32 i;
    s8 bankKind;
    u8 emKind;

    switch (gSndLoad.state) {
    case SNDLOAD_QUEUE_FIRST:
        Sound_InitStaging(buf + SNDLOAD_EM_HEAD, buf + SNDLOAD_EM_BODY);
        Sound_QueueBank(SND_BANK_FILE(SND_BANK_EM), buf);
        gSndLoad.state++;
        break;
    case SNDLOAD_WAIT_FIRST:
        if (!FileQueue_IsBusy()) {
            gSndLoad.state++;
        }
        break;
    case SNDLOAD_QUEUE_NEXT:
        emKind = emKinds[gSndLoad.index];
        if (emKind == 0) {
            if (++gSndLoad.index >= gSndLoad.count) {
                done = 1;
            }
            break;
        }
        bankKind = gEmSeBankFiles[emKind];
        if (bankKind == 0) {
            if (++gSndLoad.index >= gSndLoad.count) {
                done = 1;
            }
            break;
        }
        if (bankKind == 38) {
            for (i = 0; i < SNDLOAD_LIST_MAX; i++) {
                if (bankKind == gSndLoad.kinds[i] || gSndLoad.kinds[i] == 28) {
                    break;
                }
            }
        } else if (bankKind == 39) {
            for (i = 0; i < SNDLOAD_LIST_MAX; i++) {
                if (bankKind == gSndLoad.kinds[i] || gSndLoad.kinds[i] == 29) {
                    break;
                }
            }
        } else if (bankKind == 27) {
            for (i = 0; i < SNDLOAD_LIST_MAX; i++) {
                if (bankKind == gSndLoad.kinds[i] || gSndLoad.kinds[i] == 1) {
                    break;
                }
            }
        } else {
            for (i = 0; i < SNDLOAD_LIST_MAX; i++) {
                if (bankKind == gSndLoad.kinds[i]) {
                    break;
                }
            }
        }
        if (i < SNDLOAD_LIST_MAX) {
            /* staged already */
            if (++gSndLoad.index >= gSndLoad.count) {
                done = 1;
            }
            break;
        }
        gSndLoad.kinds[gSndLoad.index] = bankKind;
        Sound_QueueBank(SND_BANK_FILE(bankKind + SND_BANK_EM), buf + SNDLOAD_EM_FILE);
        gSndLoad.state++;
        break;
    case SNDLOAD_WAIT_NEXT:
        if (!FileQueue_IsBusy()) {
            Sound_StageBank((struct SndFile *) (buf + SNDLOAD_EM_FILE));
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

/*
 * Stages a player's two banks in a quest, one step per call: the bank of
 * the player's weapon kind (gWeaponSeBanks), then the voice bank of the
 * hunter's gender and voice. Weapon kinds 111 and 170 take VOICE_EXTRA
 * instead of the hunter's own voice. An empty player slot stages
 * SND_BANK_NO_WEAPON and SND_BANK_NO_VOICE. buf is the load buffer
 * (SNDLOAD_PL_*). Returns 1 once both are staged.
 */
s32 SndLoad_PlayerBanks(u8 *buf, Player *pl)
{
    s32 done = 0;
    u32 id;

    switch (gSndLoad.state) {
    case SNDLOAD_QUEUE_FIRST:
        Sound_InitStaging(buf + SNDLOAD_PL_HEAD, buf + SNDLOAD_PL_BODY);
        if (pl->active != 0) {
            id = SND_BANK_FILE(gWeaponSeBanks[pl->weaponKind] + SND_BANK_WEAPONS);
        } else {
            id = SND_BANK_FILE(SND_BANK_NO_WEAPON);
        }
        Sound_QueueBank(id, buf);
        gSndLoad.state++;
        break;
    case SNDLOAD_WAIT_FIRST:
        if (!FileQueue_IsBusy()) {
            gSndLoad.state++;
            Sound_StageBank((struct SndFile *) buf);
        }
        break;
    case SNDLOAD_QUEUE_NEXT:
        if (pl->active != 0) {
            if (pl->gender == 0) {
                if (pl->weaponKind == 111 || pl->weaponKind == 170) {
                    id = SND_BANK_FILE(gVoiceBanks[0][VOICE_EXTRA] + SND_BANK_QUEST_VOICES);
                } else {
                    id = SND_BANK_FILE(gVoiceBanks[0][pl->voice] + SND_BANK_QUEST_VOICES);
                }
            } else {
                if (pl->weaponKind == 111 || pl->weaponKind == 170) {
                    id = SND_BANK_FILE(gVoiceBanks[1][VOICE_EXTRA] + SND_BANK_QUEST_VOICES);
                } else {
                    id = SND_BANK_FILE(gVoiceBanks[1][pl->voice] + SND_BANK_QUEST_VOICES);
                }
            }
        } else {
            id = SND_BANK_FILE(SND_BANK_NO_VOICE);
        }
        Sound_QueueBank(id, buf + SNDLOAD_PL_VOICE);
        gSndLoad.state++;
        break;
    case SNDLOAD_WAIT_NEXT:
        if (!FileQueue_IsBusy()) {
            Sound_StageBank((struct SndFile *) (buf + SNDLOAD_PL_VOICE));
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

/*
 * The network mode's banks, read at once into the load buffer: the shared
 * bank into its slot and the network mode's bank into the monsters' slot
 * (NetMode_ReloadFrame, NetMode_BgmOn).
 */
void Sound_LoadNetModeBanks(void)
{
    void *buf = gSysSettings.loadBuf;

    Sound_StopBank(SE_SLOT_COMMON);
    FlushCache(0);
    Sound_ReadBank(SND_BANK_FILE(SND_BANK_COMMON), buf);
    Sound_LoadBankWait(buf, SE_SLOT_COMMON);
    Sound_StopBank(SE_SLOT_EM);
    FlushCache(0);
    Sound_ReadBank(SND_BANK_FILE(SND_BANK_NET_MODE), buf);
    Sound_LoadBankWait(buf, SE_SLOT_EM);
}

/*
 * The title menu's banks, read at once (GmSub_InitMenu): the menus' bank
 * into the shared slot, and the title menu's two banks into the monsters'
 * and the scene's slots.
 */
void Sound_LoadTitleMenuBanks(void)
{
    void *buf = gSysSettings.loadBuf;

    Sound_StopBank(SE_SLOT_COMMON);
    FlushCache(0);
    Sound_ReadBank(SND_BANK_FILE(SND_BANK_MENUS), buf);
    Sound_LoadBankWait(buf, SE_SLOT_COMMON);
    Sound_StopBank(SE_SLOT_EM);
    FlushCache(0);
    Sound_ReadBank(SND_BANK_FILE(SND_BANK_TITLE_MENU), buf);
    Sound_LoadBankWait(buf, SE_SLOT_EM);
    Sound_StopBank(SE_SLOT_SCENE);
    FlushCache(0);
    Sound_ReadBank(SND_BANK_FILE(SND_BANK_TITLE_MENU_SCENE), buf);
    Sound_LoadBankWait(buf, SE_SLOT_SCENE);
}

/*
 * The select screen's banks, read at once: the menus' bank into the
 * shared slot and the select screen's into the monsters' slot. The select
 * screen passes its task, which is not read. Returns 1.
 */
s32 Sound_LoadSelectBanks(void *task)
{
    void *buf = gSysSettings.loadBuf;

    gSysSettings.soundLoading = 1;
    Sound_StopBank(SE_SLOT_COMMON);
    FlushCache(0);
    Sound_ReadBank(SND_BANK_FILE(SND_BANK_MENUS), buf);
    Sound_LoadBankWait(buf, SE_SLOT_COMMON);
    Sound_StopBank(SE_SLOT_EM);
    FlushCache(0);
    Sound_ReadBank(SND_BANK_FILE(SND_BANK_SELECT), buf);
    Sound_LoadBankWait(buf, SE_SLOT_EM);
    gSysSettings.soundLoading = 0;
    return 1;
}

/* Empty: Plsel_SelectWaitSound calls it once the select screen's banks are
 * loaded. */
void Sound_OnBanksLoaded(void)
{
}

/*
 * Loads the voice bank of a hunter into player 0's sound slot: bank
 * SND_BANK_VOICES + gVoiceBanks[gender][voice] (the select overlay passes
 * the character's gender and voice when the voice is changed in the
 * character editor). Returns 1. The table is indexed as one list,
 * gender * VOICE_BANKS_PER_GENDER + voice, as the game's code adds it up:
 * gVoiceBanks[gender][voice] adds in another order.
 */
s32 Sound_LoadVoiceBank(s32 gender, s32 voice)
{
    void *buf = gSysSettings.loadBuf;

    gSysSettings.soundLoading = 1;
    Sound_StopBank(SE_SLOT_PLAYER(0));
    FlushCache(0);
    Sound_ReadBank(SND_BANK_FILE(gVoiceBanks[0][gender * VOICE_BANKS_PER_GENDER + voice] + SND_BANK_VOICES), buf);
    Sound_LoadBankWait(buf, SE_SLOT_PLAYER(0));
    gSysSettings.soundLoading = 0;
    return 1;
}

/* Loads the PSP link's bank into the monsters' slot (the select overlay's
 * PSP link, once the character is chosen). */
void Sound_LoadPspBank(void)
{
    void *buf = gSysSettings.loadBuf;

    gSysSettings.soundLoading = 1;
    Sound_StopBank(SE_SLOT_EM);
    FlushCache(0);
    Sound_ReadBank(SND_BANK_FILE(SND_BANK_PSP), buf);
    Sound_LoadBankWait(buf, SE_SLOT_EM);
    gSysSettings.soundLoading = 0;
}

/* Loads the quest results' bank into the scene's slot (the results'
 * set-up calls it last). */
void Sound_LoadResultsBank(void)
{
    void *buf = gSysSettings.loadBuf;

    gSysSettings.soundLoading = 1;
    Sound_StopBank(SE_SLOT_SCENE);
    FlushCache(0);
    Sound_ReadBank(SND_BANK_FILE(SND_BANK_RESULTS), buf);
    Sound_LoadBankWait(buf, SE_SLOT_SCENE);
    gSysSettings.soundLoading = 0;
}

/* Loads the shared bank again into its slot (Quest_Teardown, as a quest
 * ends). Unlike the others it leaves soundLoading alone. */
void Sound_ReloadCommonBank(void)
{
    void *buf = gSysSettings.loadBuf;

    Sound_StopBank(SE_SLOT_COMMON);
    FlushCache(0);
    Sound_ReadBank(SND_BANK_FILE(SND_BANK_COMMON), buf);
    Sound_LoadBankWait(buf, SE_SLOT_COMMON);
}

/* Reads bank file id into buf through the file queue (a step of the
 * scene's staged loads). */
void Sound_QueueBank(u32 id, void *buf)
{
    FileQueue_AddFile(id, buf);
}

/* Reads bank file id into buf at once; returns Afs_ReadId's result. */
s32 Sound_ReadBank(u32 id, void *buf)
{
    return Afs_ReadId(id, buf);
}
