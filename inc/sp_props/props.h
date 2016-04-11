/*
   Copyright (c) 2015,2016 Piotr Stolarz
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

/* Error codes */
typedef enum _sp_errc_t
{
    /* Negative codes are reserved for callbacks to inform the library how to
       further proceed with the handling process; the codes don't name failures.
     */
    SPEC_CB_FINISH = -1,

    /* Success (always 0)
     */
    SPEC_SUCCESS = 0,

    /* Failures (positive code)
     */
    SPEC_INV_ARG,       /* invalid argument */
    SPEC_NOMEM,         /* no memory */
    SPEC_SYNTAX,        /* grammar syntax error */
    SPEC_ACCS_ERR,      /* stream access error */
    SPEC_INV_PATH,      /* invalid path specification syntax */
    SPEC_NOTFOUND,      /* addressed element not found */
    SPEC_SIZE,          /* size exceeding a limit */
    SPEC_VAL_ERR,       /* incorrect value (e.g. number format) */
    SPEC_CB_RET_ERR,    /* incorrect return provided by a callback */

    /* This is the last error in the enumeration, callbacks may
       use this error as a base number to define more failures.
     */
    SPEC_CB_BASE = 20
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
    /* token content length in the stream (de-escaped)
       NOTE: This length may be lower than the actual length of the token
       occupied in the stream as returned by sp_loc_len() due to escaping chars
       sequences.
     */
    long len;

    /* location in the stream */
    sp_loc_t loc;
} sp_tkn_info_t;

/* Check syntax of a properties set read from an input file 'in' (must be opened
   in the binary mode with read access at least) with a given parsing scope
   'p_parsc'. In case of syntax error (SPEC_SYNTAX) 'p_line', 'p_col' will be
   provided with location of the error.
 */
sp_errc_t sp_check_syntax(
    FILE *in, const sp_loc_t *p_parsc, int *p_line, int *p_col);

/* Macro calculating actual length occupied by a given location */
#define sp_loc_len(loc) ((!loc) ? 0L : ((loc)->end-(loc)->beg+1))

/* Property iteration callback provides name and value of an iterated property
   (NULL terminated strings under 'name', 'val') with their token specific info
   located under 'p_tkname' and 'p_tkval' (may be NULL for property w/o a value).
   'p_ldef' locates overall property definition. 'arg' is passed untouched as
   provided in sp_iterate(). 'in' is the parsed input file handle.

   Return codes:
       SPEC_CB_FINISH: success; finish parsing
       SPEC_SUCCESS: success; continue parsing
       >0 error codes: failure with code as returned; abort parsing
 */
typedef sp_errc_t (*sp_cb_prop_t)(void *arg, FILE *in, const char *name,
    const sp_tkn_info_t *p_tkname, const char *val, const sp_tkn_info_t *p_tkval,
    const sp_loc_t *p_ldef);

/* Scope iteration callback provides type and scope name (strings under 'type',
   'name') of an iterated scope. 'p_lbody' points to the scope body content
   location which may be used to retrieve data from it by sp_iterate().
   'p_tktype' and 'p_lbody' may be NULL for untyped scope OR scope w/o a body).
   'in' is the parsed input file handle.

   Return codes:
       SPEC_CB_FINISH: success; finish parsing
       SPEC_SUCCESS: success; continue parsing
       >0 error codes: failure with code as returned; abort parsing
 */
typedef sp_errc_t (*sp_cb_scope_t)(void *arg, FILE *in, const char *type,
    const sp_tkn_info_t *p_tktype, const char *name, const sp_tkn_info_t *p_tkname,
    const sp_loc_t *p_lbody, const sp_loc_t *p_ldef);

/* Iterate elements (properties/scopes) under 'path'. This functions acts
   similarly to low level grammar parser's sp_parse() function, informing the
   caller about property's/scope's grammar tokens location and content. The
   principal difference is sp_iterate() allows to address specific location the
   iteration should take place.

   The path is defined as: [/][id1][:]id2/[id1][:]id2/...
   where id1 and id2 specify scope type and name on a given path level with
   colon char ':' as a separator. To address untyped scope /:id/ shall be used.
   In case no ':' is provided 'defsc' is used as the default scope type, in
   which case /id/ is translated to /defsc:id/. As a conclusion: if 'defsc'
   is "" the /id/ is translated to /:id/, that is, it provides an alternative
   way to address untyped scopes. To address global scope (0-level) 'path'
   shall be set to NULL, "" or "/".

   NOTE: id specification may contain escape characters. Primary usage of them
   is escaping colon (\x3a) and slash (\x2f) in the 'path' string to avoid
   ambiguity with the path specific characters.

   'in' and 'p_parsc' provide input file (must be opened in the binary mode with
   read access at least) to parse with a given parsing scope. The parsing scope
   may be used only inside a scope callback handler to retrieve inner scope body
   content.

   'cb_prop' and 'cb_scope' specify property and scope callbacks. The callbacks
   are provided with strings (property name/vale, scope type/name) written under
   buffers 'buf1' (of 'b1len') and 'buf2' (of 'b2len').
 */
sp_errc_t sp_iterate(FILE *in, const sp_loc_t *p_parsc, const char *path,
    const char *defsc, sp_cb_prop_t cb_prop, sp_cb_scope_t cb_scope,
    void *arg, char *buf1, size_t b1len, char *buf2, size_t b2len);

typedef struct _sp_prop_info_ex_t
{
    sp_tkn_info_t tkname;       /* property name token info */
    int val_pres;               /* if !=0: property value is present */
    sp_tkn_info_t tkval;        /* property value token info;
                                   valid only if val_pres is set */
    sp_loc_t ldef;              /* property definition location */
} sp_prop_info_ex_t;

/* Find property with 'name' and write its value to a buffer 'val' of length
   'len'. 'path' and 'defsc' specify owning scope of the property. If no property
   is found SPEC_NOTFOUND error is returned. In case many properties with the
   same name exist the first one is retrieved (if other behavior is required, use
   sp_iterate() method). If 'p_info' is not NULL it will be filled with property
   extra information.

   NOTE 1: 'name' may contain escape characters but contrary to 'path'
   specification colon and slash chars need not to be escaped (there is no
   ambiguity in this case).
   NOTE 2: if 'name' is NULL then the property name is provided as part of 'path'
   specification (last part of path after '/' char). In this case property
   name must not contain '/' or ':' characters which are not escaped by \xYY
   sequence.
   NOTE 3: 'in' input file must be opened in the binary mode with read access at
   least.
 */
sp_errc_t sp_get_prop(FILE *in, const sp_loc_t *p_parsc, const char *name,
    const char *path, const char *defsc, char *val, size_t len,
    sp_prop_info_ex_t *p_info);

/* Find integer property with 'name' and write its under 'p_val'. In case of
   integer format error SPEC_VAL_ERR is returned.

   NOTE: This method is a simple wrapper around sp_get_prop() to treat property's
   value as integer.
 */
sp_errc_t sp_get_prop_int(FILE *in, const sp_loc_t *p_parsc, const char *name,
    const char *path, const char *defsc, long *p_val, sp_prop_info_ex_t *p_info);

/* Find float property with 'name' and write its under 'p_val'. In case of
   float format error SPEC_VAL_ERR is returned.

   NOTE: This method is a simple wrapper around sp_get_prop() to treat property's
   value as float.
 */
sp_errc_t sp_get_prop_float(FILE *in, const sp_loc_t *p_parsc, const char *name,
    const char *path, const char *defsc, double *p_val, sp_prop_info_ex_t *p_info);

typedef struct _sp_enumval_t
{
    /* enumeration name; must not contain leading/trailing spaces */
    const char *name;

    /* enumeration value */
    int val;
} sp_enumval_t;

/* Find enumeration property with 'name' and write its under 'p_val'. Matching
   enumeration names to their values is done via 'p_evals' table with last
   element filled with zeros. The matching is case insensitive if 'igncase' is
   !=0. To avoid memory allocation the caller must provide working buffer 'buf'
   of length 'blen' to store enum names read from the stream. Length of the
   buffer must be at least as long as the longest enum name + 1; in other case
   SPEC_SIZE error is returned. If read property vale doesn't match any of the
   names in 'p_evals' OR the working buffer is to small to read a checked
   property SPEC_VAL_ERR error is returned.

   NOTE: This method is a simple wrapper around sp_get_prop() to treat property's
   value as enum.
 */
sp_errc_t sp_get_prop_enum(
    FILE *in, const sp_loc_t *p_parsc, const char *name, const char *path,
    const char *defsc, const sp_enumval_t *p_evals, int igncase, char *buf,
    size_t blen, int *p_val, sp_prop_info_ex_t *p_info);

#ifdef __cplusplus
}
#endif

#endif  /* __SP_PROPS_H__ */
