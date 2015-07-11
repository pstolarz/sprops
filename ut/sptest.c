/*
   Copyright (c) 2015 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <assert.h>
#include <stdio.h>
#include "sp_props/props.h"

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

static int cb_prop(void *arg, FILE *in, const char *name, const sp_tkn_info_t *p_tkname,
    const char *val, const sp_tkn_info_t *p_tkval, const sp_loc_t *p_ldef)
{
    printf("Property: \"%s\", value: \"%s\"\n", name, val);
    return 0;
}

static int cb_scope(void *arg, FILE *in, const char *type, const sp_tkn_info_t *p_tktype,
    const char *name, const sp_tkn_info_t *p_tkname, const sp_loc_t *p_lbody,
    const sp_loc_t *p_ldef)
{
    printf("scope type: \"%s\", name: \"%s\"; ", type, name);
    if (p_lbody) {
        printf("location start: ln:%d|col:%d, stop: ln:%d|col:%d\n",
            p_lbody->first_line, p_lbody->first_column, p_lbody->last_line, p_lbody->last_column);
    } else {
        printf("empty\n");
    }
    return 0;
}

int main(void)
{
    FILE *in = fopen("sptest.conf", "rb");
    /* FILE *in = fopen("sptest-1line.conf", "rb"); */

    if (in)
    {
        int ival;
        long lval;
        char buf[8], buf2[32];
        sp_errc_t ret;
        sp_prop_info_ex_t info;

        sp_enumval_t evals[] =
            {{"false", 0}, {"0", 0}, {"true", 1}, {"1", 1}, {NULL, 0}};

        printf("--- Properties read\n");

        EXEC_RG(sp_get_prop(in, NULL, "a", NULL, NULL, buf, sizeof(buf), &info));
        printf("string: \"%s\", lengths: %ld|%ld\n", buf, info.tkval.len, sp_loc_len(&info.tkval.loc));

        /* param name provided as part of path */
        EXEC_RG(sp_get_prop(in, NULL, NULL, "/b", NULL, buf, sizeof(buf), &info));
        printf("string: \"%s\", lengths: %ld|%ld\n", buf, info.tkval.len, sp_loc_len(&info.tkval.loc));

        EXEC_RG(sp_get_prop(in, NULL, "}'\\\"{", NULL, NULL, buf, sizeof(buf), &info));
        printf("string: \"%s\", lengths: %ld|%ld\n", buf, info.tkval.len, sp_loc_len(&info.tkval.loc));

        EXEC_RG(sp_get_prop(in, NULL, ";\"\\'#", NULL, NULL, buf, sizeof(buf), &info));
        printf("string: \"%s\", lengths: %ld|%ld\n", buf, info.tkval.len, sp_loc_len(&info.tkval.loc));

        EXEC_RG(sp_get_prop(in, NULL, "a", "/:'\\x3a \\x2f", NULL, buf, sizeof(buf), &info));
        printf("string: \"%s\", lengths: %ld|%ld\n", buf, info.tkval.len, sp_loc_len(&info.tkval.loc));

        /* param name provided as part of path */
        EXEC_RG(sp_get_prop(in, NULL, NULL, "/1/a", "scope", buf, sizeof(buf), &info));
        printf("string: \"%s\", lengths: %ld|%ld\n", buf, info.tkval.len, sp_loc_len(&info.tkval.loc));

        /* truncated to the buf size */
        EXEC_RG(sp_get_prop(in, NULL, "a", "1/scope:2", "scope", buf, sizeof(buf), &info));
        printf("string: \"%s\", lengths: %ld|%ld\n", buf, info.tkval.len, sp_loc_len(&info.tkval.loc));

        EXEC_RG(sp_get_prop(in, NULL, "b", "/1/2", "scope", buf, sizeof(buf), &info));
        printf("string: \"%s\", lengths: %ld|%ld\n", buf, info.tkval.len, sp_loc_len(&info.tkval.loc));

        EXEC_RG(sp_get_prop_int(in, NULL, "a", "1/2/:xxx", "scope", &lval, &info));
        printf("long: %ld, lengths: %ld|%ld\n", lval, info.tkval.len, sp_loc_len(&info.tkval.loc));

        EXEC_RG(sp_get_prop(in, NULL, "a", "1/2/:xxx/d:d", "scope", buf, sizeof(buf), &info));
        printf("string: \"%s\", lengths: %ld|%ld\n", buf, info.tkval.len, sp_loc_len(&info.tkval.loc));

        /* truncated to the buf size */
        EXEC_RG(sp_get_prop(in, NULL, "a", "/:scope", NULL, buf, sizeof(buf), &info));
        printf("string: \"%s\", lengths: %ld|%ld\n", buf, info.tkval.len, sp_loc_len(&info.tkval.loc));

        EXEC_RG(sp_get_prop(in, NULL, "b", "/2", "scope", buf, sizeof(buf), &info));
        printf("string: \"%s\", lengths: %ld|%ld\n", buf, info.tkval.len, sp_loc_len(&info.tkval.loc));

        EXEC_RG(sp_get_prop(in, NULL, "b", "/2", "scope", buf, sizeof(buf), &info));
        printf("string: \"%s\", lengths: %ld|%ld\n", buf, info.tkval.len, sp_loc_len(&info.tkval.loc));

        EXEC_RG(sp_get_prop(in, NULL, "a", "1/2/3", "", buf, sizeof(buf), &info));
        printf("string: \"%s\", lengths: %ld|%ld\n", buf, info.tkval.len, sp_loc_len(&info.tkval.loc));

        EXEC_RG(sp_get_prop(in, NULL, "b", "/1/:2/3", "", buf, sizeof(buf), &info));
        printf("string: \"%s\", lengths: %ld|%ld\n", buf, info.tkval.len, sp_loc_len(&info.tkval.loc));

        EXEC_RG(sp_get_prop_enum(
            in, NULL, "c", "1/2/3", "", evals, 1, buf, sizeof(buf), &ival, &info));
        printf("enum: %d, lengths: %ld|%ld\n", ival, info.tkval.len, sp_loc_len(&info.tkval.loc));

        EXEC_RG(sp_get_prop(in, NULL, "d", ":1/:2/:3", NULL, buf, sizeof(buf), &info));
        printf("string: \"%s\", lengths: %ld|%ld\n", buf, info.tkval.len, sp_loc_len(&info.tkval.loc));

        ret = sp_get_prop(in, NULL, "a", "1/2/3/scope:xyz", "", buf, sizeof(buf), &info);
        assert(ret==SPEC_NOTFOUND);

        printf("\n--- Iterating scope \"/\"\n");
        EXEC_RG(sp_iterate(
            in, NULL, NULL, NULL, cb_prop, cb_scope, NULL, buf, sizeof(buf), buf2, sizeof(buf2)));

        printf("\n--- Iterating splitted scope \"/:1/:2/:3\"\n");
        EXEC_RG(sp_iterate(
            in, NULL, "/1/2/3", "", cb_prop, cb_scope, NULL, buf, sizeof(buf), buf2, sizeof(buf2)));
finish:
        if (ret) printf("Error: %d\n", ret);
        fclose(in);
    }
    return 0;
}
