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

    FILE *in = fopen("c05.conf", "rb");
    if (!in) goto finish;

    printf("--- Del prop 1, scope: /, elm:0, flags:EXTEOL\n");
    EXEC_RG(sp_rm_prop(
        in, stdout,
        NULL,
        "1",
        0,
        "/", NULL,
        SP_F_EXTEOL));

    printf("\n--- Del prop 2, scope /, elm:ALL\n");
    EXEC_RG(sp_rm_prop(
        in, stdout,
        NULL,
        "2",
        SP_IND_ALL,
        "/", NULL,
        0));

    printf("\n--- Del scope 3, scope /, elm:LAST\n");
    EXEC_RG(sp_rm_scope(
        in, stdout,
        NULL,
        "scope", "3",
        SP_IND_LAST,
        "/", NULL,
        0));

    printf("\n--- Del prop 4, scope /, elm:0, flags:EXTEOL\n");
    EXEC_RG(sp_rm_prop(
        in, stdout,
        NULL,
        "4",
        0,
        "/", NULL,
        SP_F_EXTEOL));

    printf("\n--- Del prop 4, scope /, elm:LAST\n");
    EXEC_RG(sp_rm_prop(
        in, stdout,
        NULL,
        "4",
        SP_IND_LAST,
        "/", NULL,
        0));

    printf("\n--- Del scope 5, scope /, elm:LAST, flags:EXTEOL\n");
    EXEC_RG(sp_rm_scope(
        in, stdout,
        NULL,
        "scope", "5",
        SP_IND_LAST,
        "/", NULL,
        SP_F_EXTEOL));

    printf("\n--- Del scope 5, scope /, elm:ALL\n");
    EXEC_RG(sp_rm_scope(
        in, stdout,
        NULL,
        "scope", "5",
        SP_IND_ALL,
        "/", NULL,
        0));

    printf("\n--- Del prop 1, scope /scope:5, elm:0\n");
    EXEC_RG(sp_rm_prop(
        in, stdout,
        NULL,
        "1",
        0,
        "5", "scope",
        0));

    printf("\n--- Del prop 3, scope /scope:5, elm:0, flags:EXTEOL\n");
    EXEC_RG(sp_rm_prop(
        in, stdout,
        NULL,
        "3",
        0,
        "5", "scope",
        SP_F_EXTEOL));

finish:
    if (ret) printf("Error: %d\n", ret);
    if (in) fclose(in);
    return 0;
}
