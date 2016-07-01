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
#include "sprops/props.h"

#if defined(CONFIG_NO_SEMICOL_ENDS_VAL) || \
    !defined(CONFIG_CUT_VAL_LEADING_SPACES) || \
    (CONFIG_MAX_SCOPE_LEVEL_DEPTH>0 && CONFIG_MAX_SCOPE_LEVEL_DEPTH<2)
# error Bad configuration
#endif

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

/* used indentation */
static unsigned long indf = SP_F_SPIND(4);

typedef struct _argcb_t
{
    long in_off;
    int elem_n;
    unsigned long flags;
} argcb_t;

/* sp_iterate() scope callback */
static sp_errc_t cb_scope(
    void *arg, FILE *in, const char *type, const sp_tkn_info_t *p_tktype,
    const char *name, const sp_tkn_info_t *p_tkname, const sp_loc_t *p_lbody,
    const sp_loc_t *p_lbdyenc, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    argcb_t *p_argcb = (argcb_t*)arg;

    if (p_lbody)
    {
        /* copy up to body start */
        EXEC_RG(sp_cpy_to_out(in, stdout, p_argcb->in_off, p_lbody->beg, NULL));

        EXEC_RG(sp_add_scope(
            in, stdout,
            p_lbody,
            "TYPE", "SCOPE",
            p_argcb->elem_n,
            "/", NULL,
            indf|p_argcb->flags));

        /* set copy offset behind already modified body */
        p_argcb->in_off = p_lbody->end+1;
    }

finish:
    return ret;
}

int main(void)
{
    sp_errc_t ret=SPEC_SUCCESS;
    char buf1[32], buf2[32];
    argcb_t argcb;

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

#define __ITER_ADD(e, f) \
    argcb.in_off = 0; \
    argcb.elem_n = (e); \
    argcb.flags = (f); \
    EXEC_RG(sp_iterate(in, NULL, "/", NULL, NULL, cb_scope, &argcb, \
        buf1, sizeof(buf1), buf2, sizeof(buf2))); \
    EXEC_RG(sp_cpy_to_out(in, stdout, argcb.in_off, EOF, NULL));

    printf("\n--- Adding scope to /scope:2 during / scope iteration, "
        "elm:0, flags:EXTEOL\n");
    __ITER_ADD(0, SP_F_EXTEOL);

    printf("\n--- Adding scope to /scope:2 during / scope iteration, "
        "elm:1, flags:EMPCPT\n");
    __ITER_ADD(1, SP_F_EMPCPT);

    printf("\n--- Adding scope to /scope:2 during / scope iteration, "
        "elm:LAST, flags:SPLBRA|EXTEOL\n");
    __ITER_ADD(SP_ELM_LAST, SP_F_SPLBRA|SP_F_EXTEOL);

#undef __ITER_ADD

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
