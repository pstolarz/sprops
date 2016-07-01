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
    SPEC_FOPEN_ERR,     /* file open error */
    SPEC_INV_PATH,      /* invalid path/prop name specification syntax */
    SPEC_NOTFOUND,      /* addressed element not found */
    SPEC_SIZE,          /* size exceeding a limit */
    SPEC_VAL_ERR,       /* incorrect value (e.g. number format) */
    SPEC_CB_RET_ERR,    /* incorrect return provided by a callback */

    /* This is the last error threshold used by the library, callbacks
       may use this threshold as a base number for their own error codes.
     */
    SPEC_CB_BASE = 100
} sp_errc_t;

/* The following codes specify additional information related to syntax error
   (SPEC_SYNTAX).
 */
typedef enum _sp_errsyn_t
{
    SPSYN_GRAMMAR = 0,  /* Grammar error */
    SPSYN_UNEXP_EOL,    /* Unexpected EOL,
                           token has not been properly finished */
    SPSYN_UNEXP_EOF,    /* Unexpected EOF,
                           token has not been properly finished */
    SPSYN_EMPTY_TKN,    /* Empty token error */
    SPSYN_LEV_DEPTH     /* Allowed level depth exceeded */
} sp_errsyn_t;

/* EOL types */
typedef enum _sp_eol_t {
    EOL_PLAT=0,     /* compilation platform specific */
    EOL_LF,         /* unix */
    EOL_CRLF,       /* win */
    EOL_CR          /* legacy mac */
} sp_eol_t;

/* parsing location */
typedef struct _sp_loc_t {
    /* stream offsets */
    long beg;
    long end;   /* inclusive */
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

/* Check syntax of a properties set read from an input 'in' (the file must be
   opened in the binary mode with read access at least) with a given parsing
   scope 'p_parsc'. In case of syntax error (SPEC_SYNTAX) 'p_line', 'p_col'
   and 'p_syn_code' will be provided with location of the error and detailed
   informational code.
 */
sp_errc_t sp_check_syntax(FILE *in, const sp_loc_t *p_parsc,
    int *p_line, int *p_col, sp_errsyn_t *p_syn_code);

/* Macro calculating actual length occupied by a given location */
#define sp_loc_len(loc) (!(loc) ? 0L : ((loc)->end-(loc)->beg+1))

/* Property iteration callback provides name and value of an iterated property
   (NULL terminated strings under 'name', 'val') with their token specific info
   located under 'p_tkname' and 'p_tkval' (may be NULL for property w/o a value).
   'p_ldef' locates overall property definition. 'arg' is passed untouched as
   provided in sp_iterate(). 'in' is the parsed input handle.

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
   'p_lbdyenc' points to the scope body content with enclosing brackets location
   (therefore always !=NULL); external chars of the location are '{','}' OR
   consist of single ';'. 'in' is the parsed input handle.

   Return codes:
       SPEC_CB_FINISH: success; finish parsing
       SPEC_SUCCESS: success; continue parsing
       >0 error codes: failure with code as returned; abort parsing
 */
typedef sp_errc_t (*sp_cb_scope_t)(void *arg, FILE *in, const char *type,
    const sp_tkn_info_t *p_tktype, const char *name, const sp_tkn_info_t *p_tkname,
    const sp_loc_t *p_lbody, const sp_loc_t *p_lbdyenc, const sp_loc_t *p_ldef);

/* Iterate elements (properties/scopes) under 'path'. This functions acts
   similarly to low level grammar parser's sp_parse() function, informing the
   caller about property's/scope's grammar tokens location and content. The
   principal difference is sp_iterate() allows to address specific location the
   iteration should take place.

   The path is defined as: [/][TYPE][:]NAME/[TYPE][:]NAME/...
   where TYPE and NAME specify scope type and name on a given path level with ':'
   character as a separator. To address untyped scope /:NAME/ shall be used.
   In case ':' is not provided 'deftp' is used as the default scope type, in
   which case /NAME/ is translated to /deftp:NAME/. As a conclusion: if 'deftp'
   is NULL or "", /NAME/ is translated to /:NAME/, that is, it provides an
   alternative way to address untyped scopes. To address global scope (0-level)
   'path' shall be set to NULL, "" or "/".

   For split scopes there is possible to provide specific split-scope index
   (0-based) where the iteration shall occur, by appending "@n" to the scope
   name in the NAME token. "@*" names overall (combined) scope and is assumed if
   no @-addressing is provided in NAME. "@$" denotes last split-scope index
   (usage of this construct shall be avoided due to additional overhead needed
   for tracking the scope in question).

   NOTE: Both TYPE and NAME may contain escape characters. Primary usage of them
   is escaping ':', '/'  and '@' in the 'path' string to avoid ambiguity with
   the path specific characters.

   'in' and 'p_parsc' provide input (the file must be opened in the binary mode
   with read access at least) to parse with a given parsing scope. The parsing
   scope may be used only inside a scope callback handler to retrieve inner
   scope body content.

   'cb_prop' and 'cb_scope' specify property and scope callbacks. The callbacks
   are provided with strings (property name/vale, scope type/name) written under
   buffers 'buf1' (of 'b1len') and 'buf2' (of 'b2len').
 */
sp_errc_t sp_iterate(FILE *in, const sp_loc_t *p_parsc, const char *path,
    const char *deftp, sp_cb_prop_t cb_prop, sp_cb_scope_t cb_scope,
    void *arg, char *buf1, size_t b1len, char *buf2, size_t b2len);

typedef struct _sp_prop_info_ex_t
{
    sp_tkn_info_t tkname;       /* property name token info */

    int val_pres;               /* if !=0: property value is present */
    sp_tkn_info_t tkval;        /* property value token info;
                                   valid only if val_pres is set */

    sp_loc_t ldef;              /* property definition location */

    /* values of the members below depend on
       a type of containing scope used in the
       query (e.g. split scope vs its component)
     */
    int ind;                    /* property index */
    int n_elem;                 /* number of elements before the element */
} sp_prop_info_ex_t;

typedef struct _sp_scope_info_ex_t
{
    int type_pres;               /* if !=0: scope type is present */
    sp_tkn_info_t tktype;       /* scope type token info;
                                   valid only if type_pres is set */

    sp_tkn_info_t tkname;       /* scope name token info */

    int body_pres;              /* if !=0: scope body is present */
    sp_loc_t lbody;             /* scope body; valid only if body_pres is set */

    sp_loc_t lbdyenc;           /* scope body content with enclosing brackets */

    sp_loc_t ldef;              /* scope definition location */

    /* values of the members below depend on
       a type of containing scope used in the
       query (e.g. split scope vs its component)
     */
    int ind;                    /* split-scope index */
    int n_elem;                 /* number of elements before the element */
} sp_scope_info_ex_t;

#define SP_IND_LAST     -1
#define SP_IND_ALL      -2

#define SP_ELM_LAST     SP_IND_LAST

/* Find property with 'name' and write its value to a buffer 'val' of length
   'len'. 'path' and 'deftp' specify owning scope of the property. If no
   property is found SPEC_NOTFOUND error is returned. 'ind' specifies a property
   index used to avoid ambiguity in case many properties with the same name
   exist: 0 is the 1st occurrence of a prop with specified name, 1 - 2nd...,
   SP_IND_LAST - the last one. If 'p_info' is not NULL it will be filled with
   property extra information.

   NOTE: The input file must be opened in the binary mode with read access at
   least.
 */
sp_errc_t sp_get_prop(FILE *in, const sp_loc_t *p_parsc, const char *name,
    int ind, const char *path, const char *deftp, char *val, size_t len,
    sp_prop_info_ex_t *p_info);

/* Find integer property with 'name' and write its under 'p_val'. In case of
   integer format error SPEC_VAL_ERR is returned.

   NOTE: This method is a simple wrapper around sp_get_prop() to treat
   property's value as integer.
 */
sp_errc_t sp_get_prop_int(FILE *in, const sp_loc_t *p_parsc, const char *name,
    int ind, const char *path, const char *deftp, long *p_val,
    sp_prop_info_ex_t *p_info);

/* Find float property with 'name' and write its under 'p_val'. In case of
   float format error SPEC_VAL_ERR is returned.

   NOTE: This method is a simple wrapper around sp_get_prop() to treat
   property's value as float.
 */
sp_errc_t sp_get_prop_float(FILE *in, const sp_loc_t *p_parsc, const char *name,
    int ind, const char *path, const char *deftp, double *p_val,
    sp_prop_info_ex_t *p_info);

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
   SPEC_SIZE error is returned. If read property value doesn't match any of the
   names in 'p_evals' OR the working buffer is to small to read a checked
   property, SPEC_VAL_ERR error is returned.

   NOTE: This method is a simple wrapper around sp_get_prop() to treat
   property's value as enum.
 */
sp_errc_t sp_get_prop_enum(
    FILE *in, const sp_loc_t *p_parsc, const char *name, int ind,
    const char *path, const char *deftp, const sp_enumval_t *p_evals,
    int igncase, char *buf, size_t blen, int *p_val, sp_prop_info_ex_t *p_info);

/* Find scope with 'name' and 'type' and write its detailed info under 'p_info'.
   'path' and 'deftp' specify owning scope of the requested scope. If no scope
   is found SPEC_NOTFOUND error is returned. The 'ind' argument enables to
   specify a split-scope index (SP_IND_LAST - the last one).

   NOTE: One of most useful members of 'sp_scope_info_ex_t' is 'lbody', allowing
   its further usage as a parsing scope in other functions of the API.
 */
sp_errc_t sp_get_scope_info(
    FILE *in, const sp_loc_t *p_parsc, const char *type, const char *name,
    int ind, const char *path, const char *deftp, sp_scope_info_ex_t *p_info);

/*
 * Flags specification
 */

/* Use n (1-8) spaces as indent; if 0 - single tab is used.
 */
#define SP_F_SPIND(n)       ((unsigned long)(n)>8 ? 8UL : (unsigned long)(n))

/* Use single tab as indent; SP_F_SPIND(0) acronym.
 */
#define SP_F_TBIND          SP_F_SPIND(0)

/* internal use only */
#define SP_F_GET_SPIND(f)   ((unsigned long)(f) & 0x0f)

/* Keep opening bracket of a scope body in separate line rather than in-lined
   with the scope header. That is:

   scope
   {
   }

    RATHER THAN

   scope {
   }
 */
#define SP_F_SPLBRA     0x0010UL

/* Use compact version for an empty scope definition. That is:

   scope {}

    OR

   scope
   {}

    OR

   type scope;

    RATHER THAN

   scope {
   }

    OR

   scope
   {
   }
 */
#define SP_F_EMPCPT     0x0020UL

/* Don't add extra surrounding spaces around '=' char while adding a value to a
   property. That is:

   prop=val

     RATHER THAN

   prop = val

   This flag is ignored if not configured with CONFIG_CUT_VAL_LEADING_SPACES,
   since in this case surrounding spaces for an added value are never placed.
 */
#define SP_F_NVSRSP     0x0040UL

/* Element addition - put an extra EOL after the inserted element,
   Element removal - delete an extra EOL (if exists) after the removed element.
 */
#define SP_F_EXTEOL     0x0080UL

/* If adding  element on the global scope's end, don't put an extra EOL after
   it. Effectively, the element will be finished by EOF.
 */
#define SP_F_NLSTEOL    0x0100UL

/* If property being set doesn't exist, don't add it to the destination scope.

   In case the flag is not specified the new property will be added as the last
   one in the destination scope if the property index being set ('ind') is:
    - SP_IND_LAST
    - SP_IND_ALL
    - equal to the number of properties with the same name as the set one.
 */
#define SP_F_NOADD      0x0200UL


/* Add (insert) a property of 'name' with value 'val' (may be NULL for a prop
   w/o a value) in location 'n_elem' (number of elements - scopes/props, before
   inserted property) in a scope addressed by 'p_parsc', 'path' and 'deftp'.

   If 'n_elem' is SP_ELM_LAST, the property is added as the last one in the
   scope (that is after the last element). If modified scope is empty SP_ELM_LAST
   is equivalent to 0.
   If 'n_elem' is 0, the property is added as the first one in the scope. For
   split scopes the first component scope is modified (even if empty).

   Additional 'flags' may be used to tune performed formatting (SP_F_SPIND,
   SP_F_SPLBRA, SP_F_EMPCPT, SP_F_EXTEOL, SP_F_NLSTEOL).

   NOTE 1: If 'p_parsc' is used as a constraint of performed modification, the
   output is confined only to the location specified by the argument. Usage of
   this argument is reserved for special purposes like modification of many
   scopes performed during single-shot iteration of their containing scope OR
   a performance efficient, small block updates inside a large input file (for
   this case transactional API is recommended to be used). For the global scope
   (most typical use case) 'p_parsc' shall be NULL.
   NOTE 2: There is possible to use usual @-addressing in the 'path'
   specification to reach a specific component scope inside its split scope.
   NOTE 3: Contrary to 'in' which is a random access stream for every API of
   the library (therefore must not be 'stdin'), 'out' is written incrementally
   by any updating function, w/o changing stream's position indicator (fseek())
   during the writing process. This enables 'stdout' to be used as 'out'.
 */
sp_errc_t sp_add_prop(FILE *in, FILE *out, const sp_loc_t *p_parsc,
    const char *name, const char *val, int n_elem, const char *path,
    const char *deftp, unsigned long flags);

/* Add (insert) an empty scope of 'name' and 'type' (may be NULL for a scope w/o
   a type) in location 'n_elem' (number of elements - scopes/props before
   inserted scope) in a scope addressed by 'p_parsc', 'path' and 'deftp'. The
   added scope may be later populated by sp_add_prop() and sp_add_scope().
   Additional 'flags' may be used to tune performed formatting (SP_F_SPIND,
   SP_F_SPLBRA, SP_F_EMPCPT, SP_F_EXTEOL, SP_F_NLSTEOL).

   See sp_add_prop() for more details.
 */
sp_errc_t sp_add_scope(FILE *in, FILE *out, const sp_loc_t *p_parsc,
    const char *type, const char *name, int n_elem, const char *path,
    const char *deftp, unsigned long flags);

/* Remove a property of 'name' with index 'ind' in a scope addressed by
   'p_parsc', 'path' and 'deftp'. Additional 'flags' may be used to tune
   performed removal (SP_F_EXTEOL).

   NOTE 1: 'ind' me be set to SP_IND_ALL or SP_IND_LAST to remove all/last
   property identified by 'name'.
   NOTE 2: The function returns SPEC_NOTFOUND if the destination scope is not
   found. Nonetheless, the input is copied w/o any changes to the output in this
   case (so, this is more a warning than an error). If the destination scope has
   been found but the property is absent, the function returns SPEC_SUCCESS and
   input is copied w/o any changes to the output.

   See also sp_add_prop() notes.
 */
sp_errc_t sp_rm_prop(FILE *in, FILE *out, const sp_loc_t *p_parsc,
    const char *name, int ind, const char *path, const char *deftp,
    unsigned long flags);

/* Remove a scope of 'name' and 'type' in a scope addressed by 'p_parsc',
   'path' and 'deftp'. The split-scope index 'ind' enables to specify which part
   of a split scope to remove (SP_IND_ALL - to remove all component scopes
   constituting the split scope, SP_IND_LAST - remove the last one).
   Additional 'flags' may be used to tune performed removal (SP_F_EXTEOL).

   See sp_rm_prop() for more details.
 */
sp_errc_t sp_rm_scope(FILE *in, FILE *out, const sp_loc_t *p_parsc,
    const char *type, const char *name, int ind, const char *path,
    const char *deftp, unsigned long flags);

/* Set value 'val' (may be NULL to remove the value) to a property with 'name'
   and index 'ind' in a scope addressed by 'p_parsc', 'path' and 'deftp'. If
   the destination scope exists but the property being set is absent there,
   the function may add the property to the scope if SP_F_NOADD flag is not
   specified in 'flags' (see SP_F_NOADD flag description for more details).

   NOTE 1: 'ind' me be set to SP_IND_ALL or SP_IND_LAST to set all/last property
   identified by 'name'.
   NOTE 2: 'flags' may contain additional flags controlling the addition of the
   property as for sp_add_prop(). Of course, they have only sense if SP_F_NOADD
   is not specified.
   NOTE 3: The function returns SPEC_NOTFOUND if the destination scope is not
   found OR the property being set is absent in the scope and is not possible
   (or allowed) to be added.

   See also sp_add_prop() notes.
 */
sp_errc_t sp_set_prop(FILE *in, FILE *out, const sp_loc_t *p_parsc,
    const char *name, const char *val, int ind, const char *path,
    const char *deftp, unsigned long flags);

/* Move (rename) to 'new_name' a property with 'name' and index 'ind' in a scope
   addressed by 'p_parsc', 'path' and 'deftp'.

   NOTE 1: 'ind' me be set to SP_IND_ALL or SP_IND_LAST to move all/last
   property identified by 'name'.
   NOTE 2: The function returns SPEC_NOTFOUND if the moved property is not found.
   NOTE 3: 'flags' are ignored (reserved for the future).

   See also sp_add_prop() notes.
 */
sp_errc_t sp_mv_prop(FILE *in, FILE *out, const sp_loc_t *p_parsc,
    const char *name, const char *new_name, int ind, const char *path,
    const char *deftp, unsigned long flags);

/* Move (rename) to 'new_type' (may be NULL to remove the type) and 'new_name'
   a scope with 'name' and the split-scope index 'ind' in a scope addressed by
   'p_parsc', 'path' and 'deftp'.

   See sp_mv_prop() for more details.
 */
sp_errc_t sp_mv_scope(
    FILE *in, FILE *out, const sp_loc_t *p_parsc, const char *type,
    const char *name, const char *new_type, const char *new_name, int ind,
    const char *path, const char *deftp, unsigned long flags);

/* Copies 'in' stream bytes to 'out' stream starting from 'beg' up to 'end'
   (exclusive). If 'end' is EOF: copies up to the end of input. If 'p_n' is not
   NULL it will get number of copied bytes.

   NOTE: Although not part of the main set of the library API, this auxiliary
   function may be useful when playing with parsing scope (p_parsc) in write
   access API, to restore modified input after the scope modification is
   finished. See transactional API implementation for an example of usage.
 */
sp_errc_t sp_cpy_to_out(FILE *in, FILE *out, long beg, long end, long *p_n);

#ifdef __cplusplus
}
#endif

#endif  /* __SP_PROPS_H__ */
