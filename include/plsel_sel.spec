# The work area of plsel.c's character select (0x13DC bytes, cleared by
# Plsel_InitSelWork), reached through the same pointer at 0x6AE018 as PlselWork.
# Regenerate with: python tools/gen_struct.py include/plsel_sel.spec include/plsel.h
name PlselSelWork
size 0x13DC
0x0000 u8 kbdOpen // 1 while the chat keyboard is open (Plsel_ChatOpenKbd, Plsel_ChatUpdate)
0x0001 u8 logHead // the next chat log entry (wraps at 64; Plsel_AddChatLog)
0x0002 u8 logCount // chat log entries (at most 64)
0x0004 s32 sendWait // frames before the next message may be sent (60 after one; Kbd_SetSubmitWait)
0x13D8 s16 logShowTimer // frames the chat log stays shown (300 after a message)
0x0008 u8 logScroll // counts messages added past 11 log lines (at most 63)
0x000B ChatLine{79} log[64] // the chat log, a ring (logHead, logCount)
0x13D4 u8 newMsg // 1: a message was sent or came in
0x13D5 u8 newMsgMine // 1: it was the local player's
