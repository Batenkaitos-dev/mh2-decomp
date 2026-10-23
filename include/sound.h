#ifndef SOUND_H
#define SOUND_H
#include "em.h"
#include "player.h"
#include "cse.h"

/*
 * The game's sound ids. A sound effect is played by a slot (its bank) and
 * an id within that bank: Sound_PlaySeFlat and its
 * siblings take both. Slot 0 is the system bank, which the menus use.
 */

/*
 * The banks (the slot argument), as the sound wrappers use
 * them: each player plays in its own bank, the monsters in theirs, and
 * the shared bank holds the sounds of sub_main's menu shortcuts
 * (SE_MENU_*), the armour rattles and the common sounds of players and
 * monsters (Sound_PlayPlayerSe, Sound_PlayEmSe).
 */
#define SE_SLOT_SYSTEM      0           /* the menus' SE_SYS_* */
#define SE_SLOT_COMMON      1           /* the shared bank */
#define SE_SLOT_PLAYER(i)   ((i) + 2)   /* player i's own bank (Player.index 0-3) */
#define SE_SLOT_EM          6           /* the monsters' bank */
/*
 * The scene's own bank. Seen: the area's objects, mounted weapons and
 * scenery, Motion_VoiceAtFrame's voices and footsteps (id
 * voice * 2 or voice * 2 + 1 at random, with the floor's sound argument),
 * the music's track change, and the menu screens' own sounds (the
 * character select, the quest results).
 */
#define SE_SLOT_SCENE       7

/* The id that plays nothing: the wrappers skip it, or return -1. */
#define SE_ID_NONE          0xFFFF

/*
 * Slot 0's sounds, as the menus play them (Sound_PlaySeFlat(0, id, 0)).
 * Named from the button each follows: SE_SYS_CONFIRM after PAD_CIRCLE,
 * SE_SYS_CANCEL after PAD_CROSS, SE_SYS_CURSOR after the D-pad; the others
 * from what the menus do when they play them.
 */
#define SE_SYS_DESCRIPTION 0x09 /* a description window opens (the item box's and the equipment menus' square) */
#define SE_SYS_NOTICE 0x10  /* a notice banner appears */
#define SE_SYS_OPEN 0x11    /* a window opens or a setting toggles */
#define SE_SYS_CURSOR 0x12  /* the cursor moves */
#define SE_SYS_CONFIRM 0x13 /* a choice is confirmed */
#define SE_SYS_CANCEL 0x14  /* back, or a window closes */
#define SE_SYS_REFUSE 0x15  /* the choice can't be taken */
#define SE_SYS_SELECT 0x16  /* a key is typed or a key set switches (the keyboard); a grid cursor steps */
#define SE_SYS_SCROLL 0x17  /* a page, a count or a yes/no choice changes */
#define SE_SYS_SEND 0x18    /* a text is submitted (Kbd_Submit); the chat's beep */
#define SE_SYS_DONE 0x19    /* an action completes: a card load or save ends, an item goes into the pouch, an equipment set is applied */

/*
 * Slot 1's menu sounds, which sub_main's shortcuts play:
 * the title and option menus (gm_sub, select) and the character select.
 */
#define SE_MENU_SLOT_CHANGE 0x72 /* the character select's cursor moves to another save */
#define SE_MENU_CONFIRM 0x73     /* a choice is confirmed */
#define SE_MENU_CURSOR 0x78      /* the cursor moves: the first of four variants, picked at random */

/*
 * The sound bank files (Sound_QueueBank, Sound_ReadBank): the file's id is
 * its partition (1, the sound banks) << 16 | its index there.
 */
#define SND_BANK_FILE(n)    ((n) | 0x10000)
#define SND_BANK_VOICES     0x17D   /* + gVoiceBanks[sex * 16 + voice]: a hunter's voice (slot 2) */
#define SND_BANK_AREAS      0xB     /* + gAreaSeBanks[area]: an area's own bank (SE_SLOT_SCENE) */
#define SND_BANK_TOWN       4       /* the town's bank offline (slot 6, the monsters' slot) */
#define SND_BANK_TOWN_NET   5       /* online */
#define SND_BANK_HOUSE      6       /* in AREA_HOUSE and AREA_HUB_0x57 */
#define SND_BANK_COMMON     0       /* the shared bank (SE_SLOT_COMMON), loaded again as a quest ends */
#define SND_BANK_PSP        10      /* the PSP link's bank (slot 6, the monsters' slot) */
#define SND_BANK_RESULTS    45      /* the quest results' bank (SE_SLOT_SCENE), loaded by their set-up */
#define SND_BANK_MENUS      2       /* the shared slot's bank on the title menu and select screens (SE_SLOT_COMMON) */
#define SND_BANK_NET_MODE   3       /* the network mode's bank (slot 6), with the shared bank */
#define SND_BANK_SELECT     7       /* the select screen's bank (slot 6) */
#define SND_BANK_TITLE_MENU_SCENE 8 /* the title menu's bank in SE_SLOT_SCENE */
#define SND_BANK_TITLE_MENU 9       /* the title menu's bank (slot 6) */
#define SND_BANK_WEAPONS    57      /* + gWeaponSeBanks[weaponKind]: a player's weapon bank in a quest */
#define SND_BANK_NO_WEAPON  299     /* the weapon bank staged for an empty player slot */
#define SND_BANK_QUEST_VOICES 300   /* + gVoiceBanks[gender][voice]: a hunter's voice in a quest */
#define VOICE_BANKS_PER_GENDER 16   /* gVoiceBanks' row: the hunter's voices, then VOICE_EXTRA */
#define VOICE_EXTRA         15      /* the voice past the hunter's choices: weapon kinds 111 and 170 take it in a quest */
#define SND_BANK_NO_VOICE   332     /* the voice bank staged for an empty player slot */
#define SND_BANK_EM         333     /* the monsters' shared bank; + gEmSeBankFiles[kind]: a monster kind's own */

/*
 * The scene's staged bank loads (SndLoad_PlayerBanks, SndLoad_EmBanks):
 * where in the load buffer the files are read, and where their bank
 * headers and sample data are staged.
 */
#define SNDLOAD_PL_VOICE    0x20000 /* a player's voice bank file (its weapon bank's at 0) */
#define SNDLOAD_PL_HEAD     0x40000 /* a player's staged bank headers */
#define SNDLOAD_PL_BODY     0x80000 /* and sample data */
#define SNDLOAD_EM_FILE     0x4800  /* each monster kind's bank file (the shared one's at 0) */
#define SNDLOAD_EM_HEAD     0xB4800 /* the monsters' staged bank headers */
#define SNDLOAD_EM_BODY     0xD4800 /* and sample data */

/* gBgmId: the music playing; this value when none is. */
#define BGM_NONE 0xFF

void Sound_Init(void);
void Sound_InitBuffers(void);
void Sound_LoadSpuMap(void);
s32 Sound_Update(void);
s32 Sound_LoadBankFile(s32 data, s32 slot);
s32 Sound_CheckTransfer(void);
s32 Sound_SetSpuMapPage(u32 page);
s32 Sound_SetEffect(s32 core, s32 mode, s32 depth, s32 delay, s32 feedback); /* main */
s32 Sound_StopAll(void);
s32 Sound_StopBank(s32 slot);
void Sound_SetOutputMode(s32 mode);
void Sound_SetSeIds(u32 slot, u32 id, u8 ownerType, u8 ownerSlot, s32 table);
s32 Sound_PlaySeVolPan(u32 slot, u32 id, s32 vol, s32 pan, s32 unused, s32 prog);
s32 Sound_PlaySe(u32 slot, u32 id, CseOpt *opt);
s32 Sound_PlaySePos(u32 slot, u32 id, s32 prog, void *pos, s32 kind, s32 update);
s32 Sound_PlaySeAtPos(u32 slot, u32 id, s32 prog, void *pos, s32 kind, s32 update);
void Sound_InitStaging(u8 *headBase, u8 *bodyBase);
void Sound_Silence(void);
void SndLoad_Init(s8 count);
s32 SndLoad_EmBanks(u8 *buf, u8 *emKinds);
s32 SndLoad_PlayerBanks(u8 *buf, Player *pl);
void Sound_LoadNetModeBanks(void);
void Sound_LoadTitleMenuBanks(void);
s32 Sound_LoadSelectBanks(void *task);
void Sound_OnBanksLoaded(void);
s32 Sound_LoadVoiceBank(s32 gender, s32 voice);
void Sound_LoadPspBank(void);
void Sound_LoadResultsBank(void);
void Sound_ReloadCommonBank(void);
void Sound_QueueBank(u32 id, void *buf);
s32 Sound_ReadBank(u32 id, void *buf); /* loads a file by id */

void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void Sound_PlaySeFlatPan(u32 slot, u32 id, s32 prog, s32 pan);
void Sound_PlaySeMusicVol(u32 slot, u32 id, s32 prog);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 prog, void *pos, s32 range, s32 update);
s32 Sound_RandomSe(s32 id, s16 idWeight, s32 alt, s16 altWeight);
void Sound_PlayPlayerSe(void *obj, s32 id, s32 prog, void *pos, s32 range, s32 update);
void Sound_PlayArmorRattle(Player *pl, s32 kind);
void Player_PlaySe(Player *pl, s32 id, s32 prog, void *pos, s32 range, s32 update);
s32 Sound_PlayEmSe(Em *em, s32 id, s32 prog, void *pos, s32 range, s32 update);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 prog, void *pos, s32 range, s32 update);
s32 Em_PlaySeAtPos(Em *em, s32 id, s32 prog, void *pos, s32 range, s32 update);
s32 Em_PlaySeType(Em *em, s32 id, s32 prog, void *pos, s32 range, s32 update);
void Em_PlayEmBankSe(Em *em, s32 id, void *pos, s32 range);
void Em_PlayCommonSe(Em *em, s32 id, void *pos, s32 range);
extern f32 gSndVolumeScale[8]; /* the SE volume settings */

#endif /* SOUND_H */
