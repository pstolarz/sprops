/*
   Copyright (c) 2016,2019 Piotr Stolarz
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
#include "io.h"
#include "sprops/trans.h"
#include "sprops/utils.h"

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;
#define CHK_FERR(c) if ((c)==EOF) { ret=SPEC_ACCS_ERR; goto finish; }
#define CHK_FSEEK(c) if ((c)!=0) { ret=SPEC_ACCS_ERR; goto finish; }

#define FSTATE_EMPT  0
#define FSTATE_TEMP  1
#define FSTATE_PERM  2

#define IN_I(t)     ((t)->fs_i)
#define OUT_I(t)    ((t)->fs_i^1)

#define IN_F(t)     (&(t)->fs[IN_I(t)].f)
#define OUT_F(t)    (&(t)->fs[OUT_I(t)].f)

#define IN_ST(t)    ((t)->fs[IN_I(t)].state)
#define OUT_ST(t)   ((t)->fs[OUT_I(t)].state)

#define FLAGS(t, f) ((f) | SP_F_USEEOL((t)->eol_typ) | \
    (!(t)->parsc.first_column || IN_ST(t)==FSTATE_PERM ? 0 : SP_F_NLSTEOL))

#define PARSC(t) \
    (!(t)->parsc.first_column || IN_ST(t)==FSTATE_TEMP ? NULL : &(t)->parsc)

#define PREP_OUT(t) \
    if (!(t) || IN_ST(t)==FSTATE_EMPT) { ret=SPEC_INV_ARG; goto finish; } \
    if (OUT_ST(t)==FSTATE_TEMP) (t)->ths.close((t)->ths.arg, OUT_F(t)); \
    OUT_ST(t) = FSTATE_EMPT; \
    EXEC_RG((t)->ths.open((t)->ths.arg, OUT_F(t))); \
    OUT_ST(t) = FSTATE_TEMP;

/* IN <-> OUT */
#define PART_COMMIT(t) \
    (t)->fs_i^=1; \
    (t)->n_commits++;

#if CONFIG_TRANS_PARSC_MOD==PARSC_EXTIND
/* Extend a parsing scope by indentation white spaces preceding the scope.
 */
static sp_errc_t parsc_extind(SP_FILE *in, sp_loc_t *p_parsc)
{
    sp_errc_t ret=SPEC_SUCCESS;
    int n=p_parsc->first_column-1;

    if (n<=0) goto finish;

    CHK_FSEEK(sp_fseek(in, p_parsc->beg-n, SEEK_SET));
    for (; n>0 && isspace(sp_fgetc(in)); n--);

    if (!n) {
        p_parsc->beg -= p_parsc->first_column-1;
        p_parsc->first_column = 1;
    }
finish:
    return ret;
}
#elif CONFIG_TRANS_PARSC_MOD==PARSC_AS_INPUT
/* Copy indented (or partially indented) version of a parsing scope to a
   temporary stream created by the function and written under 'f'. 'p_ind_sz'
   will get number of indentation white spaces written before the parsing scope
   in the temporary stream. The result may be used as an input stream equivalent
   to the parsing scope.

   In case there is no need to create the stream and the parsing scope may be
   used as a part of the input stream in the ordinary way, the function returns
   SPEC_SUCCESS with 'p_fstate' set to FSTATE_EMPT and 'p_ind_sz' to zero.
 */
static sp_errc_t cpy_ind_parsc(SP_FILE *in, const sp_loc_t *p_parsc,
        const sp_trans_ths_t *p_ths, SP_FILE *f, int *p_fstate, int *p_ind_sz)
{
    sp_errc_t ret=SPEC_SUCCESS;
    int c, n=p_parsc->first_column-1;

    *p_fstate = FSTATE_EMPT;
    *p_ind_sz = 0;

    if (n<=0) goto finish;

    CHK_FSEEK(sp_fseek(in, p_parsc->beg-n, SEEK_SET));

    EXEC_RG(p_ths->open(p_ths->arg, f));
    *p_fstate = FSTATE_TEMP;

    for (; n>0 && isspace(c=sp_fgetc(in)); n--) {
        CHK_FERR(sp_fputc(c, f));
        *p_ind_sz += 1;
    }

    EXEC_RG(sp_util_cpy_to_out(in, f, p_parsc->beg, p_parsc->end+1, NULL));

finish:
    if (ret!=SPEC_SUCCESS && *p_fstate==FSTATE_TEMP) {
        p_ths->close(p_ths->arg, f);
        *p_fstate = FSTATE_EMPT;
    }
    return ret;
}
#endif

/* default handlers */
static sp_errc_t th_open(void *arg, SP_FILE *f) {
    FILE *cf = tmpfile();
    return (cf ? sp_fopen2(f, cf) : SPEC_FOPEN_ERR);
}
static void th_close(void *arg, SP_FILE *f) {
    sp_close(f);
}

/* exported; see header for details */
sp_errc_t sp_init_tr(sp_trans_t *p_trans, SP_FILE *in,
    const sp_loc_t *p_parsc, const sp_trans_ths_t *p_ths)
{
    sp_errc_t ret=SPEC_SUCCESS;

    if (!p_trans || (!in && p_parsc) ||
        (p_ths && (!p_ths->open || !p_ths->close)))
    {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    memset(p_trans, 0, sizeof(*p_trans));
    p_trans->in = in;

    if (!p_ths) {
        p_trans->ths.open = th_open;
        p_trans->ths.close = th_close;
    } else
        p_trans->ths = *p_ths;

    if (p_parsc)
    {
        p_trans->parsc = *p_parsc;
#if CONFIG_TRANS_PARSC_MOD==PARSC_AS_INPUT
        EXEC_RG(cpy_ind_parsc(in, p_parsc, &p_trans->ths,
            IN_F(p_trans), &IN_ST(p_trans), &p_trans->skip_in));
#elif CONFIG_TRANS_PARSC_MOD==PARSC_EXTIND
        EXEC_RG(parsc_extind(in, &p_trans->parsc));
#endif
    }

    if (!in) {
        p_trans->eol_typ = EOL_PLAT;

        /* input as an empty stream */
        sp_mopen(IN_F(p_trans), NULL, 0);
        IN_ST(p_trans) = FSTATE_PERM;
    } else {
        EXEC_RG(sp_util_detect_eol(in, &p_trans->eol_typ));

        if (IN_ST(p_trans)==FSTATE_EMPT) {
            *IN_F(p_trans) = *in;
            IN_ST(p_trans) = FSTATE_PERM;
        }
    }

finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_commit_tr(sp_trans_t *p_trans, SP_FILE *out)
{
    sp_errc_t ret=SPEC_SUCCESS;

    if (!p_trans || IN_ST(p_trans)==FSTATE_EMPT) {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    if (out)
    {
        if (!p_trans->n_commits)
        {
            /* no modification, input equals output */
            if (p_trans->in) {
                EXEC_RG(sp_util_cpy_to_out(p_trans->in, out, 0, EOF, NULL));
            }
        } else
        if (!p_trans->parsc.first_column)
        {
            /* global scope modification */
            EXEC_RG(sp_util_cpy_to_out(
                IN_F(p_trans), out, p_trans->skip_in, EOF, NULL));
        } else
        if (p_trans->in)
        {
            /* modification inside the parsing scope;
               copy original input with the modified scope */
            EXEC_RG(sp_util_cpy_to_out(
                p_trans->in, out, 0, p_trans->parsc.beg, NULL));

            EXEC_RG(sp_util_cpy_to_out(
                IN_F(p_trans), out, p_trans->skip_in, EOF, NULL));

            EXEC_RG(sp_util_cpy_to_out(
                p_trans->in, out, p_trans->parsc.end+1, EOF, NULL));
        }
    }

    /* close the handle */
    if (IN_ST(p_trans)==FSTATE_TEMP) {
        p_trans->ths.close(p_trans->ths.arg, IN_F(p_trans));
    }
    if (OUT_ST(p_trans)==FSTATE_TEMP) {
        p_trans->ths.close(p_trans->ths.arg, OUT_F(p_trans));
    }
    memset(p_trans, 0, sizeof(*p_trans));

finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_commit2_tr(sp_trans_t *p_trans, const char *new_file)
{
    sp_errc_t ret = SPEC_SUCCESS;

    if (new_file) {
        SP_FILE out;
        ret = sp_fopen(&out, new_file, SP_MODE_WRITE_NEW);
        if (ret==SPEC_SUCCESS)
            ret = sp_commit_tr(p_trans, &out);
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

    PREP_OUT(p_trans);
    EXEC_RG(sp_add_prop(IN_F(p_trans), OUT_F(p_trans), PARSC(p_trans),
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

    PREP_OUT(p_trans);
    EXEC_RG(sp_add_scope(IN_F(p_trans), OUT_F(p_trans), PARSC(p_trans),
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

    PREP_OUT(p_trans);
    ret = sp_rm_prop(IN_F(p_trans), OUT_F(p_trans), PARSC(p_trans),
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

    PREP_OUT(p_trans);
    ret = sp_rm_scope(IN_F(p_trans), OUT_F(p_trans), PARSC(p_trans),
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

    PREP_OUT(p_trans);
    EXEC_RG(sp_set_prop(IN_F(p_trans), OUT_F(p_trans), PARSC(p_trans),
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

    PREP_OUT(p_trans);
    EXEC_RG(sp_mv_prop(IN_F(p_trans), OUT_F(p_trans), PARSC(p_trans),
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

    PREP_OUT(p_trans);
    EXEC_RG(sp_mv_scope(IN_F(p_trans), OUT_F(p_trans), PARSC(p_trans),
        type, name, new_type, new_name, ind, path, deftp, FLAGS(p_trans, flags)));
    PART_COMMIT(p_trans);

finish:
    return ret;
}
