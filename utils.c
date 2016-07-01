/*
   Copyright (c) 2016 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include "sprops/utils.h"

/* exported; see header for details */
sp_errc_t sp_util_cpy_to_out(FILE *in, FILE *out, long beg, long end, long *p_n)
{
    int ret=SPEC_SUCCESS;
    long off=beg;

    if (p_n) *p_n=0;

    if (off<end || end==EOF)
    {
        if (fseek(in, off, SEEK_SET)!=0) {
            ret=SPEC_ACCS_ERR;
            goto finish;
        }

        for (; off<end || end==EOF; off++) {
            int c = fgetc(in);
            if (c==EOF && end==EOF) break;
            if (c==EOF || fputc(c, out)==EOF) {
                ret=SPEC_ACCS_ERR;
                goto finish;
            }
        }
    }

    if (p_n) *p_n=off-beg;
finish:
    return ret;
}
