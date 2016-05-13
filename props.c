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

/* path separators markers */
#define C_SEP_SCP    '/'
#define C_SEP_TYP    ':'
#define C_SEP_SIND   '@'

/* scope index markers */
#define C_IND_ALL   '*'
#define C_IND_LAST  '$'

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

typedef struct _lastsc_t
{
    int present;        /* if !=0: the struct describes last scope */
    const char *beg;    /* points to part of original path spec.  related to
                           the scope content (beginning of its path; end of
                           the path as in the original path) */
    sp_loc_t lbody;     /* scope body; for scopes w/o body line/column indexes
                           are set to 0 */
    sp_loc_t ldef;      /* scope definition */
} lastsc_t;

/* Base struct for other handlers like iter_hndl_t or getprp_hndl_t.

   NOTE: Along with its deriving structs, base_hndl_t is copied during
   upward-downward process of following a destination scope path.
 */
typedef struct _base_hndl_t
{
    /* pointer to the finish flag (shared) */
    int *p_finish;

    /* pointer to last scope spec. (shared) */
    lastsc_t *p_lsc;

    /* pointer to split scope tracking index (shared) */
    int *p_sind;

    /* path to the destination scope (not propagated) */
    path_t path;
} base_hndl_t;

/* Initialize base_hndl_t struct.

   NOTE: The function may cut last part of the path and exclude it from the path
   specification. In this case 'p_lprt' shall be !=NULL. A pointer to the last
   part will be written under 'p_lprt'.
 */
static void init_base_hndl(
    base_hndl_t *p_base, int *const p_finish, lastsc_t *p_lsc, int *p_sind,
    const char *path, const char **p_lprt, const char *defsc)
{
    p_base->p_finish = p_finish;
    *(p_base->p_finish) = 0;

    p_base->p_lsc = p_lsc;
    memset(p_base->p_lsc, 0, sizeof(*p_base->p_lsc));

    p_base->p_sind = p_sind;
    *(p_base->p_sind) = -1;

    if (p_lprt)
    {
        /* cut last part of the path spec. */
        *p_lprt = strrchr(path, C_SEP_SCP);
        if (*p_lprt) {
            p_base->path.beg = path;
            p_base->path.end = (*p_lprt)++;
        } else {
            *p_lprt = path;
            p_base->path.beg = p_base->path.end = NULL;
        }
    } else {
        p_base->path.beg = path;
        p_base->path.end = (!path ? NULL : p_base->path.beg+strlen(path));
    }

    if (p_base->path.beg && *p_base->path.beg==C_SEP_SCP) p_base->path.beg++;
    p_base->path.defsc = defsc;
}

/* Iteration handle

   NOTE: This struct is copied during upward-downward process of following
   a destination scope path, so any changes made on it are not propagated to
   scopes on higher or the same scope level. Therefore, if such propagation is
   required a struct's field must be an immutable pointer to an object
   containing propagated information.
 */
typedef struct _iter_hndl_t
{
    base_hndl_t b;

    /* parsed input file handle (const) */
    FILE *in;

    struct {
        /* argument passed untouched (const) */
        void *arg;

        /* user API callbacks (const) */
        sp_cb_prop_t prop;
        sp_cb_scope_t scope;
    } cb;

    /* buffer 1 (property name/scope type name; const) */
    struct {
        char *ptr;
        size_t sz;
    } buf1;

    /* buffer 2 (property vale/scope name; const) */
    struct {
        char *ptr;
        size_t sz;
    } buf2;
} iter_hndl_t;

/* Extract index value from prop/scope name and write the result under 'p_ind'.
   Number of chars read and constituting the index specification is written under
   'p_ind_len' (0 if such spec. is absent). If the spec. is present but erroneous
   SPEC_INV_PATH is returned.
 */
static sp_errc_t get_ind_from_name(
    const char *p_name, size_t name_len, int *p_ind, size_t *p_ind_len)
{
    sp_errc_t ret=SPEC_SUCCESS;
    size_t i;

    *p_ind=0;
    *p_ind_len=0;

    for (i=name_len, p_name+=name_len-1;
        i && *p_name!=C_SEP_SIND;
        i--, p_name--);

    if (!i)
        /* no index spec. */
        goto finish;

     *p_ind_len=name_len-i+1;
     if (*p_ind_len <= 1) {
        /* no chars after marker */
        ret=SPEC_INV_PATH;
        goto finish;
    }

    p_name++;

    if (*p_ind_len==2 && (*p_name==C_IND_ALL || *p_name==C_IND_LAST)) {
        *p_ind = (*p_name==C_IND_LAST ? IND_LAST : IND_ALL);
    } else {
        /* parse decimal number after marker */
        for (i=1; i<*p_ind_len; i++, p_name++) {
            if (*p_name>='0' && *p_name<='9') {
                *p_ind = *p_ind*10 + (*p_name-'0');
            } else {
                /* invalid number */
                *p_ind=0;
                ret=SPEC_INV_PATH;
                goto finish;
            }
        }
    }

finish:
    return ret;
}

/* Follow requested path up to the destination scope.

   The function accepts a clone of enclosing scope handle pointed by 'ph_nst'
   which is next updated (actually its base part pointed by 'ph_nstb'), to
   represent nesting scope. The nesting scope is followed if its characteristic
   meets scope criteria provided in the path).
 */
static sp_errc_t follow_scope_path(const sp_parser_hndl_t *p_phndl,
    base_hndl_t *ph_nstb, void *ph_nst, const sp_loc_t *p_ltype,
    const sp_loc_t *p_lname, const sp_loc_t *p_lbody, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;

    int ind, *p_sind=ph_nstb->p_sind;
    size_t typ_len=0, nm_len=0, ind_len;
    const char *type=NULL, *name=NULL;
    const path_t *p_path = &ph_nstb->path;
    const char *beg=p_path->beg, *end=p_path->end;
    const char *col=strchr(beg, C_SEP_TYP), *sl=strchr(beg, C_SEP_SCP);

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

    EXEC_RG(get_ind_from_name(name, nm_len, &ind, &ind_len));
    if (!(nm_len-=ind_len)) { ret=SPEC_INV_PATH; goto finish; }
    if (!ind_len) ind=IND_ALL;  /* if not specified, IND_ALL is assumed */

    CMPLOC_RG(p_phndl, SP_TKN_ID, p_ltype, type, typ_len);
    CMPLOC_RG(p_phndl, SP_TKN_ID, p_lname, name, nm_len);

    /* scope with matching name found */

    if (ind!=IND_ALL)
        /* tracking index is updated only if the matched
           scope was provided with an index specification */
        *p_sind += 1;

    if (ind==IND_LAST)
    {
        /* for last scope spec. simply track the scope */
        ph_nstb->p_lsc->present = 1;
        ph_nstb->p_lsc->beg = (!*end ? end : end+1);
        if (p_lbody) {
            ph_nstb->p_lsc->lbody = *p_lbody;
        } else {
            memset(&ph_nstb->p_lsc->lbody, 0, sizeof(ph_nstb->p_lsc->lbody));
        }
        ph_nstb->p_lsc->ldef = *p_ldef;
    } else
    if (p_lbody && (ind==IND_ALL || *p_sind==ind))
    {
        /* follow the path for matching index */
        int sind = -1;
        sp_parser_hndl_t phndl;

        if (ind!=IND_ALL)
            /* there is a need to start tracking in the followed scope */
            ph_nstb->p_sind = &sind;

        ph_nstb->path.beg = (!*end ? end : end+1);

        EXEC_RG(sp_parser_hndl_init(&phndl, p_phndl->in, p_lbody,
            p_phndl->cb.prop, p_phndl->cb.scope, ph_nst));
        EXEC_RG(sp_parse(&phndl));
    }

finish:
    return ret;
}

/* Clone nested scope handle (basing on the enclosing scope handle 'p_hndl')
   of type 'hndl_t' and follow the scope path. After the call finish flag is
   checked. To be used inside scope callbacks only.
 */
#define __CALL_FOLLOW_SCOPE_PATH(hndl_t, p_hndl) \
    hndl_t hndl = *p_hndl; \
    ret = follow_scope_path( \
        p_phndl, &hndl.b, &hndl, p_ltype, p_lname, p_lbody, p_ldef); \
    if (ret==SPEC_SUCCESS && *(p_hndl->b.p_finish)!=0) \
        ret = SPEC_CB_FINISH;

/* check iteration callback return code */
#define __CHK_ITER_CB_RET() \
    if (ret==SPEC_CB_FINISH) \
        *p_ihndl->b.p_finish = 1; \
    else if ((int)ret<0) \
        ret=SPEC_CB_RET_ERR;

/* sp_iterate() callback: property */
static sp_errc_t iter_cb_prop(const sp_parser_hndl_t *p_phndl,
    const sp_loc_t *p_lname, const sp_loc_t *p_lval, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    iter_hndl_t *p_ihndl = (iter_hndl_t*)p_phndl->cb.arg;

    /* ignore props until the destination scope */
    if ((p_ihndl->b.path.beg >= p_ihndl->b.path.end) && p_ihndl->cb.prop)
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

        __CHK_ITER_CB_RET();
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

    if (p_ihndl->b.path.beg < p_ihndl->b.path.end) {
        __CALL_FOLLOW_SCOPE_PATH(iter_hndl_t, p_ihndl);
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

        __CHK_ITER_CB_RET();
    }
finish:
    return ret;
}

#undef __CHK_ITER_CB_RET

#define __PARSE_WITH_LSC_HANDLE(hndl) \
    for (;;) { \
        sp_loc_t lsc_bdy; \
        EXEC_RG(sp_parse(&phndl)); \
        if (lsc.present && !f_finish) { \
            /* last scope spec. detected; need to re-parse the last scope */ \
            if (!lsc.lbody.first_column) { \
                break; /* empty scope; skip further processing */ \
            } \
            hndl.b.path.beg = lsc.beg; \
            lsc_bdy = lsc.lbody; \
            memset(&lsc, 0, sizeof(lsc)); \
            sind = -1; \
            EXEC_RG(sp_parser_hndl_init( \
                &phndl, in, &lsc_bdy, iter_cb_prop, iter_cb_scope, &hndl)); \
        } else break; \
    }

/* exported; see header for details */
sp_errc_t sp_iterate(FILE *in, const sp_loc_t *p_parsc, const char *path,
    const char *defsc, sp_cb_prop_t cb_prop, sp_cb_scope_t cb_scope,
    void *arg, char *buf1, size_t b1len, char *buf2, size_t b2len)
{
    sp_errc_t ret=SPEC_SUCCESS;
    iter_hndl_t ihndl;
    sp_parser_hndl_t phndl;

    /* processing finish flag (shared) */
    int f_finish;
    /* last scope spec. (shared) */
    lastsc_t lsc;
    /* split scope tracking index (shared) */
    int sind;

    if (!in || (!cb_prop && !cb_scope)) {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    memset(&ihndl, 0, sizeof(ihndl));
    init_base_hndl(&ihndl.b, &f_finish, &lsc, &sind, path, NULL, defsc);

    ihndl.in = in;

    ihndl.cb.arg = arg;
    ihndl.cb.prop = cb_prop;
    ihndl.cb.scope = cb_scope;

    if (b1len) {
        ihndl.buf1.ptr = buf1;
        ihndl.buf1.sz = b1len-1;
        ihndl.buf1.ptr[ihndl.buf1.sz] = 0;
    }
    if (b2len) {
        ihndl.buf2.ptr = buf2;
        ihndl.buf2.sz = b2len-1;
        ihndl.buf2.ptr[ihndl.buf2.sz] = 0;
    }

    EXEC_RG(sp_parser_hndl_init(
        &phndl, in, p_parsc, iter_cb_prop, iter_cb_scope, &ihndl));
    __PARSE_WITH_LSC_HANDLE(ihndl);

finish:
    return ret;
}

/* sp_get_prop() iteration handle struct.

   NOTE: This struct is copied during upward-downward process of following
   a destination scope path.
 */
typedef struct _getprp_hndl_t
{
    base_hndl_t b;

    /* property name & index (const) */
    const char *name;
    size_t name_len;
    int ind;

    /* matched props incremental index (shared) */
    int *p_prop_ind_n;

    /* property value buffer (const) */
    struct {
        char *ptr;
        size_t sz;
    } val;

    /* extra info will be written under this address (shared) */
    sp_prop_info_ex_t *p_info;
} getprp_hndl_t;

/* sp_get_prop() callback: property */
static sp_errc_t getprp_cb_prop(const sp_parser_hndl_t *p_phndl,
    const sp_loc_t *p_lname, const sp_loc_t *p_lval, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    getprp_hndl_t *p_gphndl = (getprp_hndl_t*)p_phndl->cb.arg;

    /* ignore props until the destination scope */
    if (p_gphndl->b.path.beg>=p_gphndl->b.path.end)
    {
        CMPLOC_RG(
            p_phndl, SP_TKN_ID, p_lname, p_gphndl->name, p_gphndl->name_len);
        EXEC_RG(sp_parser_tkn_cpy(p_phndl, SP_TKN_VAL, p_lval,
            p_gphndl->val.ptr, p_gphndl->val.sz, &p_gphndl->p_info->tkval.len));

        /* matching property found */
        *p_gphndl->p_prop_ind_n += 1;

        if (p_gphndl->ind==*p_gphndl->p_prop_ind_n ||
            p_gphndl->ind==IND_LAST)
        {
            p_gphndl->p_info->tkname.len = p_gphndl->name_len;
            p_gphndl->p_info->tkname.loc = *p_lname;
            if (p_lval) {
                p_gphndl->p_info->val_pres = 1;
                p_gphndl->p_info->tkval.loc = *p_lval;
            } else {
                p_gphndl->p_info->val_pres = 0;
            }
            p_gphndl->p_info->ldef = *p_ldef;
        }

        if (p_gphndl->ind==*p_gphndl->p_prop_ind_n) {
            /* requested property found, stop further processing */
            ret = SPEC_CB_FINISH;
            *p_gphndl->b.p_finish = 1;
        }
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

    if (p_gphndl->b.path.beg < p_gphndl->b.path.end) {
        __CALL_FOLLOW_SCOPE_PATH(getprp_hndl_t, p_gphndl);
    }
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_get_prop(FILE *in, const sp_loc_t *p_parsc, const char *name,
    int ind, const char *path, const char *defsc, char *val, size_t len,
    sp_prop_info_ex_t *p_info)
{
    sp_errc_t ret=SPEC_INV_ARG;
    getprp_hndl_t gphndl;
    sp_parser_hndl_t phndl;

    /* processing finish flag (shared) */
    int f_finish;
    /* last scope spec. (shared) */
    lastsc_t lsc;
    /* split scope tracking index (shared) */
    int sind;
    /* matched props inc. index (shared) */
    int prop_ind_n;

    /* line & columns are 1-based, therefore 0 has a special
       meaning to distinguish uninitialized state. */
    sp_prop_info_ex_t info;
    memset(&info, 0, sizeof(info));

    if (!in || !len) goto finish;

    /* prepare callback handle */
    memset(&gphndl, 0, sizeof(gphndl));

    if (!name && !path) goto finish;
    init_base_hndl(
        &gphndl.b, &f_finish, &lsc, &sind, path, (!name ? &name : NULL), defsc);

    gphndl.name = name;
    gphndl.name_len = strlen(name);

    if (ind<0 && ind!=IND_LAST && ind!=IND_INPRM) goto finish;
    if (ind==IND_INPRM) {
        size_t ind_len;

        EXEC_RG(get_ind_from_name(name, gphndl.name_len, &ind, &ind_len));
        if (ind==IND_ALL || !(gphndl.name_len-=ind_len)) {
            ret=SPEC_INV_PATH;
            goto finish;
        }
        /* if not specified, 0 is assumed */
        if (!ind_len) ind=0;
    }

    gphndl.ind = ind;
    prop_ind_n = -1;
    gphndl.p_prop_ind_n = &prop_ind_n;

    gphndl.val.ptr = val;
    gphndl.val.sz = len-1;
    gphndl.val.ptr[gphndl.val.sz] = 0;

    gphndl.p_info = &info;

    EXEC_RG(sp_parser_hndl_init(
        &phndl, in, p_parsc, getprp_cb_prop, getprp_cb_scope, &gphndl));
    __PARSE_WITH_LSC_HANDLE(gphndl);

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
    int ind, const char *path, const char *defsc, long *p_val,
    sp_prop_info_ex_t *p_info)
{
    sp_errc_t ret=SPEC_SUCCESS;
    sp_prop_info_ex_t info;
    char val[80], *end;
    long v=0L;

    EXEC_RG(sp_get_prop(
        in, p_parsc, name, ind, path, defsc, val, sizeof(val), &info));
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
    int ind, const char *path, const char *defsc, double *p_val,
    sp_prop_info_ex_t *p_info)
{
    sp_errc_t ret=SPEC_SUCCESS;
    sp_prop_info_ex_t info;
    char val[80], *end;
    double v=0.0;

    EXEC_RG(sp_get_prop(
        in, p_parsc, name, ind, path, defsc, val, sizeof(val), &info));
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
    FILE *in, const sp_loc_t *p_parsc, const char *name, int ind,
    const char *path, const char *defsc, const sp_enumval_t *p_evals,
    int igncase, char *buf, size_t blen, int *p_val, sp_prop_info_ex_t *p_info)
{
    sp_errc_t ret=SPEC_SUCCESS;
    int v=0;

    sp_prop_info_ex_t info;
    memset(&info, 0, sizeof(info));

    if (!p_evals) { ret=SPEC_INV_ARG; goto finish; }

    EXEC_RG(sp_get_prop(in, p_parsc, name, ind, path, defsc, buf, blen, &info));

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

#undef __PARSE_WITH_LSC_HANDLE
#undef __CALL_FOLLOW_SCOPE_PATH
