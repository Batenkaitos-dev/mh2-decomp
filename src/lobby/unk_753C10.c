/* zlib 1.1.4's inflate, as the lobby's PNG decoder links it: the end of
 * infblock.c (from inflate_blocks_new), infcodes.c, inffast.c, inflate.c
 * and inftrees.c up to inflate_trees_dynamic. inflate_blocks_reset is in
 * unk_753B50.c, inflate_trees_fixed and infutil.c in unk_756FD0.c, adler32
 * at the end of unk_7513C0.c. The functions follow the zlib 1.1.4 sources
 * (the linker dropped the unused ones: inflate_set_dictionary,
 * inflate_blocks_sync_point, inflateSetDictionary, inflateSync,
 * inflateSyncPoint; and inflate_copyright). Types and macros:
 * include/zlib.h.
 *
 * Altered source version: zlib's code with ANSI definitions, in the file
 * layout of the decompilation. The original notice, from zlib.h:
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
#include "common.h"
#define ZLIB_INTERNAL
#include "zlib.h"

/* zlib was compiled at optimization level 4 (inflate_blocks reuses the
 * DTREE bound, inflate_fast and huft_build need it too). */
#pragma optimization_level 4

/* ---- infblock.c ---- */

/* Table for deflate from PKZIP's appnote.txt. */
local const uInt border[] = { /* Order of the bit length code lengths */
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

/* inflate_blocks_new: allocates the block state, its huft space and the
 * window (w bytes); c is the check function (adler32, or NULL for raw). */
inflate_blocks_statef *inflate_blocks_new(z_streamp z, check_func c, uInt w)
{
    inflate_blocks_statef *s;

    if ((s = (inflate_blocks_statef *)ZALLOC(z, 1, sizeof(struct inflate_blocks_state))) == Z_NULL)
        return s;
    if ((s->hufts = (inflate_huft *)ZALLOC(z, sizeof(inflate_huft), MANY)) == Z_NULL)
    {
        ZFREE(z, s);
        return Z_NULL;
    }
    if ((s->window = (Bytef *)ZALLOC(z, 1, w)) == Z_NULL)
    {
        ZFREE(z, s->hufts);
        ZFREE(z, s);
        return Z_NULL;
    }
    s->end = s->window + w;
    s->checkfn = c;
    s->mode = TYPE;
    inflate_blocks_reset(s, z, Z_NULL);
    return s;
}

/* inflate_blocks: decodes deflate blocks (stored, fixed, dynamic) into the
 * window; r is the result so far, returned through inflate_flush. */
int inflate_blocks(inflate_blocks_statef *s, z_streamp z, int r)
{
    uInt t;   /* temporary storage */
    uLong b;  /* bit buffer */
    uInt k;   /* bits in bit buffer */
    Bytef *p; /* input data pointer */
    uInt n;   /* bytes available there */
    Bytef *q; /* output window write pointer */
    uInt m;   /* bytes to end of window or read pointer */

    /* copy input/output information to locals (UPDATE macro restores) */
    LOAD

    /* process input based on current state */
    while (1) switch (s->mode)
    {
    case TYPE:
        NEEDBITS(3)
        t = (uInt)b & 7;
        s->last = t & 1;
        switch (t >> 1)
        {
        case 0: /* stored */
            DUMPBITS(3)
            t = k & 7; /* go to byte boundary */
            DUMPBITS(t)
            s->mode = LENS; /* get length of stored block */
            break;
        case 1: /* fixed */
            {
                uInt bl, bd;
                inflate_huft *tl, *td;

                inflate_trees_fixed(&bl, &bd, &tl, &td, z);
                s->sub.decode.codes = inflate_codes_new(bl, bd, tl, td, z);
                if (s->sub.decode.codes == Z_NULL)
                {
                    r = Z_MEM_ERROR;
                    LEAVE
                }
            }
            DUMPBITS(3)
            s->mode = CODES;
            break;
        case 2: /* dynamic */
            DUMPBITS(3)
            s->mode = TABLE;
            break;
        case 3: /* illegal */
            DUMPBITS(3)
            s->mode = BAD;
            z->msg = (char *)"invalid block type";
            r = Z_DATA_ERROR;
            LEAVE
        }
        break;
    case LENS:
        NEEDBITS(32)
        if ((((~b) >> 16) & 0xffff) != (b & 0xffff))
        {
            s->mode = BAD;
            z->msg = (char *)"invalid stored block lengths";
            r = Z_DATA_ERROR;
            LEAVE
        }
        s->sub.left = (uInt)b & 0xffff;
        b = k = 0; /* dump bits */
        s->mode = s->sub.left ? STORED : (s->last ? DRY : TYPE);
        break;
    case STORED:
        if (n == 0)
            LEAVE
        NEEDOUT
        t = s->sub.left;
        if (t > n) t = n;
        if (t > m) t = m;
        zmemcpy(q, p, t);
        p += t;  n -= t;
        q += t;  m -= t;
        if ((s->sub.left -= t) != 0)
            break;
        s->mode = s->last ? DRY : TYPE;
        break;
    case TABLE:
        NEEDBITS(14)
        s->sub.trees.table = t = (uInt)b & 0x3fff;
        if ((t & 0x1f) > 29 || ((t >> 5) & 0x1f) > 29)
        {
            s->mode = BAD;
            z->msg = (char *)"too many length or distance symbols";
            r = Z_DATA_ERROR;
            LEAVE
        }
        t = 258 + (t & 0x1f) + ((t >> 5) & 0x1f);
        if ((s->sub.trees.blens = (uIntf *)ZALLOC(z, t, sizeof(uInt))) == Z_NULL)
        {
            r = Z_MEM_ERROR;
            LEAVE
        }
        DUMPBITS(14)
        s->sub.trees.index = 0;
        s->mode = BTREE;
    case BTREE:
        while (s->sub.trees.index < 4 + (s->sub.trees.table >> 10))
        {
            NEEDBITS(3)
            s->sub.trees.blens[border[s->sub.trees.index++]] = (uInt)b & 7;
            DUMPBITS(3)
        }
        while (s->sub.trees.index < 19)
            s->sub.trees.blens[border[s->sub.trees.index++]] = 0;
        s->sub.trees.bb = 7;
        t = inflate_trees_bits(s->sub.trees.blens, &s->sub.trees.bb, &s->sub.trees.tb, s->hufts, z);
        if (t != Z_OK)
        {
            r = t;
            if (r == Z_DATA_ERROR)
            {
                ZFREE(z, s->sub.trees.blens);
                s->mode = BAD;
            }
            LEAVE
        }
        s->sub.trees.index = 0;
        s->mode = DTREE;
    case DTREE:
        while (t = s->sub.trees.table,
               s->sub.trees.index < 258 + (t & 0x1f) + ((t >> 5) & 0x1f))
        {
            inflate_huft *h;
            uInt i, j, c;

            t = s->sub.trees.bb;
            NEEDBITS(t)
            h = s->sub.trees.tb + ((uInt)b & inflate_mask[t]);
            t = h->bits;
            c = h->base;
            if (c < 16)
            {
                DUMPBITS(t)
                s->sub.trees.blens[s->sub.trees.index++] = c;
            }
            else /* c == 16..18 */
            {
                i = c == 18 ? 7 : c - 14;
                j = c == 18 ? 11 : 3;
                NEEDBITS(t + i)
                DUMPBITS(t)
                j += (uInt)b & inflate_mask[i];
                DUMPBITS(i)
                i = s->sub.trees.index;
                t = s->sub.trees.table;
                if (i + j > 258 + (t & 0x1f) + ((t >> 5) & 0x1f) ||
                    (c == 16 && i < 1))
                {
                    ZFREE(z, s->sub.trees.blens);
                    s->mode = BAD;
                    z->msg = (char *)"invalid bit length repeat";
                    r = Z_DATA_ERROR;
                    LEAVE
                }
                c = c == 16 ? s->sub.trees.blens[i - 1] : 0;
                do {
                    s->sub.trees.blens[i++] = c;
                } while (--j);
                s->sub.trees.index = i;
            }
        }
        s->sub.trees.tb = Z_NULL;
        {
            uInt bl, bd;
            inflate_huft *tl, *td;
            inflate_codes_statef *c;

            bl = 9; /* must be <= 9 for lookahead assumptions */
            bd = 6; /* must be <= 9 for lookahead assumptions */
            t = s->sub.trees.table;
            t = inflate_trees_dynamic(257 + (t & 0x1f), 1 + ((t >> 5) & 0x1f),
                              s->sub.trees.blens, &bl, &bd, &tl, &td,
                              s->hufts, z);
            if (t != Z_OK)
            {
                if (t == (uInt)Z_DATA_ERROR)
                {
                    ZFREE(z, s->sub.trees.blens);
                    s->mode = BAD;
                }
                r = t;
                LEAVE
            }
            if ((c = inflate_codes_new(bl, bd, tl, td, z)) == Z_NULL)
            {
                r = Z_MEM_ERROR;
                LEAVE
            }
            s->sub.decode.codes = c;
        }
        ZFREE(z, s->sub.trees.blens);
        s->mode = CODES;
    case CODES:
        UPDATE
        if ((r = inflate_codes(s, z, r)) != Z_STREAM_END)
            return inflate_flush(s, z, r);
        r = Z_OK;
        inflate_codes_free(s->sub.decode.codes, z);
        LOAD
        if (!s->last)
        {
            s->mode = TYPE;
            break;
        }
        s->mode = DRY;
    case DRY:
        FLUSH
        if (s->read != s->write)
            LEAVE
        s->mode = DONE;
    case DONE:
        r = Z_STREAM_END;
        LEAVE
    case BAD:
        r = Z_DATA_ERROR;
        LEAVE
    default:
        r = Z_STREAM_ERROR;
        LEAVE
    }
}

/* inflate_blocks_free: frees the window, the huft space and the state. */
int inflate_blocks_free(inflate_blocks_statef *s, z_streamp z)
{
    inflate_blocks_reset(s, z, Z_NULL);
    ZFREE(z, s->window);
    ZFREE(z, s->hufts);
    ZFREE(z, s);
    return Z_OK;
}

/* ---- infcodes.c ---- */

/* inflate_codes_new: a codes state for the trees tl/td (bl/bd bits). */
inflate_codes_statef *inflate_codes_new(uInt bl, uInt bd, inflate_huft *tl, inflate_huft *td,
                                    z_streamp z)
{
    inflate_codes_statef *c;

    if ((c = (inflate_codes_statef *)ZALLOC(z, 1, sizeof(struct inflate_codes_state))) != Z_NULL)
    {
        c->mode = START;
        c->lbits = (Byte)bl;
        c->dbits = (Byte)bd;
        c->ltree = tl;
        c->dtree = td;
    }
    return c;
}

/* inflate_codes: decodes literals and length/distance pairs of a block. */
int inflate_codes(inflate_blocks_statef *s, z_streamp z, int r)
{
    uInt j;          /* temporary storage */
    inflate_huft *t; /* temporary pointer */
    uInt e;          /* extra bits or operation */
    uLong b;         /* bit buffer */
    uInt k;          /* bits in bit buffer */
    Bytef *p;        /* input data pointer */
    uInt n;          /* bytes available there */
    Bytef *q;        /* output window write pointer */
    uInt m;          /* bytes to end of window or read pointer */
    Bytef *f;        /* pointer to copy strings from */
    inflate_codes_statef *c = s->sub.decode.codes; /* codes state */

    /* copy input/output information to locals (UPDATE macro restores) */
    LOAD

    /* process input and output based on current state */
    while (1) switch (c->mode)
    {
    case START: /* x: set up for LEN */
        if (m >= 258 && n >= 10)
        {
            UPDATE
            r = inflate_fast(c->lbits, c->dbits, c->ltree, c->dtree, s, z);
            LOAD
            if (r != Z_OK)
            {
                c->mode = r == Z_STREAM_END ? WASH : BADCODE;
                break;
            }
        }
        c->sub.code.need = c->lbits;
        c->sub.code.tree = c->ltree;
        c->mode = LEN;
    case LEN: /* i: get length/literal/eob next */
        j = c->sub.code.need;
        NEEDBITS(j)
        t = c->sub.code.tree + ((uInt)b & inflate_mask[j]);
        DUMPBITS(t->bits)
        e = (uInt)(t->exop);
        if (e == 0) /* literal */
        {
            c->sub.lit = t->base;
            c->mode = LIT;
            break;
        }
        if (e & 16) /* length */
        {
            c->sub.copy.get = e & 15;
            c->len = t->base;
            c->mode = LENEXT;
            break;
        }
        if ((e & 64) == 0) /* next table */
        {
            c->sub.code.need = e;
            c->sub.code.tree = t + t->base;
            break;
        }
        if (e & 32) /* end of block */
        {
            c->mode = WASH;
            break;
        }
        c->mode = BADCODE; /* invalid code */
        z->msg = (char *)"invalid literal/length code";
        r = Z_DATA_ERROR;
        LEAVE
    case LENEXT: /* i: getting length extra (have base) */
        j = c->sub.copy.get;
        NEEDBITS(j)
        c->len += (uInt)b & inflate_mask[j];
        DUMPBITS(j)
        c->sub.code.need = c->dbits;
        c->sub.code.tree = c->dtree;
        c->mode = DIST;
    case DIST: /* i: get distance next */
        j = c->sub.code.need;
        NEEDBITS(j)
        t = c->sub.code.tree + ((uInt)b & inflate_mask[j]);
        DUMPBITS(t->bits)
        e = (uInt)(t->exop);
        if (e & 16) /* distance */
        {
            c->sub.copy.get = e & 15;
            c->sub.copy.dist = t->base;
            c->mode = DISTEXT;
            break;
        }
        if ((e & 64) == 0) /* next table */
        {
            c->sub.code.need = e;
            c->sub.code.tree = t + t->base;
            break;
        }
        c->mode = BADCODE; /* invalid code */
        z->msg = (char *)"invalid distance code";
        r = Z_DATA_ERROR;
        LEAVE
    case DISTEXT: /* i: getting distance extra */
        j = c->sub.copy.get;
        NEEDBITS(j)
        c->sub.copy.dist += (uInt)b & inflate_mask[j];
        DUMPBITS(j)
        c->mode = COPY;
    case COPY: /* o: copying bytes in window, waiting for space */
        f = q - c->sub.copy.dist;
        while (f < s->window)            /* modulo window size-"while" instead */
            f += s->end - s->window;     /* of "if" handles invalid distances */
        while (c->len)
        {
            NEEDOUT
            OUTBYTE(*f++)
            if (f == s->end)
                f = s->window;
            c->len--;
        }
        c->mode = START;
        break;
    case LIT: /* o: got literal, waiting for output space */
        NEEDOUT
        OUTBYTE(c->sub.lit)
        c->mode = START;
        break;
    case WASH: /* o: got eob, possibly more output */
        if (k > 7) /* return unused byte, if any */
        {
            k -= 8;
            n++;
            p--; /* can always return one */
        }
        FLUSH
        if (s->read != s->write)
            LEAVE
        c->mode = END;
    case END:
        r = Z_STREAM_END;
        LEAVE
    case BADCODE: /* x: got error */
        r = Z_DATA_ERROR;
        LEAVE
    default:
        r = Z_STREAM_ERROR;
        LEAVE
    }
}

/* inflate_codes_free */
void inflate_codes_free(inflate_codes_statef *c, z_streamp z)
{
    ZFREE(z, c);
}

/* ---- inffast.c ---- */

/* inffast.c was a unit of its own, with its own copies of the two messages
 * that infcodes.c also has (0x83AA10, 0x83AA30). */
#pragma dont_reuse_strings on

/* macros for bit input with no checking and for returning unused bytes */
#define GRABBITS(j) {while(k<(j)){b|=((uLong)NEXTBYTE)<<k;k+=8;}}
#define UNGRAB {c=z->avail_in-n;c=(k>>3)<c?k>>3:c;n+=c;p-=c;k-=c<<3;}

/* inflate_fast: the fast loop of inflate_codes, while at least 258 bytes
 * of window and 10 bytes of input are left. */
int inflate_fast(uInt bl, uInt bd, inflate_huft *tl, inflate_huft *td, inflate_blocks_statef *s,
                  z_streamp z)
{
    inflate_huft *t; /* temporary pointer */
    uInt e;          /* extra bits or operation */
    uLong b;         /* bit buffer */
    uInt k;          /* bits in bit buffer */
    Bytef *p;        /* input data pointer */
    uInt n;          /* bytes available there */
    Bytef *q;        /* output window write pointer */
    uInt m;          /* bytes to end of window or read pointer */
    uInt ml;         /* mask for literal/length tree */
    uInt md;         /* mask for distance tree */
    uInt c;          /* bytes to copy */
    uInt d;          /* distance back to copy from */
    Bytef *r;        /* copy source pointer */

    /* load input, output, bit values */
    LOAD

    /* initialize masks */
    ml = inflate_mask[bl];
    md = inflate_mask[bd];

    /* do until not enough input or output space for fast loop */
    do { /* assume called with m >= 258 && n >= 10 */
        /* get literal/length code */
        GRABBITS(20) /* max bits for literal/length code */
        if ((e = (t = tl + ((uInt)b & ml))->exop) == 0)
        {
            DUMPBITS(t->bits)
            *q++ = (Byte)t->base;
            m--;
            continue;
        }
        do {
            DUMPBITS(t->bits)
            if (e & 16)
            {
                /* get extra bits for length */
                e &= 15;
                c = t->base + ((uInt)b & inflate_mask[e]);
                DUMPBITS(e)

                /* decode distance base of block to copy */
                GRABBITS(15); /* max bits for distance code */
                e = (t = td + ((uInt)b & md))->exop;
                do {
                    DUMPBITS(t->bits)
                    if (e & 16)
                    {
                        /* get extra bits to add to distance base */
                        e &= 15;
                        GRABBITS(e) /* get extra bits (up to 13) */
                        d = t->base + ((uInt)b & inflate_mask[e]);
                        DUMPBITS(e)

                        /* do the copy */
                        m -= c;
                        r = q - d;
                        if (r < s->window) /* wrap if needed */
                        {
                            do {
                                r += s->end - s->window; /* force pointer in window */
                            } while (r < s->window);     /* covers invalid distances */
                            e = s->end - r;
                            if (c > e)
                            {
                                c -= e; /* wrapped copy */
                                do {
                                    *q++ = *r++;
                                } while (--e);
                                r = s->window;
                                do {
                                    *q++ = *r++;
                                } while (--c);
                            }
                            else /* normal copy */
                            {
                                *q++ = *r++;  c--;
                                *q++ = *r++;  c--;
                                do {
                                    *q++ = *r++;
                                } while (--c);
                            }
                        }
                        else /* normal copy */
                        {
                            *q++ = *r++;  c--;
                            *q++ = *r++;  c--;
                            do {
                                *q++ = *r++;
                            } while (--c);
                        }
                        break;
                    }
                    else if ((e & 64) == 0)
                    {
                        t += t->base;
                        e = (t += ((uInt)b & inflate_mask[e]))->exop;
                    }
                    else
                    {
                        z->msg = (char *)"invalid distance code";
                        UNGRAB
                        UPDATE
                        return Z_DATA_ERROR;
                    }
                } while (1);
                break;
            }
            if ((e & 64) == 0)
            {
                t += t->base;
                if ((e = (t += ((uInt)b & inflate_mask[e]))->exop) == 0)
                {
                    DUMPBITS(t->bits)
                    *q++ = (Byte)t->base;
                    m--;
                    break;
                }
            }
            else if (e & 32)
            {
                UNGRAB
                UPDATE
                return Z_STREAM_END;
            }
            else
            {
                z->msg = (char *)"invalid literal/length code";
                UNGRAB
                UPDATE
                return Z_DATA_ERROR;
            }
        } while (1);
    } while (m >= 258 && n >= 10);

    /* not enough input or output--restore pointers and return */
    UNGRAB
    UPDATE
    return Z_OK;
}

#undef GRABBITS
#undef UNGRAB

#pragma dont_reuse_strings reset

/* ---- inflate.c ---- */

/* inflateReset */
int inflateReset(z_streamp z)
{
    if (z == Z_NULL || z->state == Z_NULL)
        return Z_STREAM_ERROR;
    z->total_in = z->total_out = 0;
    z->msg = Z_NULL;
    z->state->mode = z->state->nowrap ? BLOCKS : METHOD;
    inflate_blocks_reset(z->state->blocks, z, Z_NULL);
    return Z_OK;
}

/* inflateEnd */
int inflateEnd(z_streamp z)
{
    if (z == Z_NULL || z->state == Z_NULL || z->zfree == Z_NULL)
        return Z_STREAM_ERROR;
    if (z->state->blocks != Z_NULL)
        inflate_blocks_free(z->state->blocks, z);
    ZFREE(z, z->state);
    z->state = Z_NULL;
    return Z_OK;
}

/* inflateInit2_: w is the window's log2 (8..15, negative for no zlib
 * header); the PNG decoder's zcalloc/zcfree are the default allocator. */
int inflateInit2_(z_streamp z, int w, const char *version, int stream_size)
{
    if (version == Z_NULL || version[0] != ZLIB_VERSION[0] ||
        stream_size != sizeof(z_stream))
        return Z_VERSION_ERROR;

    /* initialize state */
    if (z == Z_NULL)
        return Z_STREAM_ERROR;
    z->msg = Z_NULL;
    if (z->zalloc == Z_NULL)
    {
        z->zalloc = zcalloc;
        z->opaque = (voidpf)0;
    }
    if (z->zfree == Z_NULL) z->zfree = zcfree;
    if ((z->state = (struct internal_state *)ZALLOC(z, 1, sizeof(struct internal_state))) == Z_NULL)
        return Z_MEM_ERROR;
    z->state->blocks = Z_NULL;

    /* handle undocumented nowrap option (no zlib header or check) */
    z->state->nowrap = 0;
    if (w < 0)
    {
        w = -w;
        z->state->nowrap = 1;
    }

    /* set window size */
    if (w < 8 || w > 15)
    {
        inflateEnd(z);
        return Z_STREAM_ERROR;
    }
    z->state->wbits = (uInt)w;

    /* create inflate_blocks state */
    if ((z->state->blocks =
         inflate_blocks_new(z, z->state->nowrap ? Z_NULL : adler32, (uInt)1 << w))
        == Z_NULL)
    {
        inflateEnd(z);
        return Z_MEM_ERROR;
    }

    /* reset state */
    inflateReset(z);
    return Z_OK;
}

/* inflateInit_ */
int inflateInit_(z_streamp z, const char *version, int stream_size)
{
    return inflateInit2_(z, DEF_WBITS, version, stream_size);
}

/* inflate.c's own byte input macros */
#undef NEEDBYTE
#undef NEXTBYTE
#define NEEDBYTE {if(z->avail_in==0)return r;r=f;}
#define NEXTBYTE (z->avail_in--,z->total_in++,*z->next_in++)

/* inflate: decompresses as much as possible (the zlib header, the blocks,
 * the adler32 check); f is the flush mode (Z_FINISH: Z_BUF_ERROR when
 * the input runs out). */
int inflate(z_streamp z, int f)
{
    int r;
    uInt b;

    if (z == Z_NULL || z->state == Z_NULL || z->next_in == Z_NULL)
        return Z_STREAM_ERROR;
    f = f == Z_FINISH ? Z_BUF_ERROR : Z_OK;
    r = Z_BUF_ERROR;
    while (1) switch (z->state->mode)
    {
    case METHOD:
        NEEDBYTE
        if (((z->state->sub.method = NEXTBYTE) & 0xf) != Z_DEFLATED)
        {
            z->state->mode = I_BAD;
            z->msg = (char *)"unknown compression method";
            z->state->sub.marker = 5; /* can't try inflateSync */
            break;
        }
        if ((z->state->sub.method >> 4) + 8 > z->state->wbits)
        {
            z->state->mode = I_BAD;
            z->msg = (char *)"invalid window size";
            z->state->sub.marker = 5; /* can't try inflateSync */
            break;
        }
        z->state->mode = FLAG;
    case FLAG:
        NEEDBYTE
        b = NEXTBYTE;
        if (((z->state->sub.method << 8) + b) % 31)
        {
            z->state->mode = I_BAD;
            z->msg = (char *)"incorrect header check";
            z->state->sub.marker = 5; /* can't try inflateSync */
            break;
        }
        if (!(b & PRESET_DICT))
        {
            z->state->mode = BLOCKS;
            break;
        }
        z->state->mode = DICT4;
    case DICT4:
        NEEDBYTE
        z->state->sub.check.need = (uLong)NEXTBYTE << 24;
        z->state->mode = DICT3;
    case DICT3:
        NEEDBYTE
        z->state->sub.check.need += (uLong)NEXTBYTE << 16;
        z->state->mode = DICT2;
    case DICT2:
        NEEDBYTE
        z->state->sub.check.need += (uLong)NEXTBYTE << 8;
        z->state->mode = DICT1;
    case DICT1:
        NEEDBYTE
        z->state->sub.check.need += (uLong)NEXTBYTE;
        z->adler = z->state->sub.check.need;
        z->state->mode = DICT0;
        return Z_NEED_DICT;
    case DICT0:
        z->state->mode = I_BAD;
        z->msg = (char *)"need dictionary";
        z->state->sub.marker = 0; /* can try inflateSync */
        return Z_STREAM_ERROR;
    case BLOCKS:
        r = inflate_blocks(z->state->blocks, z, r);
        if (r == Z_DATA_ERROR)
        {
            z->state->mode = I_BAD;
            z->state->sub.marker = 0; /* can try inflateSync */
            break;
        }
        if (r == Z_OK)
            r = f;
        if (r != Z_STREAM_END)
            return r;
        r = f;
        inflate_blocks_reset(z->state->blocks, z, &z->state->sub.check.was);
        if (z->state->nowrap)
        {
            z->state->mode = I_DONE;
            break;
        }
        z->state->mode = CHECK4;
    case CHECK4:
        NEEDBYTE
        z->state->sub.check.need = (uLong)NEXTBYTE << 24;
        z->state->mode = CHECK3;
    case CHECK3:
        NEEDBYTE
        z->state->sub.check.need += (uLong)NEXTBYTE << 16;
        z->state->mode = CHECK2;
    case CHECK2:
        NEEDBYTE
        z->state->sub.check.need += (uLong)NEXTBYTE << 8;
        z->state->mode = CHECK1;
    case CHECK1:
        NEEDBYTE
        z->state->sub.check.need += (uLong)NEXTBYTE;

        if (z->state->sub.check.was != z->state->sub.check.need)
        {
            z->state->mode = I_BAD;
            z->msg = (char *)"incorrect data check";
            z->state->sub.marker = 5; /* can't try inflateSync */
            break;
        }
        z->state->mode = I_DONE;
    case I_DONE:
        return Z_STREAM_END;
    case I_BAD:
        return Z_DATA_ERROR;
    default:
        return Z_STREAM_ERROR;
    }
}

#undef NEEDBYTE
#undef NEXTBYTE

/* ---- inftrees.c ---- */

/* Tables for deflate from PKZIP's appnote.txt. */
local const uInt cplens[31] = { /* Copy lengths for literal codes 257..285 */
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0};
    /* see note #13 above about 258 */
local const uInt cplext[31] = { /* Extra bits for literal codes 257..285 */
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 112, 112}; /* 112==invalid */
local const uInt cpdist[30] = { /* Copy offsets for distance codes 0..29 */
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
    8193, 12289, 16385, 24577};
local const uInt cpdext[30] = { /* Extra bits for distance codes */
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
    12, 12, 13, 13};

/* If BMAX needs to be larger than 16, then h and x[] should be uLong. */
#define BMAX 15 /* maximum bit length of any code */

/* huft_build: builds the multi-level decoding tables for n code lengths b
 * (values below s are literal, the others d[]/e[]); Z_BUF_ERROR if the
 * code set is incomplete, Z_DATA_ERROR if invalid. */
local int huft_build(uIntf *b, uInt n, uInt s, const uIntf *d, const uIntf *e,
                        inflate_huft **t, uIntf *m, inflate_huft *hp, uInt *hn, uIntf *v)
{
    uInt a;                 /* counter for codes of length k */
    uInt c[BMAX + 1];       /* bit length count table */
    uInt f;                 /* i repeats in table every f entries */
    int g;                  /* maximum code length */
    int h;                  /* table level */
    register uInt i;        /* counter, current code */
    register uInt j;        /* counter */
    register int k;         /* number of bits in current code */
    int l;                  /* bits per table (returned in m) */
    uInt mask;              /* (1 << w) - 1, to avoid cc -O bug on HP */
    register uIntf *p;      /* pointer into c[], b[], or v[] */
    inflate_huft *q;        /* points to current table */
    struct inflate_huft_s r; /* table entry for structure assignment */
    inflate_huft *u[BMAX];  /* table stack */
    register int w;         /* bits before this table == (l * h) */
    uInt x[BMAX + 1];       /* bit offsets, then code stack */
    uIntf *xp;              /* pointer into x */
    int y;                  /* number of dummy codes added */
    uInt z;                 /* number of entries in current table */

    /* Generate counts for each bit length */
    p = c;
#define C0 *p++ = 0;
#define C2 C0 C0 C0 C0
#define C4 C2 C2 C2 C2
    C4 /* clear c[]--assume BMAX+1 is 16 */
#undef C0
#undef C2
#undef C4
    p = b;  i = n;
    do {
        c[*p++]++; /* assume all entries <= BMAX */
    } while (--i);
    if (c[0] == n) /* null input--all zero length codes */
    {
        *t = (inflate_huft *)Z_NULL;
        *m = 0;
        return Z_OK;
    }

    /* Find minimum and maximum length, bound *m by those */
    l = *m;
    for (j = 1; j <= BMAX; j++)
        if (c[j])
            break;
    k = j; /* minimum code length */
    if ((uInt)l < j)
        l = j;
    for (i = BMAX; i; i--)
        if (c[i])
            break;
    g = i; /* maximum code length */
    if ((uInt)l > i)
        l = i;
    *m = l;

    /* Adjust last length count to fill out codes, if needed */
    for (y = 1 << j; j < i; j++, y <<= 1)
        if ((y -= c[j]) < 0)
            return Z_DATA_ERROR;
    if ((y -= c[i]) < 0)
        return Z_DATA_ERROR;
    c[i] += y;

    /* Generate starting offsets into the value table for each length */
    x[1] = j = 0;
    p = c + 1;  xp = x + 2;
    while (--i) { /* note that i == g from above */
        *xp++ = (j += *p++);
    }

    /* Make a table of values in order of bit lengths */
    p = b;  i = 0;
    do {
        if ((j = *p++) != 0)
            v[x[j]++] = i;
    } while (++i < n);
    n = x[g]; /* set n to length of v */

    /* Generate the Huffman codes and for each, make the table entries */
    x[0] = i = 0;                  /* first Huffman code is zero */
    p = v;                         /* grab values in bit order */
    h = -1;                        /* no tables yet--level -1 */
    w = -l;                        /* bits decoded == (l * h) */
    u[0] = (inflate_huft *)Z_NULL; /* just to keep compilers happy */
    q = (inflate_huft *)Z_NULL;    /* ditto */
    z = 0;                         /* ditto */

    /* go through the bit lengths (k already is bits in shortest code) */
    for (; k <= g; k++)
    {
        a = c[k];
        while (a--)
        {
            /* here i is the Huffman code of length k bits for value *p */
            /* make tables up to required level */
            while (k > w + l)
            {
                h++;
                w += l; /* previous table always l bits */

                /* compute minimum size table less than or equal to l bits */
                z = g - w;
                z = z > (uInt)l ? l : z; /* table size upper limit */
                if ((f = 1 << (j = k - w)) > a + 1) /* try a k-w bit table */
                {                   /* too few codes for k-w bit table */
                    f -= a + 1;     /* deduct codes from patterns left */
                    xp = c + k;
                    if (j < z)
                        while (++j < z) /* try smaller tables up to z bits */
                        {
                            if ((f <<= 1) <= *++xp)
                                break; /* enough codes to use up j bits */
                            f -= *xp;  /* else deduct codes from patterns */
                        }
                }
                z = 1 << j; /* table entries for j-bit table */

                /* allocate new table */
                if (*hn + z > MANY)    /* (note: doesn't matter for fixed) */
                    return Z_DATA_ERROR; /* overflow of MANY */
                u[h] = q = hp + *hn;
                *hn += z;

                /* connect to last table, if there is one */
                if (h)
                {
                    x[h] = i;         /* save pattern for backing up */
                    r.bits = (Byte)l; /* bits to dump before this table */
                    r.exop = (Byte)j; /* bits in this table */
                    j = i >> (w - l);
                    r.base = (uInt)(q - u[h - 1] - j); /* offset to this table */
                    u[h - 1][j] = r;  /* connect to last table */
                }
                else
                    *t = q; /* first table is returned result */
            }

            /* set up table entry in r */
            r.bits = (Byte)(k - w);
            if (p >= v + n)
                r.exop = 128 + 64; /* out of values--invalid code */
            else if (*p < s)
            {
                r.exop = (Byte)(*p < 256 ? 0 : 32 + 64); /* 256 is end-of-block */
                r.base = *p++; /* simple code is just the value */
            }
            else
            {
                r.exop = (Byte)(e[*p - s] + 16 + 64); /* non-simple--look up in lists */
                r.base = d[*p++ - s];
            }

            /* fill code-like entries with r */
            f = 1 << (k - w);
            for (j = i >> w; j < z; j += f)
                q[j] = r;

            /* backwards increment the k-bit code i */
            for (j = 1 << (k - 1); i & j; j >>= 1)
                i ^= j;
            i ^= j;

            /* backup over finished tables */
            mask = (1 << w) - 1; /* needed on HP, cc -O bug */
            while ((i & mask) != x[h])
            {
                h--; /* don't need to update q */
                w -= l;
                mask = (1 << w) - 1;
            }
        }
    }

    /* Return Z_BUF_ERROR if we were given an incomplete table */
    return y != 0 && g != 1 ? Z_BUF_ERROR : Z_OK;
}

/* inflate_trees_bits: the tree of the 19 code-length codes. */
int inflate_trees_bits(uIntf *c, uIntf *bb, inflate_huft **tb, inflate_huft *hp, z_streamp z)
{
    int r;
    uInt hn = 0; /* hufts used in space */
    uIntf *v;    /* work area for huft_build */

    if ((v = (uIntf *)ZALLOC(z, 19, sizeof(uInt))) == Z_NULL)
        return Z_MEM_ERROR;
    r = huft_build(c, 19, 19, (uIntf *)Z_NULL, (uIntf *)Z_NULL, tb, bb, hp, &hn, v);
    if (r == Z_DATA_ERROR)
        z->msg = (char *)"oversubscribed dynamic bit lengths tree";
    else if (r == Z_BUF_ERROR || *bb == 0)
    {
        z->msg = (char *)"incomplete dynamic bit lengths tree";
        r = Z_DATA_ERROR;
    }
    ZFREE(z, v);
    return r;
}

/* inflate_trees_dynamic: the literal/length and distance trees of a
 * dynamic block. */
int inflate_trees_dynamic(uInt nl, uInt nd, uIntf *c, uIntf *bl, uIntf *bd, inflate_huft **tl,
                  inflate_huft **td, inflate_huft *hp, z_streamp z)
{
    int r;
    uInt hn = 0; /* hufts used in space */
    uIntf *v;    /* work area for huft_build */

    /* allocate work area */
    if ((v = (uIntf *)ZALLOC(z, 288, sizeof(uInt))) == Z_NULL)
        return Z_MEM_ERROR;

    /* build literal/length tree */
    r = huft_build(c, nl, 257, cplens, cplext, tl, bl, hp, &hn, v);
    if (r != Z_OK || *bl == 0)
    {
        if (r == Z_DATA_ERROR)
            z->msg = (char *)"oversubscribed literal/length tree";
        else if (r != Z_MEM_ERROR)
        {
            z->msg = (char *)"incomplete literal/length tree";
            r = Z_DATA_ERROR;
        }
        ZFREE(z, v);
        return r;
    }

    /* build distance tree */
    r = huft_build(c + nl, nd, 0, cpdist, cpdext, td, bd, hp, &hn, v);
    if (r != Z_OK || (*bd == 0 && nl > 257))
    {
        if (r == Z_DATA_ERROR)
            z->msg = (char *)"oversubscribed distance tree";
        else if (r == Z_BUF_ERROR) {
            z->msg = (char *)"incomplete distance tree";
            r = Z_DATA_ERROR;
        }
        else if (r != Z_MEM_ERROR)
        {
            z->msg = (char *)"empty distance tree with lengths";
            r = Z_DATA_ERROR;
        }
        ZFREE(z, v);
        return r;
    }

    /* done */
    ZFREE(z, v);
    return Z_OK;
}
