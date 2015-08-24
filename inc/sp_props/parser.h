/*
   Copyright (c) 2015 Piotr Stolarz
   Scoped properties configuration library

   Distributed under the 2-clause BSD License (the License)
   see accompanying file LICENSE for details.

   This software is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the License for more information.
 */

#ifndef __SP_PARSER_H__
#define __SP_PARSER_H__

#include "sp_props/props.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _sp_parser_token_t
{
    SP_TKN_ID = 258,
    SP_TKN_VAL
} sp_parser_token_t;

struct _sp_parser_hndl_t;

/* Property callback provides location of property name ('p_lname'), value
   ('p_lval'; may be NULL for property w/o value) and overall definition of the
   property (p_ldef).

   Callback return codes:
   SPEC_CB_FINISH: success; finish further parsing
   SPEC_SUCCESS: success; continue parsing
   >0 error codes: failure with code as returned; abort parsing
 */
typedef sp_errc_t (*sp_parser_cb_prop_t)(const struct _sp_parser_hndl_t *p_hndl,
    const sp_loc_t *p_lname, const sp_loc_t *p_lval, const sp_loc_t *p_ldef);

/* Scope callback provides location of scope's type ('p_ltype'; may be NULL
   for scope w/o type), name ('p_lname'), body ('p_lbody'; may be NULL for scope
   w/o a body) and overall definition of the scope ('p_ldef').
 */
typedef sp_errc_t (*sp_parser_cb_scope_t)(const struct _sp_parser_hndl_t *p_hndl,
    const sp_loc_t *p_ltype, const sp_loc_t *p_lname, const sp_loc_t *p_lbody,
    const sp_loc_t *p_ldef);

/* types of supported EOLs */
typedef enum _eol_t {
    EOL_UNDEF=0,    /* undefined, no new line occurs */
    EOL_LF,         /* unix */
    EOL_CRLF,       /* win */
    EOL_CR          /* legacy mac */
} eol_t;

typedef
struct _unc_cache_t
{
    size_t inbuf;   /* number of cached chars */
    int buf[16];    /* cache buffer */
} unc_cache_t;

typedef struct _sp_parser_hndl_t
{
    /* parsed input stream */
    FILE *in;

    struct {
        /* type of EOL detected on the input */
        eol_t eol_typ;

        /* next char to read */
        int line;
        int col;
        long off;       /* stream offset */

        /* last stream offset to parse; -1: stream end */
        long end;

        /* currently scope level (0-based) */
        int scope_lev;

        /* lexical context */
        int ctx;

        /* unget stream chars cache */
        unc_cache_t unc;
    } lex;

    struct {
        /* argument passed untouched */
        void *arg;

        /* parser callbacks */
        sp_parser_cb_prop_t prop;
        sp_parser_cb_scope_t scope;
    } cb;

    struct {
        sp_errc_t code;
        /* syntax error (STEC_SYNTAX) location */
        struct {
            int line;
            int col;
        } loc;
    } err;
} sp_parser_hndl_t;

/* Initialize parser handle under 'p_hndl' for an input file to parse with handle
   'in'  (the file must be opened in the binary mode with read access at least).
   Parsing scope is constrained to 'p_parsc' (if NULL: the entire file).
   Property/scope callbacks are provided by 'cb_prop' and 'cb_scope' respectively
   with caller specific argument passed untouched to these functions ('cb_arg').
 */
sp_errc_t sp_parser_hndl_init(sp_parser_hndl_t *p_hndl,
    FILE *in, const sp_loc_t *p_parsc, sp_parser_cb_prop_t cb_prop,
    sp_parser_cb_scope_t cb_scope, void *cb_arg);

/* Parser method */
sp_errc_t sp_parse(sp_parser_hndl_t *p_hndl);

/* Copy a token of type 'tkn' from location 'p_loc' into buffer 'p_buf' with
   length as set in 'buf_len'. If there is enough of space the copied string is
   NULL terminated. If 'p_tklen' is not NULL it will be provided with token's
   content length occupied in the stream.
 */
sp_errc_t sp_parser_tkn_cpy(
    const sp_parser_hndl_t *p_phndl, sp_parser_token_t tkn,
    const sp_loc_t *p_loc, char *p_buf, size_t buf_len, long *p_tklen);

/* Compare a token of type 'tkn' from location 'p_loc' with string 'str'.
   'max_num' specifies maximum number of 'str' chars to compare. In case of
   success (SPEC_SUCCESS) the function sets 'p_equ' to the comparison result -
   1: equal, 0: not equal.
 */
sp_errc_t sp_parser_tkn_cmp(
    const sp_parser_hndl_t *p_phndl, sp_parser_token_t tkn,
    const sp_loc_t *p_loc, const char *str, size_t max_num, int *p_equ);

/* Tokenize string 'str' into token of type 'tkn' and write it to the file 'out'
   (must be opened in the binary mode with write access).
 */
sp_errc_t sp_parser_tokenize_str(
    FILE *out, sp_parser_token_t tkn, const char *str);

#ifdef __cplusplus
}
#endif

#endif  /* __SP_PARSER_H__ */
