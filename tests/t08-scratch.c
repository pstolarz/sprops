/*
   Copyright (c) 2016,2022 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include "../config.h"
#include "sprops/trans.h"

#if CONFIG_NO_SEMICOL_ENDS_VAL || \
    !CONFIG_CUT_VAL_LEADING_SPACES || \
    (CONFIG_MAX_SCOPE_LEVEL_DEPTH>0 && CONFIG_MAX_SCOPE_LEVEL_DEPTH<2)
# error Bad configuration
#endif

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

/* indentation */
static unsigned long indf = SP_F_SPIND(4);

int main(void)
{
    sp_errc_t ret=SPEC_SUCCESS;

    int tr_init=0;
    sp_trans_t trans;

    SP_FILE out;

    /* initialize with an empty input*/
    EXEC_RG(sp_init_tr(&trans, NULL, NULL, NULL));
    tr_init++;

    EXEC_RG(sp_add_scope_tr(&trans,
        "TYPE", "SCOPE1",
        0,
        "/", NULL,
        indf|SP_F_EXTEOL));

    EXEC_RG(sp_add_prop_tr(&trans,
        "PROP2", "VAL",
        0,
        "/", NULL,
        indf|SP_F_EXTEOL));

    EXEC_RG(sp_add_prop_tr(&trans,
        "PROP1", "VAL",
        0,
        "/", NULL,
        indf));

    EXEC_RG(sp_add_scope_tr(&trans,
        "TYPE", "SCOPE2",
        SP_ELM_LAST,
        "/", NULL,
        indf|SP_F_EOLBFR));

    EXEC_RG(sp_add_scope_tr(&trans,
        "TYPE", "SCOPE3",
        SP_ELM_LAST,
        "/", NULL,
        indf|SP_F_EOLBFR|SP_F_EMPCPT));

    EXEC_RG(sp_add_prop_tr(&trans,
        "PROP1", "VAL",
        SP_ELM_LAST,
        "/SCOPE1", "TYPE",
        indf));

    EXEC_RG(sp_add_prop_tr(&trans,
        "PROP2", "VAL",
        SP_ELM_LAST,
        "/SCOPE1", "TYPE",
        indf));

    EXEC_RG(sp_add_scope_tr(&trans,
        "TYPE", "SCOPE1",
        0,
        "/SCOPE2", "TYPE",
        indf|SP_F_EMPCPT));

    EXEC_RG(sp_add_scope_tr(&trans,
        "TYPE", "SCOPE2",
        1,
        "/SCOPE2", "TYPE",
        indf|SP_F_SPLBRA));

    EXEC_RG(sp_add_prop_tr(&trans,
        "PROP1", "VAL",
        0,
        "/SCOPE2/SCOPE2", "TYPE",
        indf));

    sp_fopen2(&out, stdout);

    EXEC_RG(sp_commit_tr(&trans, &out));
    tr_init=0;

finish:
    if (tr_init) sp_discard_tr(&trans);
    if (ret) printf("Error: %d\n", ret);
    return 0;
}
