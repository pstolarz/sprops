/*
   Copyright (c) 2016 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <string.h>
#include "sprops/trans.h"
#include "sprops/utils.h"

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

#define IN(t)    ((t)->tfs[(t)->ind])
#define OUT(t)   ((t)->tfs[(t)->ind^1])

#define FLAGS(t, f) \
    (!(t)->parsc.first_column || (IN(t)==(t)->in) ? (f) : (f)|SP_F_NLSTEOL)

#define PARSC(t) \
    (!(t)->parsc.first_column || (IN(t)!=(t)->in) ? NULL : &(t)->parsc)

#define PREP_STREAMS(t) \
    if (!(t)) { ret=SPEC_INV_ARG; goto finish; } \
    if (fseek(IN(t), 0, SEEK_SET)!=0) { ret=SPEC_ACCS_ERR; goto finish; } \
    if (OUT(t)!=NULL && OUT(t)!=(t)->in) fclose(OUT(t)); \
    if (!(OUT(t)=tmpfile())) { ret=SPEC_FOPEN_ERR; goto finish; }

/* IN <-> OUT */
#define PART_COMMIT(t) (t)->ind ^= 1;

/* exported; see header for details */
sp_errc_t sp_init_tr(sp_trans_t *p_trans, FILE *in, const sp_loc_t *p_parsc)
{
    sp_errc_t ret=SPEC_SUCCESS;

    if (!p_trans || (!in && p_parsc)) {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    memset(p_trans, 0, sizeof(*p_trans));

    p_trans->in = in;
    if (p_parsc) p_trans->parsc = *p_parsc;

    if (!in) {
        /* in==NULL means an empty input */
        if (!(p_trans->tfs[0] = fopen(
#if defined(_WIN32) || defined(_WIN64)
            "nul"
#else
            "/dev/null"
#endif
            , "wb")))
        {
            ret=SPEC_FOPEN_ERR;
            goto finish;
        }
    } else {
        p_trans->tfs[0] = in;
    }
finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_commit_tr(sp_trans_t *p_trans, FILE *out)
{
    sp_errc_t ret=SPEC_SUCCESS;

    if (!p_trans || !IN(p_trans)) {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    if (out) {
        if (!p_trans->parsc.first_column || IN(p_trans)==p_trans->in)
        {
            EXEC_RG(sp_util_cpy_to_out(IN(p_trans), out, 0, EOF, NULL));
        } else
        if (p_trans->in)
        {
            /* copy original input with the modified scope */
            EXEC_RG(sp_util_cpy_to_out(
                p_trans->in, out, 0, p_trans->parsc.beg, NULL));

            EXEC_RG(sp_util_cpy_to_out(IN(p_trans), out, 0, EOF, NULL));

            EXEC_RG(sp_util_cpy_to_out(
                p_trans->in, out, p_trans->parsc.end+1, EOF, NULL));
        }
    }

    if (IN(p_trans)!=p_trans->in)
        fclose(IN(p_trans));

    if (OUT(p_trans)!=NULL && OUT(p_trans)!=p_trans->in)
        fclose(OUT(p_trans));

    memset(p_trans, 0, sizeof(*p_trans));
finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_commit2_tr(sp_trans_t *p_trans, const char *new_file)
{
    sp_errc_t ret = SPEC_SUCCESS;

    if (new_file) {
        FILE *out = fopen(new_file, "wb+");
        ret = (out ? sp_commit_tr(p_trans, out) : SPEC_FOPEN_ERR);
    } else {
        ret = sp_commit_tr(p_trans, NULL);
    }
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_add_prop_tr(sp_trans_t *p_trans, const char *name, const char *val,
    int n_elem, const char *path, const char *deftp, unsigned long flags)
{
    sp_errc_t ret=SPEC_SUCCESS;

    PREP_STREAMS(p_trans);
    EXEC_RG(sp_add_prop(IN(p_trans), OUT(p_trans), PARSC(p_trans),
        name, val, n_elem, path, deftp, FLAGS(p_trans, flags)));
    PART_COMMIT(p_trans);

finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_add_scope_tr(sp_trans_t *p_trans, const char *type, const char *name,
    int n_elem, const char *path, const char *deftp, unsigned long flags)
{
    sp_errc_t ret=SPEC_SUCCESS;

    PREP_STREAMS(p_trans);
    EXEC_RG(sp_add_scope(IN(p_trans), OUT(p_trans), PARSC(p_trans),
        type, name, n_elem, path, deftp, FLAGS(p_trans, flags)));
    PART_COMMIT(p_trans);

finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_rm_prop_tr(sp_trans_t *p_trans, const char *name, int ind,
    const char *path, const char *deftp, unsigned long flags)
{
    sp_errc_t ret=SPEC_SUCCESS;

    PREP_STREAMS(p_trans);
    ret = sp_rm_prop(IN(p_trans), OUT(p_trans), PARSC(p_trans),
        name, ind, path, deftp, FLAGS(p_trans, flags));
    if (ret==SPEC_SUCCESS || ret==SPEC_NOTFOUND) {
        PART_COMMIT(p_trans);
    }

finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_rm_scope_tr(sp_trans_t *p_trans, const char *type, const char *name,
    int ind, const char *path, const char *deftp, unsigned long flags)
{
    sp_errc_t ret=SPEC_SUCCESS;

    PREP_STREAMS(p_trans);
    ret = sp_rm_scope(IN(p_trans), OUT(p_trans), PARSC(p_trans),
        type, name, ind, path, deftp, FLAGS(p_trans, flags));
    if (ret==SPEC_SUCCESS || ret==SPEC_NOTFOUND) {
        PART_COMMIT(p_trans);
    }

finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_set_prop_tr(sp_trans_t *p_trans, const char *name, const char *val,
    int ind, const char *path, const char *deftp, unsigned long flags)
{
    sp_errc_t ret=SPEC_SUCCESS;

    PREP_STREAMS(p_trans);
    EXEC_RG(sp_set_prop(IN(p_trans), OUT(p_trans), PARSC(p_trans),
        name, val, ind, path, deftp, FLAGS(p_trans, flags)));
    PART_COMMIT(p_trans);

finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_mv_prop_tr(sp_trans_t *p_trans, const char *name,
    const char *new_name, int ind, const char *path, const char *deftp,
    unsigned long flags)
{
    sp_errc_t ret=SPEC_SUCCESS;

    PREP_STREAMS(p_trans);
    EXEC_RG(sp_mv_prop(IN(p_trans), OUT(p_trans), PARSC(p_trans),
        name, new_name, ind, path, deftp, FLAGS(p_trans, flags)));
    PART_COMMIT(p_trans);

finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_mv_scope_tr(sp_trans_t *p_trans, const char *type,
    const char *name, const char *new_type, const char *new_name, int ind,
    const char *path, const char *deftp, unsigned long flags)
{
    sp_errc_t ret=SPEC_SUCCESS;

    PREP_STREAMS(p_trans);
    EXEC_RG(sp_mv_scope(IN(p_trans), OUT(p_trans), PARSC(p_trans),
        type, name, new_type, new_name, ind, path, deftp, FLAGS(p_trans, flags)));
    PART_COMMIT(p_trans);

finish:
    return ret;
}
