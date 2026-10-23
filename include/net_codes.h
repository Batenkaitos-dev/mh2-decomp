#ifndef NET_CODES_H
#define NET_CODES_H

#include "common.h"

/*
 * Two encodings the lobby's packets use: the scrambling of a packet's bytes,
 * and the answer to the lobby server's challenge (Net_ChallengeReply), built
 * from the account id and a key.
 */

s32 Net_ChallengeReply(char *out, s8 *challenge, u16 key);

#endif
