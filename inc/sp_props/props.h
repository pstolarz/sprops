/*
   Copyright (c) 2015 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __SP_PROPS_H__
#define __SP_PROPS_H__

#include <stdio.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NULL
#define NULL    0
#endif

/* error codes */
typedef enum _sp_errc_t
{
    SPEC_SUCCESS = 0,

    SPEC_INV_ARG,       /* invalid argument */
    SPEC_NOMEM,         /* no memory */
    SPEC_SYNTAX,        /* grammar syntax error */
    SPEC_ACCS_ERR,      /* stream access error */
    SPEC_INV_PATH,      /* invalid path specification syntax */
    SPEC_NOTFOUND,      /* addressed element not found */
    SPEC_SIZE,          /* size exceeding a limit */
    SPEC_VAL_ERR,       /* incorrect value (e.g. number format) */
    SPEC_NONAME,        /* name expected but not provided */
    SPEC_CB_BASE = 20   /* this is the last error in the enum, callbacks may
                           use this error as a base number to define more error
                           identifiers */
} sp_errc_t;

/* parsing location */
typedef struct _sp_loc_t {
    /* stream offsets */
    long beg;
    long end;
    /* text based location */
    int first_line;
    int first_column;
    int last_line;
    int last_column;
} sp_loc_t;

typedef struct _sp_tkn_info_t
{
    long len;       /* token content length (in the stream) */
    sp_loc_t loc;   /* location in the stream */
} sp_tkn_info_t;

/* Check syntax of a properties set read from an input file 'in' within
   a given parsing scope 'p_parsc'. In case of syntax error (SPEC_SYNTAX)
   'p_line', 'p_col' will be provided with location of the error.
 */
sp_errc_t sp_check_syntax(
    FILE *in, const sp_loc_t *p_parsc, int *p_line, int *p_col);

/* Macro calculating actual length occupied by a given location */
#define sp_loc_len(loc) ((!loc) ? 0L : ((loc)->end-(loc)->beg+1))

/* Property iteration callback provides name and value of an iterated property
   (NULL terminated strings under 'name', 'val') with their token specific info
   located under 'p_tkname' and 'p_tkval' (may be NULL for property w/o value).
   'p_ldef' locates overall property definition. 'arg' is passed untouched as
   provided in sp_iterate().

   Callback return codes:
   0<: success; finish parsing
    0 (SPEC_SUCCESS): continue parsing
   >0: failure with code as returned; abort parsing
 */
typedef int (*sp_cb_prop_t)(void *arg, FILE *in, const char *name,
    const sp_tkn_info_t *p_tkname, const char *val, const sp_tkn_info_t *p_tkval,
    const sp_loc_t *p_ldef);

/* Scope iteration callback provides type and scope name (strings under 'type',
   'name') of an iterated scope. 'p_lbody' points to scope body content location
   which may be used to retrieve data from it by sp_iterate(). 'p_tktype' and 'p_lbody'
   may be NULL for untyped scope OR scope w/o body).
 */
typedef int (*sp_cb_scope_t)(void *arg, FILE *in, const char *type,
    const sp_tkn_info_t *p_tktype, const char *name, const sp_tkn_info_t *p_tkname,
    const sp_loc_t *p_lbody, const sp_loc_t *p_ldef);

/* Iterate elements (properties/scopes) under 'path'.

   The path is defined as: [/][id1][:]id2/[id1][:]id2/...
   where id1 and id2 specify scope type and name on a given path level with
   colon char ':' as a separator. To address untyped scope /:id/ shall be used.
   In case no ':' is provided 'defsc' is used as the default scope type, in
   which case /id/ is translated to /defsc:id/. As a conclusion: if 'defsc'
   is "" the /id/ is translated to /:id/, that is, it provides an alternative
   way to address untyped scopes. To address root scope (0-level) 'path' shall
   be set to NULL, "" or "/".

   NOTE: id specification may contain escape characters. Primary usage of them
   is escaping colon (: as \x3a) and slash (/ as \x2f) in the 'path' string
   to avoid ambiguity.

   'in' and 'p_parsc' provides input file to parse within a given parsing scope.
   The parsing scope shall be used only inside scope callback handler to retrieve
   inner scope content, in all other case 'p_parsc' should be NULL.

   'cb_prop' and 'cb_scope' specify property and scope callbacks. The callbacks
   are provided with strings (property name/vale, scope type/name) written under
   buffers 'buf1' (of 'b1len') and 'buf2' (of 'b2len').
 */
sp_errc_t sp_iterate(FILE *in, const sp_loc_t *p_parsc, const char *path,
    const char *defsc, sp_cb_prop_t cb_prop, sp_cb_scope_t cb_scope,
    void *arg, char *p_buf1, size_t b1len, char *p_buf2, size_t b2len);

typedef struct _sp_prop_info_ex_t
{
    sp_tkn_info_t tkname;       /* property name token info */
    int val_pres;               /* if !=0: property value is present */
    sp_tkn_info_t tkval;        /* property value token info;
                                   valid only if val_pres is set */
    sp_loc_t ldef;              /* property definition location */
} sp_prop_info_ex_t;

/* Find property with 'name' and write its value to buffer 'p_val' of length
   'len'. 'path' and 'defsc' specify owning scope of the property. If no property
   is found SPEC_NOTFOUND is returned. In case many properties with the same
   name exist the first one is retrieved. If 'p_info' is not NULL it will be
   filled with property extra information.

   NOTE 1: 'name' may contain escape characters but contrary to 'path'
   specification colon and slash chars need not to be escaped.
   NOTE 2: if 'name' is NULL then the param name is provided as part of 'path'
   specification (last part of path after '/' char). In this case parameter
   name must not contain '/' or ':' characters which are not escaped by \xYY
   sequence.
 */
sp_errc_t sp_get_prop(FILE *in, const sp_loc_t *p_parsc, const char *name,
    const char *path, const char *defsc, char *p_val, size_t len,
    sp_prop_info_ex_t *p_info);

/* Find integer property with 'name' and write its under 'p_val'. In case of
   integer format error SPEC_VAL_ERR is returned.
 */
sp_errc_t sp_get_prop_int(FILE *in, const sp_loc_t *p_parsc, const char *name,
    const char *path, const char *defsc, long *p_val, sp_prop_info_ex_t *p_info);

typedef struct _sp_enumval_t
{
    const char *name;
    int val;
} sp_enumval_t;

/* Find enumeration property with 'name' and write its under 'p_val'. Matching
   enumeration names to their values is done via 'p_evals' table with last
   element filled with zeroes. The matching is case insensitive if 'igncase' is
   !=0. To avoid memory allocation the caller must provide working buffer 'p_buf'
   of length 'blen' to store enum names read from the stream. Length of the
   buffer must be at least as long as longest enum name + 1; in other case
   SPEC_SIZE is returned. If read property vale doesn't match any of the names
   in 'p_evals' SPEC_VAL_ERR is returned.
 */
sp_errc_t sp_get_prop_enum(FILE *in, const sp_loc_t *p_parsc, const char *name,
    const char *path, const char *defsc, const sp_enumval_t *p_evals, int igncase,
    char *p_buf, size_t blen, int *p_val, sp_prop_info_ex_t *p_info);

#ifdef __cplusplus
}
#endif

#endif  /* __SP_PROPS_H__ */
