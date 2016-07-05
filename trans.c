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
#include <string.h>
#include "config.h"
#include "sprops/trans.h"
#include "sprops/utils.h"

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;
#define CHK_FERR(c) if ((c)==EOF) { ret=SPEC_ACCS_ERR; goto finish; }
#define CHK_FOPEN(c) if ((c)==0) { ret=SPEC_FOPEN_ERR; goto finish; }
#define CHK_FSEEK(c) if ((c)!=0) { ret=SPEC_ACCS_ERR; goto finish; }

#define IN(t)    ((t)->tfs[(t)->tfs_i])
#define OUT(t)   ((t)->tfs[(t)->tfs_i^1])

#define FLAGS(t, f) (SP_F_USEEOL((t)->eol_typ) | \
    (!(t)->parsc.first_column || (IN(t)==(t)->in) ? (f) : (f)|SP_F_NLSTEOL))

#define PARSC(t) \
    (!(t)->parsc.first_column || (IN(t)!=(t)->in) ? NULL : &(t)->parsc)

#define PREP_STREAMS(t) \
    if (!(t) || !IN(t)) { ret=SPEC_INV_ARG; goto finish; } \
    CHK_FSEEK(fseek(IN(t), 0, SEEK_SET)); \
    if (OUT(t)!=NULL && OUT(t)!=(t)->in) (t)->tmpf.close((t)->tmpf.arg, OUT(t)); \
    CHK_FOPEN(OUT(t)=(t)->tmpf.open((t)->tmpf.arg));

/* IN <-> OUT */
#define PART_COMMIT(t) \
    (t)->tfs_i^=1; \
    (t)->n_commits+=1;

#if CONFIG_TRANS_PARSC_MOD==PARSC_EXTIND
/* Extend a parsing scope by indentation white spaces preceding the scope.
 */
static sp_errc_t parsc_extind(FILE *in, sp_loc_t *p_parsc)
{
    sp_errc_t ret=SPEC_SUCCESS;
    int n=p_parsc->first_column-1;

    if (n<=0) goto finish;

    CHK_FSEEK(fseek(in, p_parsc->beg-n, SEEK_SET));
    for (; n>0 && isspace(fgetc(in)); n--);

    if (!n) {
        p_parsc->beg -= p_parsc->first_column-1;
        p_parsc->first_column = 1;
    }
finish:
    return ret;
}
#elif CONFIG_TRANS_PARSC_MOD==PARSC_AS_INPUT
/* Copy indented (or partially indented) version of a parsing scope to a
   temporary stream created by the function and written under 'p_tmp'.
   'p_ind_sz' will get number of indentation white spaces written before the
   parsing scope in the temporary stream. The result may be used as an input
   stream equivalent to the parsing scope.

   In case there is no need to create the stream and the parsing scope may be
   used as part of the input stream in an ordinary way, the function returns
   SPEC_SUCCESS with 'p_tmp', 'p_ind_sz' zeroed.
 */
static sp_errc_t cpy_ind_parsc(FILE *in, const sp_loc_t *p_parsc,
        const sp_trans_tmpf_t *p_tmpf, FILE **p_tmp, int *p_ind_sz)
{
    sp_errc_t ret=SPEC_SUCCESS;
    int c, n=p_parsc->first_column-1;

    *p_ind_sz = 0;
    *p_tmp = NULL;

    if (n<=0) goto finish;

    CHK_FOPEN(*p_tmp=p_tmpf->open(p_tmpf->arg));

    CHK_FSEEK(fseek(in, p_parsc->beg-n, SEEK_SET));
    for (; n>0 && isspace(c=fgetc(in)); n--) {
        CHK_FERR(fputc(c, *p_tmp));
        *p_ind_sz+=1;
    }

    EXEC_RG(sp_util_cpy_to_out(in, *p_tmp, p_parsc->beg, p_parsc->end+1, NULL));

finish:
    if (ret!=SPEC_SUCCESS && *p_tmp) {
        p_tmpf->close(p_tmpf->arg, *p_tmp);
        *p_tmp = NULL;
    }
    return ret;
}
#endif

/* default handlers */
static FILE* def_tmpf_open(void *arg) { return tmpfile(); }
static void def_tmpf_close(void *arg, FILE *f) { fclose(f); }

/* exported; see header for details */
sp_errc_t sp_init_tr(sp_trans_t *p_trans, FILE *in,
    const sp_loc_t *p_parsc, const sp_trans_tmpf_t *p_tmpf)
{
    sp_errc_t ret=SPEC_SUCCESS;

    if (!p_trans || (!in && p_parsc)) {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    memset(p_trans, 0, sizeof(*p_trans));
    p_trans->in = in;
    p_trans->eol_typ = EOL_PLAT;

    if (!p_tmpf) {
        p_trans->tmpf.open = def_tmpf_open;
        p_trans->tmpf.close = def_tmpf_close;
    } else
        p_trans->tmpf = *p_tmpf;

    if (p_parsc)
    {
        p_trans->parsc = *p_parsc;
#if CONFIG_TRANS_PARSC_MOD==PARSC_AS_INPUT
        EXEC_RG(cpy_ind_parsc(
            in, p_parsc, &p_trans->tmpf, &p_trans->tfs[0], &p_trans->skip_in));
#elif CONFIG_TRANS_PARSC_MOD==PARSC_EXTIND
        EXEC_RG(parsc_extind(in, &p_trans->parsc));
#endif
    }

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
        EXEC_RG(sp_util_detect_eol(in, &p_trans->eol_typ));
        if (!p_trans->tfs[0]) p_trans->tfs[0]=in;
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

    if (out)
    {
        if (!p_trans->n_commits)
        {
            /* no modifications, input equals output */
            if (p_trans->in) {
                EXEC_RG(sp_util_cpy_to_out(p_trans->in, out, 0, EOF, NULL));
            }
        } else
        if (!p_trans->parsc.first_column)
        {
            /* global scope modifications */
            EXEC_RG(sp_util_cpy_to_out(
                IN(p_trans), out, p_trans->skip_in, EOF, NULL));
        } else
        if (p_trans->in)
        {
            /* modifications inside a parsing scope;
               copy original input with the modified scope */
            EXEC_RG(sp_util_cpy_to_out(
                p_trans->in, out, 0, p_trans->parsc.beg, NULL));

            EXEC_RG(sp_util_cpy_to_out(
                IN(p_trans), out, p_trans->skip_in, EOF, NULL));

            EXEC_RG(sp_util_cpy_to_out(
                p_trans->in, out, p_trans->parsc.end+1, EOF, NULL));
        }
    }

    if (IN(p_trans)!=p_trans->in)
        p_trans->tmpf.close(p_trans->tmpf.arg, IN(p_trans));

    if (OUT(p_trans)!=NULL && OUT(p_trans)!=p_trans->in)
        p_trans->tmpf.close(p_trans->tmpf.arg, OUT(p_trans));

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
