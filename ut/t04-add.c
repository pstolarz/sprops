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
#include "../config.h"
#include "sprops/utils.h"

#if defined(CONFIG_NO_SEMICOL_ENDS_VAL) || \
    !defined(CONFIG_CUT_VAL_LEADING_SPACES) || \
    (CONFIG_MAX_SCOPE_LEVEL_DEPTH>0 && CONFIG_MAX_SCOPE_LEVEL_DEPTH<2)
# error Bad configuration
#endif

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

/* indentation */
static unsigned long indf = SP_F_SPIND(4);

int main(void)
{
    sp_errc_t ret=SPEC_SUCCESS;
    sp_scope_info_ex_t sc2;

    FILE *in = fopen("c04.conf", "rb");
    if (!in) goto finish;

    printf("--- Prop added to: /, elm:0, flags:EXTEOL\n");
    EXEC_RG(sp_add_prop(
        in, stdout,
        NULL,
        "PROP", "VAL",
        0,
        "/", NULL,
        indf|SP_F_EXTEOL));

    printf("\n--- Prop w/o value added to: /, elm:1\n");
    EXEC_RG(sp_add_prop(
        in, stdout,
        NULL,
        "PROP", NULL,
        1,
        "/", NULL,
        indf));

    printf("\n--- Scope added to: /scope:2, elm:0\n");
    EXEC_RG(sp_add_scope(
        in, stdout,
        NULL,
        "TYPE", "SCOPE",
        0,
        "/2", "scope",
        indf));

    printf("\n--- Scope added to: /scope:2@0, elm:LAST, flags:EMPCPT|EXTEOL\n");
    EXEC_RG(sp_add_scope(
        in, stdout,
        NULL,
        "TYPE", "SCOPE",
        SP_ELM_LAST,
        "/2@0", "scope",
        indf|SP_F_EMPCPT|SP_F_EXTEOL));

    printf("\n--- Scope w/o type added to: /scope:2@1, elm:0, flags:EMPCPT\n");
    EXEC_RG(sp_add_scope(
        in, stdout,
        NULL,
        NULL, "SCOPE",
        0,
        "/2@1", "scope",
        indf|SP_F_EMPCPT));

    printf("\n--- Scope w/o type added to: "
        "/scope:2/scope:2, elm:LAST, flags:EMPCPT|SPLBRA|EXTEOL\n");
    EXEC_RG(sp_add_scope(
        in, stdout,
        NULL,
        "", "SCOPE",
        SP_ELM_LAST,
        "/2/2", "scope",
        indf|SP_F_EMPCPT|SP_F_SPLBRA|SP_F_EXTEOL));

    printf("\n--- Scope added to: /scope:2, elm:LAST, flags:SPLBRA\n");
    EXEC_RG(sp_add_scope(
        in, stdout,
        NULL,
        "TYPE", "SCOPE",
        SP_ELM_LAST,
        "/2", "scope",
        indf|SP_F_SPLBRA));

    printf("\n--- Prop added to: /scope:2@$, elm:0, flags:EXTEOL|NVSRSP\n");
    EXEC_RG(sp_add_prop(
        in, stdout,
        NULL,
        "PROP", "VAL",
        0,
        "/2@$", "scope",
        indf|SP_F_EXTEOL|SP_F_NVSRSP));

    printf("\n--- Prop w/o value added to: /, elm:LAST, flags:EXTEOL\n");
    EXEC_RG(sp_add_prop(
        in, stdout,
        NULL,
        "PROP", "",
        SP_ELM_LAST,
        "/", NULL,
        indf|SP_F_EXTEOL));

    EXEC_RG(sp_get_scope_info(in, NULL, "scope", "2", 1, NULL, NULL, &sc2));
    assert(sc2.body_pres!=0);

    printf("\n--- Scope added, parsing scope /scope:2, elm:0, flags:EXTEOL\n");
    EXEC_RG(sp_add_scope(
        in, stdout,
        &sc2.lbody,
        "TYPE", "SCOPE",
        0,
        "/", NULL,
        indf|SP_F_EXTEOL));

    printf("\n--- Scope added, parsing scope /scope:2, elm:1, flags:EMPCPT\n");
    EXEC_RG(sp_add_scope(
        in, stdout,
        &sc2.lbody,
        "TYPE", "SCOPE",
        1,
        "/", NULL,
        indf|SP_F_EMPCPT));

    printf("\n--- Scope added, "
        "parsing scope /scope:2, elm:LAST, flags:SPLBRA|EXTEOL\n");
    EXEC_RG(sp_add_scope(
        in, stdout,
        &sc2.lbody,
        "TYPE", "SCOPE",
        SP_ELM_LAST,
        "/", NULL,
        indf|SP_F_SPLBRA|SP_F_EXTEOL));

    /* not existing elements */

    assert(sp_add_prop(
        in, stdout,
        NULL,
        "PROP", NULL,
        5,
        "/", NULL,
        indf)==SPEC_NOTFOUND);

    assert(sp_add_prop(
        in, stdout,
        NULL,
        "PROP", NULL,
        3,
        "/2", "scope",
        indf)==SPEC_NOTFOUND);

    assert(sp_add_prop(
        in, stdout,
        NULL,
        "PROP", NULL,
        1,
        "/2/2", "scope",
        indf)==SPEC_NOTFOUND);

finish:
    if (ret) printf("Error: %d\n", ret);
    if (in) fclose(in);
    return 0;
}
