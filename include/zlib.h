/* zlib.h -- the zlib 1.1.4 inflate types and interface, as the lobby
 * overlay's PNG decoder was built with them (0x7539B0-0x7571D0: adler32.c,
 * infblock.c, infcodes.c, inffast.c, inflate.c, inftrees.c, infutil.c).
 *
 * Altered source version (a reconstruction for the matching decompilation,
 * not the original zlib.h): the public interface and the private inflate
 * types of zlib.h, zconf.h, zutil.h, infutil.h, inftrees.h and infcodes.c
 * merged into one header, with MWCC's sizes (long, so uLong, is 64 bits;
 * enums take the smallest type that holds their values: one byte here).
 * The functions and tables have zlib's names
 * (config/symbols/lobby_browser.txt).
 *
 * zlib.h -- interface of the 'zlib' general purpose compression library
 * version 1.1.4, March 11th, 2002
 *
 * Copyright (C) 1995-2002 Jean-loup Gailly and Mark Adler
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Jean-loup Gailly        Mark Adler
 * jloup@gzip.org          madler@alumni.caltech.edu
 */
#ifndef ZLIB_H
#define ZLIB_H

/* zconf.h */
typedef unsigned char Byte;
typedef unsigned int uInt;
typedef unsigned long uLong; /* 64-bit with MWCC */
typedef Byte Bytef;
typedef uInt uIntf;
typedef uLong uLongf;
typedef void *voidpf;

/* zlib.h */
#define ZLIB_VERSION "1.1.4"

typedef voidpf (*alloc_func)(voidpf opaque, uInt items, uInt size);
typedef void (*free_func)(voidpf opaque, voidpf address);

typedef struct z_stream_s {
    /* 0x00 */ Bytef *next_in;
    /* 0x04 */ uInt avail_in;
    /* 0x08 */ uLong total_in;
    /* 0x10 */ Bytef *next_out;
    /* 0x14 */ uInt avail_out;
    /* 0x18 */ uLong total_out;
    /* 0x20 */ char *msg;
    /* 0x24 */ struct internal_state *state;
    /* 0x28 */ alloc_func zalloc;
    /* 0x2C */ free_func zfree;
    /* 0x30 */ voidpf opaque;
    /* 0x34 */ int data_type;
    /* 0x38 */ uLong adler;
    /* 0x40 */ uLong reserved;
} z_stream; /* 0x48 bytes */

typedef z_stream *z_streamp;

#define Z_NO_FLUSH 0
#define Z_PARTIAL_FLUSH 1
#define Z_SYNC_FLUSH 2
#define Z_FULL_FLUSH 3
#define Z_FINISH 4

#define Z_OK 0
#define Z_STREAM_END 1
#define Z_NEED_DICT 2
#define Z_ERRNO (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR (-3)
#define Z_MEM_ERROR (-4)
#define Z_BUF_ERROR (-5)
#define Z_VERSION_ERROR (-6)

#define Z_DEFLATED 8
#define Z_NULL 0

#ifdef ZLIB_INTERNAL
/* The private part, for the inflate units (zutil.h and the inflate headers) */

/* zlib's enums are one byte (the mode fields are read with lbu, stored with
 * sb): zlib was built with enums of the smallest size, not MWCC's int. */
#pragma enumsalwaysint off

/* zutil.h */
#define local static
#define DEF_WBITS 15 /* MAX_WBITS */
#define PRESET_DICT 0x20
#define zmemcpy memcpy
#define ZALLOC(strm, items, size) (*((strm)->zalloc))((strm)->opaque, (items), (size))
#define ZFREE(strm, addr) (*((strm)->zfree))((strm)->opaque, (voidpf)(addr))

typedef uLong (*check_func)(uLong check, const Bytef *buf, uInt len);

void *memcpy(void *dst, const void *src, u32 n);
void *memset(void *dst, s32 c, u32 n);

/* inftrees.h */
typedef struct inflate_huft_s inflate_huft;

struct inflate_huft_s {
    union {
        struct {
            Byte Exop; /* number of extra bits or operation */
            Byte Bits; /* number of bits in this code or subcode */
        } what;
        uInt pad;
    } word;
    uInt base; /* literal, length base, distance base, or table offset */
};

#define exop word.what.Exop
#define bits word.what.Bits

#define MANY 1440

/* infcodes.c */
typedef enum {
    START,   /* x: set up for LEN */
    LEN,     /* i: get length/literal/eob next */
    LENEXT,  /* i: getting length extra (have base) */
    DIST,    /* i: get distance next */
    DISTEXT, /* i: getting distance extra */
    COPY,    /* o: copying bytes in window, waiting for space */
    LIT,     /* o: got literal, waiting for output space */
    WASH,    /* o: got eob, possibly still output waiting */
    END,     /* x: got eob and all data flushed */
    BADCODE  /* x: got error */
} inflate_codes_mode;

typedef struct inflate_codes_state {
    /* 0x00 */ inflate_codes_mode mode;
    /* 0x04 */ uInt len;
    /* 0x08 */ union {
        struct {
            inflate_huft *tree; /* pointer into tree */
            uInt need;          /* bits needed */
        } code;
        uInt lit;
        struct {
            uInt get;  /* bits to get for extra */
            uInt dist; /* distance back to copy from */
        } copy;
    } sub;
    /* 0x10 */ Byte lbits;
    /* 0x11 */ Byte dbits;
    /* 0x14 */ inflate_huft *ltree;
    /* 0x18 */ inflate_huft *dtree;
} inflate_codes_statef; /* 0x1C bytes */

/* infutil.h */
typedef enum {
    TYPE,   /* get type bits (3, including end bit) */
    LENS,   /* get lengths for stored */
    STORED, /* processing stored block */
    TABLE,  /* get table lengths */
    BTREE,  /* get bit lengths tree for a dynamic block */
    DTREE,  /* get length, distance trees for a dynamic block */
    CODES,  /* processing fixed or dynamic block */
    DRY,    /* output remaining window bytes */
    DONE,   /* finished last block, done */
    BAD     /* got a data error--stuck here */
} inflate_block_mode;

typedef struct inflate_blocks_state {
    /* 0x00 */ inflate_block_mode mode;
    /* 0x04 */ union {
        uInt left; /* if STORED, bytes left to copy */
        struct {
            uInt table;       /* table lengths (14 bits) */
            uInt index;       /* index into blens (or border) */
            uIntf *blens;     /* bit lengths of codes */
            uInt bb;          /* bit length tree depth */
            inflate_huft *tb; /* bit length decoding tree */
        } trees;
        struct {
            inflate_codes_statef *codes;
        } decode;
    } sub;
    /* 0x18 */ uInt last;
    /* 0x1C */ uInt bitk;
    /* 0x20 */ uLong bitb;
    /* 0x28 */ inflate_huft *hufts;
    /* 0x2C */ Bytef *window;
    /* 0x30 */ Bytef *end;
    /* 0x34 */ Bytef *read;
    /* 0x38 */ Bytef *write;
    /* 0x3C */ check_func checkfn;
    /* 0x40 */ uLong check;
} inflate_blocks_statef; /* 0x48 bytes */

#define UPDBITS {s->bitb=b;s->bitk=k;}
#define UPDIN {z->avail_in=n;z->total_in+=p-z->next_in;z->next_in=p;}
#define UPDOUT {s->write=q;}
#define UPDATE {UPDBITS UPDIN UPDOUT}
#define LEAVE {UPDATE return inflate_flush(s,z,r);}
#define LOADIN {p=z->next_in;n=z->avail_in;b=s->bitb;k=s->bitk;}
#define NEEDBYTE {if(n)r=Z_OK;else LEAVE}
#define NEXTBYTE (n--,*p++)
#define NEEDBITS(j) {while(k<(j)){NEEDBYTE;b|=((uLong)NEXTBYTE)<<k;k+=8;}}
#define DUMPBITS(j) {b>>=(j);k-=(j);}
#define WAVAIL (uInt)(q<s->read?s->read-q-1:s->end-q)
#define LOADOUT {q=s->write;m=(uInt)WAVAIL;}
#define WRAP {if(q==s->end&&s->read!=s->window){q=s->window;m=(uInt)WAVAIL;}}
#define FLUSH {UPDOUT r=inflate_flush(s,z,r); LOADOUT}
#define NEEDOUT {if(m==0){WRAP if(m==0){FLUSH WRAP if(m==0) LEAVE}}r=Z_OK;}
#define OUTBYTE(a) {*q++=(Byte)(a);m--;}
#define LOAD {LOADIN LOADOUT}

/* inflate.c */
typedef enum {
    METHOD, /* waiting for method byte */
    FLAG,   /* waiting for flag byte */
    DICT4,  /* four dictionary check bytes to go */
    DICT3,  /* three dictionary check bytes to go */
    DICT2,  /* two dictionary check bytes to go */
    DICT1,  /* one dictionary check byte to go */
    DICT0,  /* waiting for inflateSetDictionary */
    BLOCKS, /* decompressing blocks */
    CHECK4, /* four check bytes to go */
    CHECK3, /* three check bytes to go */
    CHECK2, /* two check bytes to go */
    CHECK1, /* one check byte to go */
    I_DONE, /* finished check, done (DONE in inflate.c) */
    I_BAD   /* got an error--stay here (BAD in inflate.c) */
} inflate_mode;

struct internal_state {
    /* 0x00 */ inflate_mode mode;
    /* 0x08 */ union {
        uInt method; /* if FLAGS, method byte */
        struct {
            uLong was;  /* computed check value */
            uLong need; /* stream check value */
        } check;
        uInt marker; /* if BAD, inflateSync's marker bytes count */
    } sub;
    /* 0x18 */ int nowrap;
    /* 0x1C */ uInt wbits;
    /* 0x20 */ inflate_blocks_statef *blocks;
}; /* 0x28 bytes */

#pragma enumsalwaysint reset

/* Data (.data of inftrees.c and infutil.c) */
extern uInt fixed_bl;           /* inffixed.h */
extern uInt fixed_bd;
extern inflate_huft fixed_tl[]; /* [512] */
extern inflate_huft fixed_td[]; /* [32] */
extern uInt inflate_mask[17];

/* infblock.c */
void inflate_blocks_reset(inflate_blocks_statef *s, z_streamp z, uLongf *c);
inflate_blocks_statef *inflate_blocks_new(z_streamp z, check_func c, uInt w);
int inflate_blocks(inflate_blocks_statef *s, z_streamp z, int r);
int inflate_blocks_free(inflate_blocks_statef *s, z_streamp z);

/* infcodes.c */
inflate_codes_statef *inflate_codes_new(uInt bl, uInt bd, inflate_huft *tl, inflate_huft *td,
                                        z_streamp z);
int inflate_codes(inflate_blocks_statef *s, z_streamp z, int r);
void inflate_codes_free(inflate_codes_statef *c, z_streamp z);

/* inffast.c */
int inflate_fast(uInt bl, uInt bd, inflate_huft *tl, inflate_huft *td, inflate_blocks_statef *s,
                 z_streamp z);

/* inftrees.c */
int inflate_trees_bits(uIntf *c, uIntf *bb, inflate_huft **tb, inflate_huft *hp, z_streamp z);
int inflate_trees_dynamic(uInt nl, uInt nd, uIntf *c, uIntf *bl, uIntf *bd, inflate_huft **tl,
                          inflate_huft **td, inflate_huft *hp, z_streamp z);
int inflate_trees_fixed(uIntf *bl, uIntf *bd, inflate_huft **tl, inflate_huft **td, z_streamp z);

/* infutil.c */
int inflate_flush(inflate_blocks_statef *s, z_streamp z, int r);

#endif /* ZLIB_INTERNAL */

/* The PNG decoder's allocator (zutil.c's zcalloc and zcfree, the game's
 * versions on a heap of its own at gImgHeap: unk_74D3F0.c, unk_7513C0.c) */
voidpf zcalloc(voidpf opaque, uInt items, uInt size);
void zcfree(voidpf opaque, voidpf ptr);

/* adler32.c */
uLong adler32(uLong adler, const Bytef *buf, uInt len);

/* inflate.c */
int inflateReset(z_streamp z);
int inflateEnd(z_streamp z);
int inflateInit2_(z_streamp z, int w, const char *version, int stream_size);
int inflateInit_(z_streamp z, const char *version, int stream_size);
int inflate(z_streamp z, int f);

#define inflateInit(strm) inflateInit_((strm), ZLIB_VERSION, sizeof(z_stream))

#endif /* ZLIB_H */
