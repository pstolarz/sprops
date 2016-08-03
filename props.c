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
#include "io.h"
#include "sprops/parser.h"
#include "sprops/utils.h"

/* path separators markers */
#define C_SEP_SCP    '/'
#define C_SEP_TYP    ':'
#define C_SEP_SIND   '@'

/* scope index markers */
#define C_IND_ALL   '*'
#define C_IND_LAST  '$'

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;
#define CHK_FSEEK(c) if ((c)!=0) { ret=SPEC_ACCS_ERR; goto finish; }
#define CHK_FERR(c) if ((c)==EOF) { ret=SPEC_ACCS_ERR; goto finish; }

/* to be used inside parser callbacks only */
#define CMPLOC_RG(in, tkn, loc, str, len, esc) { \
    int equ=0; \
    ret = (sp_errc_t)sp_parser_tkn_cmp( \
        (in), (tkn), (loc), (str), (len), (esc), &equ); \
    if (ret!=SPEC_SUCCESS) goto finish; \
    if (!equ) goto finish; \
}

/* exported; see header for details */
sp_errc_t sp_check_syntax(
    SP_FILE *in, const sp_loc_t *p_parsc, sp_synerr_t *p_synerr)
{
    return (!in ? SPEC_INV_ARG :
        sp_parse(in, p_parsc, NULL, NULL, NULL, p_synerr));
}

/* Search for the first/last non-escaped occurrence of char 'c' in string 'str'
   on length 'len'. If len==-1 searching is performed up to the NULL-terminating
   char.

   NOTE: The escaping of char C is defined as \C.
 */
static const char *strchr_nesc(const char *str, size_t len, int c, int last)
{
    int esc=0;
    const char *ret=NULL;

    for (; len && *str;
        str++, len-=(len!=(size_t)-1 ? 1 : 0))
    {
        if (*str=='\\' && !esc) {
            esc=1;
            continue;
        }
        if (*str==c && !esc) {
            ret = str;
            if (!last) break;
        }
        esc=0;
    }
    return ret;
}

typedef struct _path_t
{
    const char *beg;        /* start pointer */
    const char *end;        /* end pointer (exclusive) */
    const char *deftp;      /* default scope type */
} path_t;

typedef struct _lastsc_t
{
    int present;        /* if !=0: the struct describes last scope */
    const char *beg;    /* points to a part of the original path spec. related
                           to the scope content (beginning of its path; end of
                           the path as in the original path) */
    sp_loc_t lbody;     /* scope body; zeroed for scopes w/o a body */
    sp_loc_t ldef;      /* scope definition */
} lastsc_t;

/* Base struct for all (read-only/update) handles.

   NOTE: Along with its deriving structs, the struct is copied during
   upward-downward process of following the destination scope path.
 */
typedef struct _base_hndl_t
{
    /* processing finish flag (shared) */
    int *p_finish;

    /* last scope spec. (shared) */
    lastsc_t *p_lsc;

    /* split scope tracking index (shared) */
    int *p_sind;

    /* destination scope path (not propagated) */
    path_t path;

    /* parser callbacks (const) */
    struct {
        sp_parser_cb_prop_t prop;
        sp_parser_cb_scope_t scope;
    } parser_cb;
} base_hndl_t;

/* Initialize base_hndl_t struct.
 */
static void init_base_hndl(base_hndl_t *p_b, int *p_finish,
    lastsc_t *p_lsc, int *p_sind, const char *path, const char *deftp,
    sp_parser_cb_prop_t parser_cb_prop, sp_parser_cb_scope_t parser_cb_scope)
{
    p_b->p_finish = p_finish;
    *p_finish = 0;

    memset(p_lsc, 0, sizeof(*p_lsc));
    p_b->p_lsc = p_lsc;

    p_b->p_sind = p_sind;
    *p_sind = -1;

    p_b->path.beg = path;
    p_b->path.end = (!path ? NULL : p_b->path.beg+strlen(path));
    if (p_b->path.beg && *p_b->path.beg==C_SEP_SCP) p_b->path.beg++;
    p_b->path.deftp = deftp;

    p_b->parser_cb.prop = parser_cb_prop;
    p_b->parser_cb.scope = parser_cb_scope;
}

/* sp_iterate() handle

   NOTE: This struct is copied during upward-downward process of following
   the destination scope path, so any changes made on it are not propagated to
   scopes on higher or the same scope level. Therefore, if such propagation is
   required a struct's field must be an immutable pointer to an object
   containing propagated information.
 */
typedef struct _iter_hndl_t
{
    base_hndl_t b;

    struct {
        /* argument passed untouched (const) */
        void *arg;

        /* user callbacks (const) */
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
   Number of chars read and constituting the index specification is written
   under 'p_ind_len' (0 if such spec. is absent). If the spec. is present but
   erroneous then SPEC_INV_PATH is returned.
 */
static sp_errc_t get_ind_from_name(
    const char *name, size_t nm_len, int *p_ind, size_t *p_ind_len)
{
    sp_errc_t ret=SPEC_SUCCESS;
    const char *ind_nm;
    size_t i;

    *p_ind=0;
    *p_ind_len=0;

    ind_nm = strchr_nesc(name, nm_len, C_SEP_SIND, 1);
    if (!ind_nm) goto finish; /* no index spec. */

     *p_ind_len = nm_len-(ind_nm-name);
     if (*p_ind_len <= 1) {
        /* no chars after the marker */
        ret=SPEC_INV_PATH;
        goto finish;
    }

    ind_nm++;

    if (*p_ind_len==2 && (*ind_nm==C_IND_ALL || *ind_nm==C_IND_LAST)) {
        *p_ind = (*ind_nm==C_IND_LAST ? SP_IND_LAST : SP_IND_ALL);
    } else {
        /* parse decimal number after the marker */
        for (i=1; i<*p_ind_len; i++, ind_nm++) {
            if (*ind_nm>='0' && *ind_nm<='9') {
                *p_ind = *p_ind*10 + (*ind_nm-'0');
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

   The function accepts a clone of the enclosing scope handle pointed by
   'ph_nst' which is then updated (actually its base part pointed by 'ph_nstb'),
   to represent the nesting scope. The nesting scope is followed if its
   characteristic meets scope criteria provided in the path).
 */
static sp_errc_t follow_scope_path(
    SP_FILE *in, base_hndl_t *ph_nstb, void *ph_nst, const sp_loc_t *p_ltype,
    const sp_loc_t *p_lname, const sp_loc_t *p_lbody, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;

    int ind, *p_sind=ph_nstb->p_sind;
    size_t typ_len=0, nm_len=0, ind_len;
    const char *type=NULL, *name=NULL;
    const path_t *p_path = &ph_nstb->path;
    const char *beg=p_path->beg, *end=p_path->end;
    const char *typ=strchr_nesc(beg, end-beg, C_SEP_TYP, 0);
    const char *scp=strchr_nesc(beg, end-beg, C_SEP_SCP, 0);

    if (scp) end=scp;
    if (typ>=end) typ=NULL;

    if (typ)
    {
        /* type and name specified */
        type = beg;
        typ_len = typ-beg;
        name = typ+1;
        nm_len = end-name;
    } else
    {
        /* scope with default type */
        type = p_path->deftp;
        typ_len = (type ? strlen(type) : 0);
        name = beg;
        nm_len = end-beg;
    }

    if (!nm_len) { ret=SPEC_INV_PATH; goto finish; }

    EXEC_RG(get_ind_from_name(name, nm_len, &ind, &ind_len));
    if (!(nm_len-=ind_len)) { ret=SPEC_INV_PATH; goto finish; }
    if (!ind_len) ind=SP_IND_ALL;  /* if not specified, SP_IND_ALL is assumed */

    CMPLOC_RG(in, SP_TKN_ID, p_ltype, type, typ_len, (p_path->deftp!=type));
    CMPLOC_RG(in, SP_TKN_ID, p_lname, name, nm_len, 1);

    /* scope with matching name found */

    if (ind!=SP_IND_ALL)
        /* the tracking index is updated only if the matched
           scope was provided with an index specification */
        *p_sind += 1;

    if (ind==SP_IND_LAST)
    {
        /* for last scope spec. simply track the scope */
        ph_nstb->p_lsc->present = 1;
        ph_nstb->p_lsc->beg = (!*end ? end : end+1);
        if (p_lbody) {
            ph_nstb->p_lsc->lbody = *p_lbody;
        } else {
            /* scope w/o a body */
            memset(&ph_nstb->p_lsc->lbody, 0, sizeof(sp_loc_t));
        }
        ph_nstb->p_lsc->ldef = *p_ldef;
    } else
    if (ind==SP_IND_ALL || *p_sind==ind)
    {
        /* follow the path for matching index */
        ph_nstb->path.beg = (!*end ? end : end+1);

        if (p_lbody)
        {
            int sind = -1;

            if (ind!=SP_IND_ALL)
                /* there is a need to start tracking in the followed scope */
                ph_nstb->p_sind = &sind;

            /* SPEC_SYNTAX will not occur during this parsing, since in case
               of such error it'd be detected earlier in the parsing process
               (before the scope callback call, which in turn calls
               follow_scope_path()).
             */
            EXEC_RG(sp_parse(in, p_lbody, ph_nstb->parser_cb.prop,
                ph_nstb->parser_cb.scope, ph_nst, NULL));
        }

        if (ind!=SP_IND_ALL && ph_nstb->path.beg>=ph_nstb->path.end)
        {
            /* the path finishes with a scope addressed by the explicit index
               specification; the scope has been already handled, therefore no
               further path following is needed */
            *ph_nstb->p_finish = 1;
        }
    } else
    if (*p_sind>ind)
    {
        /* there is no sense to further following the path, since
           the destination scope has been already passed by */
        *ph_nstb->p_finish = 1;
    }

finish:
    return ret;
}

/* Call follow_scope_path() for cloned nested scope handle 'hndl' and check
   the finish flag afterward. To be used inside scope parser callbacks only.
 */
#define CALL_FOLLOW_SCOPE_PATH(hndl) \
    ret = follow_scope_path( \
        in, &(hndl).b, &(hndl), p_ltype, p_lname, p_lbody, p_ldef); \
    if (ret==SPEC_SUCCESS && *(hndl).b.p_finish!=0) \
        ret = SPEC_CB_FINISH;

/* check user callback return code */
#define __CHK_USER_CB_RET() \
    if (ret==SPEC_CB_FINISH) \
        *p_ihndl->b.p_finish=1; \
    else if ((int)ret<0) \
        ret=SPEC_CB_RET_ERR;

/* sp_iterate() parser callback: property */
static sp_errc_t iter_cb_prop(void *arg, SP_FILE *in,
    const sp_loc_t *p_lname, const sp_loc_t *p_lval, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    iter_hndl_t *p_ihndl = (iter_hndl_t*)arg;

    /* ignore props until the destination scope */
    if ((p_ihndl->b.path.beg >= p_ihndl->b.path.end) && p_ihndl->cb.prop)
    {
        sp_tkn_info_t tkname, tkval;

        EXEC_RG(sp_parser_tkn_cpy(in, SP_TKN_ID,
            p_lname, p_ihndl->buf1.ptr, p_ihndl->buf1.sz, &tkname.len));
        EXEC_RG(sp_parser_tkn_cpy(in, SP_TKN_VAL,
            p_lval, p_ihndl->buf2.ptr, p_ihndl->buf2.sz, &tkval.len));

        tkname.loc = *p_lname;
        if (p_lval) tkval.loc = *p_lval;

        ret = p_ihndl->cb.prop(p_ihndl->cb.arg, in, p_ihndl->buf1.ptr,
            &tkname, p_ihndl->buf2.ptr, (p_lval ? &tkval : NULL), p_ldef);

        __CHK_USER_CB_RET();
    }
finish:
    return ret;
}

/* sp_iterate() parser callback: scope */
static sp_errc_t iter_cb_scope(void *arg, SP_FILE *in,
    const sp_loc_t *p_ltype, const sp_loc_t *p_lname, const sp_loc_t *p_lbody,
    const sp_loc_t *p_lbdyenc, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    iter_hndl_t *p_ihndl=(iter_hndl_t*)arg;

    if (p_ihndl->b.path.beg < p_ihndl->b.path.end) {
        iter_hndl_t ihndl = *p_ihndl;
        CALL_FOLLOW_SCOPE_PATH(ihndl);
    } else
    if (p_ihndl->cb.scope)
    {
        sp_tkn_info_t tktype, tkname;

        EXEC_RG(sp_parser_tkn_cpy(in, SP_TKN_ID,
            p_ltype, p_ihndl->buf1.ptr, p_ihndl->buf1.sz, &tktype.len));
        EXEC_RG(sp_parser_tkn_cpy(in, SP_TKN_ID,
            p_lname, p_ihndl->buf2.ptr, p_ihndl->buf2.sz, &tkname.len));

        if (p_ltype) tktype.loc = *p_ltype;
        tkname.loc = *p_lname;

        ret = p_ihndl->cb.scope(p_ihndl->cb.arg, in,
            p_ihndl->buf1.ptr, (p_ltype ? &tktype : NULL),
            p_ihndl->buf2.ptr, &tkname, p_lbody, p_lbdyenc, p_ldef);

        __CHK_USER_CB_RET();
    }
finish:
    return ret;
}

#undef __CHK_USER_CB_RET

/* Start parsing basing on the input 'in' with 'p_parsc' parsing scope. If
   the destination scope has not been reached due to the last scope addressing
   usage, start the re-parsing process until the destination will be finally
   reached.
 */
static sp_errc_t parse_with_lsc_handling(
    SP_FILE *in, const sp_loc_t *p_parsc, base_hndl_t *p_b, void *hndl)
{
    sp_errc_t ret=SPEC_SUCCESS;
    sp_loc_t lsc_bdy;
    const sp_loc_t *p_psc=p_parsc;

    for (;;)
    {
        EXEC_RG(sp_parse(
            in, p_psc, p_b->parser_cb.prop, p_b->parser_cb.scope, hndl, NULL));

        if (p_b->p_lsc->present && !*p_b->p_finish)
        {
            /* last scope spec. detected; need to re-parse the last scope */

            if (!p_b->p_lsc->lbody.first_column) {
                /* empty scope; skip further processing */
                break;
            }

            p_b->path.beg = p_b->p_lsc->beg;
            lsc_bdy = p_b->p_lsc->lbody;
            memset(p_b->p_lsc, 0, sizeof(*p_b->p_lsc));
            *p_b->p_sind = -1;

            p_psc = &lsc_bdy;
        } else break;
    }

finish:
    return ret;
}

#define __BASE_DEFS \
    /* processing finish flag */ \
    int f_finish; \
    /* last scope spec. */ \
    lastsc_t lsc; \
    /* split scope tracking index */ \
    int sind;

/* exported; see header for details */
sp_errc_t sp_iterate(SP_FILE *in, const sp_loc_t *p_parsc, const char *path,
    const char *deftp, sp_cb_prop_t cb_prop, sp_cb_scope_t cb_scope,
    void *arg, char *buf1, size_t b1len, char *buf2, size_t b2len)
{
    sp_errc_t ret=SPEC_SUCCESS;
    iter_hndl_t ihndl;

    __BASE_DEFS

    if (!in || (!cb_prop && !cb_scope)) {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    memset(&ihndl, 0, sizeof(ihndl));

    init_base_hndl(&ihndl.b,
        &f_finish, &lsc, &sind, path, deftp, iter_cb_prop, iter_cb_scope);

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

    EXEC_RG(parse_with_lsc_handling(in, p_parsc, &ihndl.b, &ihndl));

finish:
    return ret;
}

typedef struct _prop_dsc_t
{
    const char *name;
    int ind;
} prop_dsc_t;

typedef struct _scope_dsc_t
{
    const char *type;
    const char *name;
    int ind;
} scope_dsc_t;

/* sp_get_prop() handle

   NOTE: This struct is copied during upward-downward process of following
   the destination scope path.
 */
typedef struct _getprp_hndl_t
{
    base_hndl_t b;

    /* property desc. (const) */
    prop_dsc_t prop;

    /* matched props tracking index (shared) */
    int *p_eind;

    /* element position number tracking index (shared) */
    int *p_neind;

    /* property value buffer (const) */
    struct {
        char *ptr;
        size_t sz;
    } val;

    /* extra info will be written under this address (shared) */
    sp_prop_info_ex_t *p_info;
} getprp_hndl_t;

/* sp_get_prop() parser callback: property */
static sp_errc_t getprp_cb_prop(void *arg, SP_FILE *in,
    const sp_loc_t *p_lname, const sp_loc_t *p_lval, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    getprp_hndl_t *p_gphndl = (getprp_hndl_t*)arg;

    /* ignore props until the destination scope */
    if (p_gphndl->b.path.beg >= p_gphndl->b.path.end)
    {
        size_t nm_len = strlen(p_gphndl->prop.name);

        *p_gphndl->p_neind += 1;

        CMPLOC_RG(in, SP_TKN_ID, p_lname, p_gphndl->prop.name, nm_len, 0);
        EXEC_RG(sp_parser_tkn_cpy(in, SP_TKN_VAL, p_lval,
            p_gphndl->val.ptr, p_gphndl->val.sz, &p_gphndl->p_info->tkval.len));

        /* matching element found */
        *p_gphndl->p_eind += 1;

        if (p_gphndl->prop.ind==*p_gphndl->p_eind ||
            p_gphndl->prop.ind==SP_IND_LAST)
        {
            p_gphndl->p_info->tkname.len = nm_len;
            p_gphndl->p_info->tkname.loc = *p_lname;

            if (p_lval) {
                p_gphndl->p_info->val_pres = 1;
                p_gphndl->p_info->tkval.loc = *p_lval;
            } else {
                p_gphndl->p_info->val_pres = 0;
            }

            p_gphndl->p_info->ldef = *p_ldef;

            p_gphndl->p_info->ind = *p_gphndl->p_eind;
            p_gphndl->p_info->n_elem = *p_gphndl->p_neind-1;

            /* done if there is no need to track the last property */
            if (p_gphndl->prop.ind!=SP_IND_LAST) {
                ret = SPEC_CB_FINISH;
                *p_gphndl->b.p_finish = 1;
            }
        }
    }
finish:
    return ret;
}

/* sp_get_prop() parser callback: scope */
static sp_errc_t getprp_cb_scope(void *arg, SP_FILE *in,
    const sp_loc_t *p_ltype, const sp_loc_t *p_lname, const sp_loc_t *p_lbody,
    const sp_loc_t *p_lbdyenc, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    getprp_hndl_t *p_gphndl=(getprp_hndl_t*)arg;

    if (p_gphndl->b.path.beg < p_gphndl->b.path.end) {
        getprp_hndl_t gphndl = *p_gphndl;
        CALL_FOLLOW_SCOPE_PATH(gphndl);
    } else {
        *p_gphndl->p_neind += 1;
    }
    return ret;
}

#define __EIND_DEF \
    /* matched elements tracking index */ \
    int eind = -1;

#define __NEIND_DEF \
    /* element position number tracking index */ \
    int neind = 0;

/* exported; see header for details */
sp_errc_t sp_get_prop(SP_FILE *in, const sp_loc_t *p_parsc, const char *name,
    int ind, const char *path, const char *deftp, char *val, size_t len,
    sp_prop_info_ex_t *p_info)
{
    sp_errc_t ret=SPEC_SUCCESS;
    getprp_hndl_t gphndl;

    __BASE_DEFS
    __EIND_DEF
    __NEIND_DEF

    sp_prop_info_ex_t info;

    if (!in || !len || !name || (ind<0 && ind!=SP_IND_LAST))
    {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    memset(&gphndl, 0, sizeof(gphndl));
    memset(&info, 0, sizeof(info));

    init_base_hndl(&gphndl.b,
        &f_finish, &lsc, &sind, path, deftp, getprp_cb_prop, getprp_cb_scope);

    gphndl.prop.name = name;
    gphndl.prop.ind = ind;
    gphndl.p_eind = &eind;
    gphndl.p_neind = &neind;

    gphndl.val.ptr = val;
    gphndl.val.sz = len-1;
    gphndl.val.ptr[gphndl.val.sz] = 0;

    gphndl.p_info = &info;

    EXEC_RG(parse_with_lsc_handling(in, p_parsc, &gphndl.b, &gphndl));

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
sp_errc_t sp_get_prop_int(SP_FILE *in, const sp_loc_t *p_parsc,
    const char *name, int ind, const char *path, const char *deftp, long *p_val,
    sp_prop_info_ex_t *p_info)
{
    sp_errc_t ret=SPEC_SUCCESS;
    sp_prop_info_ex_t info;
    char val[80], *end;
    long v=0L;

    EXEC_RG(sp_get_prop(
        in, p_parsc, name, ind, path, deftp, val, sizeof(val), &info));
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
sp_errc_t sp_get_prop_float(SP_FILE *in, const sp_loc_t *p_parsc,
    const char *name, int ind, const char *path, const char *deftp,
    double *p_val, sp_prop_info_ex_t *p_info)
{
    sp_errc_t ret=SPEC_SUCCESS;
    sp_prop_info_ex_t info;
    char val[80], *end;
    double v=0.0;

    EXEC_RG(sp_get_prop(
        in, p_parsc, name, ind, path, deftp, val, sizeof(val), &info));
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
    SP_FILE *in, const sp_loc_t *p_parsc, const char *name, int ind,
    const char *path, const char *deftp, const sp_enumval_t *p_evals,
    int igncase, char *buf, size_t blen, int *p_val, sp_prop_info_ex_t *p_info)
{
    sp_errc_t ret=SPEC_SUCCESS;
    int v=0;

    sp_prop_info_ex_t info;

    if (!p_evals) { ret=SPEC_INV_ARG; goto finish; }

    memset(&info, 0, sizeof(info));

    EXEC_RG(sp_get_prop(in, p_parsc, name, ind, path, deftp, buf, blen, &info));

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

/* sp_get_scope_info() handle

   NOTE: This struct is copied during upward-downward process of following
   the destination scope path.
 */
typedef struct _getscp_hndl_t
{
    base_hndl_t b;

    /* scope desc. (const) */
    scope_dsc_t scp;

    /* matched scope tracking index (shared) */
    int *p_eind;

    /* element position number tracking index (shared) */
    int *p_neind;

    /* extra info will be written under this address (shared) */
    sp_scope_info_ex_t *p_info;
} getscp_hndl_t;

/* sp_get_scope_info() parser callback: property */
static sp_errc_t getscp_cb_prop(void *arg, SP_FILE *in,
    const sp_loc_t *p_lname, const sp_loc_t *p_lval, const sp_loc_t *p_ldef)
{
    getscp_hndl_t *p_gshndl=(getscp_hndl_t*)arg;

    if (p_gshndl->b.path.beg >= p_gshndl->b.path.end)
        *p_gshndl->p_neind += 1;

    return SPEC_SUCCESS;
}

/* sp_get_scope_info() parser callback: scope */
static sp_errc_t getscp_cb_scope(void *arg, SP_FILE *in,
    const sp_loc_t *p_ltype, const sp_loc_t *p_lname, const sp_loc_t *p_lbody,
    const sp_loc_t *p_lbdyenc, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    getscp_hndl_t *p_gshndl=(getscp_hndl_t*)arg;

    if (p_gshndl->b.path.beg < p_gshndl->b.path.end) {
        getscp_hndl_t gshndl = *p_gshndl;
        CALL_FOLLOW_SCOPE_PATH(gshndl);
    } else
    {
        size_t typ_len = (p_gshndl->scp.type ? strlen(p_gshndl->scp.type) : 0);
        size_t nm_len = strlen(p_gshndl->scp.name);

        *p_gshndl->p_neind += 1;

        CMPLOC_RG(in, SP_TKN_ID, p_ltype, p_gshndl->scp.type, typ_len, 0);
        CMPLOC_RG(in, SP_TKN_ID, p_lname, p_gshndl->scp.name, nm_len, 0);

        /* matching element found */
        *p_gshndl->p_eind += 1;

        if (p_gshndl->scp.ind==*p_gshndl->p_eind ||
            p_gshndl->scp.ind==SP_IND_LAST)
        {
            if (p_ltype) {
                p_gshndl->p_info->type_pres = 1;
                p_gshndl->p_info->tktype.len = typ_len;
                p_gshndl->p_info->tktype.loc = *p_ltype;
            } else {
                p_gshndl->p_info->type_pres = 0;
            }

            p_gshndl->p_info->tkname.len = nm_len;
            p_gshndl->p_info->tkname.loc = *p_lname;

            if (p_lbody) {
                p_gshndl->p_info->body_pres = 1;
                p_gshndl->p_info->lbody = *p_lbody;
            } else {
                p_gshndl->p_info->body_pres = 0;
            }

            p_gshndl->p_info->lbdyenc = *p_lbdyenc;
            p_gshndl->p_info->ldef = *p_ldef;

            p_gshndl->p_info->ind = *p_gshndl->p_eind;
            p_gshndl->p_info->n_elem = *p_gshndl->p_neind-1;

            /* done if there is no need to track the last scope */
            if (p_gshndl->scp.ind!=SP_IND_LAST) {
                ret = SPEC_CB_FINISH;
                *p_gshndl->b.p_finish = 1;
            }
        }
    }
finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_get_scope_info(
    SP_FILE *in, const sp_loc_t *p_parsc, const char *type, const char *name,
    int ind, const char *path, const char *deftp, sp_scope_info_ex_t *p_info)
{
    sp_errc_t ret=SPEC_SUCCESS;
    getscp_hndl_t gshndl;

    __BASE_DEFS
    __EIND_DEF
    __NEIND_DEF

    if (!in || !name || (ind<0 && ind!=SP_IND_LAST) || !p_info)
    {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    memset(&gshndl, 0, sizeof(gshndl));
    memset(p_info, 0, sizeof(*p_info));

    init_base_hndl(&gshndl.b,
        &f_finish, &lsc, &sind, path, deftp, getscp_cb_prop, getscp_cb_scope);

    gshndl.scp.type = type;
    gshndl.scp.name = name;
    gshndl.scp.ind = ind;
    gshndl.p_eind = &eind;
    gshndl.p_neind = &neind;
    gshndl.p_info = p_info;

    EXEC_RG(parse_with_lsc_handling(in, p_parsc, &gshndl.b, &gshndl));

    if (!p_info->ldef.first_column) ret=SPEC_NOTFOUND;

finish:
    return ret;

}

/* Base struct for update-handles.
 */
typedef struct _base_updt_hndl_t
{
    /* input/output streams */
    SP_FILE *in;
    SP_FILE *out;

    /* offset staring the not processed range of the input */
    long in_off;

    /* parsing scope */
    const sp_loc_t *p_parsc;

    /* passed flags (const) */
    unsigned long flags;

    /* type of EOL detected */
    sp_eol_t eol_typ;
} base_updt_hndl_t;

/* Initialize base_updt_hndl_t struct.
 */
static sp_errc_t init_base_updt_hndl(base_updt_hndl_t *p_bu,
    SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc, unsigned long flags)
{
    sp_errc_t ret=SPEC_SUCCESS;

    p_bu->in = in;
    p_bu->out = out;
    p_bu->in_off = (p_parsc ? p_parsc->beg : 0);
    p_bu->p_parsc = p_parsc;
    p_bu->flags = flags;

    p_bu->eol_typ = SP_F_GET_USEEOL(p_bu->flags);
    if (p_bu->eol_typ==(sp_eol_t)-1)
        ret = sp_util_detect_eol(in, &p_bu->eol_typ);

    return ret;
}

/* Copies input bytes (from the offset staring the not processed range) to
   the output up to 'end' offset (exclusive). In case of success (and there
   is something to copy) the input offset is set at 'end'.
 */
static sp_errc_t __cpy_to_out(base_updt_hndl_t *p_bu, long end)
{
    sp_errc_t ret;
    long n=0;

    ret = sp_util_cpy_to_out(p_bu->in, p_bu->out, p_bu->in_off, end, &n);
    if (ret==SPEC_SUCCESS && n>0) p_bu->in_off = p_bu->in_off+n;
    return ret;
}

/* Skip input spaces starting from the offset 'off', up to the following EOL
   (inclusive) OR a first non-space character. The function writes 'p_skip_n'
   and 'p_eol_n' with a number of skipped bytes and EOL size (0, 1 or 2)
   respectively.
 */
static sp_errc_t skip_sp_to_eol(
    const base_updt_hndl_t *p_bu, long off, long *p_skip_n, int *p_eol_n)
{
    sp_errc_t ret=SPEC_SUCCESS;
    long org_off=off;
    int c, eol_n=0;

    CHK_FSEEK(sp_fseek(p_bu->in, off, SEEK_SET));

    for (; !eol_n && isspace(c=sp_fgetc(p_bu->in)) && c!='\v' && c!='\f'; off++)
    {
        if (c!='\r' && c!='\n') continue;

        eol_n++;
        if (c=='\r') {
            if ((c=sp_fgetc(p_bu->in))=='\n') {
                off++;
                eol_n++;
            }
        }
    }

    if (p_skip_n) *p_skip_n = off-org_off;
    if (p_eol_n) *p_eol_n = eol_n;

finish:
    return ret;
}

/* Put EOL on the output.
 */
static sp_errc_t put_eol(const base_updt_hndl_t *p_bu)
{
    sp_errc_t ret=SPEC_SUCCESS;
    sp_eol_t eol_typ = (p_bu->eol_typ!=EOL_PLAT ? p_bu->eol_typ :
#if defined(_WIN32) || defined(_WIN64)
        EOL_CRLF
#else
        EOL_LF
#endif
        );

    switch (eol_typ) {
        default:
        case EOL_LF:
            CHK_FERR(sp_fputc('\n', p_bu->out));
            break;
        case EOL_CR:
            CHK_FERR(sp_fputc('\r', p_bu->out));
            break;
        case EOL_CRLF:
            CHK_FERR(sp_fputs("\r\n", p_bu->out));
            break;
    }
finish:
    return ret;
}

#define IND_F_TRIMSP    0x01U
#define IND_F_CUTGAP    0x02U
#define IND_F_SCBDY     0x04U
#define IND_F_CHKEOL    0x08U
#define IND_F_EXTEOL    0x10U

/* Put indent chars to the output.

   'p_ldef' points to a def-loc used to retrieve proper indent.
   IND_F_SCBDY - the indentation should consider additional indent insider
   a scope (as its body).
 */
static sp_errc_t put_ind(
    const base_updt_hndl_t *p_bu, const sp_loc_t *p_ldef, unsigned flgs)
{
    sp_errc_t ret=SPEC_SUCCESS;
    int c, n=p_ldef->first_column-1;

    if (n>0 || (!n && (flgs & IND_F_SCBDY))) {
        CHK_FSEEK(sp_fseek(p_bu->in, p_ldef->beg-n, SEEK_SET));
    }

    for (; n>0 && isspace(c=sp_fgetc(p_bu->in)); n--) {
        CHK_FERR(sp_fputc(c, p_bu->out));
    }

    if (flgs & IND_F_SCBDY) {
        n = (int)SP_F_GET_SPIND(p_bu->flags);
        c = (!n ? (n++, '\t') : ' ');
        for (; n; n--) { CHK_FERR(sp_fputc(c, p_bu->out)); }
    }

finish:
    return ret;
}

/* Put EOL and indent chars (according to the passed def-loc) to the output.
   The function assumes the EOL will be placed at the input offset starting not
   processed range (the in-off).

   IND_F_CUTGAP - the function skip spaces/tabs following the in-off.
   IND_F_TRIMSP - similar to IND_F_CUTGAP but the spaces/tabs are skipped only
     if followed by EOL. This effectively trims trailing spaces at the line end.
   IND_F_CHKEOL - don't do anything if the in-off points to the end of line
     (possibly followed by spaces). An extra EOL is written nonetheless if
     IND_F_EXTEOL is also specified and the configuration requires this.
   IND_F_EXTEOL - put an extra EOL if required by user flags.
 */
static sp_errc_t put_eol_ind(
    base_updt_hndl_t *p_bu, const sp_loc_t *p_ldef, unsigned flgs)
{
    sp_errc_t ret=SPEC_SUCCESS;
    int eol_n=0;

    if (flgs & (IND_F_CUTGAP|IND_F_TRIMSP|IND_F_CHKEOL))
    {
        long skip_n;
        EXEC_RG(skip_sp_to_eol(p_bu, p_bu->in_off, &skip_n, &eol_n));

        if (flgs & IND_F_CUTGAP) {
            p_bu->in_off += skip_n-eol_n;
        } else
        if ((flgs & IND_F_TRIMSP) && eol_n) {
            p_bu->in_off += skip_n-eol_n;
        }
    }

    if ((flgs & IND_F_EXTEOL) && (p_bu->flags & SP_F_EXTEOL)) {
        EXEC_RG(put_eol(p_bu));
    }

    if (!(flgs & IND_F_CHKEOL) || !eol_n) {
        EXEC_RG(put_eol(p_bu));
        if (p_ldef) { EXEC_RG(put_ind(p_bu, p_ldef, flgs)); }
    }

finish:
    return ret;
}

typedef struct _addh_frst_sc_t
{
    sp_loc_t lname;
    sp_loc_t lbdyenc;
    sp_loc_t ldef;
} addh_frst_sc_t;

/* add_elem() handle

   NOTE: This struct is copied during upward-downward process of following
   the destination scope path.
 */
typedef struct _add_hndl_t
{
    base_hndl_t b;
    /* base class for the update part (shared) */
    base_updt_hndl_t *p_bu;

    /* element position number (const) */
    int n_elem;
    /* element position number tracking index (shared) */
    int *p_neind;

    /* first, non-global scope matching
       the path; zeroed if not found (shared) */
    addh_frst_sc_t *p_frst_sc;

    /* definition's location of an element associated with
       the requested position; zeroed if not found (shared) */
    sp_loc_t *p_ldef_elem;
} add_hndl_t;

/* Element addition support function.
 */
static sp_errc_t track_add_elem(add_hndl_t *p_ahndl, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;

    /* count element in the matched scope */
    *p_ahndl->p_neind += 1;

    if (p_ahndl->n_elem==*p_ahndl->p_neind || p_ahndl->n_elem==SP_ELM_LAST)
    {
        /* save element's ldef associated with the requested position */
        *p_ahndl->p_ldef_elem = *p_ldef;

        /* done if there is no need to track the last position */
        if (p_ahndl->n_elem!=SP_ELM_LAST) {
            ret = SPEC_CB_FINISH;
            *p_ahndl->b.p_finish = 1;
        }
    }
    return ret;
}

/* add_elem() parser callback: property */
static sp_errc_t add_cb_prop(void *arg, SP_FILE *in,
    const sp_loc_t *p_lname, const sp_loc_t *p_lval, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    add_hndl_t *p_ahndl=(add_hndl_t*)arg;

    if (p_ahndl->b.path.beg >= p_ahndl->b.path.end)
        ret = track_add_elem(p_ahndl, p_ldef);

    return ret;
}

/* add_elem() parser callback: scope */
static sp_errc_t add_cb_scope(void *arg, SP_FILE *in,
    const sp_loc_t *p_ltype, const sp_loc_t *p_lname, const sp_loc_t *p_lbody,
    const sp_loc_t *p_lbdyenc, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    add_hndl_t *p_ahndl=(add_hndl_t*)arg;

    if (p_ahndl->b.path.beg < p_ahndl->b.path.end)
    {
        add_hndl_t ahndl = *p_ahndl;
        CALL_FOLLOW_SCOPE_PATH(ahndl);

        if (ahndl.b.p_lsc->present &&
            ahndl.b.p_lsc->beg >= ahndl.b.path.end &&
            ahndl.b.p_lsc->ldef.beg==p_ldef->beg &&
            ahndl.b.p_lsc->ldef.end==p_ldef->end)
        {
            /* track the last scope and write under 'p_frst_sc'
               if the scope finishes the path */
            p_ahndl->p_frst_sc->lname = *p_lname;
            p_ahndl->p_frst_sc->lbdyenc = *p_lbdyenc;
            p_ahndl->p_frst_sc->ldef = *p_ldef;
        } else
        if (ahndl.b.path.beg >= ahndl.b.path.end &&
            !p_ahndl->p_frst_sc->ldef.first_column)
        {
            /* mark first matching, non-global scope */
            p_ahndl->p_frst_sc->lname = *p_lname;
            p_ahndl->p_frst_sc->lbdyenc = *p_lbdyenc;
            p_ahndl->p_frst_sc->ldef = *p_ldef;
        }
    } else
        ret = track_add_elem(p_ahndl, p_ldef);

    return ret;
}

/* Element addition support function.
 */
static sp_errc_t put_elem(base_updt_hndl_t *p_bu, const char *prop_nm,
    const char *prop_val, const char *sc_typ, const char *sc_nm,
    const sp_loc_t *p_ind_ldef, unsigned ind_flgs, int *p_traileol)
{
    sp_errc_t ret=SPEC_SUCCESS;
    *p_traileol = 0;

    if (prop_nm)
    {
        /* element is a property */
        EXEC_RG(sp_parser_tokenize_str(p_bu->out, SP_TKN_ID, prop_nm, 0));
        if (prop_val && *prop_val)
        {
#ifdef CONFIG_CUT_VAL_LEADING_SPACES
            if (!(p_bu->flags & SP_F_NVSRSP)) {
                CHK_FERR(sp_fputs(" = ", p_bu->out));
            } else {
#endif
                CHK_FERR(sp_fputc('=', p_bu->out));
#ifdef CONFIG_CUT_VAL_LEADING_SPACES
            }
#endif
            EXEC_RG(sp_parser_tokenize_str(p_bu->out, SP_TKN_VAL, prop_val, 0));
#ifndef CONFIG_NO_SEMICOL_ENDS_VAL
            CHK_FERR(sp_fputc(';', p_bu->out));
#else
            /* added value need to be finished by EOL */
            *p_traileol = 1;
#endif
        } else {
            CHK_FERR(sp_fputc(';', p_bu->out));
        }

    } else
    {
        /* element is a scope */
        if (sc_typ && *sc_typ) {
            EXEC_RG(sp_parser_tokenize_str(p_bu->out, SP_TKN_ID, sc_typ, 0));
            CHK_FERR(sp_fputc(' ', p_bu->out));
        }
        EXEC_RG(sp_parser_tokenize_str(p_bu->out, SP_TKN_ID, sc_nm, 0));

        if (p_bu->flags & SP_F_SPLBRA) {
            EXEC_RG(put_eol_ind(p_bu, p_ind_ldef, ind_flgs));
            CHK_FERR(sp_fputc('{', p_bu->out));
        } else {
            if (sc_typ && *sc_typ && (p_bu->flags & SP_F_EMPCPT)) {
                CHK_FERR(sp_fputc(';', p_bu->out));
                goto finish;
            } else {
                CHK_FERR(sp_fputs(" {", p_bu->out));
            }
        }

        if (!(p_bu->flags & SP_F_EMPCPT)) {
            EXEC_RG(put_eol_ind(p_bu, p_ind_ldef, ind_flgs));
        }
        CHK_FERR(sp_fputc('}', p_bu->out));
    }

finish:
    return ret;
}

#define __BASEUPD_DEF \
    /* base class for the update part */ \
    base_updt_hndl_t bu;

/* Add prop/scope element.
 */
static sp_errc_t add_elem(SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc,
    const char *prop_nm, const char *prop_val, const char *sc_typ,
    const char *sc_nm, int n_elem, const char *path, const char *deftp,
    unsigned long flags)
{
    sp_errc_t ret=SPEC_SUCCESS;
    add_hndl_t ahndl;

    long lstcpy_n;
    int chk_end_eol=0, traileol;

    __BASE_DEFS
    __BASEUPD_DEF
    __NEIND_DEF

    /* first scope matching the path */
    addh_frst_sc_t frst_sc;
    /* ldef of an element associated with the requested position */
    sp_loc_t ldef_elem;

    if (!in || !out ||
        (!prop_nm && !sc_nm) ||
        (n_elem<0 && n_elem!=SP_ELM_LAST))
    {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    memset(&ahndl, 0, sizeof(ahndl));
    memset(&frst_sc, 0, sizeof(frst_sc));
    memset(&ldef_elem, 0, sizeof(ldef_elem));

    init_base_hndl(&ahndl.b,
        &f_finish, &lsc, &sind, path, deftp, add_cb_prop, add_cb_scope);

    EXEC_RG(init_base_updt_hndl(&bu, in, out, p_parsc, flags));
    ahndl.p_bu = &bu;

    ahndl.n_elem = n_elem;
    ahndl.p_neind = &neind;
    ahndl.p_frst_sc = &frst_sc;
    ahndl.p_ldef_elem = &ldef_elem;

    EXEC_RG(parse_with_lsc_handling(in, p_parsc, &ahndl.b, &ahndl));

    if ((ahndl.b.path.beg < ahndl.b.path.end) && !frst_sc.ldef.first_column)
    {
        /* destination scope was not found in the specified path */
        ret=SPEC_NOTFOUND;
        goto finish;
    }

    if ((n_elem && n_elem!=SP_ELM_LAST) && !ldef_elem.first_column)
    {
        /* requested position not found */
        ret=SPEC_NOTFOUND;
        goto finish;
    }

    if (n_elem && (n_elem!=SP_ELM_LAST || ldef_elem.first_column))
    {
        /* add after n-th elem
         */
        EXEC_RG(__cpy_to_out(&bu, ldef_elem.end+1));

        if (flags & SP_F_EOLBFR)
        {
            long skip_n, skip_n2;
            int eol_n, eol_n2;

            EXEC_RG(put_eol(&bu));
            EXEC_RG(put_eol_ind(&bu, &ldef_elem, 0));

            EXEC_RG(skip_sp_to_eol(&bu, bu.in_off, &skip_n, &eol_n));
            if (eol_n) {
                EXEC_RG(
                    skip_sp_to_eol(&bu, bu.in_off+skip_n, &skip_n2, &eol_n2));
                bu.in_off += (!eol_n2 ? skip_n-eol_n : skip_n+skip_n2-eol_n2);
            }
        } else {
            EXEC_RG(put_eol_ind(&bu, &ldef_elem, IND_F_TRIMSP));
        }

        EXEC_RG(put_elem(
            &bu, prop_nm, prop_val, sc_typ, sc_nm, &ldef_elem, 0, &traileol));

        if (traileol || (flags & SP_F_EXTEOL)) {
            EXEC_RG(put_eol_ind(
                &bu, &ldef_elem, IND_F_CUTGAP|IND_F_CHKEOL|IND_F_EXTEOL));
        } else {
            chk_end_eol=1;
        }
    } else {
        if (frst_sc.ldef.first_column)
        {
            /* add at the scope beginning
             */
            long bdyenc_sz = sp_loc_len(&frst_sc.lbdyenc);

            if (bdyenc_sz==1)
            {
                /* body as ; */
                EXEC_RG(__cpy_to_out(&bu, frst_sc.lbdyenc.beg));
                if (frst_sc.lbdyenc.beg-frst_sc.lname.end <= 1) {
                    /* put extra space before the opening bracket */
                    CHK_FERR(sp_fputc(' ', out));
                }
                CHK_FERR(sp_fputc('{', out));

                bu.in_off = frst_sc.ldef.end+1;
                EXEC_RG(put_eol_ind(
                    &bu, &frst_sc.ldef, IND_F_TRIMSP|IND_F_SCBDY));

                EXEC_RG(put_elem(&bu, prop_nm, prop_val,
                    sc_typ, sc_nm, &frst_sc.ldef, IND_F_SCBDY, &traileol));

                EXEC_RG(put_eol_ind(&bu, &frst_sc.ldef, IND_F_EXTEOL));
                CHK_FERR(sp_fputc('}', out));
            } else
            if (bdyenc_sz>=2)
            {
                /* body as {} or { ... } */
                EXEC_RG(__cpy_to_out(&bu, frst_sc.lbdyenc.beg+1));
                EXEC_RG(put_eol_ind(
                    &bu, &frst_sc.ldef, IND_F_TRIMSP|IND_F_SCBDY));

                EXEC_RG(put_elem(&bu, prop_nm, prop_val,
                    sc_typ, sc_nm, &frst_sc.ldef, IND_F_SCBDY, &traileol));

                if (bdyenc_sz==2) {
                    EXEC_RG(put_eol_ind(&bu, &frst_sc.ldef, IND_F_EXTEOL));
                } else
                if (traileol || (flags & SP_F_EXTEOL)) {
                    EXEC_RG(put_eol_ind(&bu,
                        &frst_sc.ldef, IND_F_CUTGAP|IND_F_CHKEOL|IND_F_EXTEOL));
                }
            }
        } else
        {
            /* add at the stream beginning
             */
            const sp_loc_t *p_ind_ldef = (bu.p_parsc ? bu.p_parsc : NULL);

            EXEC_RG(put_elem(&bu, prop_nm, prop_val,
                sc_typ, sc_nm, p_ind_ldef, 0, &traileol));

            EXEC_RG(put_eol_ind(&bu, p_ind_ldef, IND_F_EXTEOL));
        }
    }

    /* copy untouched last part of the input */
    lstcpy_n = bu.in_off;
    EXEC_RG(__cpy_to_out(&bu, (p_parsc ? p_parsc->end+1 : EOF)));
    lstcpy_n = bu.in_off-lstcpy_n;

    if (chk_end_eol && !lstcpy_n && !p_parsc && !(flags & SP_F_NLSTEOL))
    {
        /* ensure EOL if updated elem ends the input */
        EXEC_RG(put_eol(&bu));
    }

finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_add_prop(SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc,
    const char *name, const char *val, int n_elem, const char *path,
    const char *deftp, unsigned long flags)
{
    return add_elem(
        in, out, p_parsc, name, val, NULL, NULL, n_elem, path, deftp, flags);
}

/* exported; see header for details */
sp_errc_t sp_add_scope(SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc,
    const char *type, const char *name, int n_elem, const char *path,
    const char *deftp, unsigned long flags)
{
    return add_elem(
        in, out, p_parsc, NULL, NULL, type, name, n_elem, path, deftp, flags);
}

typedef enum _fndstat_t
{
    ELM_NOT_FND=0,  /* element and its destination scope was not found */
    ELM_DEST_FND,   /* destination scope found but not the element in it */
    ELM_FND         /* element was found */
} fndstat_t;

/* rm_elem() handle

   NOTE: This struct is copied during upward-downward process of following
   the destination scope path.
 */
typedef struct _rm_hndl_t
{
    base_hndl_t b;
    /* base class for the update part (shared) */
    base_updt_hndl_t *p_bu;

    /* element desc. (const) */
    struct {
        int is_scp;
        union {
            prop_dsc_t prop;
            scope_dsc_t scp;
        };
    } e;

    /* matched elements tracking index (shared) */
    int *p_eind;

    /* found status (shared) */
    fndstat_t *p_fndstat;

    /* last element def. (shared) */
    sp_loc_t *p_lst_ldef;
} rm_hndl_t;

/* Element removal support function.

   Copies input bytes (from the offset staring the not processed range) to
   the ldef pointed by 'p_ldef' but excluding it. In case of success the input
   offset is set behind ldef.
 */
static sp_errc_t cpy_rm_ldef(base_updt_hndl_t *p_bu, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    long skip_n, beg=p_ldef->beg, end=p_ldef->end+1;
    int eol_n;

    /* recognize ldef trailing spaces */
    EXEC_RG(skip_sp_to_eol(p_bu, end, &skip_n, &eol_n));
    end += skip_n-eol_n;

    /* if ldef occupies whole lines, delete them,
       otherwise delete ldef with trailing spaces */
    if (eol_n)
    {
        int n=p_ldef->first_column-1;

        if (n>0) {
            CHK_FSEEK(sp_fseek(p_bu->in, p_ldef->beg-n, SEEK_SET));
            for (; n>0 && isspace(sp_fgetc(p_bu->in)); n--);
        }

        if (!n) {
            beg -= p_ldef->first_column-1;
            end += eol_n;

            if (p_bu->flags & SP_F_EXTEOL) {
                EXEC_RG(skip_sp_to_eol(p_bu, end, &skip_n, &eol_n));
                if (eol_n) end += skip_n;
            }
        } else {
            int lns=n, eol2_n;

            /* cut spaces before ldef */
            for (n--; n>0; n--) if (!isspace(sp_fgetc(p_bu->in))) lns=n;
            beg -= lns-1;

            if (p_bu->flags & SP_F_EXTEOL) {
                EXEC_RG(skip_sp_to_eol(p_bu, end+eol_n, &skip_n, &eol2_n));
                if (eol2_n) end += eol_n+skip_n-eol2_n;
            }
        }
    }

    EXEC_RG(__cpy_to_out(p_bu, beg));
    p_bu->in_off = end;

finish:
    return ret;
}

/* Element removal support function.
 */
static sp_errc_t rm_ldef(rm_hndl_t *p_rhndl, int ind, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;

    *p_rhndl->p_fndstat = ELM_FND;
    *p_rhndl->p_eind += 1;

    if (ind==*p_rhndl->p_eind)
    {
        /* specific element found; done */
        EXEC_RG(cpy_rm_ldef(p_rhndl->p_bu, p_ldef));
        ret = SPEC_CB_FINISH;
        *p_rhndl->b.p_finish = 1;
    } else
    if (ind==SP_IND_ALL) {
        EXEC_RG(cpy_rm_ldef(p_rhndl->p_bu, p_ldef));
    } else
    if (ind==SP_IND_LAST)
        *p_rhndl->p_lst_ldef = *p_ldef;

finish:
    return ret;
}

/* rm_elem() parser callback: property */
static sp_errc_t rm_cb_prop(void *arg, SP_FILE *in,
    const sp_loc_t *p_lname, const sp_loc_t *p_lval, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    rm_hndl_t *p_rhndl = (rm_hndl_t*)arg;

    /* ignore props until the destination scope */
    if ((p_rhndl->b.path.beg >= p_rhndl->b.path.end) && !p_rhndl->e.is_scp)
    {
        size_t nm_len = strlen(p_rhndl->e.prop.name);

        if (*p_rhndl->p_fndstat==ELM_NOT_FND) *p_rhndl->p_fndstat=ELM_DEST_FND;
        CMPLOC_RG(in, SP_TKN_ID, p_lname, p_rhndl->e.prop.name, nm_len, 0);

        /* matching element found */
        ret = rm_ldef(p_rhndl, p_rhndl->e.prop.ind, p_ldef);
    }
finish:
    return ret;
}

/* rm_elem() parser callback: scope */
static sp_errc_t rm_cb_scope(void *arg, SP_FILE *in,
    const sp_loc_t *p_ltype, const sp_loc_t *p_lname, const sp_loc_t *p_lbody,
    const sp_loc_t *p_lbdyenc, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    rm_hndl_t *p_rhndl = (rm_hndl_t*)arg;

    if (p_rhndl->b.path.beg < p_rhndl->b.path.end) {
        rm_hndl_t rhndl = *p_rhndl;
        CALL_FOLLOW_SCOPE_PATH(rhndl);
    } else
    if (p_rhndl->e.is_scp)
    {
        size_t typ_len = (p_rhndl->e.scp.type ? strlen(p_rhndl->e.scp.type) : 0);
        size_t nm_len = strlen(p_rhndl->e.scp.name);

        if (*p_rhndl->p_fndstat==ELM_NOT_FND) *p_rhndl->p_fndstat=ELM_DEST_FND;
        CMPLOC_RG(in, SP_TKN_ID, p_ltype, p_rhndl->e.scp.type, typ_len, 0);
        CMPLOC_RG(in, SP_TKN_ID, p_lname, p_rhndl->e.scp.name, nm_len, 0);

        /* matching element found */
        ret = rm_ldef(p_rhndl, p_rhndl->e.scp.ind, p_ldef);
    }
finish:
    return ret;
}

#define __RM_MOD_DEFS \
    __EIND_DEF \
    /* found status */ \
    fndstat_t fndstat = ELM_NOT_FND;

/* Remove prop/scope element.
 */
static sp_errc_t rm_elem(SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc,
    const char *prop_nm, const char *sc_typ, const char *sc_nm, int ind,
    const char *path, const char *deftp, unsigned long flags)
{
    sp_errc_t ret=SPEC_SUCCESS;
    rm_hndl_t rhndl;

    __BASE_DEFS
    __BASEUPD_DEF
    __RM_MOD_DEFS

    /* last element def. */
    sp_loc_t lst_ldef;

    if (!in || !out ||
        (!prop_nm && !sc_nm) ||
        (ind<0 && ind!=SP_IND_LAST && ind!=SP_IND_ALL))
    {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    memset(&rhndl, 0, sizeof(rhndl));
    memset(&lst_ldef, 0, sizeof(lst_ldef));

    init_base_hndl(&rhndl.b,
        &f_finish, &lsc, &sind, path, deftp, rm_cb_prop, rm_cb_scope);

    EXEC_RG(init_base_updt_hndl(&bu, in, out, p_parsc, flags));
    rhndl.p_bu = &bu;

    if (prop_nm) {
        rhndl.e.is_scp = 0;
        rhndl.e.prop.name = prop_nm;
        rhndl.e.prop.ind = ind;
    } else {
        rhndl.e.is_scp = 1;
        rhndl.e.scp.type = sc_typ;
        rhndl.e.scp.name = sc_nm;
        rhndl.e.scp.ind = ind;
    }

    rhndl.p_eind = &eind;
    rhndl.p_fndstat = &fndstat;
    rhndl.p_lst_ldef = &lst_ldef;

    EXEC_RG(parse_with_lsc_handling(in, p_parsc, &rhndl.b, &rhndl));

    /* process the last element if required */
    if (lst_ldef.first_column) {
        EXEC_RG(cpy_rm_ldef(&bu, &lst_ldef));
    }

    /* copy untouched last part of the input */
    EXEC_RG(__cpy_to_out(&bu, (p_parsc ? p_parsc->end+1 : EOF)));

    if (fndstat==ELM_NOT_FND) {
        /* destination scope was not found (nonetheless the output is copied) */
        ret=SPEC_NOTFOUND;
    }

finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_rm_prop(SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc,
    const char *name, int ind, const char *path, const char *deftp,
    unsigned long flags)
{
    return rm_elem(in, out, p_parsc, name, NULL, NULL, ind, path, deftp, flags);
}

/* exported; see header for details */
sp_errc_t sp_rm_scope(SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc,
    const char *type, const char *name, int ind, const char *path,
    const char *deftp, unsigned long flags)
{
    return rm_elem(in, out, p_parsc, NULL, type, name, ind, path, deftp, flags);
}

typedef union _mod_lst_t
{
    struct {
        sp_loc_t lname;
        sp_loc_t lval;      /* zeroed for prop w/o a value */
        sp_loc_t ldef;
    } prop;

    struct {
        sp_loc_t ltype;     /* zeroed for scope w/o a type */
        sp_loc_t lname;
        sp_loc_t lbdyenc;
    } scp;
} mod_lst_t;

/* Element modification handle

   NOTE: This struct is copied during upward-downward process of following
   the destination scope path.
 */
typedef struct _mod_hndl_t
{
    base_hndl_t b;
    /* base class for the update part (shared) */
    base_updt_hndl_t *p_bu;

    /* element desc. (const) */
    struct
    {
        int is_scp;
        union {
            prop_dsc_t prop;
            scope_dsc_t scp;
        };
    } e;

    /* modification desc. (const) */
    union
    {
        struct {
            unsigned flags;
            const char *name;
            const char *val;
        } prop;

        struct {
            unsigned flags;
            const char *type;
            const char *name;
        } scp;
    } mod;

    /* matched elements tracking index (shared) */
    int *p_eind;

    /* found status (shared) */
    fndstat_t *p_fndstat;

    /* last element spec. (shared) */
    mod_lst_t *p_lst;
} mod_hndl_t;

#define MOD_F_PROP_NAME     1
#define MOD_F_PROP_VAL      2

/* Element modification support function.

   Copies input bytes (from the offset staring the not processed range) for
   a property (whose parts are described by appropriate sp_loc_t structs) which
   is modified by the function.

   NOTE: The function may leave some (last) part of the property not copied if
   this part need not to be modified. The part will be finally copied by the
   next call to __cpy_to_out().
 */
static sp_errc_t cpy_mod_prop(base_updt_hndl_t *p_bu,
    const sp_loc_t *p_lname, const sp_loc_t *p_lval, const sp_loc_t *p_ldef,
    const char *new_name, const char *new_val, int mod_flags)
{
    sp_errc_t ret=SPEC_SUCCESS;

    /* name */
    EXEC_RG(__cpy_to_out(p_bu, p_lname->beg));

    if (mod_flags & MOD_F_PROP_NAME) {
        EXEC_RG(sp_parser_tokenize_str(p_bu->out, SP_TKN_ID, new_name, 0));
        p_bu->in_off = p_lname->end+1;
    } else {
        EXEC_RG(__cpy_to_out(p_bu, p_lname->end+1));
    }

    /* value */
    if (mod_flags & MOD_F_PROP_VAL)
    {
        if (p_lval) {
            if (new_val && *new_val) {
                EXEC_RG(__cpy_to_out(p_bu, p_lval->beg));
                EXEC_RG(
                    sp_parser_tokenize_str(p_bu->out, SP_TKN_VAL, new_val, 0));
                p_bu->in_off = p_lval->end+1;
            } else {
                CHK_FERR(sp_fputc(';', p_bu->out));
                p_bu->in_off = p_ldef->end+1;
            }
        } else
        if (new_val && *new_val) {
            p_bu->in_off = p_ldef->end+1;

#ifdef CONFIG_CUT_VAL_LEADING_SPACES
            if (!(p_bu->flags & SP_F_NVSRSP)) {
                CHK_FERR(sp_fputs(" = ", p_bu->out));
            } else {
#endif
                CHK_FERR(sp_fputc('=', p_bu->out));
#ifdef CONFIG_CUT_VAL_LEADING_SPACES
            }
#endif
            EXEC_RG(sp_parser_tokenize_str(p_bu->out, SP_TKN_VAL, new_val, 0));
#ifndef CONFIG_NO_SEMICOL_ENDS_VAL
            CHK_FERR(sp_fputc(';', p_bu->out));
#else
            /* added value need to be finished by EOL */
            EXEC_RG(put_eol_ind(p_bu, p_ldef, IND_F_CUTGAP|IND_F_CHKEOL));
#endif
        }
    }

finish:
    return ret;
}

#define MOD_F_SCOPE_TYPE    1
#define MOD_F_SCOPE_NAME    2

/* Section-related counterpart to cpy_mod_prop().
 */
static sp_errc_t cpy_mod_scope(base_updt_hndl_t *p_bu,
    const sp_loc_t *p_ltype, const sp_loc_t *p_lname, const sp_loc_t *p_lbdyenc,
    const char *new_type, const char *new_name, int mod_flags)
{
    sp_errc_t ret=SPEC_SUCCESS;
    int typ_rmed=0;

    /* type */
    if (p_ltype) {
        EXEC_RG(__cpy_to_out(p_bu, p_ltype->beg));

        if (mod_flags & MOD_F_SCOPE_TYPE) {
            if (new_type && *new_type) {
                EXEC_RG(
                    sp_parser_tokenize_str(p_bu->out, SP_TKN_ID, new_type, 0));
                p_bu->in_off = p_ltype->end+1;
                EXEC_RG(__cpy_to_out(p_bu, p_lname->beg));
            } else {
                p_bu->in_off = p_lname->beg;
                typ_rmed = 1;
            }
        } else {
            EXEC_RG(__cpy_to_out(p_bu, p_lname->beg));
        }
    } else {
        EXEC_RG(__cpy_to_out(p_bu, p_lname->beg));

        if ((mod_flags & MOD_F_SCOPE_TYPE) && new_type && *new_type) {
            EXEC_RG(sp_parser_tokenize_str(p_bu->out, SP_TKN_ID, new_type, 0));
            CHK_FERR(sp_fputc(' ', p_bu->out));
        }
    }

    /* name */
    if (mod_flags & MOD_F_SCOPE_NAME) {
        EXEC_RG(sp_parser_tokenize_str(p_bu->out, SP_TKN_ID, new_name, 0));
        p_bu->in_off = p_lname->end+1;
    } else {
        EXEC_RG(__cpy_to_out(p_bu, p_lname->end+1));
    }

    /* body */
    if (sp_loc_len(p_lbdyenc)==1 && typ_rmed) {
        CHK_FERR(sp_fputs(" {}", p_bu->out));
        p_bu->in_off = p_lbdyenc->end+1;
    }

finish:
    return ret;
}

/* Element modification parser callback: property */
static sp_errc_t mod_cb_prop(void *arg, SP_FILE *in,
    const sp_loc_t *p_lname, const sp_loc_t *p_lval, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    mod_hndl_t *p_mhndl = (mod_hndl_t*)arg;

    /* ignore props until the destination scope */
    if ((p_mhndl->b.path.beg >= p_mhndl->b.path.end) && !p_mhndl->e.is_scp)
    {
        size_t nm_len = strlen(p_mhndl->e.prop.name);

        if (*p_mhndl->p_fndstat==ELM_NOT_FND) *p_mhndl->p_fndstat=ELM_DEST_FND;
        CMPLOC_RG(in, SP_TKN_ID, p_lname, p_mhndl->e.prop.name, nm_len, 0);

        /* matching element found */
        *p_mhndl->p_fndstat = ELM_FND;
        *p_mhndl->p_eind += 1;

        if (p_mhndl->e.prop.ind == *p_mhndl->p_eind)
        {
            EXEC_RG(cpy_mod_prop(
                p_mhndl->p_bu, p_lname, p_lval, p_ldef,
                p_mhndl->mod.prop.name, p_mhndl->mod.prop.val,
                p_mhndl->mod.prop.flags));

            /* specific element found; done */
            ret = SPEC_CB_FINISH;
            *p_mhndl->b.p_finish = 1;
        } else
        if (p_mhndl->e.prop.ind == SP_IND_ALL)
        {
            EXEC_RG(cpy_mod_prop(
                p_mhndl->p_bu, p_lname, p_lval, p_ldef,
                p_mhndl->mod.prop.name, p_mhndl->mod.prop.val,
                p_mhndl->mod.prop.flags));
        } else
        if (p_mhndl->e.prop.ind == SP_IND_LAST)
        {
            p_mhndl->p_lst->prop.lname = *p_lname;
            if (p_lval) {
                p_mhndl->p_lst->prop.lval = *p_lval;
            } else {
                /* prop w/o a value */
                memset(&p_mhndl->p_lst->prop.lval, 0, sizeof(sp_loc_t));
            }
            p_mhndl->p_lst->prop.ldef = *p_ldef;
        }
    }
finish:
    return ret;
}

/* Element modification parser callback: scope */
static sp_errc_t mod_cb_scope(void *arg, SP_FILE *in,
    const sp_loc_t *p_ltype, const sp_loc_t *p_lname, const sp_loc_t *p_lbody,
    const sp_loc_t *p_lbdyenc, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    mod_hndl_t *p_mhndl = (mod_hndl_t*)arg;

    if (p_mhndl->b.path.beg < p_mhndl->b.path.end) {
        mod_hndl_t mhndl = *p_mhndl;
        CALL_FOLLOW_SCOPE_PATH(mhndl);
    } else
    if (p_mhndl->e.is_scp)  {
        size_t typ_len = (p_mhndl->e.scp.type ? strlen(p_mhndl->e.scp.type) : 0);
        size_t nm_len = strlen(p_mhndl->e.scp.name);

        if (*p_mhndl->p_fndstat==ELM_NOT_FND) *p_mhndl->p_fndstat=ELM_DEST_FND;
        CMPLOC_RG(in, SP_TKN_ID, p_ltype, p_mhndl->e.scp.type, typ_len, 0);
        CMPLOC_RG(in, SP_TKN_ID, p_lname, p_mhndl->e.scp.name, nm_len, 0);

        /* matching element found */
        *p_mhndl->p_fndstat = ELM_FND;
        *p_mhndl->p_eind += 1;

        if (p_mhndl->e.scp.ind == *p_mhndl->p_eind)
        {
            EXEC_RG(cpy_mod_scope(
                p_mhndl->p_bu, p_ltype, p_lname, p_lbdyenc,
                p_mhndl->mod.scp.type, p_mhndl->mod.scp.name,
                p_mhndl->mod.scp.flags));

            /* specific element found; done */
            ret = SPEC_CB_FINISH;
            *p_mhndl->b.p_finish = 1;
        } else
        if (p_mhndl->e.scp.ind == SP_IND_ALL)
        {
            EXEC_RG(cpy_mod_scope(
                p_mhndl->p_bu, p_ltype, p_lname, p_lbdyenc,
                p_mhndl->mod.scp.type, p_mhndl->mod.scp.name,
                p_mhndl->mod.scp.flags));
        } else
        if (p_mhndl->e.scp.ind == SP_IND_LAST)
        {
            if (p_ltype) {
                p_mhndl->p_lst->scp.ltype = *p_ltype;
            } else {
                /* scope w/o a type */
                memset(&p_mhndl->p_lst->scp.ltype, 0, sizeof(sp_loc_t));
            }
            p_mhndl->p_lst->scp.lname = *p_lname;
            p_mhndl->p_lst->scp.lbdyenc = *p_lbdyenc;
        }
    }
finish:
    return ret;
}

/* Property modification; support funct. for sp_set_prop() and sp_mv_prop().
 */
static sp_errc_t mod_prop(SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc,
    const char *name, const char *new_name, const char *new_val, int ind,
    const char *path, const char *deftp, unsigned mod_flags, unsigned long flags)
{
    sp_errc_t ret=SPEC_SUCCESS;
    mod_hndl_t mhndl;

    __BASE_DEFS
    __BASEUPD_DEF
    __RM_MOD_DEFS

    /* last element spec. */
    mod_lst_t lst;

    if (!in || !out || !name ||
        ((mod_flags & MOD_F_PROP_NAME) && !new_name) ||
        (ind<0 && ind!=SP_IND_LAST && ind!=SP_IND_ALL))
    {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    memset(&mhndl, 0, sizeof(mhndl));
    memset(&lst, 0, sizeof(lst));

    init_base_hndl(&mhndl.b,
        &f_finish, &lsc, &sind, path, deftp, mod_cb_prop, mod_cb_scope);

    EXEC_RG(init_base_updt_hndl(&bu, in, out, p_parsc, flags));
    mhndl.p_bu = &bu;

    mhndl.e.is_scp = 0;
    mhndl.e.prop.name = name;
    mhndl.e.prop.ind = ind;

    mhndl.mod.prop.flags = mod_flags;
    mhndl.mod.prop.name = new_name;
    mhndl.mod.prop.val = new_val;

    mhndl.p_eind = &eind;
    mhndl.p_fndstat = &fndstat;
    mhndl.p_lst = &lst;

    EXEC_RG(parse_with_lsc_handling(in, p_parsc, &mhndl.b, &mhndl));

    if (fndstat==ELM_NOT_FND)
    {
        /* destination scope was not found */
        ret=SPEC_NOTFOUND;
        goto finish;
    }

    if (fndstat==ELM_DEST_FND || (ind>=0 && eind!=ind))
    {
        if ((flags & SP_F_NOADD) ||
            !(mod_flags & MOD_F_PROP_VAL) ||
            (ind>=0 && (eind+1)!=ind))
        {
            /* not found or not allowed/possible to add */
            ret=SPEC_NOTFOUND;
        } else {
            EXEC_RG(sp_add_prop(in, out, p_parsc,
                ((mod_flags & MOD_F_PROP_NAME) ? new_name : name),
                new_val, SP_ELM_LAST, path, deftp, flags));
        }
        goto finish;
    }

    /* process the last element if required */
    if (lst.prop.ldef.first_column) {
        EXEC_RG(cpy_mod_prop(&bu, &lst.prop.lname,
            (lst.prop.lval.first_column ? &lst.prop.lval : NULL),
            &lst.prop.ldef, new_name, new_val, mod_flags));
    }

    /* copy untouched last part of the input */
    EXEC_RG(__cpy_to_out(&bu, (p_parsc ? p_parsc->end+1 : EOF)));

finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_set_prop(SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc,
    const char *name, const char *val, int ind, const char *path,
    const char *deftp, unsigned long flags)
{
    return mod_prop(in, out, p_parsc, name, NULL, val,
        ind, path, deftp, MOD_F_PROP_VAL, flags);
}

/* exported; see header for details */
sp_errc_t sp_mv_prop(SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc,
    const char *name, const char *new_name, int ind, const char *path,
    const char *deftp, unsigned long flags)
{
    return mod_prop(in, out, p_parsc, name, new_name, NULL,
        ind, path, deftp, MOD_F_PROP_NAME, flags);
}

/* exported; see header for details */
sp_errc_t sp_mv_scope(
    SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc, const char *type,
    const char *name, const char *new_type, const char *new_name, int ind,
    const char *path, const char *deftp, unsigned long flags)
{
    sp_errc_t ret=SPEC_SUCCESS;
    mod_hndl_t mhndl;
    unsigned mod_flags = MOD_F_SCOPE_TYPE|MOD_F_SCOPE_NAME;

    __BASE_DEFS
    __BASEUPD_DEF
    __RM_MOD_DEFS

    /* last element spec. */
    mod_lst_t lst;

    if (!in || !out || !name || !new_name ||
        (ind<0 && ind!=SP_IND_LAST && ind!=SP_IND_ALL))
    {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    memset(&mhndl, 0, sizeof(mhndl));
    memset(&lst, 0, sizeof(lst));

    init_base_hndl(&mhndl.b,
        &f_finish, &lsc, &sind, path, deftp, mod_cb_prop, mod_cb_scope);

    EXEC_RG(init_base_updt_hndl(&bu, in, out, p_parsc, flags));
    mhndl.p_bu = &bu;

    mhndl.e.is_scp = 1;
    mhndl.e.scp.type = type;
    mhndl.e.scp.name = name;
    mhndl.e.scp.ind = ind;

    mhndl.mod.scp.flags = mod_flags;
    mhndl.mod.scp.type = new_type;
    mhndl.mod.scp.name = new_name;

    mhndl.p_eind = &eind;
    mhndl.p_fndstat = &fndstat;
    mhndl.p_lst = &lst;

    EXEC_RG(parse_with_lsc_handling(in, p_parsc, &mhndl.b, &mhndl));

    if (fndstat!=ELM_FND) {
        /* scope not found */
        ret=SPEC_NOTFOUND;
        goto finish;
    }

    /* process the last element if required */
    if (lst.scp.lbdyenc.first_column) {
        EXEC_RG(cpy_mod_scope(&bu,
            (lst.scp.ltype.first_column ? &lst.scp.ltype : NULL),
            &lst.scp.lname, &lst.scp.lbdyenc, new_type, new_name, mod_flags));
    }

    /* copy untouched last part of the input */
    EXEC_RG(__cpy_to_out(&bu, (p_parsc ? p_parsc->end+1 : EOF)));

finish:
    return ret;
}

#undef __RM_MOD_DEFS
#undef __BASEUPD_DEF
#undef __NEIND_DEF
#undef __EIND_DEF
#undef __BASE_DEFS
