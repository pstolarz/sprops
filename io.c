/*
   Copyright (c) 2016 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <errno.h>
#include "io.h"

/* exported; see props.h header for details */
sp_errc_t sp_fopen(SP_FILE *f, const char *filename, const char *mode)
{
    if (!f || !filename || !mode) return SPEC_INV_ARG;

    f->typ = SP_FILE_C;
    f->f = fopen(filename, mode);
    return (f->f ? SPEC_SUCCESS : SPEC_FOPEN_ERR);
}

/* exported; see props.h header for details */
sp_errc_t sp_fopen2(SP_FILE *f, FILE *cf)
{
    if (!f || !cf) return SPEC_INV_ARG;

    f->typ = SP_FILE_C;
    f->f = cf;
    return SPEC_SUCCESS;
}

/* exported; see props.h header for details */
sp_errc_t sp_mopen(SP_FILE *f, char *buf, size_t num)
{
    if (!f || (!buf && num>0)) return SPEC_INV_ARG;

    f->typ = SP_FILE_MEM;
    f->m.b = buf;
    f->m.num = num;
    f->m.i = 0;
    return SPEC_SUCCESS;
}

/* exported; see props.h header for details */
sp_errc_t sp_fclose(SP_FILE *f)
{
    if (!f || f->typ!=SP_FILE_C  || !f->f) return SPEC_INV_ARG;

    if (!fclose(f->f)) {
        f->f = NULL;
        return SPEC_SUCCESS;
    } else {
        return SPEC_ACCS_ERR;
    }
}

/* fgetc(3) analogous
   NOTE: As for the text stream, NULL termination char translates to EOF.
 */
int sp_fgetc(SP_FILE *f)
{
    int c;

    if (f->typ==SP_FILE_C) {
        c = fgetc(f->f);
        if (!c) {
            ungetc(c, f->f);
            c = EOF;
        }
    } else {
        if (f->m.i < f->m.num) {
            c = f->m.b[f->m.i] & 0xff;
            if (!c) c = EOF;
            else f->m.i++;
        } else {
            c = EOF;
        }
    }
    return c;
}

/* fputc(3) analogous */
int sp_fputc(int c, SP_FILE *f)
{
    if (f->typ==SP_FILE_C) {
        return fputc(c, f->f);
    } else {
        if (f->m.i < f->m.num) {
            return ((f->m.b[f->m.i++]=(char)c) & 0xff);
        } else {
            return EOF;
        }
    }
}

/* fputs(3) analogous */
int sp_fputs(const char *str, SP_FILE *f)
{
    if (f->typ==SP_FILE_C) {
        return fputs(str, f->f);
    } else {
        size_t i = f->m.num-f->m.i;
        for (; *str && i; i--, str++)
            f->m.b[f->m.i++] = *str;
        return (*str ? EOF : 0);
    }
}

/* fseek(3) analogous */
int sp_fseek(SP_FILE *f, long int offset, int origin)
{
    if (f->typ==SP_FILE_C) {
        return fseek(f->f, offset, origin);
    } else {
        switch (origin)
        {
        case SEEK_CUR:
            offset += f->m.i;
            /* fall-through */

        case SEEK_SET:
            if (offset < 0) {
                offset = 0;
            } else
            if (offset > f->m.num) {
                offset = f->m.num;
            }
            f->m.i = offset;
            return 0;

        default:
            return (errno=EINVAL);
        }
    }
}

/* ftell(3) analogous */
long int sp_ftell(SP_FILE *f)
{
    return (f->typ==SP_FILE_C ? ftell(f->f) : (long int)f->m.i);
}
