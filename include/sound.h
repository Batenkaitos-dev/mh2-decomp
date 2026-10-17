#ifndef SOUND_H
#define SOUND_H

/*
 * The game's sound ids. A sound effect is played by a slot (its bank) and
 * an id within that bank: Sound_PlaySeFlat (src/main/unk_194830.c) and its
 * siblings take both. Slot 0 is the system bank, which the menus use.
 */

/*
 * The banks (the slot argument), as the wrappers in unk_194830.c use
 * them: each player plays in its own bank, the monsters in theirs, and
 * the shared bank holds the sounds of sub_main's menu shortcuts
 * (SE_MENU_*), the armour rattles and the common sounds of players and
 * monsters (Sound_PlayPlayerSe, Sound_PlayEmSe).
 */
#define SE_SLOT_SYSTEM      0           /* the menus' SE_SYS_* */
#define SE_SLOT_COMMON      1           /* the shared bank */
#define SE_SLOT_PLAYER(i)   ((i) + 2)   /* player i's own bank (Player.index 0-3) */
#define SE_SLOT_EM          6           /* the monsters' bank */

/* The id that plays nothing: the wrappers skip it, or return -1. */
#define SE_ID_NONE          0xFFFF

/*
 * Slot 0's sounds, as the menus play them (Sound_PlaySeFlat(0, id, 0)).
 * Named from the button each follows: SE_SYS_CONFIRM after PAD_CIRCLE,
 * SE_SYS_CANCEL after PAD_CROSS, SE_SYS_CURSOR after the D-pad; the others
 * from what the menus do when they play them.
 */
#define SE_SYS_OPEN 0x11    /* a window opens or a setting toggles */
#define SE_SYS_CURSOR 0x12  /* the cursor moves */
#define SE_SYS_CONFIRM 0x13 /* a choice is confirmed */
#define SE_SYS_CANCEL 0x14  /* back, or a window closes */
#define SE_SYS_REFUSE 0x15  /* the choice can't be taken */
#define SE_SYS_SELECT 0x16  /* a key is typed or a key set switches (the keyboard); a grid cursor steps */
#define SE_SYS_SCROLL 0x17  /* a page, a count or a yes/no choice changes */
#define SE_SYS_SEND 0x18    /* a text is submitted (Kbd_Submit); the chat's beep */

/*
 * Slot 1's menu sounds, which sub_main's shortcuts play (unk_36B5B0.c):
 * the title and option menus (gm_sub, select) and the character select.
 */
#define SE_MENU_SLOT_CHANGE 0x72 /* the character select's cursor moves to another save */
#define SE_MENU_CONFIRM 0x73     /* a choice is confirmed */
#define SE_MENU_CURSOR 0x78      /* the cursor moves: the first of four variants, picked at random */

/* gBgmId (rand.c): the music playing; this value when none is. */
#define BGM_NONE 0xFF

#endif /* SOUND_H */
