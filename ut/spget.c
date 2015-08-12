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

/* sp_iterate() property callback */
static sp_errc_t cb_prop(
    void *arg, FILE *f, char *name, const sp_tkn_info_t *p_tkname,
    char *val, const sp_tkn_info_t *p_tkval, const sp_loc_t *p_ldef)
{
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
static sp_errc_t cb_scope(void *arg, FILE *f, char *type,
    const sp_tkn_info_t *p_tktype, char *name, const sp_tkn_info_t *p_tkname,
    const sp_loc_t *p_lbody, const sp_loc_t *p_ldef)
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

    printf("DEF loc %d.%d|%d.%d [0x%02lx|0x%02lx]\n",
        p_ldef->first_line,
        p_ldef->first_column,
        p_ldef->last_line,
        p_ldef->last_column,
        p_ldef->beg,
        p_ldef->end);

    return SPEC_SUCCESS;
}

static void print_info_ex(const sp_prop_info_ex_t *p_info)
{
    printf("NAME len:%ld loc:%d.%d|%d.%d [0x%02lx|0x%02lx], ",
        p_info->tkname.len,
        p_info->tkname.loc.first_line,
        p_info->tkname.loc.first_column,
        p_info->tkname.loc.last_line,
        p_info->tkname.loc.last_column,
        p_info->tkname.loc.beg,
        p_info->tkname.loc.end);

    if (p_info->val_pres)
    {
        printf("VAL len %ld, loc %d.%d|%d.%d [0x%02lx|0x%02lx]\n",
            p_info->tkval.len,
            p_info->tkval.loc.first_line,
            p_info->tkval.loc.first_column,
            p_info->tkval.loc.last_line,
            p_info->tkval.loc.last_column,
            p_info->tkval.loc.beg,
            p_info->tkval.loc.end);
    } else
        printf("VAL not present\n");
}

static void print_str_prm(
    const char *prm, const char *val, const sp_prop_info_ex_t *p_info)
{
    printf("%s, val-str \"%s\": ", prm, val);
    print_info_ex(p_info);
}

static void print_long_prm(
    const char *prm, long val, const sp_prop_info_ex_t *p_info)
{
    printf("%s, val-long %ld: ", prm, val);
    print_info_ex(p_info);
}

static void print_enum_prm(
    const char *prm, int val, const sp_prop_info_ex_t *p_info)
{
    printf("%s, enum-long %d: ", prm, val);
    print_info_ex(p_info);
}

int main(void)
{
    FILE *f = fopen("spget.conf", "rb");
    /* FILE *f = fopen("spget-1line.conf", "rb"); */

    if (f)
    {
        int ival;
        long lval;
        char buf[8], buf2[32];
        sp_errc_t ret;
        sp_prop_info_ex_t info;

        sp_enumval_t evals[] =
            {{"false", 0}, {"0", 0}, {"true", 1}, {"1", 1}, {NULL, 0}};

        printf("--- Properties read\n");

        EXEC_RG(
            sp_get_prop(f, NULL, "a", NULL, NULL, buf, sizeof(buf), &info));
        print_str_prm("a", buf, &info);

        /* property name provided as part of path */
        EXEC_RG(
            sp_get_prop(f, NULL, NULL, "/b", NULL, buf, sizeof(buf), &info));
        print_str_prm("b", buf, &info);

        EXEC_RG(sp_get_prop(
            f, NULL, "}'\\\"{", NULL, NULL, buf, sizeof(buf), &info));
        print_str_prm("}'\\\"{", buf, &info);

        EXEC_RG(sp_get_prop(
            f, NULL, ";\"\\'#", NULL, NULL, buf, sizeof(buf), &info));
        print_str_prm(";\"\\'#", buf, &info);

        EXEC_RG(sp_get_prop(
            f, NULL, "a", "/:'\\x3a \\x2f", NULL, buf, sizeof(buf), &info));
        print_str_prm("a in untyped scope \"': /\"", buf, &info);

        /* property name provided as part of path */
        EXEC_RG(sp_get_prop(
            f, NULL, NULL, "/1/a", "scope", buf, sizeof(buf), &info));
        print_str_prm("/scope:1/a", buf, &info);

        /* truncated to the buf size */
        EXEC_RG(sp_get_prop(
            f, NULL, "a", "1/scope:2", "scope", buf, sizeof(buf), &info));
        print_str_prm("/scope:1/scope:2/a", buf, &info);

        EXEC_RG(sp_get_prop(
            f, NULL, "b", "/1/2", "scope", buf, sizeof(buf), &info));
        print_str_prm("/scope:1/scope:2/b", buf, &info);

        EXEC_RG(sp_get_prop_int(
            f, NULL, "a", "1/2/:xxx", "scope", &lval, &info));
        print_long_prm("/scope:1/scope:2/:xxx/a", lval, &info);

        EXEC_RG(sp_get_prop(
            f, NULL, "a", "1/2/:xxx/d:d", "scope", buf, sizeof(buf), &info));
        print_str_prm("/scope:1/scope:2/:xxx/d:d/a", buf, &info);

        EXEC_RG(sp_get_prop(
            f, NULL, "a", "/2", "scope", buf, sizeof(buf), &info));
        print_str_prm("/scope:2/a", buf, &info);

        EXEC_RG(sp_get_prop(
            f, NULL, "b", "/2", "scope", buf, sizeof(buf), &info));
        print_str_prm("/scope:2/b", buf, &info);

        /* truncated to the buf size */
        EXEC_RG(sp_get_prop(
            f, NULL, "a", "/:scope", NULL, buf, sizeof(buf), &info));
        print_str_prm("/:scope/a", buf, &info);

        EXEC_RG(sp_get_prop(
            f, NULL, "a", "1/2/3", "", buf, sizeof(buf), &info));
        print_str_prm(":1/:2/:3/a", buf, &info);

        EXEC_RG(sp_get_prop(
            f, NULL, "b", "/1/:2/3", "", buf, sizeof(buf), &info));
        print_str_prm("/:1/:2/:3/b", buf, &info);

        EXEC_RG(sp_get_prop_enum(f,
            NULL, "c", "/1/2/3", "", evals, 1, buf, sizeof(buf), &ival, &info));
        print_enum_prm("/:1/:2/:3/c", ival, &info);

        EXEC_RG(sp_get_prop(
            f, NULL, "d", ":1/:2/:3", NULL, buf, sizeof(buf), &info));
        print_str_prm("/:1/:2/:3/d", buf, &info);

        ret = sp_get_prop(
            f, NULL, "a", "1/2/3/scope:xyz", "", buf, sizeof(buf), &info);
        assert(ret==SPEC_NOTFOUND);

        EXEC_RG(
            sp_get_prop(f, NULL, "c", NULL, NULL, buf, sizeof(buf), &info));
        print_str_prm("c", buf, &info);

        printf("\n--- Iterating global scope\n");
        EXEC_RG(sp_iterate(f, NULL, NULL, NULL, cb_prop, cb_scope,
            NULL, buf, sizeof(buf), buf2, sizeof(buf2)));

        printf("\n--- Iterating splitted scope /:1/:2/:3\n");
        EXEC_RG(sp_iterate(f, NULL, "/1/2/3", "", cb_prop, cb_scope,
            NULL, buf, sizeof(buf), buf2, sizeof(buf2)));

finish:
        if (ret) printf("Error: %d\n", ret);
        fclose(f);
    }
    return 0;
}
