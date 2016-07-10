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
#include "sprops/props.h"

#if defined(CONFIG_NO_SEMICOL_ENDS_VAL) || \
    (CONFIG_MAX_SCOPE_LEVEL_DEPTH>0 && CONFIG_MAX_SCOPE_LEVEL_DEPTH<2)
# error Bad configuration
#endif

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

int main(void)
{
    sp_errc_t ret=SPEC_SUCCESS;
    sp_scope_info_ex_t sc3;

    SP_FILE in, out;
    int in_opn=0;

    EXEC_RG(sp_fopen(&in, "c07.conf", "rb"));
    in_opn++;

    sp_fopen2(&out, stdout);

    printf("--- Move prop 1 -> PROP, own-scope: /, elm:0\n");
    EXEC_RG(sp_mv_prop(
        &in, &out,
        NULL,
        "1", "PROP",
        0,
        "/", NULL,
        0));

    printf("\n--- Move prop 1 -> PROP, own-scope: /, elm:LAST\n");
    EXEC_RG(sp_mv_prop(
        &in, &out,
        NULL,
        "1", "PROP",
        SP_IND_LAST,
        "/", NULL,
        0));

    printf("\n--- Move prop 1 -> PROP, own-scope: /scope:3, elm:ALL\n");
    EXEC_RG(sp_mv_prop(
        &in, &out,
        NULL,
        "1", "PROP",
        SP_IND_ALL,
        "/scope:3", NULL,
        0));

    printf("\n--- Move prop 1 -> PROP, own-scope: /:4, elm:ALL\n");
    EXEC_RG(sp_mv_prop(
        &in, &out,
        NULL,
        "1", "PROP",
        SP_IND_ALL,
        "/4", NULL,
        0));

    printf("\n--- Move scope /scope:3 -> /:NAME, own-scope: /, elm:0\n");
    EXEC_RG(sp_mv_scope(
        &in, &out,
        NULL,
        "scope", "3",
        NULL, "NAME",
        0,
        "/", "",
        0));

    printf("\n--- Move scope /scope:3 -> /:NAME, own-scope: /, elm:LAST\n");
    EXEC_RG(sp_mv_scope(
        &in, &out,
        NULL,
        "scope", "3",
        NULL, "NAME",
        SP_IND_LAST,
        "/", NULL,
        0));

    printf("\n--- Move scope /scope:3 -> /:NAME, own-scope: /, elm:ALL\n");
    EXEC_RG(sp_mv_scope(
        &in, &out,
        NULL,
        "scope", "3",
        "", "NAME",
        SP_IND_ALL,
        "/", "",
        0));

    printf("\n--- Move scope /scope:3 -> /TYPE:NAME, own-scope: /, elm:ALL\n");
    EXEC_RG(sp_mv_scope(
        &in, &out,
        NULL,
        "scope", "3",
        "TYPE", "NAME",
        SP_IND_ALL,
        "/", "",
        0));

    printf("\n--- Move scope /:4 -> /TYPE:NAME, own-scope: /, elm:0\n");
    EXEC_RG(sp_mv_scope(
        &in, &out,
        NULL,
        "", "4",
        "TYPE", "NAME",
        0,
        "/", NULL,
        0));

    printf("\n--- Move scope /:4 -> /:NAME, own-scope: /, elm:LAST\n");
    EXEC_RG(sp_mv_scope(
        &in, &out,
        NULL,
        "", "4",
        "", "NAME",
        SP_IND_LAST,
        "/", NULL,
        0));

    EXEC_RG(sp_get_scope_info(&in, NULL, "scope", "3", 1, NULL, NULL, &sc3));
    assert(sc3.body_pres!=0);

    printf("\n--- Move prop 1 -> PROP, parsing scope /scope:3, elm:ALL\n");
    EXEC_RG(sp_mv_prop(
        &in, &out,
        &sc3.lbody,
        "1", "PROP",
        SP_IND_ALL,
        NULL, NULL,
        0));

    printf("\n--- Move scope /scope:2 -> /:NAME, own-scope: /scope:3, elm:0\n");
    EXEC_RG(sp_mv_scope(
        &in, &out,
        &sc3.lbody,
        "scope", "2",
        NULL, "NAME",
        0,
        "/", NULL,
        0));

    printf("\n--- Move scope /:3 -> /TYPE:NAME, own-scope: /scope:3, elm:ALL\n");
    EXEC_RG(sp_mv_scope(
        &in, &out,
        &sc3.lbody,
        NULL, "3",
        "TYPE", "NAME",
        SP_IND_ALL,
        "/", NULL,
        0));

finish:
    if (ret) printf("Error: %d\n", ret);
    if (in_opn) sp_fclose(&in);
    return 0;
}
