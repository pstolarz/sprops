/*
   Copyright (c) 2015,2016 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <string.h>
#include "../config.h"
#include "sprops/props.h"

#if defined(CONFIG_NO_SEMICOL_ENDS_VAL) || \
    !defined(CONFIG_CUT_VAL_LEADING_SPACES) || \
    !defined(CONFIG_TRIM_VAL_TRAILING_SPACES) || \
    (CONFIG_MAX_SCOPE_LEVEL_DEPTH>0 && CONFIG_MAX_SCOPE_LEVEL_DEPTH<4)
# error Bad configuration
#endif

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

/* sp_iterate() property callback */
static sp_errc_t cb_prop(
    void *arg, SP_FILE *in, const char *name, const sp_tkn_info_t *p_tkname,
    const char *val, const sp_tkn_info_t *p_tkval, const sp_loc_t *p_ldef)
{
    if (!strcmp(name, "FINISH")) {
        printf("Iteration aborted on FINISH!\n");
        return SPEC_CB_FINISH;
    }

    printf("PROP %s, val-str \"%s\": "
        "NAME len:%ld loc:%d.%d|%d.%d [0x%02lx|0x%02lx], ",
        name, val,
        p_tkname->len,
        p_tkname->loc.first_line,
        p_tkname->loc.first_column,
        p_tkname->loc.last_line,
        p_tkname->loc.last_column,
        p_tkname->loc.beg,
        p_tkname->loc.end);

    if (p_tkval) {
        printf("VAL len:%ld loc:%d.%d|%d.%d [0x%02lx|0x%02lx], ",
            p_tkval->len,
            p_tkval->loc.first_line,
            p_tkval->loc.first_column,
            p_tkval->loc.last_line,
            p_tkval->loc.last_column,
            p_tkval->loc.beg,
            p_tkval->loc.end);
    } else
        printf("VAL not present, ");

    printf("DEF loc:%d.%d|%d.%d [0x%02lx|0x%02lx]\n",
        p_ldef->first_line,
        p_ldef->first_column,
        p_ldef->last_line,
        p_ldef->last_column,
        p_ldef->beg,
        p_ldef->end);

    return SPEC_SUCCESS;
}

/* sp_iterate() scope callback */
static sp_errc_t cb_scope(
    void *arg, SP_FILE *in, const char *type, const sp_tkn_info_t *p_tktype,
    const char *name, const sp_tkn_info_t *p_tkname, const sp_loc_t *p_lbody,
    const sp_loc_t *p_lbdyenc, const sp_loc_t *p_ldef)
{
    printf("SCOPE %s, type \"%s\": "
        "NAME len:%ld loc:%d.%d|%d.%d [0x%02lx|0x%02lx], ",
        name, type,
        p_tkname->len,
        p_tkname->loc.first_line,
        p_tkname->loc.first_column,
        p_tkname->loc.last_line,
        p_tkname->loc.last_column,
        p_tkname->loc.beg,
        p_tkname->loc.end);

    if (p_tktype) {
        printf("TYPE len:%ld loc:%d.%d|%d.%d [0x%02lx|0x%02lx], ",
            p_tktype->len,
            p_tktype->loc.first_line,
            p_tktype->loc.first_column,
            p_tktype->loc.last_line,
            p_tktype->loc.last_column,
            p_tktype->loc.beg,
            p_tktype->loc.end);
    } else
        printf("TYPE not present, ");

    if (p_lbody) {
        printf("BODY loc %d.%d|%d.%d [0x%02lx|0x%02lx], ",
            p_lbody->first_line,
            p_lbody->first_column,
            p_lbody->last_line,
            p_lbody->last_column,
            p_lbody->beg,
            p_lbody->end);
    } else
        printf("BODY empty, ");

    printf("ENC-BODY loc %d.%d|%d.%d [0x%02lx|0x%02lx], ",
        p_lbdyenc->first_line,
        p_lbdyenc->first_column,
        p_lbdyenc->last_line,
        p_lbdyenc->last_column,
        p_lbdyenc->beg,
        p_lbdyenc->end);

    printf("DEF loc %d.%d|%d.%d [0x%02lx|0x%02lx]\n",
        p_ldef->first_line,
        p_ldef->first_column,
        p_ldef->last_line,
        p_ldef->last_column,
        p_ldef->beg,
        p_ldef->end);

    return SPEC_SUCCESS;
}

int main(void)
{
    sp_errc_t ret=SPEC_SUCCESS;
    char buf1[32], buf2[32];

    SP_FILE in;
    int in_opn=0;

    EXEC_RG(sp_fopen(&in, "t01-2.conf", SP_MODE_READ));
#if 0
    EXEC_RG(sp_fopen(&in, "t01-2_win.conf", SP_MODE_READ));
    EXEC_RG(sp_fopen(&in, "t01-2_1line.conf", SP_MODE_READ));
#endif
    in_opn++;

    printf("\n--- Iterating scope /\n");
    EXEC_RG(sp_iterate(&in, NULL, NULL,  NULL, cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));


    printf("\n\n--- Iterating scope /:1\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1", NULL, cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@0\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1@0", NULL, cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@1\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1@1", NULL, cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@2\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1@2", NULL, cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@$\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1@$", NULL, cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));


    printf("\n\n--- Iterating scope /:1/:2\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1/2", NULL, cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2@0\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1/2@0", NULL, cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2@1\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1/2@1", NULL, cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2@2\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1/2@2", NULL, cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2@3\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1/2@3", NULL, cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2@$\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1/2@$", NULL, cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));


    printf("\n\n--- Iterating scope /:1/:2/:3\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1/2/3", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2/:3@0\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1/2/3@0", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2/:3@1\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1/2/3@1", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2/:3@2\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1/2/3@2", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2/:3@3\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1/2/3@3", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2/:3@4\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1/2/3@4", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2/:3@5\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1/2/3@5", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2/:3@$\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1/2/3@$", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));


    printf("\n\n--- Iterating scope /:1@2/:2/:3\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1@2/2/3", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@2/:2@0/:3\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1@2/2@0/3", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@2/:2@1/:3\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1@2/2@1/3", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));


    printf("\n\n--- Iterating scope /:1@$/:2@$/:3\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1@$/2@$/3", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@$/:2@$/:3@0\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1@$/2@$/3@0", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@$/:2@$/:3@1\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1@$/2@$/3@1", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@$/:2@$/:3@$\n");
    EXEC_RG(sp_iterate(&in, NULL, "/1@$/2@$/3@$", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

finish:
    if (ret) {
        if (ret==SPEC_SYNTAX) {
            sp_synerr_t synerr;
            sp_check_syntax(&in, NULL, &synerr);
            printf("Syntax error: line:%d, col:%d, code:%d\n",
                synerr.loc.line, synerr.loc.col, synerr.code);
        } else {
            printf("Error: %d\n", ret);
        }
    }
    if (in_opn) sp_fclose(&in);

    return 0;
}
