/*
   Copyright (c) 2015,2016,2019,2022 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <assert.h>
#include "../config.h"
#include "sprops/utils.h"

#if CONFIG_NO_SEMICOL_ENDS_VAL || \
    !CONFIG_CUT_VAL_LEADING_SPACES || \
    !CONFIG_TRIM_VAL_TRAILING_SPACES || \
    (CONFIG_MAX_SCOPE_LEVEL_DEPTH>0 && CONFIG_MAX_SCOPE_LEVEL_DEPTH<4)
# error Bad configuration
#endif

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

static void print_prop_info(const sp_prop_info_ex_t *p_info)
{
    printf("IND:%d ELM:%d, ", p_info->ind, p_info->n_elem);

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

static void print_scope_info(const sp_scope_info_ex_t *p_info)
{
    printf("IND:%d ELM:%d, ", p_info->ind, p_info->n_elem);

    printf("NAME len:%ld loc:%d.%d|%d.%d [0x%02lx|0x%02lx], ",
        p_info->tkname.len,
        p_info->tkname.loc.first_line,
        p_info->tkname.loc.first_column,
        p_info->tkname.loc.last_line,
        p_info->tkname.loc.last_column,
        p_info->tkname.loc.beg,
        p_info->tkname.loc.end);

    if (p_info->type_pres) {
        printf("TYPE len:%ld loc:%d.%d|%d.%d [0x%02lx|0x%02lx], ",
            p_info->tktype.len,
            p_info->tktype.loc.first_line,
            p_info->tktype.loc.first_column,
            p_info->tktype.loc.last_line,
            p_info->tktype.loc.last_column,
            p_info->tktype.loc.beg,
            p_info->tktype.loc.end);
    } else
        printf("TYPE not present, ");

    if (p_info->body_pres) {
        printf("BODY loc %d.%d|%d.%d [0x%02lx|0x%02lx], ",
            p_info->lbody.first_line,
            p_info->lbody.first_column,
            p_info->lbody.last_line,
            p_info->lbody.last_column,
            p_info->lbody.beg,
            p_info->lbody.end);
    } else
        printf("BODY empty, ");

    printf("ENC-BODY loc %d.%d|%d.%d [0x%02lx|0x%02lx], ",
        p_info->lbdyenc.first_line,
        p_info->lbdyenc.first_column,
        p_info->lbdyenc.last_line,
        p_info->lbdyenc.last_column,
        p_info->lbdyenc.beg,
        p_info->lbdyenc.end);

    printf("DEF loc %d.%d|%d.%d [0x%02lx|0x%02lx]\n",
        p_info->ldef.first_line,
        p_info->ldef.first_column,
        p_info->ldef.last_line,
        p_info->ldef.last_column,
        p_info->ldef.beg,
        p_info->ldef.end);
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
    const char *ownscp, const char *prop, int ind,
    const char *val, const sp_prop_info_ex_t *p_info)
{
    printf("OWN-SCP<%s> PROP<%s> IND<%s>, val-str \"%s\": ",
        ownscp, prop, strind(ind), val);
    print_prop_info(p_info);
}

static void print_long_prop(
    const char *ownscp, const char *prop, int ind,
    long val, const sp_prop_info_ex_t *p_info)
{
    printf("OWN-SCP<%s> PROP<%s> IND<%s>, val-long \"%ld\": ",
        ownscp, prop, strind(ind), val);
    print_prop_info(p_info);
}

static void print_double_prop(
    const char *ownscp, const char *prop, int ind,
    double val, const sp_prop_info_ex_t *p_info)
{
    printf("OWN-SCP<%s> PROP<%s> IND<%s>, val-double \"%f\": ",
        ownscp, prop, strind(ind), val);
    print_prop_info(p_info);
}

static void print_enum_prop(
    const char *ownscp, const char *prop, int ind,
    int val, const sp_prop_info_ex_t *p_info)
{
    printf("OWN-SCP<%s> PROP<%s> IND<%s>, enum \"%d\": ",
        ownscp, prop, strind(ind), val);
    print_prop_info(p_info);
}

static void print_scope(const char *ownscp,
    const char *scope, int ind, const sp_scope_info_ex_t *p_info)
{
    printf("OWN-SCP<%s> SCOPE<%s> IND<%s>: ", ownscp, scope, strind(ind));
    print_scope_info(p_info);
}

int main(void)
{
    sp_errc_t ret=SPEC_SUCCESS;

    int ival;
    long lval;
    double dval;
    char buf1[8], in_buf[2000];

    sp_prop_info_ex_t pi;
    sp_scope_info_ex_t si;

    SP_FILE in_f, in;
    long in_len;

    sp_enumval_t evals[] =
        {{"false", 0}, {"0", 0}, {"true", 1}, {"1", 1}, {NULL, 0}};

    EXEC_RG(sp_fopen(&in_f, "t01-2.conf", SP_MODE_READ));
#if 0
    EXEC_RG(sp_fopen(&in_f, "t01-2_win.conf", SP_MODE_READ));
    EXEC_RG(sp_fopen(&in_f, "t01-2_1line.conf", SP_MODE_READ));
#endif

    /* copy test's conf. file content into the memory stream (in_buf) */
    sp_mopen(&in, in_buf, sizeof(in_buf));
    ret = sp_util_cpy_to_out(&in_f, &in, 0, EOF, &in_len);
    sp_close(&in_f);
    if (ret!=SPEC_SUCCESS) goto finish;

    in_buf[in_len] = 0;

    /* reset the stream position */
    sp_mopen(&in, in_buf, sizeof(in_buf));

    printf("--- Properties info\n");

    EXEC_RG(sp_get_prop(
        &in, NULL, "a", 0, NULL, NULL, buf1, sizeof(buf1), &pi));
    print_str_prop("/", "a", 0, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "b", 0, "/", NULL, buf1, sizeof(buf1), &pi));
    print_str_prop("/", "b", 0, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "}'\"{", 0, NULL, NULL, buf1, sizeof(buf1), &pi));
    print_str_prop("/", "}'\"{", 0, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, ";\"'#", SP_IND_LAST, NULL, NULL, buf1, sizeof(buf1), &pi));
    print_str_prop("/", ";\"'#", SP_IND_LAST, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "a", 0, "/:\\'\\:\\x20\\/", NULL, buf1, sizeof(buf1), &pi));
    print_str_prop("untyped scope ': /", "a", 0, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "a", 0, "/\\x31", "scope", buf1, sizeof(buf1), &pi));
    print_str_prop("/scope:1", "a", 0, buf1, &pi);

    /* truncated to the buffer size */
    EXEC_RG(sp_get_prop(&in, NULL,
        "a", 0, "\\1/\\x73cope:\\2", "scope", buf1, sizeof(buf1), &pi));
    print_str_prop("/scope:1/scope:2", "a", 0, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "b", 0, "/1/2/", "scope", buf1, sizeof(buf1), &pi));
    print_str_prop("/scope:1/scope:2", "b", 0, buf1, &pi);

    EXEC_RG(sp_get_prop_int(
        &in, NULL, "a", 0, "1/2/:xxx/", "scope", &lval, &pi));
    print_long_prop("/scope:1/scope:2/:xxx", "a", 0, lval, &pi);

    EXEC_RG(sp_get_prop_float(
        &in, NULL, "b", 0, "1/2/:xxx", "scope", &dval, &pi));
    print_double_prop("/scope:1/scope:2/:xxx", "b", 0, dval, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "a", 0, "1/2/:xxx/d:d", "scope", buf1, sizeof(buf1), &pi));
    print_str_prop("/scope:1/scope:2/:xxx/d:d", "a", 0, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "a", 0, "/2", "scope", buf1, sizeof(buf1), &pi));
    print_str_prop("/scope:2", "a", 0, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "b", 0, "/2", "scope", buf1, sizeof(buf1), &pi));
    print_str_prop("/scope:2", "b", 0, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "a", 0, "/:scope", NULL, buf1, sizeof(buf1), &pi));
    print_str_prop("/:scope", "a", 0, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "a", 0, "1/2/3", NULL, buf1, sizeof(buf1), &pi));
    print_str_prop("/:1/:2/:3", "a", 0, buf1, &pi);

    ret = sp_get_prop(&in, NULL, "x", 0, "1@0/2/3", "", buf1, sizeof(buf1), &pi);
    assert(ret==SPEC_NOTFOUND);

    /* truncated to the buffer size */
    EXEC_RG(sp_get_prop(
        &in, NULL, "b", 0, "/1/:2/3", "", buf1, sizeof(buf1), &pi));
    print_str_prop("/:1/:2/:3", "b", 0, buf1, &pi);

    ret = sp_get_prop(&in, NULL, "x", 0, "1/2/3@0", "", buf1, sizeof(buf1), &pi);
    assert(ret==SPEC_NOTFOUND);

    EXEC_RG(sp_get_prop_enum(&in, NULL,
        "c", 0, "/1/2/3", NULL, evals, 1, buf1, sizeof(buf1), &ival, &pi));
    print_enum_prop("/:1/:2/:3", "c", 0, ival, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "d", 0, ":1/:2/:3", NULL, buf1, sizeof(buf1), &pi));
    print_str_prop("/:1/:2/:3", "d", 0, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "e", 0, ":1/:2/:3", NULL, buf1, sizeof(buf1), &pi));
    print_str_prop("/:1/:2/:3", "e", 0, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "f", 0, ":1/:2/:3", "", buf1, sizeof(buf1), &pi));
    print_str_prop("/:1/:2/:3", "f", 0, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "g", 0, ":1/:2/:3", "/", buf1, sizeof(buf1), &pi));
    print_str_prop("/:1/:2/:3", "g", 0, buf1, &pi);

    ret = sp_get_prop(
        &in, NULL, "a", 0, "1/2/3/scope:xyz", NULL, buf1, sizeof(buf1), &pi);
    assert(ret==SPEC_NOTFOUND);

    EXEC_RG(sp_get_prop(
        &in, NULL, "a", 0, "3", "scope", buf1, sizeof(buf1), &pi));
    print_str_prop("/scope:3", "a", 0, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "a", 1, "/scope:3", NULL, buf1, sizeof(buf1), &pi));
    print_str_prop("/scope:3", "a", 1, buf1, &pi);

    EXEC_RG(sp_get_prop(
            &in, NULL, "a", 2, "/scope:3", NULL, buf1, sizeof(buf1), &pi));
    print_str_prop("/scope:3", "a", 2, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "a", 3, "/3", "scope", buf1, sizeof(buf1), &pi));
    print_str_prop("/scope:3", "a", 3, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "a", SP_IND_LAST, "scope:3", "", buf1, sizeof(buf1), &pi));
    print_str_prop("/scope:3", "a", SP_IND_LAST, buf1, &pi);

    EXEC_RG(sp_get_prop(&in,
        NULL, "a", SP_IND_LAST, "scope:3@$/", NULL, buf1, sizeof(buf1), &pi));
    print_str_prop("/scope:3@$", "a", SP_IND_LAST, buf1, &pi);

    EXEC_RG(sp_get_prop(
        &in, NULL, "c", 0, NULL, NULL, buf1, sizeof(buf1), &pi));
    print_str_prop("/", "c", 0, buf1, &pi);


    printf("\n--- Scopes info\n");

    EXEC_RG(sp_get_scope_info(
        &in, NULL, NULL, "': /", SP_IND_LAST, NULL, NULL, &si));
    print_scope("/", "': /", SP_IND_LAST, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, "scope", "1", 0, "/", NULL, &si));
    print_scope("/", "/scope:1", 0, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, "scope", "2", 0, "/scope:1", NULL, &si));
    print_scope("/scope:1", "/scope:2", 0, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, NULL, "xxx", 0, "1/2", "scope", &si));
    print_scope("/scope:1/scope:2", "/:xxx", 0, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, "d", "d", 0, "/1/2/:xxx", "scope", &si));
    print_scope("/scope:1/scope:2/:xxx", "/d:d", 0, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, "scope", "2", 0, "", NULL, &si));
    print_scope("/", "/scope:2", 0, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, "scope", "2", 0, "/2", "scope", &si));
    print_scope("/scope:2", "/scope:2", 0, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, "scope", "3", 0, "/scope:2", NULL, &si));
    print_scope("/scope:2", "/scope:3", 0, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, NULL, "scope", 0, NULL, NULL, &si));
    print_scope("/", "/:scope", 0, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, NULL, "1", 0, NULL, NULL, &si));
    print_scope("/", "/:1", 0, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, NULL, "2", 0, "/:1", NULL, &si));
    print_scope("/:1", "/:2", 0, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, NULL, "3", 0, "1/2", "", &si));
    print_scope("/:1/:2", "/:3", 0, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, NULL, "1", 1, NULL, NULL, &si));
    print_scope("/", "/:1", 1, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, NULL, "2", 1, "/:1", NULL, &si));
    print_scope("/:1", "/:2", 1, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, NULL, "3", 1, "1/2", NULL, &si));
    print_scope("/:1/:2", "/:3", 1, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, NULL, "1", 2, NULL, NULL, &si));
    print_scope("/", "/:1", 2, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, NULL, "2", 2, "/:1", NULL, &si));
    print_scope("/:1", "/:2", 2, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, NULL, "3", 2, "1/2", "", &si));
    print_scope("/:1/:2", "/:3", 2, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, "scope", "xyz", 0, "1/2/3", "", &si));
    print_scope("/:1/:2/:3", "/scope:xyz", 0, &si);

    EXEC_RG(sp_get_scope_info(
        &in, NULL, NULL, "3", SP_IND_LAST, "1@2/2@0", "", &si));
    print_scope("/:1@2/:2@0", "/:3", SP_IND_LAST, &si);

    ret = sp_get_scope_info(&in, NULL, NULL, "4", SP_IND_LAST, "1@0/2", "", &si);
    assert(ret==SPEC_NOTFOUND);

    ret = sp_get_scope_info(
        &in, NULL, NULL, "4", SP_IND_LAST, "1@2/2@0", "", &si);
    assert(ret==SPEC_NOTFOUND);

    EXEC_RG(sp_get_scope_info(&in, NULL, NULL, "3", 0, "1@2/2@1", NULL, &si));
    print_scope("/:1@2/:2@1", "/:3", 0, &si);

    EXEC_RG(sp_get_scope_info(
        &in, NULL, NULL, "3", SP_IND_LAST, "1@2/2@1", "", &si));
    print_scope("/:1@2/:2@1", "/:3", SP_IND_LAST, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, NULL, "2", 3, "/:1", NULL, &si));
    print_scope("/:1", "/:2", 3, &si);

    EXEC_RG(sp_get_scope_info(&in, NULL, "scope", "3", 0, "/", NULL, &si));
    print_scope("/", "/scope:3", 0, &si);

    EXEC_RG(sp_get_scope_info(
        &in, NULL, "scope", "3", SP_IND_LAST, "/", NULL, &si));
    print_scope("/", "/scope:3", SP_IND_LAST, &si);

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
    return 0;
}
