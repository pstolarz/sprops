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
#include <stdio.h>
#include "sprops/props.h"

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;


int main(void)
{
    sp_errc_t ret=SPEC_SUCCESS;

    FILE *in = fopen("c06.conf", "rb");
    if (!in) goto finish;

    printf("--- Set prop 1, own-scope: /, elm:0\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "1", "VAL",
        0,
        "/", NULL,
        0));

    printf("\n--- Set prop 1, own-scope: /, elm:1\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "1", "VAL",
        1,
        "/", NULL,
        0));

    printf("\n--- No-val-set prop 1, own-scope: /, elm:ALL\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "1", NULL,
        SP_IND_ALL,
        "/", NULL,
        0));

    printf("\n--- No-val-set/add prop 1, own-scope: /, elm:2\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "1", NULL,
        2,
        "/", NULL,
        0));

    /* same as above but with SP_F_NOADD flag */
    assert(sp_set_prop(in, stdout,
        NULL,
        "1", NULL,
        2,
        "/", NULL,
        SP_F_NOADD)==SPEC_NOTFOUND);

    /* index conflict, adding not possible */
    assert(sp_set_prop(in, stdout,
        NULL,
        "1", "VAL",
        3,
        "/", NULL,
        0)==SPEC_NOTFOUND);

    printf("\n--- Set prop 2, own-scope: /, elm:LAST\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "2", "VAL",
        SP_IND_LAST,
        "/", NULL,
        0));

    printf("\n--- Set/add prop 2, own-scope: /, elm:1\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "2", "VAL",
        1,
        "/", NULL,
        0));

    /* same as above but with SP_F_NOADD flag */
    assert(sp_set_prop(in, stdout,
        NULL,
        "2", "VAL",
        1,
        "/", NULL,
        SP_F_NOADD)==SPEC_NOTFOUND);

    /* index conflict, adding not possible */
    assert(sp_set_prop(in, stdout,
        NULL,
        "2", "VAL",
        2,
        "/", NULL,
        0)==SPEC_NOTFOUND);

    printf("\n--- Set/add prop 3, own-scope: /, elm:LAST\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "3", "VAL",
        SP_IND_LAST,
        "/", NULL,
        0));

    /* index conflict, adding not possible */
    assert(sp_set_prop(in, stdout,
        NULL,
        "3", "VAL",
        1,
        "/", NULL,
        0)==SPEC_NOTFOUND);

    printf("\n--- Set prop 1, own-scope: /:scope, elm:0\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "1", "VAL",
        0,
        "scope", "",
        0));

    printf("\n--- Set prop 1, own-scope: /:scope, elm:LAST\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "1", "VAL",
        SP_IND_LAST,
        "scope", "",
        0));

    printf("\n--- Set prop 1, own-scope: /:scope, elm:ALL\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "1", "VAL",
        SP_IND_ALL,
        "scope", "",
        0));

    printf("\n--- No-val-set prop 1, own-scope: /:scope, elm:0\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "1", NULL,
        0,
        "scope", "",
        0));

    printf("\n--- No-val-set prop 1, own-scope: /:scope, elm:ALL\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "1", NULL,
        SP_IND_ALL,
        "scope", "",
        0));

    printf("\n--- Set prop 2, own-scope: /:scope, elm:0\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "2", "VAL",
        0,
        "scope", "",
        0));

    printf("\n--- No-val-set prop 2, own-scope: /:scope, elm:ALL\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "2", NULL,
        SP_IND_ALL,
        "scope", "",
        0));

    printf("\n--- No-val-set prop 3, own-scope: /:scope, elm:0\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "3", NULL,
        0,
        "scope", "",
        0));

    printf("\n--- Set/add prop 4, own-scope: /:scope, elm:LAST\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "4", "VAL",
        SP_IND_LAST,
        "scope", "",
        0));

    printf("\n--- Set/add prop 4, own-scope: /:scope, elm:ALL\n");
    EXEC_RG(sp_set_prop(in, stdout,
        NULL,
        "4", "VAL",
        SP_IND_ALL,
        "scope", "",
        0));

finish:
    if (ret) printf("Error: %d\n", ret);
    if (in) fclose(in);
    return 0;
}
