#ifndef AVE_H
#define AVE_H

#include "common.h"

/* NETWORK: the connection settings sent to the AVE TCP wrapper by
 * AvePpp_Connect (src/sub_main/ave_wrap.c), built by NetPpp_Connect
 * (src/sub_main/net_tcp.c). */
typedef struct AveConfig {
    u8 unk0;
    u8 unk1;
    s16 unk2;
    char *str4;
    char *str8;
    char *strC;
    char *str10;
    char *str14;
    char *str18;
    s16 unk1C;
    s16 unk1E;
    s16 unk20;
    s16 unk22;
    s16 unk24;
    u8 unk26;
    u8 unk27;
} AveConfig;

#endif /* AVE_H */
