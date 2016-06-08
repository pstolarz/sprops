/*
   Copyright (c) 2015,2016 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "sp_props/props.h"

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

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

static const char *strind(int ind)
{
    static char buf[16];
    switch (ind)
    {
    case SP_IND_LAST:
        buf[0]='$'; buf[1]=0;
        break;
    case SP_IND_ALL:
        buf[0]='*'; buf[1]=0;
        break;
    default:
        sprintf(buf, "%d", ind);
        break;
    }
    return buf;
}

static void print_str_prop(
    const char *scp, const char *prop, int ind,
    const char *val, const sp_prop_info_ex_t *p_info)
{
    printf("SCP<%s> PROP<%s> IND<%s>, val-str \"%s\": ",
        scp, prop, strind(ind), val);
    print_info_ex(p_info);
}

static void print_long_prop(
    const char *scp, const char *prop, int ind,
    long val, const sp_prop_info_ex_t *p_info)
{
    printf("SCP<%s> PROP<%s> IND<%s>, val-long \"%ld\": ",
        scp, prop, strind(ind), val);
    print_info_ex(p_info);
}

static void print_double_prop(
    const char *scp, const char *prop, int ind,
    double val, const sp_prop_info_ex_t *p_info)
{
    printf("SCP<%s> PROP<%s> IND<%s>, val-double \"%f\": ",
        scp, prop, strind(ind), val);
    print_info_ex(p_info);
}

static void print_enum_prop(
    const char *scp, const char *prop, int ind,
    int val, const sp_prop_info_ex_t *p_info)
{
    printf("SCP<%s> PROP<%s> IND<%s>, enum \"%d\": ",
        scp, prop, strind(ind), val);
    print_info_ex(p_info);
}

int main(void)
{
    int ival;
    long lval;
    double dval;
    char buf1[8];
    sp_prop_info_ex_t info;
    sp_errc_t ret=SPEC_SUCCESS;
    FILE *in;

    sp_enumval_t evals[] =
        {{"false", 0}, {"0", 0}, {"true", 1}, {"1", 1}, {NULL, 0}};

    in = fopen("c01-2.conf", "rb");
#if 0
    in = fopen("c01-2_win.conf", "rb");
    in = fopen("c01-2_1line.conf", "rb");
#endif
    if (!in) goto finish;

    printf("--- Properties read\n");

    EXEC_RG(sp_get_prop(
        in, NULL, "a", 0, NULL, NULL, buf1, sizeof(buf1), &info));
    print_str_prop("/", "a", 0, buf1, &info);

    EXEC_RG(sp_get_prop(
        in, NULL, "b", 0, "/", NULL, buf1, sizeof(buf1), &info));
    print_str_prop("/", "b", 0, buf1, &info);

    EXEC_RG( sp_get_prop(
        in, NULL, "}'\"{", 0, NULL, NULL, buf1, sizeof(buf1), &info));
    print_str_prop("/", "}'\"{", 0, buf1, &info);

    EXEC_RG( sp_get_prop(
        in, NULL, ";\"'#", 0, NULL, NULL, buf1, sizeof(buf1), &info));
    print_str_prop("/", ";\"'#", 0, buf1, &info);

    EXEC_RG(sp_get_prop(
        in, NULL, "a", 0, "/:\\'\\:\\x20\\/", NULL, buf1, sizeof(buf1), &info));
    print_str_prop("untyped scope ': /", "a", 0, buf1, &info);

    EXEC_RG(sp_get_prop(
        in, NULL, "a", 0, "/\\x31", "scope", buf1, sizeof(buf1), &info));
    print_str_prop("/scope:1", "a", 0, buf1, &info);

    /* truncated to the buffer size */
    EXEC_RG(sp_get_prop(in, NULL,
        "a", 0, "\\1/\\x73cope:\\2", "scope", buf1, sizeof(buf1), &info));
    print_str_prop("/scope:1/scope:2", "a", 0, buf1, &info);

    EXEC_RG( sp_get_prop(
        in, NULL, "b", 0, "/1/2/", "scope", buf1, sizeof(buf1), &info));
    print_str_prop("/scope:1/scope:2", "b", 0, buf1, &info);

    EXEC_RG(sp_get_prop_int(
        in, NULL, "a", 0, "1/2/:xxx/", "scope", &lval, &info));
    print_long_prop("/scope:1/scope:2/:xxx", "a", 0, lval, &info);

    EXEC_RG(sp_get_prop_float(
        in, NULL, "b", 0, "1/2/:xxx", "scope", &dval, &info));
    print_double_prop("/scope:1/scope:2/:xxx", "b", 0, dval, &info);

    EXEC_RG(sp_get_prop(
        in, NULL, "a", 0, "1/2/:xxx/d:d", "scope", buf1, sizeof(buf1), &info));
    print_str_prop("/scope:1/scope:2/:xxx/d:d", "a", 0, buf1, &info);

    EXEC_RG(sp_get_prop(
        in, NULL, "a", 0, "/2", "scope", buf1, sizeof(buf1), &info));
    print_str_prop("/scope:2", "a", 0, buf1, &info);

    EXEC_RG(sp_get_prop(
        in, NULL, "b", 0, "/2", "scope", buf1, sizeof(buf1), &info));
    print_str_prop("/scope:2", "b", 0, buf1, &info);

    EXEC_RG(sp_get_prop(
        in, NULL, "a", 0, "/:scope", NULL, buf1, sizeof(buf1), &info));
    print_str_prop("/:scope", "a", 0, buf1, &info);

    EXEC_RG(sp_get_prop(
        in, NULL, "a", 0, "1/2/3", "", buf1, sizeof(buf1), &info));
    print_str_prop("/:1/:2/:3", "a", 0, buf1, &info);

    /* truncated to the buffer size */
    EXEC_RG(sp_get_prop(
        in, NULL, "b", 0, "/1/:2/3", "", buf1, sizeof(buf1), &info));
    print_str_prop("/:1/:2/:3", "b", 0, buf1, &info);

    EXEC_RG(sp_get_prop_enum(in, NULL,
        "c", 0, "/1/2/3", "", evals, 1, buf1, sizeof(buf1), &ival, &info));
    print_enum_prop("/:1/:2/:3", "c", 0, ival, &info);

    EXEC_RG(sp_get_prop(
        in, NULL, "d", 0, ":1/:2/:3", NULL, buf1, sizeof(buf1), &info));
    print_str_prop("/:1/:2/:3", "d", 0, buf1, &info);

    EXEC_RG(sp_get_prop(
        in, NULL, "e", 0, ":1/:2/:3", NULL, buf1, sizeof(buf1), &info));
    print_str_prop("/:1/:2/:3", "e", 0, buf1, &info);

    EXEC_RG(sp_get_prop(
        in, NULL, "f", 0, ":1/:2/:3", "", buf1, sizeof(buf1), &info));
    print_str_prop("/:1/:2/:3", "f", 0, buf1, &info);

    EXEC_RG(sp_get_prop(
        in, NULL, "g", 0, ":1/:2/:3", "/", buf1, sizeof(buf1), &info));
    print_str_prop("/:1/:2/:3", "g", 0, buf1, &info);

    ret = sp_get_prop(
        in, NULL, "a", 0, "1/2/3/scope:xyz", "", buf1, sizeof(buf1), &info);
    assert(ret==SPEC_NOTFOUND);

    EXEC_RG(sp_get_prop(
        in, NULL, "a", 0, "3", "scope", buf1, sizeof(buf1), &info));
    print_str_prop("/scope:3", "a", 0, buf1, &info);

    EXEC_RG(sp_get_prop(
        in, NULL, "a", 1, "/scope:3", NULL, buf1, sizeof(buf1), &info));
    print_str_prop("/scope:3", "a", 1, buf1, &info);

    EXEC_RG(sp_get_prop(
            in, NULL, "a", 2, "/scope:3", NULL, buf1, sizeof(buf1), &info));
    print_str_prop("/scope:3", "a", 2, buf1, &info);

    EXEC_RG(sp_get_prop(
        in, NULL, "a", 3, "/3", "scope", buf1, sizeof(buf1), &info));
    print_str_prop("/scope:3", "a", 3, buf1, &info);

    EXEC_RG(sp_get_prop(
        in, NULL, "a", SP_IND_LAST, "scope:3", "", buf1, sizeof(buf1), &info));
    print_str_prop("/scope:3", "a", SP_IND_LAST, buf1, &info);

    EXEC_RG(sp_get_prop(in,
        NULL, "a", SP_IND_LAST, "scope:3@$/", "", buf1, sizeof(buf1), &info));
    print_str_prop("/scope:3@$", "a", SP_IND_LAST, buf1, &info);

    EXEC_RG(sp_get_prop(
        in, NULL, "c", 0, NULL, NULL, buf1, sizeof(buf1), &info));
    print_str_prop("/", "c", 0, buf1, &info);

finish:
    if (ret) printf("Error: %d\n", ret);
    if (in) fclose(in);

    return 0;
}
