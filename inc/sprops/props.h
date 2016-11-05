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

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NULL
#define NULL    0
#endif

/* Error codes */
typedef enum _sp_errc_t
{
    /* Negative codes are reserved for callbacks to inform the library
       how to further proceed with the handling process. The codes don't
       mean failures.
     */
    SPEC_CB_FINISH = -1,    /* done, stop further processing (successfully) */

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

    /* This is the last library error threshold. Callbacks may use
       this threshold as a base number for their own error codes.
     */
    SPEC_CB_BASE = 100
} sp_errc_t;

/* The following codes specify additional information related to
   the syntax error (SPEC_SYNTAX).
 */
typedef enum _sp_syncode_t
{
    SPSYN_GRAMMAR = 1,  /* Grammar error */
    SPSYN_UNEXP_EOL,    /* Unexpected EOL,
                           token has not been properly finished */
    SPSYN_UNEXP_EOF,    /* Unexpected EOF,
                           token has not been properly finished */
    SPSYN_EMPTY_TKN,    /* Empty token error */
    SPSYN_LEV_DEPTH     /* Allowed level depth exceeded */
} sp_syncode_t;

/* syntax error related info */
typedef struct _sp_synerr_t
{
    sp_syncode_t code;

    /* syntax error location */
    struct {
        int line;
        int col;
    } loc;
} sp_synerr_t;

/* EOL types */
typedef enum _sp_eol_t {
    EOL_PLAT=0,     /* compilation platform specific */
    EOL_LF,         /* Unix */
    EOL_CRLF,       /* Win */
    EOL_CR          /* legacy MAC */
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

       NOTE: This length may be lower than the length of the token occupied in
       the stream as returned by sp_loc_len() due to escaping chars sequences.
     */
    long len;

    /* location in the stream */
    sp_loc_t loc;
} sp_tkn_info_t;

#define SP_FILE_C   0   /* ANSI C stream */
#define SP_FILE_MEM 1   /* memory buffer */

/* stream */
typedef struct _SP_FILE
{
    int typ;    /* stream type (SP_FILE_XXX) */

    union {
        /* SP_FILE_C */
        FILE *f;

        /* SP_FILE_MEM */
        struct {
            char *b;    /* stream buffer */
            size_t num; /* number of chars in the buffer */
            size_t i;   /* current index in the buffer (stream position) */
        } m;
    };
} SP_FILE;

#define SP_MODE_READ        "rb"
#define SP_MODE_WRITE_NEW   "wb+"

/* Open a file with 'filename' and fopen(3) 'mode'. Populate SP_FILE handle
   pointed by 'f' appropriately. The handle may be closed by sp_fclose().
   In case of error SPEC_FOPEN_ERR is returned and 'errno' may be checked
   against the problem root cause.

   NOTE: A file must be always opened in the binary mode with at least read
   access for input, and read/write access for output (if possible use
   SP_MODE_XXX predefined modes).
 */
sp_errc_t sp_fopen(SP_FILE *f, const char *filename, const char *mode);

/* Use already opened ANSI C stream (with a handle 'cf') to populate SP_FILE
   handle pointed by 'f'. The handle may be closed by sp_fclose().
   Always success if valid arguments are passed.

   NOTE: The file corresponding to 'cf' must be opened in the binary mode with
   at least read access for input, and read/write access for output.
 */
sp_errc_t sp_fopen2(SP_FILE *f, FILE *cf);

/* Open SP_FILE memory stream handle with a buffer 'buf' and available number
   of chars 'num'. Always success if valid arguments are passed. The opened
   handle need not to be closed.

   NOTE 1: While reading, the memory stream is constrained by the 'num' argument
   or a NULL termination char in the buffer (which first occurs). For writing,
   'num' specifies number of chars which may be written to the buffer.
   NOTE 2: The function may be called multiple times for the same buffer to
   reinitialize the handle with a new buffer length and/or to reset the stream
   position to the buffer start.
 */
sp_errc_t sp_mopen(SP_FILE *f, char *buf, size_t num);

/* If SP_FILE was opened as an ANSI C stream (by sp_fopen() or sp_fopen2()),
   this function merely calls fclose(3) to close it. In case of other stream
   type SPEC_INV_ARG is returned. If fclose(3) fails SPEC_ACCS_ERR is returned
   and 'errno' may be checked against the problem root cause.
 */
sp_errc_t sp_fclose(SP_FILE *f);

/* Check syntax of a properties set read from an input 'in' with a given parsing
   scope 'p_parsc'. In case of the syntax error (SPEC_SYNTAX) 'p_synerr' is
   filled with the error related info.
 */
sp_errc_t sp_check_syntax(
    SP_FILE *in, const sp_loc_t *p_parsc, sp_synerr_t *p_synerr);

/* Macro calculating length occupied by a given location */
#define sp_loc_len(loc) (!(loc) ? 0L : ((loc)->end-(loc)->beg+1))

/* Property iteration callback provides name and value of an iterated property
   (strings under 'name', 'val') with their token specific info located under
   'p_tkname' and 'p_tkval' (may be NULL for property w/o a value). 'p_ldef'
   locates overall property definition. 'arg' is passed untouched as provided
   in sp_iterate(). 'in' is the parsed input handle.

   Return codes:
       SPEC_CB_FINISH: success; finish parsing
       SPEC_SUCCESS: success; continue parsing
       >0 error codes: failure with code as returned; abort parsing
 */
typedef sp_errc_t (*sp_cb_prop_t)(void *arg, SP_FILE *in, const char *name,
    const sp_tkn_info_t *p_tkname, const char *val, const sp_tkn_info_t *p_tkval,
    const sp_loc_t *p_ldef);

/* Scope iteration callback provides type and scope name (strings under 'type',
   'name') of an iterated scope. 'p_lbody' points to the scope body content
   location which may be used to retrieve data from it by sp_iterate().
   'p_tktype' and 'p_lbody' may be NULL for untyped scope OR scope w/o a body).
   'p_lbdyenc' points to the scope body content with enclosing brackets location
   (therefore always !=NULL); external chars of the location are '{','}' OR
   consist of a single ';'. 'in' is the parsed input handle.

   Return codes:
       SPEC_CB_FINISH: success; finish parsing
       SPEC_SUCCESS: success; continue parsing
       >0 error codes: failure with code as returned; abort parsing
 */
typedef sp_errc_t (*sp_cb_scope_t)(void *arg, SP_FILE *in, const char *type,
    const sp_tkn_info_t *p_tktype, const char *name, const sp_tkn_info_t *p_tkname,
    const sp_loc_t *p_lbody, const sp_loc_t *p_lbdyenc, const sp_loc_t *p_ldef);

/* Iterate elements (properties/scopes) under 'path'. This functions acts
   similarly to the low level grammar parser's sp_parse() function, informing
   the caller about property's/scope's grammar tokens location and content. The
   principal difference is sp_iterate() allows to address a specific location
   the iteration should take place.

   The path is defined as: [/][TYPE][:]NAME/[TYPE][:]NAME/...
   where TYPE and NAME specify scope type and name on a given path level with
   ':' character as a separator. To address untyped scope /:NAME/ shall be used.
   In case ':' is not provided, 'deftp' is used as the default scope type, in
   which case /NAME/ is translated to /deftp:NAME/. As a conclusion: if 'deftp'
   is NULL or "", /NAME/ is translated to /:NAME/, that is, it provides an
   alternative way to address untyped scopes. To address the global scope
   (0-level) 'path' shall be set to NULL, "" or "/".

   For split scopes there is possible to provide specific split-scope index
   (0-based) where the iteration shall occur, by appending "@n" to the scope
   name in the NAME token. "@*" names overall (combined) scope and is assumed
   if no @-addressing is provided in NAME. "@$" denotes last split-scope index
   (usage of this construct shall be avoided due to additional overhead needed
   for tracking the scope in question).

   NOTE: Both TYPE and NAME may contain escape characters. Primary usage of them
   is escaping ':', '/'  and '@' in the 'path' string to avoid ambiguity with
   the path specific characters.

   'in' and 'p_parsc' provide input to parse with a given parsing scope.

   'cb_prop' and 'cb_scope' specify property and scope callbacks. The callbacks
   are provided with strings (property name/value, scope type/name) written
   under buffers 'buf1' (of 'b1len') and 'buf2' (of 'b2len').
 */
sp_errc_t sp_iterate(SP_FILE *in, const sp_loc_t *p_parsc, const char *path,
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
       a type of the containing scope used in the
       query (split scope vs its component)
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
       a type of the containing scope used in the
       query (split scope vs its component)
     */
    int ind;                    /* split-scope index */
    int n_elem;                 /* number of elements before the element */
} sp_scope_info_ex_t;

#define SP_IND_LAST     -1
#define SP_IND_ALL      -2

#define SP_ELM_LAST     SP_IND_LAST

/* Find property with 'name' and write its value to a buffer 'val' of length
   'len'. 'path' and 'deftp' specify the containing scope of the property. If no
   property is found SPEC_NOTFOUND error is returned. 'ind' specifies a property
   index used to avoid ambiguity in case many properties with the same name
   exist: 0 is the 1st occurrence of a prop with specified name, 1 - 2nd...,
   SP_IND_LAST - the last one. If 'p_info' is not NULL it will be filled with
   property extra information.
 */
sp_errc_t sp_get_prop(SP_FILE *in, const sp_loc_t *p_parsc, const char *name,
    int ind, const char *path, const char *deftp, char *val, size_t len,
    sp_prop_info_ex_t *p_info);

/* Find integer property with 'name' and write its value under 'p_val'. In case
   of string format problem SPEC_VAL_ERR error is returned.

   NOTE 1: This function is a simple wrapper around sp_get_prop() to treat
   property's value as an integer.
   NOTE 2: The function guarantees not to modify memory under 'p_val' in case
   of failure.
 */
sp_errc_t sp_get_prop_int(SP_FILE *in, const sp_loc_t *p_parsc,
    const char *name, int ind, const char *path, const char *deftp, long *p_val,
    sp_prop_info_ex_t *p_info);

/* Find float property with 'name' and write its value under 'p_val'. In case
   of string format problem SPEC_VAL_ERR error is returned.

   NOTE 1: This function is a simple wrapper around sp_get_prop() to treat
   property's value as a float.
   NOTE 2: The function guarantees not to modify memory under 'p_val' in case
   of failure.
 */
sp_errc_t sp_get_prop_float(SP_FILE *in, const sp_loc_t *p_parsc,
    const char *name, int ind, const char *path, const char *deftp,
    double *p_val, sp_prop_info_ex_t *p_info);

typedef struct _sp_enumval_t
{
    /* enumeration name; must not contain leading/trailing spaces */
    const char *name;

    /* enumeration value */
    int val;
} sp_enumval_t;

/* Find enumeration property with 'name' and write its value under 'p_val'.
   Matching enumeration names to their values is done via 'p_evals' table with
   the last element filled with zeros. The matching is case insensitive if
   'igncase' is !=0. To avoid memory allocation the caller must provide a
   working buffer 'buf' of length 'blen' to store enum names read from the
   stream. Length of the buffer must be at least as long as the longest enum
   name + 1; in other case SPEC_SIZE error is returned. If a read property
   name doesn't match any of the names in 'p_evals' OR the working buffer is
   too small to store a checked property, SPEC_VAL_ERR error is returned.

   NOTE 1: This function is a simple wrapper around sp_get_prop() to treat
   property's value as enum.
   NOTE 2: The function guarantees not to modify memory under 'p_val' in case
   of failure.
 */
sp_errc_t sp_get_prop_enum(
    SP_FILE *in, const sp_loc_t *p_parsc, const char *name, int ind,
    const char *path, const char *deftp, const sp_enumval_t *p_evals,
    int igncase, char *buf, size_t blen, int *p_val, sp_prop_info_ex_t *p_info);

/* Find scope with 'name' and 'type' and write its detailed info under 'p_info'.
   'path' and 'deftp' specify the containing scope of the requested scope. If no
   scope is found SPEC_NOTFOUND error is returned. The 'ind' argument enables to
   specify a split-scope index (SP_IND_LAST - the last one).

   NOTE: One of most useful members of 'sp_scope_info_ex_t' is 'lbody', allowing
   its further usage as a parsing scope in other functions of the API.
 */
sp_errc_t sp_get_scope_info(
    SP_FILE *in, const sp_loc_t *p_parsc, const char *type, const char *name,
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

/* Keep opening bracket of a scope body in a separate line rather than in-lined
   with the scope header. That is:

   scope
   {
   }

    RATHER THAN

   scope {
   }
 */
#define SP_F_SPLBRA     0x00000010UL

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
#define SP_F_EMPCPT     0x00000020UL

/* Don't add extra surrounding spaces around '=' char while adding a value to a
   property. That is:

   prop=val

     RATHER THAN

   prop = val

   This flag is ignored if not configured with CONFIG_CUT_VAL_LEADING_SPACES,
   since in this case surrounding spaces for an added value are never placed.
 */
#define SP_F_NVSRSP     0x00000040UL

/* Element addition - put an extra EOL after the inserted element,
   Element removal - delete an extra EOL (if exists) after the removed element.
 */
#define SP_F_EXTEOL     0x00000080UL

/* When adding an element after another one, this flags ensures an extra EOL
   before the newly added element and after the previous one.
   Primary usage of this flag is to use an extra EOL written by SP_F_EXTEOL
   for already added element while adding a new element after it.
 */
#define SP_F_EOLBFR     0x00000100UL

/* If adding an element on the global scope's end, don't put an extra EOL after
   it. Effectively, the element will be finished by EOF.

   NOTE: This flag is utilized by the transactional API and should not be used
   otherwise.
 */
#define SP_F_NLSTEOL    0x00000200UL

/* Use specific EOL (as for sp_eol_t enumeration) during modification process.
   If not specified, the same EOL is used as detected on the input (or
   compilation platform specific if no EOL is present).

   NOTE: This flag is utilized by the transactional API and should not be used
   otherwise.
 */
#define SP_F_USEEOL(eol)    ((((unsigned long)(eol)&3)+1)<<10)

/* internal use only */
#define SP_F_GET_USEEOL(f)  ((sp_eol_t)((((unsigned long)(f)>>10)&3)-1))

/* If property being set doesn't exist, don't add it to the destination scope.

   In case the flag is not specified the new property will be added as the last
   one in the destination scope if the property index being set ('ind') is:
    - SP_IND_LAST
    - SP_IND_ALL
    - equal to the number of properties with the same name as the one being set.
 */
#define SP_F_NOADD      0x00002000UL

/* When possible try to avoid semicolons. EOLs are used as parameter value
   terminators.
 */
#define SP_F_NOSEMC     0x00004000UL

/* Add (insert) a property of 'name' with value 'val' (may be NULL for a prop
   w/o a value) in location 'n_elem' (number of elements - scopes/props, before
   the inserted property) in a scope addressed by 'p_parsc', 'path' and 'deftp'.

   If 'n_elem' is SP_ELM_LAST, the property is added as the last one in the
   scope (that is after the last element). If a modified scope is empty,
   SP_ELM_LAST is equivalent to 0.
   If 'n_elem' is 0, the property is added as the first one in the scope. For
   split scopes the first component scope is modified (even if empty).

   Additional 'flags' may be used to tune performed formatting (SP_F_SPIND,
   SP_F_SPLBRA, SP_F_EMPCPT, SP_F_EXTEOL, SP_F_NLSTEOL).

   NOTE 1: If 'p_parsc' is used as a constraint of the performed modification,
   the output is confined only to the location specified by the argument. For
   the global scope 'p_parsc' shall be NULL (most typical use case). Basically,
   usage of this argument is reserved for the transactional API.
   NOTE 2: There is possible to use @-addressing in the 'path' specification
   to reach a specific component scope inside its split scope.
   NOTE 3: Contrary to 'in' which is a random access stream for every API of
   the library (therefore must not be 'stdin'), 'out' is written incrementally
   by any write access function, w/o changing the stream position (fseek(3))
   during the writing process. This enables 'stdout' to be used as 'out'.
 */
sp_errc_t sp_add_prop(SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc,
    const char *name, const char *val, int n_elem, const char *path,
    const char *deftp, unsigned long flags);

/* Add (insert) an empty scope of 'name' and 'type' (may be NULL for a scope w/o
   a type) in 'n_elem' location (that is a number of elements - scopes/props
   before the inserted scope) in a scope addressed by 'p_parsc', 'path' and
   'deftp'. The added scope may be later populated by sp_add_prop() and
   sp_add_scope(). Additional 'flags' may be used to tune performed formatting
   (SP_F_SPIND, SP_F_SPLBRA, SP_F_EMPCPT, SP_F_EXTEOL, SP_F_NLSTEOL).

   See sp_add_prop() for more details.
 */
sp_errc_t sp_add_scope(SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc,
    const char *type, const char *name, int n_elem, const char *path,
    const char *deftp, unsigned long flags);

/* Remove a property of 'name' with index 'ind' in a scope addressed by
   'p_parsc', 'path' and 'deftp'. Additional 'flags' may be used to tune
   performed removal (SP_F_EXTEOL).

   NOTE 1: 'ind' may be set to SP_IND_ALL or SP_IND_LAST to remove all/last
   property identified by 'name'.
   NOTE 2: The function returns SPEC_NOTFOUND if the destination scope is not
   found. Nonetheless, the input is copied w/o any changes to the output in this
   case (so, this is more a warning than an error). If the destination scope has
   been found but the property is absent, the function returns SPEC_SUCCESS and
   input is copied w/o any changes to the output.

   See also sp_add_prop() notes.
 */
sp_errc_t sp_rm_prop(SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc,
    const char *name, int ind, const char *path, const char *deftp,
    unsigned long flags);

/* Remove a scope of 'name' and 'type' in a scope addressed by 'p_parsc',
   'path' and 'deftp'. The split-scope index 'ind' enables to specify which part
   of a split scope to remove (SP_IND_ALL - to remove all component scopes
   constituting the split scope, SP_IND_LAST - remove the last one).
   Additional 'flags' may be used to tune performed removal (SP_F_EXTEOL).

   See sp_rm_prop() for more details.
 */
sp_errc_t sp_rm_scope(SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc,
    const char *type, const char *name, int ind, const char *path,
    const char *deftp, unsigned long flags);

/* Set value 'val' (may be NULL to remove the value) to a property with 'name'
   and index 'ind' in a scope addressed by 'p_parsc', 'path' and 'deftp'. If
   the destination scope exists but the property being set is absent there,
   the function may add the property to the scope if SP_F_NOADD flag is not
   specified in 'flags' (see SP_F_NOADD flag description for more details).

   NOTE 1: 'ind' may be set to SP_IND_ALL or SP_IND_LAST to set all/last property
   identified by 'name'.
   NOTE 2: 'flags' may contain additional flags controlling the addition of the
   property as for sp_add_prop(). Of course, they have only sense if SP_F_NOADD
   is not specified.
   NOTE 3: The function returns SPEC_NOTFOUND if the destination scope is not
   found OR the property being set is absent in the scope and is not possible
   (or allowed) to be added.

   See also sp_add_prop() notes.
 */
sp_errc_t sp_set_prop(SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc,
    const char *name, const char *val, int ind, const char *path,
    const char *deftp, unsigned long flags);

/* Move (rename) to 'new_name' a property with 'name' and index 'ind' in a scope
   addressed by 'p_parsc', 'path' and 'deftp'.

   NOTE 1: 'ind' may be set to SP_IND_ALL or SP_IND_LAST to move all/last
   property identified by 'name'.
   NOTE 2: The function returns SPEC_NOTFOUND if the moved property is not found.
   NOTE 3: 'flags' are ignored (reserved for the future).

   See also sp_add_prop() notes.
 */
sp_errc_t sp_mv_prop(SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc,
    const char *name, const char *new_name, int ind, const char *path,
    const char *deftp, unsigned long flags);

/* Move (rename) to 'new_type' (may be NULL to remove the type) and 'new_name'
   a scope with 'name' and the split-scope index 'ind' in a scope addressed by
   'p_parsc', 'path' and 'deftp'.

   See sp_mv_prop() for more details.
 */
sp_errc_t sp_mv_scope(
    SP_FILE *in, SP_FILE *out, const sp_loc_t *p_parsc, const char *type,
    const char *name, const char *new_type, const char *new_name, int ind,
    const char *path, const char *deftp, unsigned long flags);

#ifdef __cplusplus
}
#endif

#endif  /* __SP_PROPS_H__ */
