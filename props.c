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
#include "config.h"
#include "sp_props/parser.h"

/* GENERAL NOTE: line & columns in sp_loc_t struct are are 1-based, therefore
   0 has a special meaning in the code to distinguish uninitialized/special
   state.
 */

#define EXEC_RG(c) if ((ret=(c))!=SPEC_SUCCESS) goto finish;
#define CHK_FSEEK(c) if ((c)!=0) { ret=SPEC_ACCS_ERR; goto finish; }
#define CHK_FERR(c) if ((c)<0) { ret=SPEC_ACCS_ERR; goto finish; }

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

/* iteration handle - modification specific struct */
typedef struct _mod_iter_hndl_t
{
    /* output file handle storing provided modifications */
    FILE *out;

    /* temporary output file handle used by a scope callback for scope body
       modifications */
    FILE *sc_out;

    /* parsing scope */
    const sp_loc_t *p_parsc;

    /* type of EOL detected on the input */
    eol_t eol_typ;

    /* file offset staring not processed range of the input file */
    long in_off;

    /* range of consecutive locations to delete;
       initially set to an empty range */
    struct _delrng_t {
        sp_loc_t loc;
        struct {
            unsigned eol_ended: 1;
            unsigned ready: 1;      /* !=0: contains element(s) to delete */
        };
    } delrng;

    /* current enclosing scope (non-global scope) */
    struct {
        sp_loc_t lbody;
        sp_loc_t ldef;  /* zeroed - global scope */
        unsigned set;   /* used to reduce unnecessary copying */
    } encsc;

    /* used to track the enclosing scope */
    struct {
        const sp_loc_t *p_lbody;
        const sp_loc_t *p_ldef;
    } trck_encsc;

    /* handled entry definition (initially zeroed) */
    sp_loc_t ent_ldef;

    /* used to handle indentation after inserted EOL */
    struct {
        /* offset range for which EOL is considered;
           initially set to an empty range */
        struct {
            long beg;
            long end;   /* inclusive */
        } eol_rng;

        /* !=0: deferred indentation has been requested */
        unsigned defer;
    } indent;

    unsigned finish_req;    /* finish requested by a callback */
} mod_iter_hndl_t;

/* iteration handle - base struct

   NOTE: This struct is cloned during upward-downward process of following
   a destination scope path, so any changes made on it are not propagated to
   scopes on higher or the same scope level. Therefore, if such propagation is
   required a struct's field must be an immutable pointer to an object
   containing propagated information (virtual inheritance).
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

    /* modification iteration specific handle */
    mod_iter_hndl_t *p_mihndl;
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
        FILE *sc_out = (p_ihndl->p_mihndl ? p_ihndl->p_mihndl->sc_out : NULL);

        EXEC_RG(sp_parser_tkn_cpy(p_phndl, SP_TKN_ID,
            p_ltype, p_ihndl->buf1.ptr, p_ihndl->buf1.sz, &tktype.len));
        EXEC_RG(sp_parser_tkn_cpy(p_phndl, SP_TKN_ID,
            p_lname, p_ihndl->buf2.ptr, p_ihndl->buf2.sz, &tkname.len));

        if (p_ltype) tktype.loc = *p_ltype;
        tkname.loc = *p_lname;

        ret = p_ihndl->cb.scope(p_ihndl->cb.arg, p_ihndl->in, sc_out,
            p_ihndl->buf1.ptr, (p_ltype ? &tktype : NULL),
            p_ihndl->buf2.ptr, &tkname, p_lbody, p_ldef);
    }
finish:
    return ret;
}

/* Initialize iter_hndl_t handle */
static void init_iter_hndl(iter_hndl_t *p_ihndl, FILE *in, const char *path,
    const char *defsc, sp_cb_prop_t cb_prop, sp_cb_scope_t cb_scope, void *arg,
    char *buf1, size_t b1len, char *buf2, size_t b2len,
    mod_iter_hndl_t *p_mihndl)
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

    if (path && *path=='/') path++;
    p_ihndl->path.beg = path;
    p_ihndl->path.end = (!path ? NULL : p_ihndl->path.beg+strlen(path));
    p_ihndl->path.defsc = defsc;

    p_ihndl->cb.arg = arg;
    p_ihndl->cb.prop = cb_prop;
    p_ihndl->cb.scope = cb_scope;

    p_ihndl->p_mihndl = p_mihndl;
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
        buf1, b1len, buf2, b2len, NULL);

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

/* exported; see header for details */
sp_errc_t sp_write_prop(FILE *out, const char *name, const char *val)
{
    sp_errc_t ret=SPEC_SUCCESS;

    EXEC_RG(sp_parser_tokenize_str(out, SP_TKN_ID, name));
    if (val) {
#ifdef CUT_VAL_LEADING_SPACES
        CHK_FERR(fputc(' ', out)|fputc('=', out)|fputc(' ', out));
#else
        CHK_FERR(fputc('=', out));
#endif
        EXEC_RG(sp_parser_tokenize_str(out, SP_TKN_VAL, val));
    }

#ifndef NO_SEMICOL_ENDS_VAL
    CHK_FERR(fputc(';', out)|fputc('\n', out));
#else
    CHK_FERR(fputc('\n', out));
#endif

finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_write_empty_scope(FILE *out, const char *type, const char *name)
{
    sp_errc_t ret=SPEC_SUCCESS;

    if (type) {
        EXEC_RG(sp_parser_tokenize_str(out, SP_TKN_ID, type));
        CHK_FERR(fputc(' ', out));
    }

    EXEC_RG(sp_parser_tokenize_str(out, SP_TKN_ID, name));

    if (!type) {
        CHK_FERR(
            fputc(' ', out)|fputc('{', out)|fputc('}', out)|fputc('\n', out));
    } else {
        CHK_FERR(fputc(';', out)|fputc('\n', out));
    }
finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_write_comment(FILE *out, const char *text)
{
    sp_errc_t ret=SPEC_SUCCESS;

    if (*text) {
        CHK_FERR(fputc('#', out)|fputc(' ', out));

        for (; *text; text++) {
            CHK_FERR(fputc(*text, out));
            if (*text=='\n' && *(text+1)) {
                CHK_FERR(fputc('#', out)|fputc(' ', out));
            }
        }

        /* write missing EOL */
        if (*(text-1)!='\n') { CHK_FERR(fputc('\n', out)); }
    }

finish:
    return ret;
}

/* Copies input file bytes (from the last input offset) to the output file up to
   'end' offset (exclusive). If end==EOF input is copied up to the end of the
   file. In case of success (and there is something to copy) the file offset is
   set at 'end'.
 */
static sp_errc_t cpy_to_out(iter_hndl_t *p_ihndl, long end)
{
    sp_errc_t ret=SPEC_ACCS_ERR;
    mod_iter_hndl_t *p_mihndl = p_ihndl->p_mihndl;
    long beg = p_mihndl->in_off;

    FILE *in = p_ihndl->in;
    FILE *out = p_mihndl->out;

    if (beg<end || end==EOF)
    {
        CHK_FSEEK(fseek(in, beg, SEEK_SET));
        for (; beg<end || end==EOF; beg++) {
            int c = fgetc(in);
            if (c==EOF && end==EOF) break;
            if (c==EOF || fputc(c, out)==EOF) goto finish;
        }
        p_mihndl->in_off = beg;
    }
    ret = SPEC_SUCCESS;

finish:
    return ret;
}

/* Trim line spaces starting from the last input offset. Trimming is finished on
   the first non-space character or EOL (which may be deleted or not, depending
   on 'del_eol'). If a comment is in-lined in the deleted line it's also removed.
   The function returns !=0 if trimmed spaces constitute a line (finished by EOL),
   0 otherwise.
 */
static int trim_line(iter_hndl_t *p_ihndl, int del_eol)
{
    int endc=2;   /* 0:non-space, 1:EOL, 2:EOF */
    int c, comment=0;
    mod_iter_hndl_t *p_mihndl = p_ihndl->p_mihndl;
    FILE *in = p_ihndl->in;

    if (fseek(in, p_mihndl->in_off, SEEK_SET)) goto finish;
    for (endc=0;
        !endc && (isspace(c=fgetc(in)) || (c=='#' ? (comment=1) : 0) || comment);
        p_mihndl->in_off++)
    {
        if (c!='\r' && c!='\n') continue;

        switch (p_mihndl->eol_typ)
        {
        case EOL_LF:
            if (c=='\n') endc=1;
            break;

        case EOL_CR:
        case EOL_CRLF:
            if (c=='\r') {
                if (p_mihndl->eol_typ==EOL_CR) endc=1;
                else {
                    c = fgetc(in);
                    if (c!=EOF) {
                        p_mihndl->in_off++;
                        if (c=='\n') endc=1;
                    } else
                        goto break_loop;
                }
            }
            break;

        default:    /* will never happen */
            goto break_loop;
        }
    }
break_loop:

    if (c==EOF) endc=2;
    if (endc==1 && !del_eol)
        p_mihndl->in_off -= (p_mihndl->eol_typ==EOL_CRLF ? 2 : 1);
finish:
    return (endc==1);
}

/* Delete concatenated range of locations scheduled to be removed. */
static sp_errc_t del_rng(iter_hndl_t *p_ihndl, int last_upd)
{
    sp_errc_t ret=SPEC_SUCCESS;

    mod_iter_hndl_t *p_mihndl = p_ihndl->p_mihndl;
    struct _delrng_t *p_delrng = &p_mihndl->delrng;

    long beg=p_delrng->loc.beg, end=p_delrng->loc.end+1;
    FILE *in = p_ihndl->in;

    if (!p_delrng->ready) goto finish;

    if (p_delrng->eol_ended || last_upd) {
        /* remove leading spaces */
        int n_col=p_delrng->loc.first_column-1, n_sp=n_col, i;

        if (n_col>0 && !fseek(in, p_delrng->loc.beg-n_col, SEEK_SET)) {
            for (i=n_col; i>0; i--)
                if (!isspace(fgetc(in))) n_sp=i-1;
            beg -= n_sp;
        }
        if (n_sp==n_col && p_delrng->eol_ended) {
            /* remove empty line */
            end += (p_mihndl->eol_typ==EOL_CRLF ? 2 : 1);
        }
    }

    EXEC_RG(cpy_to_out(p_ihndl, beg));
    p_mihndl->in_off = end;

    /* check if deleted range shall extend indenting EOL range */
    if ((p_mihndl->indent.eol_rng.end >= 0) &&
        (p_mihndl->indent.eol_rng.end >= beg-1))
    {
        p_mihndl->indent.eol_rng.end = end-1;
    }

    p_delrng->ready = 0;
finish:
    return ret;
}

/* Add a location to concatenated range of locations scheduled to be removed */
static sp_errc_t add_to_delrng(iter_hndl_t *p_ihndl, const sp_loc_t *p_loc)
{
    sp_errc_t ret=SPEC_SUCCESS;
    long org_off, end_off;
    int eol_ended;

    mod_iter_hndl_t *p_mihndl = p_ihndl->p_mihndl;
    struct _delrng_t *p_delrng = &p_mihndl->delrng;

    /* detect end offset of a location being deleted */
    org_off = p_mihndl->in_off;
    p_mihndl->in_off = p_loc->end+1;
    eol_ended = trim_line(p_ihndl, 0);
    end_off = p_mihndl->in_off;
    p_mihndl->in_off = org_off;

    /* detect range continuation */
    if (p_delrng->ready && p_delrng->loc.end+1==p_loc->beg) {
        p_delrng->loc.last_line = p_loc->last_line;
        p_delrng->loc.last_column = p_loc->last_column+end_off-org_off-1;
        p_delrng->loc.end = end_off-1;
        p_delrng->eol_ended = eol_ended;
    } else {
        if (p_delrng->ready) { EXEC_RG(del_rng(p_ihndl, 0)); }
        memcpy(&p_delrng->loc, p_loc, sizeof(*p_loc));
        p_delrng->loc.last_column += end_off-org_off-1;
        p_delrng->loc.end = end_off-1;
        p_delrng->eol_ended = eol_ended;
    }

    p_delrng->ready = 1;
finish:
    return ret;
}

#define __IS_ENTDEF_SET(ih) ((ih)->p_mihndl->ent_ldef.first_column!=0)

#define __IS_ON_GLOBSC(ih) (!((ih)->p_mihndl->encsc.ldef.first_column))

#define __IS_ON_EOL(ih) \
    (((ih)->p_mihndl->indent.eol_rng.beg <= (ih)->p_mihndl->in_off-1) && \
     ((ih)->p_mihndl->in_off-1 <= (ih)->p_mihndl->indent.eol_rng.end))

#define __GET_ENCSC_BDY(ih) \
    (!(ih)->p_mihndl->encsc.lbody.first_column ? \
     (ih)->p_mihndl->p_parsc : &(ih)->p_mihndl->encsc.lbody)

/* Write indent as leading spaces of a given definition starting line. */
static void
    write_def_strtln_ldsp(const iter_hndl_t *p_ihndl, const sp_loc_t *p_ldef)
{
    int n_col=p_ldef->first_column-1, c;
    if (n_col>0 && !fseek(p_ihndl->in, p_ldef->beg-n_col, SEEK_SET)) {
        for (; n_col>0 && isspace(c=fgetc(p_ihndl->in)); n_col--)
            if (fputc(c, p_ihndl->p_mihndl->out)==EOF)
                break;
    }
}

/* Write indent basing on a given definition start */
static void
    write_def_beg_ind(const iter_hndl_t *p_ihndl, const sp_loc_t *p_ldef)
{
    int n_col = p_ldef->first_column-1, c;
    if (n_col>0 && !fseek(p_ihndl->in, p_ldef->beg-n_col, SEEK_SET)) {
        for (; n_col>0 && (c=fgetc(p_ihndl->in))!=EOF; n_col--)
            if (fputc((isspace(c) ? c : ' '), p_ihndl->p_mihndl->out)==EOF)
                break;
    }
}

/* Write indent for a given definition. */
static void write_indent(const iter_hndl_t *p_ihndl, const sp_loc_t *p_ldef)
{
    const sp_loc_t *p_encsc_bdy = __GET_ENCSC_BDY(p_ihndl);
    if (!p_encsc_bdy || p_encsc_bdy->first_line < p_ldef->first_line) {
        write_def_strtln_ldsp(p_ihndl, p_ldef);
    } else {
        write_def_beg_ind(p_ihndl, p_encsc_bdy);
    }
}

/* Write the EOL marker. */
static sp_errc_t write_eol(const iter_hndl_t *p_ihndl)
{
    sp_errc_t ret=SPEC_SUCCESS;
    FILE *out = p_ihndl->p_mihndl->out;

    switch (p_ihndl->p_mihndl->eol_typ) {
        default:
        case EOL_LF:
            CHK_FERR(fputc('\n', out));
            break;
        case EOL_CR:
            CHK_FERR(fputc('\r', out));
            break;
        case EOL_CRLF:
            CHK_FERR(fputc('\r', out)|fputc('\n', out));
            break;
    }
finish:
    return ret;
}

/* EOL & indent are not written if beginning of a definition is located as
   the EOL were been already written (e.g. at the beginning of a line with
   leading spaces only).
 */
#define EOLIND_CHK_EOL      1

/* Write EOL with deferred indent. Trailing spaces and EOL are trimmed starting
   from the last input offset.
 */
#define EOLIND_DEFER        2

/* Write EOL with an indent for a definition. Writing EOL is checked
   against an error but the indent is not (simply cosmetic aspect).
 */
static sp_errc_t write_eol_indent(
    iter_hndl_t *p_ihndl, const sp_loc_t *p_ldef, unsigned flags)
{
    sp_errc_t ret = SPEC_SUCCESS;
    mod_iter_hndl_t *p_mihndl = p_ihndl->p_mihndl;
    const sp_loc_t *p_encsc_bdy = __GET_ENCSC_BDY(p_ihndl);
    int eol_ended = 0;

    if (flags & EOLIND_CHK_EOL)
    {
        long chk_beg;
        int n_col, frst_def=p_encsc_bdy && (p_encsc_bdy->beg==p_ldef->beg);

        if (frst_def || __IS_ON_EOL(p_ihndl)) goto finish;

        /* check if lastly removed range may be concatenated with
           passed def, if so then extend the def with this range */
        if ((p_mihndl->delrng.loc.end >= 0) &&
            (p_mihndl->delrng.loc.end >= p_ldef->beg-1))
        {
            chk_beg = p_mihndl->delrng.loc.beg;
            n_col = p_mihndl->delrng.loc.first_column-1;
        } else {
            chk_beg = p_ldef->beg;
            n_col = p_ldef->first_column-1;
        }

        if (n_col>0 && !fseek(p_ihndl->in, chk_beg-n_col, SEEK_SET)) {
            for (; n_col>0 && isspace(fgetc(p_ihndl->in)); n_col--);
        }
        if (!n_col) goto finish;
    }

    p_mihndl->indent.eol_rng.beg =
        p_mihndl->indent.eol_rng.end = p_mihndl->in_off;

    if (flags & EOLIND_DEFER) {
        /* trim trailing spaces and EOL */
        eol_ended = trim_line(p_ihndl, 1);
        p_mihndl->indent.eol_rng.end = p_mihndl->in_off-1;
    }
    EXEC_RG(write_eol(p_ihndl));

    if (!(flags & EOLIND_DEFER)) {
        write_indent(p_ihndl, p_ldef);
    } else {
        if (!eol_ended) p_mihndl->indent.defer=1;
    }
finish:
    return ret;
}

#define __WRITE_ENDSC_MRK(ih, encdef) \
    write_def_beg_ind((ih), (encdef)); \
    CHK_FERR(fputc('}', (ih)->p_mihndl->out)); \
    (ih)->p_mihndl->in_off = (encdef)->end+1;

/* Handle modification "end" event. 'p_eol_end' is an in/out param telling if
   EOL has been inserted at the call of the function and set to 1 in case the
   function inserts the EOL.
 */
static sp_errc_t handle_end_event(iter_hndl_t *p_ihndl, int *p_eol_end)
{
    sp_errc_t ret=SPEC_SUCCESS;
    mod_iter_hndl_t *p_mihndl = p_ihndl->p_mihndl;

    if (p_mihndl->finish_req) {
        /* finish requested; "end" context can't be established */
        goto finish;
    } else
    if (!__IS_ENTDEF_SET(p_ihndl)) {
        /* no entry handled; in case of global scope the event is applied at
           the end of the scope, in case of non-global scope it means - no
           scope has been found */
        if (p_ihndl->path.beg && *p_ihndl->path.beg) {
            goto finish;
        }
    } else {
        /* start with EOL */
        if (!*p_eol_end) {
            write_eol_indent(p_ihndl, &p_mihndl->ent_ldef, 0);
        } else {
            /* lastly inserted EOL is not indented, do it */
            write_indent(p_ihndl, &p_mihndl->ent_ldef);
        }
    }

// @@@
fprintf(p_mihndl->out, "# END");

    /* finish with EOL */
    *p_eol_end=1;
    EXEC_RG(write_eol(p_ihndl));

finish:
    return ret;
}

/* Deferred update. The previous scope is contained in the iteration handler and
   the current one is passed by 'p_encsc_def'. 'p_encsc_def' is NULL for global
   scope and the last update of sp_iterate_modify() ('last_upd' != 0).

   NOTE: The function doesn't write indent for lastly inserted EOL.
 */
static sp_errc_t deferred_update(
    iter_hndl_t *p_ihndl, const sp_loc_t *p_encsc_def, int last_upd)
{
    sp_errc_t ret=SPEC_SUCCESS;
    mod_iter_hndl_t *p_mihndl = p_ihndl->p_mihndl;
    int on_eol = __IS_ON_EOL(p_ihndl);

    EXEC_RG(del_rng(p_ihndl, last_upd));

    if (last_upd) { EXEC_RG(handle_end_event(p_ihndl, &on_eol)); }

    if (!last_upd) {
        int same_scope =
            __IS_ON_GLOBSC(p_ihndl) ||
            (p_encsc_def->beg==p_mihndl->encsc.ldef.beg &&
             p_encsc_def->end==p_mihndl->encsc.ldef.end);

        if (same_scope) {
            /* deferred indentation update */
            if (p_mihndl->indent.defer)
                write_indent(p_ihndl, &p_mihndl->ent_ldef);
        } else {
            /* if previous scope is in-lined with its last def with inserted
               EOL, then the new EOL provides an indentation issue - scope
               ending marker need to be written "by hand" */
            if (on_eol && __IS_ENTDEF_SET(p_ihndl) &&
                p_mihndl->ent_ldef.last_line==p_mihndl->encsc.lbody.last_line)
            {
                __WRITE_ENDSC_MRK(p_ihndl, &p_ihndl->p_mihndl->encsc.ldef);
            }
        }
    } else {
        /* scope body ending update */
        if (!__IS_ON_GLOBSC(p_ihndl)) {
            /* in case of EOL at last enclosing scope there is a need to
               appropriately indent scope ending marker of the scope */
            if (on_eol) {
                __WRITE_ENDSC_MRK(p_ihndl, &p_ihndl->p_mihndl->encsc.ldef);
            }
        } else
        if (on_eol && !p_mihndl->p_parsc)
            trim_line(p_ihndl, 1);
    }

finish:
    p_mihndl->indent.defer=0;
    return ret;
}

/* Copies the input up to a definition start with EOL at its beginning. */
static sp_errc_t strtdef_with_eol(iter_hndl_t *p_ihndl, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    mod_iter_hndl_t *p_mihndl = p_ihndl->p_mihndl;
    long off = p_mihndl->in_off;
    FILE *in = p_ihndl->in;

    if (p_mihndl->ent_ldef.last_line==p_ldef->first_line)
    {
        /* remove unnecessary spaces between previous def and
           the inserted EOL if the definitions are in-lined */
        if (!fseek(in, off, SEEK_SET)) {
            for (; off<p_ldef->beg; off++) {
                int c = fgetc(in);
                if (c!=' ' && c!='\t') break;
            }
        }
        if (off < p_ldef->beg) {
            EXEC_RG(cpy_to_out(p_ihndl, p_ldef->beg));
        }
    } else {
        EXEC_RG(cpy_to_out(p_ihndl, p_ldef->beg));
    }

    p_mihndl->in_off = p_ldef->beg;
    write_eol_indent(p_ihndl, p_ldef, EOLIND_CHK_EOL);
finish:
    return ret;
}

/* Finish a definition modification handling inside a sp_iterate_modify()
   callback.
 */
static sp_errc_t finish_def_handle(iter_hndl_t *p_ihndl, const sp_loc_t *p_ldef)
{
    sp_errc_t ret = SPEC_SUCCESS;
    if (p_ihndl->p_mihndl->in_off < p_ldef->end+1)
    {
        /* copy untouched last part of the def (or the entire definition
           in case it's modification has not been performed) by a callback */
        EXEC_RG(cpy_to_out(p_ihndl, p_ldef->end+1));
        p_ihndl->p_mihndl->in_off = p_ldef->end+1;
    }
finish:
    return ret;
}

/* Set current enclosing scope */
static void set_encsc(iter_hndl_t *p_ihndl)
{
    mod_iter_hndl_t *p_mihndl = p_ihndl->p_mihndl;
    if (!p_mihndl->encsc.set && p_mihndl->trck_encsc.p_ldef)
    {
        p_mihndl->encsc.ldef = *p_mihndl->trck_encsc.p_ldef;
        if (p_mihndl->trck_encsc.p_lbody) {
            p_mihndl->encsc.lbody = *p_mihndl->trck_encsc.p_lbody;
        } else {
            /* mark no body */
            memset(&p_mihndl->encsc.lbody, 0, sizeof(p_mihndl->encsc.lbody));
        }
        p_mihndl->encsc.set = 1;
    }
}

/* Check callback return and performs deferred updates. */
#define __POST_CB_ITER_CALL(mb, nb) \
    if ((int)ret>0 || (p_ihndl->path.beg < p_ihndl->path.end)) goto finish; \
    cb_bf = sp_cb_errc((int)ret); \
    is_mod = cb_bf & (mb); \
    is_del = cb_bf & SP_CBEC_FLG_DEL_DEF; \
    if ((is_mod && is_del) || ((cb_bf & (nb)) && !strlen(name))) \
    { ret=SPEC_CB_RET_ERR; goto finish; } \
    if (!is_del) { \
        EXEC_RG(deferred_update(p_ihndl, p_mihndl->trck_encsc.p_ldef, 0)); \
    }

/* sp_iterate_modify() callback: property */
static sp_errc_t mod_iter_cb_prop(const sp_parser_hndl_t *p_phndl,
    const sp_loc_t *p_lname, const sp_loc_t *p_lval, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    int cb_bf=0, is_mod, is_del;

    iter_hndl_t *p_ihndl = (iter_hndl_t*)p_phndl->cb.arg;
    mod_iter_hndl_t *p_mihndl = p_ihndl->p_mihndl;

    char *name = p_ihndl->buf1.ptr;
    char *val = p_ihndl->buf2.ptr;

    FILE *out = p_mihndl->out;

    /* follow destination path and call property callback */
    ret = iter_cb_prop(p_phndl, p_lname, p_lval, p_ldef);
    __POST_CB_ITER_CALL(SP_CBEC_FLG_MOD_PROP_NAME|SP_CBEC_FLG_MOD_PROP_VAL,
        SP_CBEC_FLG_MOD_PROP_NAME);

    if (is_del) {
        EXEC_RG(add_to_delrng(p_ihndl, p_ldef));
    } else
    {
        /* property name
         */
        if (cb_bf & SP_CBEC_FLG_MOD_PROP_NAME) {
            EXEC_RG(cpy_to_out(p_ihndl, p_lname->beg));
            EXEC_RG(sp_parser_tokenize_str(out, SP_TKN_ID, name));
            p_mihndl->in_off = p_lname->end+1;
        }

        /* property value
         */
        if (cb_bf & SP_CBEC_FLG_MOD_PROP_VAL)
        {
            if (!strlen(val)) {
                if (p_lval)
                {
                    /* delete a value */
                    EXEC_RG(cpy_to_out(p_ihndl, p_lname->end+1));
                    CHK_FERR(fputc(';', out));
                    p_mihndl->in_off = p_ldef->end+1;
                }
            } else {
                if (p_lval)
                {
                    /* modify a value */
                    EXEC_RG(cpy_to_out(p_ihndl, p_lval->beg));
                    EXEC_RG(sp_parser_tokenize_str(out, SP_TKN_VAL, val));
                    p_mihndl->in_off = p_lval->end+1;
                } else
                {
                    /* add a value */
                    EXEC_RG(cpy_to_out(p_ihndl, p_lname->end+1));
#ifdef CUT_VAL_LEADING_SPACES
                    CHK_FERR(fputc(' ', out)|fputc('=', out)|fputc(' ', out));
#else
                    CHK_FERR(fputc('=', out));
#endif
                    EXEC_RG(sp_parser_tokenize_str(out, SP_TKN_VAL, val));
                    p_mihndl->in_off = p_ldef->end+1;
#ifndef NO_SEMICOL_ENDS_VAL
                    CHK_FERR(fputc(';', out));
#else
                    /* added value need to be finished by EOL */
                    EXEC_RG(write_eol_indent(p_ihndl, p_ldef, EOLIND_DEFER));
#endif
                }
            }
        }

        EXEC_RG(finish_def_handle(p_ihndl, p_ldef));
    }

    p_mihndl->ent_ldef = *p_ldef;
    ret = (cb_bf & SP_CBEC_FLG_FINISH ?
        (p_mihndl->finish_req=1, SPEC_CB_FINISH) : SPEC_SUCCESS);
finish:
    return ret;
}

/* sp_iterate_modify() callback: scope */
static sp_errc_t mod_iter_cb_scope(
    const sp_parser_hndl_t *p_phndl, const sp_loc_t *p_ltype,
    const sp_loc_t *p_lname, const sp_loc_t *p_lbody, const sp_loc_t *p_ldef)
{
    sp_errc_t ret=SPEC_SUCCESS;
    long mod_bdy_len=0;
    int cb_bf=0, is_mod, is_del;

    iter_hndl_t *p_ihndl = (iter_hndl_t*)p_phndl->cb.arg;
    mod_iter_hndl_t *p_mihndl = p_ihndl->p_mihndl;

    char *type = p_ihndl->buf1.ptr;
    char *name = p_ihndl->buf2.ptr;

    FILE *in = p_ihndl->in;
    FILE *out = p_mihndl->out;
    FILE *sc_out = p_mihndl->sc_out;

    if (sc_out) { CHK_FSEEK(fseek(sc_out, 0, SEEK_SET)); }

    /* track enclosing scope */
    if (p_ihndl->path.beg < p_ihndl->path.end) {
        p_mihndl->trck_encsc.p_lbody = p_lbody;
        p_mihndl->trck_encsc.p_ldef = p_ldef;
        p_mihndl->encsc.set = 0;
    }

    /* follow destination path and call scope callback */
    ret = iter_cb_scope(p_phndl, p_ltype, p_lname, p_lbody, p_ldef);
    __POST_CB_ITER_CALL(SP_CBEC_FLG_MOD_SCOPE_TYPE|SP_CBEC_FLG_MOD_SCOPE_NAME,
        SP_CBEC_FLG_MOD_SCOPE_NAME);

    set_encsc(p_ihndl);

    if (sc_out && !is_del) { CHK_FERR(mod_bdy_len=ftell(sc_out)); }

    if (is_del) {
        EXEC_RG(add_to_delrng(p_ihndl, p_ldef));
    } else
    {
        int c, type_del=0;

        if (mod_bdy_len && !p_lbody) {
            /* to avoid indentation issues, adding a body to
               an empty scope moves this scope to a new line */
            EXEC_RG(strtdef_with_eol(p_ihndl, p_ldef));
        }

        /* scope type
         */
        if (cb_bf & SP_CBEC_FLG_MOD_SCOPE_TYPE)
        {
            if (!strlen(type)) {
                if (p_ltype) {
                    /* delete a type */
                    EXEC_RG(add_to_delrng(p_ihndl, p_ltype));
                    EXEC_RG(del_rng(p_ihndl, 0));
                    type_del=1;
                }
            } else {
                if (p_ltype) {
                    /* modify a type */
                    EXEC_RG(cpy_to_out(p_ihndl, p_ltype->beg));
                    EXEC_RG(sp_parser_tokenize_str(out, SP_TKN_ID, type));
                    p_mihndl->in_off = p_ltype->end+1;
                } else {
                    /* add a type */
                    EXEC_RG(cpy_to_out(p_ihndl, p_ldef->beg));
                    EXEC_RG(sp_parser_tokenize_str(out, SP_TKN_ID, type));
                    CHK_FERR(fputc(' ', out));
                    p_mihndl->in_off = p_lname->beg;
                }
            }
        }

        /* scope name
         */
        if (cb_bf & SP_CBEC_FLG_MOD_SCOPE_NAME) {
            EXEC_RG(cpy_to_out(p_ihndl, p_lname->beg));
            EXEC_RG(sp_parser_tokenize_str(out, SP_TKN_ID, name));
            p_mihndl->in_off = p_lname->end+1;
        }

        /* scope body
         */
        if (mod_bdy_len)
        {
            /* scope body updated by the callback
             */
            EXEC_RG(cpy_to_out(p_ihndl, p_lname->end+1));

            if (p_lbody) {
                /* existing body update */
                long off = p_lname->end+1;

                CHK_FSEEK(fseek(sc_out, off, SEEK_SET));
                for (; off<mod_bdy_len; off++) {
                    CHK_FERR(c=fgetc(sc_out));
                    CHK_FERR(fputc(c, out));
                }
                p_mihndl->in_off = p_lbody->end+1;

                /* scope body ending update */
                if ((c=='\n' || c=='\r')) {
                    if (p_lbody->last_line==p_ldef->last_line)
                    {
                        /* if modified body is in-lined with its enclosing scope,
                           then lastly inserted EOL provides an indentation issue -
                           scope ending marker need to be written "by hand" */
                        __WRITE_ENDSC_MRK(p_ihndl, p_ldef);
                    } else
                        trim_line(p_ihndl, 1);
                }
            } else {
                long i;

                /* empty body update
                 */
                CHK_FSEEK(fseek(sc_out, 0, SEEK_SET));
                CHK_FERR(fputc(' ', out)|fputc('{', out));
                /* start with EOL */
                write_eol_indent(p_ihndl, p_ldef, 0);
                fputc('\t', out);

                for (c=0, i=mod_bdy_len; i>0; i--) {
                    CHK_FERR(c=fgetc(sc_out));
                    if (c=='\n') {
                        EXEC_RG(write_eol_indent(p_ihndl, p_ldef, 0));
                        if (i>1) fputc('\t', out);
                    } else {
                        CHK_FERR(fputc(c, out));
                    }
                }

                /* finish with EOL */
                if (c!='\n') { EXEC_RG(write_eol_indent(p_ihndl, p_ldef, 0)); }
                CHK_FERR(fputc('}', out));
                p_mihndl->in_off = p_ldef->end+1;
                write_eol_indent(p_ihndl, p_ldef, EOLIND_DEFER);
            }
        } else
        if (type_del && !p_lbody)
        {
            /* untyped scope w/o a body must not use alternative
               version of definition to avoid ambiguity */
            EXEC_RG(cpy_to_out(p_ihndl, p_ldef->end));

            CHK_FERR(c=fgetc(in));
            if (c==';') {
                CHK_FERR(fputc('{', out)|fputc('}', out));
            } else {
                CHK_FERR(fputc(c, out));
            }
            p_mihndl->in_off = p_ldef->end+1;
        }

        EXEC_RG(finish_def_handle(p_ihndl, p_ldef));
    }

    p_mihndl->ent_ldef = *p_ldef;
    ret = (cb_bf & SP_CBEC_FLG_FINISH ?
        (p_mihndl->finish_req=1, SPEC_CB_FINISH) : SPEC_SUCCESS);
finish:
    return ret;
}

/* exported; see header for details */
sp_errc_t sp_iterate_modify(
    FILE *in, FILE *out, const sp_loc_t *p_parsc, const char *path,
    const char *defsc, sp_cb_prop_t cb_prop, sp_cb_scope_t cb_scope,
    void *arg, char *buf1, size_t b1len, char *buf2, size_t b2len)
{
    sp_errc_t ret=SPEC_SUCCESS;
    mod_iter_hndl_t mihndl;
    iter_hndl_t ihndl;
    sp_parser_hndl_t phndl;
    FILE *sc_out = NULL;

    if (!in || !out || in==out || (!cb_prop && !cb_scope)) {
        ret=SPEC_INV_ARG;
        goto finish;
    }

    if (cb_scope) {
        /* scopes modifications go to a temporary file
           before merging them to the final output */
        sc_out = tmpfile();
        if (!sc_out) {
            ret = SPEC_TMP_CREAT_ERR;
            goto finish;
        }
    }

    memset(&mihndl, 0, sizeof(mihndl));
    mihndl.out = out;
    mihndl.sc_out = sc_out;
    mihndl.p_parsc = p_parsc;

    /* set to empty ranges */
    mihndl.indent.eol_rng.end = -1;
    mihndl.delrng.loc.end = -1;

    init_iter_hndl(&ihndl, in, path, defsc, cb_prop, cb_scope,
        arg, buf1, b1len, buf2, b2len, &mihndl);

    EXEC_RG(sp_parser_hndl_init(
        &phndl, in, p_parsc, mod_iter_cb_prop, mod_iter_cb_scope, &ihndl));
    mihndl.eol_typ = phndl.lex.eol_typ;

    EXEC_RG(sp_parse(&phndl));

    /* last update */
    EXEC_RG(deferred_update(&ihndl, NULL, 1));

    /* copy untouched last part of the input */
    EXEC_RG(cpy_to_out(&ihndl, (p_parsc ? p_parsc->end+1 : EOF)));

finish:
    if (sc_out) fclose(sc_out);
    return ret;
}

#undef __POST_CB_ITER_CALL
#undef __WRITE_ENDSC_MRK
#undef __GET_ENCSC_BDY
#undef __IS_ON_EOL
#undef __IS_ON_GLOBSC
#undef __IS_ENTDEF_SET
