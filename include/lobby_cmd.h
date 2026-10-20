#ifndef LOBBY_CMD_H
#define LOBBY_CMD_H

/* The entries of the command tables (gLobbyCmdHi/Lo/Kind/Flag) that the
 * client sends: LobbyPkt_BeginCmd takes the index and writes the entry's
 * code and flag. Each comment gives the code on the wire. _ANSWER entries
 * answer a request the server sends; _NEXT ones ask for a search's next page. */
#define LOBBY_CMD_KEEPALIVE_ANSWER 0x1      /* 6001: the client's answer to the server's request */
#define LOBBY_CMD_LOBBY_LEAVE 0x2           /* 6002: the client's request */
#define LOBBY_CMD_LOGOUT 0x4                /* 6006: the client's request */
#define LOBBY_CMD_PING 0xa                  /* 600E: the client's request */
#define LOBBY_CMD_LOGIN_AUTH_ANSWER 0xd     /* 6101: the client's answer to the server's request */
#define LOBBY_CMD_CLIENT_INFO_ANSWER 0xf    /* 6103: the client's answer to the server's request */
#define LOBBY_CMD_LOGIN_NOTICE_ANSWER 0x12  /* 6105: the client's answer to the server's request */
#define LOBBY_CMD_CHARA_SELECT 0x14         /* 6132: the client's request */
#define LOBBY_CMD_6138_ANSWER 0x17          /* 6138: the client's answer to the server's request, its meaning unknown */
#define LOBBY_CMD_LOGIN_READY 0x18          /* 6141: the client's notification */
#define LOBBY_CMD_LOGIN_INFO 0x1b           /* 614C: the client's request */
#define LOBBY_CMD_HUNTER_CARD 0x1d          /* 6190: the client's request */
#define LOBBY_CMD_HUNTER_CARD_PART 0x20     /* 6192: the client's request */
#define LOBBY_CMD_LOGIN_PARAMS 0x23         /* 61A0: the client's request */
#define LOBBY_CMD_QNET_TIMEOUTS 0x25        /* 61A1: the client's request */
#define LOBBY_CMD_QUEST_RECORD 0x27         /* 6139: the client's request */
#define LOBBY_CMD_CHARA_SELECT_TEXT 0x29    /* 614D: the client's request */
#define LOBBY_CMD_MEMORY_CHECK_ANSWER 0x2c  /* 614A: the client's answer to the server's memory check */
#define LOBBY_CMD_ENTER_LEVELS 0x39         /* 6209: the client's request */
#define LOBBY_CMD_LEVEL0_COUNT 0x3d         /* 6203: the client's request */
#define LOBBY_CMD_LEVEL0_NAME 0x3f          /* 6204: the client's request */
#define LOBBY_CMD_LEVEL0_VALUES 0x41        /* 6205: the client's request */
#define LOBBY_CMD_LEVEL0_STATE 0x44         /* 6206: the client's request */
#define LOBBY_CMD_LEVEL0_TEXT 0x47          /* 620A: the client's request */
#define LOBBY_CMD_LEVEL0_LEAVE 0x4a         /* 6306: the client's request */
#define LOBBY_CMD_LEVEL0_KIND 0x4d          /* 620B: the client's request */
#define LOBBY_CMD_LEVEL1_ENTER 0x50         /* 6305: the client's request */
#define LOBBY_CMD_LEVEL1_LEAVE 0x52         /* 6408: the client's request */
#define LOBBY_CMD_LEVEL1_COUNT 0x54         /* 6301: the client's request */
#define LOBBY_CMD_LEVEL1_NAME 0x56          /* 6302: the client's request */
#define LOBBY_CMD_LEVEL1_VALUES 0x59        /* 6303: the client's request */
#define LOBBY_CMD_LEVEL1_STATE 0x5c         /* 6304: the client's request */
#define LOBBY_CMD_LEVEL1_TEXT 0x5f          /* 6308: the client's request */
#define LOBBY_CMD_LEVEL1_OPEN 0x63          /* 6310: the client's request */
#define LOBBY_CMD_LEVEL1_NAME_ALLOWED 0x65  /* 6312: the client's request */
#define LOBBY_CMD_LEVEL1_PASS_ALLOWED 0x67  /* 6313: the client's request */
#define LOBBY_CMD_LEVEL1_TEXT_ALLOWED 0x69  /* 6314: the client's request */
#define LOBBY_CMD_LEVEL1_SET_NAME 0x6b      /* 6315: the client's request */
#define LOBBY_CMD_LEVEL1_SET_PASS 0x6d      /* 6316: the client's request */
#define LOBBY_CMD_LEVEL1_SETTINGS_END 0x6f  /* 6311: the client's request */
#define LOBBY_CMD_LEVEL1_SET_TEXT 0x71      /* 6317: the client's request */
#define LOBBY_CMD_630B 0x73                 /* 630B: the client's request, its meaning unknown */
#define LOBBY_CMD_LEVEL1_SET_COLOR 0x76     /* 630C: the client's request */
#define LOBBY_CMD_LEVEL1_COLOR 0x78         /* 630D: the client's request */
#define LOBBY_CMD_LEVEL1_GROUP_INFO 0x7b    /* 631B: the client's request */
#define LOBBY_CMD_LEVEL1_USERS 0x7e         /* 630A: the client's request */
#define LOBBY_CMD_LEVEL1_KIND 0x82          /* 630E: the client's request */
#define LOBBY_CMD_LEVEL2_ENTER 0x85         /* 6406: the client's request */
#define LOBBY_CMD_LEVEL2_LEAVE 0x87         /* 6501: the client's request */
#define LOBBY_CMD_LEVEL2_COUNT 0x8a         /* 6401: the client's request */
#define LOBBY_CMD_LEVEL2_NAME 0x8c          /* 6402: the client's request */
#define LOBBY_CMD_LEVEL2_VALUES 0x8f        /* 6403: the client's request */
#define LOBBY_CMD_LEVEL2_STATE 0x92         /* 6404: the client's request */
#define LOBBY_CMD_LEVEL2_TEXT 0x95          /* 640D: the client's request */
#define LOBBY_CMD_LEVEL2_COLOR 0x98         /* 650A: the client's request */
#define LOBBY_CMD_LEVEL2_POSTED_QUEST 0x9b  /* 650C: the client's request */
#define LOBBY_CMD_LEVEL2_LOCKED 0x9e        /* 6405: the client's request */
#define LOBBY_CMD_LEVEL2_USERS 0xa1         /* 640A: the client's request */
#define LOBBY_CMD_LEVEL2_LIMITS 0xa3        /* 640B: the client's request */
#define LOBBY_CMD_LEVEL2_OPEN 0xa8          /* 6407: the client's request */
#define LOBBY_CMD_LEVEL2_NAME_EDITABLE 0xaa /* 6601: the client's request */
#define LOBBY_CMD_LEVEL2_PASS_EDITABLE 0xac /* 6602: the client's request */
#define LOBBY_CMD_LEVEL2_TEXT_EDITABLE 0xae /* 660F: the client's request */
#define LOBBY_CMD_LEVEL2_REC_COUNT 0xb0     /* 6603: the client's request */
#define LOBBY_CMD_LEVEL2_REC_NAME 0xb2      /* 6604: the client's request */
#define LOBBY_CMD_LEVEL2_REC_EDITABLE 0xb4  /* 6605: the client's request */
#define LOBBY_CMD_LEVEL2_REC_DEFAULT 0xb6   /* 6606: the client's request */
#define LOBBY_CMD_LEVEL2_REC_CHOICE_COUNT 0xb8/* 6607: the client's request */
#define LOBBY_CMD_LEVEL2_REC_CHOICE_NAME 0xba/* 6608: the client's request */
#define LOBBY_CMD_LEVEL2_REC_CHOICE_VALUES 0xbc/* 660E: the client's request */
#define LOBBY_CMD_LEVEL2_SET_NAME 0xbe      /* 6609: the client's request */
#define LOBBY_CMD_LEVEL2_SET_PASS 0xc0      /* 660A: the client's request */
#define LOBBY_CMD_LEVEL2_SET_TEXT 0xc2      /* 6610: the client's request */
#define LOBBY_CMD_LEVEL2_SET_REC_CHOICE 0xc4/* 660B: the client's request */
#define LOBBY_CMD_LEVEL2_SETTINGS_END 0xc6  /* 660C: the client's request */
#define LOBBY_CMD_QUEST_ACCEPT 0xce         /* 6509: the client's request */
#define LOBBY_CMD_QUEST_POST 0xd0           /* 650B: the client's request */
#define LOBBY_CMD_LEVEL2_KIND 0xd4          /* 6418: the client's request */
#define LOBBY_CMD_SET_OUT 0xd7              /* 6504: the client's request */
#define LOBBY_CMD_LEVEL2_MEMBERS 0xd9       /* 6412: the client's request */
#define LOBBY_CMD_QUEST_PAID 0xdd           /* 6508: the client's notification */
#define LOBBY_CMD_PARTY_COUNT 0xdf          /* 6911: the client's request */
#define LOBBY_CMD_PARTY_SELF 0xe1           /* 6912: the client's request */
#define LOBBY_CMD_PARTY_TEXT 0xe3           /* 6914: the client's request */
#define LOBBY_CMD_PARTY_MEMBER 0xe5         /* 6913: the client's request */
#define LOBBY_CMD_PARTY_MEMBER_VALUES 0xe7  /* 6917: the client's request */
#define LOBBY_CMD_PARTY_FAIL 0xe9           /* 6918: the client's notification */
#define LOBBY_CMD_PARTY_ROOM 0xea           /* 6915: the client's request */
#define LOBBY_CMD_PARTY_SERVER 0xec         /* 6916: the client's request */
#define LOBBY_CMD_PATCH_ACK_ANSWER 0xfe     /* 6124: the client's answer to the server's request */
#define LOBBY_CMD_PATCH_END_ANSWER 0x100    /* 6125: the client's answer to the server's request */
#define LOBBY_CMD_HTTP_GET 0x101            /* 6801: the client's request: an lbs:// page (HttpLbs_Send) */
#define LOBBY_CMD_HTTP_POST 0x102           /* 6802: the client's request: the same with a form's fields, obfuscated */
#define LOBBY_CMD_DL_LIST 0x104             /* 6881: the client's request */
#define LOBBY_CMD_DL_DATA 0x106             /* 6882: the client's request */
#define LOBBY_CMD_DL_FILE_INFO 0x108        /* 6883: the client's request */
#define LOBBY_CMD_SAVE_SUMMARY_BEGIN 0x10b  /* 6148: the client's request */
#define LOBBY_CMD_SAVE_SUMMARY_PART 0x10d   /* 6149: the client's request */
#define LOBBY_CMD_TOWN_DATA 0x10f           /* 6884: the client's request */
#define LOBBY_CMD_CURRENT_PLACE 0x115       /* 6891: the client's request */
#define LOBBY_CMD_TOWN_CLOCK 0x117          /* 6892: the client's request */
#define LOBBY_CMD_CHAT_TELL 0x121           /* 670B: the client's request */
#define LOBBY_CMD_TOWN_MSG 0x124            /* 6708: the client's notification */
#define LOBBY_CMD_TOWN_MSG_TO 0x126         /* 670D: the client's request */
#define LOBBY_CMD_CHAT_SAY 0x129            /* 6714: the client's notification */
#define LOBBY_CMD_USER_STATUS 0x12a         /* 6707: the client's request */
#define LOBBY_CMD_CHAT_COLOR_MODE 0x12c     /* 670F: the client's request */
#define LOBBY_CMD_USER_PROFILE 0x12e        /* 6713: the client's request */
#define LOBBY_CMD_USER_SEARCH 0x130         /* 6709: the client's request */
#define LOBBY_CMD_USER_SEARCH_NEXT 0x132    /* 6709: the client's notification */
#define LOBBY_CMD_LEVEL2_QUEST 0x135        /* 6415: the client's request */
#define LOBBY_CMD_6416_NEXT 0x139           /* 6416: the client's notification, its meaning unknown */
#define LOBBY_CMD_USER_FIND_BY_NAME 0x13c   /* 6711: the client's request */
#define LOBBY_CMD_USER_FIND_BY_NAME_NEXT 0x13e/* 6711: the client's notification */
#define LOBBY_CMD_USER_CARD_FETCH 0x13f     /* 6712: the client's request */
#define LOBBY_CMD_LEVEL1_SET_GROUP_INFO 0x141/* 6318: the client's request */
#define LOBBY_CMD_LEVEL1_SEARCH 0x143       /* 6319: the client's request */
#define LOBBY_CMD_LEVEL1_SEARCH_NEXT 0x145  /* 6319: the client's notification */
#define LOBBY_CMD_LEVEL1_SEARCH_INFO 0x146  /* 631A: the client's request */
#define LOBBY_CMD_SHORT_MAIL_SEND 0x148     /* 6704: the client's request */
#define LOBBY_CMD_SERVER_NAME 0x151         /* 68C0: the client's request */

#endif /* LOBBY_CMD_H */
