/*
   Copyright (c) 2015 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "sp_props/props.h"

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;
#define UPPER_CASE(s) for (; *(s); (s)++) *(s)=toupper((int)*(s));

static sp_errc_t cb_prop(
    void *arg, FILE *in, char *name, const sp_tkn_info_t *p_tkname,
    char *val, const sp_tkn_info_t *p_tkval, const sp_loc_t *p_ldef)
{
    int cb_bf=0;

    if (!strncmp(name, "p_vm", 4)) {
        UPPER_CASE(val);
        cb_bf = SP_CBEC_FLG_MOD_PROP_VAL;
    } else
    if (!strncmp(name, "p_nm_va", 7)) {
        UPPER_CASE(name);
        strcpy(val, "value added;");
        cb_bf = SP_CBEC_FLG_MOD_PROP_VAL|SP_CBEC_FLG_MOD_PROP_NAME;
    } else
    if (!strncmp(name, "p_vd", 4)) {
        *val = 0;
        cb_bf = SP_CBEC_FLG_MOD_PROP_VAL;
    } else
    if (!strncmp(name, "p_dd", 4)) {
        cb_bf = SP_CBEC_FLG_DEL_DEF;
    }

    return (sp_errc_t)sp_cb_errc(cb_bf);
}

static sp_errc_t cb_scope(void *arg, FILE *in, FILE *out, char *type,
    const sp_tkn_info_t *p_tktype, char *name, const sp_tkn_info_t *p_tkname,
    const sp_loc_t *p_lbody, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    int cb_bf=0;
    char buf1[32], buf2[32];

    if (!strncmp(name, "s_tm_nm", 7)) {
        UPPER_CASE(name);
        UPPER_CASE(type)
        cb_bf = SP_CBEC_FLG_MOD_SCOPE_TYPE|SP_CBEC_FLG_MOD_SCOPE_NAME;
    } else
    if (!strncmp(name, "s_td", 4)) {
        *type = 0;
        cb_bf = SP_CBEC_FLG_MOD_SCOPE_TYPE;
    } else
    if (!strncmp(name, "s_ta", 4)) {
        strcpy(type, "type added");
        cb_bf = SP_CBEC_FLG_MOD_SCOPE_TYPE;
    } else
    if (!strncmp(name, "s_dd", 4)) {
        cb_bf = SP_CBEC_FLG_DEL_DEF;
    } else
    if (!strncmp(name, "s_bm", 4) && p_lbody) {
        EXEC_RG(sp_iterate_modify(in, out, p_lbody, NULL, NULL, cb_prop,
            cb_scope, NULL, buf1, sizeof(buf1), buf2, sizeof(buf2)));
    }

    ret = (sp_errc_t)sp_cb_errc(cb_bf);
finish:
    return ret;
}

int main(void)
{
    sp_errc_t ret=SPEC_SUCCESS;
    char buf1[32], buf2[32];

    FILE *in = fopen("spset_mit.conf", "rb");
    if (!in) goto finish;

    printf("------------------------------------------------- global scope\n");
    EXEC_RG(sp_iterate_modify(in, stdout, NULL, NULL, NULL, cb_prop, cb_scope,
        NULL, buf1, sizeof(buf1), buf2, sizeof(buf2)));

    printf("\n------------------------------------------------- inner scope\n");
    EXEC_RG(sp_iterate_modify(in, stdout, NULL, "1/2", "scope", cb_prop,
        cb_scope, NULL, buf1, sizeof(buf1), buf2, sizeof(buf2)));

finish:
    if (ret) printf("Error: %d\n", ret);
    if (in) fclose(in);

    return 0;
}
