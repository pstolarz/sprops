/*
   Copyright (c) 2016 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <assert.h>
#include "../config.h"
#include "sprops/trans.h"

#if defined(CONFIG_NO_SEMICOL_ENDS_VAL) || \
    !defined(CONFIG_CUT_VAL_LEADING_SPACES) || \
    (CONFIG_MAX_SCOPE_LEVEL_DEPTH>0 && CONFIG_MAX_SCOPE_LEVEL_DEPTH<2) || \
    (CONFIG_TRANS_PARSC_MOD!=PARSC_EXTIND && \
         CONFIG_TRANS_PARSC_MOD!=PARSC_AS_INPUT)
# error Bad configuration
#endif

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

/* indentation */
static unsigned long indf = SP_F_SPIND(4);

int main(void)
{
    sp_errc_t ret=SPEC_SUCCESS;
    sp_scope_info_ex_t sc3;

    int tr_init=0;
    sp_trans_t trans;

    SP_FILE in, out;
    int in_opn=0;

    EXEC_RG(sp_fopen(&in, "t09.conf", SP_MODE_READ));
    in_opn++;

    sp_fopen2(&out, stdout);

    EXEC_RG(sp_init_tr(&trans, &in, NULL, NULL));
    tr_init++;

    printf("--- Global scope modification\n");
    EXEC_RG(sp_rm_prop_tr(&trans,
        "2",
        SP_IND_ALL,
        "/", NULL,
        SP_F_EXTEOL));

    EXEC_RG(sp_rm_scope_tr(&trans,
        "scope", "3",
        0,
        "/", NULL,
        0));

    EXEC_RG(sp_set_prop_tr(&trans,
        "1", "VAL",
        0,
        "/", NULL,
        SP_F_NOADD));

    EXEC_RG(sp_mv_prop_tr(&trans,
        "1", "PROP",
        0,
        "/", NULL,
        0));

    EXEC_RG(sp_mv_scope_tr(&trans,
        "scope", "3",
        "SCOPE", "3",
        0,
        "/", NULL,
        0));

    EXEC_RG(sp_add_prop_tr(&trans,
        "PROP2", "VAL",
        SP_ELM_LAST,
        "/", NULL,
        indf));

    EXEC_RG(sp_commit_tr(&trans, &out));
    tr_init=0;


    printf("\n--- Modification constrained to /scope:3\n");
    EXEC_RG(sp_get_scope_info(
        &in, NULL, "scope", "3", SP_IND_LAST, NULL, NULL, &sc3));
    assert(sc3.body_pres!=0);

    EXEC_RG(sp_init_tr(&trans, &in, &sc3.lbody, NULL));
    tr_init++;

    EXEC_RG(sp_set_prop_tr(&trans,
        "1", "VAL",
        0,
        NULL, NULL,
        indf));

    EXEC_RG(sp_set_prop_tr(&trans,
        "PROP", "VAL",
        0,
        NULL, NULL,
        indf));

    /* error doesn't change modifications already committed */
    assert(sp_set_prop_tr(&trans,
        "x", NULL,
        0,
        "/", NULL,
        SP_F_NOADD)==SPEC_NOTFOUND);

    EXEC_RG(sp_add_scope_tr(&trans,
        "TYPE", "SCOPE",
        SP_ELM_LAST,
        NULL, NULL,
        indf|SP_F_EMPCPT));

    EXEC_RG(sp_add_scope_tr(&trans,
        NULL, "SCOPE",
        SP_ELM_LAST,
        NULL, NULL,
        indf));

    EXEC_RG(sp_add_prop_tr(&trans,
        "PROP", "VAL",
        0,
        "SCOPE", NULL,
        indf));

    EXEC_RG(sp_commit_tr(&trans, &out));
    tr_init=0;

finish:
    if (tr_init) sp_discard_tr(&trans);
    if (in_opn) sp_fclose(&in);
    if (ret) printf("Error: %d\n", ret);
    return 0;
}
