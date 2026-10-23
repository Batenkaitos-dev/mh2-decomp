#ifndef TALK_SCRIPTS_H
#define TALK_SCRIPTS_H

#include "common.h"

/*
 * The talk scripts' choices and the talk's end: Talk_Check tests an event
 * step of a script's events through the script's own check, and Talk_Finish
 * ends a talk: it gives the line's gift, sets the event's flag, runs the
 * event's action and opens what the townsperson offers, or closes the talk.
 */

s32 Talk_Finish(void);
s32 Talk_Check(u16 script, u16 step);

#endif
