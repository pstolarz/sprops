/*
   Copyright (c) 2015,2016 Piotr Stolarz
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
#include "config.h"
#include "sp_props/parser.h"

/* path separators */
#define SEP_SCP     '/'
#define SEP_TYP     ':'
#define SEP_SIND    '@'

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;

/* to be used inside callbacks only */
#define CMPLOC_RG(ph, tkn, loc, str, len) { \
    int equ=0; \
    ret = (sp_errc_t)sp_parser_tkn_cmp((ph), (tkn), (loc), (str), (len), &equ); \
    if (ret!=SPEC_SUCCESS) goto finish; \
    if (!equ) goto finish; \
}

/* exported; see header for details */
sp_errc_t sp_check_syntax(
    FILE *in, const sp_loc_t *p_parsc, int *p_line, int *p_col)
{
    sp_errc_t ret=SPEC_SUCCESS;
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

/* Iteration handle

   NOTE: This struct is cloned during upward-downward process of following
   a destination scope path, so any changes made on it are not propagated to
   scopes on higher or the same scope level. Therefore, if such propagation is
   required a struct's field must be an immutable pointer to an object
   containing propagated information (aka virtual inheritance).
 */
typedef struct _iter_hndl_t
{
    /* parsed input file handle */
    FILE *in;

    /* path to the destination scope
       NOTE: mutable object not propagated between scopes */
    path_t path;

    struct {
        /* argument passed untouched */
        void *arg;

        /* user API callbacks */
        sp_cb_prop_t prop;
        sp_cb_scope_t scope;
    } cb;

    /* buffer 1 (property name/scope type name) */
    struct {
        char *ptr;
        size_t sz;
    } buf1;

    /* buffer 2 (property vale/scope name) */
    struct {
        char *ptr;
        size_t sz;
    } buf2;
} iter_hndl_t;

/* Follow requested path up to the destination scope. 'p_pargcpy' is a pointer
   to a clone of a parser's callback arg-object associated with the scope being
   followed. Path object pointed by 'p_path' is modified accordingly to the
   scope being followed.
 */
static sp_errc_t follow_scope_path(const sp_parser_hndl_t *p_phndl,
    path_t *p_path, const sp_loc_t *p_ltype, const sp_loc_t *p_lname,
    const sp_loc_t *p_lbody, void *p_pargcpy)
{
    sp_errc_t ret=SPEC_SUCCESS;
    size_t typ_len=0, nm_len=0;
    const char *type=NULL, *name=NULL, *beg=p_path->beg, *end=p_path->end;
    const char *col=strchr(beg, SEP_TYP), *sl=strchr(beg, SEP_SCP);

    if (sl) end=sl;
    if (col>=end) col=NULL;

    if (col) {
        /* type and name specified */
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

    CMPLOC_RG(p_phndl, SP_TKN_ID, p_ltype, type, typ_len);
    CMPLOC_RG(p_phndl, SP_TKN_ID, p_lname, name, nm_len);

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
static sp_errc_t iter_cb_prop(const sp_parser_hndl_t *p_phndl,
    const sp_loc_t *p_lname, const sp_loc_t *p_lval, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    iter_hndl_t *p_ihndl = (iter_hndl_t*)p_phndl->cb.arg;

    /* ignore props until the destination scope */
    if ((p_ihndl->path.beg >= p_ihndl->path.end) && p_ihndl->cb.prop)
    {
        sp_tkn_info_t tkname, tkval;

        EXEC_RG(sp_parser_tkn_cpy(p_phndl, SP_TKN_ID,
            p_lname, p_ihndl->buf1.ptr, p_ihndl->buf1.sz, &tkname.len));
        EXEC_RG(sp_parser_tkn_cpy(p_phndl, SP_TKN_VAL,
            p_lval, p_ihndl->buf2.ptr, p_ihndl->buf2.sz, &tkval.len));

        tkname.loc = *p_lname;
        if (p_lval) tkval.loc = *p_lval;

        ret = p_ihndl->cb.prop(p_ihndl->cb.arg, p_ihndl->in, p_ihndl->buf1.ptr,
            &tkname, p_ihndl->buf2.ptr, (p_lval ? &tkval : NULL), p_ldef);

        if ((int)ret<0 && ret!=SPEC_CB_FINISH) ret=SPEC_CB_RET_ERR;
    }
finish:
    return ret;
}

/* sp_iterate() callback: scope */
static sp_errc_t iter_cb_scope(
    const sp_parser_hndl_t *p_phndl, const sp_loc_t *p_ltype,
    const sp_loc_t *p_lname, const sp_loc_t *p_lbody, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    iter_hndl_t *p_ihndl=(iter_hndl_t*)p_phndl->cb.arg;

    if (p_ihndl->path.beg < p_ihndl->path.end)
    {
        /* clone the handle for the scope being followed */
        iter_hndl_t ihndl = *p_ihndl;
        ret = follow_scope_path(
            p_phndl, &ihndl.path, p_ltype, p_lname, p_lbody, &ihndl);
    } else
    if (p_ihndl->cb.scope)
    {
        sp_tkn_info_t tktype, tkname;

        EXEC_RG(sp_parser_tkn_cpy(p_phndl, SP_TKN_ID,
            p_ltype, p_ihndl->buf1.ptr, p_ihndl->buf1.sz, &tktype.len));
        EXEC_RG(sp_parser_tkn_cpy(p_phndl, SP_TKN_ID,
            p_lname, p_ihndl->buf2.ptr, p_ihndl->buf2.sz, &tkname.len));

        if (p_ltype) tktype.loc = *p_ltype;
        tkname.loc = *p_lname;

        ret = p_ihndl->cb.scope(p_ihndl->cb.arg, p_ihndl->in,
            p_ihndl->buf1.ptr, (p_ltype ? &tktype : NULL),
            p_ihndl->buf2.ptr, &tkname, p_lbody, p_ldef);

        if ((int)ret<0 && ret!=SPEC_CB_FINISH) ret=SPEC_CB_RET_ERR;
    }
finish:
    return ret;
}

/* Initialize iter_hndl_t handle */
static void init_iter_hndl(iter_hndl_t *p_ihndl, FILE *in, const char *path,
    const char *defsc, sp_cb_prop_t cb_prop, sp_cb_scope_t cb_scope,
    void *arg, char *buf1, size_t b1len, char *buf2, size_t b2len)
{
    memset(p_ihndl, 0, sizeof(*p_ihndl));

    p_ihndl->in = in;

    if (b1len) {
        p_ihndl->buf1.ptr = buf1;
        p_ihndl->buf1.sz = b1len-1;
        p_ihndl->buf1.ptr[p_ihndl->buf1.sz] = 0;
    }
    if (b2len) {
        p_ihndl->buf2.ptr = buf2;
        p_ihndl->buf2.sz = b2len-1;
        p_ihndl->buf2.ptr[p_ihndl->buf2.sz] = 0;
    }

    if (path && *path==SEP_SCP) path++;
    p_ihndl->path.beg = path;
    p_ihndl->path.end = (!path ? NULL : p_ihndl->path.beg+strlen(path));
    p_ihndl->path.defsc = defsc;

    p_ihndl->cb.arg = arg;
    p_ihndl->cb.prop = cb_prop;
    p_ihndl->cb.scope = cb_scope;
}

/* exported; see header for details */
sp_errc_t sp_iterate(FILE *in, const sp_loc_t *p_parsc, const char *path,
    const char *defsc, sp_cb_prop_t cb_prop, sp_cb_scope_t cb_scope,
    void *arg, char *buf1, size_t b1len, char *buf2, size_t b2len)
{
    sp_errc_t ret=SPEC_SUCCESS;
    iter_hndl_t ihndl;
    sp_parser_hndl_t phndl;

    if (!in || (!cb_prop && !cb_scope)) {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    init_iter_hndl(&ihndl, in, path, defsc, cb_prop, cb_scope, arg,
        buf1, b1len, buf2, b2len);

    EXEC_RG(sp_parser_hndl_init(
        &phndl, in, p_parsc, iter_cb_prop, iter_cb_scope, &ihndl));
    EXEC_RG(sp_parse(&phndl));

finish:
    return ret;
}

/* sp_get_prop() iteration handle struct.

   NOTE: This struct is cloned during upward-downward process of following
   a destination scope path.
 */
typedef struct _getprp_hndl_t
{
    /* path to the destination scope
       NOTE: mutable object not propagated between scopes */
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
static sp_errc_t getprp_cb_prop(const sp_parser_hndl_t *p_phndl,
    const sp_loc_t *p_lname, const sp_loc_t *p_lval, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    getprp_hndl_t *p_gphndl = (getprp_hndl_t*)p_phndl->cb.arg;

    /* ignore props until the destination scope */
    if (p_gphndl->path.beg>=p_gphndl->path.end)
    {
        CMPLOC_RG(p_phndl, SP_TKN_ID, p_lname, p_gphndl->name, (size_t)-1);
        EXEC_RG(sp_parser_tkn_cpy(p_phndl, SP_TKN_VAL, p_lval,
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

        ret = SPEC_CB_FINISH;
    }
finish:
    return ret;
}

/* sp_get_prop() callback: scope */
static sp_errc_t getprp_cb_scope(
    const sp_parser_hndl_t *p_phndl, const sp_loc_t *p_ltype,
    const sp_loc_t *p_lname, const sp_loc_t *p_lbody, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    getprp_hndl_t *p_gphndl=(getprp_hndl_t*)p_phndl->cb.arg;

    if (p_gphndl->path.beg < p_gphndl->path.end)
    {
        /* clone the handle for the scope being followed */
        getprp_hndl_t gphndl = *p_gphndl;
        ret = follow_scope_path(
            p_phndl, &gphndl.path, p_ltype, p_lname, p_lbody, &gphndl);
    }
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_get_prop(FILE *in, const sp_loc_t *p_parsc, const char *name,
    const char *path, const char *defsc, char *val, size_t len,
    sp_prop_info_ex_t *p_info)
{
    sp_errc_t ret=SPEC_INV_ARG;
    getprp_hndl_t gphndl;
    sp_parser_hndl_t phndl;

    /* line & columns are 1-based, therefore 0 has a special
       meaning to distinguish uninitialized state. */
    sp_prop_info_ex_t info;
    memset(&info, 0, sizeof(info));

    if (!in || !len) goto finish;

    /* prepare callback handle */
    memset(&gphndl, 0, sizeof(gphndl));

    gphndl.val.ptr = val;
    gphndl.val.sz = len-1;
    gphndl.val.ptr[gphndl.val.sz] = 0;

    if (!name) {
        /* property name provided as part of the path spec. */
        if (!path) goto finish;

        name = strrchr(path, SEP_SCP);
        if (name) {
            gphndl.path.beg = path;
            gphndl.path.end = name++;
        } else {
            name = path;
            gphndl.path.beg = gphndl.path.end = NULL;
        }
    } else {
        gphndl.path.beg = path;
        gphndl.path.end = (!path ? NULL : gphndl.path.beg+strlen(path));
    }

    gphndl.name = name;

    if (gphndl.path.beg && *gphndl.path.beg==SEP_SCP) gphndl.path.beg++;
    gphndl.path.defsc = defsc;

    gphndl.p_info = &info;

    EXEC_RG(sp_parser_hndl_init(
        &phndl, in, p_parsc, getprp_cb_prop, getprp_cb_scope, &gphndl));
    EXEC_RG(sp_parse(&phndl));

    /* check if the location of the property definition
       has not been set, therefore has not been found */
    if (!info.ldef.first_column) ret=SPEC_NOTFOUND;

finish:
    if (p_info) *p_info=info;
    return ret;

}

/* Update 'str' by trimming trailing spaces. Updated string length is returned.
 */
static size_t strtrim(char *str)
{
    size_t len = strlen(str);
    for (; len && isspace((int)str[len-1]); len--) str[len-1]=0;
    return len;
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
    if (!info.val_pres || info.tkval.len>=sizeof(val) || !strtrim(val)) {
        ret=SPEC_VAL_ERR;
        goto finish;
    }

    errno = 0;
    v = strtol(val, &end, 0);
    if (errno==ERANGE) { ret=SPEC_VAL_ERR; goto finish; }

    if (*end) ret=SPEC_VAL_ERR;

finish:
    if (p_info) *p_info=info;
    if (p_val) *p_val=v;
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_get_prop_float(FILE *in, const sp_loc_t *p_parsc, const char *name,
    const char *path, const char *defsc, double *p_val, sp_prop_info_ex_t *p_info)
{
    sp_errc_t ret=SPEC_SUCCESS;
    sp_prop_info_ex_t info;
    char val[80], *end;
    double v=0.0;

    EXEC_RG(sp_get_prop(in, p_parsc, name, path, defsc, val, sizeof(val), &info));
    if (!info.val_pres || info.tkval.len>=sizeof(val) || !strtrim(val)) {
        ret=SPEC_VAL_ERR;
        goto finish;
    }

    errno = 0;
    v = strtod(val, &end);
    if (errno==ERANGE) { ret=SPEC_VAL_ERR; goto finish; }

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
sp_errc_t sp_get_prop_enum(
    FILE *in, const sp_loc_t *p_parsc, const char *name, const char *path,
    const char *defsc, const sp_enumval_t *p_evals, int igncase,
    char *buf, size_t blen, int *p_val, sp_prop_info_ex_t *p_info)
{
    sp_errc_t ret=SPEC_SUCCESS;
    int v=0;

    sp_prop_info_ex_t info;
    memset(&info, 0, sizeof(info));

    if (!p_evals) { ret=SPEC_INV_ARG; goto finish; }

    EXEC_RG(sp_get_prop(in, p_parsc, name, path, defsc, buf, blen, &info));

    /* remove leading/trailing spaces */
    for (; isspace((int)*buf); buf++);
    strtrim(buf);

    for (; p_evals->name; p_evals++)
    {
        if (strlen(p_evals->name) >= blen) { ret=SPEC_SIZE; goto finish; }

        if (!(igncase ? __stricmp(p_evals->name, buf) :
            strcmp(p_evals->name, buf))) { v=p_evals->val; break; }
    }
    if (!p_evals->name || (info.val_pres && info.tkval.len>=blen))
        ret=SPEC_VAL_ERR;

finish:
    if (p_info) *p_info=info;
    if (p_val) *p_val=v;
    return ret;
}

