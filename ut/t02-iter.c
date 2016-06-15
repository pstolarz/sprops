/*
   Copyright (c) 2015,2016 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <stdio.h>
#include <string.h>
#include "sprops/props.h"

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

/* sp_iterate() property callback */
static sp_errc_t cb_prop(
    void *arg, FILE *in, const char *name, const sp_tkn_info_t *p_tkname,
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
    void *arg, FILE *in, const char *type, const sp_tkn_info_t *p_tktype,
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
    char buf1[32], buf2[32];
    sp_errc_t ret=SPEC_SUCCESS;
    FILE *in;

    in = fopen("c01-2.conf", "rb");
#if 0
    in = fopen("c01-2_win.conf", "rb");
    in = fopen("c01-2_1line.conf", "rb");
#endif
    if (!in) goto finish;

    printf("\n--- Iterating scope /\n");
    EXEC_RG(sp_iterate(in, NULL, NULL,  NULL, cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));


    printf("\n\n--- Iterating scope /:1\n");
    EXEC_RG(sp_iterate(in, NULL, "/1", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@0\n");
    EXEC_RG(sp_iterate(in, NULL, "/1@0", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@1\n");
    EXEC_RG(sp_iterate(in, NULL, "/1@1", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@2\n");
    EXEC_RG(sp_iterate(in, NULL, "/1@2", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@$\n");
    EXEC_RG(sp_iterate(in, NULL, "/1@$", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));


    printf("\n\n--- Iterating scope /:1/:2\n");
    EXEC_RG(sp_iterate(in, NULL, "/1/2", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2@0\n");
    EXEC_RG(sp_iterate(in, NULL, "/1/2@0", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2@1\n");
    EXEC_RG(sp_iterate(in, NULL, "/1/2@1", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2@2\n");
    EXEC_RG(sp_iterate(in, NULL, "/1/2@2", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2@3\n");
    EXEC_RG(sp_iterate(in, NULL, "/1/2@3", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2@$\n");
    EXEC_RG(sp_iterate(in, NULL, "/1/2@$", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));


    printf("\n\n--- Iterating scope /:1/:2/:3\n");
    EXEC_RG(sp_iterate(in, NULL, "/1/2/3", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2/:3@0\n");
    EXEC_RG(sp_iterate(in, NULL, "/1/2/3@0", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2/:3@1\n");
    EXEC_RG(sp_iterate(in, NULL, "/1/2/3@1", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2/:3@2\n");
    EXEC_RG(sp_iterate(in, NULL, "/1/2/3@2", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2/:3@3\n");
    EXEC_RG(sp_iterate(in, NULL, "/1/2/3@3", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2/:3@4\n");
    EXEC_RG(sp_iterate(in, NULL, "/1/2/3@4", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2/:3@5\n");
    EXEC_RG(sp_iterate(in, NULL, "/1/2/3@5", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1/:2/:3@$\n");
    EXEC_RG(sp_iterate(in, NULL, "/1/2/3@$", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));


    printf("\n\n--- Iterating scope /:1@2/:2/:3\n");
    EXEC_RG(sp_iterate(in, NULL, "/1@2/2/3", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@2/:2@0/:3\n");
    EXEC_RG(sp_iterate(in, NULL, "/1@2/2@0/3", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@2/:2@1/:3\n");
    EXEC_RG(sp_iterate(in, NULL, "/1@2/2@1/3", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));


    printf("\n\n--- Iterating scope /:1@$/:2@$/:3\n");
    EXEC_RG(sp_iterate(in, NULL, "/1@$/2@$/3", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@$/:2@$/:3@0\n");
    EXEC_RG(sp_iterate(in, NULL, "/1@$/2@$/3@0", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@$/:2@$/:3@1\n");
    EXEC_RG(sp_iterate(in, NULL, "/1@$/2@$/3@1", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n--- Iterating scope /:1@$/:2@$/:3@$\n");
    EXEC_RG(sp_iterate(in, NULL, "/1@$/2@$/3@$", "", cb_prop, cb_scope, NULL,
        buf1, sizeof(buf1), buf2, sizeof(buf2)));

finish:
    if (ret) {
        if (ret==SPEC_SYNTAX) {
            int line, col;
            sp_check_syntax(in, NULL, &line, &col);
            printf("Syntax error: line:%d, col:%d\n", line, col);
        } else {
            printf("Error: %d\n", ret);
        }
    }
    if (in) fclose(in);

    return 0;
}
