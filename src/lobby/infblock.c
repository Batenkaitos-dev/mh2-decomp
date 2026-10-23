/* zlib 1.1.4's infblock.c, its first function: inflate_blocks_reset (the
 * rest of zlib's infblock.c starts inflate.c). Types and macros: include/zlib.h.
 *
 * Altered source version: zlib's code with an ANSI definition, in the file
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

/* inflate_blocks_reset: frees what the current block holds and starts
 * over at TYPE; *c gets the check value so far. */
void inflate_blocks_reset(inflate_blocks_statef *s, z_streamp z, uLongf *c)
{
    if (c != Z_NULL)
        *c = s->check;
    if (s->mode == BTREE || s->mode == DTREE)
        ZFREE(z, s->sub.trees.blens);
    if (s->mode == CODES)
        inflate_codes_free(s->sub.decode.codes, z);
    s->mode = TYPE;
    s->bitk = 0;
    s->bitb = 0;
    s->read = s->write = s->window;
    if (s->checkfn != Z_NULL)
        z->adler = s->check = (*s->checkfn)(0L, (const Bytef *)Z_NULL, 0);
}
