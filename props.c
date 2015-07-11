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
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "sp_props/parser.h"

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

/* to be used inside callbacks only */
#define CMPLOC_RG(ph, tkn, loc, str, len) \
    ret = sp_parser_tkn_cmp((ph), (tkn), (loc), (str), (len)); \
    if (ret>0) goto finish; \
    if (ret<0) { ret=0; goto finish; }

/* exported; see header for details */
sp_errc_t sp_check_syntax(
    FILE *in, const sp_loc_t *p_parsc, int *p_line, int *p_col)
{
    sp_errc_t ret;
    sp_parser_hndl_t phndl;

    if (!in) { ret=SPEC_INV_ARG; goto finish; }

    EXEC_RG(sp_parser_hndl_init(&phndl, in, p_parsc, NULL, NULL, NULL));
    EXEC_RG(sp_parse(&phndl));

finish:
    if (ret==SPEC_SYNTAX) {
        if (p_line) *p_line = phndl.err.loc.line;
        if (p_col) *p_col = phndl.err.loc.col;
    }
    return ret;
}

typedef struct _path_t
{
    const char *beg;        /* start pointer */
    const char *end;        /* end pointer (exclusive) */
    const char *defsc;      /* default scope */
} path_t;

typedef struct _iter_hndl_t
{
    /* path to the destination scope */
    path_t path;

    struct {
        /* argument passed untouched */
        void *arg;

        /* user API callbacks */
        sp_cb_prop_t prop;
        sp_cb_scope_t scope;
    } cb;

    /* buffer 1 (property name/section type name) */
    struct {
        char *ptr;
        size_t sz;
    } buf1;

    /* buffer 2 (property vale/section name) */
    struct {
        char *ptr;
        size_t sz;
    } buf2;
} iter_hndl_t;

/* follow requested path up to the destination scope */
static int follow_scope_path(const sp_parser_hndl_t *p_phndl, path_t *p_path,
    const sp_loc_t *p_ltype, const sp_loc_t *p_lname, const sp_loc_t *p_lbody,
    void *p_pargcpy)
{
    int ret=0;
    size_t typ_len=0, nm_len=0;
    const char *type=NULL, *name=NULL, *beg=p_path->beg, *end=p_path->end;
    const char *col=strchr(beg, ':'), *sl=strchr(beg, '/');

    if (sl) end=sl;
    if (col>=end) col=NULL;

    if (col) {
        /* type ':' name */
        type = beg;
        typ_len = col-beg;
        name = col+1;
        nm_len = end-name;
    } else
    if (p_path->defsc)
    {
        /* scope with default type */
        type = p_path->defsc;
        typ_len = strlen(type);
        name = beg;
        nm_len = end-beg;
    }

    if (!nm_len) { ret=SPEC_INV_PATH; goto finish; }

    CMPLOC_RG(p_phndl, TKN_ID, p_ltype, type, typ_len);
    CMPLOC_RG(p_phndl, TKN_ID, p_lname, name, nm_len);

    /* matched scope found; follow the path */
    if (p_lbody) {
        sp_parser_hndl_t phndl;
        p_path->beg = (!*end ? end : end+1);

        EXEC_RG(sp_parser_hndl_init(&phndl, p_phndl->in, p_lbody,
            p_phndl->cb.prop, p_phndl->cb.scope, p_pargcpy));

        EXEC_RG(sp_parse(&phndl));
    }
finish:
    return ret;
}

/* sp_iterate() callback: property */
static int iter_cb_prop(const sp_parser_hndl_t *p_phndl,
    const sp_loc_t *p_lname, const sp_loc_t *p_lval, const sp_loc_t *p_ldef)
{
    int ret=0;
    iter_hndl_t *p_ihndl = (iter_hndl_t*)p_phndl->cb.arg;

    /* ignore props until the destination point */
    if (p_ihndl->path.beg>=p_ihndl->path.end && p_ihndl->cb.prop)
    {
        sp_tkn_info_t tkname, tkval;

        EXEC_RG(sp_parser_tkn_cpy(
            p_phndl, TKN_ID, p_lname, p_ihndl->buf1.ptr, p_ihndl->buf1.sz, &tkname.len));
        EXEC_RG(sp_parser_tkn_cpy(
            p_phndl, TKN_VAL, p_lval, p_ihndl->buf2.ptr, p_ihndl->buf2.sz, &tkval.len));

        tkname.loc = *p_lname;
        if (p_lval) tkval.loc = *p_lval;

        ret = p_ihndl->cb.prop(p_ihndl->cb.arg, p_phndl->in, p_ihndl->buf1.ptr,
            &tkname, p_ihndl->buf2.ptr, (p_lval ? &tkval : NULL), p_ldef);
    }
finish:
    return ret;
}

/* sp_iterate() callback: scope */
static int iter_cb_scope(const sp_parser_hndl_t *p_phndl, const sp_loc_t *p_ltype,
    const sp_loc_t *p_lname, const sp_loc_t *p_lbody, const sp_loc_t *p_ldef)
{
    int ret=0;
    iter_hndl_t *p_ihndl=(iter_hndl_t*)p_phndl->cb.arg;

    if (p_ihndl->path.beg < p_ihndl->path.end) {
        iter_hndl_t ihndl = *p_ihndl;
        ret = follow_scope_path(
            p_phndl, &ihndl.path, p_ltype, p_lname, p_lbody, &ihndl);
    } else
    if (p_ihndl->cb.scope)
    {
        sp_tkn_info_t tktype, tkname;

        EXEC_RG(sp_parser_tkn_cpy(
            p_phndl, TKN_ID, p_ltype, p_ihndl->buf1.ptr, p_ihndl->buf1.sz, &tktype.len));
        EXEC_RG(sp_parser_tkn_cpy(
            p_phndl, TKN_ID, p_lname, p_ihndl->buf2.ptr, p_ihndl->buf2.sz, &tkname.len));

        if (p_ltype) tktype.loc = *p_ltype;
        tkname.loc = *p_lname;

        ret = p_ihndl->cb.scope(p_ihndl->cb.arg, p_phndl->in, p_ihndl->buf1.ptr,
            (p_ltype ? &tktype : NULL), p_ihndl->buf2.ptr, &tkname, p_lbody, p_ldef);
    }
finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_iterate(FILE *in, const sp_loc_t *p_parsc, const char *path,
    const char *defsc, sp_cb_prop_t cb_prop, sp_cb_scope_t cb_scope,
    void *arg, char *p_buf1, size_t b1len, char *p_buf2, size_t b2len)
{
    sp_errc_t ret;
    iter_hndl_t ihndl;
    sp_parser_hndl_t phndl;

    if (!in) { ret=SPEC_INV_ARG; goto finish; }

    memset(&ihndl, 0, sizeof(ihndl));
    if (b1len) {
        ihndl.buf1.ptr = p_buf1;
        ihndl.buf1.sz = b1len-1;
        ihndl.buf1.ptr[ihndl.buf1.sz] = 0;
    }
    if (b2len) {
        ihndl.buf2.ptr = p_buf2;
        ihndl.buf2.sz = b2len-1;
        ihndl.buf2.ptr[ihndl.buf2.sz] = 0;
    }

    if (path && *path=='/') path++;
    ihndl.path.beg = path;
    ihndl.path.end = (!path ? NULL : ihndl.path.beg+strlen(path));
    ihndl.path.defsc = defsc;

    ihndl.cb.arg = arg;
    ihndl.cb.prop = cb_prop;
    ihndl.cb.scope = cb_scope;

    EXEC_RG(sp_parser_hndl_init(
        &phndl, in, p_parsc, iter_cb_prop, iter_cb_scope, &ihndl));
    EXEC_RG(sp_parse(&phndl));

finish:
    return ret;
}

typedef struct _getprp_hndl_t
{
    /* path to the destination scope */
    path_t path;

    /* property name */
    const char *name;

    /* property value buffer */
    struct {
        char *ptr;
        size_t sz;
    } val;

    /* extra info will be written under this address */
    sp_prop_info_ex_t *p_info;
} getprp_hndl_t;

/* sp_get_prop() callback: property */
static int getprp_cb_prop(const sp_parser_hndl_t *p_phndl,
    const sp_loc_t *p_lname, const sp_loc_t *p_lval, const sp_loc_t *p_ldef)
{
    int ret=0;
    getprp_hndl_t *p_gphndl = (getprp_hndl_t*)p_phndl->cb.arg;

    /* ignore props until the destination point */
    if (p_gphndl->path.beg>=p_gphndl->path.end)
    {
        CMPLOC_RG(p_phndl, TKN_ID, p_lname, p_gphndl->name, (size_t)-1);
        EXEC_RG(sp_parser_tkn_cpy(p_phndl, TKN_VAL, p_lval,
            p_gphndl->val.ptr, p_gphndl->val.sz, &p_gphndl->p_info->tkval.len));

        /* property found; done */
        p_gphndl->p_info->tkname.len = strlen(p_gphndl->name);
        p_gphndl->p_info->tkname.loc = *p_lname;
        if (p_lval) {
            p_gphndl->p_info->val_pres = 1;
            p_gphndl->p_info->tkval.loc = *p_lval;
        } else {
            p_gphndl->p_info->val_pres = 0;
        }
        p_gphndl->p_info->ldef = *p_ldef;

        ret = -1;
    }
finish:
    return ret;
}

/* sp_get_prop() callback: scope */
static int getprp_cb_scope(const sp_parser_hndl_t *p_phndl, const sp_loc_t *p_ltype,
    const sp_loc_t *p_lname, const sp_loc_t *p_lbody, const sp_loc_t *p_ldef)
{
    int ret=0;
    getprp_hndl_t *p_gphndl=(getprp_hndl_t*)p_phndl->cb.arg;

    if (p_gphndl->path.beg < p_gphndl->path.end) {
        getprp_hndl_t gphndl = *p_gphndl;
        ret = follow_scope_path(
            p_phndl, &gphndl.path, p_ltype, p_lname, p_lbody, &gphndl);
    }
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_get_prop(FILE *in, const sp_loc_t *p_parsc, const char *name,
    const char *path, const char *defsc, char *p_val, size_t len,
    sp_prop_info_ex_t *p_info)
{
    sp_errc_t ret=SPEC_INV_ARG;
    getprp_hndl_t gphndl;
    sp_parser_hndl_t phndl;

    sp_prop_info_ex_t info;
    memset(&info, 0, sizeof(info));

    if (!in || !len) goto finish;

    /* prepare callback handle */
    memset(&gphndl, 0, sizeof(gphndl));

    gphndl.val.ptr = p_val;
    gphndl.val.sz = len-1;
    gphndl.val.ptr[gphndl.val.sz] = 0;

    if (!name) {
        /* param provided as part of the path spec. */
        if (!path) goto finish;

        name = strrchr(path, '/');
        if (name) {
            gphndl.path.beg = path;
            gphndl.path.end = name++;
        } else {
            name = path;
            gphndl.path.beg = gphndl.path.end = NULL;
        }

        if (strchr(name, ':')) goto finish;
    } else {
        gphndl.path.beg = path;
        gphndl.path.end = (!path ? NULL : gphndl.path.beg+strlen(path));
    }

    gphndl.name = name;

    if (gphndl.path.beg && *gphndl.path.beg=='/') gphndl.path.beg++;
    gphndl.path.defsc = defsc;

    gphndl.p_info = &info;

    EXEC_RG(sp_parser_hndl_init(
        &phndl, in, p_parsc, getprp_cb_prop, getprp_cb_scope, &gphndl));
    EXEC_RG(sp_parse(&phndl));

    /* line & columns are 1-based, if 0, the location of the property
        definition has not been set, therefore has not been found */
    if (!info.ldef.first_line) ret=SPEC_NOTFOUND;

finish:
    if (p_info) *p_info=info;
    return ret;

}

/* exported; see header for details */
sp_errc_t sp_get_prop_int(FILE *in, const sp_loc_t *p_parsc, const char *name,
    const char *path, const char *defsc, long *p_val, sp_prop_info_ex_t *p_info)
{
    sp_errc_t ret=SPEC_SUCCESS;
    sp_prop_info_ex_t info;
    char val[80], *end;
    long v=0L;

    EXEC_RG(sp_get_prop(in, p_parsc, name, path, defsc, val, sizeof(val), &info));

    if (info.val_pres && info.tkval.len>=sizeof(val)) {
        ret=SPEC_VAL_ERR;
        goto finish;
    }

    v = strtol(val, &end, 0);
    if ((v==LONG_MIN || v==LONG_MAX) && errno==ERANGE) {
        ret=SPEC_VAL_ERR; goto finish;
    }

    if (*end) ret=SPEC_VAL_ERR;

finish:
    if (p_info) *p_info=info;
    if (p_val) *p_val=v;
    return ret;
}

static int __stricmp(const char *str1, const char *str2)
{
    int ret;
    size_t i=0;

    for (; !((ret=tolower((int)str1[i])-tolower((int)str2[i]))) &&
        str1[i] && str2[i]; i++);
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_get_prop_enum(FILE *in, const sp_loc_t *p_parsc, const char *name,
    const char *path, const char *defsc, const sp_enumval_t *p_evals, int igncase,
    char *p_buf, size_t blen, int *p_val, sp_prop_info_ex_t *p_info)
{
    sp_errc_t ret=SPEC_SUCCESS;
    int v=0;

    sp_prop_info_ex_t info;
    memset(&info, 0, sizeof(info));

    if (!p_evals) { ret=SPEC_INV_ARG; goto finish; }

    EXEC_RG(sp_get_prop(in, p_parsc, name, path, defsc, p_buf, blen, &info));

    for (; p_evals->name; p_evals++)
    {
        if (strlen(p_evals->name) >= blen) { ret=SPEC_SIZE; goto finish; }

        if (!(igncase ? __stricmp(p_evals->name, p_buf) :
            strcmp(p_evals->name, p_buf))) { v=p_evals->val; break; }
    }
    if (!p_evals->name || (info.val_pres && info.tkval.len>=blen))
        ret=SPEC_VAL_ERR;

finish:
    if (p_info) *p_info=info;
    if (p_val) *p_val=v;
    return ret;
}
