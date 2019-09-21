/*
   Copyright (c) 2016,2019 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <assert.h>
#include "../config.h"
#include "sprops/props.h"

#if defined(CONFIG_NO_SEMICOL_ENDS_VAL) || \
    !defined(CONFIG_CUT_VAL_LEADING_SPACES) || \
    (CONFIG_MAX_SCOPE_LEVEL_DEPTH>0 && CONFIG_MAX_SCOPE_LEVEL_DEPTH<1)
# error Bad configuration
#endif

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

int main(void)
{
    sp_errc_t ret=SPEC_SUCCESS;
    sp_scope_info_ex_t sc;

    SP_FILE in, out;
    int in_opn=0;

    EXEC_RG(sp_fopen(&in, "t06.conf", SP_MODE_READ));
    in_opn++;

    sp_fopen2(&out, stdout);

    printf("--- Set prop 1, own-scope: /, elm:0\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "1", "VAL",
        0,
        "/", NULL,
        0));

    printf("--- Set prop 1, own-scope: /, elm:0, flags:NOSEMC\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "1", "VAL",
        0,
        "/", NULL,
        SP_F_NOSEMC));

    printf("\n--- Set prop 1, own-scope: /, elm:1, flags:NVSRSP\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "1", "VAL",
        1,
        "/", NULL,
        SP_F_NVSRSP));

    printf("\n--- No-val-set prop 1, own-scope: /, elm:ALL\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "1", NULL,
        SP_IND_ALL,
        "/", NULL,
        0));

    printf("\n--- No-val-set/add prop 1, own-scope: /, elm:2\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "1", "",
        2,
        "/", NULL,
        0));

    /* same as above but with SP_F_NOADD flag */
    assert(sp_set_prop(&in, &out,
        NULL,
        "1", NULL,
        2,
        "/", NULL,
        SP_F_NOADD)==SPEC_NOTFOUND);

    /* index conflict, adding not possible */
    assert(sp_set_prop(&in, &out,
        NULL,
        "1", "VAL",
        3,
        "/", NULL,
        0)==SPEC_NOTFOUND);

    printf("\n--- Set prop 2, own-scope: /, elm:LAST\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "2", "VAL",
        SP_IND_LAST,
        "/", NULL,
        0));

    printf("\n--- Set/add prop 2, own-scope: /, elm:1\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "2", "VAL",
        1,
        "/", NULL,
        0));

    /* same as above but with SP_F_NOADD flag */
    assert(sp_set_prop(&in, &out,
        NULL,
        "2", "VAL",
        1,
        "/", NULL,
        SP_F_NOADD)==SPEC_NOTFOUND);

    /* index conflict, adding not possible */
    assert(sp_set_prop(&in, &out,
        NULL,
        "2", "VAL",
        2,
        "/", NULL,
        0)==SPEC_NOTFOUND);

    printf("\n--- Set/add prop 3, own-scope: /, elm:LAST\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "3", "VAL",
        SP_IND_LAST,
        "/", NULL,
        0));

    /* index conflict, adding not possible */
    assert(sp_set_prop(&in, &out,
        NULL,
        "3", "VAL",
        1,
        "/", NULL,
        0)==SPEC_NOTFOUND);

    printf("\n--- Set prop 1, own-scope: /:scope, elm:0\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "1", "VAL",
        0,
        "scope", "",
        0));

    printf("\n--- Set prop 1, own-scope: /:scope, elm:LAST, flags:NVSRSP\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "1", "VAL",
        SP_IND_LAST,
        "scope", NULL,
        SP_F_NVSRSP));

    printf("\n--- Set prop 1, own-scope: /:scope, elm:ALL\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "1", "VAL",
        SP_IND_ALL,
        "scope", "",
        0));

    printf("\n--- No-val-set prop 1, own-scope: /:scope, elm:0\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "1", "",
        0,
        "scope", NULL,
        0));

    printf("\n--- No-val-set prop 1, own-scope: /:scope, elm:ALL\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "1", NULL,
        SP_IND_ALL,
        "scope", "",
        0));

    printf("\n--- Set prop 2, own-scope: /:scope, elm:0\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "2", "VAL",
        0,
        "scope", "",
        0));

    printf("\n--- No-val-set prop 2, own-scope: /:scope, elm:ALL\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "2", NULL,
        SP_IND_ALL,
        "scope", NULL,
        0));

    printf("\n--- No-val-set prop 3, own-scope: /:scope, elm:0\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "3", NULL,
        0,
        "scope", NULL,
        0));

    printf("\n--- Set/add prop 4, own-scope: /:scope@0, elm:LAST\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "4", "VAL",
        SP_IND_LAST,
        "scope@0", NULL,
        0));

    printf("\n--- Set/add prop 4, own-scope: /:scope@$, elm:ALL\n");
    EXEC_RG(sp_set_prop(&in, &out,
        NULL,
        "4", "VAL",
        SP_IND_ALL,
        "scope@$", "",
        0));

    /* destination scope not found */
    assert(sp_set_prop(&in, &out,
        NULL,
        "4", "VAL",
        SP_IND_LAST,
        "scope@1", "",
        0)==SPEC_NOTFOUND);
    assert(sp_set_prop(&in, &out,
        NULL,
        "4", "VAL",
        SP_IND_LAST,
        "/:xxx", NULL,
        0)==SPEC_NOTFOUND);

    EXEC_RG(sp_get_scope_info(&in, NULL, NULL, "scope", 0, NULL, NULL, &sc));
    assert(sc.body_pres!=0);

    printf("\n--- Set prop 1, parsing scope: /:scope, elm:ALL\n");
    EXEC_RG(sp_set_prop(&in, &out,
        &sc.lbody,
        "1", "VAL",
        SP_IND_ALL,
        NULL, NULL,
        0));

    printf("\n--- No-val-set prop 3, parsing scope: /:scope, elm:LAST\n");
    EXEC_RG(sp_set_prop(&in, &out,
        &sc.lbody,
        "3", NULL,
        SP_IND_LAST,
        NULL, NULL,
        0));

    printf("\n--- Set/add prop 3, parsing scope: /:scope, elm:1, flags:EXTEOL\n");
    EXEC_RG(sp_set_prop(&in, &out,
        &sc.lbody,
        "3", "VAL",
        1,
        NULL, NULL,
        SP_F_EXTEOL));

    /* index conflict, adding not possible */
    assert(sp_set_prop(&in, &out,
        &sc.lbody,
        "3", NULL,
        2,
        NULL, NULL,
        0)==SPEC_NOTFOUND);

finish:
    if (ret) printf("Error: %d\n", ret);
    if (in_opn) sp_close(&in);
    return 0;
}
