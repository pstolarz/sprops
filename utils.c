/*
   Copyright (c) 2016 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "io.h"
#include "sprops/utils.h"

#define CHK_FSEEK(c) if ((c)!=0) { ret=SPEC_ACCS_ERR; goto finish; }

/* exported; see header for details */
sp_errc_t
    sp_util_cpy_to_out(SP_FILE *in, SP_FILE *out, long beg, long end, long *p_n)
{
    int ret=SPEC_SUCCESS;
    long off=beg;

    if (!in || !out) {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    if (p_n) *p_n=0;

    if (off<end || end==EOF)
    {
        int c;
        CHK_FSEEK(sp_fseek(in, off, SEEK_SET));

        for (; off<end || end==EOF; off++)
        {
            c = sp_fgetc(in);
            if (c==EOF && end==EOF) break;
            if (c==EOF || sp_fputc(c, out)==EOF) {
                ret=SPEC_ACCS_ERR;
                goto finish;
            }
        }
    }

    if (p_n) *p_n=off-beg;
finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_util_detect_eol(SP_FILE *in, sp_eol_t *p_eol_typ)
{
    int c;
    sp_errc_t ret=SPEC_SUCCESS;

    if (!in || !p_eol_typ) {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    *p_eol_typ = EOL_NDETECT;

    CHK_FSEEK(sp_fseek(in, 0, SEEK_SET));
    while ((c=sp_fgetc(in))!=EOF)
    {
        if (c=='\n') {
            *p_eol_typ=EOL_LF;
            break;
        } else
        if (c=='\r') {
            *p_eol_typ=EOL_CR;
            if (sp_fgetc(in)=='\n') *p_eol_typ=EOL_CRLF;
            break;
        }
    }
finish:
    return ret;
}

/* exported; see header for details */
int sp_util_stricmp(const char *str1, const char *str2)
{
    int ret;
    size_t i=0;

    if (!str1 && !str2) return 0;
    if (!str1) return INT_MIN;
    if (!str2) return INT_MAX;

    for (; !((ret=tolower((int)str1[i])-tolower((int)str2[i]))) &&
        str1[i] && str2[i]; i++);
    return ret;
}

/* exported; see header for details */
size_t sp_util_strtrim(char **p_str, int trim_lead)
{
    size_t len;

    if (!p_str || !*p_str) return 0;

    if (trim_lead)
        for (; isspace((int)**p_str); (*p_str)++);

    len = strlen(*p_str);
    for (; len && isspace((int)(*p_str)[len-1]); len--);

    (*p_str)[len] = 0;
    return len;
}

/* exported; see header for details */
sp_errc_t sp_util_parse_int(const char *str, long *p_val)
{
    char *end;
    *p_val = 0L;

    if (!str || !p_val)
        return SPEC_INV_ARG;

    errno = 0;
    *p_val = strtol(str, &end, 0);

    if (errno==ERANGE)
        return SPEC_VAL_ERR;
    else if (*end)
        return SPEC_VAL_ERR;

    return SPEC_SUCCESS;
}

/* exported; see header for details */
sp_errc_t sp_util_parse_float(const char *str, double *p_val)
{
    char *end;
    *p_val=0.0;

    if (!str || !p_val)
        return SPEC_INV_ARG;

    errno = 0;
    *p_val = strtod(str, &end);

    if (errno==ERANGE)
        return SPEC_VAL_ERR;
    else if (*end)
        return SPEC_VAL_ERR;

    return SPEC_SUCCESS;
}

/* exported; see header for details */
sp_errc_t sp_util_parse_enum(
    const char *str, const sp_enumval_t *p_evals, int igncase, int *p_val)
{
    if (!str || !p_evals || !p_val)
        return SPEC_INV_ARG;

    for (; p_evals->name; p_evals++) {
        if (!(igncase ? sp_util_stricmp(p_evals->name, str) :
            strcmp(p_evals->name, str))) { *p_val=p_evals->val; break; }
    }
    return (!p_evals->name ? SPEC_VAL_ERR : SPEC_SUCCESS);
}
