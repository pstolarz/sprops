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

#if (CONFIG_MAX_SCOPE_LEVEL_DEPTH>0 && CONFIG_MAX_SCOPE_LEVEL_DEPTH<2)
# error Bad configuration
#endif

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

int main(void)
{
    sp_errc_t ret=SPEC_SUCCESS;
    sp_scope_info_ex_t sc5;

    SP_FILE in, out;
    int in_opn=0;

    EXEC_RG(sp_fopen(&in, "c05.conf", "rb"));
    in_opn++;

    sp_fopen2(&out, stdout);

    printf("--- Del prop 1, own-scope: /, elm:0, flags:EXTEOL\n");
    EXEC_RG(sp_rm_prop(
        &in, &out,
        NULL,
        "1",
        0,
        "/", NULL,
        SP_F_EXTEOL));

    printf("\n--- Del prop 2, own-scope /, elm:ALL\n");
    EXEC_RG(sp_rm_prop(
        &in, &out,
        NULL,
        "2",
        SP_IND_ALL,
        "/", NULL,
        0));

    printf("\n--- Del scope:3, own-scope /, elm:LAST\n");
    EXEC_RG(sp_rm_scope(
        &in, &out,
        NULL,
        "scope", "3",
        SP_IND_LAST,
        "/", NULL,
        0));

    printf("\n--- Del prop 4, own-scope /, elm:0, flags:EXTEOL\n");
    EXEC_RG(sp_rm_prop(
        &in, &out,
        NULL,
        "4",
        0,
        "/", NULL,
        SP_F_EXTEOL));

    printf("\n--- Del prop 4, own-scope /, elm:LAST\n");
    EXEC_RG(sp_rm_prop(
        &in, &out,
        NULL,
        "4",
        SP_IND_LAST,
        "/", NULL,
        0));

    printf("\n--- Del scope :5, own-scope /, elm:LAST, flags:EXTEOL\n");
    EXEC_RG(sp_rm_scope(
        &in, &out,
        NULL,
        NULL, "5",
        SP_IND_LAST,
        "/", NULL,
        SP_F_EXTEOL));

    printf("\n--- Del scope :5, own-scope /, elm:ALL\n");
    EXEC_RG(sp_rm_scope(
        &in, &out,
        NULL,
        NULL, "5",
        SP_IND_ALL,
        "/", NULL,
        0));

    printf("\n--- Del prop 1, own-scope /:5, elm:0\n");
    EXEC_RG(sp_rm_prop(
        &in, &out,
        NULL,
        "1",
        0,
        "5", NULL,
        0));

    printf("\n--- Del prop 3, own-scope /:5, elm:0, flags:EXTEOL\n");
    EXEC_RG(sp_rm_prop(
        &in, &out,
        NULL,
        "3",
        0,
        "/:5", NULL,
        SP_F_EXTEOL));

    EXEC_RG(sp_get_scope_info(&in, NULL, NULL, "5", 0, NULL, NULL, &sc5));
    assert(sc5.body_pres!=0);

    printf("\n--- Del prop 1, parsing scope /:5, elm:0\n");
    EXEC_RG(sp_rm_prop(
        &in, &out,
        &sc5.lbody,
        "1",
        0,
        NULL, NULL,
        0));

    printf("\n--- Del prop 3, parsing scope /:5, elm:0, flags:EXTEOL\n");
    EXEC_RG(sp_rm_prop(
        &in, &out,
        &sc5.lbody,
        "3",
        0,
        NULL, NULL,
        SP_F_EXTEOL));

    /* not existing elements */

    printf("\n--- Del absent prop 3, own-scope /\n");
    assert(sp_rm_prop(
        &in, &out,
        NULL,
        "3",
        0,
        "/", NULL,
        0)==SPEC_SUCCESS);

    printf("\n--- Del prop 1 in absent own-scope /:1\n");
    assert(sp_rm_prop(
        &in, &out,
        NULL,
        "1",
        0,
        "/:1", NULL,
        0)==SPEC_NOTFOUND);

finish:
    if (ret) printf("Error: %d\n", ret);
    if (in_opn) sp_fclose(&in);
    return 0;
}
